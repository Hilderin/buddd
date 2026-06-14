#pragma once

#include "command.h"
#include "editor_context.h"
#include "editor_selection.h"

#include "log/log.h"
#include "scene/entity.h"
#include "scene/world.h"

#include <optional>
#include <string>
#include <string_view>

namespace buddd::editor {

/// Command that creates a new empty entity.
/// If an explicit `parent_id` is provided, the entity is created as a child of that entity.
/// Otherwise, falls back to the selection anchor. If neither is available, creates a root entity.
class CreateEntityCommand final : public Command {
public:
    explicit CreateEntityCommand(std::optional<buddd::engine::EntityId> explicit_parent = std::nullopt)
        : explicit_parent_(explicit_parent) {}

    auto execute(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();
        auto& selection = ctx.editor.selection();

        // Save pre-execution selection for undo
        pre_execution_selection_ = selection.snapshot();

        // Determine parent: explicit | stored (for redo stability) | anchor | root
        buddd::engine::EntityId parent_id;
        if (explicit_parent_.has_value()) {
            parent_id = *explicit_parent_;
        } else if (stored_parent_id_ != buddd::engine::EntityId::none()) {
            // Use previously stored parent for redo stability
            parent_id = stored_parent_id_;
        } else {
            parent_id = selection.anchor().value_or(buddd::engine::EntityId::none());
        }

        // Store parent for redo (so redo doesn't depend on changed selection state)
        stored_parent_id_ = parent_id;

        // Create entity
        buddd::engine::Entity new_entity;
        if (parent_id != buddd::engine::EntityId::none()) {
            // Find the parent entity via tree traversal
            auto find_entity = [&](auto& self, buddd::engine::Entity e) -> std::optional<buddd::engine::Entity> {
                if (e.id() == parent_id) return e;
                for (size_t i = 0; i < e.child_count(); ++i) {
                    auto found = self(self, e.get_child(i));
                    if (found.has_value()) return found;
                }
                return std::nullopt;
            };
            for (size_t i = 0; i < world.root_entity_count(); ++i) {
                auto root = world.get_root_entity(i);
                if (root.id() == parent_id) {
                    new_entity = root.create_child();
                    break;
                }
                auto found = find_entity(find_entity, root);
                if (found.has_value()) {
                    new_entity = found->create_child();
                    break;
                }
            }
        } else {
            new_entity = world.add_entity();
        }

        // Fallback: if parent entity not found (e.g. externally destroyed between undo and redo),
        // create as root instead of silently becoming a no-op.
        if (new_entity.id() == buddd::engine::EntityId::none()) {
            new_entity = world.add_entity();
            BUDDD_LOG_TAGGED_DEBUG("Editor", "CreateEntity: parent entity {} not found, created as root",
                parent_id.index);
        }

        // Store created entity ID for undo
        created_entity_id_ = new_entity.id();

        // Apply post-creation name if set (for auto-rename-on-create)
        if (post_creation_name_.has_value() && new_entity.id() != buddd::engine::EntityId::none()) {
            new_entity.set_name(*post_creation_name_);
        }

        BUDDD_LOG_TAGGED_DEBUG("Editor", "CreateEntity: entity {} created under {}",
            created_entity_id_.index,
            parent_id != buddd::engine::EntityId::none()
                ? std::to_string(parent_id.index)
                : std::string("root"));
    }

    auto undo(EditorContext const& ctx) -> void override {
        // Destroy the created entity
        if (created_entity_id_ != buddd::engine::EntityId::none()) {
            // Find and destroy entity by traversing the world
            auto& world = ctx.editor.world();
            auto find_entity = [&](auto& self, buddd::engine::Entity e) -> bool {
                if (e.id() == created_entity_id_) {
                    e.destroy();
                    return true;
                }
                for (size_t i = 0; i < e.child_count(); ++i) {
                    if (self(self, e.get_child(i))) return true;
                }
                return false;
            };
            bool found = false;
            for (size_t i = 0; i < world.root_entity_count(); ++i) {
                auto root = world.get_root_entity(i);
                if (root.id() == created_entity_id_) {
                    root.destroy();
                    found = true;
                    break;
                }
                if (find_entity(find_entity, root)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                BUDDD_LOG_TAGGED_DEBUG("Editor", "CreateEntity::undo: entity {} already destroyed",
                    created_entity_id_.index);
            }
            created_entity_id_ = buddd::engine::EntityId::none();
        }

        // Restore selection
        ctx.editor.selection().restore(pre_execution_selection_);
    }

    [[nodiscard]] auto name() const -> std::string_view override {
        return "Create Entity";
    }

    [[nodiscard]] auto created_entity_id() const -> buddd::engine::EntityId {
        return created_entity_id_;
    }

    auto set_post_creation_name(std::string name) -> void {
        post_creation_name_ = std::move(name);
    }

private:
    std::optional<buddd::engine::EntityId> explicit_parent_;
    buddd::engine::EntityId stored_parent_id_ = buddd::engine::EntityId::none();
    buddd::engine::EntityId created_entity_id_ = buddd::engine::EntityId::none();
    std::optional<std::string> post_creation_name_;
    Selection pre_execution_selection_;
};

} // namespace buddd::editor
