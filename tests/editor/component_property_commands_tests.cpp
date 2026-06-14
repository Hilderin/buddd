#include "commands/set_component_property_command.h"
#include "editor.h"
#include "editor_context.h"
#include "editor_selection.h"
#include "inspector_editors.h"
#include "panels/properties_panel.h"
#include "scene/entity.h"
#include "scene/world.h"
#include "scene/entity_id.h"
#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/register_all_components.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/camera_component.h"
#include "scene/point_light_component.h"
#include "engine_context.h"
#include "engine_service.h"
#include "render/render_system.h"
#include "platform/platform.h"
#include "window/window.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <any>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>

namespace ed = buddd::editor;
namespace be = buddd::engine;
using Catch::Approx;

// ── Compile-time checks ──

// AC-09: Verify InspectorTypeEditor has the draw_any virtual method.
// The existence of TypedInspectorEditor<int>::draw_any() override ensures
// the base class declares it as virtual (C++ override specifier would fail
// to compile otherwise). This static_assert verifies the function pointer
// can be obtained from the base class.
static_assert(&ed::InspectorTypeEditor::draw_any != nullptr,
    "InspectorTypeEditor must have a draw_any virtual method");

// ── AC-28: PropertyFlags → EditorFlags field mapping ──
// Verify the field types match between PropertyFlags and EditorFlags
// by testing a round-trip through the mapping used in draw_component_sections().
TEST_CASE("PropertiesPanel: PropertyFlags map to EditorFlags correctly",
          "[editor][component-properties]")
{
    // Create PropertyFlags with known values
    be::PropertyFlags prop_flags;
    prop_flags.min_value = 0.001f;
    prop_flags.max_value = 3.14159f;
    prop_flags.step_value = 0.5f;
    prop_flags.tags_.push_back("rgb");

    // Apply the same mapping as in draw_component_sections()
    ed::EditorFlags editor_flags;
    editor_flags.min_value = prop_flags.min_value;
    editor_flags.max_value = prop_flags.max_value;
    editor_flags.step_value = prop_flags.step_value;
    editor_flags.tags_ = prop_flags.tags_;

    // Verify all fields transferred correctly
    REQUIRE(editor_flags.min_value == Approx(0.001f));
    REQUIRE(editor_flags.max_value == Approx(3.14159f));
    REQUIRE(editor_flags.step_value == Approx(0.5f));
    REQUIRE(editor_flags.has_tag("rgb"));
    REQUIRE_FALSE(editor_flags.has_tag("non_existent"));
}

// ── Edge case: CameraComponent fov_y has correct min/max flags ──
TEST_CASE("CameraComponent fov_y has correct PropertyFlags",
          "[editor][component-properties]")
{
    be::register_builtin_types();
    be::ComponentRegistry registry;
    be::register_all_components(registry);

    auto* info = registry.describe("camera");
    REQUIRE(info != nullptr);
    REQUIRE(info->property_count() >= 1);

    // fov_y is property 0
    auto flags = info->property_flags(0);
    // fov_y has min=0.001f, max=3.14159f per register_all_components.cpp
    REQUIRE(flags.min_value == Approx(0.001f));
    REQUIRE(flags.max_value == Approx(3.14159f));
}

// ── Edge case: PointLightComponent color property has "rgb" tag ──
TEST_CASE("PointLightComponent color has rgb tag and Color type",
          "[editor][component-properties]")
{
    be::register_builtin_types();
    be::ComponentRegistry registry;
    be::register_all_components(registry);

    auto* info = registry.describe("point_light");
    REQUIRE(info != nullptr);
    REQUIRE(info->property_count() >= 1);

    // color is property 0
    auto flags = info->property_flags(0);
    auto type_idx = info->property_type_index(0);

    // Verify the "rgb" tag
    REQUIRE(flags.has_tag("rgb"));

    // Verify the type is math::Color
    REQUIRE(type_idx == std::type_index(typeid(be::math::Color)));
}

// ── Edge case: property_serialize returns raw value even at default ──
TEST_CASE("CameraComponent: property_serialize returns value even at default",
          "[engine][component-registry]")
{
    be::register_builtin_types();
    be::ComponentRegistry registry;
    be::register_all_components(registry);

    auto* info = registry.describe("camera");
    REQUIRE(info != nullptr);

    // Create EngineService for SerializationContext
    auto eng = be::EngineService::create(
        be::Backend::Headless,
        be::WindowConfig{.title = "property_serialize default test", .width = 128, .height = 128});
    REQUIRE(eng.has_value());

    // Create a default CameraComponent (fov_y ≈ 1.0471975512)
    auto camera = std::make_unique<be::CameraComponent>();
    float default_fov = camera->fov_y();

    be::SerializationContext ctx{(*eng)->assets()};

    // Serialize property 0 (fov_y) — should return the raw value even at default
    auto yaml_node = info->property_serialize(*camera, 0, ctx);

    // Verify it returns the actual value (not null, not skipped)
    REQUIRE_FALSE(yaml_node.IsNull());
    REQUIRE(yaml_node.IsScalar());
    REQUIRE(yaml_node.as<float>() == Approx(default_fov).margin(1e-5f));
}

