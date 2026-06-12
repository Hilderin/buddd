#include "scene/scene_loader.h"
#include "scene/world.h"
#include "scene/entity.h"
#include "scene/transform.h"
#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/register_all_components.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/component_registry/serialization.h"
#include "scene/camera_component.h"
#include "asset/asset_manager.h"
#include "engine_service.h"
#include "platform/platform.h"
#include "window/window.h"
#include "error.h"

#include <yaml-cpp/yaml.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

using namespace buddd::engine;
using Catch::Approx;

namespace {

constexpr float TOL = 1e-5f;

// Test engine with ComponentRegistry set up
struct TestEnv {
    std::unique_ptr<EngineService> engine;
    World world;

    TestEnv()
        : engine(EngineService::create(
              Backend::Headless,
              WindowConfig{"Test", 800, 600}).value())
    {}
};

/// Helper: create a YAML node for a camera component entry.
auto make_camera_component(double fov_y = 1.0, double aspect = 1.5,
                           double near_p = 0.1, double far_p = 100.0) -> YAML::Node
{
    YAML::Node comp;
    comp["type"] = "camera";
    comp["properties"]["fov_y"] = fov_y;
    comp["properties"]["aspect"] = aspect;
    comp["properties"]["near"] = near_p;
    comp["properties"]["far"] = far_p;
    return comp;
}

/// Helper: find the first entity that has a CameraComponent.
/// Returns Entity::none() if no such entity exists.
auto find_first_camera_entity(World& world) -> Entity {
    Entity found = Entity::none();
    world.each<CameraComponent>([&](Entity entity, CameraComponent&) {
        found = entity;
        return false; // stop after first
    });
    return found;
}

/// Helper: get the CameraComponent from an entity, or nullptr if not found.
auto get_camera_component(Entity entity) -> CameraComponent* {
    if (entity.id() == EntityId::none()) return nullptr;
    auto opt = entity.get_component<CameraComponent>();
    if (!opt) return nullptr;
    return &(*opt);
}

/// Helper: get the Transform reference from an entity (assumes entity is valid).
auto get_transform(Entity entity) -> Transform& {
    return entity.transform();
}

} // anonymous namespace

// ===========================================================================
// Test 1: Minimal scene loads one entity (AC-003, AC-006)
// ===========================================================================
TEST_CASE("Minimal scene loads one entity", "[scene_loader]") {
    TestEnv env;
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

    // Entity with a CameraComponent so we can find it via each<CameraComponent>
    YAML::Node node;
    node["type"] = "Scene";
    node["version"] = 1;
    node["entities"][0]["name"] = "test_camera";
    node["entities"][0]["components"].push_back(make_camera_component());

    auto result = loader.load_from_yaml(node);
    REQUIRE(result.has_value());

    // Verify at least one entity was created
    CHECK(env.world.entity_count() > 0);

    // Find the entity and verify its name
    Entity entity = find_first_camera_entity(env.world);
    REQUIRE(entity.id() != EntityId::none());
    CHECK(entity.name() == "test_camera");

    // Verify default transform values (entity has no transform: in YAML)
    Transform& t = get_transform(entity);
    CHECK(t.position.x == Approx(0.0f).margin(TOL));
    CHECK(t.position.y == Approx(0.0f).margin(TOL));
    CHECK(t.position.z == Approx(0.0f).margin(TOL));
    CHECK(t.rotation == math::Quat::identity());
    CHECK(t.scale.x == Approx(1.0f).margin(TOL));
    CHECK(t.scale.y == Approx(1.0f).margin(TOL));
    CHECK(t.scale.z == Approx(1.0f).margin(TOL));
}

