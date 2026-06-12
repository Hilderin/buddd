# Editor Panels

> **Current status (F-01 — editor-scene-load-save + F-02 — scene-panel-entity-tree, June 2026):** This document describes the **north-star vision** for the editor's panel system (tabs, Play mode, prefabs, viewport, inspector, toolbar). The currently implemented foundation (F-01 + F-02) includes:
> - A **main menu bar** with three menus: **File** (New Scene, Open Scene, Save Scene, Save Scene As, Quit), **Edit** (Undo/Redo), **Help** (About → modal popup with engine version).
> - **Five dockable placeholder panels**: Scene, Properties, Console, Project, Assets — each empty with only a title bar and 100×100 minimum size.
> - **Docking persistence** via `buddd_editor.ini` (layout saved/restored between sessions).
> - **Keyboard shortcuts**: Ctrl+N (New), Ctrl+O (Open), Ctrl+S (Save), Ctrl+Shift+S (Save As), Ctrl+Q (Quit), Ctrl+Z (Undo), Ctrl+Shift+Z/Ctrl+Y (Redo), gated by `WantCaptureKeyboard`.
> - **Command system**: `Command` base class + `CommandStack` with bounded 128-entry undo/redo.
> - **Two-phase lifecycle**: `Editor::update()` (shortcuts, state) + `Editor::draw_ui()` (menus, dockspace, panels, popups).
> - **Dirty state tracking**: `*` suffix in window title when scene has unsaved changes (via `Editor::mark_dirty()` / `Editor::is_dirty()`).
> - **OS file dialogs**: SDL3 native dialogs via Platform abstraction (`.yaml` filter).
> - **Save-prompt modals**: Multi-frame modal with Save/Don't Save/Cancel for dirty-scene operations.
> - **Error modals**: Displayed on SceneLoader/SceneSaver failures.
> - **OS close interception**: Close button (X/Alt+F4) triggers same save-prompt as File > Quit.
> - **F-02 additions**: `EditorContext` aggregate struct (`src/editor/editor_context.h`) provides panels with access to both `Editor&` and `EngineContext const&`; `EditorPanel`/`EditorMenu` signatures changed from `EngineContext const&` to `EditorContext const&`; `ScenePanel` renders the entity hierarchy tree via ImGui `TreeNodeEx` (empty state, expandable/collapsible, leaf/non-leaf, `PushID`/`PopID` per-entity).
>
> The north-star design below (tabs, viewport, inspector, toolbar, Play mode, prefabs) is **planned for future sprints** and is not yet implemented.

## Future vision (north-star)

The Buddd Editor has an Unreal Engine-like tab-based editing context system. One scene is open at a time in a dedicated Scene tab, with additional tabs for Prefab editing and Game testing. Each tab type has a fixed panel layout with user-resizable dividers.

**MVP1 tab types:** Scene, Prefab, Game.

**Key architectural context:**

- The editor is a separate static library (`buddd_editor` in `src/editor/`), not part of the engine (see ADR-027).
- All platform/graphics access goes through engine abstractions — no SDL3/OpenGL/GLM headers in editor code.
- ImGui's docking branch provides panel docking, resizing, and tabbing.
- The editor launches via `buddd edit` using the existing `run_app()` infrastructure.

## How it works

### Tab System

The editor window contains a **tab bar** at the top (below the menu bar), a **menu bar** at the very top, and a **toolbar** below the menu bar. The active tab's content fills the remaining area.

