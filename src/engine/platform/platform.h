#pragma once

#include "error.h"

#include <functional>
#include <memory>

namespace buddd::engine {

enum class Backend {
    SDL3,
    Headless
};

class Window;
struct WindowConfig;
class InputSystem;

class Platform {
public:
    [[nodiscard]] static auto create(Backend backend) -> Result<std::unique_ptr<Platform>>;

    virtual ~Platform() = default;

    [[nodiscard]] virtual auto create_window(const WindowConfig& config) -> Result<std::unique_ptr<Window>> = 0;

    /// Polls the platform event queue.
    /// Returns false if the user requested to quit (e.g., window close button),
    /// true otherwise. In headless mode, always returns true.
    virtual auto poll_events() -> bool = 0;

    /// Register a callback invoked when the platform receives a quit/close request.
    /// The callback returns true to allow the close, false to cancel it.
    /// If no callback is registered, the close proceeds normally.
    auto set_on_close_request(std::function<bool()> callback) -> void {
        close_request_callback_ = std::move(callback);
    }

    /// Returns a reference to the input system owned by this platform.
    /// The reference remains valid for the lifetime of the Platform.
    virtual auto input_system() -> InputSystem& = 0;

    /// Returns the time elapsed since the last poll_events() call, in seconds.
    /// Under normal operation, always > 0. Useful for framerate-independent movement.
    [[nodiscard]] virtual auto delta_time() const noexcept -> float = 0;

    Platform(const Platform&) = delete;
    auto operator=(const Platform&) -> Platform& = delete;
    Platform(Platform&&) = delete;
    auto operator=(Platform&&) -> Platform& = delete;

protected:
    Platform() = default;

    /// Close-request callback, invoked by subclasses in poll_events() on quit events.
    std::function<bool()> close_request_callback_;
};

} // namespace buddd::engine
