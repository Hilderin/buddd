#include "render/glsl_util.h"

#include <algorithm>
#include <cctype>

namespace buddd::engine::detail {

auto extract_uniform_names(std::string_view glsl_source) -> std::unordered_set<std::string> {
    std::unordered_set<std::string> names;

    size_t pos = 0;
    while (pos < glsl_source.size()) {
        // Find the "uniform" keyword
        auto kw_pos = glsl_source.find("uniform", pos);
        if (kw_pos == std::string_view::npos) break;

        // Skip if the character before the keyword is alphanumeric or underscore
        // (meaning it's part of another word, e.g. "my_uniform")
        if (kw_pos > 0 &&
            (std::isalnum(static_cast<unsigned char>(glsl_source[kw_pos - 1])) ||
             glsl_source[kw_pos - 1] == '_')) {
            pos = kw_pos + 7; // "uniform" length
            continue;
        }

        // Check that the character after "uniform" is a space or '(' (for layout)
        auto after_kw = kw_pos + 7; // skip "uniform"
        if (after_kw >= glsl_source.size()) break;

        // Skip whitespace after "uniform"
        auto scan_pos = after_kw;
        while (scan_pos < glsl_source.size() &&
               std::isspace(static_cast<unsigned char>(glsl_source[scan_pos]))) {
            ++scan_pos;
        }

        // Skip type (e.g., "vec4", "float", "sampler2D", "int", "mat4")
        while (scan_pos < glsl_source.size() &&
               (std::isalnum(static_cast<unsigned char>(glsl_source[scan_pos])) ||
                glsl_source[scan_pos] == '_')) {
            ++scan_pos;
        }

        // Skip whitespace after type
        while (scan_pos < glsl_source.size() &&
               std::isspace(static_cast<unsigned char>(glsl_source[scan_pos]))) {
            ++scan_pos;
        }

        // Now read the variable name (alphanumeric + underscore)
        std::string name;
        while (scan_pos < glsl_source.size() &&
               (std::isalnum(static_cast<unsigned char>(glsl_source[scan_pos])) ||
                glsl_source[scan_pos] == '_')) {
            name += glsl_source[scan_pos];
            ++scan_pos;
        }

        if (name.empty()) {
            pos = scan_pos;
            continue;
        }

        // Skip optional array suffix [N] after the name
        if (scan_pos < glsl_source.size() && glsl_source[scan_pos] == '[') {
            while (scan_pos < glsl_source.size() && glsl_source[scan_pos] != ']') {
                ++scan_pos;
            }
            if (scan_pos < glsl_source.size()) {
                ++scan_pos; // skip ']'
            }
        }

        // Skip whitespace
        while (scan_pos < glsl_source.size() &&
               std::isspace(static_cast<unsigned char>(glsl_source[scan_pos]))) {
            ++scan_pos;
        }

        // Skip optional "= ..." default value clause
        if (scan_pos < glsl_source.size() && glsl_source[scan_pos] == '=') {
            ++scan_pos; // skip '='
            // Skip until we hit ';'
            while (scan_pos < glsl_source.size() && glsl_source[scan_pos] != ';') {
                ++scan_pos;
            }
        }

        // Check for semicolon (end of declaration)
        if (scan_pos < glsl_source.size() && glsl_source[scan_pos] == ';') {
            names.insert(name);
        }

        pos = scan_pos;
    }

    return names;
}

auto normalize_uniform_name(std::string_view name) -> std::string {
    auto bracket_pos = name.find('[');
    if (bracket_pos != std::string_view::npos) {
        return std::string(name.substr(0, bracket_pos));
    }
    return std::string(name);
}

} // namespace buddd::engine::detail
