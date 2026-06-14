# Editor — Progressive Feature Breakdown (Spec-Driven Development)

> **Source:** `.specs/sprint-2026-06/editor-ux-design/spec.md`
> **Design ADR:** `docs/adr/ADR-029-editor-ux-decisions.md`
> **Wiki references:** `docs/wiki/editor/editor-panels.md`, `docs/wiki/editor/scene-management.md`, `docs/wiki/editor/cross-panel-communication.md`

Each feature is independently spec-able through the full SSD workflow:
spec → critic → contract → critic → human validation → implement → review → governance

---

## Phase 0 — Editor Foundation

### F-00: Editor Scene State

**Goal:** The `Editor` class owns a `World*` and manages its lifecycle. No panels use it yet — this is the plumbing.

**What it does:**
- `Editor` gets a `std::unique_ptr<World>` member, created on `setup()` and destroyed on `shutdown()`
- The World is empty on editor launch
- `Editor` exposes `world()` accessor for panels to use later
- Clean destruction path

**Dependencies:** Editor class already scaffolded
**Effort:** ~1 day
**Result:** Editor has an empty World. Verifiable by checking `world()->entity_count() == 0`.

---

### F-01: Scene Load/Save via File Menu

**Goal:** File > Open loads a YAML scene into the Editor's World. File > Save / Save As persists it. Dirty state tracked.

**What it does:**
- **File > Open Scene** → OS file dialog (`.yaml` filter) → `SceneLoader::load()` into Editor's World
- **File > Save Scene** → `SceneSaver::save()` from Editor's World to its file path
- **File > Save Scene As** → OS file dialog, save to chosen path
- **Dirty state** → Tracked per scene. `*` in tab title when dirty. Prompt on Open/Quit if dirty.
- **Untitled scenes** → "Untitled" name until first Save As
- **Quit** → Prompt save if dirty, then exit

**Dependencies:** F-00 (Editor needs a World)
**Effort:** ~2-3 days
**Result:** Load a `.yaml` scene from disk, edit nothing, save it back. Round-trip verified.

---

## Phase 1 — Hierarchy & Selection

### F-02: Scene Panel — Entity Tree

**Goal:** ScenePanel renders the entity hierarchy tree from the Editor's World.

**What it does:**
- ScenePanel reads `Editor::world()` each frame
- Renders an ImGui tree: root entities at top level, children indented
- Each row shows entity name (`ImGui::TreeNode` or custom tree)
- Expandable/collapsible hierarchy
- Empty state: "No entities" text when World is empty