```
┌──────────────────────────────────────────────────────────────┐
│ Menu Bar: File  Edit  View  Help                              │
│ Toolbar: [▶ Play] [⏸ Pause] [⏹ Stop] | [⫶ Translate] [Grid]  │
├──────────────────────────────────────────────────────────────┤
│ [🔷 Scene] [📄 Prefab_Crate] [🎮 Game]          ← Tab bar     │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│              Active tab content (dockspace)                   │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

**Tab rules:**

| Rule | Detail |
|---|---|
| Scene tab is special | Always present, leftmost, cannot be closed |
| Tab types (MVP1) | Scene, Prefab, Game |
| Tab ordering | Draggable — Scene tab always remains leftmost |
| Detached tabs | Right-click tab header → "Detach Tab" opens as separate OS window with its own GL context |
| Re-attachment | Merges the detached window back into the main editor's tab bar |
| Dirty indicator | `*` prepended to tab title when unsaved changes exist (e.g., `🔷 *Scene_01`) |

**Tab triggers:**

| Tab | Trigger |
|---|---|
| Scene | Always present on launch. Opening another scene replaces the current scene (with save prompt if dirty). |
| Prefab | Double-click a `.yaml` prefab file in the Project panel. |
| Game | Opens automatically when ▶ Play is pressed. Closes when Stop is pressed. |

---

### Scene Tab Layout

The default layout when editing a scene file. The toolbar includes Play/Pause/Stop controls and transform/grid options.

```
┌──────────────────────────────────────────────────────────┐
│ Menu Bar: File  Edit  View  Help                          │
│ Toolbar: [▶ Play] [⏸ Pause] [⏹ Stop] | [W] [Grid]        │
│ Tab Bar: [🔷 *Scene_01] [📄 Prefab_Crate]                 │
├─────────────┬────────────────────────┬───────────────────┤
│             │                        │                   │
│  Scene      │      Viewport          │   Inspector       │
│  Panel      │      (3D view)         │   Panel           │
│  (tree)     │                        │                   │
│             │    [Grid overlay]      │  Entity: "Player" │
│  ▼ Root     │    [Editor camera]     │  ─────────────── │
│    ▸ Player │    [Gizmo on selected] │  Transform        │
│      ▸ Mesh │                        │   X: 0  Y: 2  Z:0│
│    ▸ Light  │                        │  ─────────────── │
│    ▸ Camera │                        │  MeshRenderer     │
│             │                        │   Model: [...]    │
│             │                        │  ─────────────── │
│             │                        │  [+ Add Component]│
├─────────────┴────────────────────────┴───────────────────┤
│  [Project] [Console] [Assets]           ← Bottom tab bar  │
├──────────────────────────────────────────────────────────┤
│                                                           │
│  (Content of the selected tab — e.g., Console shown)      │
│  [INFO] Scene loaded: scene_01.yaml                       │
│  [WARN] Missing texture: ...                              │
│                                                           │
└──────────────────────────────────────────────────────────┘
```

**Panel structure:**

| Panel | Position | Default size | Notes |
|---|---|---|---|
| Scene Panel (Hierarchy) | Left dock | ~250px width | Entity tree view |
| Viewport | Center dock | Fills remaining space | 3D rendered view |
| Inspector | Right dock | ~300px width | Entity/component properties |
| Bottom tabs (Project / Console / Assets) | Bottom area | ~200px height | Tabbed, one visible at a time |

All panel sizes are user-resizable by dragging dividers. Sizes are remembered per tab type for the session only (lost on restart).

---

### Prefab Tab Layout

**Identical structure to the Scene tab layout** — hierarchy panel left, viewport center, inspector right, bottom area with Project/Console/Assets tabs.

**Key differences from Scene tab:**

| Difference | Detail |
|---|---|
| Hierarchy content | Shows only the prefab's entity tree (root entity + descendants) |
| Tab title | Shows the prefab filename (e.g., `📄 Prefab_Crate`) |
| Toolbar | Does **not** include Play/Pause/Stop buttons (or they are disabled/grayed out) |
| Saving | File > Save writes to the prefab's `.yaml` file via `SceneSaver` |
| Play mode | Prefab tab does **not** participate in Play mode — only the Scene tab's World is cloned |

---

### Game Tab Layout

Opens when Play is pressed. Shows the game camera view with a status overlay.

```
┌──────────────────────────────────────────────────────────┐
│ Menu Bar: File  Edit  View  Help                          │
│ Toolbar: [▶ Play] [⏸ Pause] [⏹ Stop]                      │
│ Tab Bar: [🔷 Scene_01] [🎮 Game]                          │
├──────────────────────────────────────────────────────────┤
│                                                          │
│                                                          │
│                  Game Viewport                           │
│                  (game camera, FPS view)                  │
│                                                          │
│                                                          │
│                                                          │
├──────────────────────────────────────────────────────────┤
│ Status: ▶ Playing | FPS: 60 | Entities: 42 | Time: 00:12 │
└──────────────────────────────────────────────────────────┘
```

**Panel structure:**

| Element | Position | Size | Notes |
|---|---|---|---|
| Game Viewport | Fills entire tab area | Stretches to fill | Shows the active game camera's perspective |
| Status bar | Bottom strip | ~24px, non-resizable | Shows FPS, entity count, play time, play state |

**Status bar content:** "▶ Playing" or "⏸ Paused" | FPS counter | Entity count | Play time (HH:MM:SS).

---

### Panel Reference

#### Menu Bar

Present in all tab types at the top of the editor window.

| Menu | Entries |
|---|---|
| **File** | New Scene, Open Scene, Save Scene, Save Scene As, separator, Quit |
| **Edit** | Undo, Redo, separator, Delete Selected Entity |
| **View** | Toggle panel visibility for each panel type (Scene, Inspector, Viewport, Console, Project, Assets) — checkmarks indicate visibility |
| **Help** | About Buddd Editor (version, credits) |

#### Toolbar

Present in Scene, Prefab, and Game tabs.

**Scene/Prefab tab toolbar:**
- **▶ Play** — Starts Play mode (disabled if already playing)
- **⏸ Pause** — Pauses the game loop (enabled only during Play)
- **⏹ Stop** — Stops Play mode (enabled only during Play or Pause)
- **Transform mode selector** — W (Translate active in MVP1), E (Rotate — visible but disabled), R (Scale — visible but disabled)
- **Grid toggle** — Shows/hides the ground-plane grid in the viewport

**Game tab toolbar:**
- **▶ Play, ⏸ Pause, ⏹ Stop** — same as above
- No transform or grid controls (Game tab is for runtime viewing, not editing)

#### Scene Panel (Hierarchy)

- **Appears in:** Scene tab, Prefab tab
- **Purpose:** Display and manage the entity hierarchy as a tree
- **Content (F-02 implemented):** Tree view of all entities with parent/child indentation. Root entities rendered as top-level `TreeNodeEx` nodes, children indented under parents. Empty state shows "No entities". Leaf/non-leaf distinction via `ImGuiTreeNodeFlags_Leaf`. Per-entity `PushID`/`PopID` prevents ImGui ID collisions. Entities with empty names display as "(unnamed)". Selection highlighting and entity CRUD operations deferred to future sprints (F-03, F-06+).
- **Default position/size:** Left dock, ~250px width

#### Inspector Panel

- **Appears in:** Scene tab, Prefab tab
- **Purpose:** Display and edit the selected entity's components and properties
- **Content:** Entity name field, Transform section (position editable, rotation/scale read-only in MVP1), collapsible component sections with type-appropriate editors, Add Component button, Remove Component buttons
- **Default position/size:** Right dock, ~300px width
- **Read-only mode during Play:** Grayed-out fields, lock icon banner, hidden Add/Remove buttons

##### Inspector Property Editors

The Inspector renders each component property with a type-appropriate editor widget based on its registered type in the `ComponentRegistry` and `TypeRegistry`:

| Property type | Editor widget |
|---|---|
| `bool` | Checkbox |
| `int` | Integer drag/input field |
| `float` | Float drag/input field |
| `std::string` | Text input field |
| `Vec2` | Two float fields (X, Y) |
| `Vec3` | Three float fields (X, Y, Z) |
| `Vec4` | Four float fields (X, Y, Z, W) |
| `Color` (Vec3/Vec4 interpreted as RGB/RGBA) | Color picker + float fields |
| `Entity reference` | Text field showing entity name (read-only in MVP1) |
| `Asset reference` | Text field showing asset path + drag-accept target |
| `Quat` | Euler angles (read-only in MVP1) |

The **Add Component** button at the bottom of the Inspector opens a searchable dropdown listing all registered component types — typing filters the list, clicking adds the component to the entity. Each component section has a **Remove component** button (ⓧ) on its header.

#### Viewport Panel (3D View)

- **Appears in:** Scene tab, Prefab tab
- **Purpose:** Render the scene in 3D with an editor camera for navigation and entity placement
- **Content:** 3D rendered view, ground-plane grid overlay, debug axes on selected entity, translate gizmo (MVP1)
- **Default position/size:** Center dock, fills remaining space
- **No gizmo during Play mode**

##### Editor Camera Controls

When the viewport has focus, the editor camera responds to the following inputs:

| Input | Action |
|---|---|
| Right-click + drag | Look around (yaw/pitch) |
| Right-click + W/S/A/D | Fly forward/back/strafe |
| Right-click + Q/E | Move down/up |
| Scroll wheel | Dolly forward/back |
| F key (with entity selected) | Focus camera on entity (instant snap) |
| Middle-click + drag | Pan camera |

The F key focus is an **instant snap** — the camera teleports immediately to frame the selected entity's bounding box (no animation).

#### Game Viewport

- **Appears in:** Game tab only
- **Purpose:** Display the game camera's view during Play mode
- **Content:** Full-area 3D viewport locked to the active game camera, no editor camera, no gizmo
- **Input routing:** When Game tab has focus, keyboard/mouse input is routed to the game (not the editor)

#### Project Panel

- **Appears in:** Bottom tab area (alongside Console and Assets) — shared across all tab types
- **Purpose:** File-system browsing of the project directory
- **Content:** Tree view rooted at the project working directory, filtered to relevant asset types (`.yaml`, `.gltf`/`.glb`, `.png`/`.jpg`)
- **Interactions:** Double-click `.yaml` scene → loads in Scene tab; double-click `.yaml` prefab → opens Prefab tab; right-click context menu (Open, Delete, Rename, Show in Explorer)
- **Default position/size:** Bottom area, ~200px height

#### Console Panel

- **Appears in:** Bottom tab area (alongside Project and Assets) — shared across all tab types
- **Purpose:** Display engine and editor log output
- **Content:** Scrollable log lines, color-coded by severity (Trace=gray, Info=white, Warn=yellow, Error=red, Fatal=bright red), timestamps and channel tags
- **Persistence:** Console messages persist across all mode transitions (Edit/Play/Pause/Stop). Never auto-cleared. Only the Clear button removes messages.
- **Controls:** Clear button, auto-scroll toggle, channel filter, level filter
- **Default position/size:** Bottom area, ~200px height

#### Assets Panel

- **Appears in:** Bottom tab area (alongside Project and Console) — shared across all tab types
- **Purpose:** Quick access to project assets for drag-and-drop reference assignment
- **Content (MVP1):** Flat list or simple grid of asset files in `assets/` directory; type filter buttons (All, Models, Textures, Materials); drag to asset-reference fields in Inspector
- **Default position/size:** Bottom area, ~200px height

---

### Play Mode Behavior

When ▶ Play is pressed:

1. The editor **clones** the editor World (deep copy of all entities, components, and hierarchy).
2. The original editor World is set aside (untouched).
3. The clone becomes the **runtime World** and the game loop begins.
4. A **Game tab** opens showing the game camera view.
5. Scene tab panels switch to read-only runtime mode:

| Panel | Edit Mode | Play Mode | Paused |
|---|---|---|---|
| Scene Panel (Hierarchy) | Editable (select, create, delete, rename) | Read-only (select only, view runtime entities) | Read-only (frozen) |
| Inspector | Editable | Read-only (gray fields, lock icon, hidden Add/Remove) | Read-only (frozen) |
| Viewport (Scene tab) | Editor camera, gizmo visible | Editor camera (no gizmo, view runtime) | Editor camera (frozen world) |
| Game Viewport | Not visible | Active (game camera, receives input) | Frozen |
| Console | Active | Active (game + editor logs) | Active |
| Project / Assets | Interactive | Interactive | Interactive |
| Toolbar Play/Pause/Stop | Play enabled, others disabled | Play disabled, Pause/Stop enabled | Play/Stop enabled, Pause disabled |
| Undo/Redo | Enabled | Disabled (runtime world read-only) | Disabled |

**Visual indicators during Play mode:**

- Viewport border: `#FF3300` red, 3px thick, inner edge
- Window title: prefixed with `[Playing]`
- Status bar at bottom of editor: "🔴 PLAY MODE" on dark-red background
- Inspector: grayed-out fields, lock icon (🔒) banner "Play mode — read only", hidden Add/Remove component buttons
- Toolbar: Play disabled, Pause/Stop enabled

