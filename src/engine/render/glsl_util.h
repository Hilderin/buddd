#pragma once

#include <string>
#include <string_view>
#include <unordered_set>

namespace buddd::engine::detail {

/// Extracts uniform names from GLSL source code.
/// Handles:
///   - `uniform type name;`
///   - `uniform type name[N];`        (array — stores base name)
///   - `uniform type name = default_value;`
///   - `uniform type name[N] = default_values;`
/// Also handles `layout(...) uniform type name;` by skipping the layout qualifier.
/// Returns base names (no array suffix, no default value).
auto extract_uniform_names(std::string_view glsl_source) -> std::unordered_set<std::string>;

/// Strips trailing `[N]` array subscript suffix.
/// E.g., "u_light_colors[3]" → "u_light_colors".
/// Returns name unchanged if no array subscript suffix.
auto normalize_uniform_name(std::string_view name) -> std::string;

} // namespace buddd::engine::detail
