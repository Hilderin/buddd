# SPEC-F-03 — Entity Selection with Multi-Select

## Problem

The Scene Panel now renders the entity hierarchy tree (F-02), but clicking an entity does nothing — there is no selection state, no visual feedback, and no way for other panels (Inspector, Viewport) to know which entity the user is working with. Without selection, every downstream feature (Inspector population, gizmo attachment, entity CRUD operations) is blocked because none of them know *which entity* to operate on.

Additionally, the editor needs multi-select support from the start: Ctrl+click to toggle individual entities, Shift+click to select ranges, and Ctrl+A to select all. This is required by the human design direction to avoid retrofitting multi-select later.

A separate `Selection` value class is needed so that Commands (F-04+) can snapshot the selection before executing entity-modifying operations and restore it on undo, ensuring selection stability across undo/redo cycles.

## Goals

| ID | Goal |
|---|---|
| G-01 | **Selection value class**: `Selection` stores a set of `EntityId`s as a pure value object — cloneable, comparable, independently testable, and savable in Commands. |
| G-02 | **EditorSelection manager**: `EditorSelection` owns the active selection state, provides mutation methods (`select`, `clear`, `set_selection`), and fires change callbacks. |
| G-03 | **Plain click selects entity**: Left-clicking an entity in the Scene Panel selects it (Replace modifier — clear previous, select new). |
| G-04 | **Ctrl+click toggles entity**: Ctrl+left-click adds or removes an entity from the selection without affecting other selected entities. |
| G-05 | **Shift+click range selection**: Shift+left-click selects all entities from the anchor entity to the clicked entity in linearized depth-first tree order. |
| G-06 | **Ctrl+A selects all**: Ctrl+A selects all entities in the World (Scene Panel provides the list via tree traversal). |
| G-07 | **Click empty area clears selection**: Left-clicking empty space in the Scene Panel clears the selection and resets the anchor. |
| G-08 | **Selection highlighting**: Selected entities display with `ImGuiTreeNodeFlags_Selected` in the Scene Panel tree. |
| G-09 | **Editor owns EditorSelection**: `Editor` has an `EditorSelection selection_` member, exposed via `Editor::selection()` → `EditorSelection&`. |
| G-10 | **Selection cleared on scene changes**: `Editor::new_scene()` and `Editor::open_scene()` clear the active selection. |
| G-11 | **Undo/redo future-proofing**: `EditorSelection::snapshot()` returns a copyable `Selection`; `EditorSelection::restore(Selection)` restores it and fires callbacks. |
| G-12 | **Callback infrastructure**: `EditorSelection` supports `on_change()` / `remove_on_change()` with no consumers in F-03 (future-proofing for F-05+). |
| G-13 | **Non-regression**: All existing tests pass. Zero new warnings from `src/editor/` and `tests/`. |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | **No PropertiesPanel changes** — the Properties/Inspector panel remains an empty placeholder. No "No entity selected" state. No entity property display. (F-05). |
| NG-02 | **No selection-change event consumers** — callback infrastructure exists but no consumers yet. Inspector and Viewport do not react to selection changes (F-05, F-07). |
| NG-03 | **No viewport selection** — mouse picking / click-to-select in the 3D viewport is deferred (future feature). |
| NG-04 | **No undo for selection** — selection changes are NOT undoable. Undo/redo is for entity-modifying Commands only. Selection is restored via snapshot/restore inside Commands. |
| NG-05 | **No path-based resolution** — Commands manage selection via snapshot/restore of `Selection`. No entity-path-based resolution. |
| NG-06 | **No keyboard-only selection navigation** — arrow keys, Home/End, type-to-select are not supported. |
| NG-07 | **No entity-destroyed auto-clear** — if an entity is destroyed while selected, the selection is not automatically cleared. Calling code (Commands) manages this explicitly via snapshot/restore. |
| NG-08 | **No drag-to-select** — rubber-band / marquee selection is not supported. |
| NG-09 | **No double-click behaviour** — double-clicking an entity does nothing special. |
| NG-10 | **No right-click behaviour for selection** — right-click does not change selection (context menu deferred). |
| NG-11 | **No changes to World or Entity classes** — engine APIs are consumed as-is. |

## Actors