**On Stop:** Runtime clone is discarded, editor World restored, Game tab closes, all panels return to Edit mode.

---

### Entity Operations

| Operation | Trigger | Behavior |
|---|---|---|
| **Select entity** | Left-click entity row in hierarchy | Inspector updates, viewport highlights entity + shows gizmo |
| **Deselect** | Click empty area in hierarchy | Selection clears, Inspector shows "No entity selected", gizmo hidden |
| **Create empty entity** | Right-click → "Create Empty" OR + button | Creates entity named "Entity". If single entity selected → last child. Otherwise → root level. |
| **Delete entity** | Right-click → "Delete" OR Edit menu OR Delete key | Removes entity. Children prompt confirmation dialog. Undo stack records deletion. |
| **Rename entity** | Double-click name OR select + F2 | In-place editable text field. Enter confirms, Escape cancels. Empty name rejected. Duplicates allowed. |
| **Undo delete** | Edit > Undo (Ctrl+Z) | Restores last deleted entity + children (single-level only). Disabled during Play. |
| **Add component** | "+ Add Component" button in Inspector | Opens searchable dropdown of registered component types. Selecting adds it to entity. |
| **Remove component** | ⓧ button on component header | Removes component from entity. |
| **Focus camera** | F key (entity selected) | Editor camera snaps to frame selected entity's bounding box. |
| **Translate** | Drag gizmo arrow in viewport | Moves entity along the dragged axis. Inspector Transform fields update in real-time. |

