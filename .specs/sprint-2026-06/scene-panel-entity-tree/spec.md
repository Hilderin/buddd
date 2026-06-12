# SPEC-F-02 — Scene Panel — Entity Tree

## Problem

The editor's Scene Panel is currently an empty placeholder with only a title bar. Users have no way to see the entity hierarchy of the scene they are editing. Without a visual tree of entities, the editor offers no structural overview — users cannot browse root entities, inspect parent-child relationships, or understand the composition of a loaded scene. Every future feature that needs entity selection (F-03), inspector population (F-04), viewport gizmo attachment (F-05), or entity creation/deletion (F-06+) depends on first being able to *see* the entity tree.

Additionally, the existing `EditorPanel::update()` / `draw_ui()` and `EditorMenu::update()` / `draw_ui()` signatures take `EngineContext const&`, but panels need access to the Editor's own `World` (via `editor.world()`) and other editor-specific state. A new context type is needed to pass the Editor reference to panels without coupling them to the concrete `Editor` class in their public API.

## Goals

| ID | Goal |
|---|---|
| G-01 | **Entity tree rendering**: ScenePanel renders all root entities as top-level tree nodes, with child entities indented under their parents, using ImGui `TreeNodeEx`. |
| G-02 | **Leaf/non-leaf distinction**: Entities with no children render as leaf nodes (no expand arrow). Entities with children render as expandable/collapsible nodes, default expanded. |
| G-03 | **Empty state**: When the World has zero entities, the panel displays the text "No entities" instead of an empty area. |
| G-04 | **Empty name display**: Entities with an empty name (`name() == ""`) display as "(unnamed)". |
| G-05 | **ID collision prevention**: Each tree node is wrapped in `ImGui::PushID` / `PopID` using a unique per-entity identifier to prevent ImGui ID collisions (e.g., when two entities share the same name). |
| G-06 | **EditorContext plumbing**: Create a `struct EditorContext` holding `Editor&` and `EngineContext const&`, and update `EditorPanel::update()`, `EditorPanel::draw_ui()`, `EditorMenu::update()`, and `EditorMenu::draw_ui()` to accept `EditorContext const&` instead of `EngineContext const&`. |
| G-07 | **Non-regression**: All existing panels (5 placeholder panels + MenuBar) continue to compile and work with the new `EditorContext` signature. Zero new warnings from `src/editor/` and `tests/`. |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | **No selection** — clicking a tree node does NOT select it. Selection is deferred to F-03. |
| NG-02 | **No entity creation** — no "Create Empty" or "+" button. Deferred to future features. |
| NG-03 | **No entity deletion** — no Delete key or right-click "Delete" option. Deferred to future features. |
| NG-04 | **No entity renaming** — no double-click-to-rename or F2. Deferred to future features. |
| NG-05 | **No drag-and-drop reparenting** — cannot drag entities to reparent. Deferred to future features. |
| NG-06 | **No search/filter** — no search bar or filter by name. Deferred to future features. |
| NG-07 | **No virtualized tree** — no lazy-loading or virtualization for large hierarchies. Performance for 10,000+ entities is explicitly deferred. |
| NG-08 | **No context menu** — no right-click context menu. Tree rendering only. |
| NG-09 | **No changes to World or Entity classes** — the engine APIs are consumed as-is. |
| NG-10 | **No changes to `Editor::draw_ui()` panel iteration signature** — only the per-panel/per-menu virtual method signatures change. |

## Actors

| Actor | Description |
|---|---|
| **Editor user** | Opens the editor, loads or creates a scene, sees the entity hierarchy in the Scene panel. Can expand/collapse tree nodes but cannot interact further (no selection, no editing). |
| **Editor developer** | Adds new `EditorPanel` or `EditorMenu` subclasses. Uses `EditorContext` to access `ctx.editor.world()` for the editor's World, or `ctx.engine` for engine services. |
| **Future feature developer** | Builds on this tree for F-03 (selection), F-04 (inspector population), etc. Relies on the entity tree being rendered correctly and the `EditorContext` plumbing being in place. |

