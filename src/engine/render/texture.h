#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace buddd::engine {

class Texture {
public:
    virtual ~Texture() = default;

    virtual auto width() const noexcept -> int = 0;
    virtual auto height() const noexcept -> int = 0;
    virtual auto channels() const noexcept -> int = 0;

    /// Hot-reload support: replaces the underlying GPU handle.
    /// new_handle is a backend-specific handle (GLuint cast to uint32_t).
    /// Default implementation is no-op (for headless).
    virtual auto replace_gl_handle(uint32_t /*new_handle*/) -> void {}

    /// Returns the backend-specific handle (GLuint cast to uint32_t).
    /// Default returns 0.
    virtual auto gl_handle() const noexcept -> uint32_t { return 0; }

    /// Releases ownership of the underlying GPU handle, clearing it to 0.
    /// Returns the previous handle value. Used to prevent double-deletion
    /// when the temporary texture's destructor would otherwise delete the
    /// handle that was just swapped into the existing texture.
    /// Default returns 0 (no-op for headless).
    virtual auto release_gl_handle() noexcept -> uint32_t { return 0; }

    Texture(const Texture&) = delete;
    auto operator=(const Texture&) -> Texture& = delete;
    Texture(Texture&&) = delete;
    auto operator=(Texture&&) -> Texture& = delete;

protected:
    Texture() = default;
};

} // namespace buddd::engine
