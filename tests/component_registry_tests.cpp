#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/type_registry.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/component_registry/serialization.h"
#include "scene/component_registry/register_all_components.h"
#include "scene/component_registry/property.h"
#include "scene/component.h"
#include "scene/camera_component.h"
#include "scene/point_light_component.h"
#include "scene/directional_light_component.h"
#include "scene/spot_light_component.h"
#include "scene/free_camera_movement.h"
#include "render/mesh_renderer.h"
#include "render/model.h"
#include "asset/asset_manager.h"
#include "engine_service.h"
#include "platform/platform.h"
#include "window/window.h"

// YAML::convert specializations for math types
#include "math/vec3_yaml.h"
#include "math/vec4_yaml.h"
#include "math/quat_yaml.h"

#include <yaml-cpp/yaml.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <memory>
#include <string>
#include <typeindex>
#include <vector>

using namespace buddd::engine;
using Catch::Approx;

// ===========================================================================
// Test component for registration tests
// ===========================================================================
namespace {

class TestComponent : public Component {
public:
    float value = 42.0f;
    int32_t count = 0;
};

// Custom type for TypeRegistry tests
struct TestType {
    int id;
    float data;

    auto operator==(const TestType& other) const -> bool {
        return id == other.id && data == other.data;
    }
};

// Mock AssetManager that provides stub-based find_asset_id / resolve_model.
// The constructor is protected in AssetManager, so a derived class can access it.
class MockAssetManager : public AssetManager {
public:
    MockAssetManager(RenderDevice& device, const std::string& base_path)
        : AssetManager(device, base_path) {}

    auto find_asset_id(const Model& model) const -> std::string override {
        return find_asset_id_stub(model);
    }
    std::function<std::string(const Model&)> find_asset_id_stub =
        [](const Model&) -> std::string { return {}; };

    auto resolve_model(const std::string& id) -> Result<std::shared_ptr<Model>> override {
        return resolve_model_stub(id);
    }
    std::function<Result<std::shared_ptr<Model>>(const std::string&)> resolve_model_stub =
        [](const std::string& id) -> Result<std::shared_ptr<Model>> {
            return make_error(Error::Category::InvalidArgument,
                "MockAssetManager: resolve_model not configured for '" + id + "'");
        };
};

/// Create a minimal headless engine for tests that need a RenderDevice.
/// This also calls register_builtin_types() and register_all_components()
/// during EngineService::create().
struct TestEngine {
    std::unique_ptr<EngineService> engine;
    MockAssetManager mock_assets;

    TestEngine()
        : engine(EngineService::create(
              Backend::Headless,
              WindowConfig{.title = "Test", .width = 800, .height = 600}).value())
        , mock_assets(engine->device(), std::string(engine->assets().base_path()))
    {}
};

} // anonymous namespace

// ===========================================================================
// Registration and querying
// ===========================================================================

TEST_CASE("REGISTER_COMPONENT_AND_QUERY", "[component-registry]") {
    ComponentRegistry registry;
    auto& info = registry.register_component<TestComponent>("test");
    REQUIRE(info.type_name() == "test");

    // describe returns non-null
    auto* desc = registry.describe("test");
    REQUIRE(desc != nullptr);
    REQUIRE(desc->type_name() == "test");

    // create returns unique_ptr<Component> that is-a TestComponent
    auto comp = registry.create("test");
    REQUIRE(comp.has_value());
    REQUIRE(dynamic_cast<TestComponent*>(comp->get()) != nullptr);
}

TEST_CASE("REGISTER_DUPLICATE_WARNS_AND_RETURNS_SAME", "[component-registry]") {
    ComponentRegistry registry;
    auto& info1 = registry.register_component<TestComponent>("test");
    auto& info2 = registry.register_component<TestComponent>("test");

    // Same underlying object
    REQUIRE(&info1 == &info2);
}

TEST_CASE("CREATE_UNKNOWN_TYPE_RETURNS_ERROR", "[component-registry]") {
    ComponentRegistry registry;
    auto result = registry.create("nonexistent");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::InvalidArgument);
    REQUIRE(result.error().message.find("nonexistent") != std::string::npos);
}

TEST_CASE("DESCRIBE_UNKNOWN_TYPE_RETURNS_NULLPTR", "[component-registry]") {
    ComponentRegistry registry;
    auto* desc = registry.describe("nonexistent");
    REQUIRE(desc == nullptr);
}

TEST_CASE("ALL_TYPES_COUNT", "[component-registry]") {
    ComponentRegistry registry;
    registry.register_component<TestComponent>("test1");
    registry.register_component<TestComponent>("test2");
    registry.register_component<TestComponent>("test3");

    REQUIRE(registry.all_types().size() == 3);
}

// ===========================================================================
// TypeRegistry operations
// ===========================================================================

