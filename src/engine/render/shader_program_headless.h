#pragma once

#include "render/shader_program.h"
#include "render/shader.h"
#include "error.h"

#include <memory>
#include <string>
#include <utility>

namespace buddd::engine {

/// Headless implementation of ShaderProgram.
/// Stores a generation counter and source strings. No GL dependencies.
class ShaderProgramHeadless final : public ShaderProgram {
public:
    /// Creates a headless shader program with simulated linking.
    /// Checks vertex shader outputs match fragment shader inputs.
    [[nodiscard]] static auto create(
        std::unique_ptr<Shader> vertex_shader,
        std::unique_ptr<Shader> fragment_shader
    ) -> Result<std::unique_ptr<ShaderProgramHeadless>>;

    auto handle() const -> uint32_t override { return handle_; }
    auto is_valid() const noexcept -> bool override { return generation_ > 0; }

    auto replace_handle(uint32_t new_handle) -> void override;
    auto release_handle() noexcept -> uint32_t override;

    auto testing_handle() const -> uint32_t override { return static_cast<uint32_t>(generation_); }

    /// Returns the vertex shader source string.
    auto vs_source() const noexcept -> const std::string& override { return vs_source_; }

    /// Returns the fragment shader source string.
    auto fs_source() const noexcept -> const std::string& override { return fs_source_; }

    ShaderProgramHeadless(const ShaderProgramHeadless&) = delete;
    auto operator=(const ShaderProgramHeadless&) -> ShaderProgramHeadless& = delete;
    ShaderProgramHeadless(ShaderProgramHeadless&&) = delete;
    auto operator=(ShaderProgramHeadless&&) -> ShaderProgramHeadless& = delete;

private:
    ShaderProgramHeadless(uint64_t generation,
                          std::string vs_source,
                          std::string fs_source);

    uint32_t handle_ = 0;
    uint64_t generation_ = 0;
    std::string vs_source_;
    std::string fs_source_;
};

} // namespace buddd::engine
