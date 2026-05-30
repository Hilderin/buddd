#include "render_device_headless.h"
#include "shader_headless.h"
#include "material_headless.h"
#include "vertex_buffer_headless.h"
#include "index_buffer_headless.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace buddd::engine {

// ============================================================================
// Helper functions (anonymous namespace)
// ============================================================================
namespace {

/// Concatenates strings with a separator.
auto join(const std::vector<std::string>& strings, const std::string& separator) -> std::string {
    std::ostringstream oss;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i > 0) oss << separator;
        oss << strings[i];
    }
    return oss.str();
}

/// Extracts variable names from GLSL source.
/// If is_output is true, looks for "out type name;" or "layout(...) out type name;"
/// If is_output is false, looks for "in type name;" or "layout(...) in type name;"
auto extract_variable_names(const std::string& source, bool is_output) -> std::vector<std::string> {
    std::vector<std::string> names;
    std::string keyword = is_output ? "out" : "in";

    size_t pos = 0;
    while (pos < source.size()) {
        // Find the keyword
        auto kw_pos = source.find(keyword, pos);
        if (kw_pos == std::string::npos) break;

        // Skip if the character before the keyword is alphanumeric (part of another word)
        if (kw_pos > 0 && (std::isalnum(static_cast<unsigned char>(source[kw_pos - 1])) || source[kw_pos - 1] == '_')) {
            pos = kw_pos + keyword.size();
            continue;
        }

        // The character after the keyword should be whitespace
        auto after_kw = kw_pos + keyword.size();
        if (after_kw >= source.size() || (!std::isspace(static_cast<unsigned char>(source[after_kw])) && source[after_kw] != '(')) {
            pos = after_kw;
            continue;
        }

        // Skip layout(...) qualifier if present before keyword
        // We already found the keyword, so we need to go backwards to see if there's "layout("
        // Actually, let's just handle both cases: keyword found, then we look for the type and name after it.

        // Skip until we find the type name (after keyword + whitespace)
        auto scan_pos = after_kw;

        // Skip whitespace
        while (scan_pos < source.size() && std::isspace(static_cast<unsigned char>(source[scan_pos]))) {
            ++scan_pos;
        }

        // Skip type (e.g., "vec4", "float", "int", "vec3")
        // The type is a sequence of alphanumeric characters and underscores
        while (scan_pos < source.size() && (std::isalnum(static_cast<unsigned char>(source[scan_pos])) || source[scan_pos] == '_')) {
            ++scan_pos;
        }

        // Skip whitespace
        while (scan_pos < source.size() && std::isspace(static_cast<unsigned char>(source[scan_pos]))) {
            ++scan_pos;
        }

        // Now we should be at the variable name
        // But there might be an array suffix or semicolon
        // Read the name (alphanumeric + underscore)
        std::string name;
        while (scan_pos < source.size() && (std::isalnum(static_cast<unsigned char>(source[scan_pos])) || source[scan_pos] == '_')) {
            name += source[scan_pos];
            ++scan_pos;
        }

        // Skip array suffix [N] if present
        if (scan_pos < source.size() && source[scan_pos] == '[') {
            while (scan_pos < source.size() && source[scan_pos] != ';' && source[scan_pos] != ']') {
                ++scan_pos;
            }
            if (scan_pos < source.size() && source[scan_pos] == ']') {
                ++scan_pos;
            }
        }

        // Skip whitespace
        while (scan_pos < source.size() && std::isspace(static_cast<unsigned char>(source[scan_pos]))) {
            ++scan_pos;
        }

        // Check for semicolon or comma (end of declaration)
        if (scan_pos < source.size() && (source[scan_pos] == ';' || source[scan_pos] == ',') && !name.empty()) {
            names.push_back(name);
        }

        pos = scan_pos;
    }

    return names;
}