| Actor | Description |
|---|---|
| **Editor user** | Opens the editor, loads a scene with entities, clicks entities in the Scene Panel to select them. Can multi-select with Ctrl+click, Shift+click range, Ctrl+A. Sees visual highlighting on selected entities. |
| **Editor developer** | Uses `EditorSelection` to query selection state (`size()`, `contains(id)`, `first()`, iteration). Subscribes to selection change callbacks. Uses `snapshot()`/`restore()` for Command undo/redo support. |
| **Future feature developer** | Builds on selection for F-04 (Inspector population), F-05 (Inspector property editing), F-06 (entity CRUD), F-07 (viewport gizmo). Relies on `Editor::selection()` accessor and callback infrastructure. |
| **Command author** | Calls `editor.selection().snapshot()` in Command constructor to save pre-execution selection. Calls `editor.selection().restore(saved)` in `undo()` to restore it. |

## User-visible behavior

### Selection Value Class (`Selection`)

`Selection` is a pure value type storing a set of `EntityId`s. It supports:
- **Query**: `contains(id)`, `size()`, `empty()`, `first()`
- **Iteration**: range-for over the underlying set
- **Local mutation**: `add(id)`, `remove(id)`, `clear()` — these build a new copy and do NOT affect `EditorSelection`
- **Comparison**: `operator==` (enables equality checks for change detection)
- **Copy semantics**: copying a `Selection` produces an independent clone

### Selection Manager (`EditorSelection`)

`EditorSelection` manages the *active* selection. It is owned by `Editor` and accessed via `editor.selection()`.

**Mutation methods** (all fire `on_change` callbacks after modification):
- `select(id, Replace)` — clears the selection, adds the given entity, sets anchor
- `select(id, Toggle)` — if entity is selected, removes it; if not selected, adds it (anchor unchanged)
- `clear()` — removes all entities from selection, clears anchor
- `set_selection(ids)` — sets the exact selection from a span of EntityIds (for Shift+click range and Ctrl+A)

**Snapshot/restore** (for Command undo/redo):
- `snapshot()` — returns a `Selection` copy of the current state (including anchor)
- `restore(saved)` — replaces current selection with the saved `Selection`, fires callbacks

### Multi-select Interactions

| Input | Modifier | Behavior |
|---|---|---|
| Left-click entity | None | **Replace**: clear selection → select this entity. Set anchor to this entity. |
| Left-click entity | Ctrl | **Toggle**: if entity is selected → remove it. If not selected → add it. Anchor unchanged. |
| Left-click entity | Shift | **Range**: select all entities from anchor to clicked entity in linearized depth-first tree order. Clear previous selection. Set anchor unchanged. |
| Ctrl+A | — | **Select all**: select all entities in the World. Anchor unchanged. |
| Click empty area | None | **Clear**: selection cleared. Anchor cleared. |
| Click empty area | Ctrl or Shift | **No-op**: modifier ignored on empty-click. Selection unchanged. Anchor unchanged. |

### Shift+click Range Semantics

"Linearized tree order" is defined as depth-first traversal order — the visual order entities appear in the tree. The Scene Panel handles the tree traversal to collect all entities between the anchor and the clicked entity (inclusive), passing the list to `EditorSelection::set_selection()`.

If no anchor exists when Shift+click is performed (e.g., first click in the panel is Shift+click), the behavior degrades to Replace: only the clicked entity is selected, and the anchor is set to it.

### Selection Highlighting

Entities in the Scene Panel tree that are part of the current selection display with the `ImGuiTreeNodeFlags_Selected` flag passed to `ImGui::TreeNodeEx`. Multiple selected entities all show the selection highlight simultaneously.

### Selection on World Changes

- `Editor::new_scene()` → `selection_.clear()` — selection cleared, anchor cleared
- `Editor::open_scene()` → `selection_.clear()` — selection cleared, anchor cleared

### Callback Infrastructure

`EditorSelection` provides `on_change(ChangeCallback)` returning a token, and `remove_on_change(token)`. In F-03, no consumers register callbacks — the infrastructure exists purely for future use (Inspector in F-05, Viewport in F-07).

### ImGui Keyboard Shortcut Integration

Ctrl+A is registered via `ShortcutRegistry` in `Editor::setup()`, gated by `ImGui::GetIO().WantCaptureKeyboard`. It calls a Scene Panel helper that traverses the entity tree and calls `editor.selection().set_selection(all_ids)`.

## Key entities

### `Selection` (`src/editor/editor_selection.h` — new file)

