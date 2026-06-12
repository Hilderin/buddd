#include "scene/scene_saver.h"
#include "scene/scene_loader.h"
#include "scene/world.h"
#include "scene/entity.h"
#include "scene/entity_source.h"
#include "scene/transform.h"
#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/register_all_components.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/component_registry/serialization.h"
#include "scene/camera_component.h"
#include "scene/directional_light_component.h"
#include "scene/free_camera_movement.h"
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

struct TestEnv {
    std::unique_ptr<EngineService> engine;
    World world;

    TestEnv()
        : engine(EngineService::create(
              Backend::Headless,
              WindowConfig{"Test", 800, 600}).value())
    {}
};
} // anonymous namespace

// ===========================================================================
// Test 1: Default source for directly-created entity (AC-005)
// ===========================================================================
TEST_CASE("Default source for directly-created entity", "[scene_saver]") {
    TestEnv env;
    auto entity = env.world.add_entity();

    CHECK(entity.source().type == EntitySourceType::None);
    CHECK(entity.source().path == "");
}

// ===========================================================================
// Test 2: Entity source can be set and retrieved (AC-017)
// ===========================================================================
TEST_CASE("Entity source can be set and retrieved", "[scene_saver]") {
    TestEnv env;
    auto entity = env.world.add_entity();

    entity.set_source(EntitySource{EntitySourceType::Model, "models/box/Box"});

    CHECK(entity.source().type == EntitySourceType::Model);
    CHECK(entity.source().path == "models/box/Box");

    // Verify const& access (no dangling reference)
    const auto& src = entity.source();
    CHECK(src.type == EntitySourceType::Model);
    CHECK(src.path == "models/box/Box");
}

// ===========================================================================
// Test 3: Prefab source tracking via SceneLoader (AC-006)
// ===========================================================================
TEST_CASE("Prefab source tracking via SceneLoader", "[scene_saver]") {
    TestEnv env;
    std::string base_path(env.engine->assets().base_path());
    std::string prefab_path = base_path + "/buddd_test_prefab.yaml";

    // Write prefab file
    {
        std::ofstream f(prefab_path);
        REQUIRE(f.is_open());
        f << "type: Prefab\nversion: 1\nentities:\n  - name: test_prefab_root\n";
        f.close();
    }

    // Load a scene that references the prefab
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());
    YAML::Node scene_node;
    scene_node["type"] = "Scene";
    scene_node["version"] = 1;
    scene_node["entities"][0]["prefab"] = "buddd_test_prefab";
    scene_node["entities"][0]["name"] = "instance";

    auto result = loader.load_from_yaml(scene_node);
    REQUIRE(result.has_value());

    // Find the entity by name
    Entity entity = Entity::none();
    for (size_t i = 0; i < env.world.root_entity_count(); ++i) {
        auto e = env.world.get_root_entity(i);
        if (e.name() == "instance") {
            entity = e;
            break;
        }
    }

    REQUIRE(entity.id() != EntityId::none());
    CHECK(entity.source().type == EntitySourceType::Prefab);
    CHECK_FALSE(entity.source().path.empty());

    // Clean up
    std::filesystem::remove(prefab_path);
}

// ===========================================================================
// Test 4: Model source tracking via SceneLoader (AC-007)
// ===========================================================================
TEST_CASE("Model source tracking via SceneLoader", "[scene_saver]") {
    TestEnv env;
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

    YAML::Node node;
    node["type"] = "Scene";
    node["version"] = 1;
    node["entities"][0]["name"] = "box_entity";
    node["entities"][0]["model"] = "models/box/Box";

    auto result = loader.load_from_yaml(node);
    if (!result) {
        WARN("Model asset 'models/box/Box' not available, skipping model source test");
    } else {
        // Find the entity by root iteration
        Entity entity = Entity::none();
        for (size_t i = 0; i < env.world.root_entity_count(); ++i) {
            auto e = env.world.get_root_entity(i);
            if (e.name() == "box_entity") {
                entity = e;
                break;
            }
        }
        REQUIRE(entity.id() != EntityId::none());
        CHECK(entity.source().type == EntitySourceType::Model);
        CHECK(entity.source().path == "models/box/Box");
    }
}

