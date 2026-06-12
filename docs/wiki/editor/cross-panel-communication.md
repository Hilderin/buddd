# Cross-Panel Communication

> **Current status (v1 foundation — editor-foundation + F-03 — entity-selection-multi-select + F-04 — entity-operations, June 2026):** This document describes the **north-star vision** for cross-panel communication (entity selection flow, play mode state transitions, visual mode indicators). F-03 partially implements **entity selection flow**: clicking an entity in the Scene Panel (Hierarchy) now updates the `EditorSelection` state, and selected entities are highlighted in the tree via `ImGuiTreeNodeFlags_Selected`. No downstream consumers exist yet (Inspector, Viewport updates deferred to F-05/F-07). F-04 implements **entity operations** (Create, Delete, Rename) via Commands that use `EditorContext` to access the World and selection, with snapshot/restore pattern for undo/redo. Keyboard shortcuts are dispatched via `ShortcutRegistry` and gated by `WantCaptureKeyboard`.

## Future vision (north-star)

Editor panels communicate through entity selection events and editor mode state transitions. The primary interaction paths are entity selection (propagating from Hierarchy to Inspector and Viewport) and play mode state transitions (Edit → Play → Pause → Stop), which toggle panel interactivity modes.

## How it works

### Entity Selection Flow

When a user clicks an entity in the Hierarchy panel, a cascade of updates propagates across panels:

```
User clicks entity in Hierarchy
    │
    ├──▶ Inspector updates to show entity components
    ├──▶ Viewport highlights entity (outline/axes)
    ├──▶ Viewport shows transform gizmo on entity
    └──▶ Hierarchy highlights selected row

User manipulates gizmo in Viewport (future: viewport picking)
    │
    ├──▶ Inspector updates Transform fields in real-time
    └──▶ (future) Hierarchy scrolls to show entity
```

**Deselection:** Clicking empty area in the Hierarchy clears the selection. The Inspector shows "No entity selected", the Viewport hides the gizmo, and entity highlighting is removed.

### MVP1 Selection Paths

| Path | Status | Description |
|---|---|---|---|
| Hierarchy click → EditorSelection | ✅ Implemented (F-03) | Clicking an entity in the hierarchy sets the selection state. Selection highlighting via `ImGuiTreeNodeFlags_Selected`. Multi-select with Ctrl+click toggle, Shift+click range, Ctrl+A select all, empty-area click clears. Accessible via `ctx.editor.selection()`. |
| Hierarchy context menu → Entity CRUD | ✅ Implemented (F-04) | Right-click entity shows "Create Empty", "Delete", "Rename". Right-click empty area shows "Create Empty". Delete key and F2 also trigger operations. All via Commands with undo/redo. |
| Hierarchy click → Inspector + Viewport | ⏳ Deferred (F-05/F-07) | Selecting an entity in the hierarchy does NOT yet update the Inspector or Viewport — those are future features. |
| Gizmo interaction → Inspector Transform fields | ✅ Fully implemented | Dragging a translate gizmo arrow updates Position fields in the Inspector in real-time. |
| Viewport click-to-select (mouse picking) | ❌ Deferred to post-MVP1 | Clicking an entity in the 3D viewport to select it is not available in MVP1. |

### Play Mode State Transitions

```
Edit Mode (idle)
    │
    │ User presses ▶ Play
    ▼
Transition: Clone World → Start game loop → Open Game tab
    │
    ▼
Play Mode (active)
    │
    ├── User presses ⏸ Pause
    │       │
    │       ▼
    │   Paused (game loop frozen, inspection possible)
    │       │
    │       ├── User presses ▶ Play → resumes
    │       └── User presses ⏹ Stop → transition to Edit Mode
    │
    └── User presses ⏹ Stop
            │
            ▼
Transition: Stop game loop → Discard clone → Restore editor World → Close Game tab
            │
            ▼
Edit Mode (idle)
```

### Panel State Per Mode

| Panel | Edit Mode | Play Mode | Paused |
|---|---|---|---|
| Scene Panel (Hierarchy) | Editable (select, create, delete, rename) | Read-only (select only, view runtime entities) | Read-only (frozen) |
| Inspector | Editable (all fields, add/remove components) | Read-only (view only, visual indicator) | Read-only (frozen) |
| Viewport (Scene tab) | Editor camera, gizmo visible | Editor camera (no gizmo, view runtime) | Editor camera (frozen world) |
| Game Viewport | Not visible / placeholder | Active (game camera, receives input) | Frozen (last frame shown) |
| Console | Active (editor logs) | Active (game + editor logs) | Active |
| Project Panel | Interactive | Interactive | Interactive |
| Assets Panel | Interactive | Interactive | Interactive |
| Toolbar Play/Pause/Stop | Play enabled, Stop/Pause disabled | Play disabled, Stop/Pause enabled | Play/Stop enabled, Pause disabled |
| Edit > Undo/Redo | Enabled (when undo stack non-empty) | Disabled (runtime world read-only) | Disabled |

