# IMPL-F-03 — Entity Selection with Multi-Select

## Source spec

- `.specs/sprint-2026-06/entity-selection/spec.md` (accepted)

## Goal

Implement entity selection for the Buddd Editor: a `Selection` value class (cloneable, comparable, iterable) backed by `std::unordered_set<EntityId>`, an `EditorSelection` manager owned by `Editor` with multi-select support (`SelectionModifier::Replace` / `Toggle`, anchor-based Shift+click range, Ctrl+A select-all), snapshot/restore for future Command undo/redo, and `on_change` callback infrastructure. Wire selection into the ScenePanel: plain click selects, Ctrl+click toggles, Shift+click range-selects, Ctrl+A selects all, click empty area clears. Selected entities render with `ImGuiTreeNodeFlags_Selected` highlighting. Selection is cleared on `new_scene()`/`open_scene()`.

## Non-goals

- No PropertiesPanel changes (remains empty placeholder — deferred to F-05).
- No selection-change event consumers (callback infrastructure exists but unused — Inspector/Viewport integration deferred to F-05/F-07).
- No viewport click-to-select (mouse picking — deferred).
- No undo for selection changes (selection is not undoable; snapshot/restore is for Command authors only).
- No keyboard-only selection navigation (arrow keys, Home/End).
- No entity-destroyed auto-clear (Commands manage this via snapshot/restore in F-04+).
- No drag-to-select (rubber-band / marquee).
- No double-click or right-click selection behavior.
- No changes to engine files (`world.h`, `entity.h`, `entity_id.h`).
- No changes to `editor_context.h`, `editor_panel.h`, `command.h`, `command_stack.h`, `properties_panel.h`, or any files under `src/engine/`.
- No changes to CMakeLists.txt (auto-discovered via `GLOB_RECURSE`).
- No selection persistence across editor sessions.
- No selection serialization in scene files.

## Relevant ADRs

| ADR | Relevance |
|---|---|
| ADR-027 (Editor Architecture) | Editor as static library with `Editor` class. This contract adds `Selection` and `EditorSelection` members. Direct member variables pattern (ADR-027 Decision 4) continues — `EditorSelection selection_` is a direct member. |
| ADR-029 (Editor UX Decisions) | Scene Panel (Hierarchy) is where entity selection originates. Selection state is the cross-panel communication mechanism (Decision 1: Tab-as-Editor-Context, Decision 7: Entity creation context). |
| ADR-011 (Ownership/Nullability/NoDiscard) | `[[nodiscard]]` on query methods (`contains()`, `size()`, `empty()`, `first()`, `snapshot()`, `anchor()`, `current()`). Reference members in `EditorContext` are always valid — access to `Editor::selection()` is safe. |
| ADR-019 (Architecture Boundaries) | No SDL3/OpenGL/GLM headers in `src/editor/`. `editor_selection.h` only includes `entity_id.h` from engine (a pure value header with no graphics/platform dependencies). |

## Files to inspect

| File | Reason |
|---|---|
| `src/editor/editor.h` | Current class declaration — must add `#include "editor_selection.h"`, `EditorSelection selection_` member, `selection()` accessor. |
| `src/editor/editor.cpp` | Current implementation — must add `editor_selection.h` include, implement `selection()`, add `selection_.clear()` in `new_scene()` and `open_scene()`, add Ctrl+A shortcut binding. |
| `src/editor/panels/scene_panel.h` | Current entity tree rendering — must add click handling, selection highlighting, empty-area click, tree traversal helper for Shift+click range, and select-all helper. |
| `src/editor/editor_context.h` | `EditorContext` struct — verify `ctx.editor` access pattern (unchanged). |
| `src/editor/shortcut_registry.h` | Verify `ShortcutRegistry::process()` gating behavior and `Action` callback signature (`(EngineContext const&)`). |
| `src/engine/scene/entity_id.h` | `EntityId` struct: `uint32_t index`, `uint32_t generation`, `operator==`. No `std::hash` specialization exists — will be provided in `editor_selection.h`. |
| `src/engine/scene/entity.h` | Entity API: `id()`, `child_count()`, `get_child()`, `name()` — needed for tree traversal helper. |
| `src/engine/scene/world.h` | World API: `entity_count()`, `root_entity_count()`, `get_root_entity()` — needed for tree traversal. |
| `tests/editor/editor_tests.cpp` | Existing test patterns: `HeadlessTestContext` helper, `[editor]`/`[f01]` tags, Catch2 `REQUIRE` style. |
| `tests/editor/scene_panel_tests.cpp` | F-02 test patterns: `HeadlessEnv` helper, static_assert checks, `[editor][scene_panel]` tags. |

## Files allowed to change

