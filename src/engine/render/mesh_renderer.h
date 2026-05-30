#pragma once

#include "render/model.h"
#include "scene/component.h"

#include <memory>

namespace buddd::engine {

class MeshRenderer : public Component {
public:
    explicit MeshRenderer(std::shared_ptr<Model> model);

    auto model() noexcept -> Model&;
    auto model() const noexcept -> const Model&;

private:
    std::shared_ptr<Model> model_;
};

} // namespace buddd::engine