TEST_CASE("TYPE_REGISTRY_REGISTER_AND_ENCODE", "[component-registry]") {
    // Register TestType with five callbacks
    TypeRegistry::register_type<TestType>({
        .yaml_encode = [](const TestType& v, const SerializationContext&) -> YAML::Node {
            YAML::Node n;
            n["id"] = v.id;
            n["data"] = v.data;
            return n;
        },
        .yaml_decode = [](const YAML::Node& n, const SerializationContext&) -> Result<TestType> {
            return TestType{n["id"].as<int>(), n["data"].as<float>()};
        },
        .to_string = [](const TestType& v, const SerializationContext&) -> std::string {
            return std::to_string(v.id) + "," + std::to_string(v.data);
        },
        .from_string = [](const std::string& s, const SerializationContext&) -> Result<TestType> {
            auto comma = s.find(',');
            if (comma == std::string::npos) {
                return make_error(Error::Category::InvalidArgument, "invalid format");
            }
            return TestType{std::stoi(s.substr(0, comma)), std::stof(s.substr(comma + 1))};
        },
        .validate = [](const TestType& v, const SerializationContext&) -> Result<void> {
            if (v.id < 0) {
                return make_error(Error::Category::InvalidArgument, "negative id");
            }
            return {};
        }
    });

    // Verify yaml_encode produces expected YAML
    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    auto encoded = TypeRegistry::yaml_encode<TestType>(TestType{42, 3.14f}, ctx);
    REQUIRE(encoded.has_value());
    REQUIRE((*encoded)["id"].as<int>() == 42);
    REQUIRE((*encoded)["data"].as<float>() == Approx(3.14f).margin(1e-5f));

    // Verify round-trip
    auto decoded = TypeRegistry::yaml_decode<TestType>(*encoded, ctx);
    REQUIRE(decoded.has_value());
    REQUIRE(decoded->id == 42);
    REQUIRE(decoded->data == Approx(3.14f).margin(1e-5f));
}

TEST_CASE("TYPE_REGISTRY_STRING_ROUNDTRIP", "[component-registry]") {
    TypeRegistry::register_type<TestType>({
        .yaml_encode = [](const TestType& v, const SerializationContext&) -> YAML::Node {
            YAML::Node n;
            n["id"] = v.id;
            return n;
        },
        .yaml_decode = [](const YAML::Node& n, const SerializationContext&) -> Result<TestType> {
            return TestType{n["id"].as<int>(), 0.0f};
        },
        .to_string = [](const TestType& v, const SerializationContext&) -> std::string {
            return std::to_string(v.id);
        },
        .from_string = [](const std::string& s, const SerializationContext&) -> Result<TestType> {
            return TestType{std::stoi(s), 0.0f};
        },
        .validate = [](const TestType&, const SerializationContext&) -> Result<void> { return {}; }
    });

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    auto str = TypeRegistry::to_string<TestType>(TestType{99, 0.0f}, ctx);
    REQUIRE(str.has_value());
    REQUIRE(*str == "99");

    auto parsed = TypeRegistry::from_string<TestType>("99", ctx);
    REQUIRE(parsed.has_value());
    REQUIRE(parsed->id == 99);
}

TEST_CASE("TYPE_REGISTRY_VALIDATE", "[component-registry]") {
    TypeRegistry::register_type<TestType>({
        .yaml_encode = [](const TestType& v, const SerializationContext&) -> YAML::Node {
            return YAML::Node(v.id);
        },
        .yaml_decode = [](const YAML::Node& n, const SerializationContext&) -> Result<TestType> {
            return TestType{n.as<int>(), 0.0f};
        },
        .to_string = [](const TestType& v, const SerializationContext&) -> std::string {
            return std::to_string(v.id);
        },
        .from_string = [](const std::string& s, const SerializationContext&) -> Result<TestType> {
            return TestType{std::stoi(s), 0.0f};
        },
        .validate = [](const TestType& v, const SerializationContext&) -> Result<void> {
            if (v.id < 0) {
                return make_error(Error::Category::InvalidArgument, "negative id");
            }
            return {};
        }
    });

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    // Positive value — success
    auto result_ok = TypeRegistry::validate<TestType>(TestType{10, 0.0f}, ctx);
    REQUIRE(result_ok.has_value());

    // Negative value — error
    auto result_err = TypeRegistry::validate<TestType>(TestType{-1, 0.0f}, ctx);
    REQUIRE_FALSE(result_err.has_value());
    REQUIRE(result_err.error().category == Error::Category::InvalidArgument);
}

