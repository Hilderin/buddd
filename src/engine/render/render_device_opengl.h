#pragma once

#include "render_device.h"
#include "shader.h"
#include "material.h"
#include "render/shader_program.h"
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "primitive_topology.h"

#include <SDL3/SDL.h>

namespace buddd::engine {

class RenderDeviceOpenGL final : public RenderDevice {
public:
    RenderDeviceOpenGL(Window& window, SDL_Window* sdl_window, SDL_GLContext context);
    ~RenderDeviceOpenGL() override;

    auto window() noexcept -> Window& override { return window_; }

    auto begin_frame() -> void override;
    auto end_frame() -> void override;
    auto clear() -> void override;
    auto size() const noexcept -> std::pair<int, int> override;

    // -- Resource factories --
    auto create_shader(ShaderType type, std::string_view source) -> Result<std::unique_ptr<Shader>> override;
    auto create_shader_program(
        std::unique_ptr<Shader> vertex_shader,
        std::unique_ptr<Shader> fragment_shader
    ) -> Result<std::unique_ptr<ShaderProgram>> override;
    auto create_material(
        std::unique_ptr<Shader> vertex_shader,
        std::unique_ptr<Shader> fragment_shader,
        std::span<const std::string> known_uniforms = {}
    ) -> Result<std::unique_ptr<Material>> override;
    auto create_material(std::shared_ptr<ShaderProgram> program)
        -> Result<std::unique_ptr<Material>> override;
    auto create_vertex_buffer(
        const VertexFormat& format,
        std::span<const std::byte> data
    ) -> Result<std::unique_ptr<VertexBuffer>> override;
    auto create_index_buffer(
        std::span<const std::byte> data,
        IndexType type
    ) -> Result<std::unique_ptr<IndexBuffer>> override;

    auto create_texture(const Image& image) -> Result<std::unique_ptr<Texture>> override;

    auto create_render_texture(uint32_t width, uint32_t height)
        -> Result<std::unique_ptr<Texture>> override;
    auto create_frame_buffer(uint32_t width, uint32_t height)
        -> Result<std::unique_ptr<FrameBuffer>> override;
    auto read_pixels(FrameBuffer& fbo)
        -> Result<ImageBuffer> override;

    // -- Drawing --
    auto draw(
        PrimitiveTopology topology,
        const VertexBuffer& vertices,
        const Material& material,
        uint32_t vertex_count,
        uint32_t start_vertex = 0
    ) -> void override;
    auto draw_indexed(
        PrimitiveTopology topology,
        const VertexBuffer& vertices,
        const IndexBuffer& indices,
        const Material& material,
        uint32_t index_count,
        uint32_t start_index = 0
    ) -> void override;

    auto fallback_material() noexcept -> Material& override;

    auto read_pixels() -> Result<ImageBuffer> override;

    auto render_ui() -> void override;

    RenderDeviceOpenGL(const RenderDeviceOpenGL&) = delete;
    auto operator=(const RenderDeviceOpenGL&) -> RenderDeviceOpenGL& = delete;
    RenderDeviceOpenGL(RenderDeviceOpenGL&&) = delete;
    auto operator=(RenderDeviceOpenGL&&) -> RenderDeviceOpenGL& = delete;

private:
    Window& window_;
    SDL_Window* sdl_window_;
    SDL_GLContext context_;
    std::unique_ptr<Material> fallback_material_;
};

} // namespace buddd::engine
