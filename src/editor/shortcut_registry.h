#pragma once

#include "engine_context.h"
#include "engine_service.h"
#include "platform/platform.h"
#include "input/key_code.h"
#include "input/input_system.h"

#include <functional>
#include <vector>

namespace buddd::editor {

/// Binds keyboard shortcuts to callback actions.
/// Processed via process() each frame using the engine InputSystem.
class ShortcutRegistry {
public:
    using Action = std::function<void(buddd::engine::EngineContext const&)>;

    struct Modifiers {
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
    };

    /// Register a shortcut. `action` is called with the current EngineContext when the combo is pressed.
    auto bind(buddd::engine::KeyCode key, Modifiers mods, Action action) -> void;

    /// Process all bindings using the engine InputSystem.
    /// `ctx` is forwarded to each action callback. `want_capture` gates processing.
    /// Shortcuts with any modifier (Ctrl/Shift/Alt) bypass the gate — they are treated
    /// as global/application shortcuts and fire even when ImGui captures keyboard input.
    auto process(buddd::engine::EngineContext const& ctx, bool want_capture) -> void {
        auto const& input = ctx.services.platform().input_system();

        for (auto const& b : bindings_) {
            // Check modifiers (both left/right variants)
            bool ctrl_down = input.is_down(buddd::engine::KeyCode::ControlLeft)
                          || input.is_down(buddd::engine::KeyCode::ControlRight);
            bool shift_down = input.is_down(buddd::engine::KeyCode::ShiftLeft)
                           || input.is_down(buddd::engine::KeyCode::ShiftRight);
            bool alt_down = input.is_down(buddd::engine::KeyCode::AltLeft)
                         || input.is_down(buddd::engine::KeyCode::AltRight);

            if (b.mods.ctrl != ctrl_down) continue;
            if (b.mods.shift != shift_down) continue;
            if (b.mods.alt != alt_down) continue;

            bool has_modifier = ctrl_down || shift_down || alt_down;

            // Gate: only block shortcuts WITHOUT modifiers when ImGui captures input.
            // This allows Ctrl+Z, Ctrl+Q, etc. to work even inside ImGui windows.
            if (want_capture && !has_modifier) {
                continue;
            }

            // Action key: edge-triggered
            if (input.is_pressed(b.key)) {
                b.action(ctx);
            }
        }
    }

private:
    struct Binding {
        buddd::engine::KeyCode key;
        Modifiers mods;
        Action action;
    };
    std::vector<Binding> bindings_;
};

inline auto ShortcutRegistry::bind(buddd::engine::KeyCode key, Modifiers mods, Action action) -> void {
    bindings_.push_back({key, mods, std::move(action)});
}

} // namespace buddd::editor
