#pragma once

#include <string>
#include <string_view>

namespace buddd::engine {

/// Resolves a path from YAML. If absolute (starts with '/'), return as-is.
/// Otherwise, treat as relative to the working directory and return as-is
/// (the OS resolves relative to CWD).
inline auto resolve_yaml_path(std::string_view path) -> std::string {
    if (path.empty()) return std::string(path);
    if (path.front() == '/') return std::string(path);
    return std::string(path);
}

} // namespace buddd::engine