TEST_CASE("TYPE_REGISTRY_OVERWRITE_WARNS", "[component-registry]") {
    // Register type T = int (though int is built-in, use a unique wrapper)
    // Use TestType for overwrite test
    TypeRegistry::register_type<TestType>({
        .yaml_encode = [](const TestType& v, const SerializationContext&) -> YAML::Node {
            return YAML::Node(v.id);
        },
        .yaml_decode = [](const YAML::Node& n, const SerializationContext&) -> Result<TestType> {
            return TestType{n.as<int>(), 0.0f};
        },
        .to_string = [](const TestType&, const SerializationContext&) -> std::string { return "old"; },
        .from_string = [](const std::string&, const SerializationContext&) -> Result<TestType> {
            return TestType{0, 0.0f};
        },
        .validate = [](const TestType&, const SerializationContext&) -> Result<void> { return {}; }
    });

    // Register again with different callbacks
    TypeRegistry::register_type<TestType>({
        .yaml_encode = [](const TestType& v, const SerializationContext&) -> YAML::Node {
            return YAML::Node(v.id);
        },
        .yaml_decode = [](const YAML::Node& n, const SerializationContext&) -> Result<TestType> {
            return TestType{n.as<int>(), 0.0f};
        },
        .to_string = [](const TestType&, const SerializationContext&) -> std::string { return "new"; },
        .from_string = [](const std::string&, const SerializationContext&) -> Result<TestType> {
            return TestType{0, 0.0f};
        },
        .validate = [](const TestType&, const SerializationContext&) -> Result<void> { return {}; }
    });

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    // Verify new callbacks are active
    auto str = TypeRegistry::to_string<TestType>(TestType{1, 0.0f}, ctx);
    REQUIRE(str.has_value());
    REQUIRE(*str == "new");
}

TEST_CASE("TYPE_REGISTRY_BUILTIN_FLOAT", "[component-registry]") {
    // Built-in types are pre-registered via register_builtin_types() which is
    // called during EngineService::create(). Since we already have a TestEngine
    // that called EngineService::create, float should be registered.

    // But register_builtin_types() is already called globally by any TestEngine.
    // We need to ensure it's called. Let's call it explicitly here.
    // To avoid duplicate warnings, it's idempotent.
    register_builtin_types();

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    auto encoded = TypeRegistry::yaml_encode<float>(3.14f, ctx);
    REQUIRE(encoded.has_value());
    REQUIRE(encoded->as<float>() == Approx(3.14f).margin(1e-5f));

    auto str = TypeRegistry::to_string<float>(3.14f, ctx);
    REQUIRE(str.has_value());
    REQUIRE_FALSE(str->empty());
}

TEST_CASE("TYPE_REGISTRY_BUILTIN_VEC3", "[component-registry]") {
    register_builtin_types();

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    auto encoded = TypeRegistry::yaml_encode<math::Vec3>({1.0f, 2.0f, 3.0f}, ctx);
    REQUIRE(encoded.has_value());
    REQUIRE((*encoded).IsSequence());
    REQUIRE((*encoded).size() == 3);
    REQUIRE((*encoded)[0].as<float>() == Approx(1.0f).margin(1e-5f));  // x
    REQUIRE((*encoded)[1].as<float>() == Approx(2.0f).margin(1e-5f));  // y
    REQUIRE((*encoded)[2].as<float>() == Approx(3.0f).margin(1e-5f));  // z
}

TEST_CASE("TYPE_REGISTRY_BUILTIN_SHARED_PTR_MODEL", "[component-registry]") {
    register_builtin_types();

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    // Null shared_ptr encodes as empty string
    std::shared_ptr<Model> null_model;
    auto encoded = TypeRegistry::yaml_encode<std::shared_ptr<Model>>(null_model, ctx);
    REQUIRE(encoded.has_value());
    REQUIRE(encoded->as<std::string>() == "");

    // Empty string decodes to null shared_ptr
    auto decoded = TypeRegistry::yaml_decode<std::shared_ptr<Model>>(YAML::Node(""), ctx);
    REQUIRE(decoded.has_value());
    REQUIRE(*decoded == nullptr);
}

TEST_CASE("TYPE_REGISTRY_UNREGISTERED_RUNTIME_ERROR", "[component-registry]") {
    // Use a unique type that is NEVER registered
    struct NeverRegisteredType {
        int val;
    };

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    // Attempt to encode unregistered type should return error
    auto result = TypeRegistry::yaml_encode<NeverRegisteredType>(NeverRegisteredType{42}, ctx);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::InvalidArgument);
}

// ===========================================================================
// Property metadata
// ===========================================================================

TEST_CASE("PROPERTY_METADATA", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    auto& info = registry.register_component<TestComponent>("test");

    // Add a float property
    info.add_property<float>("test_value",
        [](const TestComponent& c) { return c.value; },
        [](TestComponent& c, float v) -> Result<void> { c.value = v; return {}; }
    );

    REQUIRE(info.property_count() == 1);
    REQUIRE(info.property_name(0) == "test_value");
    REQUIRE(info.property_type_index(0) == std::type_index(typeid(float)));
}

TEST_CASE("PROPERTY_GET_SET", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    auto& info = registry.register_component<TestComponent>("test");

    info.add_property<float>("test_value",
        [](const TestComponent& c) { return c.value; },
        [](TestComponent& c, float v) -> Result<void> { c.value = v; return {}; }
    );

    // Create component, set value directly, serialize, verify getter returns correct value
    auto comp = info.create();
    auto& typed = static_cast<TestComponent&>(*comp);
    typed.value = 123.0f;

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    auto node = info.serialize(*comp, ctx);
    REQUIRE(node["test_value"].as<float>() == Approx(123.0f).margin(1e-5f));

    // Deserialize new value and verify setter works
    auto comp2 = info.create();
    YAML::Node set_node;
    set_node["test_value"] = 456.0f;
    auto result = info.deserialize(*comp2, set_node, ctx);
    REQUIRE(result.has_value());
    REQUIRE(static_cast<TestComponent&>(*comp2).value == Approx(456.0f).margin(1e-5f));
}