## User-visible behavior

### Scene Panel — Entity Tree (with entities)

The Scene panel displays a tree view of all entities in the editor's World:

- **Root entities** appear as top-level tree nodes, ordered by their index in the World (`World::get_root_entity(i)`).
- **Child entities** appear indented under their parent, ordered by their child index (`Entity::get_child(i)`).
- Entities with **no children** render as leaf nodes — the expand arrow is hidden (using `ImGuiTreeNodeFlags_Leaf`).
- Entities with **children** render as expandable/collapsible nodes — clicking the arrow toggles expansion.
- All nodes are **default expanded** when first rendered.
- Each tree node spans the available width (`ImGuiTreeNodeFlags_SpanAvailWidth`).
- The tree renders recursively: for each entity, its child entities are rendered as sub-nodes.
- Entity names are displayed as-is. Empty names display as "(unnamed)".
- The tree **reads the World every frame** via `ctx.editor.world()` — changes to the World (loaded scene, new scene) are reflected immediately on the next frame.

### Scene Panel — Empty State

When `World::entity_count() == 0`, instead of a tree, the panel displays a single line of text: "No entities", centre-aligned or left-aligned (implicitly, since ImGui Text is left-aligned). No tree nodes are rendered.

### Panel Plumbing Change: EditorContext

The `EditorPanel` and `EditorMenu` base classes change their virtual method signatures:

- `update(EngineContext const& ctx)` → `update(EditorContext const& ctx)`
- `draw_ui(EngineContext const& ctx)` → `draw_ui(EditorContext const& ctx)`

Panels access the editor's World via `ctx.editor.world()` and engine services via `ctx.engine`.

The `Editor` class constructs `EditorContext{*this, engine_ctx}` internally and passes it to panels and menus. No external API changes to `Editor::update()` or `Editor::draw_ui()` — their signatures remain `(EngineContext const&)`.

### Editor Updates (internal)

- `Editor::update(EngineContext const& ctx)` creates `EditorContext{*this, ctx}` and passes to `menu->update(editor_ctx)` / `panel->update(editor_ctx)`.
- `Editor::draw_ui(EngineContext const& ctx)` creates `EditorContext{*this, ctx}` and passes to `menu->draw_ui(editor_ctx)` / `panel->draw_ui(editor_ctx)`.

## Key entities

### `EditorContext` (`src/editor/editor_context.h` — new file)

```cpp
namespace buddd::editor {

struct EditorContext {
    Editor& editor;
    buddd::engine::EngineContext const& engine;
};

} // namespace buddd::editor
```

- A lightweight aggregate (trivially constructible, no virtual methods, no ownership).
- Provides panels and menus with access to both the editor state (via `editor`) and the engine context (via `engine`).
- All panels that previously took `EngineContext const&` now take `EditorContext const&`. They access `ctx.engine` for engine services and `ctx.editor` for editor-specific state.

### Interface Changes

**`EditorPanel` base class** (`src/editor/editor_panel.h`):

```cpp
// Before:
virtual auto update(buddd::engine::EngineContext const& /*ctx*/) -> void {}
virtual auto draw_ui(buddd::engine::EngineContext const& /*ctx*/) -> void {}

// After:
virtual auto update(EditorContext const& /*ctx*/) -> void {}
virtual auto draw_ui(EditorContext const& /*ctx*/) -> void {}
```

**`EditorMenu` base class** (`src/editor/editor_menu.h`):

```cpp
// Same change as EditorPanel
```

**`ScenePanel::draw_ui()`** (`src/editor/panels/scene_panel.h`):

```cpp
// Before:
auto draw_ui(buddd::engine::EngineContext const& /*ctx*/) -> void override { /* empty */ }

// After:
auto draw_ui(EditorContext const& ctx) -> void override {
    // Render entity tree from ctx.editor.world()
}
```

**All existing panels** (`PropertiesPanel`, `ConsolePanel`, `ProjectPanel`, `AssetsPanel`):