| File | Change type |
|---|---|
| `src/editor/editor_selection.h` | **create** — `Selection` class, `SelectionModifier` enum, `EditorSelection` class, `std::hash<EntityId>` specialization |
| `src/editor/editor.h` | **modify** — add `#include "editor_selection.h"`, add `EditorSelection selection_` private member, add `selection()` accessor |
| `src/editor/editor.cpp` | **modify** — add `selection()` implementation, `selection_.clear()` in `new_scene()` and `open_scene()`, Ctrl+A shortcut binding |
| `src/editor/panels/scene_panel.h` | **modify** — add click handling, selection highlighting, empty-area click, tree traversal helper, select-all helper |
| `tests/editor/f03_entity_selection_tests.cpp` | **create** — unit tests for `Selection`, `EditorSelection`, `Editor::selection()` integration |

## Files forbidden to change

- Any file under `src/engine/` — no changes to `World`, `Entity`, `EntityId`, or any engine file.
- `src/editor/editor_context.h` — no changes (struct stays as-is).
- `src/editor/editor_panel.h` — no changes.
- `src/editor/editor_menu.h` — no changes.
- `src/editor/command.h` — no changes (Command future integration noted in edge cases).
- `src/editor/command_stack.h` — no changes.
- `src/editor/panels/properties_panel.h` — no changes (remains empty placeholder).
- `src/editor/panels/console_panel.h`, `project_panel.h`, `assets_panel.h`, `menu_bar.h` — no changes.
- `src/editor/shortcut_registry.h` — no changes (used as-is). Ctrl+A gating handled in the callback via `ImGui::GetIO().WantCaptureKeyboard`.
- Any `CMakeLists.txt` — no build system changes (test file auto-discovered via `GLOB_RECURSE`).
- Any wiki or ADR files.

## Existing conventions to follow

1. **Include style**: `#include "..."` for project headers, `<...>` for system/external headers. Relative to `src/` (e.g., `#include "scene/entity_id.h"` for engine, `#include "editor_panel.h"` for editor).
2. **Namespace**: `buddd::editor` for editor code. `namespace std` is opened ONLY for explicit `template<>` specialization.
3. **`#pragma once`**: All new headers.
4. **`[[nodiscard]]`**: On all query-only methods (`contains()`, `size()`, `empty()`, `first()`, `snapshot()`, `anchor()`, `current()`).
5. **`noexcept`**: All `Selection` and `EditorSelection` methods are `noexcept` (pure in-memory state, no I/O, no allocations that can throw except `std::bad_alloc` from `std::unordered_set` operations — this is consistent with existing pattern).
6. **Include order**: Project headers first (alphabetical), then system headers.
7. **Type aliases in .cpp**: `editor.cpp` uses `namespace be = buddd::engine;`.
8. **ImGui include**: `<imgui.h>` for all ImGui types. Already included in `scene_panel.h`.
9. **Recursive lambda in headers**: Use C++20 generic recursive lambda for tree traversal (existing pattern from F-02).
10. **Entity iteration**: Use `root_entity_count()`/`get_root_entity()` for roots, `child_count()`/`get_child()` for children, matching the existing engine API.
11. **Container choice**: `std::unordered_set<EntityId>` for O(1) contains(). Custom `std::hash<EntityId>` specialization provided in `editor_selection.h`. EntityId is trivially copyable (8 bytes).
12. **Callback token model**: `size_t` tokens, monotonically increasing `next_token_` counter. Tokens are never reused.

## Required implementation behavior

### Step 1: Create `src/editor/editor_selection.h`

New file containing three things: **`std::hash<EntityId>` specialization**, **`SelectionModifier` enum**, **`Selection` class**, **`EditorSelection` class**.

