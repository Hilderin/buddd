# IMPL-F-04 — Scene Panel — Entity Operations

## Source spec

- `.specs/sprint-2026-06/entity-operations/spec.md` (accepted)

## Goal

Implement entity lifecycle operations (Create, Delete, Rename) in the Scene Panel with undo/redo support. Change `Command::execute()` and `undo()` signatures to accept `EditorContext const&`, update `CommandStack` to forward it, update `QuitCommand` to use `ctx.engine`, update `MenuBar` undo/redo call-sites to pass context, and add `Editor::command_stack()` accessor. Add context menus (right-click on entity / empty area), Delete key handling, F2 inline rename, confirmation dialog for hierarchical deletion, and `flush_destroyed()` call in `Editor::update()`. All three operations support undo/redo, with selection snapshot/restore.

## Non-goals

- No toolbar buttons for create/delete — context menu only.
- No drag-and-drop reparenting.
- No duplicate/copy-paste entity.
- No prefab operations.
- No undo history visualization.
- No component state preservation on delete undo (v1: only entity identity, name, hierarchy restored).
- No changes to `Editor::update()`/`draw_ui()` public signatures (remain `EngineContext const&`).
- No changes to engine files (`src/engine/`).
- No changes to `CMakeLists.txt` (auto-discovery via `GLOB_RECURSE`).
- No changes to `editor_selection.h` (uses existing `Selection`, `EditorSelection` APIs as-is).

## Relevant ADRs

| ADR | Relevance |
|---|---|
| ADR-027 (Editor Architecture) | Commands are classes stored in `CommandStack` (direct member in `Editor`). Commands have no external dependency injection — they receive context via `EditorContext`. |
| ADR-029 (Editor UX Decisions) | Decision 7: Entity creation as child of selected — F-04 uses `EditorSelection::anchor()` which is the implementation of this decision. Decision 8: Prefab editing deferred — no prefab operations. |
| ADR-011 (Ownership/Nullability/NoDiscard) | `[[nodiscard]]` on `command_stack()` accessor. No raw pointers in new public API. |
| ADR-026 (Dear ImGui Integration) | ImGui docking branch provides `TreeNodeEx`, `InputText`, `BeginPopupContextItem`, `BeginPopupModal`, `IsWindowFocused`, `IsKeyPressed`, `SetKeyboardFocusHere`. All standard ImGui API. |

## Files to inspect

| File | Reason |
|---|---|
| `src/editor/command.h` | Current `Command` base class — `execute()`/`undo()` signatures to change. |
| `src/editor/command_stack.h` | Current `CommandStack` — execute/undo/redo signatures to change. |
| `src/editor/command_stack.cpp` | Current implementation — update to pass `ctx` to command methods. Add debug logging. |
| `src/editor/commands/quit_command.h` | Must update to use `ctx.engine` instead of stored `EngineContext const*`. |
| `src/editor/editor.h` | Must add `command_stack()` accessor. |
| `src/editor/editor.cpp` | Must add `command_stack()` impl, `world().flush_destroyed()` in `update()`, update undo/redo shortcut callbacks to create `EditorContext`. |
| `src/editor/panels/menu_bar.h` | Undo/redo call-sites must pass `ctx` to `command_stack_.undo(ctx)`/`redo(ctx)`. |
| `src/editor/panels/scene_panel.h` | Current entity tree with selection. Must add context menus, keyboard handling, rename state, confirmation dialog. |
| `src/editor/editor_context.h` | Reference — `EditorContext` struct definition (unchanged). |
| `src/editor/editor_selection.h` | Reference — `Selection`, `EditorSelection` APIs for snapshot/restore. |
| `tests/editor/editor_tests.cpp` | Existing `ToggleCommand` test helper — must update `execute()`/`undo()` signatures. |
| `tests/editor/entity_selection_tests.cpp` | Reference for test patterns. |

## Files allowed to change

| File | Change type |
|---|---|
| `src/editor/command.h` | **modify** — `execute()` and `undo()` signature change: add `EditorContext const& ctx` parameter. |
| `src/editor/command_stack.h` | **modify** — `execute()`, `undo()`, `redo()` signature change: add `EditorContext const& ctx`. |
| `src/editor/command_stack.cpp` | **modify** — forward `ctx` to command methods. Add `#include "log/log.h"`. Add debug logging per spec. |
| `src/editor/commands/quit_command.h` | **modify** — `execute()` takes `EditorContext const& ctx`, uses `ctx.engine.request_exit()`. Remove stored `EngineContext const* ctx_`. |
| `src/editor/commands/create_entity_command.h` | **create** — `CreateEntityCommand` class. |
| `src/editor/commands/delete_entity_command.h` | **create** — `DeleteEntityCommand` class. |
| `src/editor/commands/rename_entity_command.h` | **create** — `RenameEntityCommand` class. |
| `src/editor/editor.h` | **modify** — add `[[nodiscard]] auto command_stack() -> CommandStack&;` public accessor. |
| `src/editor/editor.cpp` | **modify** — add `command_stack()` impl. Add `world().flush_destroyed()` in `update()`. Update undo/redo shortcut callbacks to create `EditorContext`. |
| `src/editor/panels/menu_bar.h` | **modify** — `command_stack_.undo()` → `command_stack_.undo(ctx)`. Same for `redo()`. |
| `src/editor/panels/scene_panel.h` | **modify** — add context menus, keyboard handling (Delete, F2, Enter, Escape during rename), inline rename state (`renaming_entity_`, `rename_buffer_`), confirmation dialog, `#include "command_stack.h"` and command headers. |
| `tests/editor/editor_tests.cpp` | **modify** — update `ToggleCommand::execute()`/`undo()` to accept `EditorContext const&`. Update `CommandStack` test calls to pass a dummy `EditorContext` or `EngineContext`. |
| `tests/editor/entity_operations_tests.cpp` | **create** — unit tests for all three commands and undo/redo. |

## Files forbidden to change

- Any file under `src/engine/` — no changes to `World`, `Entity`, `EntityId`, `EngineContext`, or any engine file.
- `src/editor/editor_context.h` — no changes.
- `src/editor/editor_panel.h` — no changes.
- `src/editor/editor_menu.h` — no changes.
- `src/editor/editor_selection.h` — no changes (F-03 delivered it as-is).
- `src/editor/shortcut_registry.h` — no changes.
- Any `CMakeLists.txt` — no build system changes.
- Any wiki or ADR files.

## Existing conventions to follow

1. **Include style**: `#include "..."` for project headers, relative to `src/` (e.g., `#include "command.h"` for editor, `#include "scene/entity_id.h"` for engine).
2. **Namespace**: `buddd::editor` for editor code. `namespace std` opened only for explicit `template<>` specialization.
3. **`#pragma once`**: All new headers.
4. **`[[nodiscard]]`**: On all query-only methods (`command_stack()` accessor, `name()`).
5. **`noexcept`**: Not required on `execute()`/`undo()` (they may interact with World which can throw `std::bad_alloc`).
6. **Include order**: Project headers first (alphabetical by relative path), then system/external headers.
7. **Type aliases**: `editor.cpp` uses `namespace be = buddd::engine;`.
8. **ImGui include**: `<imgui.h>` for all ImGui types.
9. **Header-only commands**: The three new commands are header-only (no `.cpp` files), following the `QuitCommand` pattern.
10. **Command file location**: `src/editor/commands/` subdirectory.

