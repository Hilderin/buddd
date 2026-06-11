#include "apps/scene_app.h"

#include "engine_context.h"
#include "engine_service.h"
#include "error.h"
#include "scene/scene_loader.h"
#include "scene/world.h"

#include <filesystem>

namespace buddd::cmd::app {

SceneApp::SceneApp(std::string scene_path)
    : scene_path_(std::move(scene_path)) {}

auto SceneApp::config() const -> AppConfig {
    auto stem = std::filesystem::path(scene_path_).stem().string();
    return {stem, 1024, 768};
}

auto SceneApp::setup(buddd::engine::EngineContext const& ctx)
    -> buddd::engine::Result<void>
{
    buddd::engine::SceneLoader loader(
        ctx.world, ctx.services.registry(), ctx.services.assets());
    return loader.load_from_file(scene_path_);
}

} // namespace buddd::cmd::app