### Visual Play Mode Indicator

When the editor enters Play mode, the following visual indicators are activated:

| Element | Change |
|---|---|
| Viewport border | Turns `#FF3300` red, 3px thick, drawn around inner edge of viewport rect |
| Window title bar | Prefixed with `[Playing]` (e.g., `[Playing] Buddd Editor — scene_01.yaml`) |
| Status bar | Shows "🔴 PLAY MODE" on dark-red background at bottom of editor window |
| Inspector fields | Grayed-out text background, non-interactive |
| Inspector component headers | Lock icon (🔒) on each component section |
| Inspector Add/Remove buttons | Hidden or disabled |
| Toolbar Play button | Disabled |
| Toolbar Pause/Stop buttons | Enabled |

On Stop, all indicators are removed and panels return to Edit mode state.

## Important conventions

### v1 foundation (currently implemented)
- All editor actions go through the `Command`/`CommandStack` system — `execute()` pushes to undo stack, `undo()`/`redo()` navigate history. Undo/Redo are direct `CommandStack` calls (not undoable commands).
- Keyboard shortcuts are processed via `ShortcutRegistry::process()` in `Editor::update()`, gated by `ImGui::GetIO().WantCaptureKeyboard`.
- The About popup is rendered via `ImGui::BeginPopupModal()` — a modal popup that blocks interaction with other windows until dismissed.
- The editor has a two-phase lifecycle: `update()` for logic, `draw_ui()` for rendering.
- **F-03 (Entity Selection):** `Editor` owns an `EditorSelection` manager, accessible via `editor.selection()`. Scene Panel handles entity clicks with modifier-key awareness (plain → Replace, Ctrl → Toggle, Shift → range). Selected entities highlighted via `ImGuiTreeNodeFlags_Selected`. `EditorSelection` provides `snapshot()`/`restore()` for Command undo/redo integration. Selection cleared on `new_scene()`/`open_scene()`. See [F-03 spec](/.specs/sprint-2026-06/entity-selection/spec.md).
- **F-04 (Entity Operations):** `Command::execute()`/`undo()` signatures changed to accept `EditorContext const&`, giving Commands access to the Editor, World, and selection state. `CommandStack` forwards the context through. `Editor` gains `command_stack()` accessor. Three new Commands in `src/editor/commands/`: `CreateEntityCommand`, `DeleteEntityCommand`, `RenameEntityCommand`. Each captures a selection snapshot before mutation and restores it on undo. Commands execute during `draw_ui()` (triggered by context menu or keyboard); entities are physically removed the next frame via `World::flush_destroyed()` in `Editor::update()`. See [F-04 spec](/.specs/sprint-2026-06/entity-operations/spec.md).

### North-star (future — not yet implemented)
- Read-only enforcement during Play mode is at the **editor UI level** (fields disabled, buttons hidden), not at the engine API level.
- The editor World is never modified during Play mode — a **clone** is used for the runtime World.
- Console messages persist across all mode transitions (Edit → Play → Pause → Stop → Edit). Never auto-cleared.
- Undo/Redo is disabled during Play and Pause modes (runtime world is read-only, editor world is untouched).
- Closing the Game tab during Play is equivalent to pressing Stop.

## Related specs

- [SPEC-028 — Editor Foundation](/.specs/sprint-2026-06/editor-foundation/spec.md) — Command system, menus, shortcuts, panels, docking persistence (v1 foundation, current)
- [SPEC-2026-06 — Editor UX Design (North-Star)](/.specs/sprint-2026-06/editor-ux-design/spec.md) — Complete editor UX design document (Cross-Panel Communication, Play mode sections — future vision)

## Related ADRs

- [ADR-029](/docs/adr/ADR-029-editor-ux-decisions.md) — Editor UX decisions (panel layout, tab system, play mode)
- [ADR-027](/docs/adr/ADR-027-editor-architecture.md) — Editor architecture: separate static library, EditorApp adapter, namespace, architecture boundary
- [ADR-026](/docs/adr/ADR-026-imgui-integration.md) — ImGui integration (docking branch, SDL3 + OpenGL3 backends)

## Last reviewed

2026-06-12 — Updated for editor-foundation v1 (SPEC-028): marked as north-star future vision. Updated for F-04: Command signature change (EditorContext), entity operations selection interaction, flush_destroyed lifecycle.
