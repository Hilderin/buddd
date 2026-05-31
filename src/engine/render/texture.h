#pragma once

#include <cstddef>
#include <memory>

namespace buddd::engine {

class Texture {
public:
    virtual ~Texture() = default;

    virtual auto width() const noexcept -> int = 0;
    virtual auto height() const noexcept -> int = 0;
    virtual auto channels() const noexcept -> int = 0;

    Texture(const Texture&) = delete;
    auto operator=(const Texture&) -> Texture& = delete;
    Texture(Texture&&) = delete;
    auto operator=(Texture&&) -> Texture& = delete;

protected:
    Texture() = default;
};

} // namespace buddd::engine
