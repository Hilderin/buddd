#pragma once

#include "error.h"
#include "image/image_buffer.h"

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace buddd::engine {

class Image;
class Shader;
class Material;
class Texture;
class ShaderProgram;
class VertexBuffer;
class IndexBuffer;
enum class ShaderType;
enum class PrimitiveTopology;
enum class IndexType;
struct VertexFormat;
class Window;

class RenderDevice {
public:
    [[nodiscard]] static auto create(Window& window) -> Result<std::unique_ptr<RenderDevice>>;

    virtual ~RenderDevice() = default;

    virtual auto window() noexcept -> Window& = 0;

    virtual auto begin_frame() -> void = 0;
    virtual auto end_frame() -> void = 0;
    virtual auto size() const noexcept -> std::pair<int, int> = 0;

    virtual auto frame_begin_count() const noexcept -> int { return 0; }
    virtual auto frame_end_count() const noexcept -> int { return 0; }
    virtual auto draw_call_count() const noexcept -> int { return 0; }

    // -- Resource factories --
    [[nodiscard]] virtual auto create_shader(ShaderType type, std::string_view source) -> Result<std::unique_ptr<Shader>> = 0;
    [[nodiscard]] virtual auto create_shader_program(
        std::unique_ptr<Shader> vertex_shader,
        std::unique_ptr<Shader> fragment_shader
    ) -> Result<std::unique_ptr<ShaderProgram>> = 0;
    [[nodiscard]] virtual auto create_material(
        std::unique_ptr<Shader> vertex_shader,
        std::unique_ptr<Shader> fragment_shader,
        std::span<const std::string> known_uniforms = {}
    ) -> Result<std::unique_ptr<Material>> = 0;
    [[nodiscard]] virtual auto create_material(std::shared_ptr<ShaderProgram> program)
        -> Result<std::unique_ptr<Material>> = 0;
    [[nodiscard]] virtual auto create_vertex_buffer(
        const VertexFormat& format,
        std::span<const std::byte> data
    ) -> Result<std::unique_ptr<VertexBuffer>> = 0;
    [[nodiscard]] virtual auto create_index_buffer(
        std::span<const std::byte> data,
        IndexType type
    ) -> Result<std::unique_ptr<IndexBuffer>> = 0;

    [[nodiscard]] virtual auto create_texture(const Image& image)
        -> Result<std::unique_ptr<Texture>> = 0;

    // -- Drawing --
    virtual auto draw(
        PrimitiveTopology topology,
        const VertexBuffer& vertices,
        const Material& material,
        uint32_t vertex_count,
        uint32_t start_vertex = 0
    ) -> void = 0;

    virtual auto draw_indexed(
        PrimitiveTopology topology,
        const VertexBuffer& vertices,
        const IndexBuffer& indices,
        const Material& material,
        uint32_t index_count,
        uint32_t start_index = 0
    ) -> void = 0;

    /// Returns a reference to a shared fallback material that renders
    /// solid magenta (RGB 1,0,1). Created once, lives as long as the RenderDevice.
    virtual auto fallback_material() noexcept -> Material& = 0;

    /// Reads the current framebuffer contents into an ImageBuffer.
    /// The returned ImageBuffer has bottom-left pixel origin (OpenGL convention).
    /// The caller should use Image::create() to flip rows to top-left origin.
    /// @return ImageBuffer with width, height, channels=4, raw RGBA data.
    ///         Returns an error if the backend does not support readback.
    [[nodiscard]] virtual auto read_pixels() -> Result<ImageBuffer> = 0;

    /// Render any active UI overlay (ImGui).
    /// Default no-op — overridden by OpenGL backend to call engine_imgui::render().
    virtual auto render_ui() -> void {}

    RenderDevice(const RenderDevice&) = delete;
    auto operator=(const RenderDevice&) -> RenderDevice& = delete;
    RenderDevice(RenderDevice&&) = delete;
    auto operator=(RenderDevice&&) -> RenderDevice& = delete;

protected:
    RenderDevice() = default;
};

} // namespace buddd::engine
