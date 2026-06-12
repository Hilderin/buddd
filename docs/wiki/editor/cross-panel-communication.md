# Cross-Panel Communication

## Current state

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
|---|---|---|
| Hierarchy click → Inspector + Viewport | ✅ Fully implemented | Selecting an entity in the hierarchy updates the Inspector and Viewport simultaneously. |
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

- Read-only enforcement during Play mode is at the **editor UI level** (fields disabled, buttons hidden), not at the engine API level.
- The editor World is never modified during Play mode — a **clone** is used for the runtime World.
- Console messages persist across all mode transitions (Edit → Play → Pause → Stop → Edit). Never auto-cleared.
- Undo/Redo is disabled during Play and Pause modes (runtime world is read-only, editor world is untouched).
- Closing the Game tab during Play is equivalent to pressing Stop.

## Related specs

- [SPEC-2026-06 — Editor UX Design (North-Star)](/.specs/sprint-2026-06/editor-ux-design/spec.md) — Complete editor UX design document (Cross-Panel Communication, Play mode sections)

## Related ADRs

- [ADR-029](/docs/adr/ADR-029-editor-ux-decisions.md) — Editor UX decisions (panel layout, tab system, play mode)
- [ADR-027](/docs/adr/ADR-027-editor-architecture.md) — Editor architecture: separate static library, EditorApp adapter, namespace, architecture boundary
- [ADR-026](/docs/adr/ADR-026-imgui-integration.md) — ImGui integration (docking branch, SDL3 + OpenGL3 backends)

## Last reviewed

2026-06-11 — Created from SPEC-2026-06 (Editor UX Design)
