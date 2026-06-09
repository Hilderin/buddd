#pragma once

#include "editor_menu.h"
#include "command_stack.h"

#include <functional>
#include <imgui.h>
#include <string_view>

namespace buddd::editor {

/// Main menu bar rendered via ImGui::BeginMainMenuBar().
/// Displays File > Quit, Edit > Undo/Redo, Help > About menus.
/// About action is dispatched via a callback (set_on_about), not a command.
class MenuBar final : public EditorMenu {
public:
    explicit MenuBar(CommandStack& command_stack)
        : command_stack_(command_stack)
    {}

    /// Set the callback invoked when Help > About is clicked.
    auto set_on_about(std::function<void()> callback) -> void {
        on_about_ = std::move(callback);
    }

    [[nodiscard]] auto id() const -> std::string_view override {
        return "menu_bar";
    }

    auto draw_ui(buddd::engine::EngineContext const& ctx) -> void override {
        if (ImGui::BeginMainMenuBar()) {
            // ── File menu ──
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
                    ctx.request_exit();
                }
                ImGui::EndMenu();
            }

            // ── Edit menu ──
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, command_stack_.can_undo())) {
                    [[maybe_unused]] auto _ = command_stack_.undo();
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z", false, command_stack_.can_redo())) {
                    [[maybe_unused]] auto _ = command_stack_.redo();
                }
                ImGui::EndMenu();
            }

            // ── Help menu ──
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About")) {
                    if (on_about_) {
                        on_about_();
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

private:
    CommandStack& command_stack_;
    std::function<void()> on_about_;
};

} // namespace buddd::editor
