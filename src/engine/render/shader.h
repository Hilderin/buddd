#pragma once

#include "error.h"

#include <memory>
#include <string_view>

namespace buddd::engine {

enum class ShaderType {
    Vertex,
    Fragment
};

class Shader {
public:
    virtual ~Shader() = default;

    virtual auto type() const noexcept -> ShaderType = 0;

    Shader(const Shader&) = delete;
    auto operator=(const Shader&) -> Shader& = delete;
    Shader(Shader&&) = delete;
    auto operator=(Shader&&) -> Shader& = delete;

protected:
    Shader() = default;
};

} // namespace buddd::engine
