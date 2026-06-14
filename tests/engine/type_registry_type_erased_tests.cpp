#include "scene/component_registry/type_registry.h"
#include "scene/component_registry/register_all_components.h"
#include "scene/component_registry/serialization_context.h"
#include "engine_service.h"
#include "platform/platform.h"
#include "window/window.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <any>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>

namespace be = buddd::engine;
using Catch::Approx;

// ═════════════════════════════════════════════════════════════════════
// TypeRegistry: type-erased yaml_encode / yaml_decode tests
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("TypeRegistry: yaml_encode(type_index, any) for registered type",
          "[engine][component-registry]")
{
    be::register_builtin_types();

    auto eng = be::EngineService::create(
        be::Backend::Headless,
        be::WindowConfig{.title = "yaml_encode test", .width = 128, .height = 128});
    REQUIRE(eng.has_value());

    be::SerializationContext ctx{(*eng)->assets()};

    // Encode a float value
    auto result = be::TypeRegistry::yaml_encode(
        std::type_index(typeid(float)), std::any(3.14f), ctx);

    REQUIRE(result.has_value());
    REQUIRE(result->IsScalar());
    REQUIRE(result->as<float>() == Approx(3.14f).margin(1e-5f));
}

TEST_CASE("TypeRegistry: yaml_encode returns error for unregistered type",
          "[engine][component-registry]")
{
    be::register_builtin_types();

    auto eng = be::EngineService::create(
        be::Backend::Headless,
        be::WindowConfig{.title = "yaml_encode unreg test", .width = 128, .height = 128});
    REQUIRE(eng.has_value());

    be::SerializationContext ctx{(*eng)->assets()};

    // Encode with unregistered type (double is not registered)
    auto result = be::TypeRegistry::yaml_encode(
        std::type_index(typeid(double)), std::any(1.0), ctx);

    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("TypeRegistry: yaml_encode returns error on type mismatch",
          "[engine][component-registry]")
{
    be::register_builtin_types();

    auto eng = be::EngineService::create(
        be::Backend::Headless,
        be::WindowConfig{.title = "yaml_encode mismatch test", .width = 128, .height = 128});
    REQUIRE(eng.has_value());

    be::SerializationContext ctx{(*eng)->assets()};

    // Type mismatch: type_index(float) but std::any holds a string
    auto result = be::TypeRegistry::yaml_encode(
        std::type_index(typeid(float)), std::any(std::string("hello")), ctx);

    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("TypeRegistry: yaml_decode(type_index, node) for registered type",
          "[engine][component-registry]")
{
    be::register_builtin_types();

    auto eng = be::EngineService::create(
        be::Backend::Headless,
        be::WindowConfig{.title = "yaml_decode test", .width = 128, .height = 128});
    REQUIRE(eng.has_value());

    be::SerializationContext ctx{(*eng)->assets()};

    // Decode a float YAML node
    auto result = be::TypeRegistry::yaml_decode(
        std::type_index(typeid(float)), YAML::Node(2.71f), ctx);

    REQUIRE(result.has_value());
    REQUIRE(std::any_cast<float>(*result) == Approx(2.71f).margin(1e-5f));
}

TEST_CASE("TypeRegistry: yaml_decode returns error for unregistered type",
          "[engine][component-registry]")
{
    be::register_builtin_types();

    auto eng = be::EngineService::create(
        be::Backend::Headless,
        be::WindowConfig{.title = "yaml_decode unreg test", .width = 128, .height = 128});
    REQUIRE(eng.has_value());

    be::SerializationContext ctx{(*eng)->assets()};

    // Decode with unregistered type (double is not registered)
    auto result = be::TypeRegistry::yaml_decode(
        std::type_index(typeid(double)), YAML::Node(1.0), ctx);

    REQUIRE_FALSE(result.has_value());
}
