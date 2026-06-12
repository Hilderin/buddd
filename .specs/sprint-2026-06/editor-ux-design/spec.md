# SPEC-2026-06 — Editor UX Design (North-Star)

## Problem

The Buddd engine has no interactive editor. Developers must write code to create entities, attach components, position objects, and test gameplay — there is no visual tool for scene composition, entity inspection, or play-mode testing. This makes content creation slow, error-prone, and inaccessible to non-programmers. A visual editor is required to make the engine usable for game development.

This document defines the **complete UX vision** for Buddd Editor MVP1. It is the north-star reference from which all individual editor feature specs (hierarchy panel, inspector, viewport, play mode, etc.) will be derived. It describes every panel, interaction, workflow, and state transition at the UX level — without prescribing implementation.

## Goals

| ID | Goal |
|---|---|
| G-01 | Define a complete, testable UX model for the Buddd Editor covering all panels, tab types, layouts, and workflows. |
| G-02 | Describe entity editing workflows: scene creation, entity management (select, create, delete, rename, transform), and component editing via the Inspector. |
| G-03 | Define the Play Mode experience: cloning the editor World, opening a Game tab, read-only Scene tab inspection, and clean Stop/restore. |
| G-04 | Define the tab system: Scene tab (always present, leftmost), Prefab tab, Game tab, with fixed per-tab-type layouts and user-resizable panels. |
| G-05 | Describe detached tabs (multi-window) as an OS-level window containing a tab's full layout. |
| G-06 | Define cross-panel communication: entity selection propagation, play mode state transitions, and panel synchronization. |
| G-07 | Define persistent utility panels (Project, Console) and their behavior across all tab types. |
| G-08 | Establish the north-star for all subsequent editor feature specs — every panel, interaction, and state machine must be traceable to this document. |

## Non-goals

| ID | Exclusion |
|---|---|
| NG-01 | No implementation-level decisions (frameworks, data structures, event systems, widget libraries beyond ImGui). |
| NG-02 | No rotate/scale gizmos — MVP1 supports translate gizmo only. |
| NG-03 | No mouse picking in the viewport — entity selection is via the hierarchy panel only in MVP1. |
| NG-04 | No drag-and-drop reparenting in the hierarchy panel. |
| NG-05 | No Prefab override system (instance overrides, revert, apply). |
| NG-06 | No advanced undo system — only basic delete undo is in scope for MVP1. |
| NG-07 | No advanced asset browser (thumbnails, metadata database, import pipeline UI). |
| NG-08 | No code editor, script editor, or debugger integration. |
| NG-09 | No editor themes beyond the default dark theme. |
| NG-10 | No multi-scene or additive scene loading. |
| NG-11 | No hot-reload during Play (runtime-to-editor sync). |
| NG-12 | No pause-mode detailed inspection (frame stepping, component debugging). |
| NG-13 | No changes to the engine layer — all editor features consume existing engine APIs (World, Entity, SceneLoader, SceneSaver, ComponentRegistry, TypeRegistry). |

## Actors

| Actor | Description |
|---|---|
| **Content Creator** | A human using the editor to compose scenes, place entities, configure components, and test gameplay. May have limited or no programming knowledge. |
| **Technical Artist** | A human who creates prefabs, configures materials, and sets up asset references within the editor. |
| **Engine Developer** | A human extending the editor with new panels, component editors, or tools. Consumes this spec as the authoritative design reference. |

## User-visible behavior

### Overview — The Editor Window

The Buddd Editor opens as a resizable OS window (default 1280×800). The window contains a **tab bar** at the top (below the menu bar), a **menu bar** at the very top, and optionally a **toolbar** below the menu bar. The active tab's content fills the remaining area.

The editor follows an **Unreal Engine-like** model: one scene open at a time in a dedicated Scene tab, with additional tabs for other editing contexts (Prefabs, Game).

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

### Tab System

**Tab types (MVP1):** Scene, Prefab, Game.

**The Scene tab** is special:
- Always present — it cannot be closed (only hidden by opening another tab type? No — the Scene tab is always visible in the tab bar, as the leftmost tab).
- Represents the main editing context: the open scene file.
- Opening another scene replaces the current scene content (with a save prompt if dirty).

**Additional tabs** open for:
- **Prefab tab**: Opened by double-clicking a `.yaml` prefab file in the Project panel. Contains the prefab's isolated World for editing.
- **Game tab**: Opens automatically when Play is pressed. Contains the game viewport. Closes (or becomes inert) when Stop is pressed.

**Tab behavior:**
- Tabs can be reordered by dragging, except the Scene tab which always remains leftmost.
- Closing the Scene tab is not possible (close button hidden or disabled).
- Closing a Prefab tab prompts to save if the prefab is dirty.
- Closing the Game tab during play is equivalent to pressing Stop (stops play mode, restores editor World).
- Right-click on any tab header opens a context menu with options including "Detach Tab" (opens tab as separate OS window).

### Layout Per Tab Type

Each tab type has a **fixed, pre-defined layout** of dockable panels. The layout structure (which panels exist and their initial positions) is fixed per tab type.

**Users can:**
- Resize panels by dragging panel borders within the tab's dockspace.
- Panel sizes are remembered per tab type for the duration of the editor session only. Resizing is lost on restart; cross-session persistence (saving panel sizes to disk) is deferred to post-MVP1.
- Panels can be tabbed together, split, or floated within the dockspace — standard ImGui docking behavior.

**Users cannot:**
- Add or remove panels from a tab type's layout (the panel set is fixed per tab type).
- Change which panels appear in which tab type.

### Detached Tabs (Multi-Window)

Any tab can be **detached** as a new OS window:
- **Trigger**: Right-click on a tab header opens a context menu with a "Detach Tab" option. Selecting it detaches the tab. (Drag-to-detach may be added post-MVP1.)
- Detaching a tab creates a new OS-level window with its own GL context and ImGui dockspace.
- The detached window contains the tab's full layout (all panels for that tab type).
- The original tab bar shows the tab as "detached" (grayed out or removed until re-attached).
- Re-attaching a detached tab merges it back into the main editor window's tab bar.
- Example workflow: Game tab on monitor 2 while Scene tab stays on monitor 1.

### Scene Management

**One scene at a time** in the Scene tab (Unreal-like model):

- **File > New Scene**: Creates an empty, untitled scene. Clears the current World. Prompts save if the current scene is dirty.
- **File > Open Scene**: Opens an OS file dialog filtered to `.yaml` files. Loads the selected YAML via `SceneLoader`. Replaces the current scene. Prompts save if the current scene is dirty.
- **File > Save Scene**: If the scene has a file path, saves via `SceneSaver`. If untitled, behaves like Save As.
- **File > Save Scene As**: Opens an OS file dialog. Saves the scene to the chosen path. Updates the scene's file path.
- **File > Quit**: Prompts save if any open tab (Scene or Prefab) is dirty. Exits the application.

**Dirty state:**
- The scene is dirty if any entity, component, or property has been modified since the last save.
- The Scene tab title shows an asterisk (`*`) when dirty (e.g., `🔷 *Scene_01`).
- Prefab tabs also track dirty state independently.

**Scene replacement workflow:**
1. User triggers File > Open Scene (or double-clicks a scene in the Project panel).
2. If the current scene is dirty, a modal dialog appears: "Save changes to [scene name]? [Save] [Don't Save] [Cancel]".
3. On Save: the scene is saved, then the new scene loads.
4. On Don't Save: the current scene is discarded, new scene loads.
5. On Cancel: the operation is aborted, current scene remains.

