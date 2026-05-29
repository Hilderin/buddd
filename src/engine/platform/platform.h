#pragma once

#include "error.h"

#include <memory>

namespace buddd::engine {

enum class Backend {
    SDL3,
    Headless
};

class Window;
struct WindowConfig;

class Platform {
public:
    static auto create(Backend backend) -> Result<std::unique_ptr<Platform>>;

    virtual ~Platform() = default;

    virtual auto create_window(const WindowConfig& config) -> Result<std::unique_ptr<Window>> = 0;

    Platform(const Platform&) = delete;
    auto operator=(const Platform&) -> Platform& = delete;
    Platform(Platform&&) = delete;
    auto operator=(Platform&&) -> Platform& = delete;

protected:
    Platform() = default;
};

} // namespace buddd::engine