- `update(EngineContext const&)` → `update(EditorContext const&)`
- `draw_ui(EngineContext const&)` → `draw_ui(EditorContext const&)`
- These panels currently have empty `update()`/`draw_ui()` bodies. They merely adjust the parameter type.

**`MenuBar`** (`src/editor/panels/menu_bar.h`):

- Same signature change for `update()` and `draw_ui()`.
- `MenuBar` callbacks that previously captured and used `EngineContext const&` now use `EditorContext::engine`.

**`Editor::update()` / `draw_ui()`** (internal change — no external API change):

- Create `EditorContext{*this, ctx}` and pass to each menu/panel instead of passing `ctx` directly.

## User stories

### Story 1 — Entity tree renders root entities (Priority: P1)

As an editor user, I want to see all root entities in a tree when I open a scene, so that I can understand the scene's top-level structure.

**Given** the editor's World has 3 root entities named "Player", "Light", and "Camera"
**When** the Scene panel renders
**Then** 3 top-level tree nodes are visible in the panel
**And** each node displays its entity name ("Player", "Light", "Camera")

**Given** the editor's World has no entities (`entity_count() == 0`)
**When** the Scene panel renders
**Then** the panel displays "No entities" instead of a tree

### Story 2 — Parent-child hierarchy is visible (Priority: P1)

As an editor user, I want to see child entities indented under their parent, so that I can understand the parent-child relationships in the scene.

**Given** the editor's World has a root entity "Player" with 2 children ("Mesh" and "Collider")
**When** the Scene panel renders
**Then** "Player" appears as a top-level node with an expand arrow
**And** clicking the arrow expands to reveal "Mesh" and "Collider" indented under "Player"

### Story 3 — Leaf nodes are not expandable (Priority: P1)

As an editor user, I want leaf entities (nodes with no children) to show no expand arrow, so that I can distinguish between branches and leaves.

**Given** the editor's World has a root entity "Camera" with `child_count() == 0`
**When** the Scene panel renders
**Then** "Camera" appears as a leaf node (no expand arrow, styled via `ImGuiTreeNodeFlags_Leaf`)

### Story 4 — Empty name displayed as "(unnamed)" (Priority: P1)

As an editor user, I want entities with empty names to be visible and identifiable, even if they have no name assigned.

**Given** the editor's World has an entity with `name() == ""`
**When** the Scene panel renders
**Then** the entity displays as "(unnamed)" in the tree

### Story 5 — Tree refreshes when World changes (Priority: P2)

As an editor user, I want the entity tree to automatically reflect World changes after load or new-scene operations, without restarting the editor.

**Given** the editor's World has 3 root entities displayed in the tree
**When** `editor.new_scene()` is called (World becomes empty)
**Then** on the next frame, the tree shows "No entities"

**Given** the empty editor World
**When** a scene is loaded via `editor.open_scene(path)` that populates the World
**Then** on the next frame, the tree displays the loaded entities

### Story 6 — Existing panels continue to work with EditorContext (Priority: P1)

As an editor developer, I want all existing panels and menus to compile and work after the `EditorContext` signature change, so that no regressions are introduced.

