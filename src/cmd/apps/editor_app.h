#pragma once

#include "app.h"

#include <memory>

namespace buddd::editor { class Editor; }

namespace buddd::cmd::app {

/// App subclass that opens the editor window.
class EditorApp final : public buddd::cmd::App {
public:
    EditorApp();
    ~EditorApp() override;

    auto config() const -> buddd::cmd::AppConfig override;

    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void> override;

    auto on_render(buddd::engine::EngineContext const& ctx) -> void override;

    auto shutdown() -> void override;

private:
    std::unique_ptr<buddd::editor::Editor> editor_;
};

} // namespace buddd::cmd::app
