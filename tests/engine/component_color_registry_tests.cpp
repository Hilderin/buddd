#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/type_registry.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/component_registry/register_all_components.h"
#include "scene/component_registry/property.h"
#include "scene/point_light_component.h"
#include "scene/directional_light_component.h"
#include "scene/spot_light_component.h"
#include "engine_service.h"
#include "platform/platform.h"
#include "window/window.h"
#include "asset/asset_manager.h"

// YAML::convert specializations
#include "math/vec3_yaml.h"
#include "math/vec4_yaml.h"
#include "math/quat_yaml.h"
#include "math/color_yaml.h"

#include <yaml-cpp/yaml.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <memory>
#include <string>
#include <typeindex>

using namespace buddd::engine;
using Catch::Approx;

namespace {
    constexpr float TOL = 1e-5f;

    struct TestEngine {
        std::unique_ptr<EngineService> engine;

        TestEngine()
            : engine(EngineService::create(
                  Backend::Headless,
                  WindowConfig{.title = "Test", .width = 800, .height = 600}).value())
        {}
    };
}

// ===========================================================================
// Color type is registered as 9th built-in type in TypeRegistry
// ===========================================================================
TEST_CASE("Color type is registered in TypeRegistry", "[registry][color]") {
    register_builtin_types();
    REQUIRE(TypeRegistry::is_registered<math::Color>());

    TestEngine test_engine;
    SerializationContext ctx{test_engine.engine->assets()};

    // Encode a color to YAML via TypeRegistry
    math::Color c{0.3f, 0.6f, 0.9f, 1.0f};
    auto encoded = TypeRegistry::yaml_encode<math::Color>(c, ctx);
    REQUIRE(encoded.has_value());
    REQUIRE((*encoded).IsSequence());
    REQUIRE((*encoded).size() == 4);
    REQUIRE((*encoded)[0].as<float>() == Approx(0.3f).margin(TOL));
    REQUIRE((*encoded)[1].as<float>() == Approx(0.6f).margin(TOL));
    REQUIRE((*encoded)[2].as<float>() == Approx(0.9f).margin(TOL));
    REQUIRE((*encoded)[3].as<float>() == Approx(1.0f).margin(TOL));

    // Decode back
    auto decoded = TypeRegistry::yaml_decode<math::Color>(*encoded, ctx);
    REQUIRE(decoded.has_value());
    REQUIRE(decoded->r == Approx(0.3f).margin(TOL));
    REQUIRE(decoded->g == Approx(0.6f).margin(TOL));
    REQUIRE(decoded->b == Approx(0.9f).margin(TOL));
    REQUIRE(decoded->a == Approx(1.0f).margin(TOL));
}

// ===========================================================================
// Color to_string / from_string via TypeRegistry
// ===========================================================================
TEST_CASE("Color to_string and from_string via TypeRegistry", "[registry][color]") {
    register_builtin_types();

    TestEngine test_engine;
    SerializationContext ctx{test_engine.engine->assets()};

    math::Color c{0.1f, 0.2f, 0.3f, 0.4f};
    auto str = TypeRegistry::to_string<math::Color>(c, ctx);
    REQUIRE(str.has_value());
    REQUIRE(str->find("0.1") != std::string::npos);
    REQUIRE(str->find("0.2") != std::string::npos);
    REQUIRE(str->find("0.3") != std::string::npos);
    REQUIRE(str->find("0.4") != std::string::npos);

    auto parsed = TypeRegistry::from_string<math::Color>(*str, ctx);
    REQUIRE(parsed.has_value());
    REQUIRE(parsed->r == Approx(c.r).margin(TOL));
    REQUIRE(parsed->g == Approx(c.g).margin(TOL));
    REQUIRE(parsed->b == Approx(c.b).margin(TOL));
    REQUIRE(parsed->a == Approx(c.a).margin(TOL));
}

// ===========================================================================
// Light component property types are registered as Color
// ===========================================================================
TEST_CASE("Point light color property type is Color", "[registry][color]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("point_light");
    REQUIRE(info != nullptr);
    REQUIRE(info->property_type_index(0) == std::type_index(typeid(math::Color)));
}

TEST_CASE("Directional light color property type is Color", "[registry][color]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("directional_light");
    REQUIRE(info != nullptr);
    REQUIRE(info->property_type_index(0) == std::type_index(typeid(math::Color)));
}

TEST_CASE("Spot light color property type is Color", "[registry][color]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("spot_light");
    REQUIRE(info != nullptr);
    REQUIRE(info->property_type_index(0) == std::type_index(typeid(math::Color)));
}
