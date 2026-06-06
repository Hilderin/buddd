#include "scene/world.h"
#include "scene/camera_component.h"
#include "scene/updatable.h"

#include <algorithm>

namespace buddd::engine {

// ---------------------------------------------------------------------------
// World constructor / destructor
// ---------------------------------------------------------------------------
World::World()
    : slots_()
    , roots_()
    , pending_destroy_()
    , free_slots_()
    , next_slot_(0)
{}

World::~World() {
    // All EntityNode destructors run via unique_ptr in roots_ or in parent
    // children_ vectors — this destroys all components and children
    // recursively. No explicit flush_destroyed() is required.
}

// ---------------------------------------------------------------------------
// Entity lifecycle
// ---------------------------------------------------------------------------
auto World::create_entity() -> Entity {
    // Allocate a slot index
    uint32_t index;
    if (!free_slots_.empty()) {
        index = free_slots_.back();
        free_slots_.pop_back();
    } else {
        index = next_slot_++;
        if (index >= slots_.size()) {
            slots_.resize(index + 1);
        }
    }

    EntityId id{index, slots_[index].generation};

    auto node = std::make_unique<EntityNode>();
    node->id_ = id;
    node->transform_ = Transform{};
    node->parent_ = nullptr;
    node->world_ = this;

    // Store the raw pointer in the slot (non-owning) for lookup
    slots_[index].node = node.get();
    slots_[index].alive = true;

    // Transfer ownership to roots_
    roots_.push_back(std::move(node));

    return Entity(*this, id);
}

void World::destroy_entity(Entity entity) {
    auto* node = lookup_node(entity.id());
    if (node) {
        mark_for_destroy(node);
    }
}

void World::flush_destroyed() noexcept {
    // Iterate in reverse order (deepest children first, then parents)
    for (auto it = pending_destroy_.rbegin(); it != pending_destroy_.rend(); ++it) {
        EntityNode* node = *it;
        if (!node) continue;

        uint32_t index = node->id_.index;

        // Unlink from parent (or roots_), which destroys the node
        // via unique_ptr going out of scope
        if (node->parent_) {
            auto& siblings = node->parent_->children_;
            for (auto sit = siblings.begin(); sit != siblings.end(); ++sit) {
                if (sit->get() == node) {
                    auto owned = std::move(*sit);
                    // Clean up Updatable pointers before the component unique_ptrs are destroyed
                    for (auto& c : owned->components_) {
                        if (auto* upd = dynamic_cast<Updatable*>(c.get())) {
                            std::erase(updatables_, upd);
                        }
                    }
                    siblings.erase(sit);
                    // owned goes out of scope here, destroying the node
                    // and recursively its remaining children
                    break;
                }
            }
        } else {
            // Root entity — find and erase from roots_
            for (auto rit = roots_.begin(); rit != roots_.end(); ++rit) {
                if (rit->get() == node) {
                    auto owned = std::move(*rit);
                    // Clean up Updatable pointers before the component unique_ptrs are destroyed
                    for (auto& c : owned->components_) {
                        if (auto* upd = dynamic_cast<Updatable*>(c.get())) {
                            std::erase(updatables_, upd);
                        }
                    }
                    roots_.erase(rit);
                    // owned goes out of scope here, destroying the node
                    break;
                }
            }
        }

        // Mark slot as dead — node pointer is now dangling so clear it
        slots_[index].node = nullptr;
        slots_[index].alive = false;
        slots_[index].generation++;
        free_slots_.push_back(index);
    }

    pending_destroy_.clear();
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
auto World::lookup_node(EntityId id) noexcept -> EntityNode* {
    if (id.index >= slots_.size()) return nullptr;
    auto& slot = slots_[id.index];
    if (!slot.alive) return nullptr;
    if (slot.generation != id.generation) return nullptr;
    return slot.node;
}

auto World::lookup_node(EntityId id) const noexcept -> const EntityNode* {
    if (id.index >= slots_.size()) return nullptr;
    const auto& slot = slots_[id.index];
    if (!slot.alive) return nullptr;
    if (slot.generation != id.generation) return nullptr;
    return slot.node;
}

auto World::is_pending_destroy(EntityId id) const noexcept -> bool {
    auto* node = lookup_node(id);
    return node && node->pending_destroy_;
}

auto World::get_transform(EntityId id) noexcept -> Transform& {
    auto* node = lookup_node(id);
    // UB if node is null
    return node->transform_;
}

auto World::get_transform(EntityId id) const noexcept -> const Transform& {
    auto* node = lookup_node(id);
    // UB if node is null
    return node->transform_;
}

// ---------------------------------------------------------------------------
// Hierarchy
// ---------------------------------------------------------------------------
auto World::create_child(EntityId parent_id) -> Entity {
    auto* parent_node = lookup_node(parent_id);
    // UB if parent_node is null or pending_destroy_

    // Allocate a slot for the child
    uint32_t index;
    if (!free_slots_.empty()) {
        index = free_slots_.back();
        free_slots_.pop_back();
    } else {
        index = next_slot_++;
        if (index >= slots_.size()) {
            slots_.resize(index + 1);
        }
    }

    EntityId id{index, slots_[index].generation};

    auto child_node = std::make_unique<EntityNode>();
    child_node->id_ = id;
    child_node->transform_ = Transform{};
    child_node->parent_ = parent_node;
    child_node->world_ = this;

    // Store raw pointer in slot for lookup
    slots_[index].node = child_node.get();
    slots_[index].alive = true;

    // Transfer ownership to parent's children list
    parent_node->children_.push_back(std::move(child_node));

    return Entity(*this, id);
}

void World::mark_for_destroy(EntityNode* node) {
    if (!node || node->pending_destroy_) return;

    std::vector<EntityNode*> stack;
    stack.push_back(node);

    while (!stack.empty()) {
        auto* n = stack.back();
        stack.pop_back();

        if (n->pending_destroy_) continue;

        n->pending_destroy_ = true;
        pending_destroy_.push_back(n);

        for (const auto& child : n->children_) {
            stack.push_back(child.get());
        }
    }
}

auto World::get_parent(EntityId id) const noexcept -> Entity {
    auto* node = lookup_node(id);
    if (!node || !node->parent_) {
        return Entity{};
    }
    return Entity(*const_cast<World*>(this), node->parent_->id_);
}

auto World::get_child_count(EntityId id) const noexcept -> size_t {
    auto* node = lookup_node(id);
    if (!node) return 0;
    return node->children_.size();
}

auto World::get_child(EntityId id, size_t index) const noexcept -> Entity {
    auto* node = lookup_node(id);
    // UB if node is null or index out of bounds
    return Entity(*const_cast<World*>(this), node->children_[index]->id_);
}

void World::reparent(EntityId id, EntityId new_parent_id) {
    auto* node = lookup_node(id);
    auto* new_parent = (new_parent_id.index < slots_.size() && slots_[new_parent_id.index].alive
                        && slots_[new_parent_id.index].generation == new_parent_id.generation)
                       ? lookup_node(new_parent_id) : nullptr;

    // UB if node is null, self-reparent, cross-world, cycle, etc.
    if (!node) return;
    if (node->parent_ == new_parent) return; // no-op

    std::unique_ptr<EntityNode> owned;

    // Unlink from current parent (or roots_)
    if (node->parent_) {
        auto& siblings = node->parent_->children_;
        for (auto it = siblings.begin(); it != siblings.end(); ++it) {
            if (it->get() == node) {
                owned = std::move(*it);
                siblings.erase(it);
                break;
            }
        }
    } else {
        for (auto it = roots_.begin(); it != roots_.end(); ++it) {
            if (it->get() == node) {
                owned = std::move(*it);
                roots_.erase(it);
                break;
            }
        }
    }

    // Attach to new parent (or roots_)
    if (new_parent) {
        new_parent->children_.push_back(std::move(owned));
        node->parent_ = new_parent;
    } else {
        roots_.push_back(std::move(owned));
        node->parent_ = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Updatable dispatch
// ---------------------------------------------------------------------------

void World::update_updatables(const EngineContext& ctx) {
    for (auto* upd : updatables_) {
        upd->update(ctx);
    }
}

// ---------------------------------------------------------------------------
// Camera registration
// ---------------------------------------------------------------------------

void World::register_camera(CameraComponent& camera) {
    // ADR-011: raw pointer allowed as private data member (implementation detail).
    active_camera_ = &camera;
}

void World::unregister_camera(const CameraComponent& camera) {
    // Address comparison: only clear if this component is the active camera
    if (active_camera_ == &camera) {
        active_camera_ = nullptr;
    }
}

auto World::active_camera() const noexcept -> std::optional<CameraComponent&> {
    if (!active_camera_) {
        return std::nullopt;
    }
    return *active_camera_;
}

} // namespace buddd::engine