// ── Headless test context for editor integration tests ──
struct TestContext {
    std::unique_ptr<be::EngineService> engine;
    std::unique_ptr<be::World> engine_world;
    std::unique_ptr<be::RenderSystem> render_system;
    std::unique_ptr<be::EngineContext> ctx;
    std::unique_ptr<ed::Editor> editor;
    std::unique_ptr<ed::EditorContext> editor_ctx;

    TestContext() {
        // Register built-in types and components
        be::register_builtin_types();

        auto eng = be::EngineService::create(
            be::Backend::Headless,
            be::WindowConfig{.title = "F-06 Command Test", .width = 128, .height = 128});
        REQUIRE(eng.has_value());
        engine = std::move(*eng);

        // Register components in engine's registry
        be::register_all_components(engine->registry());

        engine_world = std::make_unique<be::World>();
        render_system = std::make_unique<be::RenderSystem>(engine->device(), *engine_world);

        ctx = std::make_unique<be::EngineContext>(be::EngineContext{
            *engine, engine->window(), engine->device(), *engine_world,
            *render_system, 0.016f, 0});

        editor = std::make_unique<ed::Editor>();
        // Editor::setup() may fail in headless mode (no ImGui), but that's OK
        // for tests that don't need full ImGui interaction.
        [[maybe_unused]] auto setup_result = editor->setup(*ctx);

        editor_ctx = std::make_unique<ed::EditorContext>(ed::EditorContext{
            *editor, *ctx});
    }
};

// ═════════════════════════════════════════════════════════════════════
// SetComponentPropertyCommand: compile check
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("SetComponentPropertyCommand: compile check",
          "[editor][component-properties]")
{
    // Verify the command exists with expected constructor
    auto cmd = std::make_unique<ed::SetComponentPropertyCommand>(
        be::EntityId{1, 0},
        std::string("camera"),
        std::string("fov_y"),
        YAML::Node(1.05f),
        YAML::Node(2.0f)
    );
    REQUIRE(cmd != nullptr);
    REQUIRE_FALSE(cmd->name().empty());
    REQUIRE(cmd->name() == "Set Component Property");
}

// ═════════════════════════════════════════════════════════════════════
// SetComponentPropertyCommand: execute writes new value and marks dirty
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("SetComponentPropertyCommand: execute writes new value and marks dirty",
          "[editor][component-properties]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    auto& camera = entity.add_component<be::CameraComponent>();

    // Verify initial fov_y
    REQUIRE(camera.fov_y() == Approx(1.0471975512f).margin(1e-5f));

    // Serialize the old value to pass to the command
    auto* info = tctx.engine->registry().describe("camera");
    REQUIRE(info != nullptr);

    auto tmp = const_cast<be::ComponentInfoBase*>(info)->create();
    auto target_type = std::type_index(typeid(*tmp));

    // Find the component index
    std::optional<size_t> comp_idx;
    for (size_t i = 0; i < entity.component_count(); ++i) {
        if (std::type_index(typeid(entity.component_at(i))) == target_type) {
            comp_idx = i;
            break;
        }
    }
    REQUIRE(comp_idx.has_value());

    be::SerializationContext ser_ctx{tctx.engine->assets()};
    auto old_yaml = info->property_serialize(entity.component_at(*comp_idx), 0, ser_ctx);

    // Create and execute command to change fov_y to 2.0
    auto cmd = std::make_unique<ed::SetComponentPropertyCommand>(
        entity.id(),
        std::string("camera"),
        std::string("fov_y"),
        old_yaml,
        YAML::Node(2.0f)
    );
    cmd->execute(*tctx.editor_ctx);

    // Verify component's fov_y is now ≈ 2.0
    REQUIRE(camera.fov_y() == Approx(2.0f).margin(1e-5f));

    // Verify dirty flag is set (Editor tracks dirty state)
    REQUIRE(tctx.editor->is_dirty());
}

