#pragma once

#include "render/model_node.h"
#include "render/mesh_renderer.h"
#include "scene/entity.h"
#include "scene/world.h"

#include <memory>

namespace buddd::engine {

/// Traverses a ModelNode tree depth-first and creates ECS entities for each
/// mesh node. Each entity gets a Transform (set from the node's TRS) and a
/// MeshRenderer (holding the node's Model via shared_ptr). Parent-child
/// relationships in the entity hierarchy mirror the ModelNode tree.
///
/// @param world   The World (ECS container) to create entities in.
/// @param node    The root ModelNode to traverse.
/// @param parent  Optional parent Entity for hierarchy (Entity::none() = no parent).
/// @return The Entity created for `node`, or Entity::none() if the node has no mesh.
[[nodiscard]] auto add_model_to_world(
    World& world,
    ModelNode& node,
    Entity parent = Entity::none()
) -> Entity;

namespace detail {

/// Internal recursive implementation of add_model_to_world.
[[nodiscard]] inline auto add_model_to_world_impl(
    World& world,
    ModelNode& node,
    Entity parent,
    std::vector<std::shared_ptr<Material>>& shared_materials)
    -> Entity
{
    Entity entity = Entity::none();

    if (node.model.has_value()) {
        entity = Entity::create(world);
        entity.transform().position = node.translation;
        entity.transform().rotation = node.rotation;
        entity.transform().scale = node.scale;

        // Create a shared_ptr<Model> by moving the Model out of the node.
        // Note: this consumes the Model from the node (Model is move-only).
        auto model_ptr = std::make_shared<Model>(std::move(*node.model));
        entity.add_component<MeshRenderer>(std::move(model_ptr));

        if (parent.id() != EntityId::none()) {
            entity.reparent(parent);
        }
    }

    for (auto& child : node.children) {
        detail::add_model_to_world_impl(world, child, entity, shared_materials);
    }

    return entity;
}

} // namespace detail

inline auto add_model_to_world(
    World& world,
    ModelNode& node,
    Entity parent) -> Entity
{
    std::vector<std::shared_ptr<Material>> shared_materials;
    return detail::add_model_to_world_impl(world, node, parent, shared_materials);
}

} // namespace buddd::engine
