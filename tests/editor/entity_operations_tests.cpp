#include "editor.h"
#include "editor_context.h"

#include "commands/create_entity_command.h"
#include "commands/delete_entity_command.h"
#include "commands/rename_entity_command.h"
#include "command_stack.h"

#include "engine_context.h"
#include "engine_service.h"
#include "scene/world.h"
#include "render/render_system.h"
#include "platform/platform.h"
#include "window/window.h"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

namespace ed = buddd::editor;
namespace be = buddd::engine;

// ── Test context providing Editor + EngineContext + EditorContext ──
struct EntityTestCtx {
    std::unique_ptr<be::EngineService> engine;
    std::unique_ptr<be::World> engine_world;
    std::unique_ptr<be::RenderSystem> render_system;
    std::unique_ptr<be::EngineContext> engine_ctx;
    ed::Editor editor;
    ed::EditorContext editor_ctx;

    EntityTestCtx()
        : editor()
        , editor_ctx(editor, get_or_create_engine_ctx())
    {}

    auto get_or_create_engine_ctx() -> be::EngineContext const& {
        if (!engine_ctx) {
            auto eng = be::EngineService::create(
                be::Backend::Headless,
                be::WindowConfig{.title = "EntityTest", .width = 128, .height = 128});
            if (eng.has_value()) {
                engine = std::move(*eng);
                engine_world = std::make_unique<be::World>();
                render_system = std::make_unique<be::RenderSystem>(
                    engine->device(), *engine_world);
                engine_ctx = std::make_unique<be::EngineContext>(
                    be::EngineContext{
                        *engine, engine->window(), engine->device(), *engine_world,
                        *render_system, 0.016f, 0});
            }
        }
        return *engine_ctx;
    }

    auto& world() { return editor.world(); }
    auto& selection() { return editor.selection(); }
    auto& command_stack() { return editor.command_stack(); }

    /// Add a root entity with the given name.
    auto add_root_entity(const std::string& name) -> be::Entity {
        auto entity = world().add_entity();
        entity.set_name(name);
        return entity;
    }

    /// Add a child entity to the given parent with the given name.
    auto add_child_entity(be::Entity parent, const std::string& name) -> be::Entity {
        auto child = parent.create_child();
        child.set_name(name);
        return child;
    }
};

// ═════════════════════════════════════════════════════════════════════
// CreateEntityCommand tests
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("CreateEntity: creates root entity with no anchor", "[editor][commands][create]") {
    EntityTestCtx ctx;

    size_t before = ctx.world().root_entity_count();

    auto cmd = std::make_unique<ed::CreateEntityCommand>();
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);

    REQUIRE(ctx.world().root_entity_count() == before + 1);
}

TEST_CASE("CreateEntity: creates child of anchor", "[editor][commands][create]") {
    EntityTestCtx ctx;

    // Create a root entity to act as anchor
    auto parent = ctx.add_root_entity("Parent");
    ctx.selection().select(parent.id(), ed::SelectionModifier::Replace);

    // Verify anchor is set
    REQUIRE(ctx.selection().anchor().has_value());
    REQUIRE(*ctx.selection().anchor() == parent.id());

    size_t child_count_before = parent.child_count();

    auto cmd = std::make_unique<ed::CreateEntityCommand>();
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);

    // Verify child count incremented
    // Need to re-read parent entity since it may have changed
    // Actually, Entity is a value type that's re-queried from the world
    REQUIRE(parent.child_count() == child_count_before + 1);
}

TEST_CASE("CreateEntity: undo destroys entity", "[editor][commands][create][undo_redo]") {
    EntityTestCtx ctx;
    auto& world = ctx.world();

    size_t before = world.entity_count();

    auto cmd = std::make_unique<ed::CreateEntityCommand>();
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);
    REQUIRE(world.entity_count() == before + 1);

    // Undo — entity is marked pending_destroy but not flushed yet
    static_cast<void>(ctx.command_stack().undo(ctx.editor_ctx));
    REQUIRE(world.entity_count() == before + 1);  // still alive in slot, pending_destroy

    // After flush, entity should be gone
    world.flush_destroyed();
    REQUIRE(world.entity_count() == before);
}

