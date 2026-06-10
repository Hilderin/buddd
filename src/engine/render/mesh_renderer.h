#pragma once

#include "render/model.h"
#include "scene/component.h"

#include <memory>

namespace buddd::engine {

class MeshRenderer : public Component {
public:
    MeshRenderer() = default;
    explicit MeshRenderer(std::shared_ptr<Model> model);

    auto model() noexcept -> Model&;
    auto model() const noexcept -> const Model&;

    /// Returns the underlying shared_ptr<Model> directly.
    [[nodiscard]] auto model_ptr() const noexcept -> const std::shared_ptr<Model>&;

    /// Sets the model from a shared_ptr.
    auto set_model(std::shared_ptr<Model> model) -> void;

private:
    std::shared_ptr<Model> model_;
};

} // namespace buddd::engine
