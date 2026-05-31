#include "engine_service.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"

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

    return std::unique_ptr<EngineService>(
        new EngineService(std::move(*platform), std::move(*window), std::move(*device)));
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

} // namespace buddd::engine