TEST_CASE("CreateEntity: redo recreates entity", "[editor][commands][create][undo_redo]") {
    EntityTestCtx ctx;
    auto& world = ctx.world();

    size_t before = world.entity_count();

    auto cmd = std::make_unique<ed::CreateEntityCommand>();
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);
    REQUIRE(world.entity_count() == before + 1);

    // Undo + flush
    static_cast<void>(ctx.command_stack().undo(ctx.editor_ctx));
    world.flush_destroyed();
    REQUIRE(world.entity_count() == before);

    // Redo — entity recreated
    static_cast<void>(ctx.command_stack().redo(ctx.editor_ctx));
    REQUIRE(world.entity_count() == before + 1);
}

TEST_CASE("CreateEntity: undo restores selection", "[editor][commands][create][undo_redo]") {
    EntityTestCtx ctx;

    auto entity = ctx.add_root_entity("Root");
    ctx.selection().select(entity.id(), ed::SelectionModifier::Replace);
    REQUIRE(ctx.selection().size() == 1);

    // Snapshot current selection
    auto saved = ctx.selection().snapshot();

    auto cmd = std::make_unique<ed::CreateEntityCommand>();
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);

    // Undo should restore selection to saved state
    static_cast<void>(ctx.command_stack().undo(ctx.editor_ctx));
    REQUIRE(ctx.selection().snapshot() == saved);
}

TEST_CASE("CreateEntity: selection unchanged after create", "[editor][commands][create]") {
    EntityTestCtx ctx;

    auto entity = ctx.add_root_entity("Root");
    ctx.selection().select(entity.id(), ed::SelectionModifier::Replace);
    REQUIRE(ctx.selection().size() == 1);
    REQUIRE(ctx.selection().contains(entity.id()));

    auto cmd = std::make_unique<ed::CreateEntityCommand>();
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);

    // Selection should still contain the original entity
    REQUIRE(ctx.selection().size() == 1);
    REQUIRE(ctx.selection().contains(entity.id()));
}

// ═════════════════════════════════════════════════════════════════════
// CreateEntityCommand — post_creation_name + auto-rename tests
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("CreateEntity::created_entity_id returns valid ID after execute", "[editor][commands][create][auto_rename]") {
    EntityTestCtx ctx;

    auto cmd = std::make_unique<ed::CreateEntityCommand>();
    auto* raw = cmd.get();
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);

    REQUIRE(raw->created_entity_id() != be::EntityId::none());
}

TEST_CASE("CreateEntity::set_post_creation_name creates entity with name", "[editor][commands][create][auto_rename]") {
    EntityTestCtx ctx;

    auto cmd = std::make_unique<ed::CreateEntityCommand>();
    cmd->set_post_creation_name("Player");
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);

    // The entity should exist in the world with the given name
    REQUIRE(ctx.world().root_entity_count() == 1);
    auto root = ctx.world().get_root_entity(0);
    REQUIRE(root.id() != be::EntityId::none());
    REQUIRE(root.name() == "Player");
}

TEST_CASE("CreateEntity::set_post_creation_name undo destroys entity", "[editor][commands][create][auto_rename][undo_redo]") {
    EntityTestCtx ctx;
    auto& world = ctx.world();

    size_t before = world.entity_count();

    auto cmd = std::make_unique<ed::CreateEntityCommand>();
    cmd->set_post_creation_name("Player");
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);
    REQUIRE(world.entity_count() == before + 1);

    // Undo — entity is pending_destroy
    static_cast<void>(ctx.command_stack().undo(ctx.editor_ctx));
    REQUIRE(world.entity_count() == before + 1);  // still alive in slot, pending_destroy

    // After flush, entity should be gone
    world.flush_destroyed();
    REQUIRE(world.entity_count() == before);
}