## Required implementation behavior

### Step 1: Update `src/editor/command.h` — Command base class signature change

Change both pure virtual methods:

```cpp
virtual auto execute(EditorContext const& ctx) -> void = 0;
virtual auto undo(EditorContext const& ctx) -> void = 0;
```

Add forward declaration before `namespace buddd::editor`:
```cpp
struct EditorContext;
```

No other changes. The `name()` method stays unchanged.

### Step 2: Update `src/editor/command_stack.h` — CommandStack signature change

Add `EditorContext const& ctx` parameter to `execute()`, `undo()`, `redo()`:

```cpp
auto execute(std::unique_ptr<Command> command, EditorContext const& ctx) -> void;
[[nodiscard]] auto undo(EditorContext const& ctx) -> bool;
[[nodiscard]] auto redo(EditorContext const& ctx) -> bool;
```

Forward-declare `struct EditorContext;` at the top (or include `"editor_context.h"`).

### Step 3: Update `src/editor/command_stack.cpp` — forward context + logging

Changes:

1. **`#include "log/log.h"`** after `#include "command_stack.h"` (for logging macros).

2. **`execute()`**:
   ```cpp
   auto CommandStack::execute(std::unique_ptr<Command> command, EditorContext const& ctx) -> void {
       command->execute(ctx);
       BUDDD_LOG_DEBUG("[Command] {}: executed", command->name());
       undo_stack_.push_back(std::move(command));
       redo_stack_.clear();
       if (undo_stack_.size() > max_history_) {
           undo_stack_.erase(undo_stack_.begin());
       }
   }
   ```

3. **`undo()`**:
   ```cpp
   auto CommandStack::undo(EditorContext const& ctx) -> bool {
       if (undo_stack_.empty()) return false;
       auto command = std::move(undo_stack_.back());
       undo_stack_.pop_back();
       command->undo(ctx);
       BUDDD_LOG_DEBUG("[Command] {}: undone", command->name());
       redo_stack_.push_back(std::move(command));
       return true;
   }
   ```

4. **`redo()`**:
   ```cpp
   auto CommandStack::redo(EditorContext const& ctx) -> bool {
       if (redo_stack_.empty()) return false;
       auto command = std::move(redo_stack_.back());
       redo_stack_.pop_back();
       command->execute(ctx);
       BUDDD_LOG_DEBUG("[Command] {}: redone", command->name());
       undo_stack_.push_back(std::move(command));
       return true;
   }
   ```

### Step 4: Update `src/editor/commands/quit_command.h` — use ctx.engine

Before:
```cpp
explicit QuitCommand(buddd::engine::EngineContext const& ctx) : ctx_(&ctx) {}
auto execute() -> void override { ctx_->request_exit(); }
auto undo() -> void override {}
```

After:
```cpp
QuitCommand() = default;  // no constructor args needed

auto execute(EditorContext const& ctx) -> void override {
    ctx.engine.request_exit();
}

auto undo(EditorContext const& /*ctx*/) -> void override {
    // No-op: cannot un-request exit
}
```

Remove the stored `ctx_` member. Remove `#include "engine_context.h"` (not needed if only `EditorContext` is used). Add `#include "editor_context.h"` for `EditorContext` definition. Keep `#include "command.h"`.

**Note**: Any caller that constructs `QuitCommand` with an `EngineContext const&` must be updated to construct without arguments. Check `editor.cpp` for the quit shortcut callback.

### Step 5: Update `src/editor/panels/menu_bar.h` — pass ctx to undo/redo

Change lines 83-88:
```cpp
if (ImGui::MenuItem("Undo", "Ctrl+Z", false, command_stack_.can_undo())) {
    [[maybe_unused]] auto _ = command_stack_.undo(ctx);  // was: .undo()
}
if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z", false, command_stack_.can_redo())) {
    [[maybe_unused]] auto _ = command_stack_.redo(ctx);  // was: .redo()
}
```

### Step 6: Update `src/editor/editor.h` — add command_stack() accessor

Add public accessor after `selection()`:
```cpp
/// Returns the command stack for executing undoable commands.
[[nodiscard]] auto command_stack() -> CommandStack&;
```

No other changes. `command_stack_` remains a private member.

### Step 7: Update `src/editor/editor.cpp` — accessor, flush_destroyed, shortcut updates

1. **`Editor::command_stack()`** (add after `Editor::selection()`):
   ```cpp
   auto Editor::command_stack() -> CommandStack& {
       return command_stack_;
   }
   ```

2. **`Editor::update()`** — add `world().flush_destroyed()` after the panel update loop:
   ```cpp
   auto Editor::update(be::EngineContext const& ctx) -> void {
       if (!initialized_) return;

       shortcuts_.process(ctx, ImGui::GetIO().WantCaptureKeyboard);

       auto editor_ctx = EditorContext{*this, ctx};
       for (auto& menu : menus_) {
           menu->update(editor_ctx);
       }
       for (auto& panel : panels_) {
           panel->update(editor_ctx);
       }

       // Remove entities marked for destruction this frame
       world().flush_destroyed();
   }
   ```

3. **Ctrl+Z / Ctrl+Y / Ctrl+Shift+Z shortcut callbacks** — must create `EditorContext` to pass to updated `CommandStack`:
   ```cpp
   shortcuts_.bind(be::KeyCode::Z, {.ctrl = true}, [this](be::EngineContext const& ectx) {
       auto editor_ctx = EditorContext{*this, ectx};
       [[maybe_unused]] auto _ = command_stack_.undo(editor_ctx);
   });
   shortcuts_.bind(be::KeyCode::Z, {.ctrl = true, .shift = true}, [this](be::EngineContext const& ectx) {
       auto editor_ctx = EditorContext{*this, ectx};
       [[maybe_unused]] auto _ = command_stack_.redo(editor_ctx);
   });
   shortcuts_.bind(be::KeyCode::Y, {.ctrl = true}, [this](be::EngineContext const& ectx) {
       auto editor_ctx = EditorContext{*this, ectx};
       [[maybe_unused]] auto _ = command_stack_.redo(editor_ctx);
   });
   ```

### Step 8: Create `src/editor/commands/create_entity_command.h`