```cpp
#pragma once

#include "scene/entity_id.h"

#include <algorithm>       // std::find_if for callback removal
#include <cstddef>         // size_t
#include <functional>      // std::function, std::hash
#include <optional>
#include <span>
#include <unordered_set>
#include <utility>         // std::pair
#include <vector>

// ── std::hash specialization for EntityId ────────────────────────────
// EntityId has no hash in its own header; we provide one here so
// std::unordered_set<EntityId> compiles. Placed in editor_selection.h
// rather than modifying the engine file.
template<>
struct std::hash<buddd::engine::EntityId> {
    auto operator()(buddd::engine::EntityId const& id) const noexcept -> size_t {
        // Simple hash: XOR index and generation (shifted to avoid collisions)
        return static_cast<size_t>(id.index) ^ (static_cast<size_t>(id.generation) << 16);
    }
};

// ── SelectionModifier enum ───────────────────────────────────────────

namespace buddd::editor {

enum class SelectionModifier {
    Replace,  // Clear + select this entity (plain click). Sets anchor.
    Toggle,   // Add or remove this entity (Ctrl+click). Anchor unchanged.
};

// ── Selection value class ────────────────────────────────────────────

class Selection {
public:
    // -- Query --
    [[nodiscard]] auto contains(EntityId id) const noexcept -> bool;
    [[nodiscard]] auto size() const noexcept -> size_t;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto first() const noexcept -> std::optional<EntityId>;

    // -- Iteration (expose the underlying set for range-for) --
    using const_iterator = std::unordered_set<EntityId>::const_iterator;
    auto begin() const noexcept -> const_iterator;
    auto end() const noexcept -> const_iterator;

    // -- Local mutation (builds a copy — does NOT affect EditorSelection) --
    auto add(EntityId id) -> void;
    auto remove(EntityId id) -> void;
    auto clear() -> void;

    // -- Comparison --
    auto operator==(const Selection&) const noexcept -> bool = default;

private:
    friend class EditorSelection;
    std::unordered_set<EntityId> selected_;
};

// ── EditorSelection manager ──────────────────────────────────────────

class EditorSelection {
public:
    // -- Query (delegates to current_) --
    [[nodiscard]] auto contains(EntityId id) const noexcept -> bool;
    [[nodiscard]] auto size() const noexcept -> size_t;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto first() const noexcept -> std::optional<EntityId>;

    // -- Mutation (all fire callbacks after modification) --
    void select(EntityId id, SelectionModifier modifier = SelectionModifier::Replace);
    void clear();
    void set_selection(std::span<const EntityId> ids);

    // -- Snapshot for Commands (F-04+) --
    [[nodiscard]] auto snapshot() const noexcept -> Selection;
    void restore(const Selection& saved);  // fires callbacks

    // -- Shift+click anchor --
    [[nodiscard]] auto anchor() const noexcept -> std::optional<EntityId>;
    void set_anchor(EntityId id);

    // -- Callbacks --
    using ChangeCallback = std::function<void()>;
    auto on_change(ChangeCallback cb) -> size_t;   // returns token
    void remove_on_change(size_t token);

    // -- Selection object access (for iteration) --
    [[nodiscard]] auto current() const noexcept -> const Selection&;

private:
    void fire_callbacks();

    Selection current_;
    std::optional<EntityId> anchor_;
    std::vector<std::pair<size_t, ChangeCallback>> callbacks_;
    size_t next_token_ = 0;
};

// ── Selection inline implementations ─────────────────────────────────

inline auto Selection::contains(EntityId id) const noexcept -> bool {
    return selected_.contains(id);
}

inline auto Selection::size() const noexcept -> size_t {
    return selected_.size();
}

inline auto Selection::empty() const noexcept -> bool {
    return selected_.empty();
}

inline auto Selection::first() const noexcept -> std::optional<EntityId> {
    if (selected_.empty()) return std::nullopt;
    return *selected_.begin();
}

inline auto Selection::begin() const noexcept -> const_iterator {
    return selected_.begin();
}

inline auto Selection::end() const noexcept -> const_iterator {
    return selected_.end();
}

inline auto Selection::add(EntityId id) -> void {
    selected_.insert(id);
}

inline auto Selection::remove(EntityId id) -> void {
    selected_.erase(id);
}

inline auto Selection::clear() -> void {
    selected_.clear();
}

// ── EditorSelection inline implementations ───────────────────────────

inline auto EditorSelection::contains(EntityId id) const noexcept -> bool {
    return current_.contains(id);
}

inline auto EditorSelection::size() const noexcept -> size_t {
    return current_.size();
}

inline auto EditorSelection::empty() const noexcept -> bool {
    return current_.empty();
}

inline auto EditorSelection::first() const noexcept -> std::optional<EntityId> {
    return current_.first();
}

inline auto EditorSelection::snapshot() const noexcept -> Selection {
    return current_;  // copy — independent clone
}

inline auto EditorSelection::anchor() const noexcept -> std::optional<EntityId> {
    return anchor_;
}

inline auto EditorSelection::set_anchor(EntityId id) -> void {
    anchor_ = id;
}

inline auto EditorSelection::current() const noexcept -> const Selection& {
    return current_;
}

} // namespace buddd::editor
```

**Implementation notes for `editor_selection.h`:**

- **All methods are inline** in the header (consistent with existing header-only editor patterns like `shortcut_registry.h`). No `.cpp` file.

- **`select(id, Replace)`:**
  1. If `id == EntityId::none()`, return immediately (defensive no-op guard per DC-01).
  2. `current_.selected_.clear()`
  3. `current_.selected_.insert(id)`
  4. `anchor_ = id`
  5. `fire_callbacks()`

- **`select(id, Toggle)`:**
  1. If `id == EntityId::none()`, return immediately (defensive no-op guard per DC-01).
  2. If entity already in `current_.selected_`: erase it. Otherwise: insert it.
  3. `anchor_` is unchanged.
  4. `fire_callbacks()`

- **`clear()`:**
  1. `current_.selected_.clear()`
  2. `anchor_ = std::nullopt`
  3. `fire_callbacks()`

- **`set_selection(ids)`:**
  1. `current_.selected_.clear()`
  2. For each `id` in the span: if `id != EntityId::none()`, insert it into `current_.selected_` (silently skip `EntityId::none()` per DC-01).
  3. `anchor_` unchanged.
  4. `fire_callbacks()`

- **`restore(saved)`:**
  1. `current_ = saved` (copy the Selection)
  2. `fire_callbacks()` — fires regardless of whether the restored selection differs from the current one.

- **Callbacks fire on every mutation call** (`select`, `clear`, `set_selection`, `restore`) regardless of net state change. This is intentional for F-03 simplicity. Future features (F-05+) may add change-detection gating.

