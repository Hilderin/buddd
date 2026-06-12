#include "panels/scene_panel.h"

#include "editor.h"
#include "editor_selection.h"
#include "command_stack.h"
#include "commands/create_entity_command.h"
#include "commands/delete_entity_command.h"
#include "commands/rename_entity_command.h"

#include "log/log.h"
#include "scene/entity.h"
#include "scene/world.h"

#include <imgui.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace buddd::editor {

// ═══════════════════════════════════════════════════════════════════════════
// Public interface
// ═══════════════════════════════════════════════════════════════════════════

auto ScenePanel::id() const -> std::string_view { return "scene"; }
auto ScenePanel::title() const -> std::string_view { return "Scene"; }

// ═══════════════════════════════════════════════════════════════════════════
// draw_ui — main panel rendering
// ═══════════════════════════════════════════════════════════════════════════

auto ScenePanel::draw_ui(EditorContext const& ctx) -> void {
    auto& world = ctx.editor.world();
    bool open_context_menu = false;

    // ── Empty state ──
    if (world.entity_count() == 0) {
        ImGui::Text("No entities");
    }

    // ── Recursive helper to render entity subtree ──
    if (world.entity_count() > 0) {
        auto render_entity = [&](auto& self, buddd::engine::Entity entity) -> void {
            ImGui::PushID(static_cast<int>(entity.id().index));

            auto flags = ImGuiTreeNodeFlags_SpanAvailWidth
                       | ImGuiTreeNodeFlags_DefaultOpen;

            if (entity.child_count() == 0) {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }

            // Selection highlighting
            if (ctx.editor.selection().contains(entity.id())) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            auto name = entity.name();
            if (name.empty()) {
                name = "(unnamed)";
            }

            // ── Inline rename ──
            bool is_renaming = (renaming_entity_.has_value() && *renaming_entity_ == entity.id());

            if (is_renaming) {
                // Use InputText instead of TreeNodeEx label
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::SetKeyboardFocusHere();
                ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_EnterReturnsTrue
                                                | ImGuiInputTextFlags_AutoSelectAll;
                bool confirmed = ImGui::InputText("##rename", rename_buffer_, sizeof(rename_buffer_), input_flags);

                // Detect Enter/Escape/focus-loss
                if (confirmed) {
                    confirm_rename(ctx);
                } else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    cancel_rename();
                } else if (ImGui::IsItemDeactivatedAfterEdit()) {
                    // Focus loss while editing: confirm (same as Enter)
                    confirm_rename(ctx);
                }

                // Render children during rename (no tree node — avoids blue button artifact)
                if (entity.child_count() > 0) {
                    ImGui::Indent();
                    for (size_t i = 0; i < entity.child_count(); ++i) {
                        self(self, entity.get_child(i));
                    }
                    ImGui::Unindent();
                }
            } else {
                bool expanded = ImGui::TreeNodeEx(name.c_str(), flags);

                // ── Detect right-click on this entity for shared context menu ──
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
                    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                        BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel", "Right-click on entity {}", name);
                        context_menu_entity_ = entity.id();
                        open_context_menu = true;
                    }
                }

                // ── Left-click handling on entity row ──
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    // If rename is active on a different entity, cancel it
                    if (renaming_entity_.has_value() && *renaming_entity_ != entity.id()) {
                        cancel_rename();
                    }
                    if (ImGui::GetIO().KeyShift) {
                        // Shift+click: range selection
                        auto anchor = ctx.editor.selection().anchor();
                        if (anchor.has_value()) {
                            auto range = collect_range(ctx.editor.world(), *anchor, entity.id());
                            ctx.editor.selection().set_selection(range);
                            // anchor unchanged
                        } else {
                            // No anchor: degrade to Replace
                            ctx.editor.selection().select(entity.id(), SelectionModifier::Replace);
                        }
                    } else if (ImGui::GetIO().KeyCtrl) {
                        // Ctrl+click: toggle
                        ctx.editor.selection().select(entity.id(), SelectionModifier::Toggle);
                        // anchor unchanged
                    } else {
                        // Plain click: replace selection
                        ctx.editor.selection().select(entity.id(), SelectionModifier::Replace);
                        // Replace sets anchor internally
                    }
                }

                if (expanded) {
                    for (size_t i = 0; i < entity.child_count(); ++i) {
                        self(self, entity.get_child(i));
                    }
                    ImGui::TreePop();
                }
            }

            ImGui::PopID();
        };

        // Iterate root entities
        for (size_t i = 0; i < world.root_entity_count(); ++i) {
            auto entity = world.get_root_entity(i);
            if (entity.id() != buddd::engine::EntityId::none()) {
                render_entity(render_entity, entity);
            }
        }
    }

    // ── Empty-area right-click opens shared context menu ──
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        context_menu_entity_ = buddd::engine::EntityId::none();
        BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel", "Empty-area right-click");
        open_context_menu = true;
    }

    // Defer OpenPopup until after the tree rendering context is fully closed
    if (open_context_menu) {
        ImGui::OpenPopup("scene_ctx");
    }

    // ── Shared context menu (for both entity and empty-area right-click) ──
    if (ImGui::BeginPopup("scene_ctx")) {
        bool on_entity = (context_menu_entity_ != buddd::engine::EntityId::none());

        if (ImGui::MenuItem("Create Empty")) {
            if (on_entity) {
                execute_create_entity(ctx, context_menu_entity_);
            } else {
                execute_create_entity(ctx);
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Delete", nullptr, false, !ctx.editor.selection().empty())) {
            execute_delete_entity(ctx);
        }

        if (ImGui::MenuItem("Rename", nullptr, false, on_entity)) {
            start_rename(ctx, context_menu_entity_);
        }

        ImGui::EndPopup();
    }

    // ── Empty-area left-click: clear selection and cancel rename ──
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
        if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift) {
            cancel_rename();
            ctx.editor.selection().clear();
        }
    }

    // ── Delete key (gated by focus, disabled during rename) ──
    if (ImGui::IsWindowFocused() && !renaming_entity_.has_value()
        && ImGui::IsKeyPressed(ImGuiKey_Delete) && !ctx.editor.selection().empty()) {
        execute_delete_entity(ctx);
    }

    // ── F2 key (gated by focus, exactly one selected) ──
    if (ImGui::IsWindowFocused() && ctx.editor.selection().size() == 1) {
        if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
            if (renaming_entity_.has_value()) {
                confirm_rename(ctx);
            }
            start_rename(ctx, ctx.editor.selection().first().value());
        }
    }

    // ── Delete confirmation modal ──
    draw_delete_confirmation_modal(ctx);
}