/// Extracts uniform names from GLSL source.
/// Looks for "uniform type name;" or "layout(...) uniform type name;"
auto extract_uniform_names(const std::string& source) -> std::vector<std::string> {
    std::vector<std::string> names;

    size_t pos = 0;
    while (pos < source.size()) {
        // Find the "uniform" keyword
        auto kw_pos = source.find("uniform", pos);
        if (kw_pos == std::string::npos) break;

        // Skip if the character before the keyword is alphanumeric (part of another word)
        if (kw_pos > 0 && (std::isalnum(static_cast<unsigned char>(source[kw_pos - 1])) || source[kw_pos - 1] == '_')) {
            pos = kw_pos + 7; // "uniform" length
            continue;
        }

        // The character after "uniform" should be whitespace
        auto after_kw = kw_pos + 7; // "uniform"
        if (after_kw >= source.size() || !std::isspace(static_cast<unsigned char>(source[after_kw]))) {
            pos = after_kw;
            continue;
        }

        // Skip whitespace
        auto scan_pos = after_kw;
        while (scan_pos < source.size() && std::isspace(static_cast<unsigned char>(source[scan_pos]))) {
            ++scan_pos;
        }

        // Skip type (e.g., "vec4", "float", "sampler2D", "int", "mat4")
        while (scan_pos < source.size() && (std::isalnum(static_cast<unsigned char>(source[scan_pos])) || source[scan_pos] == '_')) {
            ++scan_pos;
        }

        // Skip whitespace
        while (scan_pos < source.size() && std::isspace(static_cast<unsigned char>(source[scan_pos]))) {
            ++scan_pos;
        }

        // Skip array suffix if present
        if (scan_pos < source.size() && source[scan_pos] == '[') {
            while (scan_pos < source.size() && source[scan_pos] != ']') {
                ++scan_pos;
            }
            if (scan_pos < source.size()) {
                ++scan_pos; // skip ']'
            }
            // Skip whitespace
            while (scan_pos < source.size() && std::isspace(static_cast<unsigned char>(source[scan_pos]))) {
                ++scan_pos;
            }
        }

        // Now we should be at the variable name
        std::string name;
        while (scan_pos < source.size() && (std::isalnum(static_cast<unsigned char>(source[scan_pos])) || source[scan_pos] == '_')) {
            name += source[scan_pos];
            ++scan_pos;
        }

        // Check for semicolon or comma (end of declaration)
        // Skip whitespace
        while (scan_pos < source.size() && std::isspace(static_cast<unsigned char>(source[scan_pos]))) {
            ++scan_pos;
        }

        if (scan_pos < source.size() && source[scan_pos] == ';' && !name.empty()) {
            names.push_back(name);
        }

        pos = scan_pos;
    }

    return names;
}

} // anonymous namespace

// ============================================================================
// Constructor
// ============================================================================

RenderDeviceHeadless::RenderDeviceHeadless(int width, int height)
    : width_(width), height_(height) {}

auto RenderDeviceHeadless::begin_frame() -> void {
    ++frame_begin_count_;
}

auto RenderDeviceHeadless::end_frame() -> void {
    ++frame_end_count_;
}

auto RenderDeviceHeadless::size() const noexcept -> std::pair<int, int> {
    return {width_, height_};
}

// ============================================================================
// Resource factories
// ============================================================================

auto RenderDeviceHeadless::create_shader(ShaderType type, std::string_view source)
    -> Result<std::unique_ptr<Shader>>
{
    if (source.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Shader source is empty");
    }

    // Simulate compilation error if source contains "#error"
    if (source.find("#error") != std::string_view::npos) {
        std::cerr << "Shader compilation failed (simulated)\n";
        return make_error(Error::Category::ShaderCompilationFailed,
            "Simulated compilation error: #error directive found in source");
    }

    ++shader_count_;

    std::cerr << "Shader created (Headless, type="
              << (type == ShaderType::Vertex ? "Vertex" : "Fragment")
              << ")\n";

    return std::unique_ptr<Shader>(new ShaderHeadless(type, std::string(source)));
}