- **`fire_callbacks()`**: Iterates `callbacks_` in order and calls each `ChangeCallback`. Does not catch exceptions. The vector may be non-owning (tokens are `size_t`).

- **`remove_on_change(token)`**: Uses `std::find_if` to locate the pair with matching token, then erases it. If token not found, this is a no-op.

- **`EntityId::none()` guard**: `select()` and `set_selection()` should silently ignore `EntityId::none()` entries (defensive guard, not expected in practice).

### Step 2: Modify `src/editor/editor.h`

Additions:

1. After `#include "shortcut_registry.h"`, add `#include "editor_selection.h"` (alphabetical: `"editor_selection.h"` after `"editor_menu.h"`, before `"scene/world.h"`):
   ```cpp
   #include "command_stack.h"
   #include "editor_menu.h"
   #include "editor_panel.h"
   #include "editor_selection.h"    // <-- NEW
   #include "scene/world.h"
   #include "shortcut_registry.h"
   ```

2. Add private member after `World` unique_ptr:
   ```cpp
   // ── Selection state (F-03) ──
   EditorSelection selection_;
   ```

3. Add public accessor after `world()`:
   ```cpp
   /// Returns the active selection manager.
   [[nodiscard]] auto selection() -> EditorSelection&;
   ```

### Step 3: Modify `src/editor/editor.cpp`

Changes:

1. **`Editor::selection()` implementation** (add after `Editor::world()`):
   ```cpp
   auto Editor::selection() -> EditorSelection& {
       return selection_;
   }
   ```

