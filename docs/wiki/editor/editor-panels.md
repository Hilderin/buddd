# Editor Panels

> **Current status (F-01 — editor-scene-load-save + F-02 — scene-panel-entity-tree + F-03 — entity-selection-multi-select + F-04 — entity-operations + F-05 — inspector-transform + F-06 — properties-panel-ux-polish + Editor Dialog Abstraction, June 2026):** This document describes the **north-star vision** for the editor's panel system (tabs, Play mode, prefabs, viewport, inspector, toolbar). The currently implemented foundation (F-01 + F-02 + F-03 + F-04 + F-05 + F-06 + Editor Dialog Abstraction) includes:
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
> - **Dialog abstraction**: Reusable `Dialog` base class (`src/editor/editor_dialog.h`) with `CustomDialog` concrete class and `DialogButton` struct, owned via `Editor::dialogs_` with ID-based dedup, rendered in Phase 4 of `draw_ui()`. The About popup has been migrated from ad-hoc `show_about_`/`draw_about_popup()` to a `CustomDialog` instance. All remaining popups (save-prompt, error modals, delete-confirmation) have been ported to the Dialog abstraction — no ad-hoc ImGui popup code remains for these. `DialogButton::callback` type changed from `void()` to `bool()`. New convenience helpers: `open_message_dialog()`, `open_error_dialog()`, `open_confirm_dialog()`, `open_ok_cancel_dialog()`, `defer()`.
> - **F-02 additions**: `EditorContext` aggregate struct (`src/editor/editor_context.h`) provides panels with access to both `Editor&` and `EngineContext const&`; `EditorPanel`/`EditorMenu` signatures changed from `EngineContext const&` to `EditorContext const&`; `ScenePanel` renders the entity hierarchy tree via ImGui `TreeNodeEx` (empty state, expandable/collapsible, leaf/non-leaf, `PushID`/`PopID` per-entity).
> - **F-03 additions**: `Selection` value class and `EditorSelection` manager in `src/editor/editor_selection.h` (see [F-03 spec](/.specs/sprint-2026-06/entity-selection/spec.md)). `Editor` owns an `EditorSelection`, accessible via `editor.selection()`. Scene Panel: click to select, Ctrl+click to toggle, Shift+click for range select, Ctrl+A for select all. Selection highlighting with `ImGuiTreeNodeFlags_Selected`. Snapshot/restore for future Command undo/redo integration. `Editor::new_scene()` and `Editor::open_scene()` clear selection.
> - **F-04 additions**: Three new Command classes in `src/editor/commands/`: `CreateEntityCommand`, `DeleteEntityCommand`, `RenameEntityCommand`. `Command::execute()` and `undo()` now accept `EditorContext const&` (breaking change). `Editor::command_stack()` accessor added. Context menu on entity (Create Empty, Delete, Rename) and on empty area (Create Empty). Delete key (focused) and F2 key (rename) keyboard shortcuts. Confirmation dialog for deletion of entities with children. `World::flush_destroyed()` called each frame in `Editor::update()`. Selection snapshot/restore via `EditorSelection::snapshot()`/`restore()` in Commands. Entity hierarchy preserved on delete undo (component state not preserved in v1). See [F-04 spec](/.specs/sprint-2026-06/entity-operations/spec.md).
>
> - **F-05 additions**: `InspectorTypeEditor` registry system (`src/editor/inspector_editors.h/.cpp`) provides a static registry mapping C++ types to reusable ImGui editor widgets. Base class `InspectorTypeEditor` + typed template `TypedInspectorEditor<T>` + 8 built-in editors (float, int, bool, string, Vec2, Vec3, Vec4, Quat) registered at startup. `EditorFlags` struct for numeric constraints (min, max, step). Fallback to `TypeRegistry::to_string()`/`from_string()` text input when no editor is registered. `PropertiesPanel` now implements `draw_ui()` with entity name field (editable, uses `RenameEntityCommand`), Transform section (Position editable via Vec3 editor, Rotation editable as Euler degrees via Quat editor, Scale editable via Vec3 editor), and centered "No entity selected" no-selection state. Multi-select shows `primary()` entity only. `EditorSelection::primary()` accessor added — returns last `select()`-ed entity, updated on every select, reset on clear. `Selection` value class now includes `primary_` and `anchor_` fields for correct snapshot/restore. `Quat::to_euler()` added to engine math (`src/engine/math/quat.h`). `World::entity(EntityId)` public factory method added to `src/engine/scene/world.h`. Read-only mode during Play is not yet implemented (deferred to F-15). See [F-05 spec](/.specs/sprint-2026-06/inspector-transform/spec.md).
> > - **F-06 additions**: The Transform section was upgraded to a 2-column `ImGui::Table` layout (property name | value) with no column headers. Vec2/Vec3/Vec4/Quat editors now use composite axis input widgets: a colored drag-handle (colored rectangle with white text, click+drag to scrub value) on the left and an `ImGui::InputFloat` (single-click text entry, `"%.2f"` format) on the right. Axis colors: X/Pitch=red (`#FF4444`), Y/Yaw=green (`#44FF44`), Z/Roll=blue (`#4444FF`), W=gray. Property-name labels are no longer rendered by Vec/Quat editors — the caller (e.g., `draw_transform_section()`) renders them in table column 0. The editor API parameter `label` was renamed to `id` to clarify it is used only for ImGui PushID scoping, not visual display. Rotation preserves Pitch/Yaw/Roll labels with the same axis colors. Scale enforces `min_value=0.001`. See [F-06 spec](/.specs/sprint-2026-06/properties-panel-ux-polish/spec.md).
> >
> > The north-star design below (tabs, viewport, inspector, toolbar, Play mode, prefabs) is **planned for future sprints** and is not yet implemented.

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
- **Content (F-02 + F-03 + F-04 implemented):** Tree view of all entities with parent/child indentation. Root entities rendered as top-level `TreeNodeEx` nodes, children indented under parents. Empty state shows "No entities". Leaf/non-leaf distinction via `ImGuiTreeNodeFlags_Leaf`. Per-entity `PushID`/`PopID` prevents ImGui ID collisions. Entities with empty names display as "(unnamed)". **Entity CRUD (F-04)**: Create Empty via context menu (entity right-click or empty-area right-click). Delete via Delete key or context menu (confirmation dialog when children exist). Rename via F2 or context menu (inline `ImGui::InputText` with Enter confirm, Escape cancel, empty name rejection). All operations support undo/redo via Command pattern. See [F-04 spec](/.specs/sprint-2026-06/entity-operations/spec.md).
  - **Selection (F-03):** Left-click selects an entity (Replace modifier — clears previous, selects clicked). Ctrl+click toggles entity in/out of selection (add/remove). Shift+click selects a range in depth-first tree order (anchor to clicked entity, inclusive). Ctrl+A selects all entities in the World. Clicking empty space clears the selection and anchor. Selected entities display with `ImGuiTreeNodeFlags_Selected` highlighting. The `EditorSelection` manager is owned by `Editor` and accessible via `ctx.editor.selection()`. See [F-03 spec](/.specs/sprint-2026-06/entity-selection/spec.md).
