#include "commands/add_component_command.h"
#include "commands/remove_component_command.h"
#include "editor.h"
#include "editor_context.h"
#include "editor_selection.h"
#include "scene/entity.h"
#include "scene/world.h"
#include "scene/entity_id.h"
#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/register_all_components.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/camera_component.h"
#include "scene/point_light_component.h"
#include "scene/directional_light_component.h"
#include "engine_context.h"
#include "engine_service.h"
#include "render/render_system.h"
#include "platform/platform.h"
#include "window/window.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <typeinfo>

namespace ed = buddd::editor;
namespace be = buddd::engine;
using Catch::Approx;

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
            be::WindowConfig{.title = "AddRemove Command Test", .width = 128, .height = 128});
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
        [[maybe_unused]] auto setup_result = editor->setup(*ctx);

        editor_ctx = std::make_unique<ed::EditorContext>(ed::EditorContext{
            *editor, *ctx});
    }
};

// ═════════════════════════════════════════════════════════════════════
// AddComponentCommand: compile check
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("AddComponentCommand: compile check",
          "[editor][add-remove-component]")
{
    auto cmd = std::make_unique<ed::AddComponentCommand>(
        be::EntityId{1, 0},
        std::string("camera")
    );
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->name() == "Add Component");
}

// ═════════════════════════════════════════════════════════════════════
// AddComponentCommand: execute creates component and stores index
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("AddComponentCommand: execute creates component and stores index",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();

    // Initial: no CameraComponent
    auto camera_before = entity.get_component<be::CameraComponent>();
    REQUIRE_FALSE(camera_before.has_value());

    size_t comp_count_before = entity.component_count();

    // Execute AddComponentCommand
    auto cmd = std::make_unique<ed::AddComponentCommand>(
        entity.id(), std::string("camera"));
    cmd->execute(*tctx.editor_ctx);

    // Verify CameraComponent now exists
    auto camera_after = entity.get_component<be::CameraComponent>();
    REQUIRE(camera_after.has_value());

    // Verify component count increased by 1
    REQUIRE(entity.component_count() == comp_count_before + 1);

    // Verify editor is dirty
    REQUIRE(tctx.editor->is_dirty());
}

// ═════════════════════════════════════════════════════════════════════
// AddComponentCommand: undo removes component at stored index
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("AddComponentCommand: undo removes component at stored index",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    size_t comp_count_before = entity.component_count();

    // Execute AddComponentCommand
    auto cmd = std::make_unique<ed::AddComponentCommand>(
        entity.id(), std::string("camera"));
    cmd->execute(*tctx.editor_ctx);
    REQUIRE(entity.component_count() == comp_count_before + 1);

    // Clear dirty flag to verify undo sets it
    tctx.editor->clear_dirty();

    // Undo — should remove the camera component
    cmd->undo(*tctx.editor_ctx);

    // Verify component count returns to original
    REQUIRE(entity.component_count() == comp_count_before);

    // Verify CameraComponent is gone
    auto camera_after = entity.get_component<be::CameraComponent>();
    REQUIRE_FALSE(camera_after.has_value());

    // Verify editor is dirty after undo
    REQUIRE(tctx.editor->is_dirty());
}

// ═════════════════════════════════════════════════════════════════════
// AddComponentCommand: safe with invalid entity
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("AddComponentCommand: safe with invalid entity",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto cmd = std::make_unique<ed::AddComponentCommand>(
        be::EntityId{999, 0}, std::string("camera"));
    REQUIRE_NOTHROW(cmd->execute(*tctx.editor_ctx));
}

// ═════════════════════════════════════════════════════════════════════
// AddComponentCommand: allows adding a second instance of same type
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("AddComponentCommand: allows adding a second instance of same type",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    entity.add_component<be::CameraComponent>();

    size_t comp_count_before = entity.component_count();

    auto cmd = std::make_unique<ed::AddComponentCommand>(
        entity.id(), std::string("camera"));
    cmd->execute(*tctx.editor_ctx);

    // Verify component count increased by 1 (duplicate NOT prevented)
    REQUIRE(entity.component_count() == comp_count_before + 1);
}

// ═════════════════════════════════════════════════════════════════════
// AddComponentCommand: unregistered type handled
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("AddComponentCommand: unregistered type handled",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();

    auto cmd = std::make_unique<ed::AddComponentCommand>(
        entity.id(), std::string("nonexistent"));
    REQUIRE_NOTHROW(cmd->execute(*tctx.editor_ctx));
}