**Deferred for post-MVP1:** Multi-select, drag-reparent, duplicate entity (Ctrl+D), copy/paste, viewport click-to-select, rotate/scale gizmos.

---

### Edge Cases

| ID | Scenario | Expected behavior |
|---|---|---|
| EC-01 | Empty scene (no entities) | Hierarchy shows nothing. Viewport shows grid only. Inspector shows "No entity selected". File > Save As works. |
| EC-02 | Entity with deep hierarchy | All levels expandable/collapsible. rename, delete cascade correctly. |
| EC-03 | Scene with 10,000+ entities | Hierarchy may lag (deferred: virtualized tree). Console should not overflow. |
| EC-04 | No game camera in scene | Pressing Play still works but Game tab shows black viewport (no camera assigned). Warning in Console. |
| EC-05 | Rapid Play/Stop cycling | Each Play clones fresh World. Each Stop discards cleanly. No memory leak. |
| EC-06 | Collapsed/zero-size panels | Panel shows minimum height/width constraint. Cannot be dragged to zero. |
| EC-07 | Minimized editor window | Editor loops but does not render ImGui frames (standard behavior). |
| EC-08 | Scene file deleted externally | Save writes new file. Load of missing file shows error dialog. |
| EC-09 | Scene file permissions changed | Save returns error ("Cannot write file"). Error dialog, scene stays dirty. |
| EC-10 | OS close (Alt+F4) before saving | Equivalent to Quit. Prompt save if dirty. |
| EC-11 | Closing last Prefab tab | No effect on Scene tab. Prefab tab closes normally (with save prompt if dirty). |
| EC-12 | Component type with no registered properties | Inspector shows empty component section with name only. No crash. |
| EC-13 | Loading a scene with unknown component types | SceneLoader skips unknown types (warning logged). Other components load normally. |
| EC-14 | Multiple Prefab tabs open for same prefab | Each tab tracks independent dirty state. Saving one tab updates file but not the other tab. |

