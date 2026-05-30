#define GL_GLEXT_PROTOTYPES
#include "render_device_opengl.h"
#include "shader_opengl.h"
#include "material_opengl.h"
#include "vertex_buffer_opengl.h"
#include "index_buffer_opengl.h"

#include <SDL3/SDL_opengl.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <tuple>

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
    }
    return {GL_FLOAT, 1}; // fallback
}

auto primitive_topology_to_gl(PrimitiveTopology topology) -> GLenum {
    switch (topology) {
        case PrimitiveTopology::Triangles:     return GL_TRIANGLES;
        case PrimitiveTopology::TriangleStrip: return GL_TRIANGLE_STRIP;
        case PrimitiveTopology::Lines:         return GL_LINES;
        case PrimitiveTopology::LineStrip:     return GL_LINE_STRIP;
        case PrimitiveTopology::Points:        return GL_POINTS;
    }
    return GL_TRIANGLES; // fallback
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

} // anonymous namespace

// ============================================================================
// Constructor / Destructor
// ============================================================================

RenderDeviceOpenGL::RenderDeviceOpenGL(SDL_Window* window, SDL_GLContext context)
    : window_(window), context_(context) {}

RenderDeviceOpenGL::~RenderDeviceOpenGL() {
    SDL_GL_DestroyContext(context_);
}

auto RenderDeviceOpenGL::begin_frame() -> void {
    glClear(GL_COLOR_BUFFER_BIT);
}

auto RenderDeviceOpenGL::end_frame() -> void {
    SDL_GL_SwapWindow(window_);
}

auto RenderDeviceOpenGL::size() const noexcept -> std::pair<int, int> {
    int w, h;
    SDL_GetWindowSize(window_, &w, &h);
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

        std::cerr << "Shader compilation failed: " << log << "\n";
        return make_error(Error::Category::ShaderCompilationFailed, std::move(log));
    }

    std::cerr << "Shader created (type="
              << (type == ShaderType::Vertex ? "Vertex" : "Fragment")
              << ")\n";

    return std::unique_ptr<Shader>(new ShaderOpenGL(shader_id, type));
}

auto RenderDeviceOpenGL::create_material(
    std::unique_ptr<Shader> vertex_shader,
    std::unique_ptr<Shader> fragment_shader,
    std::span<const std::string> /*known_uniforms*/
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

        std::cerr << "Material linking failed: " << log << "\n";
        return make_error(Error::Category::LinkingFailed, std::move(log));
    }

    // Shaders are marked for deletion; they stay alive until program is deleted.
    glDeleteShader(vs.handle());
    glDeleteShader(fs.handle());

    std::cerr << "Material created\n";
    return std::unique_ptr<Material>(new MaterialOpenGL(program));
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

    GLuint vao;
    glCreateVertexArrays(1, &vao);

    GLuint vbo;
    glCreateBuffers(1, &vbo);
    glNamedBufferStorage(vbo, data.size(), data.data(), GL_DYNAMIC_DRAW);

    // Configure VAO
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, static_cast<GLsizei>(format.stride));

    for (const auto& attr : format.attributes) {
        auto [gl_type, component_count] = vertex_attribute_type_to_gl(attr.type);

        glEnableVertexArrayAttrib(vao, attr.location);
        glVertexArrayAttribFormat(
            vao,
            attr.location,
            component_count,
            gl_type,
            attr.normalized ? GL_TRUE : GL_FALSE,
            static_cast<GLuint>(attr.offset)
        );
        glVertexArrayAttribBinding(vao, attr.location, 0);
    }

    uint32_t vertex_count = static_cast<uint32_t>(data.size() / format.stride);
    std::cerr << "Vertex buffer created (" << vertex_count
              << " vertices, " << format.attributes.size() << " attributes)\n";

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
    glCreateBuffers(1, &ibo);
    glNamedBufferStorage(ibo, data.size(), data.data(), GL_DYNAMIC_DRAW);

    std::cerr << "Index buffer created (" << data.size() << " bytes, "
              << (type == IndexType::Uint16 ? "Uint16" : "Uint32") << ")\n";

    return std::unique_ptr<IndexBuffer>(
        new IndexBufferOpenGL(ibo, type, data.size()));
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
    auto& mat = static_cast<const MaterialOpenGL&>(material);
    auto& vb = static_cast<const VertexBufferOpenGL&>(vertices);

    glUseProgram(mat.program());
    glBindVertexArray(vb.vao());
    glDrawArrays(primitive_topology_to_gl(topology), static_cast<GLint>(start_vertex), static_cast<GLsizei>(vertex_count));

#ifndef NDEBUG
    std::cerr << "Draw: " << primitive_topology_to_string(topology)
              << " " << vertex_count << " vertices\n";
#endif
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
    auto& mat = static_cast<const MaterialOpenGL&>(material);
    auto& vb = static_cast<const VertexBufferOpenGL&>(vertices);
    auto& ib = static_cast<const IndexBufferOpenGL&>(indices);

    GLenum gl_index_type = (ib.index_type() == IndexType::Uint16)
        ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

    GLsizeiptr index_byte_size = (ib.index_type() == IndexType::Uint16)
        ? static_cast<GLsizeiptr>(sizeof(uint16_t)) : static_cast<GLsizeiptr>(sizeof(uint32_t));

    glUseProgram(mat.program());
    glBindVertexArray(vb.vao());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib.handle());
    glDrawElements(
        primitive_topology_to_gl(topology),
        static_cast<GLsizei>(index_count),
        gl_index_type,
        reinterpret_cast<const void*>(static_cast<uintptr_t>(start_index) * static_cast<uintptr_t>(index_byte_size))
    );

#ifndef NDEBUG
    std::cerr << "Draw indexed: " << primitive_topology_to_string(topology)
              << " " << index_count << " indices\n";
#endif
}

} // namespace buddd::engine
