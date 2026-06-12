#include "editor_selection.h"
#include "editor.h"

#include "engine_context.h"
#include "engine_service.h"
#include "scene/world.h"
#include "render/render_system.h"
#include "platform/platform.h"
#include "window/window.h"

#include <catch2/catch_test_macros.hpp>

#include <unistd.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace ed = buddd::editor;
namespace be = buddd::engine;

// ── Helper: a test EntityId factory ──
inline auto make_id(uint32_t index, uint32_t generation = 0) -> be::EntityId {
    return be::EntityId{index, generation};
}

static constexpr be::EntityId ID1 = be::EntityId{1, 0};
static constexpr be::EntityId ID2 = be::EntityId{2, 0};
static constexpr be::EntityId ID3 = be::EntityId{3, 0};
static constexpr be::EntityId ID4 = be::EntityId{4, 0};
static constexpr be::EntityId ID5 = be::EntityId{5, 0};

// ═════════════════════════════════════════════════════════════════════
// Selection — value semantics
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("Selection: contains/size/empty", "[editor][selection]") {
    ed::Selection sel;
    REQUIRE(sel.empty());
    REQUIRE(sel.size() == 0);
    REQUIRE_FALSE(sel.contains(ID1));

    sel.add(ID1);
    REQUIRE_FALSE(sel.empty());
    REQUIRE(sel.size() == 1);
    REQUIRE(sel.contains(ID1));
    REQUIRE_FALSE(sel.contains(ID2));
}

TEST_CASE("Selection: first() behavior", "[editor][selection]") {
    ed::Selection sel;

    // Empty: first() returns nullopt
    auto first = sel.first();
    REQUIRE_FALSE(first.has_value());

    // Non-empty: first() returns a valid EntityId
    sel.add(ID2);
    first = sel.first();
    REQUIRE(first.has_value());
    // Should be the id we added
    REQUIRE(*first == ID2);
}

TEST_CASE("Selection: copy semantics", "[editor][selection]") {
    ed::Selection original;
    original.add(ID1);
    REQUIRE(original.size() == 1);

    // Copy
    ed::Selection copy = original;
    REQUIRE(copy.size() == 1);
    REQUIRE(copy.contains(ID1));

    // Modify copy: add a new id
    copy.add(ID2);
    REQUIRE(copy.size() == 2);
    REQUIRE(copy.contains(ID2));

    // Original must be unchanged
    REQUIRE(original.size() == 1);
    REQUIRE_FALSE(original.contains(ID2));
}

TEST_CASE("Selection: add/remove/clear", "[editor][selection]") {
    ed::Selection sel;

    // Add
    sel.add(ID1);
    REQUIRE(sel.contains(ID1));
    REQUIRE(sel.size() == 1);

    // Remove
    sel.remove(ID1);
    REQUIRE_FALSE(sel.contains(ID1));
    REQUIRE(sel.empty());

    // Clear
    sel.add(ID1);
    sel.add(ID2);
    REQUIRE(sel.size() == 2);
    sel.clear();
    REQUIRE(sel.empty());
    REQUIRE_FALSE(sel.contains(ID1));
    REQUIRE_FALSE(sel.contains(ID2));
}

TEST_CASE("Selection: operator==", "[editor][selection]") {
    ed::Selection a, b;

    // Both empty: equal
    REQUIRE(a == b);

    // Same single element
    a.add(ID1);
    b.add(ID1);
    REQUIRE(a == b);

    // Different sets
    b.add(ID2);
    REQUIRE_FALSE(a == b);

    // Same multi-element set
    a.add(ID2);
    REQUIRE(a == b);

    // Order doesn't matter for set
    ed::Selection c, d;
    c.add(ID2);
    c.add(ID1);
    d.add(ID1);
    d.add(ID2);
    REQUIRE(c == d);
}

