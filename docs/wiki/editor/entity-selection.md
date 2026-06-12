# Entity Selection

> **Current status (F-03 — entity-selection-multi-select, June 2026):** Entity selection with full multi-select support is implemented. The `Selection` value class, `EditorSelection` manager, and Scene Panel click handling are all functional. No downstream consumers yet (Inspector and Viewport updates deferred to F-05/F-07).

The editor provides a unified selection system for entities. Selection is managed through two classes in `src/editor/editor_selection.h`:

| Class | Role |
|---|---|
| `Selection` | Pure value object — a snapshot of which entities are selected. Cloneable, comparable, saveable in Commands. |
| `EditorSelection` | Active selection manager — owns the current `Selection`, handles modifier-aware mutations, fires change callbacks. |

`Editor` owns an `EditorSelection` instance, accessible via `editor.selection()` from any panel or command.

---

## How it works

### Multi-Select Interactions

In the Scene Panel (Hierarchy), the following interactions are supported:

| Input | Modifier | Behavior | Anchor |
|---|---|---|---|
| Left-click entity | *(none)* | **Replace**: clear selection → select clicked entity | Set to clicked entity |
| Left-click entity | `Ctrl` | **Toggle**: if selected → remove. If not → add. | Unchanged |
| Left-click entity | `Shift` | **Range**: select all entities from anchor to clicked (depth-first order, inclusive) | Unchanged |
| `Ctrl+A` | *(global shortcut)* | **Select all**: all entities in the World become selected | Unchanged |
| Click empty area | *(none)* | **Clear**: selection emptied, anchor cleared | Cleared |
| Click empty area | `Ctrl` or `Shift` | **No-op**: modifier keys are ignored on empty-area clicks | Unchanged |
| Click already-selected entity | *(none)* | **No-op**: selection remains unchanged | Unchanged |

**Shift+click anchor:** The anchor is set on every plain (Replace) click. It is the starting point for the next Shift+click range. The anchor is cleared when selection is explicitly cleared, and is never modified by Toggle or Range operations.

**Range order:** Entities between anchor and clicked are selected regardless of direction — clicking an entity above the anchor works the same as clicking one below. The range follows depth-first tree traversal order (the visual order in the panel).

---

## API Reference

### `Selection` (value class)

A cloneable snapshot of a set of selected entity IDs. This is a value type — equality-comparable, copyable, and independent of the `EditorSelection` manager.

```cpp
class Selection {
public:
    // -- Query --
    [[nodiscard]] auto contains(EntityId id) const noexcept -> bool;
    [[nodiscard]] auto size() const noexcept -> size_t;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto first() const noexcept -> std::optional<EntityId>;

    // -- Iteration (range-for over EntityId) --
    using const_iterator = /* set::const_iterator */;
    auto begin() const noexcept -> const_iterator;
    auto end() const noexcept -> const_iterator;

    // -- Local mutation (modifies this copy only) --
    auto add(EntityId id) -> void;      // silently ignores EntityId::none()
    auto remove(EntityId id) -> void;   // silently ignores EntityId::none()
    auto clear() -> void;

    // -- Comparison --
    auto operator==(const Selection&) const noexcept -> bool = default;

private:
    friend class EditorSelection;
    std::unordered_set<EntityId> selected_;
};
```

**Usage notes:**
- `first()` returns the first element in iteration order (determined by `std::unordered_set`). For deterministic semantics, do not rely on a specific entity being "first."
- `add()` and `remove()` silently ignore `EntityId::none()` — the invalid sentinel is never stored.
- A `Selection` constructed by `EditorSelection::snapshot()` is fully independent — mutating it does NOT affect the editor's active selection. To apply changes, call `EditorSelection::restore()`.

### `SelectionModifier` enum

```cpp
enum class SelectionModifier {
    Replace,  // Clear + select this entity (plain click). Sets anchor.
    Toggle,   // Add or remove this entity (Ctrl+click). Anchor unchanged.
};
```

### `EditorSelection` (manager)

The active selection manager owned by `Editor`. All mutation methods fire registered callbacks.

```cpp
class EditorSelection {
public:
    // -- Query (delegates to the current Selection) --
    [[nodiscard]] auto contains(EntityId id) const noexcept -> bool;
    [[nodiscard]] auto size() const noexcept -> size_t;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto first() const noexcept -> std::optional<EntityId>;
    [[nodiscard]] auto current() const noexcept -> const Selection&;

    // -- Mutation (all fire callbacks) --
    void select(EntityId id, SelectionModifier modifier = SelectionModifier::Replace);
    void clear();
    void set_selection(std::span<const EntityId> ids);

    // -- Snapshot/restore for Commands --
    [[nodiscard]] auto snapshot() const noexcept -> Selection;
    void restore(const Selection& saved);  // fires callbacks

    // -- Shift+click anchor --
    [[nodiscard]] auto anchor() const noexcept -> std::optional<EntityId>;
    void set_anchor(EntityId id);

    // -- Callbacks --
    using ChangeCallback = std::function<void()>;
    auto on_change(ChangeCallback cb) -> size_t;   // returns token
    void remove_on_change(size_t token);
};
```

**Mutation behaviour:**

| Method | Selection effect | Anchor effect | Callbacks |
|---|---|---|---|
| `select(id, Replace)` | Clear → add `id` | Set to `id` | Fires |
| `select(id, Toggle)` | If present → remove. If absent → add | Unchanged | Fires |
| `clear()` | Empty | Cleared | Fires |
| `set_selection(ids)` | Replace with given span | Unchanged | Fires |
| `restore(saved)` | Replace with saved copy | Unchanged (saved from snapshot) | Fires |
| `snapshot()` | No change | No change | None |