// ═════════════════════════════════════════════════════════════════════
// AddComponentCommand: try_update_new_value returns false
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("AddComponentCommand: try_update_new_value returns false",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto cmd = std::make_unique<ed::AddComponentCommand>(
        be::EntityId{1, 0}, std::string("camera"));
    REQUIRE_FALSE(cmd->try_update_new_value(YAML::Node(), *tctx.editor_ctx, ""));
}

// ═════════════════════════════════════════════════════════════════════
// AddComponentCommand: execute marks dirty
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("AddComponentCommand: execute marks dirty",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();

    tctx.editor->clear_dirty();

    auto cmd = std::make_unique<ed::AddComponentCommand>(
        entity.id(), std::string("camera"));
    cmd->execute(*tctx.editor_ctx);

    REQUIRE(tctx.editor->is_dirty());
}

// ═════════════════════════════════════════════════════════════════════
// AddComponentCommand: undo also marks dirty
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("AddComponentCommand: undo also marks dirty",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();

    auto cmd = std::make_unique<ed::AddComponentCommand>(
        entity.id(), std::string("camera"));
    cmd->execute(*tctx.editor_ctx);

    tctx.editor->clear_dirty();

    cmd->undo(*tctx.editor_ctx);

    REQUIRE(tctx.editor->is_dirty());
}

// ═════════════════════════════════════════════════════════════════════
// AddComponentCommand: component index correctness after multi-add
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("AddComponentCommand: component index correctness after multi-add",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();

    // Add two components sequentially
    auto cmd1 = std::make_unique<ed::AddComponentCommand>(
        entity.id(), std::string("camera"));
    cmd1->execute(*tctx.editor_ctx);

    auto cmd2 = std::make_unique<ed::AddComponentCommand>(
        entity.id(), std::string("point_light"));
    cmd2->execute(*tctx.editor_ctx);

    // Verify CameraComponent is at index 0, PointLight at index 1
    REQUIRE(std::type_index(typeid(entity.component_at(0))) ==
            std::type_index(typeid(be::CameraComponent)));
    REQUIRE(std::type_index(typeid(entity.component_at(1))) ==
            std::type_index(typeid(be::PointLightComponent)));

    // Undo cmd1 (remove camera at index 0)
    cmd1->undo(*tctx.editor_ctx);

    // Now point_light should have shifted to index 0
    REQUIRE(entity.component_count() == 1);
    REQUIRE(std::type_index(typeid(entity.component_at(0))) ==
            std::type_index(typeid(be::PointLightComponent)));
}

// ═════════════════════════════════════════════════════════════════════
// RemoveComponentCommand: compile check
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("RemoveComponentCommand: compile check",
          "[editor][add-remove-component]")
{
    // Construct RemoveComponentCommand with mock data
    auto cmd = std::make_unique<ed::RemoveComponentCommand>(
        be::EntityId{1, 0}, std::string("camera"), 0);
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->name() == "Remove Component");
}

// ═════════════════════════════════════════════════════════════════════
// RemoveComponentCommand: execute with index 0 removes first component
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("RemoveComponentCommand: execute with index 0 removes first component",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    entity.add_component<be::CameraComponent>();
    entity.add_component<be::PointLightComponent>();
    REQUIRE(entity.component_count() == 2);

    // Remove camera at index 0
    auto cmd = std::make_unique<ed::RemoveComponentCommand>(
        entity.id(), std::string("camera"), 0);
    cmd->execute(*tctx.editor_ctx);

    // Verify only 1 component remains
    REQUIRE(entity.component_count() == 1);

    // Verify remaining component is PointLightComponent
    REQUIRE(std::type_index(typeid(entity.component_at(0))) ==
            std::type_index(typeid(be::PointLightComponent)));
}

// ═════════════════════════════════════════════════════════════════════
// RemoveComponentCommand: execute with index last removes last component
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("RemoveComponentCommand: execute with index last removes last component",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    entity.add_component<be::CameraComponent>();
    entity.add_component<be::PointLightComponent>();
    REQUIRE(entity.component_count() == 2);

    // Remove point_light at index 1 (last)
    auto cmd = std::make_unique<ed::RemoveComponentCommand>(
        entity.id(), std::string("point_light"), 1);
    cmd->execute(*tctx.editor_ctx);

    // Verify only 1 component remains
    REQUIRE(entity.component_count() == 1);

    // Verify remaining component is CameraComponent
    REQUIRE(std::type_index(typeid(entity.component_at(0))) ==
            std::type_index(typeid(be::CameraComponent)));
}

