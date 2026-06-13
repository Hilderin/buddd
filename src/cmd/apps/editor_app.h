#pragma once

#include "app.h"

#include <memory>
#include <optional>

namespace buddd::editor { class Editor; }

namespace buddd::cmd::app {

/// App subclass that opens the editor window.
class EditorApp final : public buddd::cmd::App {
public:
    explicit EditorApp(std::optional<std::string> scene_path = std::nullopt);
    ~EditorApp() override;

    auto config() const -> buddd::cmd::AppConfig override;

    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void> override;

    auto on_render(buddd::engine::EngineContext const& ctx) -> void override;

    auto update(buddd::engine::EngineContext const& ctx) -> void override;

    auto shutdown() -> void override;

private:
    std::unique_ptr<buddd::editor::Editor> editor_;
    std::optional<std::string> scene_path_;
};

} // namespace buddd::cmd::app
