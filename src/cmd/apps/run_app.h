#pragma once

#include "app.h"

namespace buddd::cmd::app {

/// Empty window: clears framebuffer each frame, no draw calls.
/// Runs interactively until window close.
class RunApp : public App {
public:
    auto config() const -> AppConfig override {
        return {};  // defaults: "Buddd Engine", 1024x768
    }

    [[nodiscard]] auto setup(buddd::engine::EngineContext const&) -> buddd::engine::Result<void> override {
        return {};
    }
};

} // namespace buddd::cmd::app
