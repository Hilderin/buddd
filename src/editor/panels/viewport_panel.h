#pragma once

#include "editor_panel.h"
#include "editor_context.h"

#include "render/render_system.h"
#include "render/frame_buffer.h"
#include "math/mat4.h"
#include "math/vec3.h"

#include <memory>
#include <string_view>

namespace buddd::editor {

class ViewportPanel final : public EditorPanel {
public:
    explicit ViewportPanel(buddd::engine::RenderDevice& device,
                           buddd::engine::World& editor_world);

    [[nodiscard]] auto id() const -> std::string_view override;
    [[nodiscard]] auto title() const -> std::string_view override;
    auto draw_ui(EditorContext const& ctx) -> void override;

private:
    struct ViewportCamera {
        buddd::engine::math::Vec3 position{3.0f, 3.0f, 3.0f};
        buddd::engine::math::Vec3 target{0.0f, 0.0f, 0.0f};
        buddd::engine::math::Vec3 up{0.0f, 1.0f, 0.0f};
        float fov_y = 1.0471975512f;  // 60° in radians (π/3)
        float near_plane = 0.1f;
        float far_plane = 100.0f;

        [[nodiscard]] auto view_projection(float aspect) const noexcept -> buddd::engine::math::Mat4 {
            if (aspect <= 0.0f) return buddd::engine::math::Mat4::identity();
            auto proj = buddd::engine::math::Mat4::perspective(fov_y, aspect, near_plane, far_plane);
            auto view = buddd::engine::math::Mat4::look_at(position, target, up);
            return proj * view;
        }
    };

    buddd::engine::RenderDevice* device_;
    buddd::engine::World* editor_world_;
    std::unique_ptr<buddd::engine::FrameBuffer> fbo_;
    std::unique_ptr<buddd::engine::RenderSystem> render_system_;
    ViewportCamera camera_;
    int last_width_ = 0;
    int last_height_ = 0;
};

} // namespace buddd::editor
