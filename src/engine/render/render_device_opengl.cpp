#define GL_GLEXT_PROTOTYPES
#include "render_device_opengl.h"
#include "shader_opengl.h"
#include "material_opengl.h"
#include "vertex_buffer_opengl.h"
#include "index_buffer_opengl.h"
#include "texture_opengl.h"

#include "image/image.h"
#include "render/glsl_util.h"
#include "render/shader_program.h"
#include "render/shader_program_opengl.h"

#include <SDL3/SDL_opengl.h>

#include <cstdint>
#include <cstdio>
#include <string>
#include <tuple>

#include "log/log.h"
#include "debug/assert.h"

BUDDD_LOG_TAG("Render:OpenGL");

namespace buddd::engine {

// ============================================================================
// Helper functions (anonymous namespace)
// ============================================================================
namespace {

struct GlTypeAndCount {
    GLenum type;
    int count;
};

auto vertex_attribute_type_to_gl(VertexAttributeType attr_type) -> GlTypeAndCount {
    switch (attr_type) {
        case VertexAttributeType::Float:      return {GL_FLOAT, 1};
        case VertexAttributeType::Float2:     return {GL_FLOAT, 2};
        case VertexAttributeType::Float3:     return {GL_FLOAT, 3};
        case VertexAttributeType::Float4:     return {GL_FLOAT, 4};
        case VertexAttributeType::Int:        return {GL_INT, 1};
        case VertexAttributeType::Int2:       return {GL_INT, 2};
        case VertexAttributeType::Int3:       return {GL_INT, 3};
        case VertexAttributeType::Int4:       return {GL_INT, 4};
        case VertexAttributeType::UByte:      return {GL_UNSIGNED_BYTE, 1};
        case VertexAttributeType::UByte4:     return {GL_UNSIGNED_BYTE, 4};
        case VertexAttributeType::UByte4Norm: return {GL_UNSIGNED_BYTE, 4};
        default:
            BUDDD_FAIL_MSG("Unknown VertexAttributeType: {}", static_cast<int>(attr_type));
    }
}

auto primitive_topology_to_gl(PrimitiveTopology topology) -> GLenum {
    switch (topology) {
        case PrimitiveTopology::Triangles:     return GL_TRIANGLES;
        case PrimitiveTopology::TriangleStrip: return GL_TRIANGLE_STRIP;
        case PrimitiveTopology::Lines:         return GL_LINES;
        case PrimitiveTopology::LineStrip:     return GL_LINE_STRIP;
        case PrimitiveTopology::Points:        return GL_POINTS;
        default:
            BUDDD_FAIL_MSG("Unknown PrimitiveTopology: {}", static_cast<int>(topology));
    }
}

auto primitive_topology_to_string(PrimitiveTopology topology) -> const char* {
    switch (topology) {
        case PrimitiveTopology::Triangles:     return "Triangles";
        case PrimitiveTopology::TriangleStrip: return "TriangleStrip";
        case PrimitiveTopology::Lines:         return "Lines";
        case PrimitiveTopology::LineStrip:     return "LineStrip";
        case PrimitiveTopology::Points:        return "Points";
    }
    return "Unknown";
}

/// Formats a GLenum as "0x" followed by 4 lowercase hex digits.
auto to_hex_string(GLenum value) -> std::string {
    char buf[12];
    std::snprintf(buf, sizeof(buf), "0x%04x", static_cast<unsigned int>(value));
    return std::string(buf);
}

} // anonymous namespace

// ============================================================================
// Constructor / Destructor
// ============================================================================

RenderDeviceOpenGL::RenderDeviceOpenGL(Window& window, SDL_Window* sdl_window, SDL_GLContext context)
    : window_(window), sdl_window_(sdl_window), context_(context)
{
    // Enable hardware depth testing — fragments closer to the camera
    // (smaller Z after perspective divide and viewport depth-range
    // transform, in window coordinates [0,1]) occlude farther ones.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    BUDDD_LOG_DEBUG("Depth testing enabled (GL_LESS)");

    // Check for OpenGL errors after depth state setup (debug builds only).
    // Clear any prior error first, then check.
    glGetError();
    GLenum depth_err = glGetError();
    if (depth_err != GL_NO_ERROR) {
        BUDDD_LOG_DEBUG("Warning: OpenGL error during depth state setup: {}", depth_err);
    }
}

RenderDeviceOpenGL::~RenderDeviceOpenGL() {
    SDL_GL_DestroyContext(context_);
}

auto RenderDeviceOpenGL::begin_frame() -> void {
    int w, h;
    SDL_GetWindowSize(sdl_window_, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

auto RenderDeviceOpenGL::end_frame() -> void {
    SDL_GL_SwapWindow(sdl_window_);
}

auto RenderDeviceOpenGL::size() const noexcept -> std::pair<int, int> {
    int w, h;
    SDL_GetWindowSize(sdl_window_, &w, &h);
    return {w, h};
}

// ============================================================================
// Resource factories
// ============================================================================

auto RenderDeviceOpenGL::create_shader(ShaderType type, std::string_view source)
    -> Result<std::unique_ptr<Shader>>
{
    if (source.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Shader source is empty");
    }

    GLenum gl_type = (type == ShaderType::Vertex) ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
    GLuint shader_id = glCreateShader(gl_type);

    const GLchar* sources[] = { source.data() };
    GLint lengths[] = { static_cast<GLint>(source.size()) };
    glShaderSource(shader_id, 1, sources, lengths);
    glCompileShader(shader_id);

    GLint compile_status;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_status);

    if (compile_status != GL_TRUE) {
        GLint log_length;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
        std::string log(log_length, '\0');
        glGetShaderInfoLog(shader_id, log_length, nullptr, log.data());
        glDeleteShader(shader_id);

        BUDDD_LOG_ERROR("Shader compilation failed: {}", log);
        return make_error(Error::Category::ShaderCompilationFailed, std::move(log));
    }

    BUDDD_LOG_INFO("Shader created (type={})", (type == ShaderType::Vertex ? "Vertex" : "Fragment"));

    return std::unique_ptr<Shader>(new ShaderOpenGL(shader_id, type));
}

auto RenderDeviceOpenGL::create_shader_program(
    std::unique_ptr<Shader> vertex_shader,
    std::unique_ptr<Shader> fragment_shader
) -> Result<std::unique_ptr<ShaderProgram>>
{
    return ShaderProgramOpenGL::create(std::move(vertex_shader), std::move(fragment_shader));
}

auto RenderDeviceOpenGL::create_material(
    std::unique_ptr<Shader> vertex_shader,
    std::unique_ptr<Shader> fragment_shader,
    std::span<const std::string> known_uniforms
) -> Result<std::unique_ptr<Material>>
{
    if (!vertex_shader || !fragment_shader) {
        return make_error(Error::Category::InvalidArgument,
            "Null shader passed to create_material");
    }

    auto& vs = static_cast<ShaderOpenGL&>(*vertex_shader);
    auto& fs = static_cast<ShaderOpenGL&>(*fragment_shader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs.handle());
    glAttachShader(program, fs.handle());
    glLinkProgram(program);

    GLint link_status;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);

    if (link_status != GL_TRUE) {
        GLint log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        std::string log(log_length, '\0');
        glGetProgramInfoLog(program, log_length, nullptr, log.data());
        glDeleteProgram(program);

        BUDDD_LOG_ERROR("Material linking failed: {}", log);
        return make_error(Error::Category::LinkingFailed, std::move(log));
    }

    // Shaders are marked for deletion; they stay alive until program is deleted.
    glDeleteShader(vs.handle());
    glDeleteShader(fs.handle());

    // known_uniforms are accepted but not stored in MaterialOpenGL at this time;
    // OpenGL uses glGetUniformLocation at set_uniform time instead.
    // The include of glsl_util.h ensures the backend can be extended later.
    for (const auto& name : known_uniforms) {
        (void)name;
    }

    BUDDD_LOG_INFO("Material created");
    return std::unique_ptr<Material>(new MaterialOpenGL(program));
}

auto RenderDeviceOpenGL::create_material(std::shared_ptr<ShaderProgram> program)
    -> Result<std::unique_ptr<Material>>
{
    if (!program || !program->is_valid()) {
        return make_error(Error::Category::InvalidArgument, "Invalid ShaderProgram");
    }
    BUDDD_LOG_INFO("Material created (from ShaderProgram)");
    return std::unique_ptr<Material>(new MaterialOpenGL(std::move(program)));
}

auto RenderDeviceOpenGL::create_vertex_buffer(
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

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    // Upload vertex data (non-DSA for maximum compatibility)
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size(), data.data(), GL_DYNAMIC_DRAW);

