#pragma once

#include "error.h"

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace buddd::engine {

class Shader;
class Material;
class VertexBuffer;
class IndexBuffer;
enum class ShaderType;
enum class PrimitiveTopology;
enum class IndexType;
struct VertexFormat;
class Window;

class RenderDevice {
public:
    static auto create(Window& window) -> Result<std::unique_ptr<RenderDevice>>;

    virtual ~RenderDevice() = default;

    virtual auto begin_frame() -> void = 0;
    virtual auto end_frame() -> void = 0;
    virtual auto size() const noexcept -> std::pair<int, int> = 0;

    // -- Resource factories --
    virtual auto create_shader(ShaderType type, std::string_view source) -> Result<std::unique_ptr<Shader>> = 0;
    virtual auto create_material(
        std::unique_ptr<Shader> vertex_shader,
        std::unique_ptr<Shader> fragment_shader,
        std::span<const std::string> known_uniforms = {}
    ) -> Result<std::unique_ptr<Material>> = 0;
    virtual auto create_vertex_buffer(
        const VertexFormat& format,
        std::span<const std::byte> data
    ) -> Result<std::unique_ptr<VertexBuffer>> = 0;
    virtual auto create_index_buffer(
        std::span<const std::byte> data,
        IndexType type
    ) -> Result<std::unique_ptr<IndexBuffer>> = 0;

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

    RenderDevice(const RenderDevice&) = delete;
    auto operator=(const RenderDevice&) -> RenderDevice& = delete;
    RenderDevice(RenderDevice&&) = delete;
    auto operator=(RenderDevice&&) -> RenderDevice& = delete;

protected:
    RenderDevice() = default;
};

} // namespace buddd::engine