---

## Snapshot / Restore Pattern (Undo/Redo)

Commands that modify entities (F-04+) use this pattern to preserve selection across undo/redo:

```cpp
class DeleteEntityCommand final : public Command {
public:
    DeleteEntityCommand(Editor& editor, /* ... */)
        : editor_(&editor)
    {
        pre_selection_ = editor.selection().snapshot();  // 📸 capture
    }

    auto execute() -> void override {
        // ... delete entity from world ...
        editor_->selection().clear();  // update selection
    }

    auto undo() -> void override {
        // ... restore entity in world ...
        editor_->selection().restore(pre_selection_);  // ↩️ restore
    }

private:
    Editor* editor_;
    Selection pre_selection_;
};
```

This approach is preferred over path-based resolution (walking the hierarchy by name or index) because:

- The Command **knows** the new `EntityId` — it just created the entity during `undo()`
- No ambiguity from duplicate entity names (permitted by ADR-029)
- No stale-EntityId window — selection is managed synchronously with the mutation

---

## Selection Lifecycle

| Event | Selection effect | Source |
|---|---|---|
| Plain click entity | Set to clicked entity | `ScenePanel::draw_ui()` |
| Ctrl+click entity | Toggle entity | `ScenePanel::draw_ui()` |
| Shift+click entity | Range (anchor → clicked) | `ScenePanel::draw_ui()` |
| Click empty area | Cleared | `ScenePanel::draw_ui()` |
| `Ctrl+A` | All entities selected | `Editor::update()` (shortcut) |
| `new_scene()` | Cleared | `Editor::new_scene()` |
| `open_scene(path)` | Cleared | `Editor::open_scene()` |
| Entity destroyed while selected | *Not auto-cleared* | Deferred to F-04 Commands |

**Note:** F-03 does not auto-clear selection when an entity is destroyed. When F-04 adds entity CRUD, each Command will manage selection explicitly via the snapshot/restore pattern. Meanwhile, if an entity is destroyed programmatically while selected, the selection will contain a stale `EntityId`. This is safe — the `EntityId` will fail to resolve in the World — but the selection will not be cleared until the user clicks elsewhere.

---

## Access from Panels

Panels access selection through the `EditorContext`:

```cpp
// In any EditorPanel::draw_ui() or update():
void MyPanel::draw_ui(EditorContext const& ctx) {
    // Query
    if (ctx.editor.selection().contains(entity.id())) {
        // entity is selected
    }
    auto count = ctx.editor.selection().size();

    // Mutation (ScenePanel only — other panels should query, not mutate)
    ctx.editor.selection().select(entity.id(), SelectionModifier::Toggle);
}
```

The `Editor` itself exposes selection via:

```cpp
class Editor {
public:
    [[nodiscard]] auto selection() -> EditorSelection&;
    [[nodiscard]] auto selection() const -> EditorSelection const&;
};
```

---

## Callbacks

`EditorSelection` supports change callbacks for cross-panel communication (future use — Inspector in F-05, Viewport in F-07):

```cpp
// Register a callback (returns a unique token for later removal)
auto token = editor.selection().on_change([]() {
    // React to selection change
});

// Remove the callback when no longer needed
editor.selection().remove_on_change(token);
```

Callbacks fire on **every mutation** regardless of whether the selection set actually changed. If change-detection gating is needed (e.g., skip callback if the set is identical), it can be added by a future consumer.

---

## Backing Store

`Selection` uses `std::unordered_set<EntityId>` internally:

- **O(1)** `contains()` — called per-entity per frame during tree rendering
- **O(1)** average insert/erase for toggle operations
- **Deterministic iteration** within a single frame (sufficient for range selection and Inspector query)

`EntityId` (8 bytes: `uint32_t index` + `uint32_t generation`) has a custom `std::hash` specialization defined in `editor_selection.h` — no modification to the engine header was needed.

---

## Related specs

- [F-03 Spec — Entity Selection with Multi-Select](/.specs/sprint-2026-06/entity-selection/spec.md) — Functional spec, acceptance criteria, edge cases
- [F-02 Spec — Scene Panel Entity Tree](/.specs/sprint-2026-06/scene-panel-entity-tree/spec.md) — Prerequisite: entity tree rendering
- [F-05 Spec — Inspector Transform](/.specs/sprint-2026-06/editor-ux-design/spec.md) — Future consumer of selection events

## Related ADRs

- [ADR-029](/docs/adr/ADR-029-editor-ux-decisions.md) — Editor UX decisions (entity selection flow, multi-select deferred to MVP1 — now implemented as F-03)
- [ADR-027](/docs/adr/ADR-027-editor-architecture.md) — Editor architecture (separate static library, namespace, architecture boundary)

## Related code

- `src/editor/editor_selection.h` — `Selection`, `EditorSelection`, `SelectionModifier` (inline implementation)
- `src/editor/editor.h` — `Editor::selection()` accessor
- `src/editor/panels/scene_panel.h` — Click handling, selection highlighting, tree traversal helpers (`collect_range`, `collect_all`)
- `src/editor/editor.cpp` — `new_scene()`/`open_scene()` selection clear, `Ctrl+A` shortcut binding
- `tests/editor/entity_selection_tests.cpp` — 21 unit tests covering all selection operations

## Last reviewed

2026-06-12 — Initial version for F-03 (Entity Selection with Multi-Select)
