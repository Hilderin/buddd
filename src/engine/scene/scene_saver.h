#pragma once

#include "error.h"
#include "scene/entity.h"
#include "scene/entity_source.h"

#include <string>
#include <typeindex>
#include <unordered_map>

// Forward declare YAML types — no yaml-cpp include in public header per ADR-016 / ADR-019.
namespace YAML {
class Node;
}

namespace buddd::engine {

class World;
class ComponentRegistry;
class AssetManager;
class ComponentInfoBase;
struct SerializationContext;

class SceneSaver {
public:
    SceneSaver(World& world, ComponentRegistry& registry, AssetManager& assets);

    /// Save the World to a YAML file.
    [[nodiscard]] auto save_to_file(const std::string& path) -> Result<void>;

    /// Save the World to a YAML::Node (useful for testing and in-memory workflows).
    [[nodiscard]] auto save_to_yaml() -> YAML::Node;

private:
    /// Serialize a single entity to a YAML node.
    auto save_entity(Entity entity) -> YAML::Node;

    /// Serialize a Transform to a YAML node.
    auto save_transform(const Transform& t) -> YAML::Node;

    /// Build the reverse-lookup map from std::type_index to ComponentInfoBase*.
    void build_type_to_info_map();

    World& world_;
    ComponentRegistry& registry_;
    AssetManager& assets_;

    /// Reverse map: std::type_index(Component subclass) → ComponentInfoBase*
    std::unordered_map<std::type_index, const ComponentInfoBase*> type_to_info_;
};

} // namespace buddd::engine