// ═══════════════════════════════════════════════════════════════════════════
// Command helpers
// ═══════════════════════════════════════════════════════════════════════════

auto ScenePanel::execute_create_entity(EditorContext const& ctx,
                                       std::optional<buddd::engine::EntityId> parent) -> void {
    auto cmd = std::make_unique<CreateEntityCommand>(parent);
    ctx.editor.command_stack().execute(std::move(cmd), ctx);
}

auto ScenePanel::execute_delete_entity(EditorContext const& ctx) -> void {
    auto& selection = ctx.editor.selection();
    auto& world = ctx.editor.world();

    // Collect selected entities and check if any have children
    std::vector<buddd::engine::EntityId> ids;
    size_t with_children = 0;

    for (auto id : selection.current()) {
        ids.push_back(id);
        auto find_entity = [&](auto& self, buddd::engine::Entity e) -> bool {
            if (e.id() == id) {
                if (e.child_count() > 0) ++with_children;
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
                if (root.child_count() > 0) ++with_children;
                break;
            }
            if (find_entity(find_entity, root)) break;
        }
    }

    if (with_children > 0) {
        pending_deletion_ids_ = std::move(ids);
        pending_deletion_with_children_ = with_children;
        if (pending_deletion_ids_.size() == 1) {
            auto& w = ctx.editor.world();
            auto find_name = [&](auto& self, buddd::engine::Entity e) -> bool {
                if (e.id() == pending_deletion_ids_[0]) {
                    pending_deletion_first_name_ = e.name();
                    return true;
                }
                for (size_t i = 0; i < e.child_count(); ++i) {
                    if (self(self, e.get_child(i))) return true;
                }
                return false;
            };
            for (size_t i = 0; i < w.root_entity_count(); ++i) {
                auto root = w.get_root_entity(i);
                if (root.id() == pending_deletion_ids_[0]) {
                    pending_deletion_first_name_ = root.name();
                    break;
                }
                if (find_name(find_name, root)) break;
            }
        }
        show_delete_confirmation_ = true;
    } else {
        auto cmd = std::make_unique<DeleteEntityCommand>(std::move(ids));
        ctx.editor.command_stack().execute(std::move(cmd), ctx);
    }
}

