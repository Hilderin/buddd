#pragma once

#include "math/camera.h"
#include "scene/component.h"

namespace buddd::engine {

class CameraComponent : public Component {
public:
    CameraComponent() = default;
    explicit CameraComponent(const math::Camera& camera);

    auto camera() noexcept -> math::Camera&;
    auto camera() const noexcept -> const math::Camera&;

    // -- Lifecycle --
    auto on_attach() -> void override;
    ~CameraComponent() override;

private:
    math::Camera camera_;
};

} // namespace buddd::engine