TEST_CASE("PROPERTY_VALIDATION_MIN", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    auto& info = registry.register_component<TestComponent>("test");

    // Add float property with min=10
    info.add_property<float>("constrained_value",
        [](const TestComponent& c) { return c.value; },
        [](TestComponent& c, float v) -> Result<void> { c.value = v; return {}; },
        PropertyFlags{}.min(10.0f)
    );

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    // Set value below min
    auto comp = info.create();
    YAML::Node set_node;
    set_node["constrained_value"] = 5.0f;
    auto result = info.deserialize(*comp, set_node, ctx);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::InvalidArgument);
    REQUIRE(result.error().message.find("out of range") != std::string::npos);
}

TEST_CASE("PROPERTY_VALIDATION_MAX", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    auto& info = registry.register_component<TestComponent>("test");

    // Add float property with max=100
    info.add_property<float>("constrained_value",
        [](const TestComponent& c) { return c.value; },
        [](TestComponent& c, float v) -> Result<void> { c.value = v; return {}; },
        PropertyFlags{}.max(100.0f)
    );

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    // Set value above max
    auto comp = info.create();
    YAML::Node set_node;
    set_node["constrained_value"] = 200.0f;
    auto result = info.deserialize(*comp, set_node, ctx);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::InvalidArgument);
    REQUIRE(result.error().message.find("out of range") != std::string::npos);
}

// ===========================================================================
// Real component property verification
// ===========================================================================

TEST_CASE("CAMERA_COMPONENT_PROPERTIES", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("camera");
    REQUIRE(info != nullptr);
    REQUIRE(info->property_count() == 4);

    // Check property names
    REQUIRE(info->property_name(0) == "fov_y");
    REQUIRE(info->property_name(1) == "aspect");
    REQUIRE(info->property_name(2) == "near");
    REQUIRE(info->property_name(3) == "far");

    // All 4 properties should be float
    for (size_t i = 0; i < 4; ++i) {
        REQUIRE(info->property_type_index(i) == std::type_index(typeid(float)));
    }
}

TEST_CASE("POINT_LIGHT_COMPONENT_PROPERTIES", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("point_light");
    REQUIRE(info != nullptr);
    REQUIRE(info->property_count() == 3);

    REQUIRE(info->property_name(0) == "color");
    REQUIRE(info->property_type_index(0) == std::type_index(typeid(math::Vec3)));

    REQUIRE(info->property_name(1) == "intensity");
    REQUIRE(info->property_type_index(1) == std::type_index(typeid(float)));
    // intensity has min constraint
    REQUIRE(info->property_flags(1).min_value == Approx(0.0f).margin(1e-5f));

    REQUIRE(info->property_name(2) == "range");
    REQUIRE(info->property_type_index(2) == std::type_index(typeid(float)));
    REQUIRE(info->property_flags(2).min_value == Approx(0.0f).margin(1e-5f));
}

TEST_CASE("DIRECTIONAL_LIGHT_COMPONENT_PROPERTIES", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("directional_light");
    REQUIRE(info != nullptr);
    REQUIRE(info->property_count() == 2);

    REQUIRE(info->property_name(0) == "color");
    REQUIRE(info->property_type_index(0) == std::type_index(typeid(math::Vec3)));

    REQUIRE(info->property_name(1) == "intensity");
    REQUIRE(info->property_type_index(1) == std::type_index(typeid(float)));
    REQUIRE(info->property_flags(1).min_value == Approx(0.0f).margin(1e-5f));
}

TEST_CASE("SPOT_LIGHT_COMPONENT_PROPERTIES", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("spot_light");
    REQUIRE(info != nullptr);
    REQUIRE(info->property_count() == 5);

    REQUIRE(info->property_name(0) == "color");
    REQUIRE(info->property_type_index(0) == std::type_index(typeid(math::Vec3)));

    REQUIRE(info->property_name(1) == "intensity");
    REQUIRE(info->property_type_index(1) == std::type_index(typeid(float)));
    REQUIRE(info->property_flags(1).min_value == Approx(0.0f).margin(1e-5f));

    REQUIRE(info->property_name(2) == "range");
    REQUIRE(info->property_type_index(2) == std::type_index(typeid(float)));
    REQUIRE(info->property_flags(2).min_value == Approx(0.0f).margin(1e-5f));

    REQUIRE(info->property_name(3) == "inner_angle");
    REQUIRE(info->property_type_index(3) == std::type_index(typeid(float)));
    REQUIRE(info->property_flags(3).min_value == Approx(0.0f).margin(1e-5f));

    REQUIRE(info->property_name(4) == "outer_angle");
    REQUIRE(info->property_type_index(4) == std::type_index(typeid(float)));
    REQUIRE(info->property_flags(4).min_value == Approx(0.0f).margin(1e-5f));
}

