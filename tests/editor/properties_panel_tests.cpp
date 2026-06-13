#include "editor.h"
#include "editor_context.h"
#include "editor_selection.h"
#include "editor_panel.h"
#include "panels/properties_panel.h"
#include "scene/entity.h"
#include "scene/world.h"
#include "scene/entity_id.h"
#include "engine_context.h"
#include "engine_service.h"
#include "render/render_system.h"
#include "platform/platform.h"
#include "window/window.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <memory>
#include <type_traits>

namespace ed = buddd::editor;
namespace be = buddd::engine;
using Catch::Approx;

// ═════════════════════════════════════════════════════════════════════
// PropertiesPanel: compile-time signature checks
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("PropertiesPanel: draw_ui signature matches base class", "[editor][properties]") {
    // Verify that PropertiesPanel inherits from EditorPanel
    static_assert(std::is_base_of_v<ed::EditorPanel, ed::PropertiesPanel>,
        "PropertiesPanel must inherit from EditorPanel");

    // Verify draw_ui is overridden (compile-time check)
    constexpr bool has_draw_ui = std::is_invocable_r_v<void,
        decltype([](ed::PropertiesPanel& p, ed::EditorContext const& c) -> decltype(p.draw_ui(c)) {}),
        ed::PropertiesPanel&, ed::EditorContext const&>;
    REQUIRE(has_draw_ui);
}

TEST_CASE("PropertiesPanel: id and title are correct", "[editor][properties]") {
    ed::PropertiesPanel panel;
    REQUIRE(panel.id() == "properties");
    REQUIRE(panel.title() == "Properties");
}

// ═════════════════════════════════════════════════════════════════════
// PropertiesPanel: selection access (no ImGui dependency)
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("PropertiesPanel: entity name can round-trip via EditorSelection", "[editor][properties]") {
    // This test verifies that primary() accessor works correctly and
    // can be used to select entities that would be shown in the panel.
    // Full draw_ui verification requires a display (ImGui) and is
    // deferred to manual smoke testing.

    auto eng = be::EngineService::create(
        be::Backend::Headless,
        be::WindowConfig{.title = "F-05 Properties Test", .width = 128, .height = 128});
    REQUIRE(eng.has_value());

    auto engine_world = std::make_unique<be::World>();
    auto render_system = std::make_unique<be::RenderSystem>((*eng)->device(), *engine_world);
    auto ctx = std::make_unique<be::EngineContext>(be::EngineContext{
        **eng, (*eng)->window(), (*eng)->device(), *engine_world,
        *render_system, 0.016f, 0});

    ed::Editor editor;
    [[maybe_unused]] auto setup_result = editor.setup(*ctx);
    ed::EditorContext editor_ctx{editor, *ctx};

    // Add an entity to the editor's world and set a known name
    auto& world = editor.world();
    auto entity = world.add_entity();
    entity.set_name("TestEntity");

    // Select the entity
    editor.selection().select(entity.id(), ed::SelectionModifier::Replace);

    // Verify primary returns the entity id
    REQUIRE(editor.selection().primary().has_value());
    REQUIRE(*editor.selection().primary() == entity.id());

    // Verify the entity name is readable
    REQUIRE(entity.name() == "TestEntity");
}

TEST_CASE("PropertiesPanel: transform read path works", "[editor][properties]") {
    using namespace buddd::engine::math;

    auto eng = be::EngineService::create(
        be::Backend::Headless,
        be::WindowConfig{.title = "F-05 Properties Test", .width = 128, .height = 128});
    REQUIRE(eng.has_value());

    auto engine_world = std::make_unique<be::World>();
    auto render_system = std::make_unique<be::RenderSystem>((*eng)->device(), *engine_world);
    auto ctx = std::make_unique<be::EngineContext>(be::EngineContext{
        **eng, (*eng)->window(), (*eng)->device(), *engine_world,
        *render_system, 0.016f, 0});

    ed::Editor editor;
    [[maybe_unused]] auto setup_result = editor.setup(*ctx);

    auto& world = editor.world();
    auto entity = world.add_entity();

    // Set transform values
    auto& transform = entity.transform();
    transform.position = Vec3(1.0f, 2.0f, 3.0f);
    transform.rotation = Quat::identity();
    transform.scale = Vec3(2.0f, 2.0f, 2.0f);

    // Verify transform values are correct
    REQUIRE(transform.position.x == Approx(1.0f).margin(1e-5f));
    REQUIRE(transform.position.y == Approx(2.0f).margin(1e-5f));
    REQUIRE(transform.position.z == Approx(3.0f).margin(1e-5f));

    REQUIRE(transform.scale.x == Approx(2.0f).margin(1e-5f));
    REQUIRE(transform.scale.y == Approx(2.0f).margin(1e-5f));
    REQUIRE(transform.scale.z == Approx(2.0f).margin(1e-5f));
}