```cpp
namespace buddd::editor {

class Selection {
public:
    // Query
    [[nodiscard]] auto contains(EntityId id) const noexcept -> bool;
    [[nodiscard]] auto size() const noexcept -> size_t;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto first() const noexcept -> std::optional<EntityId>;

    // Iteration (expose the set for range-for)
    using const_iterator = /* set::const_iterator */;
    auto begin() const noexcept -> const_iterator;
    auto end() const noexcept -> const_iterator;

    // Local mutation (builds a copy — does NOT affect EditorSelection)
    auto add(EntityId id) -> void;
    auto remove(EntityId id) -> void;
    auto clear() -> void;

    auto operator==(const Selection&) const noexcept -> bool = default;

private:
    friend class EditorSelection;
    std::unordered_set<EntityId> selected_;
};

} // namespace buddd::editor
```

### `SelectionModifier` enum

```cpp
namespace buddd::editor {

enum class SelectionModifier {
    Replace,  // Plain click: clear + select just this one
    Toggle,   // Ctrl+click: add or remove this one
};

} // namespace buddd::editor
```

### `EditorSelection` (`src/editor/editor_selection.h`)

```cpp
namespace buddd::editor {

class EditorSelection {
public:
    // Query
    [[nodiscard]] auto contains(EntityId id) const noexcept -> bool;
    [[nodiscard]] auto size() const noexcept -> size_t;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto first() const noexcept -> std::optional<EntityId>;

    // Mutation (fires callbacks)
    void select(EntityId id, SelectionModifier modifier = SelectionModifier::Replace);
    void clear();
    void set_selection(std::span<const EntityId> ids);

    // Snapshot for Commands
    [[nodiscard]] auto snapshot() const noexcept -> Selection;
    void restore(const Selection& saved);  // fires callbacks

    // Shift+click anchor
    [[nodiscard]] auto anchor() const noexcept -> std::optional<EntityId>;
    void set_anchor(EntityId id);

    // Callbacks
    using ChangeCallback = std::function<void()>;
    auto on_change(ChangeCallback cb) -> size_t;   // returns token
    auto remove_on_change(size_t token) -> void;

    // Selection object access (for iteration)
    [[nodiscard]] auto current() const noexcept -> const Selection&;

private:
    Selection current_;
    std::optional<EntityId> anchor_;
    std::vector<std::pair<size_t, ChangeCallback>> callbacks_;
    size_t next_token_ = 0;
    void fire_callbacks();
};

} // namespace buddd::editor
```

### Interface Changes

**New file**: `src/editor/editor_selection.h` — `Selection`, `EditorSelection`, `SelectionModifier`.

**`Editor`** (`src/editor/editor.h`):
- Add `#include "editor_selection.h"` (or forward declare and include in .cpp).
- Add `EditorSelection selection_;` as a private member.
- Add `[[nodiscard]] auto selection() -> EditorSelection&;` public accessor.

**`Editor`** (`src/editor/editor.cpp`):
- Implement `Editor::selection()` returning `selection_`.
- Update `Editor::new_scene()`: call `selection_.clear()` after creating the new World.
- Update `Editor::open_scene()`: call `selection_.clear()` after successful load.
- Register Ctrl+A shortcut that calls a helper (Scene Panel tree traversal → select all).

**`ScenePanel`** (`src/editor/panels/scene_panel.h`):
- Modify `draw_ui()` to handle mouse clicks on tree nodes:
  - `ImGui::IsItemClicked()` / `ImGui::IsItemClicked(ImGuiMouseButton_Left)` for entity click detection.
  - Check `ImGui::GetIO().KeyCtrl` and `ImGui::GetIO().KeyShift` to determine modifier.
  - Call `ctx.editor.selection().select(id, modifier)` with the appropriate `SelectionModifier`.
- Modify entity rendering to check selection state:
  - If `ctx.editor.selection().contains(entity.id())`, add `ImGuiTreeNodeFlags_Selected` to flags.
- Add empty-area click handling:
  - Check `ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()` → clear selection.
- Add tree traversal helper for Shift+click range and Ctrl+A:
  - Depth-first traversal collecting EntityIds in linearized order.
  - Find index of anchor and clicked entity; return the subspan.
- Add `on_selection_change` callback registration (optional — Scene Panel does not need to react to external changes in F-03, but the tree is read each frame so it updates automatically).

