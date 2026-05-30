#include "render/model.h"
#include "render/render_device.h"

#include <iostream>

namespace be = buddd::engine;

be::Model::Model(
    std::unique_ptr<VertexBuffer> vb,
    std::unique_ptr<IndexBuffer> ib,
    std::shared_ptr<Material> material,
    PrimitiveTopology topology,
    uint32_t vertex_count,
    uint32_t index_count
) noexcept
    : vb_(std::move(vb))
    , ib_(std::move(ib))
    , material_(std::move(material))
    , topology_(topology)
    , vertex_count_(vertex_count)
    , index_count_(index_count)
{}

auto be::Model::create(
    RenderDevice& device,
    const VertexFormat& vertex_format,
    std::span<const std::byte> vertex_data,
    std::shared_ptr<Material> material,
    PrimitiveTopology topology
) -> Result<Model>
{
    // Validate arguments
    if (vertex_data.size() == 0) {
        return make_error(Error::Category::InvalidArgument, "Vertex data is empty");
    }
    if (vertex_format.stride == 0) {
        return make_error(Error::Category::InvalidArgument, "Vertex format stride must be positive");
    }
    if (vertex_format.attributes.empty()) {
        return make_error(Error::Category::InvalidArgument, "Vertex format must have at least one attribute");
    }

    // Create vertex buffer
    auto vb = device.create_vertex_buffer(vertex_format, vertex_data);
    if (!vb) {
        return std::unexpected(vb.error());
    }

    // Compute vertex count
    auto vcount = static_cast<uint32_t>(vertex_data.size() / vertex_format.stride);

    // Construct and return Model
    return Model(std::move(*vb), nullptr, std::move(material), topology, vcount, 0);
}

auto be::Model::create_indexed(
    RenderDevice& device,
    const VertexFormat& vertex_format,
    std::span<const std::byte> vertex_data,
    std::span<const std::byte> index_data,
    IndexType index_type,
    std::shared_ptr<Material> material,
    PrimitiveTopology topology
) -> Result<Model>
{
    // Validate arguments
    if (vertex_data.size() == 0) {
        return make_error(Error::Category::InvalidArgument, "Vertex data is empty");
    }
    if (vertex_format.stride == 0) {
        return make_error(Error::Category::InvalidArgument, "Vertex format stride must be positive");
    }
    if (vertex_format.attributes.empty()) {
        return make_error(Error::Category::InvalidArgument, "Vertex format must have at least one attribute");
    }
    if (index_data.size() == 0) {
        return make_error(Error::Category::InvalidArgument, "Index data is empty");
    }

    // Create vertex buffer
    auto vb = device.create_vertex_buffer(vertex_format, vertex_data);
    if (!vb) {
        return std::unexpected(vb.error());
    }

    // Create index buffer
    auto ib = device.create_index_buffer(index_data, index_type);
    if (!ib) {
        return std::unexpected(ib.error());
    }

    // Compute counts
    auto vcount = static_cast<uint32_t>(vertex_data.size() / vertex_format.stride);
    auto icount = static_cast<uint32_t>(
        index_data.size() / (index_type == IndexType::Uint16 ? sizeof(uint16_t) : sizeof(uint32_t)));

    // Construct and return Model
    return Model(std::move(*vb), std::move(*ib), std::move(material), topology, vcount, icount);
}

auto be::Model::draw(RenderDevice& device) const -> void {
    if (!vb_ || !material_) {
        return;  // null/moved-from model: no-op
    }

    if (ib_) {
        device.draw_indexed(topology_, *vb_, *ib_, *material_, index_count_, 0);
    } else {
        device.draw(topology_, *vb_, *material_, vertex_count_, 0);
    }
}

auto be::Model::material() noexcept -> Material& { return *material_; }
auto be::Model::material() const noexcept -> const Material& { return *material_; }
auto be::Model::vertices() const noexcept -> const VertexBuffer& { return *vb_; }
auto be::Model::indices() const noexcept -> const IndexBuffer& { return *ib_; }
auto be::Model::has_indices() const noexcept -> bool { return ib_ != nullptr; }
auto be::Model::vertex_count() const noexcept -> uint32_t { return vertex_count_; }
auto be::Model::index_count() const noexcept -> uint32_t { return index_count_; }

be::Model::Model(Model&& other) noexcept
    : vb_(std::move(other.vb_))
    , ib_(std::move(other.ib_))
    , material_(std::move(other.material_))
    , topology_(other.topology_)
    , vertex_count_(other.vertex_count_)
    , index_count_(other.index_count_)
{
    other.vertex_count_ = 0;
    other.index_count_ = 0;
    other.topology_ = PrimitiveTopology::Triangles;
}

auto be::Model::operator=(Model&& other) noexcept -> Model& {
    if (this != &other) {
        vb_ = std::move(other.vb_);
        ib_ = std::move(other.ib_);
        material_ = std::move(other.material_);
        topology_ = other.topology_;
        vertex_count_ = other.vertex_count_;
        index_count_ = other.index_count_;
        other.vertex_count_ = 0;
        other.index_count_ = 0;
        other.topology_ = PrimitiveTopology::Triangles;
    }
    return *this;
}
