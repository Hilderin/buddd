#include "render_device_headless.h"

namespace buddd::engine {

RenderDeviceHeadless::RenderDeviceHeadless(int width, int height)
    : width_(width), height_(height) {}

auto RenderDeviceHeadless::begin_frame() -> void {
    // no-op
}

auto RenderDeviceHeadless::end_frame() -> void {
    // no-op
}

auto RenderDeviceHeadless::size() const noexcept -> std::pair<int, int> {
    return {width_, height_};
}

} // namespace buddd::engine