**No changes** to `editor_context.h`, `editor_panel.h`, `command.h`, `command_stack.h`, or `properties_panel.h`.

**New file**: `tests/f03_entity_selection_tests.cpp` — unit tests for `Selection` and `EditorSelection`.

## User stories

### Story 1 — Plain click selects entity (Priority: P1)

As an editor user, I want to click an entity in the Scene Panel to select it, so that I can indicate which entity I want to work with.

**Given** the Scene Panel shows 3 root entities ("Player", "Light", "Camera") with none selected
**When** I left-click "Player"
**Then** "Player" becomes highlighted in the tree
**And** `EditorSelection::size()` returns 1
**And** `EditorSelection::first()` returns "Player"'s EntityId

**Given** "Player" is currently selected
**When** I left-click "Light"
**Then** "Player" is no longer highlighted
**And** "Light" is highlighted
**And** `EditorSelection::size()` returns 1

### Story 2 — Ctrl+click toggles entity selection (Priority: P1)

As an editor user, I want to Ctrl+click multiple entities to select them simultaneously, so that I can perform bulk operations.

**Given** no entities are selected
**When** I Ctrl+click "Player"
**Then** "Player" is highlighted
**And** `EditorSelection::size()` returns 1

**Given** "Player" is currently selected
**When** I Ctrl+click "Light"
**Then** both "Player" and "Light" are highlighted
**And** `EditorSelection::size()` returns 2

**Given** both "Player" and "Light" are selected
**When** I Ctrl+click "Player"
**Then** "Player" is no longer highlighted
**And** "Light" remains highlighted
**And** `EditorSelection::size()` returns 1

### Story 3 — Shift+click range selection (Priority: P1)

As an editor user, I want to Shift+click to select a contiguous range of entities in the tree, so that I can quickly select a group of related entities.

**Given** the Scene Panel shows entities in depth-first order: "RootA", "ChildA1", "ChildA2", "RootB", "RootC"
**And** "RootA" is currently selected (anchor = "RootA")
**When** I Shift+click "RootC"
**Then** all 5 entities are highlighted ("RootA", "ChildA1", "ChildA2", "RootB", "RootC")
**And** `EditorSelection::size()` returns 5

**Given** the Scene Panel shows entities: "RootA", "ChildA1", "ChildA2", "RootB", "RootC"
**And** "RootC" is currently selected (anchor = "RootC")
**When** I Shift+click "RootA"
**Then** all 5 entities are highlighted
**And** `EditorSelection::size()` returns 5

### Story 4 — Ctrl+A selects all entities (Priority: P2)

As an editor user, I want to press Ctrl+A to select all entities in the scene, so that I can perform batch operations on the entire scene.

**Given** the World has 5 entities
**And** no entities are currently selected
**When** I press Ctrl+A
**Then** all 5 entities are highlighted
**And** `EditorSelection::size()` returns 5

**Given** an empty World (`entity_count() == 0`)
**When** I press Ctrl+A
**Then** the selection remains empty
**And** `EditorSelection::size()` returns 0

### Story 5 — Click empty area clears selection (Priority: P1)

As an editor user, I want to click empty space in the Scene Panel to clear my selection, so that I can deselect without selecting another entity.

**Given** "Player" is currently selected
**When** I left-click empty space in the Scene Panel (not on any entity)
**Then** "Player" is no longer highlighted
**And** `EditorSelection::size()` returns 0
**And** `EditorSelection::anchor()` returns `std::nullopt`

**Given** no entities are selected
**When** I left-click empty space (Ctrl or Shift held)
**Then** the selection remains empty (modifier is ignored on empty click)

### Story 6 — Selection cleared on new/open scene (Priority: P1)

As an editor user, I want my selection to be cleared when I create a new scene or load a different scene, so that I don't have stale selection state referencing destroyed entities.

**Given** "Player" is currently selected
**When** `editor.new_scene()` is called
**Then** `EditorSelection::size()` returns 0
**And** `EditorSelection::anchor()` returns `std::nullopt`

**Given** "Player" is currently selected
**When** `editor.open_scene(path)` loads successfully
**Then** `EditorSelection::size()` returns 0
**And** `EditorSelection::anchor()` returns `std::nullopt`

### Story 7 — Snapshot/restore for Commands (Priority: P2)

