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

private:
    RenderDevice* device_;
    World* world_;
};

} // namespace buddd::engine
