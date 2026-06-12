#pragma once

#include "command.h"
#include "editor_context.h"
#include "editor_selection.h"  // for EntityId hash, Selection

#include "log/log.h"
#include "scene/entity.h"
#include "scene/world.h"

#include <string>
#include <string_view>
#include <vector>

namespace buddd::editor {

/// Serialisable state of a single deleted entity (identity, name, hierarchy).
/// Component state is NOT preserved in v1.
struct SavedEntityState {
    buddd::engine::EntityId old_id;
    std::string name;
    buddd::engine::EntityId parent_old_id;  // EntityId::none() for root
};

/// Command that destroys one or more entities with confirmation-gated destruction.
/// Snapshot of selection saved pre-execution and restored on undo.
class DeleteEntityCommand final : public Command {
public:
    /// @param entity_ids  The top-level entities to delete (children are derived automatically).
    explicit DeleteEntityCommand(std::vector<buddd::engine::EntityId> entity_ids)
        : entity_ids_(std::move(entity_ids))
    {}

    auto execute(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();

        // Save pre-execution selection for undo
        pre_execution_selection_ = ctx.editor.selection().snapshot();

        // Collect all entities to delete (selected + all descendants)
        saved_entities_.clear();

        auto collect_and_save = [&](auto& self, buddd::engine::Entity entity) -> void {
            auto parent_entity = entity.parent();
            SavedEntityState state;
            state.old_id = entity.id();
            state.name = entity.name();
            state.parent_old_id = parent_entity.id();
            saved_entities_.push_back(std::move(state));

            for (size_t i = 0; i < entity.child_count(); ++i) {
                self(self, entity.get_child(i));
            }
        };

        // Find each top-level entity and collect
        for (auto id : entity_ids_) {
            auto find_and_collect = [&](auto& self, buddd::engine::Entity e) -> bool {
                if (e.id() == id) {
                    collect_and_save(self, e);
                    return true;
                }
                for (size_t i = 0; i < e.child_count(); ++i) {
                    if (self(self, e.get_child(i))) return true;
                }
                return false;
            };
            // Search roots
            for (size_t i = 0; i < world.root_entity_count(); ++i) {
                auto root = world.get_root_entity(i);
                if (root.id() == id) {
                    collect_and_save(collect_and_save, root);
                    break;
                }
                if (find_and_collect(find_and_collect, root)) break;
            }
        }

        // Destroy top-level entities only.
        // Entity::destroy() recursively marks all descendants via mark_for_destroy,
        // so we only need to destroy the top-level entities. Destroying children
        // individually would cause dangling pointers in World::flush_destroyed()
        // when a parent is flushed before its child in pending_destroy_.
        for (auto id : entity_ids_) {
            auto find_and_destroy = [&](auto& self, buddd::engine::Entity e) -> bool {
                if (e.id() == id) {
                    e.destroy();
                    return true;
                }
                for (size_t i = 0; i < e.child_count(); ++i) {
                    if (self(self, e.get_child(i))) return true;
                }
                return false;
            };
            for (size_t i = 0; i < world.root_entity_count(); ++i) {
                auto root = world.get_root_entity(i);
                if (root.id() == id) {
                    root.destroy();
                    break;
                }
                if (find_and_destroy(find_and_destroy, root)) break;
            }
        }

        // Clear selection
        ctx.editor.selection().clear();

        BUDDD_LOG_TAGGED_DEBUG("Editor", "DeleteEntity: {} entities destroyed", saved_entities_.size());
    }

    auto undo(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();

        // Recreate entities in saved order (parents first — already parent-first
        // because we collected parent-first during execute)
        // Map old EntityId → new Entity entity for parenting lookup
        struct RecreatedEntity {
            buddd::engine::EntityId old_id;
            buddd::engine::EntityId new_id;
        };
        std::vector<RecreatedEntity> recreated;

        for (auto& saved : saved_entities_) {
            buddd::engine::Entity new_entity;

            if (saved.parent_old_id != buddd::engine::EntityId::none()) {
                // Find parent: either recreated (previously deleted + recreated now)
                // or still alive in the World (was not deleted).
                auto parent_it = std::find_if(recreated.begin(), recreated.end(),
                    [&](const RecreatedEntity& r) { return r.old_id == saved.parent_old_id; });

                // Helper: find an entity by ID in the World tree and create a child.
                auto create_child_of = [&](buddd::engine::EntityId target_id) -> buddd::engine::Entity {
                    auto find_in_world = [&](auto& self, buddd::engine::Entity e) -> std::optional<buddd::engine::Entity> {
                        if (e.id() == target_id) return e;
                        for (size_t i = 0; i < e.child_count(); ++i) {
                            auto found = self(self, e.get_child(i));
                            if (found.has_value()) return found;
                        }
                        return std::nullopt;
                    };
                    for (size_t i = 0; i < world.root_entity_count(); ++i) {
                        auto root = world.get_root_entity(i);
                        if (root.id() == target_id) return root.create_child();
                        auto found = find_in_world(find_in_world, root);
                        if (found.has_value()) return found->create_child();
                    }
                    return {};  // not found
                };

                if (parent_it != recreated.end()) {
                    // Parent was recreated — use its new EntityId
                    new_entity = create_child_of(parent_it->new_id);
                } else {
                    // Parent still exists in the World — search by original ID
                    new_entity = create_child_of(saved.parent_old_id);
                }

                // Fallback if parent not found anywhere
                if (new_entity.id() == buddd::engine::EntityId::none()) {
                    new_entity = world.add_entity();
                    BUDDD_LOG_TAGGED_DEBUG("Editor", "DeleteEntity::undo: parent {} not found, created as root",
                        saved.parent_old_id.index);
                }
            } else {
                new_entity = world.add_entity();
            }

            // Set name
            new_entity.set_name(saved.name);

            recreated.push_back({saved.old_id, new_entity.id()});
        }

        // Update entity_ids_ with new EntityIds so redo's execute()
        // can find the recreated entities by their current IDs.
        for (auto& id : entity_ids_) {
            for (auto const& r : recreated) {
                if (r.old_id == id) {
                    id = r.new_id;
                    break;
                }
            }
        }

        // Restore selection
        ctx.editor.selection().restore(pre_execution_selection_);

        BUDDD_LOG_TAGGED_DEBUG("Editor", "DeleteEntity::undo: {} entities recreated", recreated.size());
    }

    [[nodiscard]] auto name() const -> std::string_view override {
        return "Delete Entity";
    }

private:
    std::vector<buddd::engine::EntityId> entity_ids_;
    std::vector<SavedEntityState> saved_entities_;
    Selection pre_execution_selection_;
};

} // namespace buddd::editor
