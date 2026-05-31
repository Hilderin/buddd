#pragma once

#include <memory>
#include <string>

namespace buddd::engine {

class Platform;
struct WindowConfig;

struct WindowConfig {
    std::string title;
    int width;
    int height;
};

class Window {
public:
    virtual ~Window() = default;

    auto platform() noexcept -> Platform& { return platform_; }

    virtual auto width() const noexcept -> int = 0;
    virtual auto height() const noexcept -> int = 0;
    virtual auto native_handle() const noexcept -> void* = 0;

    virtual auto set_mouse_capture(bool captured) -> void = 0;
    virtual auto is_mouse_captured() const noexcept -> bool = 0;

    Window(const Window&) = delete;
    auto operator=(const Window&) -> Window& = delete;
    Window(Window&&) = delete;
    auto operator=(Window&&) -> Window& = delete;

protected:
    explicit Window(Platform& platform) : platform_(platform) {}
    Platform& platform_;
};

} // namespace buddd::engine