TEST_CASE("Selection: range-for iteration", "[editor][selection]") {
    ed::Selection sel;
    sel.add(ID1);
    sel.add(ID2);
    sel.add(ID3);

    std::vector<be::EntityId> visited;
    for (auto id : sel) {
        visited.push_back(id);
    }

    // All 3 must be present
    REQUIRE(visited.size() == 3);
    // Check each expected id is present
    auto contains_id = [&](be::EntityId target) {
        return std::find(visited.begin(), visited.end(), target) != visited.end();
    };
    REQUIRE(contains_id(ID1));
    REQUIRE(contains_id(ID2));
    REQUIRE(contains_id(ID3));
}

TEST_CASE("Selection: ignores EntityId::none()", "[editor][selection]") {
    ed::Selection sel;

    // Add none() — should be silently ignored via the Selection::add (no guard needed,
    // but EditorSelection::select() has the guard; Selection::add just inserts it)
    // This test verifies that EditorSelection guards against EntityId::none().
    ed::EditorSelection esel;
    esel.select(be::EntityId::none(), ed::SelectionModifier::Replace);
    REQUIRE(esel.empty());

    esel.select(be::EntityId::none(), ed::SelectionModifier::Toggle);
    REQUIRE(esel.empty());

    // set_selection with none() entries
    std::vector<be::EntityId> ids = {ID1, be::EntityId::none(), ID2};
    esel.set_selection(ids);
    REQUIRE(esel.size() == 2);
    REQUIRE(esel.contains(ID1));
    REQUIRE(esel.contains(ID2));
}

// ═════════════════════════════════════════════════════════════════════
// EditorSelection — mutations
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("EditorSelection: select(Replace)", "[editor][selection]") {
    ed::EditorSelection esel;

    // Select first entity
    esel.select(ID1, ed::SelectionModifier::Replace);
    REQUIRE(esel.size() == 1);
    REQUIRE(esel.contains(ID1));
    REQUIRE(esel.anchor().has_value());
    REQUIRE(*esel.anchor() == ID1);

    // Replace with a different entity
    esel.select(ID2, ed::SelectionModifier::Replace);
    REQUIRE(esel.size() == 1);
    REQUIRE_FALSE(esel.contains(ID1));
    REQUIRE(esel.contains(ID2));
    REQUIRE(*esel.anchor() == ID2);
}

TEST_CASE("EditorSelection: select(Toggle)", "[editor][selection]") {
    ed::EditorSelection esel;

    // Start with id1 selected (via Replace to establish anchor)
    esel.select(ID1, ed::SelectionModifier::Replace);
    REQUIRE(esel.anchor().has_value());
    auto original_anchor = *esel.anchor();

    // Toggle id2: should add it
    esel.select(ID2, ed::SelectionModifier::Toggle);
    REQUIRE(esel.size() == 2);
    REQUIRE(esel.contains(ID1));
    REQUIRE(esel.contains(ID2));
    // Anchor unchanged by toggle
    REQUIRE(esel.anchor().has_value());
    REQUIRE(*esel.anchor() == original_anchor);

    // Toggle id1 again: should remove it
    esel.select(ID1, ed::SelectionModifier::Toggle);
    REQUIRE(esel.size() == 1);
    REQUIRE_FALSE(esel.contains(ID1));
    REQUIRE(esel.contains(ID2));
    // Anchor still unchanged
    REQUIRE(*esel.anchor() == original_anchor);
}

TEST_CASE("EditorSelection: clear()", "[editor][selection]") {
    ed::EditorSelection esel;

    esel.select(ID1, ed::SelectionModifier::Replace);
    REQUIRE(esel.size() == 1);
    REQUIRE(esel.anchor().has_value());

    esel.clear();
    REQUIRE(esel.empty());
    REQUIRE(esel.size() == 0);
    REQUIRE_FALSE(esel.anchor().has_value());
}

