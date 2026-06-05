#pragma once

#include "render_device.h"
#include "image/image_buffer.h"
#include "shader.h"
#include "material.h"
#include "render/shader_program.h"
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "primitive_topology.h"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace buddd::engine {

class RenderDeviceHeadless final : public RenderDevice {
public:
    explicit RenderDeviceHeadless(Window& window);
    ~RenderDeviceHeadless() override = default;

    auto window() noexcept -> Window& override { return window_; }

    auto begin_frame() -> void override;
    auto end_frame() -> void override;
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

    // -- Diagnostics / counters --
    auto frame_begin_count() const noexcept -> int override { return frame_begin_count_; }
    auto frame_end_count() const noexcept -> int override { return frame_end_count_; }
    auto draw_call_count() const noexcept -> int override { return draw_call_count_; }

    RenderDeviceHeadless(const RenderDeviceHeadless&) = delete;
    auto operator=(const RenderDeviceHeadless&) -> RenderDeviceHeadless& = delete;
    RenderDeviceHeadless(RenderDeviceHeadless&&) = delete;
    auto operator=(RenderDeviceHeadless&&) -> RenderDeviceHeadless& = delete;

private:
    Window& window_;

    // Headless state tracking (optional — for diagnostics)
    int shader_count_{0};
    int material_count_{0};
    int vertex_buffer_count_{0};
    int index_buffer_count_{0};
    int draw_call_count_{0};
    int frame_begin_count_{0};
    int frame_end_count_{0};

    std::unique_ptr<Material> fallback_material_;
};

} // namespace buddd::engine
