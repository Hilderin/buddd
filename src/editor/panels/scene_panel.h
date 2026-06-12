#pragma once

#include "editor_panel.h"
#include "editor_context.h"

#include "scene/world.h"

#include <imgui.h>
#include <string_view>

#include <algorithm>

namespace buddd::editor {

class ScenePanel final : public EditorPanel {
public:
    [[nodiscard]] auto id() const -> std::string_view override { return "scene"; }
    [[nodiscard]] auto title() const -> std::string_view override { return "Scene"; }

    auto draw_ui(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();

        // Empty state
        if (world.entity_count() == 0) {
            ImGui::Text("No entities");
            return;
        }

        // Recursive helper to render entity subtree
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

            bool expanded = ImGui::TreeNodeEx(name.c_str(), flags);

            // Click handling on entity row
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
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

            ImGui::PopID();
        };

        // Iterate root entities
        for (size_t i = 0; i < world.root_entity_count(); ++i) {
            auto entity = world.get_root_entity(i);
            if (entity.id() != buddd::engine::EntityId::none()) {
                render_entity(render_entity, entity);
            }
        }

        // Empty-area click: clear selection (only without modifiers)
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
            if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift) {
                ctx.editor.selection().clear();
            }
        }
    }

private:
    /// Collect EntityIds in depth-first tree order between anchor and clicked (inclusive).
    /// Both anchor and clicked must be valid (alive) EntityIds.
    auto collect_range(buddd::engine::World& world,
                        buddd::engine::EntityId anchor,
                        buddd::engine::EntityId clicked) const
        -> std::vector<buddd::engine::EntityId>
    {
        // Collect all entities in depth-first order
        auto all = collect_all(world);

        // Find indices of anchor and clicked
        auto anchor_it = std::find(all.begin(), all.end(), anchor);
        auto clicked_it = std::find(all.begin(), all.end(), clicked);

        // Both must be found (guaranteed by caller)
        if (anchor_it == all.end() || clicked_it == all.end()) {
            return {clicked};  // fallback: just the clicked entity
        }

        // Return the subspan between anchor and clicked (inclusive)
        // NOTE: manual min/max to avoid std::minmax(const T&, const T&) dangling reference UB
        // when passed temporary size_t values.
        auto a_idx = static_cast<size_t>(std::distance(all.begin(), anchor_it));
        auto b_idx = static_cast<size_t>(std::distance(all.begin(), clicked_it));
        auto first = (std::min)(a_idx, b_idx);
        auto last  = (std::max)(a_idx, b_idx);

        return std::vector<buddd::engine::EntityId>(
            all.begin() + static_cast<ptrdiff_t>(first),
            all.begin() + static_cast<ptrdiff_t>(last) + 1);
    }

    /// Collect all EntityIds in depth-first tree order.
    auto collect_all(buddd::engine::World& world) const
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
};

} // namespace buddd::editor