### Project Panel (File Browser)

A tab in the bottom panel area, alongside **Console** and **Assets**. Only one tab is visible at a time; the user switches between them by clicking the tab header (**Project | Console | Assets**). Provides file-system browsing of the project directory.

**Content:**
- A tree view rooted at the project's working directory (or a configured project root).
- Files are filtered to show relevant asset types: `.yaml` (scenes, prefabs), `.gltf`/`.glb` (models), `.png`/`.jpg` (textures). Other files may be hidden or shown with a toggle.
- Directories are expandable/collapsible.

**Interactions:**
- **Double-click a `.yaml` scene file**: Opens it in the Scene tab (replaces current scene, with save prompt).
- **Double-click a `.yaml` prefab file**: Opens it in a new Prefab tab.
- **Right-click context menu**: Open, Delete, Rename, Show in Explorer (opens OS file manager at the file's location).
- **Single click**: Selects the file (highlights it). Details shown in a status area or tooltip (filename, type, size — deferred).

### Assets Panel (Asset Browser)

A tab in the bottom panel area, alongside **Project** and **Console**. Only one tab is visible at a time; the user switches between them by clicking the tab header (**Project | Console | Assets**). Provides quick access to project assets for drag-and-drop reference assignment.

**MVP1 content:**
- A flat list or simple grid of asset files in the project's `assets/` directory.
- **Type filter buttons**: All, Models, Textures, Materials — filter the displayed items.
- **Drag from Assets panel** to an asset-reference field in the Inspector to assign the asset.

**Deferred for post-MVP1:** Thumbnails, metadata panel, import status, search bar, favorites.

### Console Panel

A tab in the bottom panel area, alongside **Project** and **Assets**. Only one tab is visible at a time; the user switches between them by clicking the tab header (**Project | Console | Assets**). Displays engine and editor log output.

**Content:**
- Scrollable log lines, newest at bottom. Auto-scrolls to bottom by default.
- Each line is color-coded by severity level:
  - Trace/Debug: gray
  - Info: white
  - Warn: yellow
  - Error: red
  - Fatal: bright red
- Each line shows a timestamp and optional channel tag.

**Persistence behavior:**
- Console messages **persist across all mode transitions** (Edit → Play → Pause → Stop → Edit). The console is never auto-cleared.
- The only way to clear messages is the explicit **Clear button** (manual user action).

**Controls:**
- **Clear button**: Clears all visible log lines.
- **Auto-scroll toggle**: When locked, the view stays at the current scroll position (does not jump to new entries).
- **Channel filter**: Dropdown or checkboxes to show/hide log channels (engine, editor, render, asset, input, physics, game).
- **Level filter**: Dropdown to set minimum visible severity (Trace, Debug, Info, Warn, Error, Fatal).

### Menu Bar

Present in all tab types at the top of the editor window.

| Menu | Entries |
|---|---|
| **File** | New Scene, Open Scene, Save Scene, Save Scene As, separator, Quit |
| **Edit** | Undo, Redo, separator, Delete Selected Entity | Undo/Redo limited to single-level entity deletion undo (see A-09). Undo/Redo disabled (grayed out) during Play mode. |
| **View** | Toggle panel visibility for each panel type (Scene, Inspector, Viewport, Console, Project, Assets) — checkmarks indicate visibility |
| **Help** | About Buddd Editor (version, credits) |

### Toolbar

Present in Scene, Prefab, and Game tabs (may differ per tab type).

**Scene/Prefab tab toolbar buttons:**
- **▶ Play**: Starts Play mode (disabled if already playing).
- **⏸ Pause**: Pauses the running game (enabled only during Play).
- **⏹ Stop**: Stops Play mode (enabled only during Play or Pause).
- **Transform mode selector**: A segmented control — W (Translate), E (Rotate), R (Scale). In MVP1, Rotate and Scale are visible but disabled (grayed out). Only Translate is active.
- **Grid toggle**: Shows/hides the ground-plane grid in the viewport.

**Game tab toolbar buttons:**
- **▶ Play, ⏸ Pause, ⏹ Stop** — same as above.
- No transform or grid controls (Game tab is for runtime viewing, not editing).

---

### Panel: Scene Panel (Hierarchy)

**Purpose:** Display and manage the entity hierarchy as a tree.

**Appears in:** Scene tab, Prefab tab.

**Content:**
- A tree view of all entities in the current World, with parent/child indentation.
- Each row shows the entity name.
- The selected entity is highlighted (background color change).
- Root-level entities appear at the top level; children are indented under their parent.

**MVP1 entity operations:**

| Operation | Trigger | Behavior |
|---|---|---|
| **Select entity** | Left-click on entity row | Entity becomes selected. Inspector updates to show its components. Viewport highlights the entity and shows the transform gizmo. |
| **Deselect** | Click empty area in hierarchy | Selection clears. Inspector shows nothing (or "No entity selected"). Viewport hides gizmo. |
| **Create empty entity** | Right-click → "Create Empty" OR + button in panel header | Creates a new entity with default name ("Entity") and identity Transform. If a single entity is selected, the new entity becomes its **last child** (placed at local origin, appended at the end of the child list). If nothing is selected or multiple entities are selected, the new entity goes to root level (appended at the end of the root list). |
| **Delete entity** | Right-click → "Delete" OR Edit menu → "Delete Selected Entity" OR Delete key | If the entity has children, show confirmation dialog: "Delete [name] and its N children?" [Delete] [Cancel]. On confirm, entity and all descendants are removed from the World. Undo stack records the deletion. |
| **Rename entity** | Double-click entity name OR select entity and press F2 | The name field becomes an in-place editable text field. Press Enter to confirm, Escape to cancel. Empty name is rejected (reverts to previous name). Duplicate names are allowed (no uniqueness constraint) — entity IDs are used internally for all operations, names are user-facing labels only. |

**Deferred for post-MVP1:**
- Drag-reparent (drag an entity row onto another to set parent)
- Duplicate entity (Ctrl+D)
- Toggle visibility (eye icon per entity — sets a visibility flag)
- Toggle enable/disable (checkbox per entity)
- Copy/Paste entities between scenes
- Multi-select (Ctrl+click, Shift+click range)

### Panel: Inspector

**Purpose:** Display and edit the selected entity's components and properties.

**Appears in:** Scene tab, Prefab tab.

**Content (when an entity is selected):**

1. **Entity name field** at the top — editable text field. Changing it updates the entity name in the hierarchy.
2. **Transform section** — always first, always expanded. Shows:
   - **Position**: X, Y, Z float fields (drag to edit, or type directly).
   - **Rotation**: X, Y, Z float fields (euler angles in degrees for display; internally stored as quaternion). In MVP1, rotation fields are read-only (no rotate gizmo). Future: editable with rotate gizmo.
   - **Scale**: X, Y, Z float fields. In MVP1, scale fields are read-only. Future: editable with scale gizmo.
3. **Component sections** — one collapsible section per attached component. Each section header shows the component type name and a remove button (ⓧ). Expanded by default for 1–2 components; collapsed for 3+.
4. **Component properties** — each property is rendered with an appropriate editor widget based on its type:
   - `bool`: checkbox
   - `int`: integer drag/input
   - `float`: float drag/input
   - `std::string`: text input
   - `Vec2`: two float fields (X, Y)
   - `Vec3`: three float fields (X, Y, Z)
   - `Vec4`: four float fields (X, Y, Z, W)
   - `Color` (Vec3/Vec4 interpreted as RGB/RGBA): color picker + float fields
   - `Entity reference`: text field showing entity name (read-only in MVP1; future: drag-drop or picker)
   - `Asset reference`: text field showing asset path; drag-accept target for Assets panel drops
   - `Quat`: represented as euler angles (read-only in MVP1)
5. **Add Component button** at the bottom — opens a searchable dropdown listing all registered component types. Selecting one adds an instance of that component to the entity. The dropdown filters as the user types.
6. **Remove component button** (ⓧ) on each component section header. Removes the component from the entity.

**Content (when no entity is selected):**
- Empty state: "No entity selected" text centered in the panel.

**Read-only mode (during Play):**
- When the editor is in Play mode and the Scene tab shows the runtime World:
  - All property fields are rendered as read-only (grayed out, non-interactive).
  - The Inspector panel background shifts to a slightly darker gray tint as a visual read-only indicator.
  - A banner at the top of the Inspector shows a lock icon (🔒) and text: "Play mode — read only".
  - The Add Component and Remove Component buttons are hidden or disabled.
- The Game tab has no Inspector (its layout does not include one).

### Panel: Viewport (3D View)

**Purpose:** Render the scene in 3D with an editor camera for navigation and entity placement.

**Appears in:** Scene tab, Prefab tab.

**Content:**
- The 3D rendered view of the current World (editor World normally; runtime World clone during Play mode).
- **Grid overlay** on the XZ plane (Y-up). Grid lines at regular intervals (e.g., major grid every 10 units, minor every 1 unit). The grid fades with distance.
- **Debug drawing** overlaid on entities:
  - Coordinate axes on the selected entity (red=X, green=Y, blue=Z).
  - Collider outlines (wireframe) for entities with physics colliders — deferred to post-MVP1.
- **Transform gizmo** on the selected entity:
  - MVP1: translate gizmo only (three colored arrows: red=X, green=Y, blue=Z). Dragging an arrow translates the entity along that axis.
  - Gizmo uses ImGuizmo for rendering and interaction.
  - The gizmo is visible only when the editor is in Edit mode (not during Play).
- **Editor camera**: A free-fly camera that is independent of any scene camera.

**Editor camera controls (when viewport has focus):**

| Input | Action |
|---|---|
| **Right-click + drag** | Look around (yaw and pitch). The cursor is hidden and captured during drag. |
| **Right-click + W** | Fly forward (in the camera's look direction). |
| **Right-click + S** | Fly backward. |
| **Right-click + A** | Strafe left. |
| **Right-click + D** | Strafe right. |
| **Right-click + Q** | Move down (world Y axis). |
| **Right-click + E** | Move up (world Y axis). |
| **Scroll wheel** | Dolly forward/back (move along look direction). |
| **F key** (with entity selected) | Focus the editor camera on the selected entity. The camera instantly teleports (snaps) to frame the entity's bounding box. |
| **Middle-click + drag** | Pan the camera (move perpendicular to look direction). |

**Viewport rendering during Play mode:**
- The Scene tab viewport continues to show the runtime World (the clone). The editor camera can still fly around.
- The Game tab viewport shows the game camera's view (e.g., the FPS camera).
- The Scene tab viewport does NOT show the transform gizmo (read-only mode).

### Panel: Game Viewport

**Purpose:** Display the game camera's view during Play mode.

**Appears in:** Game tab only.

**Content:**
- Full-area 3D viewport showing the game camera's perspective (e.g., the FPS camera in MVP1).
- No editor camera — the view is locked to the active game camera.
- **Status overlay** (bottom strip):
  - FPS counter (e.g., "FPS: 60").
  - Entity count (e.g., "Entities: 42").
  - Play time (e.g., "Play time: 00:00:12").
  - Play state indicator: "▶ Playing" or "⏸ Paused".

**Input routing:**
- When the Game tab has focus, keyboard and mouse input are routed to the game (not the editor).
- When the Scene tab has focus during Play, input goes to the editor camera (so the user can inspect the runtime scene).
- Clicking the Game tab focuses it and captures input for the game.

### Play Mode

Play mode allows testing the scene as a game without permanently modifying the editor's World.

**Starting Play:**
1. User presses the ▶ Play button in the toolbar.
2. The editor **clones** the editor World (deep copy of all entities, components, and hierarchy).
3. The original editor World is set aside (untouched).
4. The clone becomes the **runtime World**.
5. The game loop begins running on the runtime World (Updatable components update each frame).
6. A **Game tab** opens (or is focused if already open from a previous play session).
7. The Scene tab's panels switch to **read-only runtime mode**:
   - Viewport shows the runtime World (no gizmo).
   - Hierarchy shows the runtime entities.
   - Inspector shows runtime component values (read-only).
8. A **visual indicator** appears throughout the editor:
   - The viewport border in the Scene tab and Game tab turns bright red (hex `#FF3300`), 3px thick, drawn around the inner edge of the viewport rect.
   - The window title bar shows the prefix `[Playing]` prepended to the editor window title (e.g., `[Playing] Buddd Editor — scene_01.yaml`).
   - A colored status bar is shown at the very bottom of the editor window (below all panels) with the text "🔴 PLAY MODE" on a dark-red background.
   - All Inspector fields become read-only: grayed-out text background, a lock icon (🔒) on each component header, and the Add/Remove Component buttons are hidden.
   - Toolbar buttons reflect the play state (Play disabled, Pause and Stop enabled).

**During Play:**
- The game runs on the cloned World.
- The editor World is preserved unchanged.
- The Game tab shows the game camera view and receives game input.
- The Scene tab allows read-only inspection — the editor camera can fly around, entities can be selected, component values can be viewed.
- The Console continues to show log output from the running game.
- **Undo/Redo is disabled** — the runtime world is read-only and the editor world is untouched, so there are no undoable edit operations during Play mode.

**Pausing:**
- Pressing ⏸ Pause freezes the game loop (Updatable components stop receiving `update()` calls).
- The Game tab viewport shows the last rendered frame (or a paused overlay).
- The Scene tab remains read-only but the world is static — useful for inspection.
- Pressing Pause again (or Play) resumes the game loop.

**Stopping:**
1. User presses the ⏹ Stop button (or closes the Game tab).
2. The game loop stops.
3. The runtime World (clone) is **discarded**.
4. The editor World is **restored** to its pre-Play state.
5. The Scene tab panels return to **edit mode** (Inspector editable, gizmo visible).
6. The Game tab **closes entirely**. The user returns to the Scene tab. Pressing Play again opens a fresh Game tab.
7. The visual Play mode indicators are removed.

---

### Prefab Editing

A prefab is a YAML file containing a partial entity hierarchy (type: `Prefab` in the YAML format). Prefabs are reusable templates that can be instantiated in scenes.

**Opening a prefab:**
- Double-click a `.yaml` prefab file in the Project panel.
- A new **Prefab tab** opens with the Scene tab layout (hierarchy, viewport, inspector, etc.).
- The Prefab tab's World contains only the prefab's root entity and its descendants.

**Editing a prefab:**
- The hierarchy shows the prefab's entity tree.
- The inspector allows editing component properties (same as scene editing).
- The viewport shows the prefab in isolation, with the editor camera.
- Transform gizmo works on the prefab's root entity and children.
- Creating/deleting entities within the prefab modifies the prefab's structure.

**Saving a prefab:**
- File > Save (or Ctrl+S) while the Prefab tab is focused saves the prefab to its `.yaml` file via `SceneSaver`.
- Closing a dirty Prefab tab prompts to save.
- The Prefab tab title shows an asterisk (`*`) when dirty.

**Prefab tab and Play mode:**
- The Prefab tab does NOT participate in Play mode. Only the Scene tab's World is cloned for play.
- The Prefab tab's toolbar does NOT show Play/Pause/Stop buttons (or they are disabled).
- There is no isolated Play button for Prefabs in MVP1. Prefab editing is purely an edit-time activity. Playing always runs the main scene.

---

### Tab Layouts (Visual Reference)

#### Scene Tab Layout

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
- Scene Panel: left dock, ~250px default width
- Viewport: center dock, fills remaining space
- Inspector: right dock, ~300px default width
- Bottom area: tab bar with three tabs — **Project**, **Console**, **Assets** (one visible at a time, ~200px default height). The tab bar is always visible; the user switches between tabs by clicking the header.

All panel sizes are user-resizable and remembered per tab type (session-only).

#### Prefab Tab Layout

**Identical structure to the Scene tab layout** (hierarchy panel left, viewport center, inspector right, bottom area with Project/Console/Assets tabbed together), but:
- The hierarchy shows only the prefab's entity tree.
- The tab title shows the prefab filename.
- The toolbar does NOT include Play/Pause/Stop buttons (or they are disabled).
- Saving writes to the prefab's `.yaml` file.

#### Game Tab Layout

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
- Game Viewport: fills entire tab area
- Status bar: bottom strip, ~24px height, non-resizable

When detached to a separate monitor, the Game tab becomes a full standalone game window.

---

### Cross-Panel Communication

#### Entity Selection Flow

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

**MVP1 selection paths:**
1. Hierarchy click → Inspector + Viewport update (fully implemented).
2. Gizmo interaction → Inspector Transform fields update in real-time (drag feedback).
3. Viewport click-to-select (mouse picking) is **deferred to post-MVP1**.

#### Play Mode State Transitions

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

**Panel state per mode:**

| Panel | Edit Mode | Play Mode | Paused |
|---|---|---|---|
| Scene Panel (Hierarchy) | Editable (select, create, delete, rename) | Read-only (select only, view runtime entities) | Read-only (frozen) |
| Inspector | Editable (all fields, add/remove components) | Read-only (view only, visual indicator) | Read-only (frozen) |
| Viewport (Scene tab) | Editor camera, gizmo visible | Editor camera (no gizmo, view runtime) | Editor camera (frozen world) |
| Game Viewport (Game tab) | Not visible / placeholder | Active (game camera, receives input) | Frozen (last frame shown) |
| Console | Active (editor logs) | Active (game + editor logs) | Active |
| Project Panel | Interactive | Interactive | Interactive |
| Assets Panel | Interactive | Interactive | Interactive |
| Toolbar | Play enabled, Stop/Pause disabled | Play disabled, Stop/Pause enabled | Play/Stop enabled, Pause disabled |
| Edit > Undo/Redo | Enabled (when undo stack non-empty) | Disabled (runtime world read-only) | Disabled |

---

## Key entities

The following domain concepts underpin the editor UX. They exist in the engine already (the editor consumes them). **Exception:** `World::clone()` (deep copy of all entities, components, and hierarchy) is a required engine-level capability for Play mode that does **not yet exist** — see A-03 for details.

| Entity | Description |
|---|---|
| **World** | Container holding all entities, their hierarchy, and the component registry. The editor has an "editor World" (the one being edited) and a "runtime World" (the clone used during Play mode). |
| **Entity** | A node in the hierarchy with a name, Transform, and zero or more Components. Each entity has a unique `EntityId` within its World. |
| **Transform** | Position (Vec3), rotation (Quat), and scale (Vec3). Local transform relative to parent; world transform computed from hierarchy. |
| **Component** | A data container attached to an entity. Examples: `MeshRenderer`, `CameraComponent`, `DirectionalLightComponent`, `FreeCameraMovement`. Components are introspectable via `ComponentRegistry`. |
| **SceneLoader** | Parses YAML scene/prefab files and populates a `World` with entities and components. |
| **SceneSaver** | Serializes a `World` back to YAML, respecting entity source types (scene entities expanded, prefab/model entities referenced). |
| **ComponentRegistry** | Runtime registry of all component types, providing factory functions, property descriptors, and type metadata for the Inspector. |
| **TypeRegistry** | Static registry mapping C++ types to YAML encode/decode, string conversion, and validation. Enables generic property editing in the Inspector. |
| **AssetManager** | Centralized asset loading, caching, and hot-reload. Provides asset references for the Inspector's asset-reference fields. |
| **Editor** | The top-level editor class (already scaffolded) that owns tab management, panel layout, and the editor World. |
| **Tab** | An editing context with a type (Scene, Prefab, Game), a World, and a fixed panel layout. |

## User stories

### Story 1 — Create and navigate a scene (Priority: P1)

As a content creator, I want to create a new empty scene and navigate its 3D viewport, so that I can start building a game level.

**Given** the editor is running with the default Scene tab
**When** I select File > New Scene
**Then** the viewport clears to show an empty world with the ground grid
**And** the hierarchy panel shows no entities
**And** the Scene tab title shows "Untitled*" (dirty indicator)

**Given** the editor has an empty scene open
**When** I right-click and drag in the viewport
**Then** the editor camera rotates to look around
**And** WASD keys (with right-click held) move the camera through the scene

### Story 2 — Edit entities via hierarchy and inspector (Priority: P1)

As a content creator, I want to create, select, rename, and delete entities, so that I can build a scene hierarchy.

**Given** the editor has a scene open
**When** I right-click in the hierarchy panel and select "Create Empty"
**Then** a new entity named "Entity" is created (as the **last child** of the selected entity if one is selected; at root level otherwise, appended at the end of the root list)
**And** the entity is selected (highlighted in hierarchy)
**And** the Inspector shows the entity's Transform and no components

**Given** an entity is selected in the hierarchy
**When** I double-click the entity name
**Then** the name becomes an editable text field
**And** pressing Enter confirms the new name
**And** the hierarchy updates to show the new name

**Given** an entity is selected
**When** I press the Delete key (or right-click > Delete)
**Then** the entity is removed from the hierarchy
**And** the Inspector clears (no entity selected)

**Given** an entity with children is selected
**When** I press Delete
**Then** a confirmation dialog appears: "Delete [name] and its N children?"
**And** clicking Delete removes the entity and all descendants

### Story 3 — Modify component properties (Priority: P1)

As a content creator, I want to edit entity component properties in the Inspector, so that I can configure entity behavior.

**Given** an entity with a `MeshRenderer` component is selected
**When** I view the Inspector
**Then** the Transform section is shown first (Position X/Y/Z fields)
**And** the MeshRenderer section is shown with its properties (e.g., model reference)
**And** changing a value in the Inspector immediately updates the entity in the viewport

**Given** an entity is selected
**When** I click the "+ Add Component" button at the bottom of the Inspector
**Then** a searchable dropdown appears listing all registered component types
**And** typing filters the list
**And** clicking a component type adds it to the entity
**And** the new component section appears in the Inspector

### Story 4 — Place and manipulate entities with the gizmo (Priority: P1)

As a content creator, I want to use the translate gizmo to position entities in the 3D viewport, so that I can compose my scene visually.

**Given** an entity is selected and the editor is in Edit mode
**When** I look at the viewport
**Then** a translate gizmo (three colored arrows) appears on the selected entity
**And** dragging the red arrow moves the entity along the X axis
**And** the Inspector's Transform Position fields update in real-time during the drag

### Story 5 — Play mode: test the game (Priority: P1)

As a content creator, I want to press Play and test my scene as a game, so that I can verify gameplay behavior.

**Given** the editor has a scene open with a player entity and FPS camera
**When** I press the ▶ Play button in the toolbar
**Then** a Game tab opens showing the game camera's view
**And** the editor window shows a visual Play mode indicator (colored border/banner)
**And** the Scene tab hierarchy and inspector switch to read-only mode
**And** the game runs — I can move and look around in the Game tab

**Given** the editor is in Play mode
**When** I press the ⏹ Stop button
**Then** the Game tab closes
**And** the editor returns to Edit mode
**And** the original scene is restored (no changes from gameplay persist)

### Story 6 — Save and open scenes (Priority: P1)

As a content creator, I want to save my scene to a file and open existing scenes, so that my work persists between sessions.

**Given** the editor has a modified (dirty) scene
**When** I select File > Save Scene
**Then** if the scene has a file path, it is saved and the dirty indicator clears
**And** if the scene is untitled, a Save As dialog opens

**Given** the editor has a scene open
**When** I select File > Open Scene and choose a `.yaml` file
**Then** if the current scene is dirty, a save prompt appears
**And** the selected scene loads and replaces the current scene
**And** the hierarchy and viewport update to show the new scene's content

### Story 7 — Browse project files (Priority: P2)

As a content creator, I want to browse my project's file system within the editor, so that I can find and open assets quickly.

**Given** the editor is running
**When** I look at the Project panel
**Then** I see a file tree rooted at the project directory
**And** `.yaml`, `.gltf`, `.glb`, `.png`, and `.jpg` files are visible

**Given** the Project panel is visible
**When** I double-click a `.yaml` scene file
**Then** the scene loads in the Scene tab (with save prompt if dirty)

**Given** the Project panel is visible
**When** I double-click a `.yaml` prefab file
**Then** a new Prefab tab opens with the prefab's content

### Story 8 — Edit prefabs in isolation (Priority: P2)

As a technical artist, I want to open a prefab in its own tab and edit it in isolation, so that I can refine reusable assets.

**Given** a `.yaml` prefab file exists in the project
**When** I double-click it in the Project panel
**Then** a new Prefab tab opens showing the prefab's entity hierarchy
**And** the viewport shows only the prefab's contents
**And** I can edit the prefab's entities and components
**And** saving (File > Save) writes back to the prefab's `.yaml` file

### Story 9 — Detach a tab to a separate window (Priority: P2)

As a content creator with multiple monitors, I want to detach the Game tab to my second monitor, so that I can see both the editor viewport and the game view simultaneously.

**Given** the Game tab is open during Play mode
**When** I right-click the Game tab header and select "Detach Tab"
**Then** the Game tab becomes a new OS window on my desktop
**And** the window contains the full Game tab layout (viewport + status bar)
**And** I can move this window to my second monitor
**And** the main editor window continues to show the Scene tab

### Story 10 — View engine logs in the Console (Priority: P2)

As an engine developer, I want to see engine and editor log output in the Console panel, so that I can debug issues.

**Given** the editor is running
**When** the engine logs an info message
**Then** it appears in the Console panel as a white line with a timestamp

**Given** the Console has many log lines
**When** I toggle the "Error" filter on (and others off)
**Then** only error-level messages are displayed

**Given** the Console is auto-scrolling
**When** I click the lock toggle
**Then** the Console stops scrolling to new messages
**And** I can scroll back through history

### Story 11 — Undo entity deletion (Priority: P3)

As a content creator, I want to undo an accidental entity deletion, so that I don't lose work.

**Given** I just deleted an entity
**When** I select Edit > Undo (or press Ctrl+Z)
**Then** the entity is restored to the hierarchy at its original position
**And** all its components and children are restored
**And** the Inspector shows the restored entity

### Story 12 — Dock and arrange panels (Priority: P3)

As a content creator, I want to resize and rearrange panels within a tab's layout, so that I can customize my workspace.

**Given** the Scene tab is active
**When** I drag the divider between the hierarchy and viewport
**Then** the panels resize proportionally
**And** the new sizes are remembered for future Scene tabs in this session

### Story 13 — Keyboard shortcut for quick actions (Priority: P3)

As a power user, I want to use keyboard shortcuts for common actions, so that I can work faster.

**Given** the editor is in Edit mode with a scene open
**When** I press Delete (with an entity selected)
**Then** the entity is deleted

**Given** the editor is in Edit mode
**When** I press Ctrl+S
**Then** the current scene is saved

**Given** the editor is in Edit mode
**When** I press Ctrl+Z
**Then** the last undoable action is undone

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | The editor opens to a Scene tab with the Scene tab layout (hierarchy left, viewport center, inspector right, bottom area with Project/Console/Assets tabs). The bottom tab bar shows **Project | Console | Assets**; clicking each switches the bottom content area. | Manual: launch `buddd edit`, verify hierarchy, viewport, and inspector are visible. Verify bottom tab bar shows Project/Console/Assets and clicking each switches content. |
| AC-002 | The hierarchy panel displays the entity tree with parent/child indentation and entity names. Clicking an entity row selects it (highlighted). | Manual: load a scene with entities, verify tree rendering, verify click selection. |
| AC-003 | Right-click > "Create Empty" in the hierarchy creates a new entity. If a single entity is selected, the new entity is appended as the **last child** of the selected entity. Otherwise, it goes to root level (appended at the end of the root list). The entity appears in the tree, is selected, and shows in the Inspector with default Transform. | Manual: create entity with and without selection, verify placement (last child position). |
| AC-004 | Double-clicking an entity name in the hierarchy enables in-place rename. Enter confirms; Escape cancels. Empty name is rejected. Duplicate names are allowed. | Manual: rename entity, verify name updates in hierarchy and inspector. Try duplicate name, verify it's accepted. |
| AC-005 | Pressing Delete on a selected entity removes it from the hierarchy. If the entity has children, a confirmation dialog appears. | Manual: delete entity with children, verify dialog, verify removal. |
| AC-006 | The Inspector shows the selected entity's name (editable), Transform (position X/Y/Z editable, rotation/scale read-only in MVP1), and each component's properties with type-appropriate editors. | Manual: select entity with components (e.g., MeshRenderer, Light), verify all property fields render correctly. |
| AC-007 | The "+ Add Component" button opens a searchable dropdown of registered component types. Selecting one adds it to the entity. | Manual: add a component to an entity, verify it appears in the inspector and is reflected in the viewport if applicable. |
| AC-008 | The viewport renders the 3D scene with a ground-plane grid. The editor camera can be controlled with right-click + WASD (fly), right-click + mouse (look), scroll wheel (dolly), and F key (focus). | Manual: navigate the viewport with each control scheme, verify expected camera movement. |
| AC-009 | A translate gizmo (three colored arrows) appears on the selected entity in the viewport during Edit mode. Dragging an arrow translates the entity along that axis. | Manual: select entity, drag each gizmo arrow, verify entity moves in viewport and inspector position fields update. |
| AC-010 | Pressing ▶ Play clones the editor World, opens a Game tab showing the game camera view, applies a visual Play mode indicator (viewport border turns `#FF3300` 3px, title bar shows `[Playing]` prefix, status bar shows "🔴 PLAY MODE" on dark-red background, Inspector fields become read-only with gray background and lock icons), and switches Scene tab panels to read-only. | Manual: press Play, verify Game tab opens, verify `#FF3300` border on viewport, verify `[Playing]` prefix in title bar, verify Inspector read-only (gray fields, lock icons, hidden Add/Remove buttons). |
| AC-011 | During Play mode, the Scene tab viewport shows the runtime world without the gizmo. The editor camera can still navigate for inspection. | Manual: enter Play mode, navigate Scene tab viewport, verify no gizmo, verify runtime world is visible. |
| AC-012 | During Play mode, the Game tab viewport shows the game camera view and receives input when focused. Status overlay shows FPS, entity count, and play time. | Manual: enter Play mode, click Game tab, move mouse/look around, verify status overlay. |
| AC-013 | Pressing ⏸ Pause freezes the game loop. Game tab viewport shows frozen frame. Pressing ▶ Play resumes. | Manual: pause during Play, verify game stops, verify resume works. |
| AC-014 | Pressing ⏹ Stop discards the runtime World clone, restores the editor World, closes the Game tab, and returns all panels to Edit mode. | Manual: stop Play mode, verify editor World matches pre-Play state exactly (entities, transforms, component values). |
| AC-015 | File > New Scene creates an empty scene (clears hierarchy, resets viewport, shows "Untitled*" tab title). | Manual: create new scene, verify empty state. |
| AC-016 | File > Open Scene opens an OS file dialog, loads the selected `.yaml` file, and replaces the current scene. If the current scene is dirty, a save prompt dialog appears (Save / Don't Save / Cancel). | Manual: open a different scene with a dirty current scene, verify all three dialog options behave correctly. |
| AC-017 | File > Save Scene saves the current scene to its file path. If untitled, behaves like Save As (opens dialog). The dirty indicator clears after save. | Manual: save a scene, verify file is written, verify dirty indicator clears. |
| AC-018 | File > Save Scene As opens an OS file dialog, saves the scene to the chosen path, and updates the scene's file path. | Manual: save as to a new path, verify new file exists on disk. |
| AC-019 | Double-clicking a `.yaml` scene file in the Project panel loads it in the Scene tab (with save prompt if dirty). | Manual: double-click scene in project panel, verify it loads. |
| AC-020 | Double-clicking a `.yaml` prefab file in the Project panel opens a new Prefab tab with the prefab's content (hierarchy, viewport, inspector). | Manual: double-click prefab in project panel, verify Prefab tab opens with correct content. |
| AC-021 | Saving while a Prefab tab is focused saves the prefab to its `.yaml` file. The Prefab tab's dirty indicator clears. | Manual: modify prefab, save, verify file updated. |
| AC-022 | The Console panel displays engine/editor log lines color-coded by severity. Filter controls (channel, level) affect which lines are visible. Clear button empties the display. | Manual: trigger various log levels, verify color coding, verify filters work. |
| AC-023 | The Project panel displays a file tree of the project directory, filtered to relevant asset types. Right-click context menu provides Open, Delete, Rename, Show in Explorer actions. | Manual: browse project, verify file visibility, test context menu actions. |
| AC-024 | Panels within a tab can be resized by dragging dividers. New sizes are remembered for the session. | Manual: resize panels, switch tabs and back, verify sizes persist. |
| AC-025 | The File > Quit menu item prompts to save if any tab (Scene or Prefab) is dirty, then exits the application. | Manual: quit with unsaved changes, verify prompt, verify exit. |
| AC-026 | All toolbar buttons (Play, Pause, Stop, Translate mode, Grid toggle) are correctly enabled/disabled based on editor state (Edit vs Play vs Pause). | Manual: verify button states across all mode transitions. |
| AC-027 | Closing a dirty Prefab tab prompts to save (Save / Don't Save / Cancel). | Manual: close dirty prefab tab, verify prompt. |
| AC-028 | The Scene tab cannot be closed (close button hidden or disabled). | Manual: verify no close button on Scene tab. |

## E2E Verification

- **Method 1: Manual smoke test (display required)** — Launch `buddd edit`. Walk through the following script:
   1. Verify Scene tab layout: hierarchy (left), viewport (center), inspector (right), bottom tab bar with Project/Console/Assets tabs.
  2. Create 3 entities in the hierarchy (Create Empty). Rename one. Delete one (undo via Ctrl+Z).
  3. Add a Light component to an entity via "+ Add Component". Verify properties appear.
  4. Navigate the viewport with editor camera controls. Verify grid and entity visibility.
  5. Select an entity. Verify gizmo appears. Drag translate arrow. Verify movement.
  6. Save the scene (Save As). Verify file exists on disk.
  7. Open a different scene (Open Scene). Verify it replaces the current scene.
  8. Press Play. Verify Game tab opens, game camera view visible, Scene tab read-only.
  9. Press Pause. Verify game loop freezes.
  10. Press Stop. Verify editor World is restored, Game tab closes, edit mode returns.
  11. Open a prefab from the Project panel. Verify Prefab tab opens. Edit and save. Verify file updated.
  12. Verify Console shows log output. Test filter controls.
  13. Quit (File > Quit). Verify clean exit.

- **Method 2: Automated headless tests** — While the full editor UX cannot be tested headless (no GPU, no ImGui rendering), the following can be tested in CI:
  - World clone/discard cycle (simulates Play/Stop at the engine level).
  - Entity CRUD operations via the engine's World API.
  - SceneLoader/SceneSaver round-trip (save scene, load it, verify entity tree matches).
  - Component property get/set via ComponentRegistry/TypeRegistry introspection.
  - Dirty state tracking logic in the editor model layer.

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A new user can create a scene, add entities, configure components, and enter Play mode within 5 minutes of launching the editor, without reading documentation. | Timed user test with a task script. |
| SC-002 | The editor restores the editor World to its exact pre-Play state 100% of the time (no state leakage from gameplay). | Automated test: create scene with known state → Play → modify runtime state → Stop → verify editor World matches snapshot. |
| SC-003 | All panel interactions (hierarchy selection, inspector editing, viewport navigation) respond within 1 frame (no visible lag or stuttering). | Visual inspection; frame timing verified by FPS counter in Console. **Note:** This is an aspirational target without formal resource budgets (GPU, CPU, memory). Sub-millisecond timings may vary by hardware; subjective "no stutter" is the minimum bar. |
| SC-004 | The editor window opens and renders all panels within 3 seconds on a development machine (measured from `buddd edit` invocation to first complete frame). | Manual timing with stopwatch; automated startup time measurement in CI. **Note:** This is an aspirational target; a concrete baseline hardware spec and measurement methodology must be defined in the implementation phase. |
| SC-005 | No engine code changes are required to add a new component to the Inspector — it appears automatically via ComponentRegistry introspection. | Developer creates a new component type, registers it, opens editor, verifies it appears in Add Component dropdown and renders properties. |

## Edge cases

| ID | Case | Expected behavior |
|---|---|---|
| EC-01 | **Empty scene (no entities)**: User opens a scene with zero entities. | Hierarchy shows empty state ("No entities in scene"). Inspector shows "No entity selected". Viewport shows empty world with grid. All operations (Play, Save, etc.) still work. |
| EC-02 | **Very deep hierarchy**: Scene has entities nested 50+ levels deep. | Hierarchy tree renders with indentation. Scroll bar appears. Performance remains acceptable (no frame drops from tree rendering). |
| EC-03 | **Very large scene**: Scene has 10,000+ entities. | Hierarchy may show only top-level entities with lazy expansion. Viewport renders normally (GPU handles draw calls). Inspector works for selected entity. Play mode clone takes longer but completes. |
| EC-04 | **Play mode with no game camera**: Scene has no active `CameraComponent`. | Game tab shows a black screen or placeholder text: "No active camera. Add a CameraComponent to an entity." The game loop still runs. |
| EC-05 | **Rapid Play/Stop cycling**: User presses Play and Stop in quick succession (before the clone finishes). | Play request queues; Stop cancels any in-progress transition. Editor remains in a consistent state. World is not corrupted. |
| EC-06 | **Panel collapsed to zero size**: User drags panel divider all the way to the edge. | Panel collapses but a thin grab handle remains visible. User can drag it back to restore the panel. Minimum panel size of ~20px enforced. |
| EC-07 | **Window minimized during Play**: User minimizes the editor while the game is running. | Game continues running (updates still tick). When restored, the Game tab viewport renders the current frame. No crash or state loss. |
| EC-08 | **Scene file deleted externally**: The open scene's `.yaml` file is deleted from disk while open in the editor. | Save operation fails with error: "File not found: [path]". Editor marks scene as dirty. User can Save As to a new path. Logged to Console as error. |
| EC-09 | **Prefab file deleted externally**: A Prefab tab is open and the source `.yaml` file is deleted. | Similar to EC-08 — save fails, user can Save As. Tab title may show a warning icon. |
| EC-10 | **Dirty scene on quit via OS**: User closes the editor window via the X button (not File > Quit) with a dirty scene. | Same save prompt as File > Quit. If user cancels, window close is aborted. |
| EC-11 | **Multiple prefab tabs open**: User has 10+ prefab tabs open simultaneously. | Tab bar scrolls or shows a dropdown for overflow tabs. Each tab's World is independent. Memory usage scales with number of open prefabs. |
| EC-12 | **Component with no registered properties**: A component type is registered but has zero properties. | Inspector shows the component section with just the header (name + remove button). No empty property list clutter. |
| EC-13 | **Circular hierarchy**: User somehow creates a parent-child cycle (should be prevented by engine). | Engine's `World` API rejects circular parent assignments. Editor hierarchy panel should never show a cycle. |
| EC-14 | **Undo during Play mode**: User attempts Edit > Undo (Ctrl+Z) while the editor is in Play mode. | Undo is disabled during Play mode — the runtime world is read-only and the editor world is untouched. The Undo menu item is grayed out. |

## Error cases

| ID | Case | Expected behavior |
|---|---|---|
| ER-01 | **Scene file fails to load** (corrupt YAML, missing version, unknown component types). | Error dialog: "Failed to load scene: [error message]". Scene tab remains on the previous scene (or empty if no previous scene). Error logged to Console. |
| ER-02 | **Scene file fails to save** (disk full, permission denied, path does not exist). | Error dialog: "Failed to save scene: [error message]". Scene remains open and dirty. Error logged to Console. |
| ER-03 | **Prefab file fails to save** (same as ER-02 but for prefab tabs). | Error dialog on the Prefab tab. Prefab remains open and dirty. |
| ER-04 | **World clone fails during Play** (out of memory). | Error dialog: "Failed to start Play mode: [error message]". Editor remains in Edit mode. Game tab does not open. |
| ER-05 | **Double-click non-scene, non-prefab `.yaml` file** in Project panel. | If the file is not a valid Buddd scene or prefab (wrong `type:` field), the SceneLoader returns an error. Error message displayed: "File is not a valid scene or prefab: [path]". |
| ER-06 | **Edit > Undo with empty undo stack**. | No action. Undo menu item is disabled (grayed out) when stack is empty. |
| ER-06a | **Edit > Undo during Play mode**. | Undo/Redo menu items are disabled (grayed out) during Play and Pause modes. Keyboard shortcut (Ctrl+Z) is ignored. |
| ER-07 | **Add Component when entity selection is lost** (race condition). | The component is not added. No crash. Inspector shows "No entity selected". |
| ER-08 | **Gizmo drag on read-only entity** (during Play mode). | Gizmo is not visible during Play mode, so this cannot occur. If somehow triggered, the operation is rejected silently. |
| ER-09 | **Invalid input in Inspector property field** (e.g., text in a float field). | The field rejects the input (reverts to previous valid value). Field may flash red briefly. |
| ER-10 | **Console overwhelmed with messages** (e.g., 10,000 messages/second). | Console limits visible lines (e.g., last 1,000). Older lines are discarded from the display buffer. Performance should not degrade significantly. A warning may be shown: "Console message rate exceeded — some messages discarded." |
| ER-11 | **Missing asset reference** in Inspector (asset file was deleted). | The asset reference field shows "[Missing: path]" in red text. Entity still functions — just the referenced asset won't render. |

## Permissions and security

- The editor requires no elevated privileges (root/admin) to run.
- The editor accesses the file system only within the project directory (for loading/saving scenes, prefabs, and assets). It does not access user data outside the project.
- The editor does not require network access at runtime.
- File dialogs (Open, Save As) use OS-native dialogs, which are sandboxed by the OS.
- The architecture boundary (ADR-019) is preserved: no SDL3, OpenGL, or GLM headers are included from editor source files. All platform and graphics access goes through engine abstractions.
- ImGui's internal state is confined to the engine's `engine_imgui` module.

## Observability

| Signal | Source | When |
|---|---|---|
| Editor launched | Console (info) | On `buddd edit` startup: "Editor started." |
| Scene loaded | Console (info) | After successful SceneLoader call: "Scene loaded: [path] (N entities)" |
| Scene saved | Console (info) | After successful SceneSaver call: "Scene saved: [path]" |
| Scene load/save error | Console (error) + dialog | On SceneLoader/SceneSaver error: "Failed to [load/save] scene: [error]" |
| Play mode started | Console (info) | "Play mode started. Entities: N" |
| Play mode stopped | Console (info) | "Play mode stopped. Editor world restored." |
| Entity created | Console (debug) | "Entity created: [name] (id: [id])" |
| Entity deleted | Console (debug) | "Entity deleted: [name] (id: [id])" |
| Component added | Console (debug) | "Component [type] added to [entity name]" |
| Component removed | Console (debug) | "Component [type] removed from [entity name]" |
| Tab opened/closed | Console (debug) | "Tab [type] opened: [title]" / "Tab closed: [title]" |
| Panel resize | None (not logged) | — |
| Editor exit | Console (info) | "Editor shutting down." |

**Log channels used by the editor:**
- `Editor` — general editor lifecycle events
- `Editor:Scene` — scene load/save/create operations
- `Editor:Play` — play mode transitions
- `Editor:Entity` — entity CRUD operations
- `Editor:UI` — panel/tab/layout events

All editor log messages use the `BUDDD_LOG_*` macros with appropriate channel tags, consumed by the engine's logging system and displayed in the Console panel.

## Out of scope

This section repeats and expands on the Non-goals for clarity. These are explicitly excluded from MVP1 and deferred to future sprints or releases.

| Area | Exclusion |
|---|---|
| **Gizmos** | Rotate and Scale gizmos (translate only). |
| **Viewport interaction** | Mouse picking (click-to-select in viewport). |
| **Hierarchy** | Drag-and-drop reparenting, multi-select, copy/paste between scenes. |
| **Inspector** | Custom component editors (e.g., curve editor, gradient picker). Entity/asset reference picker (read-only text field only). |
| **Prefabs** | Override system (instance overrides, revert, apply to base). Nested prefabs. Prefab variants. |
| **Undo/Redo** | Undo beyond basic entity deletion undo. No undo for property changes, component add/remove, or transform edits. |
| **Asset browser** | Thumbnails, metadata database, import pipeline UI, search bar, favorites. |
| **Project panel** | Favorites, recent files, search, file preview. |
| **Play mode** | Pause-mode detailed inspection (component value editing during pause). Frame stepping. Hot-reload (runtime to editor sync). |
| **Multi-scene** | Additive scene loading. Sub-scenes. |
| **Editor customization** | Themes beyond default dark. Custom panel layouts (adding/removing panels from tab types). Custom keyboard shortcuts. |
| **Tools** | Code editor, script editor, debugger integration, profiler integration. |
| **Platform** | Mobile/console target preview. Multiplayer testing. |
| **Rendering** | Viewport render modes (wireframe, unlit, lighting only). Post-processing preview. |
| **Animation** | Animation preview, timeline, animation state machine editor. |
| **Physics** | Physics debug draw (collider outlines in viewport). |
| **Build** | Build/publish workflow. Package manager. |

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | The existing `Editor` class scaffold (from the editor-scaffolding feature) serves as the entry point for all editor functionality. This spec defines the UX that extends it. |
| A-02 | ImGui's docking branch (`v1.91.8-docking`) provides all necessary panel docking, resizing, and tabbing infrastructure. The layout definitions in this spec map to ImGui dockspace node splits. |
| A-03 | The engine's `World` class supports deep cloning (all entities, components, and hierarchy copied). **⚠️ PREREQUISITE NOT YET IMPLEMENTED:** A search of the engine source confirms that no `World::clone()` or deep-copy mechanism exists. This is a required engine-level prerequisite that must be implemented before Play mode can function. A separate feature spec must be created for `World::clone()` before any Play mode implementation begins. |
| A-04 | The `ComponentRegistry` and `TypeRegistry` APIs provide sufficient introspection for the Inspector to dynamically render property editors for any registered component type. |
| A-05 | ImGuizmo (or equivalent) provides translate gizmo rendering and interaction. It integrates with the existing ImGui render pipeline. |
| A-06 | OS-native file dialogs are available via a platform abstraction (e.g., SDL3 dialog or a minimal native dialog helper). The editor does not implement its own file browser for Open/Save operations. |
| A-07 | The editor's "project directory" is the current working directory from which `buddd edit` is launched. No `.budddproject` file or explicit project root configuration is required for MVP1. |
| A-08 | Panel size persistence is session-only (in memory, lost on restart). Persisting panel sizes to disk is deferred to post-MVP1. |
| A-09 | The undo stack is limited to the most recent deletion in MVP1 (single-level undo for entity deletions only). Undo for property changes, component operations, and other actions is deferred. |
| A-10 | The ground-plane grid in the viewport is rendered as part of the editor's debug draw layer — it is not a scene entity. |
| A-11 | Detached tabs (multi-window) are implemented using OS-level windows with their own GL context — not via ImGui's `ViewportsEnable` flag. Each detached tab creates a new SDL window. |
| A-12 | The Scene tab's read-only mode during Play is enforced at the editor UI level (fields disabled, buttons hidden) — not at the engine API level. The engine does not need to know about editor mode. |
| A-13 | The entity hierarchy tree in the Scene panel is rendered using ImGui's tree node API (`ImGui::TreeNodeEx`), with one tree node per entity and child nodes for descendants. |

## Documentation to update

The following existing documentation must be updated when this proposal is accepted and implementation begins:

| Document | Update needed |
|---|---|
| `docs/wiki/architecture/overview.md` | Likely needs a reference to the editor (architecture boundary, how the editor fits into the engine). |
| `docs/wiki/engineering/setup.md` | May need editor-specific setup instructions (e.g., ImGui dependencies, build flags). |
| `docs/adr/ADR-027-editor-architecture.md` | Record the UX decisions from this spec (panel layout, tab system, play mode behavior). |
| `docs/wiki/editor/editor-panels.md` (new page) | Create a new wiki page documenting the panel layout reference (panel types, positions, default sizes, tab behaviors). |

These updates are **not** required for the spec to be accepted, but must be tracked and completed during the implementation phase.

## Open questions

All open questions from the initial draft have been resolved. Decisions are recorded below.

| ID | Question | Decision |
|---|---|---|
| Q-01 | When creating a new entity (Create Empty), should it be placed at root level or as child of selected? | **Child of selected entity.** If a single entity is selected, the new entity becomes its child (at local origin). If nothing is selected or multiple entities are selected, the new entity goes to root level. |
| Q-02 | Must entity names be unique among siblings? | **No uniqueness constraint.** Entities can have duplicate names. The editor uses entity IDs internally for all operations; names are user-facing labels only. |
| Q-03 | Should the editor camera "F to focus" be an instant snap or smooth animation? | **Instant snap.** The camera teleports immediately to frame the selected entity. No smooth animation. |
| Q-04 | After pressing Stop, should the Game tab close entirely or stay open? | **Close entirely.** The Game tab disappears when Stop is pressed. The user returns to the Scene tab. Pressing Play again opens a fresh Game tab. |
| Q-05 | Should the Prefab tab have its own Play button for isolated testing? | **No Play for Prefabs (MVP1).** The Prefab tab is edit-only. Play always runs the main scene. Isolated prefab testing is deferred to post-MVP1. |
| Q-06 | How should detached tabs be triggered? | **Right-click context menu on tab header.** The context menu has a "Detach Tab" option. Drag-to-detach may be added post-MVP1. |
| Q-07 | Should the Console persist log messages across mode transitions? | **Persist, never auto-clear.** Console keeps all messages across Edit/Play/Pause/Stop transitions. Only the explicit Clear button removes messages. |
| Q-08 | What visual indicator does the Inspector show during read-only mode? | **Clear visual indicator:** panel background shifts to darker gray tint, a lock icon (🔒) and "Play mode — read only" banner appear at the top, all property fields are grayed out and non-interactive, Add/Remove Component buttons are hidden/disabled. |
| Q-09 | Does the Prefab tab use the same panel layout as the Scene tab? | **Yes, identical layout.** Prefab tab has the same panel structure: hierarchy left, viewport center, inspector right, bottom area with Project/Console/Assets tabs. |
| Q-10 | Is Undo/Redo available during Play mode? | **Disabled during Play mode.** The runtime world is read-only and the editor world is untouched, so there are no undoable edit operations. Undo/Redo menu items are grayed out during Play and Pause. |
