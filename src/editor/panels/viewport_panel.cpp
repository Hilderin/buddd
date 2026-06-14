#include "panels/viewport_panel.h"

#include "editor.h"
#include "render/render_device.h"
#include "render/texture.h"

#include "log/log.h"

#include <imgui.h>

BUDDD_LOG_TAG("Editor");

namespace buddd::editor {

ViewportPanel::ViewportPanel(buddd::engine::RenderDevice& device,
                             buddd::engine::World& editor_world)
    : device_(&device)
    , editor_world_(&editor_world)
    , fbo_([&]() -> std::unique_ptr<buddd::engine::FrameBuffer> {
          auto result = device.create_frame_buffer(1, 1);
          if (!result) {
              BUDDD_LOG_ERROR("ViewportPanel: failed to create initial FBO: {}",
                              buddd::engine::to_string(result.error()));
              return nullptr;
          }
          return std::move(*result);
      }())
    , render_system_(std::make_unique<buddd::engine::RenderSystem>(device, editor_world))
{
    BUDDD_LOG_DEBUG("ViewportPanel: created (initial FBO 1x1)");
}

auto ViewportPanel::id() const -> std::string_view {
    return "viewport";
}

auto ViewportPanel::title() const -> std::string_view {
    return "Viewport";
}

auto ViewportPanel::draw_ui(EditorContext const& ctx) -> void {
    // 1. Get available content area size
    auto size = ImGui::GetContentRegionAvail();
    int w = static_cast<int>(size.x);
    int h = static_cast<int>(size.y);

    // 2. Guard against zero/negative dimensions (collapsed/minimized panel)
    if (w <= 0 || h <= 0) {
        BUDDD_LOG_TRACE("ViewportPanel: skip render (panel too small: {}×{})", w, h);
        return;
    }

    // 3. Resize FBO if needed
    if (w != last_width_ || h != last_height_) {
        if (fbo_) {
            auto res = fbo_->resize(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
            if (!res) {
                BUDDD_LOG_ERROR("ViewportPanel: FBO resize failed: {}",
                                buddd::engine::to_string(res.error()));
                // Keep using previous FBO at old size — continue rendering
            } else {
                BUDDD_LOG_DEBUG("ViewportPanel: FBO resized {}×{}", w, h);
                last_width_ = w;
                last_height_ = h;
            }
        } else {
            // FBO was null (creation failed) — attempt to recreate
            auto new_fbo = device_->create_frame_buffer(
                static_cast<uint32_t>(w), static_cast<uint32_t>(h));
            if (new_fbo) {
                fbo_ = std::move(*new_fbo);
                last_width_ = w;
                last_height_ = h;
                BUDDD_LOG_DEBUG("ViewportPanel: FBO recreated {}×{} after previous failure", w, h);
            } else {
                BUDDD_LOG_ERROR("ViewportPanel: failed to recreate FBO: {}",
                                buddd::engine::to_string(new_fbo.error()));
                ImGui::Text("Viewport error: failed to create framebuffer");
                return;
            }
        }
    }

    if (!fbo_) {
        ImGui::Text("Viewport error: framebuffer unavailable");
        return;
    }

    // 4. Compute aspect ratio and guard against invalid values
    float aspect = static_cast<float>(last_width_) / static_cast<float>(last_height_);
    if (aspect <= 0.0f) {
        BUDDD_LOG_TRACE("ViewportPanel: skip render (invalid aspect: {})", aspect);
        return;
    }

    // 5. Compute view-projection matrix
    auto vp = camera_.view_projection(aspect);

    // 6. Verify world pointer still valid (may change on Editor::new_scene())
    auto& active_world = ctx.editor.world();
    if (&active_world != editor_world_) {
        // The Editor replaced its World — recreate RenderSystem bound to the new world
        editor_world_ = &active_world;
        render_system_ = std::make_unique<buddd::engine::RenderSystem>(*device_, *editor_world_);
        BUDDD_LOG_DEBUG("ViewportPanel: re-bound RenderSystem to new editor World");
    }

    // 7. Render into FBO
    BUDDD_LOG_TRACE("ViewportPanel: rendering scene with camera (aspect {})", aspect);
    render_system_->render_scene_with_camera(*fbo_, vp, camera_.position);

    // 8. Display via ImGui::Image
    // Flip UV vertically: OpenGL FBO textures have origin at bottom-left,
    // while ImGui expects top-left. UV (0,1)->(1,0) corrects the Y-flip.
    uint32_t tex_id = fbo_->color_texture().gl_handle();
    ImGui::Image(static_cast<ImTextureID>(tex_id),
                 ImVec2(static_cast<float>(last_width_), static_cast<float>(last_height_)),
                 ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
}

} // namespace buddd::editor