// ===========================================================================
// Test 2: Entity name set via YAML (AC-004, AC-022)
// ===========================================================================
TEST_CASE("Entity name defaults to empty string and can be set via YAML", "[scene_loader]") {
    TestEnv env;

    // Test 2a: Entity created via World::add_entity() has empty name
    {
        World w;
        auto e = w.add_entity();
        REQUIRE(e.name() == "");
    }

    // Test 2b: Entity loaded with name: foo has name "foo"
    {
        SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

        YAML::Node node;
        node["type"] = "Scene";
        node["version"] = 1;
        node["entities"][0]["name"] = "foo";
        node["entities"][0]["components"].push_back(make_camera_component());

        auto result = loader.load_from_yaml(node);
        REQUIRE(result.has_value());

        Entity entity = find_first_camera_entity(env.world);
        REQUIRE(entity.id() != EntityId::none());
        CHECK(entity.name() == "foo");
    }

    // Test 2c: Entity loaded without name has empty name
    {
        World w2;
        SceneLoader loader2(w2, env.engine->registry(), env.engine->assets());

        YAML::Node node2;
        node2["type"] = "Scene";
        node2["version"] = 1;
        node2["entities"][0]["components"].push_back(make_camera_component());

        auto result2 = loader2.load_from_yaml(node2);
        REQUIRE(result2.has_value());

        Entity entity = find_first_camera_entity(w2);
        REQUIRE(entity.id() != EntityId::none());
        CHECK(entity.name() == "");
    }
}

// ===========================================================================
// Test 3: Transform parsing (AC-006)
// ===========================================================================
TEST_CASE("Transform parsing from YAML", "[scene_loader]") {
    TestEnv env;
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

    YAML::Node node;
    node["type"] = "Scene";
    node["version"] = 1;
    node["entities"][0]["name"] = "transformed_entity";
    node["entities"][0]["transform"]["position"] = YAML::Node(std::vector<float>{1.0f, 2.0f, 3.0f});
    node["entities"][0]["transform"]["rotation"] = YAML::Node(std::vector<float>{0.707f, 0.0f, 0.707f, 0.0f});
    node["entities"][0]["transform"]["scale"] = YAML::Node(std::vector<float>{2.0f, 2.0f, 2.0f});
    node["entities"][0]["components"].push_back(make_camera_component());

    auto result = loader.load_from_yaml(node);
    REQUIRE(result.has_value());

    Entity entity = find_first_camera_entity(env.world);
    REQUIRE(entity.id() != EntityId::none());

    Transform& t = get_transform(entity);
    CHECK(t.position.x == Approx(1.0f).margin(TOL));
    CHECK(t.position.y == Approx(2.0f).margin(TOL));
    CHECK(t.position.z == Approx(3.0f).margin(TOL));
    CHECK(t.rotation.w == Approx(0.707f).margin(TOL));
    CHECK(t.rotation.x == Approx(0.0f).margin(TOL));
    CHECK(t.rotation.y == Approx(0.707f).margin(TOL));
    CHECK(t.rotation.z == Approx(0.0f).margin(TOL));
    CHECK(t.scale.x == Approx(2.0f).margin(TOL));
    CHECK(t.scale.y == Approx(2.0f).margin(TOL));
    CHECK(t.scale.z == Approx(2.0f).margin(TOL));

    // Verify compose_transform directly
    Transform prefab;
    prefab.position = math::Vec3(3.0f, 2.0f, 3.0f);
    prefab.rotation = math::Quat::identity();
    prefab.scale = math::Vec3(1.0f, 1.0f, 1.0f);

    Transform instance;
    instance.position = math::Vec3(1.0f, 2.0f, 3.0f);
    instance.rotation = math::Quat::identity();
    instance.scale = math::Vec3(2.0f, 2.0f, 2.0f);

    Transform composed = SceneLoader::compose_transform(prefab, instance);
    CHECK(composed.position.x == Approx(4.0f).margin(TOL));
    CHECK(composed.position.y == Approx(4.0f).margin(TOL));
    CHECK(composed.position.z == Approx(6.0f).margin(TOL));
    CHECK(composed.scale.x == Approx(2.0f).margin(TOL));
    CHECK(composed.scale.y == Approx(2.0f).margin(TOL));
    CHECK(composed.scale.z == Approx(2.0f).margin(TOL));
    CHECK(composed.rotation == math::Quat::identity());
}

