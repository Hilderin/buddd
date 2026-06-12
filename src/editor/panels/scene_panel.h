#pragma once

#include "editor_panel.h"
#include "editor_context.h"

#include "scene/world.h"

#include <imgui.h>
#include <string_view>

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

            auto name = entity.name();
            if (name.empty()) {
                name = "(unnamed)";
            }

            bool expanded = ImGui::TreeNodeEx(name.c_str(), flags);
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
    }
};

} // namespace buddd::editor
