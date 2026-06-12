# ADR-029: Editor UX Decisions — Tab System, Layouts, Play Mode

## Status

`Accepted`

## Context

The Buddd Engine gained editor scaffolding (ADR-027) with a static library, `App` lifecycle integration, and ImGui init. What remained undefined was the *user experience* — how the editor looks, how panels are organised, how tabs work, how Play mode behaves, and how the user interacts with entities, prefabs, and the console.

A north-star UX design spec was produced (`.specs/sprint-2026-06/editor-ux-design/spec.md`) covering all panels, tab types, layouts, workflows, and state transitions. This ADR records the architectural UX decisions made during that design process — the decisions that downstream feature specs, implementation contracts, and code must follow.

The following existing decisions and constraints shaped the UX design:

- **ADR-027 (Editor Architecture)**: The editor is a static library (`buddd_editor`) that consumes engine APIs. No SDL3/OpenGL/GLM headers outside `src/engine/`. ImGui init is fatal in display mode.
- **ADR-026 (Dear ImGui Integration)**: ImGui docking branch (`v1.91.8-docking`) provides panel docking, resizing, and tabbing infrastructure. Multi-viewport (`ImGuiConfigFlags_ViewportsEnable`) is not used — detached tabs use separate OS windows instead.
- **ImGui Docking API**: Layout definitions map to `ImGui::DockBuilder` node splits. Panel size persistence uses `ImGui::SaveIniSettingsToMemory` / `LoadIniSettingsFromMemory` (session-only).
- **Engine constraints**: No `World::clone()` exists yet — it is a required prerequisite for Play mode (flagged in spec as A-03). The `ComponentRegistry` and `TypeRegistry` APIs provide the introspection the Inspector depends on.
- **Architecture boundary (ADR-019)**: All editor features consume existing engine APIs; no engine code changes are required for adding new component types to the Inspector.
- **One binary, one entry point**: `buddd edit` launches the editor via the existing CLI command dispatch (ADR-027 Decision 7).

## Decision

### Decision 1: Tab-as-Editor-Context Model

The editor uses a tab-based system where each tab represents an editing context. Tab types are fixed per MVP1: **Scene**, **Prefab**, **Game** (future: Material, Model, Animation). Each tab type has a fixed, pre-defined layout of panels.

The **Scene tab** is special:
- Always present — the leftmost tab, never closeable.
- Represents the main editing context: the open scene file.
- Opens by default on editor launch.

**Rationale**: Unreal Engine's editor modes (Level Editor, Blueprint Editor, Material Editor, etc.) inspired this model. Each context has exactly the panels it needs. The Scene tab is the anchor of the editor — destroying it would leave the editor without a purpose. Making it permanent avoids edge cases (what happens when all tabs are closed?).

### Decision 2: One-Scene-at-a-Time

