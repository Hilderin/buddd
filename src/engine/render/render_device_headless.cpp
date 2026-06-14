#include "render_device_headless.h"
#include "window/window.h"
#include "shader_headless.h"
#include "material_headless.h"
#include "vertex_buffer_headless.h"
#include "index_buffer_headless.h"
#include "texture_headless.h"

#include "image/image.h"
#include "render/glsl_util.h"
#include "render/shader_program.h"
#include "render/shader_program_headless.h"
#include "render/frame_buffer_headless.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_set>

#include "log/log.h"

BUDDD_LOG_TAG("Render:Headless");

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

} // anonymous namespace

// ============================================================================
// Constructor
// ============================================================================

RenderDeviceHeadless::RenderDeviceHeadless(Window& window)
    : window_(window) {}

auto RenderDeviceHeadless::begin_frame() -> void {
    ++frame_begin_count_;
}

auto RenderDeviceHeadless::end_frame() -> void {
    ++frame_end_count_;
}

auto RenderDeviceHeadless::size() const noexcept -> std::pair<int, int> {
    return {window_.width(), window_.height()};
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
        BUDDD_LOG_ERROR("Shader compilation failed (simulated)");
        return make_error(Error::Category::ShaderCompilationFailed,
            "Simulated compilation error: #error directive found in source");
    }

    ++shader_count_;

    BUDDD_LOG_INFO("Shader created (Headless, type={})", (type == ShaderType::Vertex ? "Vertex" : "Fragment"));

    return std::unique_ptr<Shader>(new ShaderHeadless(type, std::string(source)));
}

auto RenderDeviceHeadless::create_shader_program(
    std::unique_ptr<Shader> vertex_shader,
    std::unique_ptr<Shader> fragment_shader
) -> Result<std::unique_ptr<ShaderProgram>>
{
    return ShaderProgramHeadless::create(std::move(vertex_shader), std::move(fragment_shader));
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
        BUDDD_LOG_ERROR("Material linking failed (simulated: no matching vertex output / fragment input variables)");
        return make_error(Error::Category::LinkingFailed,
            "Simulated linking error: vertex shader outputs ("
            + join(vs_outputs, ", ") + ") do not match fragment shader inputs ("
            + join(fs_inputs, ", ") + ")");
    }

    ++material_count_;

    // Collect uniform names: from shader source parsing + explicit known_uniforms
    std::unordered_set<std::string> uniform_names;
    auto vs_uniforms = detail::extract_uniform_names(vs.source());
    auto fs_uniforms = detail::extract_uniform_names(fs.source());
    uniform_names.insert(vs_uniforms.begin(), vs_uniforms.end());
    uniform_names.insert(fs_uniforms.begin(), fs_uniforms.end());
    for (const auto& name : known_uniforms) {
        uniform_names.insert(name);
    }

    BUDDD_LOG_INFO("Material created (Headless)");

    return std::unique_ptr<Material>(
        new MaterialHeadless(std::move(uniform_names)));
}

auto RenderDeviceHeadless::create_material(std::shared_ptr<ShaderProgram> program)
    -> Result<std::unique_ptr<Material>>
{
    if (!program || !program->is_valid()) {
        return make_error(Error::Category::InvalidArgument, "Invalid ShaderProgram");
    }
    ++material_count_;

    // Extract uniform names from shader sources (available in headless mode)
    std::unordered_set<std::string> uniform_names;
    auto vs_uniforms = detail::extract_uniform_names(program->vs_source());
    auto fs_uniforms = detail::extract_uniform_names(program->fs_source());
    uniform_names.insert(vs_uniforms.begin(), vs_uniforms.end());
    uniform_names.insert(fs_uniforms.begin(), fs_uniforms.end());

    BUDDD_LOG_INFO("Material created (Headless, from ShaderProgram)");
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
    BUDDD_LOG_INFO("Vertex buffer created (Headless, {} vertices)", vertex_count);

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

    BUDDD_LOG_INFO("Index buffer created (Headless, {} bytes)", data.size());

    return std::unique_ptr<IndexBuffer>(
        new IndexBufferHeadless(type, data));
}