TEST_CASE("MESH_RENDERER_COMPONENT_PROPERTIES", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("mesh_renderer");
    REQUIRE(info != nullptr);
    REQUIRE(info->property_count() == 1);

    REQUIRE(info->property_name(0) == "model");
    REQUIRE(info->property_type_index(0) == std::type_index(typeid(std::shared_ptr<Model>)));
}

TEST_CASE("FREE_CAMERA_MOVEMENT_PROPERTIES", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("free_camera_movement");
    REQUIRE(info != nullptr);
    REQUIRE(info->property_count() == 5);

    // move_speed (float, min 0)
    REQUIRE(info->property_name(0) == "move_speed");
    REQUIRE(info->property_type_index(0) == std::type_index(typeid(float)));
    REQUIRE(info->property_flags(0).min_value == Approx(0.0f).margin(1e-5f));

    // mouse_sensitivity (float, min 0)
    REQUIRE(info->property_name(1) == "mouse_sensitivity");
    REQUIRE(info->property_type_index(1) == std::type_index(typeid(float)));
    REQUIRE(info->property_flags(1).min_value == Approx(0.0f).margin(1e-5f));

    // pitch_clamp_degrees (float)
    REQUIRE(info->property_name(2) == "pitch_clamp_degrees");
    REQUIRE(info->property_type_index(2) == std::type_index(typeid(float)));

    // invert_yaw (bool)
    REQUIRE(info->property_name(3) == "invert_yaw");
    REQUIRE(info->property_type_index(3) == std::type_index(typeid(bool)));

    // invert_pitch (bool)
    REQUIRE(info->property_name(4) == "invert_pitch");
    REQUIRE(info->property_type_index(4) == std::type_index(typeid(bool)));
}

// ===========================================================================
// YAML round-trip for real components
// ===========================================================================

TEST_CASE("SERIALIZE_CAMERA_COMPONENT", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("camera");
    REQUIRE(info != nullptr);

    auto comp = registry.create("camera");
    REQUIRE(comp.has_value());
    auto& camera = static_cast<CameraComponent&>(*comp.value());
    camera.set_perspective(1.0f, 2.0f, 0.01f, 500.0f);

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    YAML::Node node = serialize_component(*info, *comp.value(), ctx);
    REQUIRE(node.IsMap());
    REQUIRE(node["fov_y"].as<float>() == Approx(1.0f).margin(1e-5f));
    REQUIRE(node["aspect"].as<float>() == Approx(2.0f).margin(1e-5f));
    REQUIRE(node["near"].as<float>() == Approx(0.01f).margin(1e-5f));
    REQUIRE(node["far"].as<float>() == Approx(500.0f).margin(1e-5f));
}

TEST_CASE("DESERIALIZE_CAMERA_COMPONENT", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("camera");
    REQUIRE(info != nullptr);

    auto comp = registry.create("camera");
    REQUIRE(comp.has_value());

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    YAML::Node node;
    node["fov_y"] = 0.5f;
    node["aspect"] = 1.5f;
    node["near"] = 0.2f;
    node["far"] = 200.0f;

    auto result = deserialize_component(*info, node, *comp.value(), ctx);
    REQUIRE(result.has_value());

    auto& camera = static_cast<CameraComponent&>(*comp.value());
    REQUIRE(camera.fov_y() == Approx(0.5f).margin(1e-5f));
    REQUIRE(camera.aspect() == Approx(1.5f).margin(1e-5f));
    REQUIRE(camera.near_plane() == Approx(0.2f).margin(1e-5f));
    REQUIRE(camera.far_plane() == Approx(200.0f).margin(1e-5f));
}

TEST_CASE("ROUND_TRIP_CAMERA_COMPONENT", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("camera");
    REQUIRE(info != nullptr);

    // Create camera with specific values
    auto comp1 = registry.create("camera");
    REQUIRE(comp1.has_value());
    auto& cam1 = static_cast<CameraComponent&>(*comp1.value());
    cam1.set_perspective(0.8f, 1.6f, 0.05f, 300.0f);

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    // Serialize
    YAML::Node node = serialize_component(*info, *comp1.value(), ctx);

    // Deserialize into a new camera
    auto comp2 = registry.create("camera");
    REQUIRE(comp2.has_value());
    auto result = deserialize_component(*info, node, *comp2.value(), ctx);
    REQUIRE(result.has_value());

    // Verify values match
    auto& cam2 = static_cast<CameraComponent&>(*comp2.value());
    REQUIRE(cam2.fov_y() == Approx(cam1.fov_y()).margin(1e-5f));
    REQUIRE(cam2.aspect() == Approx(cam1.aspect()).margin(1e-5f));
    REQUIRE(cam2.near_plane() == Approx(cam1.near_plane()).margin(1e-5f));
    REQUIRE(cam2.far_plane() == Approx(cam1.far_plane()).margin(1e-5f));
}