TEST_CASE("CreateEntity::set_post_creation_name redo recreates entity with name", "[editor][commands][create][auto_rename][undo_redo]") {
    EntityTestCtx ctx;
    auto& world = ctx.world();

    size_t before = world.entity_count();

    auto cmd = std::make_unique<ed::CreateEntityCommand>();
    cmd->set_post_creation_name("Player");
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);
    REQUIRE(world.entity_count() == before + 1);

    // Undo + flush
    static_cast<void>(ctx.command_stack().undo(ctx.editor_ctx));
    world.flush_destroyed();
    REQUIRE(world.entity_count() == before);

    // Redo — entity should be recreated with name
    static_cast<void>(ctx.command_stack().redo(ctx.editor_ctx));
    REQUIRE(world.entity_count() == before + 1);
    auto root = world.get_root_entity(0);
    REQUIRE(root.name() == "Player");
}

TEST_CASE("CreateEntity::no post_creation_name creates unnamed entity", "[editor][commands][create][auto_rename]") {
    EntityTestCtx ctx;

    auto cmd = std::make_unique<ed::CreateEntityCommand>();
    // Do NOT call set_post_creation_name — post_creation_name_ stays std::nullopt
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);

    REQUIRE(ctx.world().root_entity_count() == 1);
    auto root = ctx.world().get_root_entity(0);
    REQUIRE(root.name().empty());
}

TEST_CASE("CreateEntity::set_post_creation_name empty string leaves entity unnamed", "[editor][commands][create][auto_rename]") {
    EntityTestCtx ctx;

    auto cmd = std::make_unique<ed::CreateEntityCommand>();
    cmd->set_post_creation_name("");
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);

    REQUIRE(ctx.world().root_entity_count() == 1);
    auto root = ctx.world().get_root_entity(0);
    REQUIRE(root.name().empty());
}

TEST_CASE("CreateEntity::created_entity_id returns none before execute", "[editor][commands][create][auto_rename]") {
    auto cmd = std::make_unique<ed::CreateEntityCommand>();
    REQUIRE(cmd->created_entity_id() == be::EntityId::none());
}

TEST_CASE("CreateEntity::auto-select after execute", "[editor][commands][create][auto_rename]") {
    EntityTestCtx ctx;

    // Start with a selected entity
    auto existing = ctx.add_root_entity("Existing");
    ctx.selection().select(existing.id(), ed::SelectionModifier::Replace);
    REQUIRE(ctx.selection().size() == 1);
    REQUIRE(ctx.selection().contains(existing.id()));

    // Command saves pre-execution selection but does NOT modify it
    auto cmd = std::make_unique<ed::CreateEntityCommand>();
    auto* raw = cmd.get();
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);

    auto created_id = raw->created_entity_id();
    REQUIRE(created_id != be::EntityId::none());

    // The command itself does NOT modify selection (auto-select is ScenePanel's job)
    // Selection should still contain the original entity
    REQUIRE(ctx.selection().contains(existing.id()));
    REQUIRE(ctx.selection().size() == 1);
}

// ═════════════════════════════════════════════════════════════════════
// DeleteEntityCommand tests
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("DeleteEntity: single leaf destroyed", "[editor][commands][delete]") {
    EntityTestCtx ctx;

    auto entity = ctx.add_root_entity("Leaf");
    be::EntityId target_id = entity.id();

    auto cmd = std::make_unique<ed::DeleteEntityCommand>(
        std::vector<be::EntityId>{target_id});
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);

    // Entity should be pending destroy (not yet flushed)
    REQUIRE(ctx.world().entity_count() == 1);  // still counted as alive
    REQUIRE(entity.is_pending_destroy());
}

