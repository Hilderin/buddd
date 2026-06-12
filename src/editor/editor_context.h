#pragma once

#include <cstdint>  // not strictly needed for forward declarations, but kept for consistency

namespace buddd::editor { class Editor; }
namespace buddd::engine { struct EngineContext; }

namespace buddd::editor {

/// Lightweight aggregate providing panels and menus with access to
/// both the Editor (for editor-specific state, e.g. editor.world())
/// and the EngineContext (for engine services, e.g. ctx.engine).
struct EditorContext {
    Editor& editor;
    buddd::engine::EngineContext const& engine;
};

} // namespace buddd::editor