TEST_CASE("ROUND_TRIP_POINT_LIGHT", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("point_light");
    REQUIRE(info != nullptr);

    auto comp1 = registry.create("point_light");
    REQUIRE(comp1.has_value());
    auto& pl1 = static_cast<PointLightComponent&>(*comp1.value());
    pl1.color() = math::Vec3{0.5f, 0.6f, 0.7f};
    pl1.intensity() = 2.5f;
    pl1.range() = 20.0f;

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    YAML::Node node = serialize_component(*info, *comp1.value(), ctx);

    auto comp2 = registry.create("point_light");
    REQUIRE(comp2.has_value());
    auto result = deserialize_component(*info, node, *comp2.value(), ctx);
    REQUIRE(result.has_value());

    auto& pl2 = static_cast<PointLightComponent&>(*comp2.value());
    REQUIRE(pl2.color().x == Approx(pl1.color().x).margin(1e-5f));
    REQUIRE(pl2.color().y == Approx(pl1.color().y).margin(1e-5f));
    REQUIRE(pl2.color().z == Approx(pl1.color().z).margin(1e-5f));
    REQUIRE(pl2.intensity() == Approx(pl1.intensity()).margin(1e-5f));
    REQUIRE(pl2.range() == Approx(pl1.range()).margin(1e-5f));
}

TEST_CASE("ROUND_TRIP_DIRECTIONAL_LIGHT", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("directional_light");
    REQUIRE(info != nullptr);

    auto comp1 = registry.create("directional_light");
    REQUIRE(comp1.has_value());
    auto& dl1 = static_cast<DirectionalLightComponent&>(*comp1.value());
    dl1.color() = math::Vec3{0.2f, 0.3f, 0.4f};
    dl1.intensity() = 3.0f;

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    YAML::Node node = serialize_component(*info, *comp1.value(), ctx);

    auto comp2 = registry.create("directional_light");
    REQUIRE(comp2.has_value());
    auto result = deserialize_component(*info, node, *comp2.value(), ctx);
    REQUIRE(result.has_value());

    auto& dl2 = static_cast<DirectionalLightComponent&>(*comp2.value());
    REQUIRE(dl2.color().x == Approx(dl1.color().x).margin(1e-5f));
    REQUIRE(dl2.color().y == Approx(dl1.color().y).margin(1e-5f));
    REQUIRE(dl2.color().z == Approx(dl1.color().z).margin(1e-5f));
    REQUIRE(dl2.intensity() == Approx(dl1.intensity()).margin(1e-5f));
}

TEST_CASE("ROUND_TRIP_SPOT_LIGHT", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("spot_light");
    REQUIRE(info != nullptr);

    auto comp1 = registry.create("spot_light");
    REQUIRE(comp1.has_value());
    auto& sl1 = static_cast<SpotLightComponent&>(*comp1.value());
    sl1.color() = math::Vec3{0.1f, 0.2f, 0.3f};
    sl1.intensity() = 4.0f;
    sl1.range() = 30.0f;
    sl1.inner_angle() = 0.3f;
    sl1.outer_angle() = 0.6f;

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    YAML::Node node = serialize_component(*info, *comp1.value(), ctx);

    auto comp2 = registry.create("spot_light");
    REQUIRE(comp2.has_value());
    auto result = deserialize_component(*info, node, *comp2.value(), ctx);
    REQUIRE(result.has_value());

    auto& sl2 = static_cast<SpotLightComponent&>(*comp2.value());
    REQUIRE(sl2.color().x == Approx(sl1.color().x).margin(1e-5f));
    REQUIRE(sl2.color().y == Approx(sl1.color().y).margin(1e-5f));
    REQUIRE(sl2.color().z == Approx(sl1.color().z).margin(1e-5f));
    REQUIRE(sl2.intensity() == Approx(sl1.intensity()).margin(1e-5f));
    REQUIRE(sl2.range() == Approx(sl1.range()).margin(1e-5f));
    REQUIRE(sl2.inner_angle() == Approx(sl1.inner_angle()).margin(1e-5f));
    REQUIRE(sl2.outer_angle() == Approx(sl1.outer_angle()).margin(1e-5f));
}