auto RenderDeviceHeadless::create_texture(const Image& image) -> Result<std::unique_ptr<Texture>> {
    // Validation
    if (image.width() <= 0 || image.height() <= 0) {
        return make_error(Error::Category::InvalidArgument,
            "Image dimensions must be positive (got " + std::to_string(image.width())
            + "x" + std::to_string(image.height()) + ")");
    }

    int ch = image.channels();
    if (ch != 1 && ch != 3 && ch != 4) {
        return make_error(Error::Category::Unsupported,
            "Unsupported channel count: " + std::to_string(ch)
            + " (supported: 1, 3, 4)");
    }

    if (image.data().empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Image data is empty");
    }

    if (image.data().size() != static_cast<size_t>(image.width() * image.height() * ch)) {
        return make_error(Error::Category::InvalidArgument,
            "Image data size mismatch: expected "
            + std::to_string(image.width() * image.height() * ch)
            + " bytes, got " + std::to_string(image.data().size()));
    }

    BUDDD_LOG_INFO("Texture created (Headless, {}x{}, {} channels)", image.width(), image.height(), ch);

    return std::unique_ptr<Texture>(
        new TextureHeadless(image.width(), image.height(), ch,
                            image.data()));
}

// ============================================================================
// Render texture / FBO / read_pixels(FBO)
// ============================================================================

auto RenderDeviceHeadless::create_render_texture(uint32_t width, uint32_t height)
    -> Result<std::unique_ptr<Texture>>
{
    // Validate dimensions
    if (width == 0 || height == 0) {
        return make_error(Error::Category::InvalidArgument,
            "Render texture dimensions must be positive");
    }

    std::vector<std::byte> data(static_cast<size_t>(width) * static_cast<size_t>(height) * 4, std::byte{0});

    BUDDD_LOG_INFO("Render texture created (Headless, {}x{})", width, height);

    return std::unique_ptr<Texture>(
        new TextureHeadless(static_cast<int>(width), static_cast<int>(height), 4, std::move(data)));
}

auto RenderDeviceHeadless::create_frame_buffer(uint32_t width, uint32_t height)
    -> Result<std::unique_ptr<FrameBuffer>>
{
    return FrameBufferHeadless::create(width, height);
}

auto RenderDeviceHeadless::read_pixels(FrameBuffer& /*fbo*/)
    -> Result<ImageBuffer>
{
    return make_error(Error::Category::Unsupported,
        "read_pixels with FBO is not supported in headless mode");
}

// ============================================================================
// Drawing methods
// ============================================================================

auto RenderDeviceHeadless::draw(
    PrimitiveTopology /*topology*/,
    const VertexBuffer& /*vertices*/,
    const Material& material,
    uint32_t /*vertex_count*/,
    uint32_t /*start_vertex*/
) -> void
{
    material.bind();
    ++draw_call_count_;

    BUDDD_LOG_DEBUG("Draw (Headless)");
}

auto RenderDeviceHeadless::draw_indexed(
    PrimitiveTopology /*topology*/,
    const VertexBuffer& /*vertices*/,
    const IndexBuffer& /*indices*/,
    const Material& material,
    uint32_t /*index_count*/,
    uint32_t /*start_index*/
) -> void
{
    material.bind();
    ++draw_call_count_;

    BUDDD_LOG_DEBUG("Draw indexed (Headless)");
}

auto RenderDeviceHeadless::fallback_material() noexcept -> Material& {
    if (!fallback_material_) {
        constexpr std::string_view vs_src = R"(
            #version 450 core
            layout(location = 0) in vec3 a_position;
            void main() {
                gl_Position = vec4(a_position, 1.0);
            }
        )";
        constexpr std::string_view fs_src = R"(
            #version 450 core
            out vec4 frag_color;
            void main() {
                frag_color = vec4(1.0, 0.0, 1.0, 1.0);
            }
        )";

        auto vs = create_shader(ShaderType::Vertex, vs_src);
        if (!vs) {
            BUDDD_LOG_ERROR("FATAL: fallback vertex shader creation failed: {}", to_string(vs.error()));
            std::terminate();
        }
        auto fs = create_shader(ShaderType::Fragment, fs_src);
        if (!fs) {
            BUDDD_LOG_ERROR("FATAL: fallback fragment shader creation failed: {}", to_string(fs.error()));
            std::terminate();
        }
        auto mat = create_material(std::move(*vs), std::move(*fs));
        if (!mat) {
            BUDDD_LOG_ERROR("FATAL: fallback material creation failed: {}", to_string(mat.error()));
            std::terminate();
        }
        fallback_material_ = std::move(*mat);
    }
    return *fallback_material_;
}

auto RenderDeviceHeadless::read_pixels() -> Result<ImageBuffer> {
    return make_error(Error::Category::Unsupported,
        "read_pixels is not supported in headless mode");
}

} // namespace buddd::engine