**Given** the codebase after the `EditorContext` change
**When** I build the project
**Then** all panels (Scene, Properties, Console, Project, Assets) and menus (MenuBar) compile without errors
**And** the editor runs identically to before (same layout, same menu bar, same empty panels)

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-01 | ScenePanel renders root entities as top-level tree nodes when World has entities. | Unit test: add 2 root entities to `editor.world()`, call `ScenePanel::draw_ui()` with a test-friendly context, verify `ImGui::TreeNodeEx` was called twice with the entity names. |
| AC-02 | Child entities are rendered as indented children under their parent. | Unit test: add a root entity with 2 children, render tree, verify `ImGui::TreeNodeEx` is called for the parent with children rendered via recursive calls. Verify indentation structure (children rendered inside the parent's tree node block). |
| AC-03 | Entities with no children are rendered as leaf nodes (no expand arrow). | Unit test: add a root entity with `child_count() == 0`, verify `ImGui::TreeNodeFlags_Leaf` flag is passed in the `TreeNodeEx` call. |
| AC-04 | Entities with children are expandable/collapsible, default expanded. | Unit test: add a root entity with 1 child, render tree, verify `ImGui::TreeNodeFlags_DefaultOpen` flag is passed for the parent node. Verify `ImGui::TreeNodeFlags_Leaf` is NOT set for the parent. |
| AC-05 | Tree renders correctly when World is empty (shows "No entities" placeholder). | Unit test: render panel on an empty World (`entity_count() == 0`), verify `ImGui::Text("No entities")` is called and no `TreeNodeEx` calls are made. |
| AC-06 | Entities with empty name are displayed as "(unnamed)". | Unit test: add entity with empty name (`set_name("")`), render tree, verify the text displayed for that node is "(unnamed)" instead of an empty string. |
| AC-07 | `EditorContext` is defined in `src/editor/editor_context.h` as `struct EditorContext { Editor& editor; EngineContext const& engine; };`. | File exists; inspect the struct definition. |
| AC-08 | `EditorPanel::update()` and `draw_ui()` accept `EditorContext const&` instead of `EngineContext const&`. | Inspect `editor_panel.h` — verify signature change. |
| AC-09 | `EditorMenu::update()` and `draw_ui()` accept `EditorContext const&` instead of `EngineContext const&`. | Inspect `editor_menu.h` — verify signature change. |
| AC-10 | `ScenePanel::draw_ui()` renders the entity tree from `ctx.editor.world()`. | Inspect `scene_panel.h` — verify `draw_ui()` uses `ctx.editor.world()` to iterate entities. |
| AC-11 | All 5 existing panels (Scene, Properties, Console, Project, Assets) and MenuBar compile with new `EditorContext` signature. | Build with `cmake --build --preset debug` — compile succeeds with zero errors from `src/editor/`. |
| AC-12 | Zero warnings from `src/editor/` and `tests/`. | Build with `cmake --build --preset debug` — verify zero warnings. |
| AC-13 | All existing tests still pass. | Run `buddd_tests` — all previously passing tests continue to pass. |
| AC-14 | `ImGui::PushID`/`PopID` wraps each tree node using a unique per-entity identifier. | Inspect `scene_panel.h` — verify `PushID` is called before each `TreeNodeEx` with an entity-specific value, and `PopID` after the tree node block. |

## E2E Verification

| Method | Description |
|---|---|
| **Headless unit test (CI)** | Build with `BUDDD_HAS_DISPLAY=OFF`. Run `buddd_tests`. Verify `[editor][scene_panel]` tagged tests pass — empty state, root entities, hierarchy, leaf nodes, default expanded, empty name display. |
| **Manual smoke test (display)** | Run `buddd edit`. Verify editor launches with Scene panel showing "No entities" (empty World). Create entities in code (if F-01 scene load is available) or verify the panel is structurally correct. Expand/collapse tree nodes. |
| **Clean build verification (CI)** | Run `cmake --build --preset debug` and verify zero warnings from `src/editor/` and `tests/`. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A user can open the editor with a loaded scene (via F-01) and see all entities in a tree, with expandable/collapsible parent-child structure. | Load a test scene with known entity hierarchy via F-01, observe tree in Scene panel matches expected structure. |
| SC-002 | An empty-scene state clearly shows "No entities" — no confusing blank area. | Launch editor without scene, Scene panel shows "No entities" text. |
| SC-003 | All 5 placeholder panels and MenuBar compile and work with `EditorContext` with zero regressions. | Full test suite passes, manual smoke test confirms editor behaviour unchanged. |
| SC-004 | The entity tree renders with zero frame-time impact when the World is empty, and negligible impact for typical scenes (< 1000 entities). | `ImGui::TreeNodeEx` cost for typical scene is < 0.1 ms per frame. |

## Edge cases

| Case | Expected behavior |
|---|---|
| **Empty World (0 entities)** | Panel shows "No entities" text. No tree nodes rendered. |
| **World with root entities only (no children)** | All root entities render as leaf nodes (`ImGuiTreeNodeFlags_Leaf`). No expand arrows. |
| **Single root entity with deep children (e.g., 10 levels)** | Tree renders all levels. Each level is expandable/collapsible. Default expanded for all nodes. |
| **Entity with empty name** | Displayed as "(unnamed)". |
| **Entity with very long name (e.g., >1000 chars)** | ImGui handles text truncation natively, no special handling required. The text may overflow the panel width but is still visible by resizing the panel. |
| **Entity with special characters in name (Unicode, emoji)** | ImGui renders UTF-8 text correctly. No special handling needed. |
| **World with 10,000+ entities** | Tree renders but may cause frame-time lag. This is a known deferred issue (virtualized tree not in scope). The tree does NOT crash or produce undefined behaviour. |
| **World that is valid but empty after `new_scene()`** | Panel shows "No entities" on the next frame (tree reads `world()` each frame). |
| **Scene loaded via F-01 (`open_scene`)** | Tree automatically reflects the loaded entities on the next frame. No explicit refresh needed. |
| **Two entities with identical names** | Each node is wrapped in `PushID(entity.id())` so ImGui treats them as distinct tree nodes. No ID collision. |
| **Single root entity with one child that also has children** | All levels are expandable. The chain renders correctly — root → child → grandchild, each indented one level deeper. |

## Error cases

| Case | Expected behavior |
|---|---|
| **World access failure (edge case: `editor.world()` returns invalid reference)** | Not possible — the World is created in the Editor constructor and destroyed in the destructor. `world()` always returns a valid reference (guaranteed by SPEC-029 / F-00). No error handling needed. |
| **`ImGui::TreeNodeEx` fails or returns false** | Standard ImGui node pattern: `if (ImGui::TreeNodeEx(...)) { ... ImGui::TreePop(); }`. If the node returns false (collapsed), children are not rendered. This is correct behaviour, not an error. |
| **Out-of-memory during tree rendering** | ImGui operations allocate internally. If memory is exhausted, the behaviour is undefined (ImGui's standard behaviour). This is consistent with the rest of the editor. |
| **Entity count changes mid-frame** | Not possible — the World is not modified during `draw_ui()`. Entity mutations happen during `update()`. |
| **`EditorContext` construction with null references** | Not possible — `EditorContext` holds `Editor&` (always valid, Editor is the owning object) and `EngineContext const&` (always valid, passed from Editor::update/draw_ui). |

## Permissions and security

- No changes to permissions or security posture.
- The entity tree renders data already owned by the Editor — no file I/O, no network access.
- No sensitive data is exposed.
- No authentication or authorisation boundaries are crossed.

## Observability

| Signal | Source |
|---|---|
| **Scene panel tree render** | No per-entity logging. Consider a debug-level log for panel rendering start/end if debugging tree rendering issues. |
| **Entity count at render time** | Could be logged at debug level: `BUDDD_LOG_DEBUG("ScenePanel: rendering {} entities", world.entity_count())` — useful for performance debugging. |
| **EditorContext creation** | No logging needed — it is a trivial aggregate created stack-local in `Editor::update()`/`draw_ui()`. |

## Documentation impact

The following existing wiki pages must be updated when this spec is implemented:

| Document | Reason for update |
|---|---|
| `docs/wiki/editor/editor-panels.md` | `EditorPanel` and `EditorMenu` base class signatures change from `EngineContext const&` to `EditorContext const&`. The doc must reflect the new `update()` / `draw_ui()` signatures, document the new `EditorContext` struct, and note that `ScenePanel` now renders the entity tree instead of being an empty placeholder. |
| `docs/wiki/editor/scene-management.md` | The `EditorContext` pattern introduces a standard way for panels to access the editor's `World` via `ctx.editor.world()`. The doc should be reviewed to ensure the World-access pattern for panels is consistent with this change — if it currently describes a different access method, it must be updated. |

These updates are the responsibility of the implementation phase and will be tracked in the implementation contract.

## Out of scope

- Entity selection (F-03) — click-to-select, multi-select, selection highlighting.
- Entity creation (F-06) — "Create Empty", "+" button.
- Entity deletion (F-06) — Delete key, right-click delete.
- Entity renaming (F-06) — double-click rename, F2.
- Drag-and-drop reparenting (F-06).
- Search/filter bar in the Scene panel.
- Context menu (right-click) on tree nodes.
- Virtualized / lazy-loaded tree for large hierarchies.
- Play mode read-only tree rendering (future F-14).
- Any changes to World, Entity, or engine APIs.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `World::entity_count()`, `World::root_entity_count()`, `World::get_root_entity(size_t)`, `Entity::child_count()`, `Entity::get_child(size_t)`, `Entity::name()`, and `Entity::id()` are all available as described in `world.h` and `entity.h`. No additional engine changes are needed. |
| A-02 | `ImGui::TreeNodeEx`, `ImGui::TreePop`, `ImGui::PushID`, `ImGui::PopID`, `ImGui::Text`, `ImGuiTreeNodeFlags_SpanAvailWidth`, `ImGuiTreeNodeFlags_Leaf`, and `ImGuiTreeNodeFlags_DefaultOpen` are all available from the ImGui docking branch (v1.91.8-docking). |
| A-03 | The Editor's World is always valid (guaranteed by SPEC-029 / F-00). No null checks or assertions are needed when accessing `ctx.editor.world()`. |
| A-04 | Entity IDs are unique for the lifetime of the World (not reused while entities exist). Using `EntityId::index` (or a composite of index+generation) for `PushID` provides sufficient uniqueness for ImGui's ID stack. |
| A-05 | The `EditorContext` struct is a pure aggregate — no constructors, destructors, or virtual methods. It is constructed stack-local each frame in `Editor::update()` and `Editor::draw_ui()`. |
| A-06 | All 5 placeholder panels (Scene, Properties, Console, Project, Assets) currently have empty `update()`/`draw_ui()` bodies that only need the parameter type change. No panel logic changes are needed beyond the signature update (except ScenePanel which gets the tree implementation). |
| A-07 | The `MenuBar` class currently uses `EngineContext const&` in its `update()`/`draw_ui()` overrides and in callback lambdas. The signature change only affects the override signatures; internal callbacks that reference `ctx` will need to use `ctx.engine` instead. |
| A-08 | `Editor::update()` and `Editor::draw_ui()` public signatures remain `(EngineContext const&)` — only the internal dispatch to menus/panels uses `EditorContext`. No changes to `EditorApp`, `App` base class, or callers outside `src/editor/`. |
| A-09 | The tree renders in World insertion order (root entity index order, child entity index order). No sorting by name or any other criterion is applied. |
| A-10 | The Scene panel is a single panel (not split into multiple docked views). The entity tree fills the entire panel content area. |

## Open questions

| ID | Question | Resolution |
|---|---|---|
| Q-01 | **`EntityId` unique identifier for `PushID`**: What value should be passed to `ImGui::PushID()` for each entity? `EntityId` has `index` (uint32_t) and `generation` (uint32_t). Should we use `index` alone (simpler) or a composite hash (safer)? | **Use `static_cast<int>(entity.id().index)`** — the index is unique among live entities within the same generation cycle, so it is sufficient for ImGui ID disambiguation. The generation field is not needed for PushID because an entity that has been destroyed and its index reused will be a different ImGui ID context (different parent node or different frame state). If hash collisions later become a practical issue, a composite can be introduced. This is an implementation contract detail. |
| Q-02 | **Default expand depth**: Should all nodes be expanded by default, or only the first N levels? | **All levels default expanded.** The user sees the full hierarchy on first view. This matches the `ImGuiTreeNodeFlags_DefaultOpen` usage described in the grill-me decision. If performance concerns arise for very deep hierarchies, depth limits can be introduced later. |

All questions resolved. No `[NEEDS CLARIFICATION]` markers remain.