---

### Error Cases

| ID | Scenario | Expected behavior |
|---|---|---|
| ER-01 | Scene file load failure (corrupt/invalid YAML) | Error dialog: "Failed to load scene: [reason]". Current scene preserved. |
| ER-02 | Scene file save failure (disk full, permissions) | Error dialog: "Failed to save scene: [reason]". Scene remains dirty. |
| ER-03 | World clone failure (out of memory) | Error dialog: "Failed to enter Play mode — out of memory". Editor remains in Edit mode. |
| ER-04 | Game module load failure (DLL missing/error) | Error dialog: "Failed to load game module: [reason]". Scene loads without gameplay components. |
| ER-05 | Invalid YAML syntax in scene file | SceneLoader returns error. Error dialog with parsing info. No entities loaded. |
| ER-06 | Invalid input in Inspector (e.g., text in float field) | Field rejects input or shows validation highlight. Value reverts to last valid value. |
| ER-07 | Drag invalid file to Asset reference field | Drag rejected. Visual feedback (red highlight) if over field. |
| ER-08 | Attempt to delete entity that is a prefab root | Confirmation dialog: "This entity is a prefab instance root. Delete anyway?" |
| ER-09 | Console flood (1000+ logs per frame) | Console throttles or batches. Auto-scroll may skip frames. |
| ER-10 | Detached tab window closed via OS | Equivalent to closing tab normally. If it's the Game tab, stops Play mode. If Prefab, prompts save if dirty. |
| ER-11 | Rapid entity create/delete operations | Entity IDs are reused only after generation wraps (virtual infinite). No crash on rapid operations. |

