#include "engine_service.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"
#include "asset/asset_manager.h"

namespace buddd::engine {

EngineService::EngineService(std::unique_ptr<Platform> platform,
                             std::unique_ptr<Window> window,
                             std::unique_ptr<RenderDevice> device)
    : platform_(std::move(platform))
    , window_(std::move(window))
    , device_(std::move(device)) {}

EngineService::~EngineService() = default;

auto EngineService::create(Backend backend, const WindowConfig& config)
    -> Result<std::unique_ptr<EngineService>>
{
    auto platform = Platform::create(backend);
    if (!platform) {
        return std::unexpected(platform.error());
    }

    auto window = (*platform)->create_window(config);
    if (!window) {
        return std::unexpected(window.error());
    }

    auto device = RenderDevice::create(*window.value());
    if (!device) {
        return std::unexpected(device.error());
    }

    auto engine = std::unique_ptr<EngineService>(
        new EngineService(std::move(*platform), std::move(*window), std::move(*device)));

    // Create AssetManager after engine is constructed (asset_manager_ must be
    // declared after device_ for correct destruction order).
    auto asset_mgr = AssetManager::create(engine->device(), "assets");
    if (!asset_mgr) {
        return std::unexpected(asset_mgr.error());
    }
    engine->asset_manager_ = std::move(*asset_mgr);

    return engine;
}

auto EngineService::platform() noexcept -> Platform& {
    return *platform_;
}

auto EngineService::window() noexcept -> Window& {
    return *window_;
}

auto EngineService::device() noexcept -> RenderDevice& {
    return *device_;
}

auto EngineService::assets() noexcept -> AssetManager& {
    return *asset_manager_;
}

} // namespace buddd::engine
