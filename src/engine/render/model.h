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

namespace buddd::engine {

// Forward declaration — RenderDevice& appears in factory method parameters.
class RenderDevice;

class Model {
public:
    // -- Null model (draw is no-op) --
    Model() noexcept = default;

    // -- Factory methods --
    /// Creates a non-indexed Model.
    /// On failure, returns an Error describing the failure.
    /// The material is shared (not owned exclusively) via shared_ptr.
    [[nodiscard]] static auto create(
        RenderDevice& device,
        const VertexFormat& vertex_format,
        std::span<const std::byte> vertex_data,
        std::shared_ptr<Material> material,
        PrimitiveTopology topology = PrimitiveTopology::Triangles
    ) -> Result<Model>;

    /// Creates an indexed Model.
    [[nodiscard]] static auto create_indexed(
        RenderDevice& device,
        const VertexFormat& vertex_format,
        std::span<const std::byte> vertex_data,
        std::span<const std::byte> index_data,
        IndexType index_type,
        std::shared_ptr<Material> material,
        PrimitiveTopology topology = PrimitiveTopology::Triangles
    ) -> Result<Model>;

    // -- Drawing --
    /// Issues one draw call (indexed or non-indexed depending on
    /// whether an index buffer was provided at creation).
    /// No-op on a moved-from (null) model.
    /// Behaviour is undefined if called outside begin_frame()/end_frame().
    auto draw(RenderDevice& device) const -> void;

    // -- Accessors --
    auto material() noexcept -> Material&;
    auto material() const noexcept -> const Material&;
    auto vertices() const noexcept -> const VertexBuffer&;
    auto indices() const noexcept -> const IndexBuffer&;
    auto has_indices() const noexcept -> bool;
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
        std::shared_ptr<Material> material,
        PrimitiveTopology topology,
        uint32_t vertex_count,
        uint32_t index_count
    ) noexcept;

    std::unique_ptr<VertexBuffer> vb_;
    std::unique_ptr<IndexBuffer> ib_;
    std::shared_ptr<Material> material_;
    PrimitiveTopology topology_{PrimitiveTopology::Triangles};
    uint32_t vertex_count_{0};
    uint32_t index_count_{0};
};

static_assert(!std::is_copy_constructible_v<Model>,
    "Model must be non-copyable");
static_assert(std::is_move_constructible_v<Model>,
    "Model must be movable");

} // namespace buddd::engine