TEST_CASE("DeleteEntity: selection cleared after delete", "[editor][commands][delete]") {
    EntityTestCtx ctx;

    auto entity = ctx.add_root_entity("Leaf");
    ctx.selection().select(entity.id(), ed::SelectionModifier::Replace);
    REQUIRE(!ctx.selection().empty());

    auto cmd = std::make_unique<ed::DeleteEntityCommand>(
        std::vector<be::EntityId>{entity.id()});
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);

    REQUIRE(ctx.selection().empty());
}

TEST_CASE("DeleteEntity: undo restores entity with name and hierarchy", "[editor][commands][delete][undo_redo]") {
    EntityTestCtx ctx;
    auto& world = ctx.world();

    // Create parent with one child
    auto parent = ctx.add_root_entity("Parent");
    auto child = ctx.add_child_entity(parent, "Child");
    be::EntityId parent_id = parent.id();

    auto cmd = std::make_unique<ed::DeleteEntityCommand>(
        std::vector<be::EntityId>{parent_id});
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);

    // Flush destroyed entities so we start fresh for undo
    world.flush_destroyed();
    REQUIRE(world.entity_count() == 0);

    // Undo — entities should be recreated
    static_cast<void>(ctx.command_stack().undo(ctx.editor_ctx));

    // Root entity count should be 1
    REQUIRE(world.root_entity_count() == 1);

    // Verify the recreated root entity has the correct name
    auto recreated_parent = world.get_root_entity(0);
    REQUIRE(recreated_parent.id() != be::EntityId::none());
    REQUIRE(recreated_parent.name() == "Parent");

    // Verify the child exists and has the correct name
    REQUIRE(recreated_parent.child_count() == 1);
    auto recreated_child = recreated_parent.get_child(0);
    REQUIRE(recreated_child.name() == "Child");
}

TEST_CASE("DeleteEntity: undo restores previous selection", "[editor][commands][delete][undo_redo]") {
    EntityTestCtx ctx;

    auto entityA = ctx.add_root_entity("A");
    auto entityB = ctx.add_root_entity("B");

    // Select A
    ctx.selection().select(entityA.id(), ed::SelectionModifier::Replace);
    auto saved = ctx.selection().snapshot();

    // Delete B (not selected)
    auto cmd = std::make_unique<ed::DeleteEntityCommand>(
        std::vector<be::EntityId>{entityB.id()});
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);

    // Selection should be cleared by delete
    REQUIRE(ctx.selection().empty());

    // Undo — selection should be restored to saved (A selected)
    static_cast<void>(ctx.command_stack().undo(ctx.editor_ctx));
    REQUIRE(ctx.selection().snapshot() == saved);
}

TEST_CASE("DeleteEntity: redo destroys again", "[editor][commands][delete][undo_redo]") {
    EntityTestCtx ctx;
    auto& world = ctx.world();

    auto entity = ctx.add_root_entity("Target");
    be::EntityId target_id = entity.id();

    std::vector<be::EntityId> ids = {target_id};
    auto cmd = std::make_unique<ed::DeleteEntityCommand>(ids);
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);
    REQUIRE(entity.is_pending_destroy());

    // Flush + undo
    world.flush_destroyed();
    REQUIRE(world.entity_count() == 0);
    static_cast<void>(ctx.command_stack().undo(ctx.editor_ctx));
    REQUIRE(world.entity_count() == 1);

    // Redo — entity should be pending_destroy again
    static_cast<void>(ctx.command_stack().redo(ctx.editor_ctx));
    REQUIRE(world.root_entity_count() == 1);
    auto root = world.get_root_entity(0);
    REQUIRE(root.is_pending_destroy());

    // After flush, entity should be gone
    world.flush_destroyed();
    REQUIRE(world.entity_count() == 0);
}