As a Command author, I want to save the current selection before executing an entity-modifying command and restore it on undo, so that the user's selection state is preserved across undo/redo.

**Given** "Player" is currently selected
**When** I call `auto saved = editor.selection().snapshot()`
**Then** `saved` contains a copy of the current selection with `contains("Player") == true`

**Given** the selection is currently empty
**When** I call `editor.selection().restore(saved)` where `saved` contains "Player"
**Then** `EditorSelection::contains("Player")` returns true
**And** `EditorSelection::size()` returns 1

### Story 8 — Re-click on selected entity is no-op (Priority: P2)

As an editor user, I want clicking an already-selected entity (without modifiers) to do nothing, so that I don't accidentally lose my selection when clicking a selected entity.

**Given** "Player" is currently selected
**When** I left-click "Player" (no modifiers)
**Then** "Player" remains selected
**And** `EditorSelection::size()` remains 1

### Story 9 — Shift+click without anchor degrades to Replace (Priority: P2)

As an editor user, I want Shift+click to work even if no anchor is set, so that the first click in an empty panel still does something reasonable.

**Given** no entities are selected (no anchor)
**When** I Shift+click "Player"
**Then** "Player" is selected (only)
**And** the anchor is set to "Player"
**And** `EditorSelection::size()` returns 1

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-01 | `Selection` class stores `EntityId`s in an unordered set. `contains(id)` returns true for added ids, false for others. | Unit test |
| AC-02 | `Selection::size()` returns the correct count. `empty()` returns true when no ids are stored. | Unit test |
| AC-03 | `Selection::first()` returns `std::nullopt` when empty, and a valid `EntityId` when non-empty. | Unit test |
| AC-04 | `Selection` supports copy semantics: copying a `Selection`, adding an id to the copy, verifying the original is unchanged. | Unit test |
| AC-05 | `Selection::add(id)`, `remove(id)`, `clear()` modify the local copy correctly. | Unit test |
| AC-06 | `Selection::operator==` returns true for equal sets, false for different sets. | Unit test |
| AC-07 | `Selection` is iterable via range-for. | Unit test |
| AC-08 | `EditorSelection::select(id, Replace)` clears any previous selection and selects only the given entity. | Unit test |
| AC-09 | `EditorSelection::select(id, Toggle)` adds an entity not already in the selection, removes it if already present. | Unit test |
| AC-10 | `EditorSelection::clear()` empties the selection and clears the anchor. | Unit test |
| AC-11 | `EditorSelection::set_selection(ids)` sets the exact list of entities, replacing any previous selection. | Unit test |
| AC-12 | `EditorSelection::snapshot()` returns a `Selection` copy; modifying the original after snapshot does not affect the copy. | Unit test |
| AC-13 | `EditorSelection::restore(saved)` replaces the current selection with the saved selection and fires callbacks. | Unit test |
| AC-14 | `EditorSelection::anchor()` and `set_anchor()` work correctly. `anchor()` returns `std::nullopt` after `clear()`. | Unit test |
| AC-15 | `EditorSelection::on_change()` returns a unique token; callback fires on `select()`, `clear()`, `set_selection()`, `restore()`. | Unit test |
| AC-16 | `EditorSelection::remove_on_change(token)` removes a previously registered callback; removed callback does not fire on subsequent changes. | Unit test |
| AC-17 | `Editor::selection()` returns a reference to the `EditorSelection` owned by the Editor. | Unit test |
| AC-18 | `Editor::new_scene()` clears the selection (size == 0, anchor == nullopt). | Unit test |
| AC-19 | `Editor::open_scene()` clears the selection on successful load. | Unit test |
| AC-20 | Scene Panel left-click on an entity selects it (Replace modifier): entity highlighted, anchor set. | Manual: click entity, see it highlighted |
| AC-21 | Scene Panel Ctrl+click toggles entity selection: Ctrl+click first entity selects it, Ctrl+click second adds it, Ctrl+click first removes it. | Manual: Ctrl+click multiple entities |
| AC-22 | Scene Panel Shift+click selects range in depth-first order. | Manual: click entity A, Shift+click entity B |
| AC-23 | Scene Panel Ctrl+A selects all entities in the World. | Manual: Ctrl+A, see all entities highlighted |
| AC-24 | Scene Panel click empty area clears selection and anchor. | Manual: click empty area, see highlights removed |
| AC-25 | Clicking already-selected entity (no modifier) is a no-op — does not deselect. | Manual: click selected entity, still selected |
| AC-26 | Shift+click without anchor selects only the clicked entity (degrade to Replace). | Manual: Shift+click without prior selection |
| AC-27 | Ctrl+click empty area is a no-op (modifier ignored). | Manual: Ctrl+click empty area, selection unchanged |
| AC-28 | Selected entities display with `ImGuiTreeNodeFlags_Selected` highlighting. | Manual: select entity, observe visual highlight |
| AC-29 | Ctrl+A on empty World is a no-op. | Manual: empty scene, Ctrl+A, no crash, selection empty |
| AC-30 | Ctrl+A shortcut is gated by `WantCaptureKeyboard`. | Manual: focus another ImGui widget (e.g., menu), Ctrl+A does not trigger select-all |
| AC-31 | All existing tests still pass. | Run `buddd_tests` |
| AC-32 | Zero new warnings from `src/editor/` and `tests/`. | Build with `cmake --build --preset debug` — verify zero warnings |

