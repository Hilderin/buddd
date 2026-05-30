#include "shader_headless.h"

namespace buddd::engine {

ShaderHeadless::ShaderHeadless(ShaderType type, std::string source)
    : type_(type), source_(std::move(source)) {}

auto ShaderHeadless::type() const noexcept -> ShaderType {
    return type_;
}

auto ShaderHeadless::source() const noexcept -> const std::string& {
    return source_;
}

} // namespace buddd::engine
