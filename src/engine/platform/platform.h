#pragma once

#include "error.h"

#include <functional>
#include <memory>
#include <optional>

namespace buddd::engine {

enum class Backend {
    SDL3,
    Headless
};

class Window;
struct WindowConfig;
class InputSystem;

/// Callback invoked when a native file dialog completes.
/// filepath is the selected path, or std::nullopt if cancelled/error.
/// The callback is invoked on the main thread during poll_events().
using FileDialogCallback = std::function<void(std::optional<std::string> filepath)>;

struct DisplayBounds { int x; int y; int width; int height; };

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

    /// Returns the number of connected video displays.
    [[nodiscard]] virtual auto display_count() const noexcept -> int = 0;

    /// Returns the bounding rectangle of the specified display in virtual screen coordinates.
    /// If index is out of range, returns {0, 0, 0, 0}.
    [[nodiscard]] virtual auto display_bounds(int index) const noexcept -> DisplayBounds = 0;

    /// Show a native "Open File" dialog (non-blocking).
    /// callback is invoked on the main thread (during poll_events()) when
    /// the user selects a file or cancels.
    virtual auto show_open_file_dialog(FileDialogCallback callback,
                                        const char* filter_name,
                                        const char* filter_pattern) -> void = 0;

    /// Show a native "Save File" dialog (non-blocking).
    /// callback is invoked on the main thread (during poll_events()) when
    /// the user selects a file or cancels.
    /// default_name is the suggested file name (may be nullptr).
    virtual auto show_save_file_dialog(FileDialogCallback callback,
                                        const char* filter_name,
                                        const char* filter_pattern,
                                        const char* default_name) -> void = 0;

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