2. **`Editor::new_scene()`**: Add `selection_.clear();` at the beginning (before or after creating a new World — order doesn't matter, but putting it before the world reset ensures any stale references are cleared first):
   ```cpp
   auto Editor::new_scene() -> void {
       selection_.clear();
       // ... existing code ...
   }
   ```

3. **`Editor::open_scene()`**: Add `selection_.clear();` after successful load (inside the `if (result.has_value())` block):
   ```cpp
   if (result.has_value()) {
       selection_.clear();
       current_file_path_ = path;
       // ... rest of existing code ...
   }
   ```

4. **Ctrl+A shortcut**: Register in `Editor::setup()` after existing shortcuts (e.g., after Ctrl+Y binding):
   ```cpp
   shortcuts_.bind(be::KeyCode::A, {.ctrl = true}, [this](be::EngineContext const&) {
       // Gate: do nothing if ImGui captures keyboard (e.g., text input focused)
       if (ImGui::GetIO().WantCaptureKeyboard) return;
       
       // Collect all entity IDs via tree traversal
       auto& w = world();
       std::vector<buddd::engine::EntityId> all_ids;
       all_ids.reserve(w.entity_count());
       
       // Recursive traversal
       auto collect = [&](auto& self, buddd::engine::Entity entity) -> void {
           all_ids.push_back(entity.id());
           for (size_t i = 0; i < entity.child_count(); ++i) {
               self(self, entity.get_child(i));
           }
       };
       for (size_t i = 0; i < w.root_entity_count(); ++i) {
           auto entity = w.get_root_entity(i);
           if (entity.id() != buddd::engine::EntityId::none()) {
               collect(collect, entity);
           }
       }
       
       BUDDD_LOG_DEBUG("Ctrl+A: selected {} entities", all_ids.size());
       selection_.set_selection(all_ids);
   });
   ```

   Note: The `ImGui::GetIO().WantCaptureKeyboard` check ensures Ctrl+A does NOT fire when an ImGui text input is focused (AC-30). This is needed because `ShortcutRegistry::process()` currently bypasses the `want_capture` gate for modifier-key shortcuts.

### Step 4: Modify `src/editor/panels/scene_panel.h`

Changes to the `ScenePanel` class:

1. **Include `"scene/entity.h"`** (for `Entity::get_child()`, `Entity::child_count()` — already transitively included via `"scene/world.h"`, but add explicit include for `Entity` access):
   - Already has `#include "scene/world.h"` which includes `"scene/entity.h"`. No additional include needed.

2. **Add private helper methods** to `ScenePanel`:
   ```cpp
   private:
       /// Collect EntityIds in depth-first tree order between anchor and clicked (inclusive).
       /// Both anchor and clicked must be valid (alive) EntityIds.
       auto collect_range(buddd::engine::World& world,
                           buddd::engine::EntityId anchor,
                           buddd::engine::EntityId clicked) const
           -> std::vector<buddd::engine::EntityId>;
   
       /// Collect all EntityIds in depth-first tree order.
       auto collect_all(buddd::engine::World& world) const
           -> std::vector<buddd::engine::EntityId>;
   ```

3. **Modify the recursive `render_entity` lambda** in `draw_ui()`:

   a. **Add `flags` for selected state** after the leaf check:
      ```cpp
      // After: if (entity.child_count() == 0) { flags |= ImGuiTreeNodeFlags_Leaf; }
      if (ctx.editor.selection().contains(entity.id())) {
          flags |= ImGuiTreeNodeFlags_Selected;
      }
      ```

   b. **Add click handling** after `ImGui::TreeNodeEx()` call (inside the same scope, before `if (expanded)`):
      ```cpp
      // Click handling — after TreeNodeEx call, before if(expanded)
      if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
          if (ImGui::GetIO().KeyShift) {
              // Shift+click: range selection
              auto anchor = ctx.editor.selection().anchor();
              if (anchor.has_value()) {
                  auto range = collect_range(ctx.editor.world(), *anchor, entity.id());
                  ctx.editor.selection().set_selection(range);
                  // anchor unchanged
              } else {
                  // No anchor: degrade to Replace
                  ctx.editor.selection().select(entity.id(), SelectionModifier::Replace);
              }
          } else if (ImGui::GetIO().KeyCtrl) {
              // Ctrl+click: toggle
              ctx.editor.selection().select(entity.id(), SelectionModifier::Toggle);
              // anchor unchanged
          } else {
              // Plain click: replace selection
              ctx.editor.selection().select(entity.id(), SelectionModifier::Replace);
              // Replace sets anchor internally
          }
      }
      ```

   c. **No change to tree node expansion logic** — the `if (expanded)` block stays identical.

4. **Empty-area click handling**: Add at the end of `draw_ui()`, before the closing brace of `draw_ui()` (outside the per-entity loop, after the root entity iteration):
   ```cpp
    // Empty-area click: if window is hovered and no item is hovered, clear selection.
    // Ctrl+click and Shift+click on empty area are no-ops (AC-27).
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
        if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift) {
            ctx.editor.selection().clear();
        }
    }
   ```

5. **Implement `collect_range` helper**:
   ```cpp
   inline auto ScenePanel::collect_range(buddd::engine::World& world,
                                          buddd::engine::EntityId anchor,
                                          buddd::engine::EntityId clicked) const
       -> std::vector<buddd::engine::EntityId>
   {
       // Collect all entities in depth-first order
       auto all = collect_all(world);
       
       // Find indices of anchor and clicked
       auto anchor_it = std::find(all.begin(), all.end(), anchor);
       auto clicked_it = std::find(all.begin(), all.end(), clicked);
       
       // Both must be found (guaranteed by caller)
       if (anchor_it == all.end() || clicked_it == all.end()) {
           return {clicked};  // fallback: just the clicked entity
       }
       
        // Return the subspan between anchor and clicked (inclusive)
        // NOTE: manual min/max to avoid std::minmax(const T&, const T&) dangling reference UB
        // when passed temporary size_t values.
        auto a_idx = static_cast<size_t>(std::distance(all.begin(), anchor_it));
        auto b_idx = static_cast<size_t>(std::distance(all.begin(), clicked_it));
        auto first = (std::min)(a_idx, b_idx);
        auto last  = (std::max)(a_idx, b_idx);
       
       return std::vector<buddd::engine::EntityId>(
           all.begin() + static_cast<ptrdiff_t>(first),
           all.begin() + static_cast<ptrdiff_t>(last) + 1);
   }
   ```

6. **Implement `collect_all` helper**:
   ```cpp
   inline auto ScenePanel::collect_all(buddd::engine::World& world) const
       -> std::vector<buddd::engine::EntityId>
   {
       std::vector<buddd::engine::EntityId> ids;
       ids.reserve(world.entity_count());
       
       auto collect = [&](auto& self, buddd::engine::Entity entity) -> void {
           ids.push_back(entity.id());
           for (size_t i = 0; i < entity.child_count(); ++i) {
               self(self, entity.get_child(i));
           }
       };
       
       for (size_t i = 0; i < world.root_entity_count(); ++i) {
           auto entity = world.get_root_entity(i);
           if (entity.id() != buddd::engine::EntityId::none()) {
               collect(collect, entity);
           }
       }
       
       return ids;
   }
   ```

**Implementation notes for ScenePanel:**

- Click detection uses `ImGui::IsItemClicked(ImGuiMouseButton_Left)` which fires on both single and double clicks. Double-click's first click triggers selection (Replace). This is standard ImGui behavior and within NG-09's "no double-click behaviour" scope (the second click of a double-click does nothing special).
- The `KeyShift`/`KeyCtrl` checks use `ImGui::GetIO().KeyShift`/`KeyCtrl` which reflect current modifier state at the time of the click.
- Empty-area click detection: `IsWindowHovered()` must be true, `IsMouseClicked(Left)` must be true, and `IsAnyItemHovered()` must be false. This prevents clearing selection when clicking on an entity (which is hovered via `IsItemHovered` after `TreeNodeEx`).
- The existing entity tree has no `update()` override — all logic stays in `draw_ui()`. This is acceptable because selection state persists in `EditorSelection` (which lives in `Editor`) across frames.
- No `on_selection_change` callback registration in ScenePanel — the tree reads `ctx.editor.selection()` every frame in `draw_ui()`, so highlighting updates automatically.

### Step 5: Create test file `tests/editor/f03_entity_selection_tests.cpp`

Follow existing patterns from `tests/editor/scene_panel_tests.cpp` and `tests/editor/editor_tests.cpp`. Use `#include "editor_selection.h"` and `#include "editor.h"`.

## Required tests

### Unit tests in `tests/editor/f03_entity_selection_tests.cpp`

Test cases tagged with `[editor][selection]`.

| Test | AC(s) | What it verifies |
|---|---|---|
| `Selection: contains/size/empty` | AC-01, AC-02 | Default-constructed Selection is empty. After `add(id)`, `contains(id)` returns true, `size()` returns 1, `empty()` returns false. |
| `Selection: first() behavior` | AC-03 | Empty: `first()` returns `nullopt`. Non-empty: `first()` returns a valid `EntityId`. |
| `Selection: copy semantics` | AC-04 | Copy a Selection. Add id to copy. Original is unchanged (`contains` returns false). |
| `Selection: add/remove/clear` | AC-05 | `add()` adds, `remove()` removes, `clear()` empties. |
| `Selection: operator==` | AC-06 | Equal sets compare equal. Different sets compare unequal. |
| `Selection: range-for iteration` | AC-07 | Insert 3 ids. Iterate with range-for. All 3 are visited. |
| `EditorSelection: select(Replace)` | AC-08 | Select id1. Then select(id2, Replace). Selection contains only id2. Anchor set to id2. |
| `EditorSelection: select(Toggle)` | AC-09 | Select id1 via Replace. Then toggle(id2): id1 and id2 both in selection. Then toggle(id1): id1 removed, id2 remains. Anchor unchanged by toggle. |
| `EditorSelection: clear()` | AC-10, AC-14 | Select id1. `clear()` empties selection and clears anchor (`anchor()` returns `nullopt`). |
| `EditorSelection: set_selection` | AC-11 | Set selection from a span of 3 ids. Contains all 3. Anchor unchanged (no anchor initially). |
| `EditorSelection: snapshot isolation` | AC-12 | Select id1. Snapshot. Add id2 to snapshot. Original selection still contains only id1. |
| `EditorSelection: restore` | AC-13 | Select id1. Snapshot. Select id2. Restore snapshot. Selection contains only id1. Callback fires. |
| `EditorSelection: anchor` | AC-14 | `anchor()` returns nullopt initially. `set_anchor(id)` sets it. `clear()` clears it. |
| `EditorSelection: on_change fires` | AC-15 | Register callback. Call `select()`, `clear()`, `set_selection()`, `restore()` — each fires callback once. |
| `EditorSelection: remove_on_change` | AC-16 | Register callback, get token. Remove via token. Subsequent mutations do NOT fire the removed callback. |
| `Editor::selection() accessor` | AC-17 | Create Editor. Call `editor.selection()`. Returns `EditorSelection&`. Verify via `select()` + `contains()`. |
| `Editor::new_scene() clears selection` | AC-18 | Select entity. `new_scene()`. Size == 0. Anchor == nullopt. |
| `Editor::open_scene() clears selection on success` | AC-19 | Select entity. `open_scene()` to a valid temp file. Size == 0. Anchor == nullopt. |
| `No warnings from src/editor/` | AC-32 | Build with `cmake --build --preset debug` — zero warnings from `src/editor/` and `tests/`. |
| `All existing tests pass` | AC-31 | Run `buddd_tests` — all tests pass. |

**Testing approach for headless:**

- `Selection` and `EditorSelection` tests are pure value/behavioral tests with no ImGui dependency — they run in headless mode.
- `Editor::selection()` integration test creates an `Editor` in headless mode and verifies the accessor works. `new_scene()` / `open_scene()` tests create an `Editor` in headless mode (using `HeadlessTestContext` from F-01 tests).
- For `open_scene()` testing: use `HeadlessTestContext`, call `editor.setup()` (may fail in headless), then `editor.save_scene_as(temp_path)`, then `editor.open_scene(temp_path)`. If setup fails (no ImGui), the test is skipped with a note.

**ACs verified manually (display-dependent):**

- AC-20 through AC-30 (ScenePanel click interactions, highlighting, Ctrl+A gating) are verified via manual smoke test on display builds. The contract specifies precise implementation behavior that must be followed.

### E2E / Integration verification

| Method | Description |
|---|---|
| **Build verification (CI)** | `cmake --build --preset debug` succeeds with zero new warnings from `src/editor/` and `tests/`. |
| **Test suite pass (CI)** | `buddd_tests` — all existing tests pass. No new test failures. |
| **Manual smoke test (display)** | Run `buddd edit` with a scene loaded. Verify: plain click selects entity with visual highlight; Ctrl+click toggles; Shift+click range selects; Ctrl+A selects all; click empty area clears selection. Verify re-click on selected entity is no-op. Verify Shift+click without anchor degrades correctly. |
| **Code review** | Verify `ImGuiTreeNodeFlags_Selected` flag, click handling branches (plain/Ctrl/Shift), empty-area click logic, tree traversal for range selection, anchor management, callback firing patterns, and `EntityId::none()` guard. |

## Edge cases

| Case | Expected behavior | Verified in |
|---|---|---|
| **Click already-selected entity (no modifier)** | No-op — stays selected. Anchor unchanged (Replace sets anchor to the same id which is a no-op). | Manual smoke test |
| **Click already-selected entity (Ctrl)** | Removed from selection. If last entity, selection becomes empty. Anchor unchanged. | Unit test (AC-09) |
| **Click empty area (no modifier)** | Selection cleared. Anchor cleared. | Manual smoke test |
| **Click empty area (Ctrl or Shift)** | No-op — modifier ignored on empty click. Selection and anchor unchanged. | Manual smoke test |
| **Shift+click with no anchor** | Degrade to Replace — select only the clicked entity, set it as anchor. | Manual smoke test |
| **Shift+click when anchor equals clicked entity** | `collect_range()` returns vector containing only that entity. Single-entity selection. | Code review |
| **Ctrl+A on empty World** | No-op — selection stays empty. | Manual smoke test |
| **`new_scene()` with non-empty selection** | Selection cleared. | Unit test (AC-18) |
| **`open_scene()` with non-empty selection** | Selection cleared after successful load. | Unit test (AC-19) |
| **`set_selection` called with empty span** | Selection becomes empty. Anchor unchanged. | Code review of `set_selection` implementation |
| **`select()` called with `EntityId::none()`** | Ignored — no-op (defensive guard). | Code review of select implementation |
| **`restore()` called with stale `Selection`** | Selection contains the stale ids. Caller (Command) is responsible for validity. EditorSelection does not validate. | Code review |
| **`remove_on_change()` called with invalid token** | No-op — token not found in callbacks vector. | Code review of `remove_on_change` (uses find-if + erase) |
| **Duplicate registration of same callback** | Allowed — each `on_change()` creates unique entry with unique token. Both fire. | Code review of `on_change` |
| **Callback fires on no-op mutation** | Callbacks fire regardless of net state change. No consumers in F-03 so harmless. | Code review of `fire_callbacks()` calls in `select`, `clear`, `set_selection`, `restore` |
| **Anchor asymmetry: `clear()` vs `set_selection({})`** | `clear()` clears anchor. `set_selection({})` leaves anchor unchanged. This is intentional documented behavior. | Code review |
| **Double-click entity** | First click of double-click triggers selection (Replace modifier). Second click is a no-op per NG-09 (no special double-click behavior). This is standard ImGui behavior — `IsItemClicked()` fires for both single and double clicks. | Code review |
| **Entity with `EntityId::none()` in tree traversal** | Guard `entity.id() != EntityId::none()` skips null entities during root iteration and child traversal. | Code review of existing F-02 pattern + new helpers |
| **Ctrl+A while ImGui input focused** | Gated by `ImGui::GetIO().WantCaptureKeyboard` check in the shortcut callback. | Manual smoke test (AC-30) |
| **Future Command integration** | `Command::execute()` takes no context argument. Commands needing `Editor::selection()` must capture `Editor&` in their constructor. This is a future concern for F-04+. The `snapshot()`/`restore()` API exists now for this purpose. | Not verified in F-03 (documented) |

## Security impact

None. Selection is entirely in-memory state — no file I/O, no network access, no authentication or authorisation boundaries crossed. No new input parsing or data exposure.

## Data and migration impact

None. No schema changes, no data migrations, no seed data, no data loss risks. Selection state is not persisted between editor sessions.

## API compatibility impact

- **New public API**: `Editor::selection()` returns `EditorSelection&`. This is a pure addition — no existing callers are affected.
- **New file**: `src/editor/editor_selection.h` — new types `Selection`, `EditorSelection`, `SelectionModifier`. These are internal to the editor library and not part of any public SDK.
- **No breaking changes**: No existing public API signatures or behavior are changed. All additions are backwards-compatible.
- **`ShortcutRegistry`**: Used as-is. The Ctrl+A callback captures `this` (Editor*) and accesses `selection_` directly. The shortcut registry callback signature `(EngineContext const&)` is unchanged.

## Documentation impact

- **README**: None.
- **Wiki pages** (to be updated by wiki-agent):
  - `docs/wiki/editor/editor-panels.md` — Update: Document the `EditorSelection` manager and `Selection` value class. Mark entity selection as implemented (was "deferred" in F-02). Update the "Entity Operations" table: "Select entity" is now implemented. Update the "v1 foundation" section to reflect F-03 additions.
  - `docs/wiki/editor/cross-panel-communication.md` — Update: F-03 implements the entity selection flow (Hierarchy click → selection state). Add `EditorSelection` as the new cross-panel communication mechanism. Mark the selection path from "future" to "partially implemented" (selection exists, no consumers yet). Update north-star status.
- **Other specs**: None.

## ADR impact

No new ADR needed. The `Selection` value class and `EditorSelection` manager follow existing patterns (direct member variables per ADR-027, `[[nodiscard]]` per ADR-011, namespace `buddd::editor` per ADR-027). No existing ADR is deprecated or amended.

## Done criteria

The Code Agent must satisfy all of the following:

- [ ] **DC-01**: `src/editor/editor_selection.h` created with:
  - `std::hash<EntityId>` specialization in `namespace std`.
  - `SelectionModifier` enum (`Replace`, `Toggle`).
  - `Selection` class with `contains()`, `size()`, `empty()`, `first()`, `begin()`/`end()`, `add()`, `remove()`, `clear()`, `operator==`. Uses `std::unordered_set<EntityId>` internally.
  - `EditorSelection` class with `contains()`, `size()`, `empty()`, `first()`, `select(id, modifier)`, `clear()`, `set_selection(span)`, `snapshot()`, `restore()`, `anchor()`, `set_anchor()`, `on_change()`, `remove_on_change()`, `current()`. All methods are `noexcept`. All query methods are `[[nodiscard]]`.
  - `select(Replace)` clears + selects + sets anchor. `select(Toggle)` adds/removes + anchor unchanged. `clear()` empties + clears anchor. `set_selection(span)` empties + inserts span + anchor unchanged. `restore(Selection)` replaces + fires callbacks.
  - Callbacks fire on every mutation (`select`, `clear`, `set_selection`, `restore`) regardless of net state change.
  - `on_change()` returns monotonically incrementing `size_t` token. `remove_on_change(token)` erases the matching callback; no-op if token not found.
  - `EntityId::none()` entries are silently ignored by `select()` and `set_selection()`.

- [ ] **DC-02**: `src/editor/editor.h` — `#include "editor_selection.h"` added. `EditorSelection selection_` private member added. `[[nodiscard]] auto selection() -> EditorSelection&;` public accessor added.

- [ ] **DC-03**: `src/editor/editor.cpp`:
  - `Editor::selection()` returns `selection_` reference.
  - `Editor::new_scene()` calls `selection_.clear()` before or after creating the new World.
  - `Editor::open_scene()` calls `selection_.clear()` inside the success path (after `result.has_value()` check).
  - Ctrl+A shortcut bound via `shortcuts_.bind(be::KeyCode::A, {.ctrl = true}, ...)`.
  - Ctrl+A callback: gated by `ImGui::GetIO().WantCaptureKeyboard` (no-op if true), collects all entity IDs via depth-first traversal, calls `selection_.set_selection(all_ids)`.

- [ ] **DC-04**: `src/editor/panels/scene_panel.h`:
  - Entity tree rendering checks `ctx.editor.selection().contains(entity.id())` and adds `ImGuiTreeNodeFlags_Selected` to `flags`.
  - Left-click on entity (no modifier): `ctx.editor.selection().select(entity.id(), SelectionModifier::Replace)`.
  - Ctrl+click on entity: `ctx.editor.selection().select(entity.id(), SelectionModifier::Toggle)`.
  - Shift+click on entity: if anchor exists, collect range via `collect_range()` and call `ctx.editor.selection().set_selection(range)`. If anchor does not exist, degrade to Replace via `select(id, Replace)`.
  - Empty-area click (window hovered + mouse clicked left + no item hovered): `ctx.editor.selection().clear()`.
  - `collect_range(world, anchor, clicked)` private helper: collects all entities in depth-first order via `collect_all()`, finds anchor and clicked indices, returns the subspan inclusive.
  - `collect_all(world)` private helper: traverses all root entities and their children recursively in depth-first order, returns `std::vector<EntityId>`.
  - Entity names fallback `"(unnamed)"` preserved. `PushID`/`PopID` pattern preserved. Null entity guard (`EntityId::none()` check) preserved.

- [ ] **DC-05**: `tests/editor/f03_entity_selection_tests.cpp` created with tests tagged `[editor][selection]` covering:
  - `Selection`: contains/size/empty (AC-01, AC-02), first() (AC-03), copy semantics (AC-04), add/remove/clear (AC-05), operator== (AC-06), range-for iteration (AC-07).
  - `EditorSelection`: select(Replace) (AC-08), select(Toggle) (AC-09), clear() + anchor (AC-10, AC-14), set_selection (AC-11), snapshot isolation (AC-12), restore fires callbacks (AC-13), anchor/set_anchor/clear clears anchor (AC-14), on_change fires on each mutation (AC-15), remove_on_change (AC-16).
  - `Editor::selection()` accessor (AC-17).
  - `Editor::new_scene()` clears selection (AC-18).
  - `Editor::open_scene()` clears selection on success (AC-19).

- [ ] **DC-06**: `cmake --build --preset debug` succeeds with **zero new warnings** from `src/editor/` and `tests/`.

- [ ] **DC-07**: All existing tests pass: `buddd_tests` run with zero failures.

- [ ] **DC-08**: No changes to files under `src/engine/`, `src/editor/editor_context.h`, `src/editor/editor_panel.h`, `src/editor/editor_menu.h`, `src/editor/command.h`, `src/editor/command_stack.h`, `src/editor/panels/properties_panel.h`, `src/editor/panels/console_panel.h`, `src/editor/panels/project_panel.h`, `src/editor/panels/assets_panel.h`, `src/editor/panels/menu_bar.h`, `src/editor/shortcut_registry.h`, or any `CMakeLists.txt`.
