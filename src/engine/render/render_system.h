#pragma once

namespace buddd::engine {

class RenderDevice;
class World;

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

private:
    RenderDevice* device_;
    World* world_;
};

} // namespace buddd::engine