---

### Log Channels

```
Editor           — General editor lifecycle (startup, shutdown, mode changes)
Editor:Scene     — Scene load/save/create operations
Editor:Play      — Play mode transitions (Play, Pause, Stop)
Editor:Entity    — Entity CRUD operations (create, delete, rename)
Editor:UI        — Panel/tab/layout events (open, close, detach, dock)
```

**Key log signals:**

| When | Channel | Level | Message |
|---|---|---|---|
| Editor starts | Editor | Info | "Editor initialized" |
| Scene loaded | Editor:Scene | Info | "Scene loaded: [path]" |
| Scene load failed | Editor:Scene | Error | "Failed to load scene: [reason]" |
| Scene saved | Editor:Scene | Info | "Scene saved: [path]" |
| Play pressed | Editor:Play | Info | "Play mode started" |
| Stop pressed | Editor:Play | Info | "Play mode stopped" |
| Entity created | Editor:Entity | Debug | "Entity created: [name] (id:[ID])" |
| Entity deleted | Editor:Entity | Debug | "Entity deleted: [name] (id:[ID])" |
| Tab detached | Editor:UI | Debug | "Tab detached: [tab name]" |

---

### Assumptions

| ID | Assumption |
|---|---|
| A-01 | ImGui docking branch `v1.91.8-docking` is available and provides the dockspace/multi-window APIs required for tab layouts and panel docking. |
| A-02 | All editor panels are implemented as ImGui widgets drawn within the ImGui dockspace (no native OS widgets). |
| A-03 | `World::clone()` is a required engine-level capability for Play mode that does **not yet exist**. A separate feature will be needed to implement it. |
| A-04 | Entity IDs remain valid for the lifetime of the World (no ID reuse while entities exist). |
| A-05 | The engine's `SceneLoader` and `SceneSaver` are sufficiently robust for editor use. |
| A-06 | The `ComponentRegistry` and `TypeRegistry` provide enough information for the Inspector to render type-appropriate property editors generically. |
| A-07 | The project directory is determined by the current working directory at editor launch (or a future project selection dialog). |
| A-08 | Panel sizes are remembered per-session only (in-memory). Cross-session persistence is deferred. |
| A-09 | Undo is limited to single-level entity deletion undo (Ctrl+Z). More complex undo systems are deferred. |
| A-10 | The editor operates on a **clone** of the World during Play mode (see A-03). The editor World is never modified while the game runs. |
| A-11 | Detached tab windows each have their own GL context and ImGui context, initialized by the engine's `Window` abstraction (preserving ADR-019 architecture boundaries). |
| A-12 | Console messages are buffered in-memory. No hard limit in MVP1, but excessive spam (>10K messages/frame) is throttled. |
| A-13 | All editor file operations (Open, Save) use the OS native file dialog. |

---

### Decisions