    // Configure vertex attributes
    for (const auto& attr : format.attributes) {
        auto [gl_type, component_count] = vertex_attribute_type_to_gl(attr.type);

        glEnableVertexAttribArray(attr.location);
        glVertexAttribFormat(
            attr.location,
            component_count,
            gl_type,
            attr.normalized ? GL_TRUE : GL_FALSE,
            static_cast<GLuint>(attr.offset)
        );
        glVertexAttribBinding(attr.location, 0);
    }

    // Bind the VBO to the VAO at binding index 0
    glBindVertexBuffer(0, vbo, 0, static_cast<GLsizei>(format.stride));

    // Unbind to clean up state (avoids accidental state leaks)
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    uint32_t vertex_count = static_cast<uint32_t>(data.size() / format.stride);
    BUDDD_LOG_INFO("Vertex buffer created ({} vertices, {} attributes)", vertex_count, format.attributes.size());

    return std::unique_ptr<VertexBuffer>(
        new VertexBufferOpenGL(vao, vbo, format, data.size()));
}

auto RenderDeviceOpenGL::create_index_buffer(
    std::span<const std::byte> data,
    IndexType type
) -> Result<std::unique_ptr<IndexBuffer>>
{
    if (data.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Index data is empty");
    }

    GLuint ibo;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.size(), data.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    BUDDD_LOG_INFO("Index buffer created ({} bytes, {})", data.size(), (type == IndexType::Uint16 ? "Uint16" : "Uint32"));

    return std::unique_ptr<IndexBuffer>(
        new IndexBufferOpenGL(ibo, type, data.size()));
}