```cpp
#pragma once

#include "command.h"
#include "editor_context.h"

#include "log/log.h"
#include "scene/entity.h"
#include "scene/world.h"

#include <string>
#include <string_view>

namespace buddd::editor {

/// Command that creates a new empty entity as a child of the selection anchor
/// (or as a root entity if no anchor exists).
class CreateEntityCommand final : public Command {
public:
    auto execute(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();
        auto& selection = ctx.editor.selection();
        auto anchor = selection.anchor();

        // Save pre-execution selection for undo
        pre_execution_selection_ = selection.snapshot();

        // Determine parent: use anchor if present, otherwise root (EntityId::none())
        buddd::engine::EntityId parent_id = anchor.value_or(buddd::engine::EntityId::none());

        // Create entity
        buddd::engine::Entity new_entity;
        if (parent_id != buddd::engine::EntityId::none()) {
            // Find the parent entity in the world
            // Entity::get_child is not a static lookup — we need to find entity by ID.
            // Use a traversal or world API. For now assume World::find_entity(id) exists
            // or traverse. But actually World doesn't have a find_entity — we use
            // Entity::create_child() from the parent entity. We need to get the parent Entity.
            // In the existing engine API, we can iterate roots and traverse to find.
            // Simplest: use a helper or assume ctx.editor.world() has a lookup.
            // For v1, traverse the entity tree to find the parent entity.
            // BUT — this is expensive. A better approach: store a parent EntityId and
            // in the ScenePanel, pass the parent Entity directly. However, the command
            // doesn't have direct access to Entity objects.
            //
            // Since the editor owns the World and World doesn't expose get_entity(EntityId),
            // we need another approach. The simplest: have the ScenePanel find the parent
            // Entity before constructing the command, or use a World API.
            //
            // For now, use world.add_entity() and then reparent via the entity tree.
            // Actually, looking at the engine API: World::add_entity() creates a root.
            // There is no reparent API in v1.
            //
            // DECISION: For create, the parent Entity must be looked up. We traverse:
            auto find_entity = [&](auto& self, buddd::engine::Entity e) -> std::optional<buddd::engine::Entity> {
                if (e.id() == parent_id) return e;
                for (size_t i = 0; i < e.child_count(); ++i) {
                    auto found = self(self, e.get_child(i));
                    if (found.has_value()) return found;
                }
                return std::nullopt;
            };
            for (size_t i = 0; i < world.root_entity_count(); ++i) {
                auto root = world.get_root_entity(i);
                if (root.id() == parent_id) {
                    new_entity = root.create_child();
                    break;
                }
                auto found = find_entity(find_entity, root);
                if (found.has_value()) {
                    new_entity = found->create_child();
                    break;
                }
            }
        } else {
            new_entity = world.add_entity();
        }

        // Fallback: if parent entity not found (e.g. externally destroyed between undo and redo),
        // create as root instead of silently becoming a no-op.
        if (new_entity.id() == buddd::engine::EntityId::none()) {
            new_entity = world.add_entity();
            BUDDD_LOG_DEBUG("CreateEntity: parent entity {} not found, created as root",
                parent_id.index);
        }

        // Store created entity ID for undo
        created_entity_id_ = new_entity.id();

        BUDDD_LOG_DEBUG("CreateEntity: entity {} created under {}",
            created_entity_id_.index,
            parent_id != buddd::engine::EntityId::none()
                ? std::to_string(parent_id.index)
                : std::string("root"));
    }

    auto undo(EditorContext const& ctx) -> void override {
        // Destroy the created entity
        if (created_entity_id_ != buddd::engine::EntityId::none()) {
            // Find and destroy entity by traversing the world
            auto& world = ctx.editor.world();
            auto find_entity = [&](auto& self, buddd::engine::Entity e) -> bool {
                if (e.id() == created_entity_id_) {
                    e.destroy();
                    return true;
                }
                for (size_t i = 0; i < e.child_count(); ++i) {
                    if (self(self, e.get_child(i))) return true;
                }
                return false;
            };
            bool found = false;
            for (size_t i = 0; i < world.root_entity_count(); ++i) {
                auto root = world.get_root_entity(i);
                if (root.id() == created_entity_id_) {
                    root.destroy();
                    found = true;
                    break;
                }
                if (find_entity(find_entity, root)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                BUDDD_LOG_DEBUG("CreateEntity::undo: entity {} already destroyed",
                    created_entity_id_.index);
            }
            created_entity_id_ = buddd::engine::EntityId::none();
        }

        // Restore selection
        ctx.editor.selection().restore(pre_execution_selection_);
    }

    [[nodiscard]] auto name() const -> std::string_view override {
        return "Create Entity";
    }

private:
    buddd::engine::EntityId created_entity_id_ = buddd::engine::EntityId::none();
    Selection pre_execution_selection_;
};

} // namespace buddd::editor
```

**Important notes for `CreateEntityCommand`**:
- The parent Entity lookup via tree traversal is O(n). This is acceptable for v1. A future optimization would add `World::find_entity(EntityId)` or `World::get_entity(EntityId)`.
- If the parent entity no longer exists when `execute()` runs, create as root (fallback: call `world.add_entity()`).
- The `created_entity_id_` acts as a flag: if `execute()` succeeds, it's set; `undo()` checks it and destroys. On `redo()`, `execute()` is called again, which re-traverses and re-creates, setting a new `created_entity_id_`.
- `Selection snapshot`: saved as `pre_execution_selection_` at the start of `execute()`, restored in `undo()`.

### Step 9: Create `src/editor/commands/delete_entity_command.h`