// ═════════════════════════════════════════════════════════════════════
// RenameEntityCommand tests
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("RenameEntity: execute changes name", "[editor][commands][rename]") {
    EntityTestCtx ctx;

    auto entity = ctx.add_root_entity("OldName");
    be::EntityId id = entity.id();

    auto cmd = std::make_unique<ed::RenameEntityCommand>(id, "OldName", "NewName");
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);

    // Need to re-query entity to get updated name
    // Entity is a value type — re-read from world
    auto root = ctx.world().get_root_entity(0);
    REQUIRE(root.name() == "NewName");
}

TEST_CASE("RenameEntity: undo restores old name", "[editor][commands][rename][undo_redo]") {
    EntityTestCtx ctx;

    auto entity = ctx.add_root_entity("OldName");
    be::EntityId id = entity.id();

    auto cmd = std::make_unique<ed::RenameEntityCommand>(id, "OldName", "NewName");
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);
    REQUIRE(ctx.world().get_root_entity(0).name() == "NewName");

    static_cast<void>(ctx.command_stack().undo(ctx.editor_ctx));
    REQUIRE(ctx.world().get_root_entity(0).name() == "OldName");
}

TEST_CASE("RenameEntity: redo re-applies new name", "[editor][commands][rename][undo_redo]") {
    EntityTestCtx ctx;

    auto entity = ctx.add_root_entity("OldName");
    be::EntityId id = entity.id();

    auto cmd = std::make_unique<ed::RenameEntityCommand>(id, "OldName", "NewName");
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);
    REQUIRE(ctx.world().get_root_entity(0).name() == "NewName");

    static_cast<void>(ctx.command_stack().undo(ctx.editor_ctx));
    REQUIRE(ctx.world().get_root_entity(0).name() == "OldName");

    static_cast<void>(ctx.command_stack().redo(ctx.editor_ctx));
    REQUIRE(ctx.world().get_root_entity(0).name() == "NewName");
}

TEST_CASE("RenameEntity: undo restores selection", "[editor][commands][rename][undo_redo]") {
    EntityTestCtx ctx;

    auto entity = ctx.add_root_entity("OldName");
    be::EntityId id = entity.id();

    ctx.selection().select(id, ed::SelectionModifier::Replace);
    auto saved = ctx.selection().snapshot();

    auto cmd = std::make_unique<ed::RenameEntityCommand>(id, "OldName", "NewName");
    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);

    static_cast<void>(ctx.command_stack().undo(ctx.editor_ctx));
    REQUIRE(ctx.selection().snapshot() == saved);
}

// ═════════════════════════════════════════════════════════════════════
// CommandStack: EditorContext forwarding test
// ═════════════════════════════════════════════════════════════════════

// A command that captures the EditorContext passed to execute/undo
class ContextCaptureCommand final : public ed::Command {
public:
    bool execute_called = false;
    bool undo_called = false;
    ed::EditorContext const* captured_execute_ctx = nullptr;
    ed::EditorContext const* captured_undo_ctx = nullptr;

    auto execute(ed::EditorContext const& ctx) -> void override {
        execute_called = true;
        captured_execute_ctx = &ctx;
    }

    auto undo(ed::EditorContext const& ctx) -> void override {
        undo_called = true;
        captured_undo_ctx = &ctx;
    }

    [[nodiscard]] auto name() const -> std::string_view override {
        return "ContextCapture";
    }
};

TEST_CASE("CommandStack: EditorContext forwarded to execute/undo", "[editor][commands]") {
    EntityTestCtx ctx;

    auto cmd = std::make_unique<ContextCaptureCommand>();
    auto* raw = cmd.get();

    ctx.command_stack().execute(std::move(cmd), ctx.editor_ctx);
    REQUIRE(raw->execute_called);
    // The ctx passed to execute should be the same object
    REQUIRE(raw->captured_execute_ctx == &ctx.editor_ctx);

    static_cast<void>(ctx.command_stack().undo(ctx.editor_ctx));
    REQUIRE(raw->undo_called);
    REQUIRE(raw->captured_undo_ctx == &ctx.editor_ctx);
}
