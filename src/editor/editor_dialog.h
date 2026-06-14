#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <imgui.h>

namespace buddd::editor {

struct DialogButton {
    std::string label;
    std::string label_id;        // ImGui ID suffix for the button (unique within the dialog)
    std::function<bool()> callback;
    std::optional<ImGuiKey> shortcut = std::nullopt;  // stored, not auto-bound
};

class Dialog {
public:
    virtual ~Dialog() = default;
    virtual auto id() const -> std::string = 0;
    virtual auto title() const -> std::string = 0;
    virtual auto draw_content() -> void = 0;

    auto request_close() -> void { should_close_ = true; }
    [[nodiscard]] auto should_close() const -> bool { return should_close_; }

    virtual auto handle_escape() -> void { request_close(); }

protected:
    Dialog() = default;

private:
    bool should_close_ = false;
};

class CustomDialog final : public Dialog {
public:
    CustomDialog(
        std::string id,
        std::string title,
        std::function<void()> content_fn,
        std::vector<DialogButton> buttons,
        std::function<void()> on_close = nullptr
    );

    auto id() const -> std::string override { return id_; }
    auto title() const -> std::string override { return title_; }
    auto draw_content() -> void override;
    auto handle_escape() -> void override;

private:
    std::string id_;
    std::string title_;
    std::function<void()> content_fn_;
    std::vector<DialogButton> buttons_;
    std::function<void()> on_close_;
};

// ── Inline implementations ──

inline CustomDialog::CustomDialog(
    std::string id,
    std::string title,
    std::function<void()> content_fn,
    std::vector<DialogButton> buttons,
    std::function<void()> on_close)
    : id_(std::move(id))
    , title_(std::move(title))
    , content_fn_(std::move(content_fn))
    , buttons_(std::move(buttons))
    , on_close_(std::move(on_close))
{}

inline auto CustomDialog::draw_content() -> void {
    if (content_fn_) {
        content_fn_();
    }

    for (size_t i = 0; i < buttons_.size(); ++i) {
        if (i > 0) ImGui::SameLine();
        ImGui::PushID(buttons_[i].label_id.c_str());
        if (ImGui::Button(buttons_[i].label.c_str())) {
            if (buttons_[i].callback && buttons_[i].callback()) {
                request_close();
            }
        }
        ImGui::PopID();
    }
}

inline auto CustomDialog::handle_escape() -> void {
    if (on_close_) on_close_();
    request_close();
}

} // namespace buddd::editor
