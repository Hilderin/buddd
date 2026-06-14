#pragma once

#include "math/mat4.h"
#include "math/vec3.h"

namespace buddd::engine {

class RenderDevice;
class World;
class FrameBuffer;

class RenderSystem {
public:
    RenderSystem(RenderDevice& device, World& world);

    /// Renders one frame. Must be called once per frame.
    /// Behaviour is undefined if called re-entrantly or from within
    /// a World::each() callback.
    auto render() -> void;

    /// Renders one frame's worth of the scene WITHOUT begin_frame()/end_frame().
    /// The caller is responsible for framing. Same rendering logic as render(),
    /// but does not call begin_frame() or end_frame().
    /// Behaviour is undefined if called outside a begin_frame()/end_frame() pair.
    auto render_scene() -> void;

    /// Renders the scene into the specified FBO.
    /// Binds the FBO before rendering, unbinds it after.
    /// Behaviour is undefined if called from within a render_scene() call.
    /// @param target The FBO to render into.
    auto render_scene(FrameBuffer& target) -> void;

    /// Render the scene using an explicit camera (view-projection matrix and position).
    /// Binds the target FBO before rendering, clears it, and unbinds it after.
    /// The camera parameters are provided explicitly — no CameraComponent lookup is performed.
    /// @param target      The FrameBuffer to render into.
    /// @param vp          Combined view-projection matrix (projection * view).
    /// @param camera_pos  Camera world-space position (for lighting uniforms).
    /// Behaviour is undefined if called from within a render_scene() call.
    auto render_scene_with_camera(FrameBuffer& target, math::Mat4 const& vp,
                                  math::Vec3 const& camera_pos) -> void;

private:
    /// Shared implementation: collects lights and iterates MeshRenderers
    /// using the given view-projection matrix and camera position.
    /// Does NOT bind/unbind any framebuffer — the caller is responsible.
    auto render_impl(math::Mat4 const& vp, math::Vec3 const& camera_pos) -> void;

    RenderDevice* device_;
    World* world_;
};

} // namespace buddd::engine