// ===========================================================================
// Test 4: Component deserialization (AC-007)
// ===========================================================================
TEST_CASE("Component deserialization from YAML", "[scene_loader]") {
    TestEnv env;
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

    YAML::Node node;
    node["type"] = "Scene";
    node["version"] = 1;
    node["entities"][0]["name"] = "camera_entity";
    node["entities"][0]["components"][0]["type"] = "camera";
    node["entities"][0]["components"][0]["properties"]["fov_y"] = 1.0;
    node["entities"][0]["components"][0]["properties"]["aspect"] = 1.5;
    node["entities"][0]["components"][0]["properties"]["near"] = 0.01;
    node["entities"][0]["components"][0]["properties"]["far"] = 200.0;

    auto result = loader.load_from_yaml(node);
    REQUIRE(result.has_value());

    // Find the camera entity and verify the fov_y property
    Entity entity = find_first_camera_entity(env.world);
    REQUIRE(entity.id() != EntityId::none());

    auto* camera = get_camera_component(entity);
    REQUIRE(camera != nullptr);
    CHECK(camera->fov_y() == Approx(1.0f).margin(TOL));
}

// ===========================================================================
// Test 5: Unknown component skipped with warning (AC-008)
// ===========================================================================
TEST_CASE("Unknown component types are skipped with warning", "[scene_loader]") {
    TestEnv env;
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

    YAML::Node node;
    node["type"] = "Scene";
    node["version"] = 1;
    node["entities"][0]["name"] = "mixed_entity";
    // Known component: camera
    node["entities"][0]["components"][0]["type"] = "camera";
    node["entities"][0]["components"][0]["properties"]["fov_y"] = 1.0;
    // Unknown component
    node["entities"][0]["components"][1]["type"] = "future_component";

    // Should succeed (unknown component is skipped with warning)
    auto result = loader.load_from_yaml(node);
    REQUIRE(result.has_value());

    // Verify the camera component exists
    Entity entity = find_first_camera_entity(env.world);
    REQUIRE(entity.id() != EntityId::none());

    auto* camera = get_camera_component(entity);
    REQUIRE(camera != nullptr);
    CHECK(camera->fov_y() == Approx(1.0f).margin(TOL));

    // Verify only one CameraComponent exists (unknown was skipped)
    size_t camera_count = env.world.each<CameraComponent>([](Entity, CameraComponent&) {
        return true; // count all
    });
    CHECK(camera_count == 1);
}

// ===========================================================================
// Test 6: Unknown YAML keys produce warning, not error (AC-009)
// ===========================================================================
TEST_CASE("Unknown YAML keys at entity level produce warning", "[scene_loader]") {
    TestEnv env;
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

    YAML::Node node;
    node["type"] = "Scene";
    node["version"] = 1;
    node["unexpected_key"] = "hello";
    node["entities"][0]["name"] = "test";
    node["entities"][0]["unknown_field"] = "world";
    node["entities"][0]["components"].push_back(make_camera_component());

    auto result = loader.load_from_yaml(node);
    // Should succeed — unknown keys produce warnings, not errors
    REQUIRE(result.has_value());

    // Entity should still have been created despite the unknown key
    Entity entity = find_first_camera_entity(env.world);
    REQUIRE(entity.id() != EntityId::none());
    CHECK(entity.name() == "test");
}