auto RenderDeviceOpenGL::create_texture(const Image& image) -> Result<std::unique_ptr<Texture>> {
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

    // Create OpenGL texture with DSA
    GLuint tex;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);

    GLenum internal_format = (ch == 4) ? GL_RGBA8 :
                             (ch == 3) ? GL_RGB8 : GL_R8;

    GLenum format = (ch == 4) ? GL_RGBA :
                    (ch == 3) ? GL_RGB : GL_RED;

    glTextureStorage2D(tex, 1, internal_format, image.width(), image.height());
    glTextureSubImage2D(tex, 0, 0, 0, image.width(), image.height(), format, GL_UNSIGNED_BYTE, image.data().data());

    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Check for GL errors after upload
    GLenum gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        glDeleteTextures(1, &tex);
        return make_error(Error::Category::TextureCreationFailed,
            "OpenGL texture creation failed with error code 0x"
            + to_hex_string(gl_error));
    }

    BUDDD_LOG_INFO("Texture created (OpenGL, {}x{}, {} channels)", image.width(), image.height(), ch);

    return std::unique_ptr<Texture>(new TextureOpenGL(tex, image.width(), image.height(), ch));
}

// ============================================================================
// Drawing methods
// ============================================================================

auto RenderDeviceOpenGL::draw(
    PrimitiveTopology topology,
    const VertexBuffer& vertices,
    const Material& material,
    uint32_t vertex_count,
    uint32_t start_vertex
) -> void
{
    auto& vb = static_cast<const VertexBufferOpenGL&>(vertices);

    material.bind();  // virtual dispatch — supports MaterialOpenGL, PhongMaterial, etc.
    glBindVertexArray(vb.vao());
    glDrawArrays(primitive_topology_to_gl(topology), static_cast<GLint>(start_vertex), static_cast<GLsizei>(vertex_count));

    BUDDD_LOG_DEBUG("Draw: {} {} vertices", primitive_topology_to_string(topology), vertex_count);
}

auto RenderDeviceOpenGL::draw_indexed(
    PrimitiveTopology topology,
    const VertexBuffer& vertices,
    const IndexBuffer& indices,
    const Material& material,
    uint32_t index_count,
    uint32_t start_index
) -> void
{
    auto& vb = static_cast<const VertexBufferOpenGL&>(vertices);
    auto& ib = static_cast<const IndexBufferOpenGL&>(indices);

    GLenum gl_index_type = (ib.index_type() == IndexType::Uint16)
        ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

    GLsizeiptr index_byte_size = (ib.index_type() == IndexType::Uint16)
        ? static_cast<GLsizeiptr>(sizeof(uint16_t)) : static_cast<GLsizeiptr>(sizeof(uint32_t));

    material.bind();  // virtual dispatch — supports MaterialOpenGL, PhongMaterial, etc.
    glBindVertexArray(vb.vao());
    // The index buffer binding is part of the VAO state, so we bind it before drawing
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib.handle());
    glDrawElements(
        primitive_topology_to_gl(topology),
        static_cast<GLsizei>(index_count),
        gl_index_type,
        reinterpret_cast<const void*>(static_cast<uintptr_t>(start_index) * static_cast<uintptr_t>(index_byte_size))
    );

    BUDDD_LOG_DEBUG("Draw indexed: {} {} indices", primitive_topology_to_string(topology), index_count);
}

// ============================================================================
// Fallback material
// ============================================================================

auto RenderDeviceOpenGL::fallback_material() noexcept -> Material& {
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

// ============================================================================
// read_pixels
// ============================================================================

auto RenderDeviceOpenGL::read_pixels() -> Result<ImageBuffer> {
    auto [width, height] = size();

    ImageBuffer buffer;
    buffer.width = width;
    buffer.height = height;
    buffer.channels = 4;  // RGBA
    buffer.data.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);

    // Read from the back buffer (where we draw). We call read_pixels BEFORE
    // end_frame() to capture the freshly rendered frame before the buffer swap.
    glReadBuffer(GL_BACK);

    // Set pixel storage alignment to 1 (tightly packed)
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    // Clear previous GL error
    glGetError();

    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data.data());

    GLenum gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        return make_error(Error::Category::ReadbackFailed,
            "glReadPixels failed with error code " + to_hex_string(gl_error));
    }

    return buffer;
}

} // namespace buddd::engine
