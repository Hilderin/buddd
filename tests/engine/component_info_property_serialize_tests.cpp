#include "scene/component_registry/type_registry.h"
#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/register_all_components.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/camera_component.h"
#include "engine_service.h"
#include "platform/platform.h"
#include "window/window.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <memory>

namespace be = buddd::engine;
using Catch::Approx;

// ═════════════════════════════════════════════════════════════════════
// ComponentInfoBase: property_serialize round-trip tests
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("ComponentInfoBase: property_serialize round-trip",
          "[engine][component-registry]")
{
    // Register built-in types and camera component
    be::register_builtin_types();
    be::ComponentRegistry registry;
    be::register_all_components(registry);

    auto* info = registry.describe("camera");
    REQUIRE(info != nullptr);
    REQUIRE(info->property_count() >= 1);

    // Create an EngineService to get a valid AssetManager for SerializationContext
    auto eng = be::EngineService::create(
        be::Backend::Headless,
        be::WindowConfig{.title = "property_serialize test", .width = 128, .height = 128});
    REQUIRE(eng.has_value());

    // Create a CameraComponent with known fov_y
    auto camera = std::make_unique<be::CameraComponent>(1.05f, 16.0f / 9.0f, 0.1f, 100.0f);

    be::SerializationContext ctx{(*eng)->assets()};

    // Serialize property 0 (fov_y)
    auto yaml_node = info->property_serialize(*camera, 0, ctx);

    // Verify it's a float scalar ≈ 1.05
    REQUIRE_FALSE(yaml_node.IsNull());
    REQUIRE(yaml_node.IsScalar());
    REQUIRE(yaml_node.as<float>() == Approx(1.05f).margin(1e-5f));
}

TEST_CASE("ComponentInfoBase: property_deserialize modifies component",
          "[engine][component-registry]")
{
    be::register_builtin_types();
    be::ComponentRegistry registry;
    be::register_all_components(registry);

    auto* info = registry.describe("camera");
    REQUIRE(info != nullptr);

    auto eng = be::EngineService::create(
        be::Backend::Headless,
        be::WindowConfig{.title = "property_deserialize test", .width = 128, .height = 128});
    REQUIRE(eng.has_value());

    // Create a CameraComponent with default fov_y
    auto camera = std::make_unique<be::CameraComponent>();

    be::SerializationContext ctx{(*eng)->assets()};

    // Deserialize property 0 (fov_y) with new value 2.0
    auto result = info->property_deserialize(*camera, 0, YAML::Node(2.0), ctx);
    REQUIRE(result.has_value());

    // Verify component's fov_y is now ≈ 2.0
    REQUIRE(camera->fov_y() == Approx(2.0f).margin(1e-5f));
}

TEST_CASE("ComponentInfoBase: property_serialize with out-of-bounds index",
          "[engine][component-registry]")
{
    be::register_builtin_types();
    be::ComponentRegistry registry;
    be::register_all_components(registry);

    auto* info = registry.describe("camera");
    REQUIRE(info != nullptr);

    auto eng = be::EngineService::create(
        be::Backend::Headless,
        be::WindowConfig{.title = "property_serialize OOB test", .width = 128, .height = 128});
    REQUIRE(eng.has_value());

    auto camera = std::make_unique<be::CameraComponent>();

    be::SerializationContext ctx{(*eng)->assets()};

    // Serialize with out-of-bounds index
    auto yaml_node = info->property_serialize(*camera, 99, ctx);
    REQUIRE(yaml_node.IsNull());
}

TEST_CASE("ComponentInfoBase: property_deserialize with out-of-bounds index returns error",
          "[engine][component-registry]")
{
    be::register_builtin_types();
    be::ComponentRegistry registry;
    be::register_all_components(registry);

    auto* info = registry.describe("camera");
    REQUIRE(info != nullptr);

    auto eng = be::EngineService::create(
        be::Backend::Headless,
        be::WindowConfig{.title = "property_deserialize OOB test", .width = 128, .height = 128});
    REQUIRE(eng.has_value());

    auto camera = std::make_unique<be::CameraComponent>();

    be::SerializationContext ctx{(*eng)->assets()};

    // Deserialize with out-of-bounds index
    auto result = info->property_deserialize(*camera, 99, YAML::Node(2.0), ctx);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == be::Error::Category::InvalidArgument);
}
