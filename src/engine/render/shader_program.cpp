#include "render/shader_program.h"

namespace buddd::engine {

ShaderProgram::~ShaderProgram() = default;

auto ShaderProgram::testing_handle() const -> uint32_t {
    return handle();
}

auto ShaderProgram::vs_source() const noexcept -> const std::string& {
    static const std::string empty;
    return empty;
}

auto ShaderProgram::fs_source() const noexcept -> const std::string& {
    static const std::string empty;
    return empty;
}

} // namespace buddd::engine
