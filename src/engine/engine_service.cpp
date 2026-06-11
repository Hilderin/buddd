#include "engine_service.h"
#include "debug/assert.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"
#include "asset/asset_manager.h"
#include "scene/component_registry/register_all_components.h"
#include "scene/component_registry/component_registry.h"

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
        return make_error(platform);
    }

    auto window = (*platform)->create_window(config);
    if (!window) {
        return make_error(window);
    }

    auto device = RenderDevice::create(*window.value());
    if (!device) {
        return make_error(device);
    }

    auto engine = std::unique_ptr<EngineService>(
        new EngineService(std::move(*platform), std::move(*window), std::move(*device)));

    // Create AssetManager after engine is constructed (asset_manager_ must be
    // declared after device_ for correct destruction order).
    auto asset_mgr = AssetManager::create(engine->device(), "assets");
    if (!asset_mgr) {
        return make_error(asset_mgr);
    }
    engine->asset_manager_ = std::move(*asset_mgr);

    // Register built-in types in TypeRegistry
    register_builtin_types();

    // Register all engine components
    engine->registry_ = std::make_unique<ComponentRegistry>();
    register_all_components(*engine->registry_);

    return engine;
}

auto EngineService::platform() noexcept -> Platform& {
    BUDDD_ASSERT(platform_ != nullptr);
    return *platform_;
}

auto EngineService::window() noexcept -> Window& {
    BUDDD_ASSERT(window_ != nullptr);
    return *window_;
}

auto EngineService::device() noexcept -> RenderDevice& {
    BUDDD_ASSERT(device_ != nullptr);
    return *device_;
}

auto EngineService::assets() noexcept -> AssetManager& {
    BUDDD_ASSERT(asset_manager_ != nullptr);
    return *asset_manager_;
}

auto EngineService::registry() noexcept -> ComponentRegistry& {
    BUDDD_ASSERT(registry_ != nullptr);
    return *registry_;
}

} // namespace buddd::engine
