#pragma once

#include <memory>
#include <string>

namespace buddd::engine {

class Platform;
struct WindowConfig;

enum class WindowState { Normal, Maximized, Minimized };
struct WindowPosition { int x; int y; };

struct WindowConfig {
    std::string title;
    int width;
    int height;

    /// Window position (screen coordinates). (-1, -1) = let system decide.
    int x = -1;
    int y = -1;

    /// Window state (Normal / Maximized / Minimized).
    WindowState state = WindowState::Normal;
};

class Window {
public:
    virtual ~Window() = default;

    auto platform() noexcept -> Platform& { return platform_; }

    virtual auto width() const noexcept -> int = 0;
    virtual auto height() const noexcept -> int = 0;
    virtual auto native_handle() const noexcept -> void* = 0;

    virtual auto on_resize(int w, int h) -> void = 0;

    /// Set the OS window title string.
    virtual auto set_title(std::string title) -> void = 0;

    virtual auto set_mouse_capture(bool captured) -> void = 0;
    virtual auto is_mouse_captured() const noexcept -> bool = 0;

    [[nodiscard]] virtual auto position() const noexcept -> WindowPosition = 0;
    virtual auto set_position(WindowPosition pos) -> void = 0;
    [[nodiscard]] virtual auto state() const noexcept -> WindowState = 0;
    virtual auto set_state(WindowState state) -> void = 0;
    virtual auto resize(int width, int height) -> void = 0;

    Window(const Window&) = delete;
    auto operator=(const Window&) -> Window& = delete;
    Window(Window&&) = delete;
    auto operator=(Window&&) -> Window& = delete;

protected:
    explicit Window(Platform& platform) : platform_(platform) {}
    Platform& platform_;
};

} // namespace buddd::engine