- **Default position/size:** Left dock, ~250px width

#### Inspector Panel

- **Appears in:** Scene tab, Prefab tab
- **Purpose:** Display and edit the selected entity's components and properties
- **Content (F-05 + F-06 implemented):** Entity name field (editable, uses `RenameEntityCommand`), Transform section with Position/Rotation/Scale rows in a 2-column `ImGui::Table` layout (property name | value) with no column headers. Property-name labels ("Position", "Rotation", "Scale") are rendered in column 0 by the panel. In column 1, the editors use composite axis input widgets: a colored drag-handle (colored rectangle with white text, click+drag to scrub) on the left and an `ImGui::InputFloat` (single-click text entry, `"%.2f"` format) on the right. Axis colors: X/Pitch=red, Y/Yaw=green, Z/Roll=blue. Rotation preserves Pitch/Yaw/Roll labels with the same axis colors, values in degrees wrapped to [-180, 180] via `Quat::to_euler()`/`from_euler()` round-trip. Scale enforces `min_value=0.001`. The `id` parameter passed to editors is used only for ImGui PushID scoping (not displayed). All editors use `InspectorTypeEditorRegistry` with `EditorContext` for dirty marking. No-selection state shows centered "No entity selected". Multi-select shows `primary()` entity only.
- **Default position/size:** Right dock, ~300px width
- **Read-only mode during Play:** Not yet implemented (deferred to F-15). Grayed-out fields, lock icon banner, hidden Add/Remove buttons are planned but not wired.

##### Inspector Property Editors