// ===========================================================================
// Test 4b: Model child entities have None source (AC-008)
// ===========================================================================
TEST_CASE("Model child entities have None source", "[scene_saver]") {
    TestEnv env;
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

    YAML::Node node;
    node["type"] = "Scene";
    node["version"] = 1;
    node["entities"][0]["name"] = "box_container";
    node["entities"][0]["model"] = "models/box/Box";

    auto result = loader.load_from_yaml(node);
    if (!result) {
        WARN("Model asset 'models/box/Box' not available, skipping model child source test");
    } else {
        // Find the container entity
        Entity container = Entity::none();
        for (size_t i = 0; i < env.world.root_entity_count(); ++i) {
            auto e = env.world.get_root_entity(i);
            if (e.name() == "box_container") {
                container = e;
                break;
            }
        }
        REQUIRE(container.id() != EntityId::none());

        // Check each child has None source
        for (size_t i = 0; i < container.child_count(); ++i) {
            auto child = container.get_child(i);
            CHECK(child.source().type == EntitySourceType::None);
            CHECK(child.source().path == "");
        }
    }
}

// ===========================================================================
// Test 5: Save empty World (AC-010, AC-019)
// ===========================================================================
TEST_CASE("Save empty World", "[scene_saver]") {
    TestEnv env;
    SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());

    YAML::Node saved = saver.save_to_yaml();

    CHECK(saved["type"].as<std::string>() == "Scene");
    CHECK(saved["version"].as<int>() == 1);
    REQUIRE(saved["entities"].IsSequence());
    CHECK(saved["entities"].size() == 0);
}

// ===========================================================================
// Test 6: Save None-source entity with components (AC-011, AC-015)
// ===========================================================================
TEST_CASE("Save None-source entity with components", "[scene_saver]") {
    TestEnv env;
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

    YAML::Node node;
    node["type"] = "Scene";
    node["version"] = 1;
    node["entities"][0]["name"] = "camera_entity";
    node["entities"][0]["transform"]["position"] = YAML::Node(std::vector<float>{1.0f, 0.0f, 0.0f});
    node["entities"][0]["components"][0]["type"] = "camera";
    node["entities"][0]["components"][0]["properties"]["fov_y"] = 1.0;
    node["entities"][0]["components"][0]["properties"]["aspect"] = 1.5;

    auto load_result = loader.load_from_yaml(node);
    REQUIRE(load_result.has_value());

    SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
    YAML::Node saved = saver.save_to_yaml();

    REQUIRE(saved["type"].as<std::string>() == "Scene");
    REQUIRE(saved["version"].as<int>() == 1);
    REQUIRE(saved["entities"].IsSequence());
    REQUIRE(saved["entities"].size() == 1);

    auto ent = saved["entities"][0];
    CHECK(ent["name"].as<std::string>() == "camera_entity");
    CHECK(ent["transform"].IsDefined());
    CHECK(ent["components"].IsSequence());
    CHECK(ent["components"].size() == 1);
    CHECK(ent["components"][0]["type"].as<std::string>() == "camera");
    CHECK(ent["components"][0]["properties"]["fov_y"].as<double>() == Approx(1.0));
}

// ===========================================================================
// Test 7: Save Prefab source entity as reference (AC-012)
// ===========================================================================
TEST_CASE("Save Prefab source entity as reference", "[scene_saver]") {
    TestEnv env;
    auto entity = env.world.add_entity();
    entity.set_name("main_camera");
    entity.set_source(EntitySource{EntitySourceType::Prefab, "prefabs/free_camera"});
    entity.transform().position.x = 1.0f;  // Non-default transform so key appears

    SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
    YAML::Node saved = saver.save_to_yaml();

    REQUIRE(saved["entities"].IsSequence());
    REQUIRE(saved["entities"].size() == 1);

    auto ent = saved["entities"][0];
    CHECK(ent["prefab"].as<std::string>() == "prefabs/free_camera");
    CHECK(ent["name"].as<std::string>() == "main_camera");
    CHECK(ent["transform"].IsDefined());
    CHECK_FALSE(ent["components"].IsDefined());
    CHECK_FALSE(ent["children"].IsDefined());
}