The editor opens **one scene at a time**. Opening a new scene (via File > Open Scene or double-clicking a `.yaml` file in the Project panel) replaces the current scene content. If the current scene is dirty, a modal save prompt appears (Save / Don't Save / Cancel).

Prefabs open in their own tab alongside the Scene tab — they do not replace the scene.

**Rationale**: This is an Unreal-like model (one persistent level), chosen over Unity's additive scene loading and Godot's per-scene tabs. The decision was made because:
- Simplifies the engine: no multi-scene support needed in the World API.
- Simplifies the UI: no multi-scene management UI (scene visibility, sub-scene hierarchy).
- Simplifies Play mode: one World to clone, one World to restore.
- Prefabs in separate tabs preserve the ability to edit reusable assets alongside the scene.

**Explicitly deferred**: Additive scene loading and multi-scene workflows are post-MVP1 goals.

### Decision 3: Fixed Layout Per Tab Type

Each tab type has a **fixed, pre-defined layout** of panels determined by its type:

| Tab Type | Panel Layout |
|---|---|
| **Scene** | Hierarchy (left, ~250px default), Viewport (center), Inspector (right, ~300px default), Bottom tab bar with Project/Console/Assets (~200px default height) |
| **Prefab** | Identical to Scene layout, but toolbar lacks Play/Pause/Stop, and saving writes to the prefab's `.yaml` |
| **Game** | Full-area game viewport with a status bar (~24px) |

Users **can**:
- Resize panels by dragging panel borders within the tab's dockspace.
- Panel sizes are remembered per tab type for the **duration of the editor session** only (cross-session persistence deferred).

Users **cannot**:
- Add or remove panels from a tab type's layout.
- Change which panels appear in which tab type.

**Rationale**: Standard ImGui docking behavior (tabbing together, splitting, floating) is available within the fixed panel set. The fixed structure ensures a consistent, predictable workspace — every Scene tab has the same panels, every Game tab has the same layout. Custom layout (adding/removing panels) is deferred as a post-MVP1 power-user feature. Session-only persistence avoids the complexity of saving/loading layout state to disk for MVP1.

### Decision 4: Detached Tabs as Separate OS Windows

Any tab can be **detached** as a new OS window:
- **Trigger**: Right-click on tab header → context menu → "Detach Tab" (drag-to-detach deferred).
- Detaching creates a new OS-level window with its own GL context and ImGui dockspace.
- The detached window contains the tab's full layout (all panels for that tab type).
- The original tab bar shows the tab as "detached" (grayed out or removed until re-attached).
- Re-attaching merges the tab back into the main editor window's tab bar.

**Rationale**: Multi-monitor workflows are a core UX requirement (Story 9 in the spec). ImGui's built-in `ViewportsEnable` flag was considered but rejected — it would couple all windows into a single GL context and require changes to the engine's `RenderDevice` abstraction. Instead, each detached tab creates a new SDL window with its own GL context, consistent with the architecture boundary (ADR-027, Decision 6: no platform/graphics headers outside `src/engine/`). The `buddd_editor` library owns window creation for detached tabs, using the same `Window` engine abstraction that the main editor window uses.

### Decision 5: Play Mode with World Cloning + Read-Only Inspection

Play mode follows this workflow:

1. User presses ▶ Play.
2. The editor **clones** the editor World via `World::clone()` (deep copy of all entities, components, hierarchy).
3. The original editor World is set aside (untouched).
4. The clone becomes the **runtime World** — the game loop runs on it.
5. A **Game tab** opens showing the game camera view.
6. The **Scene tab** switches to read-only mode:
   - Viewport shows the runtime World (no gizmo, editor camera still navigates).
   - Hierarchy shows runtime entities (selection only, no editing).
   - Inspector shows runtime component values (read-only: grayed fields, lock icons, hidden Add/Remove buttons).
7. On Stop:
   - Game loop stops, runtime World (clone) is **discarded**.
   - Editor World is **restored** unchanged.
   - Game tab closes.
   - Scene tab returns to edit mode.

**Rationale**: This is a hybrid of Unity-like debugging (inspect runtime entities) and Unreal-like safety (clone + discard). The clone approach guarantees zero state leakage from gameplay — the editor World is never modified during Play. Read-only inspection in the Scene tab gives developers the same debugging capability as Unity (see what's happening in the runtime world) without the risk of accidental edits to live entities.

**Prerequisite**: `World::clone()` does not exist yet. A separate feature spec and implementation must precede any Play mode implementation (spec assumption A-03).

**Deferred**: "Apply runtime state to scene" hot-reload (syncing selected runtime changes back to the editor world after Stop) is post-MVP1.

### Decision 6: Bottom Panel Tab Bar

The bottom area of the Scene and Prefab tab layouts contains a **tabbed panel area** with three tabs: **Project**, **Console**, **Assets**. Only one is visible at a time; the user switches by clicking the tab header.

| Tab | Purpose |
|---|---|
| **Project** | File system browser of the project directory (tree view, filtered to relevant asset types). |
| **Console** | Engine and editor log output, color-coded by severity, with channel/level filters. |
| **Assets** | Simple grid/list of asset files in `assets/` for drag-and-drop asset reference assignment. |

**Rationale**: These three utility panels share the same bottom space because they are secondary to the core editing workflow (hierarchy, viewport, inspector). The tabbed approach uses screen real estate efficiently — only one of the three is visible at a time, but all are a single click away. This matches the pattern used by Unity (Console/Project/Animation in the bottom area) and Unreal (Output Log/Content Drawer).

### Decision 7: Entity Creation as Child of Selected

When creating a new entity (right-click → "Create Empty" or + button):
- If **a single entity** is selected: the new entity becomes the **last child** of the selected entity (placed at local origin, appended at end of child list).
- If **nothing is selected or multiple entities are selected**: the new entity goes to root level (appended at end of root list).

**No name uniqueness constraint**: Entities can have duplicate names. Entity IDs are used internally for all operations; names are user-facing labels only.

**Rationale**: Child-of-selected is the most intuitive default for scene composition — the user selects where they want the entity to live in the hierarchy. Root-level fallback handles the common case of creating top-level entities. Allowing duplicate names avoids the complexity of auto-renaming and the frustration of "name already taken" errors, at the cost of potential confusion in the hierarchy panel.

### Decision 8: Prefab Editing in Tabs

Prefabs open in their **own tab** (not inline in the hierarchy) with the **same layout as the Scene tab** (hierarchy, viewport, inspector, bottom tab bar). The Prefab tab's World contains only the prefab's root entity and its descendants.

Prefab tabs **do not participate in Play mode**:
- They have no Play/Pause/Stop buttons in the toolbar (or they are disabled).
- Playing always runs the main scene (Scene tab's World), never a prefab in isolation.
- Prefab editing is purely an edit-time activity.

**Rationale**: The Scene tab layout is reused because the same editing operations (entity CRUD, component editing, transform manipulation) apply to prefabs. Isolated prefab editing avoids concurrency issues with the playing scene. Isolated Play for prefabs is deferred. Opening prefabs in tabs (rather than inline) is consistent with the "one scene at a time" model — the scene stays open in its tab alongside any number of prefab tabs.

### Decision 9: Console Persistence Across Mode Transitions

Console messages **never auto-clear** across mode transitions:
- Edit → Play: messages persist.
- Play → Pause → Stop: messages persist.
- Stop → Edit: messages persist.
- The only way to clear messages is the explicit **Clear button** (manual user action).

**Rationale**: Auto-clearing on mode transitions destroys valuable debugging information. A developer inspecting a Play-mode crash needs to see the error log after stopping. Manual clearing puts the developer in control. This is the same behavior as Unity's Console panel (persistent by default) and Unreal's Output Log.

### Decision 10: Play Mode Visual Indicator

A multi-point visual indicator signals that the editor is in Play mode:

| Element | Specification |
|---|---|
| **Viewport border** | `#FF3300` (bright red), 3px thick, drawn around inner edge of viewport rect. |
| **Window title** | `[Playing]` prefix prepended to the editor window title (e.g., `[Playing] Buddd Editor — scene_01.yaml`). |
| **Status bar** | "🔴 PLAY MODE" on dark-red background at the very bottom of the editor window. |
| **Inspector** | Read-only mode: grayed-out text background, lock icon (🔒) on component headers, "Play mode — read only" banner at top, Add/Remove Component buttons hidden. |

**Rationale**: The indicator must be unmistakable — a developer should never wonder "am I in Play mode?" The red viewport border is the primary visual cue (most prominent location), reinforced by the title prefix (visible in the taskbar/Alt+Tab), the status bar (persistent across tab switches), and the Inspector read-only state (prevents accidental editing). The `#FF3300` color was chosen for high contrast against the editor's default dark theme.

## Alternatives considered

| Design Area | Alternative | Verdict |
|---|---|---|
| **Scene management** | **Unity-like additive scenes** — Multiple scenes open simultaneously, shown in a hierarchy with visibility toggles. | **Rejected.** Increases engine and UI complexity. No multi-scene support in World API. Deferred post-MVP1. |
| **Scene management** | **Godot-like per-scene tabs** — Each opened scene gets its own tab in the tab bar. | **Rejected.** Conflicts with the single-persistent-Scene-tab model. Would require tab management for multiple scenes. |
| **Layout** | **Free-form layout** — Users can add/remove any panel from any tab type, saved per user. | **Rejected.** Increases complexity and support burden. Fixed layout per tab type is simpler for MVP1 and provides a consistent workspace. |
| **Detached tabs** | **ImGui ViewportsEnable** — Use ImGui's built-in multi-viewport support to create separate windows. | **Rejected.** Couples all windows into a single GL context. Requires changes to `RenderDevice` abstraction. Each detached tab gets its own OS window with its own GL context. |
| **Play mode** | **In-place play** — Game runs inside the viewport, no cloning, edits modify the same World. | **Rejected.** Unsafe — gameplay changes can corrupt the editor World. No reliable restore. |
| **Play mode** | **Scene tab shows editor World, Game tab shows runtime** (Unity-like). | **Rejected in favour of clone model.** Unity shows the runtime world in both Scene and Game tabs. The clone model is safer (zero state leakage). |
| **Play mode** | **Hide Scene tab during Play** (only Game tab visible). | **Rejected.** Developers need to inspect the runtime scene to debug gameplay. The Scene tab in read-only mode is a key debugging tool. |
| **Play mode visual** | **Single indicator** (e.g., just a toolbar color change). | **Rejected.** Too subtle. The multi-point indicator (border + title + status bar + Inspector) ensures the mode is unmistakable. |
| **Console** | **Auto-clear on Play** — Clear console when entering Play mode. | **Rejected.** Destroys debugging context. Manual clearing puts the developer in control. |
| **Entity creation** | **Always root level** — New entities are always created at root level. | **Rejected.** Less intuitive for building hierarchies. Users expect the new entity to be related to the selected entity. |
| **Entity naming** | **Unique names enforced** — Reject duplicate entity names, auto-rename on conflict. | **Rejected.** Adds friction without benefit. Entity IDs are the canonical identifier; names are display labels. |
| **Prefab editing** | **Inline prefab editing** — Edit prefab contents directly in the Scene hierarchy, like Unity. | **Rejected.** Inline editing introduces complexity around override tracking, instance differentiation, and revert/apply workflows. Separate prefab tabs keep the model simple for MVP1. |

## Consequences

### Positive

- **Design clarity**: Every downstream editor feature spec (hierarchy panel, inspector, viewport, Play mode, project panel, console) has a clear north-star reference. All decisions are traceable to this ADR.
- **Consistent workspace**: Fixed layouts per tab type ensure every developer sees the same panel arrangement. Reduces support burden ("my Inspector is missing").
- **Play mode safety**: Clone + discard guarantees the editor World is never modified during gameplay. Zero state leakage. Clear separation of editor and runtime concerns.
- **Multi-monitor support**: Detached tabs enable multi-monitor workflows without ImGui `ViewportsEnable`, preserving the architecture boundary and single-GL-context-per-window model.
- **Simple scene management**: One scene at a time eliminates multi-scene complexity from both the engine (World API) and the editor (UI). Prefabs in tabs provide parallel editing.
- **Developer-friendly Console**: Persistent messages mean no lost debugging context across mode transitions. The developer always has full log history.
- **Unmistakable Play mode**: The multi-point visual indicator (red border, `[Playing]` title, status bar, read-only Inspector) makes the mode state obvious at a glance.
- **Immediate entity creation feedback**: Child-of-selected placement matches user expectation for hierarchy building. No name conflicts to resolve.
- **Engine boundary preserved**: New component types appear in the Inspector automatically via `ComponentRegistry` introspection — no engine changes needed.

### Negative

- **Layout rigidity**: Users cannot add or remove panels from a tab type's layout. Customisation is deferred. Power users may find this limiting.
- **No cross-session panel persistence**: Panel sizes reset on every editor restart. Users who carefully arrange their workspace will lose their layout on exit.
- **Detached tab complexity**: Multi-window with per-window GL contexts is more complex to implement than ImGui `ViewportsEnable`. The `buddd_editor` library must own SDL window creation for detached tabs.
- **No inline prefab editing**: Opening a prefab in a separate tab disrupts the workflow of editing a prefab "in context" (seeing it alongside other scene entities). Override workflows are deferred.
- **No multi-scene workflows**: Users who want to reference content across scenes (e.g., lighting-only scene, gameplay scene) must wait for post-MVP1.
- **No Play for prefabs**: Prefab authors cannot test prefab behaviour in isolation within the editor. They must instantiate the prefab in a scene and play the scene.
- **`World::clone()` prerequisite**: Play mode cannot be implemented until the engine gains deep-clone capability. This is a blocking dependency.

### Neutral

- **Session-only panel persistence**: Session-only persistence avoids the complexity of saving/loading ImGui ini data to disk. Acceptable for MVP1. Cross-session persistence can be added later without changing the UX model.
- **Duplicate entity names allowed**: Simplifies entity creation but may cause confusion in the hierarchy panel when multiple entities share the same name. Users are expected to rename entities for clarity.
- **Game tab closes on Stop**: Users who want to compare "before and after" state across Play sessions must press Play again. A persistent Game tab with "last frame" was considered but deemed less useful than the clean slate on each Play.
- **Right-click detach only**: Drag-to-detach is deferred. The right-click context menu is discoverable and functional for MVP1. Drag-to-detach can be added without UX model changes.

## Impact on existing ADRs

- **ADR-027 (Editor Architecture)**: This ADR extends ADR-027 with the full UX model. ADR-027 covers technical architecture (static library, `App` lifecycle, ImGui init); this ADR covers the UX that the technical architecture enables. No amendment to ADR-027 is needed.
- **ADR-026 (Dear ImGui Integration)**: Confirms ADR-026's decision to use the docking branch. No amendment needed.
- **ADR-019 (Architecture Boundaries)**: The detached-tab multi-window design creates new SDL windows from within `src/editor/`, which must go through the engine's `Window` abstraction (not raw SDL3 calls). This ADR confirms the architecture boundary is preserved — no SDL3/OpenGL/GLM headers in `src/editor/`.

## Related documents

- `.specs/sprint-2026-06/editor-ux-design/spec.md`: North-star UX design spec containing detailed acceptance criteria, user stories, edge cases, and panel descriptions.
- `docs/adr/ADR-027-editor-architecture.md`: Technical editor architecture (static library, App lifecycle, ImGui init, CLI command).
- `docs/adr/ADR-026-imgui-integration.md`: Dear ImGui integration with docking branch — provides panel docking infrastructure.
- `docs/adr/ADR-019-architecture-boundaries.md`: Architecture boundary enforced in `src/editor/`.