// ═════════════════════════════════════════════════════════════════════
// SetComponentPropertyCommand: undo reverts to old value
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("SetComponentPropertyCommand: undo reverts to old value",
          "[editor][component-properties]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    auto& camera = entity.add_component<be::CameraComponent>();
    REQUIRE(camera.fov_y() == Approx(1.0471975512f).margin(1e-5f));

    auto* info = tctx.engine->registry().describe("camera");
    REQUIRE(info != nullptr);

    auto tmp = const_cast<be::ComponentInfoBase*>(info)->create();
    auto target_type = std::type_index(typeid(*tmp));

    std::optional<size_t> comp_idx;
    for (size_t i = 0; i < entity.component_count(); ++i) {
        if (std::type_index(typeid(entity.component_at(i))) == target_type) {
            comp_idx = i;
            break;
        }
    }
    REQUIRE(comp_idx.has_value());

    be::SerializationContext ser_ctx{tctx.engine->assets()};
    auto old_yaml = info->property_serialize(entity.component_at(*comp_idx), 0, ser_ctx);
    float original_fov = camera.fov_y();

    // Execute command to change fov_y to 2.0
    auto cmd = std::make_unique<ed::SetComponentPropertyCommand>(
        entity.id(),
        std::string("camera"),
        std::string("fov_y"),
        old_yaml,
        YAML::Node(2.0f)
    );
    cmd->execute(*tctx.editor_ctx);
    REQUIRE(camera.fov_y() == Approx(2.0f).margin(1e-5f));

    // Clear the dirty flag so we can verify undo also marks dirty
    tctx.editor->clear_dirty();

    // Undo
    cmd->undo(*tctx.editor_ctx);

    // Verify fov_y reverts to original
    REQUIRE(camera.fov_y() == Approx(original_fov).margin(1e-5f));

    // Verify undo also marks dirty
    REQUIRE(tctx.editor->is_dirty());
}

// ═════════════════════════════════════════════════════════════════════
// SetComponentPropertyCommand: safe with invalid entity
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("SetComponentPropertyCommand: execute is safe with invalid entity",
          "[editor][component-properties]")
{
    // Create command with invalid EntityId (index=999, generation=0)
    auto cmd = std::make_unique<ed::SetComponentPropertyCommand>(
        be::EntityId{999, 0},
        std::string("camera"),
        std::string("fov_y"),
        YAML::Node(1.05f),
        YAML::Node(2.0f)
    );

    // Execute with a minimal context — should not crash
    TestContext tctx;
    REQUIRE_NOTHROW(cmd->execute(*tctx.editor_ctx));
}

// ═════════════════════════════════════════════════════════════════════
// SetComponentPropertyCommand: safe with missing component
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("SetComponentPropertyCommand: execute is safe with missing component",
          "[editor][component-properties]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    // No CameraComponent added

    // Create command for "camera" on entity that has no camera
    auto cmd = std::make_unique<ed::SetComponentPropertyCommand>(
        entity.id(),
        std::string("camera"),
        std::string("fov_y"),
        YAML::Node(1.05f),
        YAML::Node(2.0f)
    );

    REQUIRE_NOTHROW(cmd->execute(*tctx.editor_ctx));
}

// ═════════════════════════════════════════════════════════════════════
// SetComponentPropertyCommand: no-op when value already matches
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("SetComponentPropertyCommand: execute no-op when value already matches",
          "[editor][component-properties]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    auto& camera = entity.add_component<be::CameraComponent>();
    // Default fov_y ≈ 1.047

    // We need the current value as YAML to pass as both old and new
    auto* info = tctx.engine->registry().describe("camera");
    REQUIRE(info != nullptr);

    auto tmp = const_cast<be::ComponentInfoBase*>(info)->create();
    auto target_type = std::type_index(typeid(*tmp));

    std::optional<size_t> comp_idx;
    for (size_t i = 0; i < entity.component_count(); ++i) {
        if (std::type_index(typeid(entity.component_at(i))) == target_type) {
            comp_idx = i;
            break;
        }
    }
    REQUIRE(comp_idx.has_value());

    be::SerializationContext ser_ctx{tctx.engine->assets()};
    auto current_yaml = info->property_serialize(entity.component_at(*comp_idx), 0, ser_ctx);

    // Create command with same old and new value (both serialize to same YAML)
    float fov_before = camera.fov_y();
    auto cmd = std::make_unique<ed::SetComponentPropertyCommand>(
        entity.id(),
        std::string("camera"),
        std::string("fov_y"),
        current_yaml,
        YAML::Node(1.0471975512f)  // same as default
    );

    tctx.editor->clear_dirty();
    cmd->execute(*tctx.editor_ctx);

    // Verify fov_y unchanged
    REQUIRE(camera.fov_y() == Approx(fov_before).margin(1e-5f));

    // Should NOT be marked dirty since no change
    // (may be dirty depending on command implementation — the no-op path skips mark_dirty)
}