**Dependencies:** F-00 (needs Editor's World)
**Effort:** ~1-2 days
**Result:** Open a scene, see entities in a tree with correct parent/child indentation.

---

### F-03: Entity Selection

**Goal:** Click an entity in ScenePanel → it becomes selected. Inspector clears when nothing selected.

**What it does:**
- Single-click on entity row → sets `selected_entity_id_` on Editor
- Click empty area → clears selection
- Selected entity row highlighted in ScenePanel
- `Editor::selected_entity()` accessor for other panels
- Empty selection state → Inspector shows "No entity selected"

**Dependencies:** F-02 (ScenePanel tree exists)
**Effort:** ~1 day
**Result:** Click entities in the tree. Selection highlight follows clicks. Clear selection by clicking empty space.

---

### F-04: Scene Panel — Entity Operations

**Goal:** Create, Delete, Rename entities from the ScenePanel.

**What it does:**
- **Create Empty:** Right-click → "Create Empty" OR + button. New entity becomes last child of selected (or root if none selected).
- **Delete:** Right-click → "Delete" OR Delete key. Confirmation dialog if entity has children.
- **Rename:** Double-click entity name (or F2), in-place edit. Enter = confirm, Escape = cancel. Empty name rejected.
- Undo tracks entity deletion (single-level, via existing CommandStack)

**Dependencies:** F-03 (selection), F-00 (World)
**Effort:** ~2-3 days
**Result:** Create entities, rename them, delete them. Hierarchy updates immediately.

---

## Phase 2 — Inspector

### F-05: Inspector — Transform

**Goal:** PropertiesPanel shows the selected entity's Transform. Position fields are editable.

**What it does:**
- PropertiesPanel reads selected entity from `Editor::selected_entity()`
- Shows entity name at top (editable text field, same rename as ScenePanel)
- Transform section (always first, expanded):
  - Position X/Y/Z: editable float drag fields
  - Rotation: read-only display (euler angles in degrees)
  - Scale: read-only display
- Changing position value immediately updates Entity's Transform

**Dependencies:** F-03 (selection)
**Effort:** ~1-2 days
**Result:** Select an entity, see its transform in Inspector. Change position value, entity updates.

---

### F-06: Inspector — Component Properties

**Goal:** Inspector shows all attached components with type-appropriate property editors. Add/Remove components.

**What it does:**
- Each component rendered as collapsible section with type name header
- Property editors per type (from ComponentRegistry):
  - `bool` → checkbox
  - `int` → integer drag
  - `float` → float drag
  - `string` → text input
  - `Vec2/3/4` → N float fields
  - `Color` → color picker
  - etc.
- **Add Component** button → searchable dropdown of registered types
- **Remove Component** button (ⓧ) per component section header
- Properties update entity in real-time

**Dependencies:** F-05 (Inspector Transform), ComponentRegistry (exists)
**Effort:** ~3-4 days (property rendering is tedious)
**Result:** Select entity with light component — see light color picker. Change color, light updates in viewport.

---

## Phase 3 — Viewport

### F-07: Viewport Panel — Scene Rendering

**Goal:** The ViewportPanel renders the 3D scene into an ImGui window using an editor camera.

**What it does:**
- ViewportPanel creates an ImGui window in the center dock
- Renders the editor's World (entities, meshes, lights) to a render target
- Editor camera independent of scene cameras (fixed initial position)
- Basic rendering pipeline reuses existing engine rendering

**Dependencies:** F-00 (World), engine rendering (exists)
**Effort:** ~2-3 days (ImGui render-to-texture, editor camera entity)
**Result:** Open a scene — see entities rendered in 3D in the viewport panel.

---

### F-08: Editor Camera Controls

**Goal:** Navigate the 3D viewport with the editor camera.

**What it does:**
| Input | Action |
|---|---|
| Right-click + drag | Look around (yaw/pitch) |
| Right-click + W/S/A/D | Fly forward/back/strafe |
| Right-click + Q/E | Move down/up |
| Scroll wheel | Dolly forward/back |
| F key (entity selected) | Focus camera on entity (instant snap) |
| Middle-click + drag | Pan camera |

- Viewport captures input only when hovered/focused
- Grid overlay on XZ plane (Y-up), major/minor lines, fade with distance

**Dependencies:** F-07 (ViewportPanel exists)
**Effort:** ~2-3 days
**Result:** Navigate the 3D scene freely. Press F to focus selected entity.

---

### F-09: Translate Gizmo

**Goal:** ImGuizmo translate gizmo on selected entity. Drag arrows to move in 3D.

**What it does:**
- ImGuizmo library already available (part of Dear ImGui)
- Three colored arrows on selected entity: red=X, green=Y, blue=Z
- Click + drag an arrow → entity translates along that axis
- Inspector Position fields update in real-time during drag
- Gizmo hidden during Play mode
- Only translate mode in MVP1 (rotate/scale not wired)

**Dependencies:** F-07 + F-08 (Viewport exists + camera works), entity selection
**Effort:** ~2 days
**Result:** Select entity, drag the translate arrow, entity moves in viewport and Inspector updates.

---

## Phase 4 — Utility Panels

### F-10: Console Panel

**Goal:** ConsolePanel displays engine/editor log output with filters.

**What it does:**
- Receives log output via the engine's logging system (ring buffer)
- Color-coded lines: trace/gray, info/white, warn/yellow, error/red, fatal/bright red
- Auto-scroll to bottom, with lock toggle
- Level filter dropdown (min level to show)
- Channel filter (show/hide by channel: engine, editor, render, asset, input, physics, game)
- Clear button
- Persists across Play/Stop transitions (never auto-clears)

**Dependencies:** Existing logging system (exists), ConsolePanel stub (exists)
**Effort:** ~1-2 days
**Result:** Launch editor, see engine log output in Console panel. Filter by level/channel.

---

### F-11: Project Panel

**Goal:** Browse project files. Double-click to load scenes / open prefabs.

**What it does:**
- File tree browser rooted at project directory
- Filtered to show: `.yaml`, `.gltf`, `.glb`, `.png`, `.jpg`
- Double-click `.yaml` scene → load (with save prompt if dirty)
- Double-click `.yaml` prefab → open in Prefab tab (once F-17 exists; until then, load in Scene tab?)
- Right-click context menu: Open, Delete, Rename, Show in Explorer
- Single-click → highlight file

**Dependencies:** F-01 (scene load), or independent as a file browser panel
**Effort:** ~2 days
**Result:** Browse project files, double-click a scene to load it.

---

### F-12: Assets Panel

**Goal:** Browse assets/. Drag asset to Inspector reference fields.

**What it does:**
- Flat list or simple grid of files in `assets/`
- Type filter buttons: All, Models, Textures, Materials
- Drag asset file → drop on Inspector asset reference field assigns the asset
- Right-click: Reload, Show in Explorer

**Dependencies:** F-06 (Inspector with asset reference fields)
**Effort:** ~2 days
**Result:** Browse assets, drag a model onto a MeshRenderer component's model field.

---

## Phase 5 — Play Mode

### F-13: World::clone() [Engine Feature]

**Goal:** Engine gains `World::clone()` for deep-copying entities, components, hierarchy.

**What it does:**
- `World::clone()` returns a new `World` with deep copies of all entities
- Each entity cloned with its Transform, components (deep-copied via ComponentRegistry factory), hierarchy, name, source
- Entity IDs preserved (or remapped consistently)
- Used by Play mode to create the runtime World

**Dependencies:** World class (exists), ComponentRegistry (exists)
**Effort:** ~2-3 days
**Result:** `World::clone()` produces an independent deep copy of any scene.

---

### F-14: Play/Stop — Game Tab

**Goal:** Press Play → clone World → Game tab opens → game runs. Press Stop → restore editor.

**What it does:**
- **▶ Play button** in toolbar:
  - Clone Editor's World (F-13)
  - Open Game tab with game camera viewport
  - Game loop runs on clone (Updatable components tick each frame)
  - Input routed to Game tab when focused
- **⏹ Stop button**:
  - Discard clone
  - Restore Editor's World to pre-Play state
  - Close Game tab
  - Return to Edit mode
- Game tab includes status bar overlay (FPS, entity count, play time)
- Play/Pause/Stop button state machine: Play enabled when stopped, Stop enabled when playing

**Dependencies:** F-13 (World::clone()), F-07+ (viewport rendering)
**Effort:** ~3-4 days
**Result:** Press Play → game runs in Game tab. Press Stop → editor scene restored unchanged.

---

### F-15: Read-Only Runtime Inspection

**Goal:** During Play, the Scene tab shows the runtime World. Inspector is read-only.

**What it does:**
- Scene panel switches to show runtime entities (from clone)
- Inspector shows runtime component values — **read-only**
  - Gray background, lock icon per component, "Play mode — read only" banner
  - Add/Remove component buttons hidden
- Viewport shows runtime World (editor camera can still navigate, but no gizmo)
- Visual Play mode indicators:
  - `#FF3300` 3px viewport border
  - `[Playing]` title prefix
  - Dark-red status bar: "🔴 PLAY MODE"
  - Toolbar: Play disabled, Stop/Pause enabled
- Console continues to show game + editor logs

**Dependencies:** F-14 (Play/Stop works)
**Effort:** ~2 days
**Result:** During Play, see runtime entities in hierarchy, inspect live component values (read-only).

---

### F-16: Pause

**Goal:** ⏸ Pause freezes the game loop. Resume with ▶ Play.

**What it does:**
- Pause button freezes Updatable component updates
- Game tab viewport freezes on last frame
- Scene tab remains read-only (frozen world)
- Resume with ▶ Play (or toggle Pause)
- Stop still works while paused

**Dependencies:** F-14 (Play/Stop)
**Effort:** ~1 day
**Result:** Press Pause during Play → game freezes. Press Play → game resumes.

---

## Phase 6 — Advanced Editing

### F-17: Prefab Tab

**Goal:** Double-click a prefab in Project panel → opens in its own Prefab tab for isolated editing.

**What it does:**
- Prefab tab opens as a new tab (Scene tab stays in place)
- Same layout as Scene tab (hierarchy left, viewport center, inspector right, bottom tab bar)
- World contains only the prefab's root entity + children
- Edits modify the prefab in isolation
- Save (Ctrl+S / File > Save) writes back to the `.yaml` prefab file
- Dirty state tracked per prefab tab
- Toolbar has no Play/Pause/Stop buttons
- Multiple prefab tabs can be open simultaneously

**Dependencies:** F-01 (save), F-02/04 (entity editing), F-07 (viewport), F-05/06 (inspector), F-11 (project panel trigger)
**Effort:** ~3-4 days (tab management is the tricky part)
**Result:** Open prefab, edit its entities/save, use it in scene.

---

### F-18: Detached Tabs (Multi-Window)

**Goal:** Right-click tab → Detach → new OS window with the tab's full layout. Re-attach to merge back.

**What it does:**
- Right-click tab header → "Detach Tab" context menu
- New OS window created with own GL context and ImGui dockspace
- Detached window contains the tab's full panel layout
- Original tab shows as "detached" in the tab bar (grayed out / absent)
- Re-attach merges back into main editor window
- Game tab detached = game on second monitor
- Prefab tab detached = prefab editing on second monitor

**Dependencies:** F-14 (Game tab concept exists), F-17 (Prefab tab concept exists), engine multi-window support
**Effort:** ~3-5 days (most complex feature — GL context + event routing + layout sync)
**Result:** Detach Game tab to second monitor. Play game on one screen, edit on the other.

---

## Summary: Dependencies & Parallelism

```
F-00 (Editor World)
  │
  ├── F-01 (Load/Save)
  │     │
  │     ├── F-02 (Entity Tree) ←─ can start after F-00, before F-01
  │     │     │
  │     │     ├── F-03 (Selection)
  │     │     │     │
  │     │     │     ├── F-04 (Entity Ops) ←─ can start after F-03
  │     │     │     │
  │     │     │     ├── F-05 (Inspector Transform)
  │     │     │     │     │
  │     │     │     │     └── F-06 (Inspector Components)
  │     │     │     │
  │     │     │     └── F-07 (Viewport Panel)
  │     │     │           │
  │     │     │           ├── F-08 (Camera Controls)
  │     │     │           │     │
  │     │     │           │     └── F-09 (Translate Gizmo)
  │     │     │           │
  │     │     │           └── F-13 (World::clone) ←─ engine work, independent
  │     │     │                 │
  │     │     │                 └── F-14 (Play/Stop + Game Tab)
  │     │     │                       │
  │     │     │                       ├── F-15 (Read-Only Runtime)
  │     │     │                       │     │
  │     │     │                       │     └── F-16 (Pause)
  │     │     │                       │
  │     │     │                       └── F-18 (Detached Tabs)
  │     │     │
  │     │     └── F-10 (Console) ←─ independent, no panel dependencies
  │     │
  │     └── F-11 (Project Panel) ←─ can start after F-01
  │           │
  │           └── F-17 (Prefab Tab) ←─ needs most features
  │
  └── F-12 (Assets Panel) ←─ can start after F-06 (needs Inspector drag-drop)
```

**Parallelization opportunities:**

| Track A (Core editor loop) | Track B (Utility panels) | Track C (Engine work) |
|---|---|---|
| F-00 Editor World | F-10 Console Panel | F-13 World::clone() |
| F-01 Load/Save | F-11 Project Panel | |
| F-02 Entity Tree | F-12 Assets Panel | |
| F-03 Selection | | |
| F-04 Entity Ops | | |
| F-05 Inspector Transform | | |
| F-06 Inspector Components | | |
| F-07 Viewport Panel | | |
| F-08 Camera Controls | | |
| F-09 Translate Gizmo | | |
| F-14 Play/Stop | | |
| F-15 Runtime Inspection | | |
| F-16 Pause | | |
| F-17 Prefab Tab | | |
| F-18 Detached Tabs | | |

---

## Quick-start recommendation

If you want the fastest path to a tangible result:

```
F-00 → F-01 → F-02 → F-03 → F-05 → F-07 → F-08
```
This gives you: Editor with a World → Load scene from disk → See entity tree → Click to select → See Transform in Inspector → See 3D viewport → Fly around. That's a complete feedback loop in ~8-12 days of work.

Then: F-04 (entity ops) + F-06 (component editor) + F-09 (gizmo) = full scene editing.
Then: F-10/11/12 (utility panels) in parallel with F-13/14 (play mode).

---

*Generated from: `.specs/sprint-2026-06/editor-ux-design/spec.md`*