// Test 7b: Prefab source with resolved path is sanitized (assets/ prefix + .yaml ext stripped)
TEST_CASE("Save prefab source with resolved path sanitizes correctly", "[scene_saver]") {
    TestEnv env;
    auto entity = env.world.add_entity();
    entity.set_name("camera_from_resolved");
    // Simulate what SceneLoader stores: resolved path with base_path + "/" + ext
    entity.set_source(EntitySource{EntitySourceType::Prefab,
        std::string(env.engine->assets().base_path()) + "/prefabs/free_camera.yaml"});

    SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
    YAML::Node saved = saver.save_to_yaml();

    REQUIRE(saved["entities"].IsSequence());
    REQUIRE(saved["entities"].size() == 1);

    auto ent = saved["entities"][0];
    // Must be saved as relative path without .yaml extension
    CHECK(ent["prefab"].as<std::string>() == "prefabs/free_camera");
    CHECK(ent["name"].as<std::string>() == "camera_from_resolved");
}

// ===========================================================================
// Test 8: Save Model source entity as reference (AC-013)
// ===========================================================================
TEST_CASE("Save Model source entity as reference", "[scene_saver]") {
    TestEnv env;
    auto entity = env.world.add_entity();
    entity.set_name("my_box");
    entity.set_source(EntitySource{EntitySourceType::Model, "models/box/Box"});
    entity.transform().position.x = 1.0f;  // Non-default transform so key appears

    SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
    YAML::Node saved = saver.save_to_yaml();

    REQUIRE(saved["entities"].IsSequence());
    REQUIRE(saved["entities"].size() == 1);

    auto ent = saved["entities"][0];
    CHECK(ent["name"].as<std::string>() == "my_box");
    CHECK(ent["model"].as<std::string>() == "models/box/Box");
    CHECK(ent["transform"].IsDefined());
    CHECK_FALSE(ent["children"].IsDefined());
}

// ===========================================================================
// Test 9: Round-trip save then load (AC-016)
// ===========================================================================
TEST_CASE("Round-trip save then load", "[scene_saver]") {
    TestEnv env;

    // Load initial scene
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());
    YAML::Node node;
    node["type"] = "Scene";
    node["version"] = 1;
    node["entities"][0]["name"] = "test_entity";
    node["entities"][0]["transform"]["position"] = YAML::Node(std::vector<float>{1.0f, 2.0f, 3.0f});
    node["entities"][0]["components"][0]["type"] = "camera";
    node["entities"][0]["components"][0]["properties"]["fov_y"] = 1.0;
    node["entities"][0]["components"][0]["properties"]["aspect"] = 1.5;

    auto load_result = loader.load_from_yaml(node);
    REQUIRE(load_result.has_value());

    // Save
    SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
    YAML::Node saved = saver.save_to_yaml();

    // Load into fresh World
    World world2;
    SceneLoader loader2(world2, env.engine->registry(), env.engine->assets());
    auto reload_result = loader2.load_from_yaml(saved);
    REQUIRE(reload_result.has_value());

    // Verify properties
    CHECK(world2.entity_count() == env.world.entity_count());
    // Find entity in world2 by name
    Entity found = Entity::none();
    for (size_t i = 0; i < world2.root_entity_count(); ++i) {
        auto e = world2.get_root_entity(i);
        if (e.name() == "test_entity") {
            found = e;
            break;
        }
    }
    REQUIRE(found.id() != EntityId::none());

    // Verify source type is None (direct entity)
    CHECK(found.source().type == EntitySourceType::None);

    // Verify transform
    auto& t = found.transform();
    CHECK(t.position.x == Approx(1.0f).margin(TOL));
    CHECK(t.position.y == Approx(2.0f).margin(TOL));
    CHECK(t.position.z == Approx(3.0f).margin(TOL));

    // Verify component
    auto cam = found.get_component<CameraComponent>();
    REQUIRE(cam.has_value());
    CHECK(cam->fov_y() == Approx(1.0f).margin(TOL));
}

// ===========================================================================
// Test 10: Save entity with no components omits components key (AC-011 edge)
// ===========================================================================
TEST_CASE("Save entity with no components omits components key", "[scene_saver]") {
    TestEnv env;
    auto entity = env.world.add_entity();
    entity.set_name("simple_entity");
    entity.transform().position.x = 1.0f;  // Non-default transform so key appears

    SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
    YAML::Node saved = saver.save_to_yaml();

    REQUIRE(saved["entities"].IsSequence());
    REQUIRE(saved["entities"].size() == 1);

    auto ent = saved["entities"][0];
    CHECK(ent["name"].as<std::string>() == "simple_entity");
    CHECK(ent["transform"].IsDefined());
    CHECK_FALSE(ent["components"].IsDefined());
    CHECK_FALSE(ent["children"].IsDefined());
}

