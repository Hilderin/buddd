#pragma once

#include "error.h"

#include <memory>

namespace buddd::engine {

class ComponentRegistry;
class Platform;
class Window;
class RenderDevice;
class InputSystem;
class AssetManager;
struct WindowConfig;
enum class Backend;

class EngineService {
public:
    [[nodiscard]] static auto create(Backend backend, const WindowConfig& config)
        -> Result<std::unique_ptr<EngineService>>;

    ~EngineService();

    auto platform() noexcept -> Platform&;
    auto window() noexcept -> Window&;
    auto device() noexcept -> RenderDevice&;
    auto assets() noexcept -> AssetManager&;
    auto registry() noexcept -> ComponentRegistry&;

    EngineService(const EngineService&) = delete;
    auto operator=(const EngineService&) -> EngineService& = delete;
    EngineService(EngineService&&) = default;
    auto operator=(EngineService&&) -> EngineService& = default;

private:
    EngineService(std::unique_ptr<Platform> platform,
                  std::unique_ptr<Window> window,
                  std::unique_ptr<RenderDevice> device);

    // Member declaration order MUST be: platform_, window_, device_, asset_manager_
    // This ensures Platform outlives Window outlives RenderDevice on destruction.
    // AssetManager must be destroyed before RenderDevice (holds a RenderDevice&).
    std::unique_ptr<Platform> platform_;
    std::unique_ptr<Window> window_;
    std::unique_ptr<RenderDevice> device_;
    std::unique_ptr<AssetManager> asset_manager_;
    std::unique_ptr<ComponentRegistry> registry_;
};

} // namespace buddd::engine
