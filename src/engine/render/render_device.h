#pragma once

#include "error.h"

#include <memory>
#include <utility>

namespace buddd::engine {

class Window;

class RenderDevice {
public:
    static auto create(Window& window) -> Result<std::unique_ptr<RenderDevice>>;

    virtual ~RenderDevice() = default;

    virtual auto begin_frame() -> void = 0;
    virtual auto end_frame() -> void = 0;
    virtual auto size() const noexcept -> std::pair<int, int> = 0;

    RenderDevice(const RenderDevice&) = delete;
    auto operator=(const RenderDevice&) -> RenderDevice& = delete;
    RenderDevice(RenderDevice&&) = delete;
    auto operator=(RenderDevice&&) -> RenderDevice& = delete;

protected:
    RenderDevice() = default;
};

} // namespace buddd::engine