## E2E Verification

| Method | Description |
|---|---|
| **Headless unit test (CI)** | Build with `BUDDD_HAS_DISPLAY=OFF`. Run `buddd_tests`. Verify `[editor][selection]` tagged tests pass — `Selection` value semantics, `EditorSelection` mutations, snapshot/restore, callbacks. |
| **Manual smoke test (display)** | Run `buddd edit` with a scene loaded. Verify: plain click selects entity with visual highlight; Ctrl+click toggles; Shift+click range selects; Ctrl+A selects all; click empty area clears selection. Verify re-click on selected entity is no-op. Verify Shift+click without anchor degrades correctly. |
| **Clean build verification (CI)** | Run `cmake --build --preset debug` and verify zero warnings from `src/editor/` and `tests/`. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A user can click any entity in the Scene Panel tree and see it become visually highlighted (selected) immediately. | Manual: click entity, observe highlight |
| SC-002 | A user can select multiple entities using Ctrl+click and see all highlighted simultaneously. | Manual: Ctrl+click 3 entities, verify all 3 highlighted |
| SC-003 | A user can select a contiguous range of entities with Shift+click in depth-first tree order. | Manual: click "RootA", Shift+click "RootC", verify middle entities also selected |
| SC-004 | A user can select all entities with Ctrl+A in < 100 ms even with 1000+ entities. | Manual + rough timing: 1000 entities, Ctrl+A feels instant |
| SC-005 | Selection state is correctly cleared on new scene or open scene operations — no stale selection references. | Manual: select entity, File > New Scene, verify selection empty |
| SC-006 | `Selection` and `EditorSelection` are fully covered by unit tests with no regressions. | Run `buddd_tests`, verify all selection tests pass |

## Edge cases

| Case | Expected behavior |
|---|---|
| **Click already-selected entity (no modifier)** | No-op — stays selected. Anchor unchanged. |
| **Click already-selected entity (Ctrl)** | Removed from selection. If last entity, selection becomes empty. Anchor unchanged. |
| **Click empty area (no modifier)** | Selection cleared. Anchor cleared. |
| **Click empty area (Ctrl or Shift)** | No-op — modifier ignored on empty click. Selection and anchor unchanged. |
| **Shift+click with no anchor** | Degrade to Replace — select only the clicked entity, set it as anchor. |
| **Shift+click when anchor equals clicked entity** | Select only that single entity (range of length 1). |
| **Ctrl+A on empty World** | No-op — selection stays empty. |
| **new_scene() with non-empty selection** | Selection cleared. |
| **open_scene() with non-empty selection** | Selection cleared after successful load. |
| **Entity destroyed while selected** | Not handled in F-03 — deferred to F-04 where Commands manage via snapshot/restore. Selection continues to hold the destroyed EntityId. |
| **Rapid clicks** | Each click processed independently — no debouncing needed. ImGui event handling ensures clicks are serialized per-frame. |
| **Very long list of entities in range select (10,000+)** | `set_selection()` with a span of 10K+ ids is efficient (single operation). No copy of the entity list within `EditorSelection` — Scene Panel builds the list and passes as span. |
| **Multi-select with 0 entities in selection** | `first()` returns `nullopt`, `empty()` returns true, `size()` returns 0. `contains(id)` returns false for all ids. |
| **Multi-select of every entity in World** | All entities highlighted. `size()` matches `world.entity_count()`. |
| **Entity with `EntityId::none()` in selection** | Not possible — `EditorSelection` only adds valid `EntityId`s from `Entity::id()`. `EntityId::none()` is not a valid selectable entity. |
| **Selection on scene load failure** | `open_scene()` restores the previous World on failure, but selection was already cleared. This is fine — the user can re-select. |
| **Frame-perfect timing: click happens during `update()` vs. `draw_ui()`** | Click handling is in `draw_ui()` (ImGui event processing). Selection state persists between frames via `EditorSelection`. No timing issues. |
| **Ctrl+A while an ImGui text input is focused** | Gated by `WantCaptureKeyboard` — if an input is focused, `WantCaptureKeyboard` is true, and Ctrl+A is NOT processed by the editor shortcut system. |