| # | Question | Decision | Rationale |
|---|---|---|---|
| D-01 | Where should new entities be created? | As **child of selected** entity (last child). If none selected, root level. | Natural hierarchy building. Users expect parent context. |
| D-02 | Must entity names be unique? | **No** — duplicate names allowed. Entity IDs are the internal identifier. | Simpler UX, less friction. Names are labels only. |
| D-03 | F-focus: snap or animate? | **Instant snap** — camera teleports to frame the entity. | Faster, simpler, no animation system needed for MVP1. |
| D-04 | Game tab after Stop: close or stay? | **Close entirely** — Game tab disappears on Stop. | Clean return to editing. No dead tab state. |
| D-05 | Prefab tab: isolated Play? | **No** — Prefab tab is edit-only for MVP1. | Simpler. Playing always runs the main scene. |
| D-06 | Detach tab: how to trigger? | **Right-click context menu** → "Detach Tab". | Simple, discoverable, no drag-to-detach complexity in MVP1. |
| D-07 | Console: persist or clear? | **Persist forever** — never auto-clear across mode transitions. | Useful for debugging across play sessions. |
| D-08 | Inspector read-only indicator? | Gray background, lock icon (🔒) per component header, "Play mode — read only" banner, Add/Remove buttons hidden. | Clear visual distinction between editable and read-only. |
| D-09 | Prefab tab layout? | **Identical to Scene tab layout** (hierarchy left, viewport center, inspector right, bottom tab bar). | Consistency. No need for a different layout. |
| D-10 | Undo during play? | **Disabled** — runtime world is read-only, editor world untouched. | No undoable operations during Play. Menu items grayed out. |

---

## Important conventions

### EditorContext

The `EditorContext` aggregate struct (defined in `src/editor/editor_context.h`) provides panels and menus with access to both editor state and engine services:

```cpp
struct EditorContext {
    Editor& editor;                              // editor-specific state (e.g., editor.world())
    buddd::engine::EngineContext const& engine;  // engine services
};
```

- Introduced in F-02 (Scene Panel — Entity Tree) to replace `EngineContext const&` in `EditorPanel` and `EditorMenu` virtual methods.
- Constructed stack-local each frame in `Editor::update()` and `Editor::draw_ui()` via `EditorContext{*this, ctx}`.
- Panels access the editor's World via `ctx.editor.world()` and engine services via `ctx.engine`.
- Lightweight aggregate — no constructors, destructors, or virtual methods. Trivially copyable.

This is now the standard context-passing mechanism for all editor panels and menus. See [F-02 spec](/.specs/sprint-2026-06/scene-panel-entity-tree/spec.md).

### v1 foundation (currently implemented)