```cpp
#pragma once

#include "command.h"
#include "editor_context.h"
#include "editor_selection.h"  // for EntityId hash, Selection

#include "log/log.h"
#include "scene/entity.h"
#include "scene/world.h"

#include <string>
#include <string_view>
#include <vector>

namespace buddd::editor {

/// Serialisable state of a single deleted entity (identity, name, hierarchy).
/// Component state is NOT preserved in v1.
struct SavedEntityState {
    buddd::engine::EntityId old_id;
    std::string name;
    buddd::engine::EntityId parent_old_id;  // EntityId::none() for root
};

/// Command that destroys one or more entities with confirmation-gated destruction.
/// Snapshot of selection saved pre-execution and restored on undo.
class DeleteEntityCommand final : public Command {
public:
    /// @param entity_ids  The top-level entities to delete (children are derived automatically).
    explicit DeleteEntityCommand(std::vector<buddd::engine::EntityId> entity_ids)
        : entity_ids_(std::move(entity_ids))
    {}

    auto execute(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();

        // Save pre-execution selection for undo
        pre_execution_selection_ = ctx.editor.selection().snapshot();

        // Collect all entities to delete (selected + all descendants)
        saved_entities_.clear();

        auto collect_and_save = [&](auto& self, buddd::engine::Entity entity) -> void {
            auto parent_entity = entity.parent();
            SavedEntityState state;
            state.old_id = entity.id();
            state.name = entity.name();
            state.parent_old_id = parent_entity.id();
            saved_entities_.push_back(std::move(state));

            for (size_t i = 0; i < entity.child_count(); ++i) {
                self(self, entity.get_child(i));
            }
        };

        // Find each top-level entity and collect
        for (auto id : entity_ids_) {
            auto find_and_collect = [&](auto& self, buddd::engine::Entity e) -> bool {
                if (e.id() == id) {
                    collect_and_save(self, e);
                    return true;
                }
                for (size_t i = 0; i < e.child_count(); ++i) {
                    if (self(self, e.get_child(i))) return true;
                }
                return false;
            };
            // Search roots
            for (size_t i = 0; i < world.root_entity_count(); ++i) {
                auto root = world.get_root_entity(i);
                if (root.id() == id) {
                    // Root entity: use entity_ptr_0 to find it
                    collect_and_save(self, root);
                    break;
                }
                if (find_and_collect(find_and_collect, root)) break;
            }
        }

        // Destroy all collected entities (in reverse order: children before parents)
        for (auto it = saved_entities_.rbegin(); it != saved_entities_.rend(); ++it) {
            auto find_and_destroy = [&](auto& self, buddd::engine::Entity e) -> bool {
                if (e.id() == it->old_id) {
                    e.destroy();
                    return true;
                }
                for (size_t i = 0; i < e.child_count(); ++i) {
                    if (self(self, e.get_child(i))) return true;
                }
                return false;
            };
            for (size_t i = 0; i < world.root_entity_count(); ++i) {
                auto root = world.get_root_entity(i);
                if (root.id() == it->old_id) {
                    root.destroy();
                    break;
                }
                if (find_and_destroy(find_and_destroy, root)) break;
            }
        }

        // Clear selection
        ctx.editor.selection().clear();

        BUDDD_LOG_DEBUG("DeleteEntity: {} entities destroyed", saved_entities_.size());
    }

    auto undo(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();

        // Recreate entities in saved order (parents first — already parent-first
        // because we collected parent-first during execute)
        // Map old EntityId → new Entity entity for parenting lookup
        // Since Entity is a value type and we're creating new ones, we track by old_id.
        // We need to find the new parent Entity for each recreated entity.
        struct RecreatedEntity {
            buddd::engine::EntityId old_id;
            buddd::engine::EntityId new_id;
        };
        std::vector<RecreatedEntity> recreated;

        for (auto& saved : saved_entities_) {
            buddd::engine::Entity new_entity;

            if (saved.parent_old_id != buddd::engine::EntityId::none()) {
                // Find the recreated parent entity by old parent ID
                auto parent_it = std::find_if(recreated.begin(), recreated.end(),
                    [&](const RecreatedEntity& r) { return r.old_id == saved.parent_old_id; });
                if (parent_it != recreated.end()) {
                    // Need to find the parent Entity object to call create_child()
                    auto find_entity_by_id = [&](auto& self, buddd::engine::Entity e) -> std::optional<buddd::engine::Entity> {
                        if (e.id() == parent_it->new_id) return e;
                        for (size_t i = 0; i < e.child_count(); ++i) {
                            auto found = self(self, e.get_child(i));
                            if (found.has_value()) return found;
                        }
                        return std::nullopt;
                    };
                    for (size_t i = 0; i < world.root_entity_count(); ++i) {
                        auto root = world.get_root_entity(i);
                        if (root.id() == parent_it->new_id) {
                            new_entity = root.create_child();
                            break;
                        }
                        auto found = find_entity_by_id(find_entity_by_id, root);
                        if (found.has_value()) {
                            new_entity = found->create_child();
                            break;
                        }
                    }
                } else {
                    // Parent not recreated yet — should not happen (parent-first order)
                    // Fallback: create as root
                    new_entity = world.add_entity();
                }
            } else {
                new_entity = world.add_entity();
            }

            // Set name
            new_entity.set_name(saved.name);

            recreated.push_back({saved.old_id, new_entity.id()});
        }

        // Update entity_ids_ with new EntityIds so redo's execute()
        // can find the recreated entities by their current IDs.
        // (EntityIds are generational — recycled slots get new IDs.)
        for (auto& id : entity_ids_) {
            for (auto const& r : recreated) {
                if (r.old_id == id) {
                    id = r.new_id;
                    break;
                }
            }
        }

        // Restore selection
        ctx.editor.selection().restore(pre_execution_selection_);

        // Keep entity_ids_ permanent (now updated to new IDs) so redo's execute() can
        // find the recreated entities. saved_entities_ will be overwritten by execute() on redo.

        BUDDD_LOG_DEBUG("DeleteEntity::undo: {} entities recreated", recreated.size());
    }

    [[nodiscard]] auto name() const -> std::string_view override {
        return "Delete Entity";
    }

private:
    std::vector<buddd::engine::EntityId> entity_ids_;
    std::vector<SavedEntityState> saved_entities_;
    Selection pre_execution_selection_;
};

} // namespace buddd::editor
```

**Notes for DeleteEntityCommand**:
- The `entity_ids_` passed to the constructor are the top-level selected IDs. Descendants are collected during `execute()` via tree traversal.
- On `undo()`, entities are recreated parent-first (matching the collection order). A map from old_id → new_id tracks the recreated entities for parenting.
- **Critical**: After `undo()` recreates entities, `entity_ids_` is updated from the old→new mapping so that redo's `execute()` can find the recreated entities by their current (new) `EntityId`. Without this update, `execute()` on redo would search for stale IDs, find nothing, and silently no-op — violating AC-29.
- `execute()` always collects fresh state using `entity_ids_` (which after undo+redo contains the current IDs), destroying whatever entities match those IDs. `undo()` always restores all saved entities. The `entity_ids_` vector persists across the undo/redo cycle.

### Step 10: Create `src/editor/commands/rename_entity_command.h`

```cpp
#pragma once

#include "command.h"
#include "editor_context.h"
#include "editor_selection.h"

#include "log/log.h"
#include "scene/entity.h"

#include <string>
#include <string_view>

namespace buddd::editor {

/// Command that renames a single entity.
class RenameEntityCommand final : public Command {
public:
    RenameEntityCommand(buddd::engine::EntityId entity_id,
                        std::string old_name,
                        std::string new_name)
        : entity_id_(entity_id)
        , old_name_(std::move(old_name))
        , new_name_(std::move(new_name))
    {}

    auto execute(EditorContext const& ctx) -> void override {
        // Save pre-execution selection for undo
        pre_execution_selection_ = ctx.editor.selection().snapshot();

        auto& world = ctx.editor.world();
        // Find entity and set name
        auto find_and_rename = [&](auto& self, buddd::engine::Entity e) -> bool {
            if (e.id() == entity_id_) {
                e.set_name(new_name_);
                return true;
            }
            for (size_t i = 0; i < e.child_count(); ++i) {
                if (self(self, e.get_child(i))) return true;
            }
            return false;
        };
        bool found = false;
        for (size_t i = 0; i < world.root_entity_count(); ++i) {
            auto root = world.get_root_entity(i);
            if (root.id() == entity_id_) {
                root.set_name(new_name_);
                found = true;
                break;
            }
            if (find_and_rename(find_and_rename, root)) {
                found = true;
                break;
            }
        }
        if (!found) {
            BUDDD_LOG_DEBUG("RenameEntity::execute: entity {} not found", entity_id_.index);
        }

        BUDDD_LOG_DEBUG("RenameEntity: {} -> {}", old_name_, new_name_);
    }

    auto undo(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();
        // Revert to old name
        auto find_and_rename = [&](auto& self, buddd::engine::Entity e) -> bool {
            if (e.id() == entity_id_) {
                e.set_name(old_name_);
                return true;
            }
            for (size_t i = 0; i < e.child_count(); ++i) {
                if (self(self, e.get_child(i))) return true;
            }
            return false;
        };
        for (size_t i = 0; i < world.root_entity_count(); ++i) {
            auto root = world.get_root_entity(i);
            if (root.id() == entity_id_) {
                root.set_name(old_name_);
                break;
            }
            if (find_and_rename(find_and_rename, root)) break;
        }

        // Restore selection
        ctx.editor.selection().restore(pre_execution_selection_);
    }

    [[nodiscard]] auto name() const -> std::string_view override {
        return "Rename Entity";
    }

private:
    buddd::engine::EntityId entity_id_;
    std::string old_name_;
    std::string new_name_;
    Selection pre_execution_selection_;
};

} // namespace buddd::editor
```