## Error cases

| Case | Expected behavior |
|---|---|
| **`set_selection` called with empty span** | Selection becomes empty. Anchor unchanged. |
| **`select()` called with `EntityId::none()`** | Ignored — no-op (defensive guard). Not expected in practice. |
| **`restore()` called with `Selection` containing stale EntityIds** | Selection contains the stale ids. The caller (Command) is responsible for ensuring ids are valid. EditorSelection does not validate. |
| **Out-of-memory during `Selection` copy** | Standard C++ `std::bad_alloc` may be thrown. This is consistent with existing editor behaviour. |
| **`on_change` callback throws exception** | `EditorSelection::fire_callbacks()` does not catch exceptions. If a callback throws, the exception propagates. Callback code should be exception-safe. |
| **`remove_on_change()` called with invalid token** | No-op — the token is simply not found in the callbacks vector. |
| **Duplicate registration of same callback** | Allowed — each `on_change()` call creates a new entry with a unique token. Both will fire on change. |
| **Scene Panel rendering during selection modification mid-frame** | Not possible — `draw_ui()` is called once per frame and reads the selection state which is stable during the draw phase. Selection mutations happen during `update()` or during `draw_ui()` Input handling (ImGui IsItemClicked), but mutation callbacks are deferred. |

## Permissions and security

- No changes to permissions or security posture.
- The selection is entirely in-memory state — no file I/O, no network access, no sensitive data.
- No authentication or authorisation boundaries are crossed.
- Selection state is not persisted between editor sessions (not serialized to the `.yaml` scene file).

## Observability

| Signal | Source |
|---|---|
| **Selection changed** | `EditorSelection::fire_callbacks()` could emit `BUDDD_LOG_DEBUG("Selection changed: {} entities", current_.size())` — useful for debugging selection flow. |
| **Scene panel click** | Debug-level log: `BUDDD_LOG_DEBUG("ScenePanel: click on entity {} (modifier={})", entity.id().index, modifier_string)` — useful for debugging click handling. |
| **Ctrl+A triggered** | Debug-level log: `BUDDD_LOG_DEBUG("Ctrl+A: selected {} entities", count)` |
| **Scene clear on new_scene/open_scene** | Existing logs (`"New scene created"`, `"Scene loaded: ..."`) are sufficient. No additional logging needed for selection clear at these points. |
| **Snapshot/restore** | Debug-level log in Command execution: `BUDDD_LOG_DEBUG("Command: saving selection ({} entities)", saved.size())` and `BUDDD_LOG_DEBUG("Command: restoring selection ({} entities)", saved.size())` — added by Command authors, not by F-03. |

## Documentation impact

The following existing wiki pages must be updated when this spec is implemented:

| Document | Reason for update |
|---|---|
| `docs/wiki/editor/editor-panels.md` | Update the Scene Panel section to document selection behaviour (click-to-select, multi-select, selection highlighting). Update the "Entity Operations" table to reflect that "Select entity" is now implemented. |
| `docs/wiki/editor/cross-panel-communication.md` | Update the north-star status to reflect that F-03 implements the entity selection flow (Hierarchy click → selection state). Add `EditorSelection` as the new cross-panel communication mechanism. Mark the selection path from "future" to "partially implemented". |
| `docs/wiki/editor/scene-management.md` | No changes expected — `Editor::selection()` is an additive API that does not affect scene management. |

These updates are the responsibility of the implementation phase and will be tracked in the implementation contract.

## Out of scope