auto ScenePanel::start_rename(EditorContext const& ctx, buddd::engine::EntityId id) -> void {
    renaming_entity_ = id;
    std::string current_name;
    auto& world = ctx.editor.world();
    auto find_entity = [&](auto& self, buddd::engine::Entity e) -> bool {
        if (e.id() == id) {
            current_name = e.name();
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
            current_name = root.name();
            break;
        }
        if (find_entity(find_entity, root)) break;
    }
    std::strncpy(rename_buffer_, current_name.c_str(), sizeof(rename_buffer_) - 1);
    rename_buffer_[sizeof(rename_buffer_) - 1] = '\0';
}

auto ScenePanel::confirm_rename(EditorContext const& ctx) -> void {
    if (!renaming_entity_.has_value()) return;
    auto id = *renaming_entity_;
    renaming_entity_.reset();

    std::string current_name;
    auto& world = ctx.editor.world();
    auto find_entity = [&](auto& self, buddd::engine::Entity e) -> bool {
        if (e.id() == id) {
            current_name = e.name();
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
            current_name = root.name();
            break;
        }
        if (find_entity(find_entity, root)) break;
    }

    std::string new_name(rename_buffer_);
    if (new_name.empty() || new_name == current_name) {
        rename_buffer_[0] = '\0';
        return;
    }

    auto cmd = std::make_unique<RenameEntityCommand>(id, std::move(current_name), std::move(new_name));
    ctx.editor.command_stack().execute(std::move(cmd), ctx);
    rename_buffer_[0] = '\0';
}

auto ScenePanel::cancel_rename() -> void {
    renaming_entity_.reset();
    rename_buffer_[0] = '\0';
}

auto ScenePanel::draw_delete_confirmation_modal(EditorContext const& ctx) -> void {
    if (!show_delete_confirmation_) return;

    ImGui::OpenPopup("Confirm Delete");
    if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (pending_deletion_ids_.size() == 1) {
            ImGui::Text("Delete %s and its %zu children?",
                pending_deletion_first_name_.c_str(),
                pending_deletion_with_children_);
        } else {
            ImGui::Text("Delete %zu entities? (%zu have children that will also be deleted.)",
                pending_deletion_ids_.size(),
                pending_deletion_with_children_);
        }

        bool deleted = false;
        if (ImGui::Button("Delete")) {
            auto cmd = std::make_unique<DeleteEntityCommand>(std::move(pending_deletion_ids_));
            ctx.editor.command_stack().execute(std::move(cmd), ctx);
            deleted = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel") || deleted) {
            show_delete_confirmation_ = false;
            pending_deletion_ids_.clear();
            pending_deletion_with_children_ = 0;
            if (deleted) {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            }
        }
        ImGui::EndPopup();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Tree traversal helpers
// ═══════════════════════════════════════════════════════════════════════════

auto ScenePanel::collect_range(buddd::engine::World& world,
                               buddd::engine::EntityId anchor,
                               buddd::engine::EntityId clicked) const
    -> std::vector<buddd::engine::EntityId>
{
    auto all = collect_all(world);

    auto anchor_it = std::find(all.begin(), all.end(), anchor);
    auto clicked_it = std::find(all.begin(), all.end(), clicked);

    if (anchor_it == all.end() || clicked_it == all.end()) {
        return {clicked};
    }

    auto a_idx = static_cast<size_t>(std::distance(all.begin(), anchor_it));
    auto b_idx = static_cast<size_t>(std::distance(all.begin(), clicked_it));
    auto first = (std::min)(a_idx, b_idx);
    auto last  = (std::max)(a_idx, b_idx);

    return std::vector<buddd::engine::EntityId>(
        all.begin() + static_cast<ptrdiff_t>(first),
        all.begin() + static_cast<ptrdiff_t>(last) + 1);
}

auto ScenePanel::collect_all(buddd::engine::World& world) const
    -> std::vector<buddd::engine::EntityId>
{
    std::vector<buddd::engine::EntityId> ids;
    ids.reserve(world.entity_count());

    auto collect = [&](auto& self, buddd::engine::Entity entity) -> void {
        ids.push_back(entity.id());
        for (size_t i = 0; i < entity.child_count(); ++i) {
            self(self, entity.get_child(i));
        }
    };

    for (size_t i = 0; i < world.root_entity_count(); ++i) {
        auto entity = world.get_root_entity(i);
        if (entity.id() != buddd::engine::EntityId::none()) {
            collect(collect, entity);
        }
    }

    return ids;
}

} // namespace buddd::editor
