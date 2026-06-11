#pragma once

#include "error.h"
#include "scene/entity.h"
#include "scene/transform.h"

#include <string>
#include <unordered_set>

// Forward declare YAML types — no yaml-cpp include in public header per ADR-016 / ADR-019.
namespace YAML {
class Node;
}

namespace buddd::engine {

class World;
class ComponentRegistry;
class AssetManager;

class SceneLoader {
public:
    SceneLoader(World& world, ComponentRegistry& registry, AssetManager& assets);

    /// Load a scene YAML file and populate the World.
    [[nodiscard]] auto load_from_file(const std::string& path) -> Result<void>;

    /// Load from an already-parsed YAML node.
    [[nodiscard]] auto load_from_yaml(const YAML::Node& node) -> Result<void>;

    /// Compose prefab root transform with instance transform (public for testing).
    static auto compose_transform(const Transform& prefab, const Transform& instance) -> Transform;

private:
    /// Load and return a single entity from a YAML node.
    /// If parent is not Entity::none(), the entity is created as a child of parent.
    [[nodiscard]] auto load_entity(const YAML::Node& node, Entity parent = Entity::none()) -> Result<Entity>;

    /// Load a prefab YAML file and return the root entity (for transform composition).
    [[nodiscard]] auto load_prefab(const std::string& path) -> Result<Entity>;

    /// Parse a transform block from a YAML node, applying defaults for missing fields.
    auto parse_transform(const YAML::Node& node) -> Transform;

    World& world_;
    ComponentRegistry& registry_;
    AssetManager& assets_;

    /// Set of prefab paths currently being loaded (cycle detection).
    std::unordered_set<std::string> loading_prefabs_;
};

} // namespace buddd::engine