TEST_CASE("EditorSelection: set_selection", "[editor][selection]") {
    ed::EditorSelection esel;

    // Set from span of 3 ids
    std::vector<be::EntityId> ids = {ID1, ID2, ID3};
    esel.set_selection(ids);
    REQUIRE(esel.size() == 3);
    REQUIRE(esel.contains(ID1));
    REQUIRE(esel.contains(ID2));
    REQUIRE(esel.contains(ID3));

    // Anchor unchanged (no anchor was set initially)
    REQUIRE_FALSE(esel.anchor().has_value());

    // Replace with different set
    std::vector<be::EntityId> ids2 = {ID3, ID4};
    esel.set_selection(ids2);
    REQUIRE(esel.size() == 2);
    REQUIRE_FALSE(esel.contains(ID1));
    REQUIRE_FALSE(esel.contains(ID2));
    REQUIRE(esel.contains(ID3));
    REQUIRE(esel.contains(ID4));

    // Empty span
    esel.set_selection(std::span<const be::EntityId>{});
    REQUIRE(esel.empty());
}

TEST_CASE("EditorSelection: snapshot isolation", "[editor][selection]") {
    ed::EditorSelection esel;

    esel.select(ID1, ed::SelectionModifier::Replace);
    REQUIRE(esel.size() == 1);

    // Snapshot
    auto saved = esel.snapshot();
    REQUIRE(saved.size() == 1);
    REQUIRE(saved.contains(ID1));

    // Modify snapshot via Selection API (local mutation)
    saved.add(ID2);
    REQUIRE(saved.size() == 2);
    REQUIRE(saved.contains(ID2));

    // Original EditorSelection unchanged
    REQUIRE(esel.size() == 1);
    REQUIRE_FALSE(esel.contains(ID2));
}

TEST_CASE("EditorSelection: restore", "[editor][selection]") {
    ed::EditorSelection esel;

    // Select id1 and save snapshot
    esel.select(ID1, ed::SelectionModifier::Replace);
    auto saved = esel.snapshot();

    // Change selection to something else
    esel.select(ID2, ed::SelectionModifier::Replace);
    REQUIRE(esel.size() == 1);
    REQUIRE(esel.contains(ID2));

    // Restore: should go back to id1
    esel.restore(saved);
    REQUIRE(esel.size() == 1);
    REQUIRE(esel.contains(ID1));
    REQUIRE_FALSE(esel.contains(ID2));
}

TEST_CASE("EditorSelection: anchor", "[editor][selection]") {
    ed::EditorSelection esel;

    // Initially no anchor
    REQUIRE_FALSE(esel.anchor().has_value());

    // Set anchor
    esel.set_anchor(ID3);
    REQUIRE(esel.anchor().has_value());
    REQUIRE(*esel.anchor() == ID3);

    // Clear clears anchor
    esel.clear();
    REQUIRE_FALSE(esel.anchor().has_value());
}

TEST_CASE("EditorSelection: on_change fires", "[editor][selection]") {
    ed::EditorSelection esel;

    int callback_count = 0;
    esel.on_change([&]() { ++callback_count; });

    // select() fires callback
    esel.select(ID1, ed::SelectionModifier::Replace);
    REQUIRE(callback_count == 1);

    // clear() fires callback
    esel.clear();
    REQUIRE(callback_count == 2);

    // set_selection() fires callback
    std::vector<be::EntityId> ids = {ID2};
    esel.set_selection(ids);
    REQUIRE(callback_count == 3);

    // restore() fires callback
    auto saved = esel.snapshot();
    esel.restore(saved);
    REQUIRE(callback_count == 4);
}

TEST_CASE("EditorSelection: remove_on_change", "[editor][selection]") {
    ed::EditorSelection esel;

    int callback_count = 0;
    auto token = esel.on_change([&]() { ++callback_count; });

    // Fires initially
    esel.select(ID1, ed::SelectionModifier::Replace);
    REQUIRE(callback_count == 1);

    // Remove callback
    esel.remove_on_change(token);

    // Should NOT fire
    esel.clear();
    REQUIRE(callback_count == 1);  // unchanged

    // Removing invalid token is a no-op
    esel.remove_on_change(999);
}

