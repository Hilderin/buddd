#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace buddd::engine {

/// Abstract base class for shader program backends.
/// Provides a platform-independent interface for GPU program handles.
/// CONST-001 compliant: uses uint32_t for handle type.
class ShaderProgram {
public:
    virtual ~ShaderProgram();

    /// Returns the backend-specific shader program handle.
    virtual auto handle() const -> uint32_t = 0;

    /// Returns true if the program has been successfully linked/compiled.
    virtual auto is_valid() const noexcept -> bool = 0;

    /// Hot-reload support: replaces the internal handle with a new one.
    /// In headless mode, this increments a generation counter.
    virtual auto replace_handle(uint32_t new_handle) -> void = 0;

    /// Releases ownership of the internal handle, clearing it.
    /// Returns the previous handle value. Used to prevent double-deletion
    /// when transferring ownership.
    virtual auto release_handle() noexcept -> uint32_t = 0;

    /// Returns the vertex shader source string (primarily for headless mode).
    /// Default implementation returns empty string.
    virtual auto vs_source() const noexcept -> const std::string&;

    /// Returns the fragment shader source string (primarily for headless mode).
    /// Default implementation returns empty string.
    virtual auto fs_source() const noexcept -> const std::string&;

    ShaderProgram(const ShaderProgram&) = delete;
    auto operator=(const ShaderProgram&) -> ShaderProgram& = delete;
    ShaderProgram(ShaderProgram&&) = delete;
    auto operator=(ShaderProgram&&) -> ShaderProgram& = delete;

protected:
    ShaderProgram() = default;
};

// Out-of-line destructor definition (needed for vtable emission).
// Implemented in shader_program.cpp.

} // namespace buddd::engine