// ═════════════════════════════════════════════════════════════════════
// RemoveComponentCommand: undo restores at same position
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("RemoveComponentCommand: undo restores at same position",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    entity.add_component<be::CameraComponent>();
    auto& point_light = entity.add_component<be::PointLightComponent>();
    point_light.intensity() = 2.0f;

    // Verify intensity is 2.0
    REQUIRE(point_light.intensity() == Approx(2.0f).margin(1e-5f));

    // Remove point_light at index 1
    auto cmd = std::make_unique<ed::RemoveComponentCommand>(
        entity.id(), std::string("point_light"), 1);
    cmd->execute(*tctx.editor_ctx);
    REQUIRE(entity.component_count() == 1);

    // Undo — should restore point_light at same position with intensity=2.0
    cmd->undo(*tctx.editor_ctx);
    REQUIRE(entity.component_count() == 2);

    // Verify CameraComponent is still at index 0
    REQUIRE(std::type_index(typeid(entity.component_at(0))) ==
            std::type_index(typeid(be::CameraComponent)));

    // Verify PointLightComponent is back at index 1 with intensity=2.0
    REQUIRE(std::type_index(typeid(entity.component_at(1))) ==
            std::type_index(typeid(be::PointLightComponent)));
    auto& restored_light = static_cast<be::PointLightComponent&>(entity.component_at(1));
    REQUIRE(restored_light.intensity() == Approx(2.0f).margin(1e-5f));
}

// ═════════════════════════════════════════════════════════════════════
// RemoveComponentCommand: safe with invalid entity
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("RemoveComponentCommand: safe with invalid entity",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto cmd = std::make_unique<ed::RemoveComponentCommand>(
        be::EntityId{999, 0}, std::string("camera"), 0);
    REQUIRE_NOTHROW(cmd->execute(*tctx.editor_ctx));
}

// ═════════════════════════════════════════════════════════════════════
// RemoveComponentCommand: safe when index out of bounds
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("RemoveComponentCommand: safe when index out of bounds",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    entity.add_component<be::CameraComponent>();
    REQUIRE(entity.component_count() == 1);

    // Remove at index 5 which is out of bounds (entity has 1 component)
    auto cmd = std::make_unique<ed::RemoveComponentCommand>(
        entity.id(), std::string("camera"), 5);
    REQUIRE_NOTHROW(cmd->execute(*tctx.editor_ctx));

    // Entity should still have 1 component
    REQUIRE(entity.component_count() == 1);
}

// ═════════════════════════════════════════════════════════════════════
// RemoveComponentCommand: safe when type at index doesn't match
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("RemoveComponentCommand: safe when type at index doesn't match",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    entity.add_component<be::CameraComponent>();
    REQUIRE(entity.component_count() == 1);

    // Try to remove "point_light" at index 0 which is actually a CameraComponent
    auto cmd = std::make_unique<ed::RemoveComponentCommand>(
        entity.id(), std::string("point_light"), 0);
    REQUIRE_NOTHROW(cmd->execute(*tctx.editor_ctx));

    // Entity should still have 1 component (no-op due to type mismatch)
    REQUIRE(entity.component_count() == 1);
    REQUIRE(std::type_index(typeid(entity.component_at(0))) ==
            std::type_index(typeid(be::CameraComponent)));
}

// ═════════════════════════════════════════════════════════════════════
// RemoveComponentCommand: unregistered type handled
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("RemoveComponentCommand: unregistered type handled",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();

    auto cmd = std::make_unique<ed::RemoveComponentCommand>(
        entity.id(), std::string("nonexistent"), 0);
    REQUIRE_NOTHROW(cmd->execute(*tctx.editor_ctx));
}

// ═════════════════════════════════════════════════════════════════════
// RemoveComponentCommand: try_update_new_value returns false
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("RemoveComponentCommand: try_update_new_value returns false",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto cmd = std::make_unique<ed::RemoveComponentCommand>(
        be::EntityId{1, 0}, std::string("camera"), 0);
    REQUIRE_FALSE(cmd->try_update_new_value(YAML::Node(), *tctx.editor_ctx, ""));
}