TEST_CASE("EditorSelection: multiple toggles", "[editor][selection]") {
    ed::EditorSelection esel;

    // Toggle 3 entities
    esel.select(ID1, ed::SelectionModifier::Toggle);
    esel.select(ID2, ed::SelectionModifier::Toggle);
    esel.select(ID3, ed::SelectionModifier::Toggle);

    // All 3 should be selected
    REQUIRE(esel.size() == 3);
    REQUIRE(esel.contains(ID1));
    REQUIRE(esel.contains(ID2));
    REQUIRE(esel.contains(ID3));

    // Toggle one off
    esel.select(ID2, ed::SelectionModifier::Toggle);
    REQUIRE(esel.size() == 2);
    REQUIRE(esel.contains(ID1));
    REQUIRE_FALSE(esel.contains(ID2));
    REQUIRE(esel.contains(ID3));
}

TEST_CASE("EditorSelection: current() accessor", "[editor][selection]") {
    ed::EditorSelection esel;

    esel.select(ID1, ed::SelectionModifier::Replace);
    auto const& current = esel.current();
    REQUIRE(current.contains(ID1));
    REQUIRE(current.size() == 1);
}

// ═════════════════════════════════════════════════════════════════════
// Editor::selection() integration
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("Editor::selection() accessor", "[editor][selection]") {
    ed::Editor editor;

    auto& sel = editor.selection();
    REQUIRE(sel.empty());
    REQUIRE_FALSE(sel.anchor().has_value());

    // Use the accessor to modify state
    sel.select(ID1, ed::SelectionModifier::Replace);
    REQUIRE(sel.contains(ID1));
    REQUIRE(sel.size() == 1);
}

TEST_CASE("Editor::new_scene() clears selection", "[editor][selection]") {
    ed::Editor editor;

    // Select something
    editor.selection().select(ID1, ed::SelectionModifier::Replace);
    REQUIRE(editor.selection().size() == 1);
    REQUIRE(editor.selection().anchor().has_value());

    // new_scene() should clear selection
    editor.new_scene();
    REQUIRE(editor.selection().empty());
    REQUIRE(editor.selection().size() == 0);
    REQUIRE_FALSE(editor.selection().anchor().has_value());
}

// ── Helper for open_scene tests ──
struct HeadlessTestContext {
    std::unique_ptr<be::EngineService> engine;
    std::unique_ptr<be::World> engine_world;
    std::unique_ptr<be::RenderSystem> render_system;
    std::unique_ptr<be::EngineContext> ctx;

    HeadlessTestContext() {
        auto eng = be::EngineService::create(
            be::Backend::Headless,
            be::WindowConfig{.title = "F-03 Test", .width = 128, .height = 128});
        REQUIRE(eng.has_value());
        engine = std::move(*eng);

        engine_world = std::make_unique<be::World>();
        render_system = std::make_unique<be::RenderSystem>(engine->device(), *engine_world);

        ctx = std::make_unique<be::EngineContext>(be::EngineContext{
            *engine, engine->window(), engine->device(), *engine_world,
            *render_system, 0.016f, 0});
    }
};

TEST_CASE("Editor::open_scene() clears selection on success", "[editor][selection]") {
    HeadlessTestContext htc;
    ed::Editor editor;

    [[maybe_unused]] auto _setup = editor.setup(*htc.ctx);

    // First save a scene so we can load it
    char temp_template[] = "/tmp/buddd_f03_XXXXXX";
    int fd = mkstemp(temp_template);
    REQUIRE(fd != -1);
    close(fd);
    std::string temp_path = std::string(temp_template) + ".yaml";

    auto save_result = editor.save_scene_as(temp_path);
    REQUIRE(save_result.has_value());

    // Select something
    editor.selection().select(ID1, ed::SelectionModifier::Replace);
    REQUIRE(editor.selection().size() == 1);
    REQUIRE(editor.selection().anchor().has_value());

    // Open the saved scene — selection should clear
    auto result = editor.open_scene(temp_path);
    REQUIRE(result.has_value());
    REQUIRE(editor.selection().empty());
    REQUIRE(editor.selection().size() == 0);
    REQUIRE_FALSE(editor.selection().anchor().has_value());

    std::remove(temp_path.c_str());
}