auto RenderDeviceHeadless::create_material(
    std::unique_ptr<Shader> vertex_shader,
    std::unique_ptr<Shader> fragment_shader,
    std::span<const std::string> known_uniforms
) -> Result<std::unique_ptr<Material>>
{
    if (!vertex_shader || !fragment_shader) {
        return make_error(Error::Category::InvalidArgument,
            "Null shader passed to create_material");
    }

    auto& vs = static_cast<ShaderHeadless&>(*vertex_shader);
    auto& fs = static_cast<ShaderHeadless&>(*fragment_shader);

    // Simulate linking error: vertex output names vs fragment input names
    auto vs_outputs = extract_variable_names(vs.source(), /*is_output=*/true);
    auto fs_inputs  = extract_variable_names(fs.source(), /*is_output=*/false);

    // Check if any vertex output matches any fragment input
    bool has_matching = false;
    for (const auto& vs_out : vs_outputs) {
        for (const auto& fs_in : fs_inputs) {
            if (vs_out == fs_in) {
                has_matching = true;
                break;
            }
        }
        if (has_matching) break;
    }

    if (!fs_inputs.empty() && !has_matching) {
        std::cerr << "Material linking failed (simulated: no matching "
                     "vertex output / fragment input variables)\n";
        return make_error(Error::Category::LinkingFailed,
            "Simulated linking error: vertex shader outputs ("
            + join(vs_outputs, ", ") + ") do not match fragment shader inputs ("
            + join(fs_inputs, ", ") + ")");
    }

    ++material_count_;

    // Collect uniform names: from shader source parsing + explicit known_uniforms
    std::unordered_set<std::string> uniform_names;
    auto vs_uniforms = extract_uniform_names(vs.source());
    auto fs_uniforms = extract_uniform_names(fs.source());
    uniform_names.insert(vs_uniforms.begin(), vs_uniforms.end());
    uniform_names.insert(fs_uniforms.begin(), fs_uniforms.end());
    for (const auto& name : known_uniforms) {
        uniform_names.insert(name);
    }

    std::cerr << "Material created (Headless)\n";

    return std::unique_ptr<Material>(
        new MaterialHeadless(std::move(uniform_names)));
}

auto RenderDeviceHeadless::create_vertex_buffer(
    const VertexFormat& format,
    std::span<const std::byte> data
) -> Result<std::unique_ptr<VertexBuffer>>
{
    if (data.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Vertex data is empty");
    }
    if (format.stride == 0) {
        return make_error(Error::Category::InvalidArgument,
            "Vertex format stride must be positive");
    }
    if (format.attributes.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Vertex format must have at least one attribute");
    }

    ++vertex_buffer_count_;

    uint32_t vertex_count = static_cast<uint32_t>(data.size() / format.stride);
    std::cerr << "Vertex buffer created (Headless, " << vertex_count
              << " vertices)\n";

    return std::unique_ptr<VertexBuffer>(
        new VertexBufferHeadless(format, data));
}

auto RenderDeviceHeadless::create_index_buffer(
    std::span<const std::byte> data,
    IndexType type
) -> Result<std::unique_ptr<IndexBuffer>>
{
    if (data.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Index data is empty");
    }

    ++index_buffer_count_;

    std::cerr << "Index buffer created (Headless, " << data.size()
              << " bytes)\n";

    return std::unique_ptr<IndexBuffer>(
        new IndexBufferHeadless(type, data));
}

// ============================================================================
// Drawing methods
// ============================================================================

auto RenderDeviceHeadless::draw(
    PrimitiveTopology /*topology*/,
    const VertexBuffer& /*vertices*/,
    const Material& /*material*/,
    uint32_t /*vertex_count*/,
    uint32_t /*start_vertex*/
) -> void
{
    ++draw_call_count_;

#ifndef NDEBUG
    std::cerr << "Draw (Headless, " << /*vertex_count*/ "?"
              << ")\n";
#endif
}

auto RenderDeviceHeadless::draw_indexed(
    PrimitiveTopology /*topology*/,
    const VertexBuffer& /*vertices*/,
    const IndexBuffer& /*indices*/,
    const Material& /*material*/,
    uint32_t /*index_count*/,
    uint32_t /*start_index*/
) -> void
{
    ++draw_call_count_;

#ifndef NDEBUG
    std::cerr << "Draw indexed (Headless, " << /*index_count*/ "?"
              << ")\n";
#endif
}

auto RenderDeviceHeadless::read_pixels() -> Result<ImageBuffer> {
    return make_error(Error::Category::Unsupported,
        "read_pixels is not supported in headless mode");
}

} // namespace buddd::engine
