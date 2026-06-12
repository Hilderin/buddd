#pragma once

#include "command.h"
#include "editor_context.h"
#include "editor_selection.h"

#include "log/log.h"
#include "scene/entity.h"

#include <string>
#include <string_view>

namespace buddd::editor {

/// Command that renames a single entity.
class RenameEntityCommand final : public Command {
public:
    RenameEntityCommand(buddd::engine::EntityId entity_id,
                        std::string old_name,
                        std::string new_name)
        : entity_id_(entity_id)
        , old_name_(std::move(old_name))
        , new_name_(std::move(new_name))
    {}

    auto execute(EditorContext const& ctx) -> void override {
        // Save pre-execution selection for undo
        pre_execution_selection_ = ctx.editor.selection().snapshot();

        auto& world = ctx.editor.world();
        // Find entity and set name
        auto find_and_rename = [&](auto& self, buddd::engine::Entity e) -> bool {
            if (e.id() == entity_id_) {
                e.set_name(new_name_);
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
            if (root.id() == entity_id_) {
                root.set_name(new_name_);
                found = true;
                break;
            }
            if (find_and_rename(find_and_rename, root)) {
                found = true;
                break;
            }
        }
        if (!found) {
            BUDDD_LOG_TAGGED_DEBUG("Editor", "RenameEntity::execute: entity {} not found", entity_id_.index);
        }

        BUDDD_LOG_TAGGED_DEBUG("Editor", "RenameEntity: {} -> {}", old_name_, new_name_);
    }

    auto undo(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();
        // Revert to old name
        auto find_and_rename = [&](auto& self, buddd::engine::Entity e) -> bool {
            if (e.id() == entity_id_) {
                e.set_name(old_name_);
                return true;
            }
            for (size_t i = 0; i < e.child_count(); ++i) {
                if (self(self, e.get_child(i))) return true;
            }
            return false;
        };
        for (size_t i = 0; i < world.root_entity_count(); ++i) {
            auto root = world.get_root_entity(i);
            if (root.id() == entity_id_) {
                root.set_name(old_name_);
                break;
            }
            if (find_and_rename(find_and_rename, root)) break;
        }

        // Restore selection
        ctx.editor.selection().restore(pre_execution_selection_);
    }

    [[nodiscard]] auto name() const -> std::string_view override {
        return "Rename Entity";
    }

private:
    buddd::engine::EntityId entity_id_;
    std::string old_name_;
    std::string new_name_;
    Selection pre_execution_selection_;
};

} // namespace buddd::editor