// ═════════════════════════════════════════════════════════════════════
// RemoveComponentCommand: execute marks dirty
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("RemoveComponentCommand: execute marks dirty",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    entity.add_component<be::CameraComponent>();

    tctx.editor->clear_dirty();

    auto cmd = std::make_unique<ed::RemoveComponentCommand>(
        entity.id(), std::string("camera"), 0);
    cmd->execute(*tctx.editor_ctx);

    REQUIRE(tctx.editor->is_dirty());
}

// ═════════════════════════════════════════════════════════════════════
// RemoveComponentCommand: selection preserved after remove
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("RemoveComponentCommand: selection preserved after remove",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    entity.add_component<be::CameraComponent>();

    // Select the entity
    tctx.editor->selection().select(entity.id(), ed::SelectionModifier::Replace);
    REQUIRE(tctx.editor->selection().primary().has_value());
    REQUIRE(*tctx.editor->selection().primary() == entity.id());

    // Remove camera component
    auto cmd = std::make_unique<ed::RemoveComponentCommand>(
        entity.id(), std::string("camera"), 0);
    cmd->execute(*tctx.editor_ctx);

    // Entity should still be selected
    REQUIRE(tctx.editor->selection().primary().has_value());
    REQUIRE(*tctx.editor->selection().primary() == entity.id());
}

// ═════════════════════════════════════════════════════════════════════
// RemoveComponentCommand: undo also marks dirty
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("RemoveComponentCommand: undo also marks dirty",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    entity.add_component<be::CameraComponent>();

    auto cmd = std::make_unique<ed::RemoveComponentCommand>(
        entity.id(), std::string("camera"), 0);
    cmd->execute(*tctx.editor_ctx);

    tctx.editor->clear_dirty();

    cmd->undo(*tctx.editor_ctx);

    REQUIRE(tctx.editor->is_dirty());
}

// ═════════════════════════════════════════════════════════════════════
// World::remove_component_at: valid
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("World::remove_component_at: valid",
          "[engine][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    entity.add_component<be::CameraComponent>();
    REQUIRE(entity.component_count() == 1);

    bool result = world.remove_component_at(entity.id(), 0);
    REQUIRE(result);
    REQUIRE(entity.component_count() == 0);
}

// ═════════════════════════════════════════════════════════════════════
// World::remove_component_at: out of bounds
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("World::remove_component_at: out of bounds",
          "[engine][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    entity.add_component<be::CameraComponent>();
    REQUIRE(entity.component_count() == 1);

    // Try removing at index 5 (out of bounds)
    bool result = world.remove_component_at(entity.id(), 5);
    REQUIRE_FALSE(result);
    REQUIRE(entity.component_count() == 1);
}

// ═════════════════════════════════════════════════════════════════════
// World::remove_component_at: invalid entity
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("World::remove_component_at: invalid entity",
          "[engine][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    bool result = world.remove_component_at(be::EntityId::none(), 0);
    REQUIRE_FALSE(result);
}

// ═════════════════════════════════════════════════════════════════════
// World::remove_component_at: pending_destroy entity
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("World::remove_component_at: pending_destroy entity",
          "[engine][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    entity.add_component<be::CameraComponent>();
    REQUIRE(entity.component_count() == 1);

    entity.destroy();
    // Entity is pending destroy but not flushed yet
    bool result = world.remove_component_at(entity.id(), 0);
    REQUIRE_FALSE(result);
}

// ═════════════════════════════════════════════════════════════════════
// World::insert_component_raw_at: insert at 0
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("World::insert_component_raw_at: insert at 0",
          "[engine][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();

    auto camera = std::make_unique<be::CameraComponent>();
    auto& result = world.insert_component_raw_at(entity.id(), 0, std::move(camera));

    REQUIRE(entity.component_count() == 1);
    REQUIRE(std::type_index(typeid(result)) ==
            std::type_index(typeid(be::CameraComponent)));
    REQUIRE(std::type_index(typeid(entity.component_at(0))) ==
            std::type_index(typeid(be::CameraComponent)));
}

// ═════════════════════════════════════════════════════════════════════
// World::insert_component_raw_at: insert in middle
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("World::insert_component_raw_at: insert in middle",
          "[engine][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    entity.add_component<be::CameraComponent>();
    entity.add_component<be::PointLightComponent>();

    // Insert a DirectionalLightComponent at index 1 (between camera and point_light)
    auto dir_light = std::make_unique<be::DirectionalLightComponent>();
    world.insert_component_raw_at(entity.id(), 1, std::move(dir_light));

    REQUIRE(entity.component_count() == 3);

    // Verify ordering: camera, directional_light, point_light
    REQUIRE(std::type_index(typeid(entity.component_at(0))) ==
            std::type_index(typeid(be::CameraComponent)));
    REQUIRE(std::type_index(typeid(entity.component_at(1))) ==
            std::type_index(typeid(be::DirectionalLightComponent)));
    REQUIRE(std::type_index(typeid(entity.component_at(2))) ==
            std::type_index(typeid(be::PointLightComponent)));
}