> **F-05 + F-06 implementation**: The `InspectorTypeEditorRegistry` (in `src/editor/inspector_editors.h/.cpp`) provides reusable ImGui editor widgets for the 8 built-in types listed below, with a fallback text-input path for unregistered types (uses `TypeRegistry::to_string()`/`from_string()`). The Transform section (Position, Rotation, Scale) uses these editors via `InspectorTypeEditorRegistry::draw<Vec3>()` and `InspectorTypeEditorRegistry::draw<Quat>()`. As of F-06, Vec2/Vec3/Vec4/Quat editors use composite axis input widgets (colored drag-handle + InputFloat) and no longer render a property-name label — the `id` parameter is used only for ImGui PushID scoping. Component property editors (for future use) will consume the same registry.

The Inspector renders each component property with a type-appropriate editor widget based on its registered type in the `ComponentRegistry` and `TypeRegistry`:

| Property type | Editor widget |
|---|---|
| `bool` | Checkbox |
| `int` | Integer drag/input field |
| `float` | Float drag/input field |
| `std::string` | Text input field |
| `Vec2` | Composite axis widgets: colored drag-handle (red=X, green=Y) + InputFloat |
| `Vec3` | Composite axis widgets: colored drag-handle (red=X, green=Y, blue=Z) + InputFloat |
| `Vec4` | Composite axis widgets: colored drag-handle (red=X, green=Y, blue=Z, gray=W) + InputFloat |
| `Color` (Vec3/Vec4 interpreted as RGB/RGBA) | Color picker + float fields (not yet implemented) |
| `Entity reference` | Text field showing entity name (read-only in MVP1) |
| `Asset reference` | Text field showing asset path + drag-accept target (not yet implemented) |
| `Quat` | Euler angles in degrees via composite axis widgets (Pitch=red, Yaw=green, Roll=blue), wrapped to [-180, 180] |

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
|---|---|---|---|
| **Select entity** | Left-click entity row in hierarchy | Inspector updates, viewport highlights entity + shows gizmo |
| **Deselect** | Click empty area in hierarchy | Selection clears, Inspector shows "No entity selected", gizmo hidden |
| **Create empty entity** | Right-click entity → "Create Empty" OR right-click empty area → "Create Empty" | Creates entity with empty name (displays as "(unnamed)"). If selection anchor exists → last child. Otherwise → root level. Not auto-selected. Undo/redo via `CreateEntityCommand`. |
| **Delete entity** | Right-click entity → "Delete" OR Delete key (Scene Panel focused) | Removes entity. If any selected entity has children → confirmation dialog. Selection cleared after deletion. Undo restores entity identity, name, and hierarchy (component state not preserved in v1). Undo/redo via `DeleteEntityCommand`. |
| **Rename entity** | Select + F2 OR right-click entity → "Rename" (exactly one selected) | Inline `ImGui::InputText` replaces tree node label. Enter confirms, Escape cancels, focus loss confirms. Empty name rejected (no command pushed). Same name → no-op. Undo/redo via `RenameEntityCommand`. |
| **Undo delete** | Edit > Undo (Ctrl+Z) | Restores deleted entities with hierarchy preserved, selection restored via snapshot. Disabled during Play. |
| **Add component** | "+ Add Component" button in Inspector | Opens searchable dropdown of registered component types. Selecting adds it to entity. |
| **Remove component** | ⓧ button on component header | Removes component from entity. |
| **Focus camera** | F key (entity selected) | Editor camera snaps to frame selected entity's bounding box. |
| **Translate** | Drag gizmo arrow in viewport | Moves entity along the dragged axis. Inspector Transform fields update in real-time. |