### Step 11: Update `src/editor/panels/scene_panel.h` — context menu, keyboard, rename, confirmation dialog

Add includes:
```cpp
#include "command_stack.h"
#include "commands/create_entity_command.h"
#include "commands/delete_entity_command.h"
#include "commands/rename_entity_command.h"
```

Add private state members to `ScenePanel` class:
```cpp
private:
    // ── Inline rename state ──
    std::optional<buddd::engine::EntityId> renaming_entity_;
    std::string rename_buffer_;

        // ── Delete confirmation state ──
    bool show_delete_confirmation_ = false;
    std::vector<buddd::engine::EntityId> pending_deletion_ids_;
    size_t pending_deletion_with_children_ = 0;
    std::string pending_deletion_first_name_;
```

**Entity tree context menu** (add inside `render_entity` lambda, after `TreeNodeEx` and click handling, before the `if (expanded)` block):

```cpp
// ── Context menu on entity ──
if (ImGui::BeginPopupContextItem()) {
    bool single_selected = (ctx.editor.selection().size() == 1);
    bool multi_selected = (ctx.editor.selection().size() > 1);
    bool any_selected = !ctx.editor.selection().empty();

    if (ImGui::MenuItem("Create Empty")) {
        execute_create_entity(ctx);
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Delete", nullptr, false, any_selected)) {
        execute_delete_entity(ctx);
    }

    if (ImGui::MenuItem("Rename", nullptr, false, single_selected)) {
        start_rename(ctx, ctx.editor.selection().first().value());
    }

    ImGui::EndPopup();
}
```

**Empty-area context menu** (add at the end of `draw_ui()`, after the empty-area click block):

```cpp
// ── Context menu on empty area ──
if (ImGui::BeginPopupContextWindow()) {
    if (ImGui::MenuItem("Create Empty")) {
        execute_create_entity(ctx);
    }
    ImGui::EndPopup();
}
```

**Delete key handling** (add inside `draw_ui()`, before the empty-area click section, gated by window focus and not in rename):

```cpp
// ── Delete key (gated by focus, disabled during rename) ──
if (ImGui::IsWindowFocused() && !renaming_entity_.has_value()
    && ImGui::IsKeyPressed(ImGuiKey_Delete) && !ctx.editor.selection().empty()) {
    execute_delete_entity(ctx);
}
```

**F2 key handling** (add inside `draw_ui()`, gated by focus and exactly one selected):

```cpp
// ── F2 key (gated by focus, exactly one selected) ──
if (ImGui::IsWindowFocused() && ctx.editor.selection().size() == 1) {
    if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
        if (renaming_entity_.has_value()) {
            // Confirm pending rename first, then start new
            confirm_rename(ctx);
        }
        start_rename(ctx, ctx.editor.selection().first().value());
    }
}
```

**Inline rename rendering** — modify the entity name rendering section inside `render_entity`:

Before the `bool expanded = ImGui::TreeNodeEx(name.c_str(), flags);` line, add:

```cpp
// ── Inline rename ──
bool is_renaming = (renaming_entity_.has_value() && *renaming_entity_ == entity.id());

if (is_renaming) {
    // Use InputText instead of TreeNodeEx label
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::SetKeyboardFocusHere();
    ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_EnterReturnsTrue
                                    | ImGuiInputTextFlags_AutoSelectAll;
    bool confirmed = ImGui::InputText("##rename", &rename_buffer_, input_flags);

    // Detect Enter/Escape/focus-loss
    if (confirmed) {
        confirm_rename(ctx);
    } else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        cancel_rename();
    } else if (ImGui::IsItemDeactivatedAfterEdit()) {
        // Focus loss while editing: confirm (same as Enter)
        confirm_rename(ctx);
    }

    // Skip TreeNodeEx for this entity when renaming
    // We still render the tree node structure for ImGui consistency
    // Use an invisible placeholder or just skip
    // Actually, for the rename, render the TreeNodeEx with an empty label
    // and let the InputText overlay it
    bool expanded = ImGui::TreeNodeEx("##rename_placeholder", flags | ImGuiTreeNodeFlags_NoTreePushOnOpen);
    // Don't TreePop — we handle expand for children separately
    if (expanded) {
        // Render children
        for (size_t i = 0; i < entity.child_count(); ++i) {
            self(self, entity.get_child(i));
        }
        ImGui::TreePop();
    }
} else {
    bool expanded = ImGui::TreeNodeEx(name.c_str(), flags);
    // ... rest of existing click handling and expanded block ...
}
```

**Delete confirmation dialog** (add as a new method `draw_delete_confirmation_modal()` called at the end of `draw_ui()`):

```cpp
auto draw_delete_confirmation_modal() -> void {
    if (!show_delete_confirmation_) return;

    ImGui::OpenPopup("Confirm Delete");
    if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (pending_deletion_ids_.size() == 1) {
            ImGui::Text("Delete %s and its %zu children?",
                pending_deletion_first_name_.c_str(),
                pending_deletion_with_children_);
        } else {
            ImGui::Text("Delete %zu entities? (%zu have children that will also be deleted.)",
                pending_deletion_ids_.size(),
                pending_deletion_with_children_);
        }

        if (ImGui::Button("Delete")) {
            // Execute the delete command with pending_deletion_ids_
            // This requires EditorContext — pass it through.
            // Actually, the modal is rendered inside draw_ui which has ctx.
            // We'll handle this in the main draw_ui flow.
            show_delete_confirmation_ = false;
            // The actual command execution happens in draw_ui after the modal
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            show_delete_confirmation_ = false;
            pending_deletion_ids_.clear();
            pending_deletion_with_children_ = 0;
        }
        ImGui::EndPopup();
    }
}
```

**Helper methods** to add to ScenePanel:

```cpp
auto execute_create_entity(EditorContext const& ctx) -> void {
    auto cmd = std::make_unique<CreateEntityCommand>();
    ctx.editor.command_stack().execute(std::move(cmd), ctx);
}

auto execute_delete_entity(EditorContext const& ctx) -> void {
    auto& selection = ctx.editor.selection();
    auto& world = ctx.editor.world();

    // Collect selected entities and check if any have children
    std::vector<buddd::engine::EntityId> ids;
    size_t with_children = 0;

    // We iterate selection and look up entities
    for (auto id : selection.current()) {
        ids.push_back(id);
        // Find entity and check child_count
        auto find_entity = [&](auto& self, buddd::engine::Entity e) -> bool {
            if (e.id() == id) {
                if (e.child_count() > 0) ++with_children;
                return true;
            }
            for (size_t i = 0; i < e.child_count(); ++i) {
                if (self(self, e.get_child(i))) return true;
            }
            return false;
        };
        for (size_t i = 0; i < world.root_entity_count(); ++i) {
            auto root = world.get_root_entity(i);
            if (root.id() == id) {
                if (root.child_count() > 0) ++with_children;
                break;
            }
            if (find_entity(find_entity, root)) break;
        }
    }

    if (with_children > 0) {
        // Show confirmation dialog
        pending_deletion_ids_ = std::move(ids);
        pending_deletion_with_children_ = with_children;
        // Find first entity name for single-entity dialog
        if (pending_deletion_ids_.size() == 1) {
            auto& w = ctx.editor.world();
            auto find_name = [&](auto& self, buddd::engine::Entity e) -> bool {
                if (e.id() == pending_deletion_ids_[0]) {
                    pending_deletion_first_name_ = e.name();
                    return true;
                }
                for (size_t i = 0; i < e.child_count(); ++i) {
                    if (self(self, e.get_child(i))) return true;
                }
                return false;
            };
            for (size_t i = 0; i < w.root_entity_count(); ++i) {
                auto root = w.get_root_entity(i);
                if (root.id() == pending_deletion_ids_[0]) {
                    pending_deletion_first_name_ = root.name();
                    break;
                }
                if (find_name(find_name, root)) break;
            }
        }
        show_delete_confirmation_ = true;
    } else {
        // Execute immediately (no children involved)
        auto cmd = std::make_unique<DeleteEntityCommand>(std::move(ids));
        ctx.editor.command_stack().execute(std::move(cmd), ctx);
    }
}

auto start_rename(EditorContext const& ctx, buddd::engine::EntityId id) -> void {
    renaming_entity_ = id;
    // Pre-fill buffer with current name
    // Find entity and get name
    auto& world = ctx.editor.world();
    auto find_entity = [&](auto& self, buddd::engine::Entity e) -> bool {
        if (e.id() == id) {
            rename_buffer_ = e.name();
            return true;
        }
        for (size_t i = 0; i < e.child_count(); ++i) {
            if (self(self, e.get_child(i))) return true;
        }
        return false;
    };
    for (size_t i = 0; i < world.root_entity_count(); ++i) {
        auto root = world.get_root_entity(i);
        if (root.id() == id) {
            rename_buffer_ = root.name();
            break;
        }
        if (find_entity(find_entity, root)) break;
    }
}

auto confirm_rename(EditorContext const& ctx) -> void {
    if (!renaming_entity_.has_value()) return;
    auto id = *renaming_entity_;
    renaming_entity_.reset();

    // Find current name
    std::string current_name;
    auto& world = ctx.editor.world();
    auto find_entity = [&](auto& self, buddd::engine::Entity e) -> bool {
        if (e.id() == id) {
            current_name = e.name();
            return true;
        }
        for (size_t i = 0; i < e.child_count(); ++i) {
            if (self(self, e.get_child(i))) return true;
        }
        return false;
    };
    for (size_t i = 0; i < world.root_entity_count(); ++i) {
        auto root = world.get_root_entity(i);
        if (root.id() == id) {
            current_name = root.name();
            break;
        }
        if (find_entity(find_entity, root)) break;
    }

    // Validate: empty name or same name → no-op
    if (rename_buffer_.empty() || rename_buffer_ == current_name) {
        rename_buffer_.clear();
        return;
    }

    // Execute rename command
    auto cmd = std::make_unique<RenameEntityCommand>(id, std::move(current_name), rename_buffer_);
    ctx.editor.command_stack().execute(std::move(cmd), ctx);
    rename_buffer_.clear();
}

auto cancel_rename() -> void {
    renaming_entity_.reset();
    rename_buffer_.clear();
}
```

**Integration of confirmation dialog**: The confirmation dialog needs access to `EditorContext`. In `draw_ui()`, call a helper that renders the modal and, on confirmation, executes the delete command:

```cpp
// At end of draw_ui():
draw_delete_confirmation_modal(ctx);
```

And the modal method receives `EditorContext const&`:

```cpp
auto draw_delete_confirmation_modal(EditorContext const& ctx) -> void {
    if (!show_delete_confirmation_) return;

    // Cache first entity name for the single-entity dialog message
    ImGui::OpenPopup("Confirm Delete");
    if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (pending_deletion_ids_.size() == 1) {
            ImGui::Text("Delete %s and its %zu children?",
                pending_deletion_first_name_.c_str(),
                pending_deletion_with_children_);
        } else {
            ImGui::Text("Delete %zu entities? (%zu have children that will also be deleted.)",
                pending_deletion_ids_.size(),
                pending_deletion_with_children_);
        }

        bool deleted = false;
        if (ImGui::Button("Delete")) {
            auto cmd = std::make_unique<DeleteEntityCommand>(std::move(pending_deletion_ids_));
            ctx.editor.command_stack().execute(std::move(cmd), ctx);
            deleted = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel") || deleted) {
            show_delete_confirmation_ = false;
            pending_deletion_ids_.clear();
            pending_deletion_with_children_ = 0;
            if (deleted) {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            }
        }
        ImGui::EndPopup();
    }
}
```

### Step 12: Update `tests/editor/editor_tests.cpp` — ToggleCommand signature

Update the `ToggleCommand` class:
```cpp
class ToggleCommand final : public ed::Command {
public:
    explicit ToggleCommand(bool* target, std::string_view name)
        : target_(target), name_(name) {}

    auto execute(ed::EditorContext const& /*ctx*/) -> void override { *target_ = true;  }
    auto undo(ed::EditorContext const& /*ctx*/)    -> void override { *target_ = false; }
    [[nodiscard]] auto name() const -> std::string_view override { return name_; }
private:
    bool* target_;
    std::string_view name_;
};
```

Also include `"editor_context.h"` in the test file.

Update all `CommandStack` test calls to construct an `EditorContext` and pass it. Since tests don't have an `Editor` instance, create a minimal `HeadlessTestContext` or use a dummy approach:

The simplest approach:
```cpp
// In tests that don't need a real Editor, create a minimal EditorContext
// by creating a no-op Editor subclass or by passing nullptr references.
// Better: use HeadlessTestContext and an Editor instance.
```

But many `CommandStack` tests are pure unit tests that don't use `Editor` at all. For these, we need a way to create `EditorContext` without a real `Editor`. One approach: make the `CommandStack` tests pass a null/empty `EngineContext` and create a minimal Editor, then construct `EditorContext` from it.

Actually, looking at the test pattern, the cleanest approach is:
1. For `CommandStack` tests (which test execute/undo/redo behavior), create a minimal `Editor` instance and use a dummy `HeadlessTestContext` to construct an `EditorContext`. The `Editor` constructor creates an empty World and doesn't require setup for basic operations.
2. Update `CommandStack::execute()` calls to pass `ctx`.

```cpp
// Helper for command stack tests that need EditorContext
struct CommandTestContext {
    buddd::editor::Editor editor;
    // We need an EngineContext — create a minimal one
    // But EngineContext requires EngineService, etc.
    // Alternative: Create EditorContext from editor + null engine reference?
    // No, EditorContext requires a valid EngineContext reference.
    
    // Best approach: Keep tests simple by making tests construct
    // a minimal engine context for the command stack tests.
};
```

Actually, this is getting complex. The cleanest solution for the test update is to make `CommandStack` overloads that don't require `EditorContext` in addition to the new ones — but the spec says the signatures change, not overloads.

For the test file, the approach should be:
- Create a helper struct that creates a minimal engine context
- Or modify the test to create an Editor, get its world, and construct EditorContext

