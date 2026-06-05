#include "render/model.h"
#include "render/render_device.h"

#include <iostream>

namespace be = buddd::engine;

be::Model::Model(
    std::unique_ptr<VertexBuffer> vb,
    std::unique_ptr<IndexBuffer> ib,
    std::vector<SubMesh> submeshes,
    std::vector<std::shared_ptr<Material>> materials,
    PrimitiveTopology topology,
    uint32_t vertex_count,
    uint32_t index_count
) noexcept
    : vb_(std::move(vb))
    , ib_(std::move(ib))
    , submeshes_(std::move(submeshes))
    , materials_(std::move(materials))
    , topology_(topology)
    , vertex_count_(vertex_count)
    , index_count_(index_count)
{}

auto be::Model::create_indexed(
    RenderDevice& device,
    const VertexFormat& vertex_format,
    std::span<const std::byte> vertex_data,
    std::span<const std::byte> index_data,
    IndexType index_type,
    std::vector<SubMesh> submeshes,
    std::vector<std::shared_ptr<Material>> materials,
    PrimitiveTopology topology
) -> Result<Model>
{
    if (vertex_data.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Vertex data is empty");
    }
    if (index_data.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Index data is empty");
    }
    if (vertex_format.stride == 0 || vertex_format.attributes.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Vertex format must have a positive stride and at least one attribute");
    }

    auto vb = device.create_vertex_buffer(vertex_format, vertex_data);
    if (!vb) {
        return std::unexpected(vb.error());
    }

    auto ib = device.create_index_buffer(index_data, index_type);
    if (!ib) {
        return std::unexpected(ib.error());
    }

    // Count vertices/indices from spans
    uint32_t vcount = static_cast<uint32_t>(vertex_data.size() / vertex_format.stride);
    auto index_size = (index_type == IndexType::Uint16) ? sizeof(uint16_t) : sizeof(uint32_t);
    uint32_t icount = static_cast<uint32_t>(index_data.size() / index_size);

    return Model(
        std::move(*vb),
        std::move(*ib),
        std::move(submeshes),
        std::move(materials),
        topology,
        vcount,
        icount
    );
}

auto be::Model::draw(RenderDevice& device) const -> void {
    if (!vb_ || !ib_ || submeshes_.empty()) {
        return;
    }

    for (const auto& sm : submeshes_) {
        const Material* mat = nullptr;
        if (sm.material_index < materials_.size()) {
            mat = materials_[sm.material_index].get();
        }
        const auto& material = mat ? *mat : device.fallback_material();
        device.draw_indexed(topology_, *vb_, *ib_, material,
            sm.index_count, sm.index_start);
    }
}

auto be::Model::submeshes() const noexcept -> const std::vector<SubMesh>& {
    return submeshes_;
}

auto be::Model::materials() const noexcept -> const std::vector<std::shared_ptr<Material>>& {
    return materials_;
}

auto be::Model::vertices() const noexcept -> const VertexBuffer& {
    return *vb_;
}

auto be::Model::indices() const noexcept -> const IndexBuffer& {
    return *ib_;
}

auto be::Model::vertex_count() const noexcept -> uint32_t {
    return vertex_count_;
}

auto be::Model::index_count() const noexcept -> uint32_t {
    return index_count_;
}

be::Model::Model(Model&& other) noexcept
    : vb_(std::move(other.vb_))
    , ib_(std::move(other.ib_))
    , submeshes_(std::move(other.submeshes_))
    , materials_(std::move(other.materials_))
    , topology_(other.topology_)
    , vertex_count_(other.vertex_count_)
    , index_count_(other.index_count_)
{
    other.topology_ = PrimitiveTopology::Triangles;
    other.vertex_count_ = 0;
    other.index_count_ = 0;
}

auto be::Model::operator=(Model&& other) noexcept -> Model& {
    if (this != &other) {
        vb_ = std::move(other.vb_);
        ib_ = std::move(other.ib_);
        submeshes_ = std::move(other.submeshes_);
        materials_ = std::move(other.materials_);
        topology_ = other.topology_;
        vertex_count_ = other.vertex_count_;
        index_count_ = other.index_count_;
        other.topology_ = PrimitiveTopology::Triangles;
        other.vertex_count_ = 0;
        other.index_count_ = 0;
    }
    return *this;
}