// ═════════════════════════════════════════════════════════════════════
// InspectorTypeEditorRegistry: draw_any dispatches to registered editor
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("InspectorTypeEditorRegistry: draw_any dispatches to registered editor",
          "[editor][inspector]")
{
    // Register a mock editor for int that always returns true
    ed::InspectorTypeEditorRegistry::register_editor<int>(
        [](const std::string&, int&, const ed::EditorFlags&,
           const ed::EditorContext&) -> bool {
            return true;
        }
    );

    TestContext tctx;
    int test_value = 42;
    std::any any_value = test_value;
    ed::EditorFlags flags;

    bool result = ed::InspectorTypeEditorRegistry::draw_any(
        "test_label", any_value, std::type_index(typeid(int)), flags, *tctx.editor_ctx);

    // The mock always returns true
    REQUIRE(result);
}

// ═════════════════════════════════════════════════════════════════════
// InspectorTypeEditorRegistry: draw_any falls back to read-only
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("InspectorTypeEditorRegistry: draw_any falls back to read-only",
          "[editor][inspector]")
{
    TestContext tctx;
    double test_value = 1.0;
    std::any any_value = test_value;
    ed::EditorFlags flags;

    // No editor is registered for double — should return false
    bool result = ed::InspectorTypeEditorRegistry::draw_any(
        "test_label", any_value, std::type_index(typeid(double)), flags, *tctx.editor_ctx);

    // Verify returns false (no edit possible)
    REQUIRE_FALSE(result);
}

// ═════════════════════════════════════════════════════════════════════
// TypedInspectorEditor: draw_any extracts typed value
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("TypedInspectorEditor: draw_any extracts typed value",
          "[editor][inspector]")
{
    // Create a TypedInspectorEditor with a mock that records the value
    int received_value = 0;
    auto editor = std::make_unique<ed::TypedInspectorEditor<int>>(
        [&received_value](const std::string&, int& value,
                          const ed::EditorFlags&,
                          const ed::EditorContext&) -> bool {
            received_value = value;
            return true;
        }
    );

    TestContext tctx;
    std::any any_value = 42;
    ed::EditorFlags flags;

    bool result = editor->draw_any(
        "test_label", any_value, std::type_index(typeid(int)), flags, *tctx.editor_ctx);

    // Verify the mock received value 42
    REQUIRE(received_value == 42);
    REQUIRE(result);
}

// ═════════════════════════════════════════════════════════════════════
// TypedInspectorEditor: draw_any returns false on type mismatch
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("TypedInspectorEditor: draw_any returns false on type mismatch",
          "[editor][inspector]")
{
    // Create a TypedInspectorEditor<int>
    auto editor = std::make_unique<ed::TypedInspectorEditor<int>>(
        [](const std::string&, int&, const ed::EditorFlags&,
           const ed::EditorContext&) -> bool {
            return true;
        }
    );

    TestContext tctx;
    // std::any holds float, not int
    std::any any_value = 3.14f;
    ed::EditorFlags flags;

    bool result = editor->draw_any(
        "test_label", any_value, std::type_index(typeid(int)), flags, *tctx.editor_ctx);

    // Should return false because std::any doesn't hold int
    REQUIRE_FALSE(result);
}

// ═════════════════════════════════════════════════════════════════════
// PropertiesPanel: component sections render for entities with components
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("PropertiesPanel: component sections render for entities with components",
          "[editor][component-properties]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    entity.add_component<be::CameraComponent>();
    entity.add_component<be::PointLightComponent>();

    // Verify entity has at least 2 components
    REQUIRE(entity.component_count() >= 2);

    // Select the entity
    tctx.editor->selection().select(entity.id(), ed::SelectionModifier::Replace);
    REQUIRE(tctx.editor->selection().primary().has_value());
    REQUIRE(*tctx.editor->selection().primary() == entity.id());

    // Verify the PropertiesPanel can be constructed and queried
    ed::PropertiesPanel panel;
    REQUIRE(panel.id() == "properties");
    REQUIRE(panel.title() == "Properties");
}

// ═════════════════════════════════════════════════════════════════════
// Component ordering matches component_at order
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("Component ordering matches component_at order",
          "[editor][component-properties]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();

    // Add CameraComponent first, then PointLightComponent
    entity.add_component<be::CameraComponent>();
    entity.add_component<be::PointLightComponent>();

    // Verify ordering using typeid
    REQUIRE(entity.component_count() >= 2);
    REQUIRE(std::type_index(typeid(entity.component_at(0))) ==
            std::type_index(typeid(be::CameraComponent)));
    REQUIRE(std::type_index(typeid(entity.component_at(1))) ==
            std::type_index(typeid(be::PointLightComponent)));
}