// ===========================================================================
// Test 11: save_to_file() writes valid YAML to disk (AC-014)
// ===========================================================================
TEST_CASE("save_to_file writes valid YAML to disk", "[scene_saver]") {
    TestEnv env;
    auto entity = env.world.add_entity();
    entity.set_name("disk_entity");

    SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
    YAML::Node mem = saver.save_to_yaml();

    // Save to temp file
    std::string tmp_path = std::filesystem::temp_directory_path() / "buddd_test_save.yaml";
    auto result = saver.save_to_file(tmp_path);
    REQUIRE(result.has_value());

    // Read back and verify
    YAML::Node disk = YAML::LoadFile(tmp_path);
    CHECK(disk["type"].as<std::string>() == mem["type"].as<std::string>());
    CHECK(disk["version"].as<int>() == mem["version"].as<int>());
    CHECK(disk["entities"].IsSequence());
    CHECK(disk["entities"].size() == mem["entities"].size());

    // Clean up
    std::filesystem::remove(tmp_path);
}

// ===========================================================================
// Test 12: Default transform fields omitted from output (AC-020)
// ===========================================================================
TEST_CASE("Default transform fields omitted from output", "[scene_saver]") {
    TestEnv env;

    // Add entity with all-default transform
    auto entity = env.world.add_entity();
    entity.set_name("default_entity");
    // Add a component so we can find the entity
    entity.add_component<CameraComponent>();

    SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
    YAML::Node saved = saver.save_to_yaml();
    REQUIRE(saved["entities"].IsSequence());
    REQUIRE(saved["entities"].size() == 1);

    auto ent = saved["entities"][0];
    // Transform should be entirely omitted when all fields are default
    CHECK_FALSE(ent["transform"].IsDefined());
}

// ===========================================================================
// Test 13: Default component properties omitted from output (AC-021)
// ===========================================================================
TEST_CASE("Default component properties omitted from output", "[scene_saver]") {
    TestEnv env;
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

    YAML::Node node;
    node["type"] = "Scene";
    node["version"] = 1;
    node["entities"][0]["name"] = "cam";
    // Only set fov_y to a non-default value (default is ~1.047)
    node["entities"][0]["components"][0]["type"] = "camera";
    node["entities"][0]["components"][0]["properties"]["fov_y"] = 0.8;

    auto load_result = loader.load_from_yaml(node);
    REQUIRE(load_result.has_value());

    SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
    YAML::Node saved = saver.save_to_yaml();

    REQUIRE(saved["entities"].IsSequence());
    REQUIRE(saved["entities"].size() == 1);

    auto comps = saved["entities"][0]["components"];
    REQUIRE(comps.IsSequence());
    REQUIRE(comps.size() == 1);
    REQUIRE(comps[0]["type"].as<std::string>() == "camera");

    // fov_y (non-default) should be present
    CHECK(comps[0]["properties"]["fov_y"].as<double>() == Approx(0.8));
    // aspect should be absent (at default 1.333...)
    CHECK_FALSE(comps[0]["properties"]["aspect"].IsDefined());
}

// ===========================================================================
// Test 14: All-default component has no properties key (AC-022)
// ===========================================================================
TEST_CASE("All-default component has no properties key", "[scene_saver]") {
    TestEnv env;
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

    YAML::Node node;
    node["type"] = "Scene";
    node["version"] = 1;
    node["entities"][0]["name"] = "default_cam";
    // Camera component with default properties (no explicit properties block)
    node["entities"][0]["components"][0]["type"] = "camera";

    auto load_result = loader.load_from_yaml(node);
    REQUIRE(load_result.has_value());

    SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
    YAML::Node saved = saver.save_to_yaml();

    REQUIRE(saved["entities"].IsSequence());
    REQUIRE(saved["entities"].size() == 1);

    auto comps = saved["entities"][0]["components"];
    REQUIRE(comps.IsSequence());
    REQUIRE(comps.size() == 1);
    REQUIRE(comps[0]["type"].as<std::string>() == "camera");

    // properties key should be ABSENT when all values are at defaults
    CHECK_FALSE(comps[0]["properties"].IsDefined());
}

