#pragma once

#include "app.h"

#include <string>

namespace buddd::cmd::app {

class SceneApp final : public App {
public:
    explicit SceneApp(std::string scene_path);

    auto config() const -> AppConfig override;

    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void> override;

private:
    std::string scene_path_;
};

} // namespace buddd::cmd::app
