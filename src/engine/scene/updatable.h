#pragma once

namespace buddd::engine {

class EngineContext;

/// Pure abstract interface for per-frame update logic.
/// Components can inherit from both Component and Updatable via multiple
/// inheritance. Updatable is orthogonal to Component — it does not depend on or
/// inherit from Component.
class Updatable {
public:
    virtual ~Updatable() = default;

    /// Called once per frame before app.render().
    /// @param ctx  Engine context providing services, window, and delta time.
    virtual auto update(const EngineContext& ctx) -> void = 0;
};

} // namespace buddd::engine
