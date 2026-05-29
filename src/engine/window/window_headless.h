#pragma once

#include "window.h"

namespace buddd::engine {

class WindowHeadless final : public Window {
public:
    WindowHeadless(int width, int height);
    ~WindowHeadless() override = default;

    auto width() const noexcept -> int override;
    auto height() const noexcept -> int override;
    auto native_handle() const noexcept -> void* override;

    WindowHeadless(const WindowHeadless&) = delete;
    auto operator=(const WindowHeadless&) -> WindowHeadless& = delete;
    WindowHeadless(WindowHeadless&&) = delete;
    auto operator=(WindowHeadless&&) -> WindowHeadless& = delete;

private:
    int width_;
    int height_;
};

} // namespace buddd::engine