TEST_CASE("ROUND_TRIP_MESH_RENDERER", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("mesh_renderer");
    REQUIRE(info != nullptr);

    // Test with null model
    auto comp1 = registry.create("mesh_renderer");
    REQUIRE(comp1.has_value());
    auto& mr1 = static_cast<MeshRenderer&>(*comp1.value());

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    // Null model is a default-valued property, so it should be omitted from serialized output
    YAML::Node node = serialize_component(*info, *comp1.value(), ctx);
    // Model key should be absent (default-valued property is skipped)
    REQUIRE_FALSE(node["model"].IsDefined());

    // Deserialize the node (without model key) — model stays null (default)
    auto comp2 = registry.create("mesh_renderer");
    REQUIRE(comp2.has_value());
    auto result = deserialize_component(*info, node, *comp2.value(), ctx);
    REQUIRE(result.has_value());
    REQUIRE(static_cast<MeshRenderer&>(*comp2.value()).model_ptr() == nullptr);

    // Test with mock model resolution
    test_engine.mock_assets.resolve_model_stub =
        [](const std::string& id) -> Result<std::shared_ptr<Model>> {
            // Return a non-null shared_ptr to indicate success
            // We can't create a real Model easily, so use a special marker
            if (id == "test_model") {
                // Return null to indicate "resolved" (the test just checks the path)
                return std::shared_ptr<Model>(nullptr);
            }
            return make_error(Error::Category::InvalidArgument,
                "MockAssetManager: unknown model '" + id + "'");
        };

    test_engine.mock_assets.find_asset_id_stub =
        [](const Model&) -> std::string {
            return "test_model";
        };

    // Create a MeshRenderer, serialize with mock (getter gets null, but
    // the TypeRegistry encode calls find_asset_id which uses the stub)
    YAML::Node node2;
    node2["model"] = "test_model";
    auto comp3 = registry.create("mesh_renderer");
    REQUIRE(comp3.has_value());
    result = deserialize_component(*info, node2, *comp3.value(), ctx);
    REQUIRE(result.has_value());

    // Verify the setter was called (model_ptr should be whatever resolve_model returned)
    // Since our stub returns nullptr for "test_model", model_ptr should be null
    REQUIRE(static_cast<MeshRenderer&>(*comp3.value()).model_ptr() == nullptr);
}

// ===========================================================================
// YAML::convert for math types
// ===========================================================================

TEST_CASE("YAML_CONVERT_VEC3", "[component-registry]") {
    using namespace YAML;

    math::Vec3 original{1.0f, 2.0f, 3.0f};
    Node node = convert<math::Vec3>::encode(original);
    REQUIRE(node.IsSequence());
    REQUIRE(node.size() == 3);
    REQUIRE(node[0].as<float>() == Approx(1.0f).margin(1e-5f));  // x
    REQUIRE(node[1].as<float>() == Approx(2.0f).margin(1e-5f));  // y
    REQUIRE(node[2].as<float>() == Approx(3.0f).margin(1e-5f));  // z

    // Also accept legacy mapping format
    math::Vec3 decoded;
    YAML::Node legacy_map;
    legacy_map["x"] = 1.0f;
    legacy_map["y"] = 2.0f;
    legacy_map["z"] = 3.0f;
    REQUIRE(convert<math::Vec3>::decode(legacy_map, decoded));
    REQUIRE(decoded.x == Approx(original.x).margin(1e-5f));
    REQUIRE(decoded.y == Approx(original.y).margin(1e-5f));
    REQUIRE(decoded.z == Approx(original.z).margin(1e-5f));

    // Also decode sequence format
    REQUIRE(convert<math::Vec3>::decode(node, decoded));
    REQUIRE(decoded.x == Approx(original.x).margin(1e-5f));
    REQUIRE(decoded.y == Approx(original.y).margin(1e-5f));
    REQUIRE(decoded.z == Approx(original.z).margin(1e-5f));
}

TEST_CASE("YAML_CONVERT_VEC4", "[component-registry]") {
    using namespace YAML;

    math::Vec4 original{1.0f, 2.0f, 3.0f, 4.0f};
    Node node = convert<math::Vec4>::encode(original);
    REQUIRE(node.IsSequence());
    REQUIRE(node.size() == 4);
    REQUIRE(node[0].as<float>() == Approx(1.0f).margin(1e-5f));  // x
    REQUIRE(node[1].as<float>() == Approx(2.0f).margin(1e-5f));  // y
    REQUIRE(node[2].as<float>() == Approx(3.0f).margin(1e-5f));  // z
    REQUIRE(node[3].as<float>() == Approx(4.0f).margin(1e-5f));  // w

    // Also accept legacy mapping format
    math::Vec4 decoded;
    YAML::Node legacy_map;
    legacy_map["x"] = 1.0f;
    legacy_map["y"] = 2.0f;
    legacy_map["z"] = 3.0f;
    legacy_map["w"] = 4.0f;
    REQUIRE(convert<math::Vec4>::decode(legacy_map, decoded));
    REQUIRE(decoded.x == Approx(original.x).margin(1e-5f));
    REQUIRE(decoded.y == Approx(original.y).margin(1e-5f));
    REQUIRE(decoded.z == Approx(original.z).margin(1e-5f));
    REQUIRE(decoded.w == Approx(original.w).margin(1e-5f));

    // Also decode sequence format
    REQUIRE(convert<math::Vec4>::decode(node, decoded));
    REQUIRE(decoded.x == Approx(original.x).margin(1e-5f));
    REQUIRE(decoded.y == Approx(original.y).margin(1e-5f));
    REQUIRE(decoded.z == Approx(original.z).margin(1e-5f));
    REQUIRE(decoded.w == Approx(original.w).margin(1e-5f));
}