**Deferred for post-MVP1:** Drag-reparent, duplicate entity (Ctrl+D), copy/paste, viewport click-to-select, rotate/scale gizmos.

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
| A-09 | Undo for entity operations (Create, Delete, Rename) is implemented via the Command pattern with full selection snapshots. Delete undo restores entity identity, name, and hierarchy (component state not preserved in v1). 128-entry bounded undo stack. |
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
- **F-03 additions**: `Selection` value class (`src/editor/editor_selection.h`) stores a set of `EntityId`s — cloneable, comparable, independently testable, and savable in Commands. Provides `contains(id)`, `size()`, `empty()`, `first()`, iteration, `add()`, `remove()`, `clear()`, `operator==`. `EditorSelection` manager owns the active selection state with `select(id, modifier)`, `clear()`, `set_selection(ids)`, `snapshot()`/`restore()`, anchor management, and callback infrastructure (`on_change()`/`remove_on_change()`). `SelectionModifier` enum: `Replace` (plain click — clear + select) and `Toggle` (Ctrl+click — add/remove). Multi-select: Ctrl+click toggle, Shift+click range (depth-first tree order), Ctrl+A select all. Selection highlighting via `ImGuiTreeNodeFlags_Selected`. Panels access selection via `ctx.editor.selection()`. Snapshot/restore for Command undo/redo future-proofing. Selection cleared on `Editor::new_scene()` and `Editor::open_scene()`. See [F-03 spec](/.specs/sprint-2026-06/entity-selection/spec.md).
- **F-04 additions**: `Command::execute()` and `undo()` now accept `EditorContext const&` (breaking change to `src/editor/command.h`). `CommandStack::execute()`, `undo()`, `redo()` accept `EditorContext const&` and forward it to the command. `Editor::command_stack()` accessor added. Three new Command classes in `src/editor/commands/`: `CreateEntityCommand` (creates entity as child of anchor or root), `DeleteEntityCommand` (destroys selected entities, saves state for undo), `RenameEntityCommand` (renames a single entity). All three capture selection snapshot before mutation and restore on undo. `ScenePanel` gains context menu (entity: Create Empty, Delete, Rename; empty area: Create Empty), Delete key handler (gated by focus), F2 key handler (inline rename), confirmation modal for deletion of entities with children. Inline rename uses `ImGui::InputText` (Enter confirm, Escape cancel, focus loss confirms, empty name rejected). `World::flush_destroyed()` called each frame in `Editor::update()` after command processing. See [F-04 spec](/.specs/sprint-2026-06/entity-operations/spec.md).
- **F-05 additions**: `InspectorTypeEditor` abstract base class and `TypedInspectorEditor<T>` typed template in `src/editor/inspector_editors.h`. `EditorFlags` struct with `min_value`, `max_value`, `step_value`. `InspectorTypeEditorRegistry` static class providing `register_editor<T>()`, `draw<T>()`, and `has_editor<T>()`. 8 built-in editors registered at startup in `register_builtin_inspector_editors()` (float, int, bool, string, Vec2, Vec3, Vec4, Quat). See [F-05 spec](/.specs/sprint-2026-06/inspector-transform/spec.md).
- `PropertiesPanel` implementation (`src/editor/panels/properties_panel.cpp`) replaces the empty placeholder. `draw_ui()` checks `EditorSelection::primary()`, retrieves entity via `World::entity(EntityId)`, and renders: entity name field (editable `ImGui::InputText` using `RenameEntityCommand`), Transform section (always-expanded `ImGui::CollapsingHeader` with Position/Vec3, Rotation/Quat→Euler degrees, Scale/Vec3 rows via `InspectorTypeEditorRegistry::draw<T>()`). `draw_no_selection_state()` shows centered "No entity selected". `draw_entity_name()` syncs buffer with selection changes, reverts on empty. `draw_transform_section()` uses `InspectorTypeEditorRegistry` for Vec3/Quat editing — dirty marking handled internally by editors.
- `EditorSelection::primary()` accessor added — returns the last `select()`-ed entity (`std::optional<EntityId>`). Updated on every `select()` call, reset on `clear()`. `Selection` value class now includes `primary_` and `anchor_` members so `snapshot()`/`restore()` capture the full selection state atomically (required for correct Command undo).
- `Quat::to_euler()` added to `src/engine/math/quat.h` — returns `Vec3` (pitch, yaw, roll) in radians via `glm::eulerAngles()`. Matches `from_euler()` convention. Round-trips within single-precision epsilon (non-gimbal-lock).
- `World::entity(EntityId)` public factory method added to `src/engine/scene/world.h` — validates the slot and returns an `Entity` handle or default-constructed Entity (invalid ID). Required by PropertiesPanel for entity lookup from primary ID.
- **F-06 additions**: `draw_axis_widget()` file-local helper added to `inspector_editors.cpp` — composite axis input widget with colored drag-handle (ImDrawList colored rectangle + InvisibleButton for drag-to-scrub) and `ImGui::InputFloat` for single-click text entry, format `"%.2f"`. Vec2/Vec3/Vec4/Quat editors rewritten to use composite axis widgets with axis colors (X/Pitch=red, Y/Yaw=green, Z/Roll=blue, W=gray). Label rendering removed from all Vec/Quat editors — the `id` parameter is used only for `PushID` scoping, not display. `draw_transform_section()` in `properties_panel.cpp` rewritten to use a 2-column `ImGui::Table` (property name | value) with no column headers. Property-name labels ("Position", "Rotation", "Scale") are rendered in column 0; editors are called in column 1. Scale row passes `EditorFlags{min_value=0.001f}`. All Vec editors clamp component values to `[flags.min_value, flags.max_value]` after change. Graceful degradation if `BeginTable` returns `false`. See [F-06 spec](/.specs/sprint-2026-06/properties-panel-ux-polish/spec.md).
> - **Editor Dialog Abstraction**: A reusable `Dialog` abstraction introduced in `src/editor/editor_dialog.h`. The `Dialog` abstract base class defines `id()`, `title()`, `draw_content()`, `request_close()`, `should_close()`, and virtual `handle_escape()` (default calls `request_close()`). `CustomDialog` is a concrete subclass taking a content function, button list (vector of `DialogButton`), and optional `on_close` callback — the framework renders buttons after `draw_content()` with auto-close after any click. `DialogButton` stores `label`, `label_id`, `callback`, and optional `ImGuiKey shortcut` — callback changed from `void()` to `bool()` (return `true` to close dialog). The `Editor` class manages dialogs via `std::vector<std::unique_ptr<Dialog>> dialogs_` with ID-based dedup (previously tracked via `std::unordered_set<std::string> opened_dialog_ids_`, since removed as OpenPopup is now called unconditionally each frame). The public method `open_dialog(std::unique_ptr<Dialog>) -> bool` opens a dialog (returns `false` on duplicate ID). Convenience helpers: `open_message_dialog()`, `open_error_dialog()`, `open_confirm_dialog()`, `open_ok_cancel_dialog()`. A `defer()` mechanism enqueues actions for execution at the start of the next `draw_ui()` frame. Phase 4 of `draw_ui()` is now the general dialog rendering phase: iterates `dialogs_`, calls `ImGui::OpenPopup((dialog->title() + "###" + dialog->id()).c_str())` (the `"title###id"` pattern prevents ImGui ID collisions), renders each via `ImGui::BeginPopupModal`, dispatches Escape to the topmost dialog only, and removes closed dialogs via `std::erase_if`. The About popup, save-prompt, error modals, and delete confirmation all use the Dialog abstraction — no ad-hoc ImGui popup code remains. See [Editor Dialog Abstraction spec](/.specs/sprint-2026-06/editor-dialog-abstraction/spec.md) and [Port Popups to Dialog spec](/.specs/sprint-2026-06/port-popups-to-dialog/spec.md).

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
- [SPEC-F-04 — Entity Operations](/.specs/sprint-2026-06/entity-operations/spec.md) — Create Empty, Delete, Rename, context menu, keyboard shortcuts, Command signature change, flush_destroyed lifecycle
- [SPEC-Editor-Scaffolding](/.specs/sprint-2026-06/editor-scaffolding/spec.md) — Editor scaffolding and architecture setup
- [SPEC-F-06 — Properties Panel UX Polish](/.specs/sprint-2026-06/properties-panel-ux-polish/spec.md) — Table layout, composite axis widgets, axis colors, label→id rename
- [Editor Dialog Abstraction spec](/.specs/sprint-2026-06/editor-dialog-abstraction/spec.md) — Reusable Dialog base class, CustomDialog, DialogButton, ID-based dedup, Phase 4 dialog rendering

