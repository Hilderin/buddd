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

auto MeshRenderer::model_ptr() const noexcept -> const std::shared_ptr<Model>& {
    return model_;
}

auto MeshRenderer::set_model(std::shared_ptr<Model> model) -> void {
    model_ = std::move(model);
}

} // namespace buddd::engine