- The `Editor` class orchestrates all subsystems. It owns a `CommandStack` (128-entry bounded undo/redo), a `ShortcutRegistry` (keyboard shortcut bindings), and vectors of `EditorMenu`/`EditorPanel` subclasses.
- Panels and menus are registered via `Editor::add_menu()` / `Editor::add_panel()` in `Editor::setup()`. No runtime plugin discovery.
- The editor uses **ImGui `DockBuilder`** for the default panel layout on first launch (Scene center, Properties right, Console bottom, Project/Assets bottom-left/bottom-right).
- Docking layout is persisted via `buddd_editor.ini` in the current working directory (`ImGui::GetIO().IniFilename`).
- Panels have a **100×100 minimum size constraint** via `ImGui::SetNextWindowSizeConstraints()`.
- The editor has a **two-phase lifecycle**: `Editor::update()` (shortcuts, command dispatch, state updates) runs before `render_scene()`; `Editor::draw_ui()` (7-phase UI rendering) runs after `render_scene()` in `EditorApp::on_render()`.
- Keyboard shortcuts are processed via `ShortcutRegistry::process()` gated by `ImGui::GetIO().WantCaptureKeyboard`.
- The `App` base class has a `virtual update(EngineContext const&)` method (default no-op), called once per frame after `World::update_updatables()`. All existing demo apps are unaffected.
- The editor also owns a `World` via `std::unique_ptr<World>` — created in the **Editor constructor**, available via `editor.world()`, and destroyed in the destructor. The World is always valid (no null checks needed) and is separate from `ctx.world` (the engine's demo-scene world). See [SPEC-029](/.specs/sprint-2026-06/editor-scene-state/spec.md).
- No SDL3, OpenGL, or GLM headers are included in `src/editor/` (per ADR-019).
- **F-01 additions**: File menu now includes New Scene (Ctrl+N), Open Scene (Ctrl+O), Save Scene (Ctrl+S), Save Scene As (Ctrl+Shift+S). Dirty state tracking (`dirty_` boolean + `*` in window title via `Window::set_title()`). OS file dialogs via Platform abstraction (SDL3 native dialogs — ImGuiFileDialog removed from build). Save-prompt modal state machine (`PendingOp` enum). Error modals for SceneLoader/SceneSaver failures. OS close button interception via `Platform::set_on_close_request()`. Scene management methods: `new_scene()`, `open_scene(path)`, `save_scene()`, `save_scene_as(path)`.
- **F-02 additions**: `EditorContext` aggregate struct (`src/editor/editor_context.h`) holds `Editor&` and `EngineContext const&` — provides panels with access to both editor state and engine services. `EditorPanel::update()`/`draw_ui()` and `EditorMenu::update()`/`draw_ui()` signatures changed from `EngineContext const&` to `EditorContext const&`. Panels access the editor's World via `ctx.editor.world()`. `ScenePanel` now renders the entity hierarchy tree via `ImGui::TreeNodeEx` (recursive traversal, empty state, expandable/collapsible, leaf/non-leaf distinction, `PushID`/`PopID` per-entity, `"(unnamed)"` fallback). See [F-02 spec](/.specs/sprint-2026-06/scene-panel-entity-tree/spec.md).

### North-star (future — not yet implemented)

- The editor is a **consumer** of engine APIs — it does not own entities, worlds, or components. The engine's `World` class manages all entity lifecycle.
- During Play mode, read-only enforcement is at the **editor UI level** (fields disabled, buttons hidden), not at the engine API level.
- Panel size persistence is **session-only** (lost on restart). Cross-session persistence is deferred to post-MVP1.
- The editor uses `World::clone()` for Play mode — this engine-level prerequisite is not yet implemented (see A-03 in the spec).
- Detached tabs use **OS-level windows with their own GL context** — not ImGui's `ViewportsEnable` flag.
- The editor's project directory is the **current working directory** from which `buddd edit` is launched.
- Console messages persist across all mode transitions — only the Clear button removes them.
- The Scene tab cannot be closed (close button hidden/disabled).

## Related specs

- [SPEC-028 — Editor Foundation](/.specs/sprint-2026-06/editor-foundation/spec.md) — Command system, menus, shortcuts, panels, docking persistence, About popup (v1 foundation, current)
- [SPEC-F-02 — Scene Panel Entity Tree](/.specs/sprint-2026-06/scene-panel-entity-tree/spec.md) — `EditorContext` struct, `EditorPanel`/`EditorMenu` signature changes, `ScenePanel` entity tree implementation
- [SPEC-2026-06 — Editor UX Design (North-Star)](/.specs/sprint-2026-06/editor-ux-design/spec.md) — Complete editor UX design document (future vision)
- [SPEC-Editor-Scaffolding](/.specs/sprint-2026-06/editor-scaffolding/spec.md) — Editor scaffolding and architecture setup

## Related ADRs

- [ADR-027](/docs/adr/ADR-027-editor-architecture.md) — Editor architecture: separate static library, EditorApp adapter, namespace, architecture boundary
- [ADR-026](/docs/adr/ADR-026-imgui-integration.md) — ImGui integration (docking branch, SDL3 + OpenGL3 backends)
- [ADR-028](/docs/adr/ADR-028-component-type-registry.md) — Component type registry (Inspector property panel consumes this)
- [ADR-019](/docs/adr/ADR-019-architecture-boundaries.md) — Architecture boundaries (applies to `src/editor/`)
- [ADR-014](/docs/adr/ADR-014-cli-app-system.md) — CLI App System (editor uses `run_app()`)
- ADR-029 — Editor UX decisions (panel layout, tab system, play mode) — Accepted

## Last reviewed

2026-06-12 — Updated for F-01 (SPEC-028): command system, menus, shortcuts, five placeholder panels, docking persistence, two-phase lifecycle, `App::update()` extension. Updated for F-02: `EditorContext` struct, `EditorPanel`/`EditorMenu` signature changes, `ScenePanel` entity tree implementation.
