#pragma once

#include "render_device.h"
#include "shader.h"
#include "material.h"
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
    RenderDeviceHeadless(int width, int height);
    ~RenderDeviceHeadless() override = default;

    auto begin_frame() -> void override;
    auto end_frame() -> void override;
    auto size() const noexcept -> std::pair<int, int> override;

    // -- Resource factories --
    auto create_shader(ShaderType type, std::string_view source) -> Result<std::unique_ptr<Shader>> override;
    auto create_material(
        std::unique_ptr<Shader> vertex_shader,
        std::unique_ptr<Shader> fragment_shader,
        std::span<const std::string> known_uniforms = {}
    ) -> Result<std::unique_ptr<Material>> override;
    auto create_vertex_buffer(
        const VertexFormat& format,
        std::span<const std::byte> data
    ) -> Result<std::unique_ptr<VertexBuffer>> override;
    auto create_index_buffer(
        std::span<const std::byte> data,
        IndexType type
    ) -> Result<std::unique_ptr<IndexBuffer>> override;

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

    RenderDeviceHeadless(const RenderDeviceHeadless&) = delete;
    auto operator=(const RenderDeviceHeadless&) -> RenderDeviceHeadless& = delete;
    RenderDeviceHeadless(RenderDeviceHeadless&&) = delete;
    auto operator=(RenderDeviceHeadless&&) -> RenderDeviceHeadless& = delete;

private:
    int width_;
    int height_;

    // Headless state tracking (optional — for diagnostics)
    int shader_count_{0};
    int material_count_{0};
    int vertex_buffer_count_{0};
    int index_buffer_count_{0};
    int draw_call_count_{0};
};

} // namespace buddd::engine