Looking at how `HeadlessTestContext` works in `editor_tests.cpp`, I can create a similar helper:

```cpp
// Minimal EditorContext for command tests
struct CmdTestCtx {
    buddd::editor::Editor editor;
    // We need EngineContext — create minimal
    // Actually, we can construct EditorContext{editor, dummy_engine_ctx}
    // where dummy_engine_ctx is a minimal EngineContext
};
```

This is a significant test infrastructure change. Let me specify it clearly in the contract.

Actually, the simplest: modify the test to use `HeadlessTestContext` (already defined in the same file) and create an `Editor`, then construct `EditorContext{editor, *htc.ctx}`.

I'll specify this clearly in the contract's implementation behavior.

### Step 13: Create `tests/editor/entity_operations_tests.cpp`

Follow existing test patterns. Use `HeadlessTestContext` from `editor_tests.cpp` (or re-create the same helper). Tests tagged with `[editor][commands]` and specific sub-tags.

## Required tests

### Unit tests in `tests/editor/entity_operations_tests.cpp`

| Test | AC(s) | What it verifies |
|---|---|---|
| `CreateEntity: creates root entity with no anchor` | AC-10 | Clear selection, execute CreateEntityCommand, verify `world.root_entity_count()` incremented. |
| `CreateEntity: creates child of anchor` | AC-09 | Set anchor to entity A, execute create, verify A's child count incremented. |
| `CreateEntity: undo destroys entity` | AC-26 | Execute create, undo, verify entity count returns to original. |
| `CreateEntity: redo recreates entity` | AC-29 | Execute → undo → redo, verify entity count is back to post-create. |
| `CreateEntity: undo restores selection` | AC-28 | Select A, snapshot, execute create, undo, verify selection matches snapshot. |
| `CreateEntity: selection unchanged after create` | AC-09, story 1 | Execute create with A selected, verify A is still in selection. |
| `DeleteEntity: single leaf, no confirmation, entity destroyed` | AC-11 | Delete leaf entity, verify `Entity::destroy()` called (pending_destroy). |
| `DeleteEntity: selection cleared after delete` | AC-16, AC-31 | Execute delete, verify `selection().empty()`. |
| `DeleteEntity: undo restores entity with name and hierarchy` | AC-27 | Delete entity with child, undo, verify both entities exist with correct names and parent-child relationship. |
| `DeleteEntity: undo restores previous selection` | AC-28 | Select A, delete B, undo, verify selection contains A. |
| `DeleteEntity: redo destroys again` | AC-29 | Execute → undo → redo, verify entities destroyed again. |
| `RenameEntity: execute changes name` | AC-21 | Execute rename "A"→"B", verify entity name is "B". |
| `RenameEntity: undo restores old name` | AC-25 | Execute rename "A"→"B", undo, verify name is "A". |
| `RenameEntity: redo re-applies new name` | AC-29 | Execute → undo → redo, verify name is "B". |
| `RenameEntity: undo restores selection` | AC-28 | Select A, rename, undo, verify selection contains A. |
| `CommandStack: EditorContext forwarded to execute/undo` | AC-01, AC-02, AC-03 | Create a test command that captures `EditorContext const&` in execute/undo, verify it is the same context passed to CommandStack. |

**Testing note**: Tests requiring `EditorContext` (all command tests) need a minimally constructed `EditorContext`. Use test helper `CommandTestContext` or `HeadlessTestContext`:

```cpp
struct CmdTestHelper {
    std::unique_ptr<buddd::engine::EngineService> engine;
    std::unique_ptr<buddd::engine::World> engine_world;
    std::unique_ptr<buddd::engine::RenderSystem> render_system;
    std::unique_ptr<buddd::engine::EngineContext> engine_ctx;
    buddd::editor::Editor editor;
    buddd::editor::EditorContext editor_ctx;

    CmdTestHelper() {
        auto eng = buddd::engine::EngineService::create(
            buddd::engine::Backend::Headless,
            buddd::engine::WindowConfig{.title = "CmdTest", .width = 128, .height = 128});
        // Handle potential failure
        if (eng.has_value()) {
            engine = std::move(*eng);
            engine_world = std::make_unique<buddd::engine::World>();
            render_system = std::make_unique<buddd::engine::RenderSystem>(
                engine->device(), *engine_world);
            engine_ctx = std::make_unique<buddd::engine::EngineContext>(
                buddd::engine::EngineContext{
                    *engine, engine->window(), engine->device(), *engine_world,
                    *render_system, 0.016f, 0});
            editor_ctx = buddd::editor::EditorContext{editor, *engine_ctx};
        }
    }

    bool valid() const { return engine_ctx != nullptr; }
};
```

### E2E / Integration verification

| Method | Description |
|---|---|
| **Build verification (CI)** | `cmake --build --preset debug` succeeds with zero new warnings from `src/editor/` and `tests/`. |
| **Test suite pass (CI)** | `buddd_tests` — all existing tests pass. No new test failures. |
| **Manual smoke test (display)** | Run `buddd edit` with a scene. Verify: right-click entity → context menu with three items; "Create Empty" creates child entity under anchor; right-click empty area → "Create Empty" creates root; Delete key deletes selection; F2 starts inline rename; Enter confirms rename; Escape cancels; hierarchical delete shows confirmation; Ctrl+Z undoes; Ctrl+Y redoes. |
| **Code review** | Verify all signature changes are consistent. Verify undo/redo cycles correctly restore entity state and selection. |

## Edge cases

| Case | Expected behavior | Verified in |
|---|---|---|
| **Create with multi-select (no anchor)** | New entity created as root. | Unit test + manual |
| **Delete-while-rename** | InputText consumes Delete key. User must confirm/cancel rename first, then Delete works. | Manual |
| **F2 while rename active on another entity** | Pending rename confirmed first (Enter-equivalent), then F2 triggers new rename. | Manual |
| **F2 with no/multi selection** | No-op — gated by `selection.size() == 1`. | Manual |
| **Rename empty name (Enter)** | Rejected — name reverts, no command pushed. | Unit test |
| **Rename same name** | No-op — `rename_buffer_ == current_name` check skips command. | Unit test |
| **Delete all entities in World** | All destroyed, selection cleared. Confirmation shown if any have children. | Manual |
| **Delete key while confirmation dialog open** | Modal captures keyboard — Delete key not processed by ScenePanel. | Manual |
| **F2 on collapsed ancestor entity** | Entity not rendered; F2 is no-op (entity not visible in tree). | Manual |
| **Undo after scene load** | `CommandStack::clear()` called by MenuBar on new/open scene. No stale commands. | Inspect code |
| **Redo with empty stack** | `redo()` returns false. No-op. | Unit test |
| **Undo with empty stack** | `undo()` returns false. No-op. | Unit test |
| **Context menu on entity does not change selection** | Right-click alone does not select; `BeginPopupContextItem` does not change selection. | Manual |
| **Selection snapshot on redo overwrites saved selection** | On `execute()` via redo, the command re-snapshots current selection. This is correct — the new snapshot reflects the state before redo, which is the correct state to restore on the next undo. | Unit test (AC-29) |
| **Delete confirmation dismissed by clicking outside** | ImGui modal: click-outside closes modal (equivalent to Cancel). No entities destroyed. | Manual |

