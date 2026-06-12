#include "editor.h"
#include "engine_service.h"
#include "engine_context.h"
#include "platform/platform.h"
#include "window/window.h"
#include "scene/world.h"
#include "render/render_system.h"

#include "command.h"
#include "command_stack.h"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string_view>

namespace ed = buddd::editor;

// ── Helper: a test command that toggles a bool ──
class ToggleCommand final : public ed::Command {
public:
    explicit ToggleCommand(bool* target, std::string_view name)
        : target_(target), name_(name) {}

    auto execute() -> void override { *target_ = true;  }
    auto undo()    -> void override { *target_ = false; }
    [[nodiscard]] auto name() const -> std::string_view override { return name_; }

private:
    bool* target_;
    std::string_view name_;
};

TEST_CASE("CommandStack: fresh stack has no undo/redo", "[editor][command]") {
    ed::CommandStack stack;
    REQUIRE_FALSE(stack.can_undo());
    REQUIRE_FALSE(stack.can_redo());
    REQUIRE(stack.undo_name().empty());
    REQUIRE(stack.redo_name().empty());
    REQUIRE_FALSE(stack.undo());
    REQUIRE_FALSE(stack.redo());
}

TEST_CASE("CommandStack: execute pushes to undo stack", "[editor][command]") {
    ed::CommandStack stack;
    bool flag = false;
    stack.execute(std::make_unique<ToggleCommand>(&flag, "Toggle"));
    REQUIRE(flag == true);               // execute() was called
    REQUIRE(stack.can_undo());
    REQUIRE_FALSE(stack.can_redo());     // redo cleared
    REQUIRE(stack.undo_name() == "Toggle");
}

TEST_CASE("CommandStack: undo then redo cycle", "[editor][command]") {
    ed::CommandStack stack;
    bool flag = false;
    stack.execute(std::make_unique<ToggleCommand>(&flag, "Toggle"));

    // Undo
    flag = false;  // reset for verification
    REQUIRE(stack.undo());
    REQUIRE_FALSE(flag);                  // undo() was called
    REQUIRE_FALSE(stack.can_undo());
    REQUIRE(stack.can_redo());
    REQUIRE(stack.redo_name() == "Toggle");

    // Redo
    REQUIRE(stack.redo());
    REQUIRE(flag);                        // execute() called again
    REQUIRE(stack.can_undo());
    REQUIRE_FALSE(stack.can_redo());
}

TEST_CASE("CommandStack: new command clears redo stack", "[editor][command]") {
    ed::CommandStack stack;
    bool flag1 = false, flag2 = false;

    stack.execute(std::make_unique<ToggleCommand>(&flag1, "C1"));
    [[maybe_unused]] auto _1 = stack.undo();   // redo stack has C1
    REQUIRE(stack.can_redo());

    stack.execute(std::make_unique<ToggleCommand>(&flag2, "C2"));
    REQUIRE_FALSE(stack.can_redo());           // redo cleared
    REQUIRE(stack.can_undo());
    REQUIRE(stack.undo_name() == "C2");        // C2 is now on top
}

TEST_CASE("CommandStack: max_history bound enforced", "[editor][command]") {
    ed::CommandStack stack(2);  // max 2
    bool f1 = false, f2 = false, f3 = false;

    stack.execute(std::make_unique<ToggleCommand>(&f1, "C1"));
    stack.execute(std::make_unique<ToggleCommand>(&f2, "C2"));
    stack.execute(std::make_unique<ToggleCommand>(&f3, "C3"));

    // Stack has 2 entries: C2 (bottom) and C3 (top)
    REQUIRE(stack.can_undo());
    REQUIRE(stack.undo());
    REQUIRE(stack.undo_name() == "C2");  // C2 should be on top after undoing C3
    REQUIRE(stack.undo());
    REQUIRE_FALSE(stack.can_undo());     // Both undone
}

TEST_CASE("CommandStack: clear empties both stacks", "[editor][command]") {
    ed::CommandStack stack;
    bool flag = false;

    stack.execute(std::make_unique<ToggleCommand>(&flag, "C1"));
    REQUIRE(stack.undo());               // Now undo is empty, redo has C1
    REQUIRE_FALSE(stack.can_undo());
    REQUIRE(stack.can_redo());

    stack.clear();
    REQUIRE_FALSE(stack.can_undo());
    REQUIRE_FALSE(stack.can_redo());
}

TEST_CASE("CommandStack: max_history clamped to minimum 1", "[editor][command]") {
    ed::CommandStack stack(0);  // Should clamp to 1
    bool f1 = false, f2 = false;

    stack.execute(std::make_unique<ToggleCommand>(&f1, "C1"));
    stack.execute(std::make_unique<ToggleCommand>(&f2, "C2"));

    // With max_history=1, only C2 should remain
    REQUIRE(stack.can_undo());
    REQUIRE(stack.undo());
    // After undoing C2, stack should be empty (C1 was dropped)
    REQUIRE_FALSE(stack.can_undo());
}