- PropertiesPanel (Inspector) population — empty placeholder remains (F-05).
- PropertiesPanel "No entity selected" state — deferred to F-05.
- Selection-change event consumers in Inspector, Viewport — deferred to F-05, F-07.
- Viewport mouse picking / click-to-select — deferred (future feature).
- Undo for selection changes — selection is not undoable.
- Keyboard-only selection navigation (arrow keys, Home/End).
- Entity destroyed → auto-remove from selection — deferred to F-04.
- Drag-to-select / rubber-band selection.
- Double-click selection behaviour.
- Right-click selection behaviour (context menu deferred).
- Selection persistence across editor sessions.
- Selection serialization in scene files.
- Any changes to World, Entity, or engine APIs.
- Any changes to PropertiesPanel, ConsolePanel, ProjectPanel, AssetsPanel.
- Any changes to Command or CommandStack base classes (used unchanged).

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `EntityId` has `index` (uint32_t) and `generation` (uint32_t) fields, is trivially copyable, and supports `operator==` and `std::hash` (for `std::unordered_set`). Confirmed by `entity_id.h`. |
| A-02 | `ImGui::IsItemClicked()`, `ImGui::IsWindowHovered()`, `ImGui::IsMouseClicked()`, `ImGui::IsAnyItemHovered()`, `ImGui::GetIO().KeyCtrl`, `ImGui::GetIO().KeyShift`, and `ImGuiTreeNodeFlags_Selected` are all available from the ImGui docking branch (v1.91.8-docking). |
| A-03 | The Editor's World is always valid (guaranteed by SPEC-029 / F-00). No null checks are needed when accessing `ctx.editor.world()` from Scene Panel. |
| A-04 | `World::entity_count()`, `World::root_entity_count()`, `World::get_root_entity(size_t)`, `Entity::child_count()`, `Entity::get_child(size_t)`, `Entity::id()` are all available as described in `world.h` and `entity.h`. |
| A-05 | `EditorSelection::on_change()` / `remove_on_change()` are designed for future use. In F-03, no consumers register callbacks. The callback infrastructure is tested but unused. |
| A-06 | `std::unordered_set<EntityId>` requires `std::hash<EntityId>`. A hash specialization must be provided in the `buddd::editor` namespace (or in `std`). This is a minor implementation detail noted for the implementation contract. |
| A-07 | Ctrl+A is registered as a `ShortcutRegistry` binding in `Editor::setup()`. The Scene Panel does NOT handle Ctrl+A directly via ImGui key events — it is handled at the Editor level. |
| A-08 | Entity IDs in the `set_selection` span are guaranteed to be valid (alive) at the time of the call. The Scene Panel's tree traversal only visits live entities. |
| A-09 | The Scene Panel's recursive tree traversal for Shift+click range is O(n) in the number of entities. For scenes with <10,000 entities, this is negligible (<0.1ms). For larger scenes, performance is acceptable for MVP1 (virtualized tree deferred). |
| A-10 | `ImGuiTreeNodeFlags_Selected` is styled by the active ImGui theme (dark theme by default). No custom styling is applied for selected items in F-03. |
| A-11 | The `Selection` class uses `std::unordered_set<EntityId>` internally. This is an implementation choice documented here; the implementation contract may choose a different container if justified (e.g., `std::vector` for cache-friendly iteration, `std::set` for deterministic ordering). |
| A-12 | `EditorSelection::first()` returns the first element from the underlying set, which is arbitrary (no ordering guarantee). This is documented as `std::optional<EntityId>` — the caller should not rely on any specific ordering. |

## Open questions

| ID | Question | Resolution |
|---|---|---|
| Q-01 | **Shift+click range: what is the anchor when selection is empty?** When selection is empty, `anchor()` returns `std::nullopt`. Shift+click in this case degrades to Replace (select only the clicked entity, set as anchor). This is documented in edge cases and Story 9. | **No clarification needed.** |
| Q-02 | **Ctrl+A: should the anchor change?** No — the anchor is unchanged by Ctrl+A. The anchor is only relevant for Shift+click operations. | **No clarification needed.** |
| Q-03 | **`ImGuiTreeNodeFlags_Selected` behaviour on multi-select: do all selected entities show the highlight?** Yes — ImGui applies the selected flag per-item, not per-selection. Each tree node with the flag renders with the selection highlight independently. All selected entities show the highlight simultaneously. | **No clarification needed.** |