// ===========================================================================
// Test 15: Vec3 default property omitted from output (AC-021)
// DirectionalLightComponent has `color` (Vec3, default [1,1,1])
// ===========================================================================
TEST_CASE("Default Vec3 property omitted from output", "[scene_saver]") {
    TestEnv env;
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

    YAML::Node node;
    node["type"] = "Scene";
    node["version"] = 1;
    node["entities"][0]["name"] = "light";
    // Directional light with default color [1,1,1] and non-default intensity
    node["entities"][0]["components"][0]["type"] = "directional_light";
    node["entities"][0]["components"][0]["properties"]["intensity"] = 2.0;

    auto load_result = loader.load_from_yaml(node);
    REQUIRE(load_result.has_value());

    SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
    YAML::Node saved = saver.save_to_yaml();

    REQUIRE(saved["entities"].IsSequence());
    REQUIRE(saved["entities"].size() == 1);

    auto comps = saved["entities"][0]["components"];
    REQUIRE(comps.IsSequence());
    REQUIRE(comps.size() == 1);
    REQUIRE(comps[0]["type"].as<std::string>() == "directional_light");

    // color (Vec3 default [1,1,1]) should be absent
    CHECK_FALSE(comps[0]["properties"]["color"].IsDefined());
    // intensity (non-default 2.0) should be present
    CHECK(comps[0]["properties"]["intensity"].as<double>() == Approx(2.0));
}

// ===========================================================================
// Test 16: Bool default property omitted from output (AC-021)
// FreeCameraMovement has `invert_yaw` (bool, default false)
// ===========================================================================
TEST_CASE("Default bool property omitted from output", "[scene_saver]") {
    TestEnv env;
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

    YAML::Node node;
    node["type"] = "Scene";
    node["version"] = 1;
    node["entities"][0]["name"] = "cam_movement";
    // FreeCameraMovement with default invert_yaw and non-default move_speed
    node["entities"][0]["components"][0]["type"] = "free_camera_movement";
    node["entities"][0]["components"][0]["properties"]["move_speed"] = 10.0;

    auto load_result = loader.load_from_yaml(node);
    REQUIRE(load_result.has_value());

    SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
    YAML::Node saved = saver.save_to_yaml();

    REQUIRE(saved["entities"].IsSequence());
    REQUIRE(saved["entities"].size() == 1);

    auto comps = saved["entities"][0]["components"];
    REQUIRE(comps.IsSequence());
    REQUIRE(comps.size() == 1);
    REQUIRE(comps[0]["type"].as<std::string>() == "free_camera_movement");

    // invert_yaw (bool default false) should be absent
    CHECK_FALSE(comps[0]["properties"]["invert_yaw"].IsDefined());
    // move_speed (non-default 10.0) should be present
    CHECK(comps[0]["properties"]["move_speed"].as<double>() == Approx(10.0));
}

// ===========================================================================
// Test 17: shared_ptr<Model> default (null) property omitted (AC-021)
// MeshRenderer has `model` (shared_ptr<Model>, default null)
// ===========================================================================
TEST_CASE("Default shared_ptr<Model> property omitted from output", "[scene_saver]") {
    TestEnv env;
    SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

    YAML::Node node;
    node["type"] = "Scene";
    node["version"] = 1;
    node["entities"][0]["name"] = "mesh_entity";
    // MeshRenderer with default (null/empty) model — no properties block
    node["entities"][0]["components"][0]["type"] = "mesh_renderer";

    auto load_result = loader.load_from_yaml(node);
    REQUIRE(load_result.has_value());

    SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
    YAML::Node saved = saver.save_to_yaml();

    REQUIRE(saved["entities"].IsSequence());
    REQUIRE(saved["entities"].size() == 1);

    auto comps = saved["entities"][0]["components"];
    REQUIRE(comps.IsSequence());
    REQUIRE(comps.size() == 1);
    REQUIRE(comps[0]["type"].as<std::string>() == "mesh_renderer");

    // When the model is null (default), the properties key should be absent
    // since the only property (model) is at its default
    CHECK_FALSE(comps[0]["properties"].IsDefined());
}