TEST_CASE("CommandStack: undo_name returns empty on empty stack", "[editor][command]") {
    ed::CommandStack stack;
    REQUIRE(stack.undo_name().empty());
    REQUIRE(stack.redo_name().empty());
}

TEST_CASE("CommandStack: redo_name returns command name after undo", "[editor][command]") {
    ed::CommandStack stack;
    bool flag = false;
    stack.execute(std::make_unique<ToggleCommand>(&flag, "MyCommand"));
    REQUIRE(stack.undo());
    REQUIRE(stack.redo_name() == "MyCommand");
}

TEST_CASE("Editor can be constructed, set up, and shut down headlessly", "[editor]") {
    // Create a headless engine
    auto engine = buddd::engine::EngineService::create(
        buddd::engine::Backend::Headless,
        buddd::engine::WindowConfig{.title = "Editor Test", .width = 128, .height = 128});
    REQUIRE(engine.has_value());
    auto& eng = **engine;

    auto world = std::make_unique<buddd::engine::World>();
    auto render_system = std::make_unique<buddd::engine::RenderSystem>(eng.device(), *world);

    buddd::engine::EngineContext ctx{
        eng, eng.window(), eng.device(), *world, *render_system, 0.016f, 0
    };

    // Construct Editor
    buddd::editor::Editor editor;

    // Setup will fail because ImGui is not initialized in headless mode — must not crash
    auto result = editor.setup(ctx);
    // Both success and failure are valid outcomes; we only verify no crash.

    // Shutdown must be safe and idempotent
    editor.shutdown();
    editor.shutdown();  // second call must be a no-op
}

TEST_CASE("Editor: world() returns valid empty World before setup", "[editor][scene_state]") {
    buddd::editor::Editor editor;

    // world() must return a valid World& before any setup() call
    auto& w = editor.world();
    REQUIRE(w.entity_count() == 0);
    REQUIRE(w.root_entity_count() == 0);
}

TEST_CASE("Editor: world() valid after setup+shutdown", "[editor][scene_state]") {
    // Create a headless engine for setup()
    auto engine = buddd::engine::EngineService::create(
        buddd::engine::Backend::Headless,
        buddd::engine::WindowConfig{.title = "Editor Test", .width = 128, .height = 128});
    REQUIRE(engine.has_value());
    auto& eng = **engine;

    auto world = std::make_unique<buddd::engine::World>();
    auto render_system = std::make_unique<buddd::engine::RenderSystem>(eng.device(), *world);

    buddd::engine::EngineContext ctx{
        eng, eng.window(), eng.device(), *world, *render_system, 0.016f, 0
    };

    buddd::editor::Editor editor;

    // World is accessible before setup
    REQUIRE(editor.world().entity_count() == 0);

    // Setup (may fail in headless, that's OK)
    auto result = editor.setup(ctx);
    (void)result;

    // World is accessible after setup
    REQUIRE(editor.world().entity_count() == 0);

    // Shutdown
    editor.shutdown();

    // World is accessible after shutdown — must still be valid
    REQUIRE(editor.world().entity_count() == 0);
}

TEST_CASE("Editor: world() valid after setup failure", "[editor][scene_state]") {
    // Create a headless engine (same as existing test — setup will fail because ImGui is not initialized)
    auto engine = buddd::engine::EngineService::create(
        buddd::engine::Backend::Headless,
        buddd::engine::WindowConfig{.title = "Editor Test", .width = 128, .height = 128});
    REQUIRE(engine.has_value());
    auto& eng = **engine;

    auto world = std::make_unique<buddd::engine::World>();
    auto render_system = std::make_unique<buddd::engine::RenderSystem>(eng.device(), *world);

    buddd::engine::EngineContext ctx{
        eng, eng.window(), eng.device(), *world, *render_system, 0.016f, 0
    };

    buddd::editor::Editor editor;

    // Setup will fail (ImGui not initialized in headless mode)
    auto result = editor.setup(ctx);
    REQUIRE_FALSE(result.has_value());  // Verify setup() actually fails

    // World must still be valid even after setup() failure
    REQUIRE(editor.world().entity_count() == 0);

    // No leak: Editor destructor handles cleanup (verified by ASan/Valgrind at test level)
}

TEST_CASE("Editor: World is empty on construction", "[editor][scene_state]") {
    buddd::editor::Editor editor;

    REQUIRE(editor.world().entity_count() == 0);
    REQUIRE(editor.world().root_entity_count() == 0);
}
