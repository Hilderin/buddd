#include "render/mesh_renderer.h"

namespace buddd::engine {

MeshRenderer::MeshRenderer(std::shared_ptr<Model> model)
    : model_(std::move(model)) {}

auto MeshRenderer::model() noexcept -> Model& {
    return *model_;
}

auto MeshRenderer::model() const noexcept -> const Model& {
    return *model_;
}

} // namespace buddd::engine
