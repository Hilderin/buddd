#include "scene/scene_loader.h"

#include "debug/assert.h"
#include "log/log.h"
#include "error.h"
#include "scene/component_registry/component_registry.h"
#include "scene/entity_source.h"

#include <yaml-cpp/yaml.h>
#include "asset/model_asset.h"
#include "render/model_utils.h"
#include "scene/component_registry/component_info.h"
#include "scene/component_registry/serialization.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/component_registry/type_registry.h"
#include "scene/transform.h"
#include "scene/world.h"
#include "asset/asset_manager.h"

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_set>

BUDDD_LOG_TAG("SceneLoader");

namespace buddd::engine {

namespace {

auto resolve_prefab_path(const std::string& base, const std::string& prefab_path) -> Result<std::string> {
    // Try .yaml first, then .yml
    auto try_ext = [&](const std::string& ext) -> std::optional<std::string> {
        auto path = base + "/" + prefab_path + ext;
        if (std::filesystem::exists(path)) {
            return path;
        }
        return std::nullopt;
    };

    if (auto r = try_ext(".yaml")) return *r;
    if (auto r = try_ext(".yml")) return *r;

    return make_error(Error::Category::IoFailed,
        "prefab file not found: " + base + "/" + prefab_path + "(.yaml/.yml)");
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
SceneLoader::SceneLoader(World& world, ComponentRegistry& registry, AssetManager& assets)
    : world_(world)
    , registry_(registry)
    , assets_(assets)
{}

// ---------------------------------------------------------------------------
// load_from_file
// ---------------------------------------------------------------------------
auto SceneLoader::load_from_file(const std::string& path) -> Result<void> {
    BUDDD_LOG_INFO("Loading scene: {}", path);

    YAML::Node node;
    try {
        node = YAML::LoadFile(path);
    } catch (const YAML::Exception& e) {
        return make_error(Error::Category::InvalidFormat,
            "YAML parse error: " + std::string(e.what()));
    }

    return load_from_yaml(node);
}

// ---------------------------------------------------------------------------
// load_from_yaml
// ---------------------------------------------------------------------------
auto SceneLoader::load_from_yaml(const YAML::Node& node) -> Result<void> {
    // Validate type field
    if (!node["type"] || !node["type"].IsScalar()) {
        return make_error(Error::Category::InvalidFormat, "missing type field");
    }
    std::string type = node["type"].as<std::string>();
    if (type != "Scene" && type != "Prefab") {
        return make_error(Error::Category::InvalidFormat,
            "unsupported type: " + type);
    }

    // Validate version field
    if (!node["version"] || !node["version"].IsScalar()) {
        return make_error(Error::Category::InvalidFormat, "missing version field");
    }
    int version = node["version"].as<int>();
    if (version != 1) {
        return make_error(Error::Category::InvalidFormat, "unsupported version");
    }

    // Warn about unknown top-level keys (forward compatibility)
    for (const auto& kv : node) {
        auto key = kv.first.as<std::string>();
        if (key != "type" && key != "version" && key != "entities" && key != "metadata") {
            BUDDD_LOG_WARN("Unknown top-level key '{}' in scene file (forward-compatible)", key);
        }
    }

    // Process entities
    if (!node["entities"] || !node["entities"].IsSequence()) {
        BUDDD_LOG_INFO("Scene loaded: 0 entities");
        return {};
    }

    int entity_count = 0;
    for (const auto& entity_node : node["entities"]) {
        auto result = load_entity(entity_node, Entity::none());
        if (!result) {
            return make_error(result.error());
        }
        ++entity_count;
    }

    BUDDD_LOG_INFO("Scene loaded: {} entities", entity_count);
    return {};
}

// ---------------------------------------------------------------------------
// load_entity
// ---------------------------------------------------------------------------
auto SceneLoader::load_entity(const YAML::Node& node, Entity parent) -> Result<Entity> {
    Entity entity = Entity::none();

    // Warn about unknown entity-level keys (AC-009)
    static const std::unordered_set<std::string> known_entity_keys = {
        "prefab", "name", "transform", "components", "children", "model"
    };
    for (const auto& kv : node) {
        auto key = kv.first.as<std::string>();
        if (known_entity_keys.find(key) == known_entity_keys.end()) {
            BUDDD_LOG_WARN("Unknown entity-level key '{}' ignored", key);
        }
    }

    // ── Prefab check ──
    if (node["prefab"]) {
        // Resolve prefab path
        std::string prefab_path = node["prefab"].as<std::string>();
        std::string base(assets_.base_path());
        auto resolved = resolve_prefab_path(base, prefab_path);
        if (!resolved) {
            return make_error(resolved.error());
        }

        // Load prefab — this creates entities in the World and returns the root
        auto prefab_result = load_prefab(*resolved);
        if (!prefab_result) {
            return make_error(prefab_result.error());
        }

        entity = *prefab_result;

        // Set source: Prefab type with resolved path
        entity.set_source(EntitySource{EntitySourceType::Prefab, *resolved});

        // Reparent if needed
        if (parent.id() != EntityId::none()) {
            entity.reparent(parent);
        }

        // Override name
        if (node["name"] && node["name"].IsScalar()) {
            entity.set_name(node["name"].as<std::string>());
        }

        // Compose transform
        if (node["transform"]) {
            auto instance_transform = parse_transform(node["transform"]);
            entity.transform() = compose_transform(entity.transform(), instance_transform);
        }
    } else {
        // Direct entity (no prefab)
        entity = world_.add_entity();

        if (parent.id() != EntityId::none()) {
            entity.reparent(parent);
        }

        // Set name
        if (node["name"] && node["name"].IsScalar()) {
            entity.set_name(node["name"].as<std::string>());
        }

        // Parse transform
        if (node["transform"]) {
            entity.transform() = parse_transform(node["transform"]);
        }
    }

    // ── Model directive (shared) ──
    // If the entity has a model: directive, load the ModelAsset and expand
    // it into child entities via add_model_to_world.
    if (node["model"] && node["model"].IsScalar()) {
        std::string model_path = node["model"].as<std::string>();
        auto model_asset = assets_.create<ModelAsset>(model_path);
        if (!model_asset) {
            return make_error(Error::Category::InvalidArgument,
                "Failed to load model asset '" + model_path
                + "': " + model_asset.error().message);
        }
        auto& root = (*model_asset)->root_node();
        add_model_to_world(world_, root, entity);

        // Set source: Model type with the model path from the YAML
        entity.set_source(EntitySource{EntitySourceType::Model, model_path});
    }

    // ── Component processing (shared) ──
    if (node["components"] && node["components"].IsSequence()) {
        for (const auto& comp_node : node["components"]) {
            if (!comp_node["type"] || !comp_node["type"].IsScalar()) {
                return make_error(Error::Category::InvalidFormat,
                    "component entry missing 'type' field");
            }
            std::string comp_type = comp_node["type"].as<std::string>();

            // Check if component type is known
            const auto* info = registry_.describe(comp_type);
            if (!info) {
                BUDDD_LOG_WARN("Unknown component type '{}' skipped", comp_type);
                continue;
            }

            // Create default component instance
            auto comp_instance = registry_.create(comp_type);
            if (!comp_instance) {
                return make_error(comp_instance.error());
            }

            // Deserialize properties
            if (comp_node["properties"]) {
                SerializationContext ctx{assets_};
                auto deser_result = deserialize_component(
                    *info, comp_node["properties"], *comp_instance.value(), ctx);
                if (!deser_result) {
                    return make_error(Error::Category::InvalidArgument,
                        "Failed to deserialize component '" + comp_type
                        + "': " + deser_result.error().message);
                }
            }

            // Attach component to entity
            world_.add_component_raw(entity.id(), std::move(comp_instance.value()));
        }
    }

    // ── Children processing (shared) ──
    if (node["children"] && node["children"].IsSequence()) {
        for (const auto& child_node : node["children"]) {
            auto child_result = load_entity(child_node, entity);
            if (!child_result) {
                return make_error(child_result.error());
            }
        }
    }

    return entity;
}

// ---------------------------------------------------------------------------
// load_prefab
// ---------------------------------------------------------------------------
auto SceneLoader::load_prefab(const std::string& path) -> Result<Entity> {
    // Cycle detection
    if (loading_prefabs_.find(path) != loading_prefabs_.end()) {
        return make_error(Error::Category::InvalidFormat,
            "circular prefab reference detected: " + path);
    }
    loading_prefabs_.insert(path);

    // Parse the prefab YAML file
    YAML::Node node;
    try {
        node = YAML::LoadFile(path);
    } catch (const YAML::Exception& e) {
        loading_prefabs_.erase(path);
        return make_error(Error::Category::InvalidFormat,
            "YAML parse error in prefab '" + path + "': " + std::string(e.what()));
    }

    // Validate type
    if (!node["type"] || !node["type"].IsScalar()) {
        loading_prefabs_.erase(path);
        return make_error(Error::Category::InvalidFormat,
            "prefab file has invalid type (missing type field)");
    }
    std::string type = node["type"].as<std::string>();
    if (type != "Prefab") {
        loading_prefabs_.erase(path);
        return make_error(Error::Category::InvalidFormat,
            "prefab file has invalid type: '" + type + "' (expected 'Prefab')");
    }

    // Validate version
    if (!node["version"] || !node["version"].IsScalar()) {
        loading_prefabs_.erase(path);
        return make_error(Error::Category::InvalidFormat,
            "prefab file missing version field");
    }
    int version = node["version"].as<int>();
    if (version != 1) {
        loading_prefabs_.erase(path);
        return make_error(Error::Category::InvalidFormat,
            "prefab file has unsupported version: " + std::to_string(version));
    }

    // Validate entities count
    if (!node["entities"] || !node["entities"].IsSequence()) {
        loading_prefabs_.erase(path);
        return make_error(Error::Category::InvalidFormat,
            "prefab must have at least one entity");
    }

    if (node["entities"].size() != 1) {
        loading_prefabs_.erase(path);
        return make_error(Error::Category::InvalidFormat,
            "prefab must have exactly one root entity");
    }

    // Load the single root entity
    auto result = load_entity(node["entities"][0], Entity::none());

    // Clean up cycle-detection set
    loading_prefabs_.erase(path);

    return result;
}

// ---------------------------------------------------------------------------
// parse_transform
// ---------------------------------------------------------------------------
auto SceneLoader::parse_transform(const YAML::Node& node) -> Transform {
    Transform t{};
    SerializationContext ctx{assets_};

    if (node["position"]) {
        auto pos = TypeRegistry::yaml_decode<math::Vec3>(node["position"], ctx);
        if (pos) {
            t.position = *pos;
        }
    }

    if (node["rotation"]) {
        auto rot = TypeRegistry::yaml_decode<math::Quat>(node["rotation"], ctx);
        if (rot) {
            t.rotation = *rot;
        }
    }

    if (node["scale"]) {
        auto sc = TypeRegistry::yaml_decode<math::Vec3>(node["scale"], ctx);
        if (sc) {
            t.scale = *sc;
        }
    }

    return t;
}

// ---------------------------------------------------------------------------
// compose_transform (static)
// ---------------------------------------------------------------------------
auto SceneLoader::compose_transform(const Transform& prefab, const Transform& instance) -> Transform {
    Transform result;
    result.position = prefab.position + instance.position;
    result.scale = math::Vec3{
        prefab.scale.x * instance.scale.x,
        prefab.scale.y * instance.scale.y,
        prefab.scale.z * instance.scale.z
    };
    result.rotation = prefab.rotation * instance.rotation;
    return result;
}

} // namespace buddd::engine