## Security impact

None. Entity operations are in-memory only — no file I/O, network access, or sensitive data. No new input parsing. Entity names are plain strings; no sanitisation needed.

## Data and migration impact

None. No schema changes, data migrations, seed data, or data loss risks. `flush_destroyed()` lifecycle ensures destroyed entities are removed from the World before next frame's panel rendering.

## API compatibility impact

- **`Command` base class**: Breaking change — `execute()` and `undo()` add `EditorContext const&` parameter. All existing `Command` subclasses must update.
- **`CommandStack`**: Breaking change — `execute()`, `undo()`, `redo()` add `EditorContext const&` parameter. All callers must update.
- **`QuitCommand`**: Constructor no longer takes `EngineContext const&`. Any code constructing `QuitCommand` with engine context must remove the argument.
- **New public API**: `Editor::command_stack()` returns `CommandStack&`. This is a pure addition.
- **New files**: Three command headers in `src/editor/commands/`. Internal to editor library, not part of any SDK.
- **No changes to engine public API**.

## Documentation impact

| Document | Reason for update |
|---|---|
| `docs/wiki/editor/editor-panels.md` | Add entity operations section: Create Empty, Delete, Rename. Document context menu, keyboard shortcuts, confirmation dialogs. Note Command signature change. Document new command files. |
| `docs/wiki/editor/cross-panel-communication.md` | Update to reflect that Commands use `EditorContext` to access World and selection. Document selection snapshot/restore for entity operations. |

## ADR impact

No new ADR needed. The implementation follows existing patterns (ADR-027 direct member variables, ADR-011 `[[nodiscard]]`, ADR-026 ImGui). No existing ADR is deprecated or amended.

## Done criteria

- [ ] **DC-01**: `src/editor/command.h` — `execute()` and `undo()` signatures changed to `virtual auto execute(EditorContext const& ctx) -> void = 0;` and `virtual auto undo(EditorContext const& ctx) -> void = 0;`. `EditorContext` forward-declared.

- [ ] **DC-02**: `src/editor/command_stack.h` — `execute(unique_ptr<Command>, EditorContext const&)`, `undo(EditorContext const&) -> bool`, `redo(EditorContext const&) -> bool`. `EditorContext` forward-declared.

- [ ] **DC-03**: `src/editor/command_stack.cpp` — all three methods forward `ctx` to command methods. `#include "log/log.h"` added. Debug logging: `BUDDD_LOG_DEBUG("[Command] {}: executed/undone/redone", command->name())` in each method.

- [ ] **DC-04**: `src/editor/commands/quit_command.h` — `execute(EditorContext const& ctx)` uses `ctx.engine.request_exit()`. No stored `EngineContext const* ctx_`. Default constructor `QuitCommand() = default`. Includes `"editor_context.h"`.

- [ ] **DC-05**: `src/editor/commands/create_entity_command.h` created with `CreateEntityCommand` class. Constructor takes no args. `execute()` creates entity as child of `selection.anchor()` (or root), saves snapshot. `undo()` destroys entity, restores snapshot. `name()` returns `"Create Entity"`.

- [ ] **DC-06**: `src/editor/commands/delete_entity_command.h` created with `DeleteEntityCommand` class. Constructor takes `std::vector<EntityId>`. `execute()` saves entity state (name, parent, children) for all descendants, destroys them, clears selection. `undo()` recreates all entities with original names/hierarchy, updates `entity_ids_` with new EntityIds from the old→new mapping, restores snapshot. `name()` returns `"Delete Entity"`.

- [ ] **DC-07**: `src/editor/commands/rename_entity_command.h` created with `RenameEntityCommand` class. Constructor takes `EntityId, string old_name, string new_name`. `execute()` sets new name. `undo()` sets old name. `name()` returns `"Rename Entity"`. Both save/restore selection snapshot.

- [ ] **DC-08**: `src/editor/editor.h` — public `[[nodiscard]] auto command_stack() -> CommandStack&;` accessor added.

- [ ] **DC-09**: `src/editor/editor.cpp`:
  - `Editor::command_stack()` returns `command_stack_`.
  - `Editor::update()` calls `world().flush_destroyed()` after panel update loop.
  - Ctrl+Z / Ctrl+Shift+Z / Ctrl+Y shortcut callbacks construct `EditorContext{*this, ectx}` and pass to `command_stack_.undo(editor_ctx)` / `.redo(editor_ctx)`.

- [ ] **DC-10**: `src/editor/panels/menu_bar.h` — `command_stack_.undo()` → `command_stack_.undo(ctx)`. `command_stack_.redo()` → `command_stack_.redo(ctx)`.

- [ ] **DC-11**: `src/editor/panels/scene_panel.h`:
  - Includes `"command_stack.h"`, `"commands/create_entity_command.h"`, `"commands/delete_entity_command.h"`, `"commands/rename_entity_command.h"`.
  - State members: `renaming_entity_` (optional EntityId), `rename_buffer_` (string), `show_delete_confirmation_` (bool), `pending_deletion_ids_` (vector), `pending_deletion_with_children_` (size_t).
  - Context menu on entity (right-click): "Create Empty", separator, "Delete" (disabled if empty selection), "Rename" (disabled if not single selection).
  - Context menu on empty area (right-click): "Create Empty" only.
  - Delete key handler: gated by `IsWindowFocused() && !renaming_entity_ && IsKeyPressed(ImGuiKey_Delete) && !selection.empty()`.
  - F2 key handler: gated by `IsWindowFocused() && selection.size() == 1`.
  - Inline rename: `InputText` with `EnterReturnsTrue`, auto-focused via `SetKeyboardFocusHere()`. Enter confirms (pushes `RenameEntityCommand`), Escape cancels, focus loss confirms.
  - Confirmation dialog: `ImGui::BeginPopupModal("Confirm Delete")` shown when `show_delete_confirmation_` is true. "Delete" button executes `DeleteEntityCommand`. "Cancel" button dismisses.
  - `start_rename()`, `confirm_rename()`, `cancel_rename()` helper methods.

- [ ] **DC-12**: `tests/editor/editor_tests.cpp` — `ToggleCommand` updated to accept `EditorContext const&`. All `CommandStack` test calls updated to pass `EditorContext` (using a helper or `HeadlessTestContext`).

- [ ] **DC-13**: `tests/editor/entity_operations_tests.cpp` created with tests tagged `[editor][commands]` covering:
  - CreateEntity: root creation, child creation, undo, redo, selection restore.
  - DeleteEntity: single leaf delete, selection cleared, undo restores name/hierarchy, undo restores selection, redo.
  - RenameEntity: execute name change, undo name revert, redo, selection restore.
  - CommandStack context forwarding.

- [ ] **DC-14**: `cmake --build --preset debug` succeeds with **zero new warnings** from `src/editor/` and `tests/`.

- [ ] **DC-15**: All existing tests pass: `buddd_tests` run with zero failures.

- [ ] **DC-16**: No changes to files under `src/engine/`, `src/editor/editor_context.h`, `src/editor/editor_panel.h`, `src/editor/editor_menu.h`, `src/editor/editor_selection.h`, `src/editor/shortcut_registry.h`, or any `CMakeLists.txt`.