## Related ADRs

- [ADR-027](/docs/adr/ADR-027-editor-architecture.md) — Editor architecture: separate static library, EditorApp adapter, namespace, architecture boundary
- [ADR-026](/docs/adr/ADR-026-imgui-integration.md) — ImGui integration (docking branch, SDL3 + OpenGL3 backends)
- [ADR-028](/docs/adr/ADR-028-component-type-registry.md) — Component type registry (Inspector property panel consumes this)
- [ADR-019](/docs/adr/ADR-019-architecture-boundaries.md) — Architecture boundaries (applies to `src/editor/`)
- [ADR-014](/docs/adr/ADR-014-cli-app-system.md) — CLI App System (editor uses `run_app()`)
- ADR-029 — Editor UX decisions (panel layout, tab system, play mode) — Accepted

## Last reviewed

2026-06-13 — Updated for Editor Dialog Abstraction: reusable Dialog abstraction, `editor_dialog.h` (Dialog, CustomDialog, DialogButton), Phase 4 dialog rendering, About popup migrated to CustomDialog. Updated for F-01 (SPEC-028): command system, menus, shortcuts, five placeholder panels, docking persistence, two-phase lifecycle, `App::update()` extension. Updated for F-02: `EditorContext` struct, `EditorPanel`/`EditorMenu` signature changes, `ScenePanel` entity tree implementation. Updated for F-04: entity operations (Create, Delete, Rename), context menu, keyboard shortcuts, Command signature change, `flush_destroyed()` lifecycle. Updated for F-06: composite axis widgets, table layout, axis colors, label→id rename, Scale min_value.
