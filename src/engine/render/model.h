#pragma once

#include "error.h"
#include "render/index_buffer.h"
#include "render/material.h"
#include "render/primitive_topology.h"
#include "render/vertex_buffer.h"
#include "render/vertex_format.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace buddd::engine {

// Forward declaration — RenderDevice& appears in factory method parameters.
class RenderDevice;

struct SubMesh {
    uint32_t index_start;
    uint32_t index_count;
    uint32_t material_index;
};

class Model {
public:
    // -- Null model (draw is no-op) --
    Model() noexcept = default;

    // -- Factory (the only one) --
    /// Creates an indexed Model with submeshes and materials.
    /// All geometry and material data is specified upfront.
    /// Returns InvalidArgument on empty vertex data, empty index data,
    /// or invalid vertex format.
    [[nodiscard]] static auto create_indexed(
        RenderDevice& device,
        const VertexFormat& vertex_format,
        std::span<const std::byte> vertex_data,
        std::span<const std::byte> index_data,
        IndexType index_type,
        std::vector<SubMesh> submeshes,
        std::vector<std::shared_ptr<Material>> materials,
        PrimitiveTopology topology = PrimitiveTopology::Triangles
    ) -> Result<Model>;

    // -- Drawing --
    /// Issues one draw_indexed call per SubMesh.
    /// Each call binds materials_[submesh.material_index] (or fallback if null/out-of-bounds).
    /// No-op on a moved-from (null) model or if submeshes is empty.
    auto draw(RenderDevice& device) const -> void;

    // -- Accessors --
    auto submeshes() const noexcept -> const std::vector<SubMesh>&;
    auto materials() const noexcept -> const std::vector<std::shared_ptr<Material>>&;
    auto vertices() const noexcept -> const VertexBuffer&;
    auto indices() const noexcept -> const IndexBuffer&;
    auto vertex_count() const noexcept -> uint32_t;
    auto index_count() const noexcept -> uint32_t;

    // -- Lifecycle --
    Model(const Model&) = delete;
    auto operator=(const Model&) -> Model& = delete;
    Model(Model&& other) noexcept;
    auto operator=(Model&& other) noexcept -> Model&;
    ~Model() = default;

private:
    Model(
        std::unique_ptr<VertexBuffer> vb,
        std::unique_ptr<IndexBuffer> ib,
        std::vector<SubMesh> submeshes,
        std::vector<std::shared_ptr<Material>> materials,
        PrimitiveTopology topology,
        uint32_t vertex_count,
        uint32_t index_count
    ) noexcept;

    std::unique_ptr<VertexBuffer> vb_;
    std::unique_ptr<IndexBuffer> ib_;
    std::vector<SubMesh> submeshes_;
    std::vector<std::shared_ptr<Material>> materials_;
    PrimitiveTopology topology_{PrimitiveTopology::Triangles};
    uint32_t vertex_count_{0};
    uint32_t index_count_{0};
};

static_assert(!std::is_copy_constructible_v<Model>,
    "Model must be non-copyable");
static_assert(std::is_move_constructible_v<Model>,
    "Model must be movable");

} // namespace buddd::engine