// ===========================================================================
// Test 7: Prefab transform composition via compose_transform()
// ===========================================================================
TEST_CASE("compose_transform produces correct composed transform", "[scene_loader]") {
    // Prefab transform
    Transform prefab;
    prefab.position = math::Vec3(3.0f, 2.0f, 3.0f);
    prefab.scale = math::Vec3(1.0f, 1.0f, 1.0f);
    prefab.rotation = math::Quat::identity();

    // Instance transform
    Transform instance;
    instance.position = math::Vec3(1.0f, 2.0f, 3.0f);
    instance.scale = math::Vec3(2.0f, 2.0f, 2.0f);
    instance.rotation = math::Quat::identity();

    Transform composed = SceneLoader::compose_transform(prefab, instance);

    // Position: prefab + instance = [4, 4, 6]
    CHECK(composed.position.x == Approx(4.0f).margin(TOL));
    CHECK(composed.position.y == Approx(4.0f).margin(TOL));
    CHECK(composed.position.z == Approx(6.0f).margin(TOL));

    // Scale: prefab * instance = [2, 2, 2]
    CHECK(composed.scale.x == Approx(2.0f).margin(TOL));
    CHECK(composed.scale.y == Approx(2.0f).margin(TOL));
    CHECK(composed.scale.z == Approx(2.0f).margin(TOL));

    // Rotation: identity * identity = identity
    CHECK(composed.rotation == math::Quat::identity());
}

// ===========================================================================
// Test 8: Children hierarchy (AC-023)
// ===========================================================================
TEST_CASE("Entities with children create hierarchy", "[scene_loader]") {
    TestEnv env;
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

    YAML::Node node;
    node["type"] = "Scene";
    node["version"] = 1;
    node["entities"][0]["name"] = "parent";
    node["entities"][0]["components"].push_back(make_camera_component());
    node["entities"][0]["children"][0]["name"] = "child1";
    node["entities"][0]["children"][1]["name"] = "child2";

    auto result = loader.load_from_yaml(node);
    REQUIRE(result.has_value());

    // Find the parent entity (has camera)
    Entity parent_entity = find_first_camera_entity(env.world);
    REQUIRE(parent_entity.id() != EntityId::none());
    CHECK(parent_entity.name() == "parent");

    // Verify children were created
    CHECK(parent_entity.child_count() == 2);

    // Verify first child
    Entity child1 = parent_entity.get_child(0);
    CHECK(child1.id() != EntityId::none());
    CHECK(child1.name() == "child1");
    CHECK(child1.parent() == parent_entity);

    // Verify second child
    Entity child2 = parent_entity.get_child(1);
    CHECK(child2.id() != EntityId::none());
    CHECK(child2.name() == "child2");
    CHECK(child2.parent() == parent_entity);
}

// ===========================================================================
// Test 9: Missing prefab file returns error (AC-010)
// ===========================================================================
TEST_CASE("Missing prefab file produces hard error", "[scene_loader]") {
    TestEnv env;
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

    YAML::Node node;
    node["type"] = "Scene";
    node["version"] = 1;
    // Prefab path that does not exist anywhere
    node["entities"][0]["prefab"] = "nonexistent_xyzzy_path_that_does_not_exist";

    auto result = loader.load_from_yaml(node);
    // Should fail — prefab file not found
    REQUIRE_FALSE(result.has_value());
}

// ===========================================================================
// Test 10: Prefab with >1 root entity returns error (AC-011)
// ===========================================================================
TEST_CASE("Prefab with multiple root entities returns error", "[scene_loader]") {
    // Create a temporary prefab file with 2 root entities.
    // Place it in the assets base path so the prefab resolver can find it.
    TestEnv env;
    std::string base_path(env.engine->assets().base_path());
    std::string prefab_path = base_path + "/buddd_test_multi_prefab.yaml";
    std::string scene_path = base_path + "/buddd_test_multi_scene.yaml";

    // Write the prefab file with 2 root entities
    {
        std::ofstream f(prefab_path);
        REQUIRE(f.is_open());
        f << "type: Prefab\nversion: 1\nentities:\n  - name: root1\n  - name: root2\n";
        f.close();
    }

    // Write a scene file that references this prefab
    {
        std::ofstream f(scene_path);
        REQUIRE(f.is_open());
        f << "type: Scene\nversion: 1\nentities:\n  - prefab: buddd_test_multi_prefab\n";
        f.close();
    }

    // Load the scene — should fail because prefab has 2 root entities
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());
    auto result = loader.load_from_file(scene_path);
    REQUIRE_FALSE(result.has_value());

    // Clean up temp files
    std::filesystem::remove(prefab_path);
    std::filesystem::remove(scene_path);
}
