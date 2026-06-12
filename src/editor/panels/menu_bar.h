#pragma once

#include "editor_menu.h"
#include "command_stack.h"

#include <functional>
#include <imgui.h>
#include <string_view>

namespace buddd::editor {

/// Main menu bar rendered via ImGui::BeginMainMenuBar().
/// Displays File > New/Open/Save/Save As/Quit, Edit > Undo/Redo, Help > About menus.
/// Actions are dispatched via callbacks set with set_on_* methods.
class MenuBar final : public EditorMenu {
public:
    explicit MenuBar(CommandStack& command_stack)
        : command_stack_(command_stack)
    {}

    /// Set the callback invoked when Help > About is clicked.
    auto set_on_about(std::function<void()> callback) -> void {
        on_about_ = std::move(callback);
    }

    /// Set the callback invoked when File > New Scene is clicked.
    auto set_on_new_scene(std::function<void()> callback) -> void {
        on_new_scene_ = std::move(callback);
    }

    /// Set the callback invoked when File > Open Scene is clicked.
    auto set_on_open_scene(std::function<void()> callback) -> void {
        on_open_scene_ = std::move(callback);
    }

    /// Set the callback invoked when File > Save Scene is clicked.
    auto set_on_save_scene(std::function<void()> callback) -> void {
        on_save_scene_ = std::move(callback);
    }

    /// Set the callback invoked when File > Save Scene As is clicked.
    auto set_on_save_scene_as(std::function<void()> callback) -> void {
        on_save_scene_as_ = std::move(callback);
    }

    /// Set the callback invoked when File > Quit is clicked.
    /// Receives the current EngineContext for calling ctx.request_exit().
    auto set_on_quit(std::function<void(buddd::engine::EngineContext const&)> callback) -> void {
        on_quit_ = std::move(callback);
    }

    [[nodiscard]] auto id() const -> std::string_view override {
        return "menu_bar";
    }

    auto draw_ui(buddd::engine::EngineContext const& ctx) -> void override {
        if (ImGui::BeginMainMenuBar()) {
            // ── File menu ──
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                    if (on_new_scene_) on_new_scene_();
                }
                if (ImGui::MenuItem("Open Scene", "Ctrl+O")) {
                    if (on_open_scene_) on_open_scene_();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                    if (on_save_scene_) on_save_scene_();
                }
                if (ImGui::MenuItem("Save Scene As", "Ctrl+Shift+S")) {
                    if (on_save_scene_as_) on_save_scene_as_();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
                    if (on_quit_) on_quit_(ctx);
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
    std::function<void()> on_new_scene_;
    std::function<void()> on_open_scene_;
    std::function<void()> on_save_scene_;
    std::function<void()> on_save_scene_as_;
    std::function<void(buddd::engine::EngineContext const&)> on_quit_;
};

} // namespace buddd::editor