// ═════════════════════════════════════════════════════════════════════
// World::insert_component_raw_at: insert at end
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("World::insert_component_raw_at: insert at end",
          "[engine][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    entity.add_component<be::CameraComponent>();

    auto dir_light = std::make_unique<be::DirectionalLightComponent>();
    world.insert_component_raw_at(entity.id(), 1, std::move(dir_light));

    REQUIRE(entity.component_count() == 2);

    // Verify: camera at 0, directional_light at 1 (appended)
    REQUIRE(std::type_index(typeid(entity.component_at(0))) ==
            std::type_index(typeid(be::CameraComponent)));
    REQUIRE(std::type_index(typeid(entity.component_at(1))) ==
            std::type_index(typeid(be::DirectionalLightComponent)));
}

// ═════════════════════════════════════════════════════════════════════
// World::insert_component_raw_at: insert past end
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("World::insert_component_raw_at: insert past end",
          "[engine][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    entity.add_component<be::CameraComponent>();

    // Insert at index 5 which is > component_count (1) — should clamp to end
    auto dir_light = std::make_unique<be::DirectionalLightComponent>();
    world.insert_component_raw_at(entity.id(), 5, std::move(dir_light));

    REQUIRE(entity.component_count() == 2);

    // Verify: camera at 0, directional_light at 1 (clamped to end)
    REQUIRE(std::type_index(typeid(entity.component_at(0))) ==
            std::type_index(typeid(be::CameraComponent)));
    REQUIRE(std::type_index(typeid(entity.component_at(1))) ==
            std::type_index(typeid(be::DirectionalLightComponent)));
}

// ═════════════════════════════════════════════════════════════════════
// World: remove then insert at same index
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("World: remove then insert at same index",
          "[engine][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();
    entity.add_component<be::CameraComponent>();
    entity.add_component<be::PointLightComponent>();
    REQUIRE(entity.component_count() == 2);

    // Remove component at index 0 (camera)
    bool removed = world.remove_component_at(entity.id(), 0);
    REQUIRE(removed);
    REQUIRE(entity.component_count() == 1);
    REQUIRE(std::type_index(typeid(entity.component_at(0))) ==
            std::type_index(typeid(be::PointLightComponent)));

    // Insert new camera at index 0
    auto new_camera = std::make_unique<be::CameraComponent>();
    world.insert_component_raw_at(entity.id(), 0, std::move(new_camera));

    // Verify camera is back at index 0, point_light at index 1
    REQUIRE(entity.component_count() == 2);
    REQUIRE(std::type_index(typeid(entity.component_at(0))) ==
            std::type_index(typeid(be::CameraComponent)));
    REQUIRE(std::type_index(typeid(entity.component_at(1))) ==
            std::type_index(typeid(be::PointLightComponent)));
}

// ═════════════════════════════════════════════════════════════════════
// Combined: add and remove cycle with undo/redo
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("Combined: add/remove cycle with undo/redo",
          "[editor][add-remove-component]")
{
    TestContext tctx;

    auto& world = tctx.editor->world();
    auto entity = world.add_entity();

    // Add CameraComponent
    auto add_cmd = std::make_unique<ed::AddComponentCommand>(
        entity.id(), std::string("camera"));
    add_cmd->execute(*tctx.editor_ctx);
    REQUIRE(entity.component_count() == 1);

    // Remove CameraComponent at index 0
    auto remove_cmd = std::make_unique<ed::RemoveComponentCommand>(
        entity.id(), std::string("camera"), 0);
    remove_cmd->execute(*tctx.editor_ctx);
    REQUIRE(entity.component_count() == 0);

    // Undo remove — camera should come back
    remove_cmd->undo(*tctx.editor_ctx);
    REQUIRE(entity.component_count() == 1);
    REQUIRE(std::type_index(typeid(entity.component_at(0))) ==
            std::type_index(typeid(be::CameraComponent)));

    // Undo add — camera should be removed
    add_cmd->undo(*tctx.editor_ctx);
    REQUIRE(entity.component_count() == 0);
}
