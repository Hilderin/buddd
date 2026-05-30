#pragma once

#include "shader.h"

#include <string>

namespace buddd::engine {

class ShaderHeadless final : public Shader {
public:
    ShaderHeadless(ShaderType type, std::string source);
    ~ShaderHeadless() override = default;

    auto type() const noexcept -> ShaderType override;

    auto source() const noexcept -> const std::string&;

    ShaderHeadless(const ShaderHeadless&) = delete;
    auto operator=(const ShaderHeadless&) -> ShaderHeadless& = delete;
    ShaderHeadless(ShaderHeadless&&) = delete;
    auto operator=(ShaderHeadless&&) -> ShaderHeadless& = delete;

private:
    ShaderType type_;
    std::string source_;
};

} // namespace buddd::engine
