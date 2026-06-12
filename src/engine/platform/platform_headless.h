#pragma once

#include "platform.h"
#include "input/input_system_headless.h"

namespace buddd::engine {

class PlatformHeadless final : public Platform {
public:
    ~PlatformHeadless() override = default;

    auto create_window(const WindowConfig& config) -> Result<std::unique_ptr<Window>> override;
    auto poll_events() -> bool override;
    auto input_system() -> InputSystem& override;
    auto show_open_file_dialog(FileDialogCallback callback,
                                const char* filter_name,
                                const char* filter_pattern) -> void override;
    auto show_save_file_dialog(FileDialogCallback callback,
                                const char* filter_name,
                                const char* filter_pattern,
                                const char* default_name) -> void override;
    [[nodiscard]] auto delta_time() const noexcept -> float override;

    PlatformHeadless(const PlatformHeadless&) = delete;
    auto operator=(const PlatformHeadless&) -> PlatformHeadless& = delete;
    PlatformHeadless(PlatformHeadless&&) = delete;
    auto operator=(PlatformHeadless&&) -> PlatformHeadless& = delete;

private:
    friend auto Platform::create(Backend) -> Result<std::unique_ptr<Platform>>;
    PlatformHeadless() = default;

    InputSystemHeadless input_system_;
};

} // namespace buddd::engine
