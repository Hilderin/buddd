#pragma once

#include "render_device.h"

namespace buddd::engine {

class RenderDeviceHeadless final : public RenderDevice {
public:
    RenderDeviceHeadless(int width, int height);
    ~RenderDeviceHeadless() override = default;

    auto begin_frame() -> void override;
    auto end_frame() -> void override;
    auto size() const noexcept -> std::pair<int, int> override;

    RenderDeviceHeadless(const RenderDeviceHeadless&) = delete;
    auto operator=(const RenderDeviceHeadless&) -> RenderDeviceHeadless& = delete;
    RenderDeviceHeadless(RenderDeviceHeadless&&) = delete;
    auto operator=(RenderDeviceHeadless&&) -> RenderDeviceHeadless& = delete;

private:
    int width_;
    int height_;
};

} // namespace buddd::engine
