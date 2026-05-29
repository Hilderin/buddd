#pragma once

#include <memory>
#include <string>

namespace buddd::engine {

struct WindowConfig {
    std::string title;
    int width;
    int height;
};

class Window {
public:
    virtual ~Window() = default;

    virtual auto width() const noexcept -> int = 0;
    virtual auto height() const noexcept -> int = 0;
    virtual auto native_handle() const noexcept -> void* = 0;

    Window(const Window&) = delete;
    auto operator=(const Window&) -> Window& = delete;
    Window(Window&&) = delete;
    auto operator=(Window&&) -> Window& = delete;

protected:
    Window() = default;
};

} // namespace buddd::engine
