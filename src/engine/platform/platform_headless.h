#pragma once

#include "platform.h"

namespace buddd::engine {

class PlatformHeadless final : public Platform {
public:
    ~PlatformHeadless() override = default;

    auto create_window(const WindowConfig& config) -> Result<std::unique_ptr<Window>> override;

    PlatformHeadless(const PlatformHeadless&) = delete;
    auto operator=(const PlatformHeadless&) -> PlatformHeadless& = delete;
    PlatformHeadless(PlatformHeadless&&) = delete;
    auto operator=(PlatformHeadless&&) -> PlatformHeadless& = delete;

private:
    friend auto Platform::create(Backend) -> Result<std::unique_ptr<Platform>>;
    PlatformHeadless() = default;
};

} // namespace buddd::engine
