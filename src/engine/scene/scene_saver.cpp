#include "scene/scene_saver.h"

#include "asset/asset_manager.h"
#include "debug/assert.h"
#include "log/log.h"
#include "error.h"
#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/component_info.h"
#include "scene/component_registry/serialization.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/world.h"
#include "scene/transform.h"

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <typeindex>
#include <unordered_map>

BUDDD_LOG_TAG("SceneSaver");

namespace buddd::engine {

namespace {

/// Strip base path prefix and file extension from a resolved prefab/model path.
/// This ensures saved YAML files contain relative paths without extensions,
/// matching the format that SceneLoader::resolve_prefab_path() expects.
/// Example: "assets/prefabs/free_camera.yaml" → "prefabs/free_camera"
auto sanitize_asset_path(const std::string& path, std::string_view base_path) -> std::string {
    std::string result = path;

    // Strip base path prefix if present (e.g., "assets/prefabs/..." → "prefabs/...")
    if (result.size() > base_path.size() + 1 &&
        result.compare(0, base_path.size(), base_path.data()) == 0 &&
        result[base_path.size()] == '/')
    {
        result = result.substr(base_path.size() + 1);
    }

    // Strip .yaml/.yml extension if present
    for (auto ext : {".yaml", ".yml"}) {
        auto ext_len = std::char_traits<char>::length(ext);
        if (result.size() > ext_len &&
            result.compare(result.size() - ext_len, ext_len, ext) == 0)
        {
            result = result.substr(0, result.size() - ext_len);
            break;
        }
    }

    return result;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
SceneSaver::SceneSaver(World& world, ComponentRegistry& registry, AssetManager& assets)
    : world_(world)
    , registry_(registry)
    , assets_(assets)
{
    build_type_to_info_map();
}

// ---------------------------------------------------------------------------
// build_type_to_info_map
// ---------------------------------------------------------------------------
void SceneSaver::build_type_to_info_map() {
    for (const auto* info : registry_.all_types()) {
        // registry_.all_types() returns const pointers, but create() is non-const.
        // The underlying objects in ComponentRegistry::infos_ are stored as
        // non-const unique_ptr<ComponentInfoBase> — the const in all_types() is
        // an access-level constraint, not an ownership qualifier. create() does
        // not modify the ComponentInfoBase state (it constructs a new Component
        // subclass instance). So const_cast is safe here.
        auto* mutable_info = const_cast<ComponentInfoBase*>(info);
        auto tmp = mutable_info->create();
        BUDDD_ASSERT(tmp != nullptr);
        type_to_info_[typeid(*tmp)] = info;
    }
}

// ---------------------------------------------------------------------------
// save_to_file
// ---------------------------------------------------------------------------
auto SceneSaver::save_to_file(const std::string& path) -> Result<void> {
    BUDDD_LOG_INFO("Saving scene: {}", path);

    YAML::Node node;
    try {
        node = save_to_yaml();
    } catch (const YAML::Exception& e) {
        return make_error(Error::Category::IoFailed,
            "Failed to build YAML node: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return make_error(Error::Category::IoFailed,
            "Unexpected error saving scene: " + std::string(e.what()));
    }

    try {
        std::ofstream fout(path);
        if (!fout.is_open()) {
            return make_error(Error::Category::IoFailed,
                "Failed to open file for writing: " + path);
        }
        fout << node;
        if (!fout.good()) {
            return make_error(Error::Category::IoFailed,
                "Failed to write YAML to file: " + path);
        }
    } catch (const std::exception& e) {
        return make_error(Error::Category::IoFailed,
            "I/O error writing scene file: " + std::string(e.what()));
    }

    // Count non-pending-destroy entities for the log message
    size_t entity_count = 0;
    for (size_t i = 0; i < world_.root_entity_count(); ++i) {
        auto entity = world_.get_root_entity(i);
        if (!entity.is_pending_destroy()) ++entity_count;
    }

    BUDDD_LOG_INFO("Scene saved: {} entities", entity_count);
    return {};
}

// ---------------------------------------------------------------------------
// save_to_yaml
// ---------------------------------------------------------------------------
auto SceneSaver::save_to_yaml() -> YAML::Node {
    YAML::Node root;
    root["type"] = "Scene";
    root["version"] = 1;

    YAML::Node entities = YAML::Node(YAML::NodeType::Sequence);
    for (size_t i = 0; i < world_.root_entity_count(); ++i) {
        auto entity = world_.get_root_entity(i);
        if (entity.is_pending_destroy()) continue;
        entities.push_back(save_entity(entity));
    }
    root["entities"] = entities;

    return root;
}

// ---------------------------------------------------------------------------
// save_entity
// ---------------------------------------------------------------------------
auto SceneSaver::save_entity(Entity entity) -> YAML::Node {
    YAML::Node node;

    const auto& src = entity.source();

    auto maybe_transform = [&]() -> YAML::Node {
        auto t = save_transform(entity.transform());
        // Only emit transform if at least one field is non-default
        return t.IsMap() && t.size() > 0 ? t : YAML::Node{};
    };

    // ── Prefab source: emit only prefab ref + name + transform ──
    if (src.type == EntitySourceType::Prefab) {
        node["prefab"] = sanitize_asset_path(src.path, assets_.base_path());
        node["name"] = entity.name();
        if (auto t = maybe_transform(); !t.IsNull()) node["transform"] = t;
        return node;
    }

    // ── Model source: emit only name + model ref + transform ──
    if (src.type == EntitySourceType::Model) {
        node["name"] = entity.name();
        node["model"] = sanitize_asset_path(src.path, assets_.base_path());
        if (auto t = maybe_transform(); !t.IsNull()) node["transform"] = t;
        return node;
    }

    // ── None source: full entity serialization ──
    node["name"] = entity.name();
    if (auto t = maybe_transform(); !t.IsNull()) node["transform"] = t;

    // Serialize components
    if (entity.component_count() > 0) {
        YAML::Node components;
        SerializationContext ctx{assets_};
        for (size_t i = 0; i < entity.component_count(); ++i) {
            Component& comp = entity.component_at(i);
            auto it = type_to_info_.find(typeid(comp));
            if (it == type_to_info_.end()) {
                // Unregistered component type — this is an error in the World state.
                // Propagate by throwing an exception that will be caught by save_to_file().
                throw std::runtime_error(
                    "Unregistered component type encountered during scene save: " +
                    std::string(typeid(comp).name()));
            }
            const ComponentInfoBase* info = it->second;
            YAML::Node comp_node;
            comp_node["type"] = std::string(info->type_name());
            auto props = serialize_component(*info, comp, ctx);
            // Only include properties if there are non-default ones
            if (props.IsMap() && props.size() > 0) {
                comp_node["properties"] = std::move(props);
            }
            components.push_back(comp_node);
        }
        // Only include components if there's at least one
        if (components.size() > 0) {
            node["components"] = components;
        }
    }

    // Recursively save children
    if (entity.child_count() > 0) {
        YAML::Node children;
        for (size_t i = 0; i < entity.child_count(); ++i) {
            auto child = entity.get_child(i);
            children.push_back(save_entity(child));
        }
        node["children"] = children;
    }

    return node;
}

// ---------------------------------------------------------------------------
// save_transform
// ---------------------------------------------------------------------------
auto SceneSaver::save_transform(const Transform& t) -> YAML::Node {
    YAML::Node node;
    auto ser_ctx = SerializationContext{assets_};

    // Only emit fields that differ from defaults (skip default-valued fields
    // for cleaner YAML output).

    // Default position: [0, 0, 0]
    if (t.position.x != 0.0f || t.position.y != 0.0f || t.position.z != 0.0f) {
        auto encoded = TypeRegistry::yaml_encode(t.position, ser_ctx);
        if (encoded) {
            node["position"] = *std::move(encoded);
        }
    }

    // Default rotation: identity quaternion [1, 0, 0, 0]
    if (t.rotation.w != 1.0f || t.rotation.x != 0.0f || t.rotation.y != 0.0f || t.rotation.z != 0.0f) {
        auto encoded = TypeRegistry::yaml_encode(t.rotation, ser_ctx);
        if (encoded) {
            node["rotation"] = *std::move(encoded);
        }
    }

    // Default scale: [1, 1, 1]
    if (t.scale.x != 1.0f || t.scale.y != 1.0f || t.scale.z != 1.0f) {
        auto encoded = TypeRegistry::yaml_encode(t.scale, ser_ctx);
        if (encoded) {
            node["scale"] = *std::move(encoded);
        }
    }

    return node;
}

} // namespace buddd::engine