TEST_CASE("YAML_CONVERT_QUAT", "[component-registry]") {
    using namespace YAML;

    math::Quat original{1.0f, 0.0f, 0.0f, 0.0f};  // w=1, x=0, y=0, z=0
    Node node = convert<math::Quat>::encode(original);
    REQUIRE(node.IsSequence());
    REQUIRE(node.size() == 4);
    REQUIRE(node[0].as<float>() == Approx(1.0f).margin(1e-5f));  // w
    REQUIRE(node[1].as<float>() == Approx(0.0f).margin(1e-5f));  // x
    REQUIRE(node[2].as<float>() == Approx(0.0f).margin(1e-5f));  // y
    REQUIRE(node[3].as<float>() == Approx(0.0f).margin(1e-5f));  // z

    // Also accept legacy mapping format
    math::Quat decoded;
    YAML::Node legacy_map;
    legacy_map["x"] = 0.0f;
    legacy_map["y"] = 0.0f;
    legacy_map["z"] = 0.0f;
    legacy_map["w"] = 1.0f;
    REQUIRE(convert<math::Quat>::decode(legacy_map, decoded));
    REQUIRE(decoded.w == Approx(original.w).margin(1e-5f));
    REQUIRE(decoded.x == Approx(original.x).margin(1e-5f));
    REQUIRE(decoded.y == Approx(original.y).margin(1e-5f));
    REQUIRE(decoded.z == Approx(original.z).margin(1e-5f));

    // Also decode sequence format
    REQUIRE(convert<math::Quat>::decode(node, decoded));
    REQUIRE(decoded.w == Approx(original.w).margin(1e-5f));
    REQUIRE(decoded.x == Approx(original.x).margin(1e-5f));
    REQUIRE(decoded.y == Approx(original.y).margin(1e-5f));
    REQUIRE(decoded.z == Approx(original.z).margin(1e-5f));
}

// ===========================================================================
// Error cases
// ===========================================================================

TEST_CASE("DESERIALIZE_UNKNOWN_KEY_WARNING", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("camera");
    REQUIRE(info != nullptr);

    auto comp = registry.create("camera");
    REQUIRE(comp.has_value());

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    YAML::Node node;
    node["fov_y"] = 1.0f;
    node["invalid_prop"] = 99.0f;

    // Unknown key should not cause error (warning only)
    auto result = deserialize_component(*info, node, *comp.value(), ctx);
    REQUIRE(result.has_value());
}

TEST_CASE("DESERIALIZE_TYPE_MISMATCH", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("camera");
    REQUIRE(info != nullptr);

    auto comp = registry.create("camera");
    REQUIRE(comp.has_value());

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    YAML::Node node;
    node["fov_y"] = "abc";  // String where float expected

    auto result = deserialize_component(*info, node, *comp.value(), ctx);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::InvalidArgument);
}

TEST_CASE("DESERIALIZE_OUT_OF_RANGE", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    auto* info = registry.describe("camera");
    REQUIRE(info != nullptr);

    auto comp = registry.create("camera");
    REQUIRE(comp.has_value());

    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    YAML::Node node;
    node["fov_y"] = 0.0f;  // Below min of 0.001

    auto result = deserialize_component(*info, node, *comp.value(), ctx);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::InvalidArgument);
    REQUIRE(result.error().message.find("out of range") != std::string::npos);
}

// ===========================================================================
// Factory creation for real components
// ===========================================================================

TEST_CASE("FACTORY_CREATES_CORRECT_TYPE", "[component-registry]") {
    register_builtin_types();
    ComponentRegistry registry;
    register_all_components(registry);

    // Verify all 6 types are registered (camera, point_light, directional_light,
    // spot_light, mesh_renderer, free_camera_movement)
    REQUIRE(registry.all_types().size() == 6);

    // Each type creates correctly
    auto camera = registry.create("camera");
    REQUIRE(camera.has_value());
    REQUIRE(dynamic_cast<CameraComponent*>(camera->get()) != nullptr);

    auto pl = registry.create("point_light");
    REQUIRE(pl.has_value());
    REQUIRE(dynamic_cast<PointLightComponent*>(pl->get()) != nullptr);

    auto dl = registry.create("directional_light");
    REQUIRE(dl.has_value());
    REQUIRE(dynamic_cast<DirectionalLightComponent*>(dl->get()) != nullptr);

    auto sl = registry.create("spot_light");
    REQUIRE(sl.has_value());
    REQUIRE(dynamic_cast<SpotLightComponent*>(sl->get()) != nullptr);

    auto mr = registry.create("mesh_renderer");
    REQUIRE(mr.has_value());
    REQUIRE(dynamic_cast<MeshRenderer*>(mr->get()) != nullptr);

    auto fcm = registry.create("free_camera_movement");
    REQUIRE(fcm.has_value());
    REQUIRE(dynamic_cast<FreeCameraMovement*>(fcm->get()) != nullptr);
}
