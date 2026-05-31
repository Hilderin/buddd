#pragma once

#include "error.h"

#include <memory>

namespace buddd::engine {

class Platform;
class Window;
class RenderDevice;
class InputSystem;
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

    EngineService(const EngineService&) = delete;
    auto operator=(const EngineService&) -> EngineService& = delete;
    EngineService(EngineService&&) = default;
    auto operator=(EngineService&&) -> EngineService& = default;

private:
    EngineService(std::unique_ptr<Platform> platform,
                  std::unique_ptr<Window> window,
                  std::unique_ptr<RenderDevice> device);

    // Member declaration order MUST be: platform_, window_, device_
    // This ensures Platform outlives Window outlives RenderDevice on destruction.
    std::unique_ptr<Platform> platform_;
    std::unique_ptr<Window> window_;
    std::unique_ptr<RenderDevice> device_;
};

} // namespace buddd::engine
