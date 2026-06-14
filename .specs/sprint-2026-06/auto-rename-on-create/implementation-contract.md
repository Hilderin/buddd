# IMPL-2026-06 — Scene Panel — Auto-Rename on Create

## Source spec

`.specs/sprint-2026-06/auto-rename-on-create/spec.md`

## Goal

Modify the Scene Panel's "Create Empty" flow so that entity creation automatically selects the new entity, enters inline rename mode, and records the name as part of the `CreateEntityCommand` (single undo step). Pressing Escape during post-creation auto-rename discards the entity entirely. The existing F2/context-menu rename flow (separate `RenameEntityCommand`) is unchanged.

## Non-goals

- No changes to F2 or context menu "Rename" behavior — they still push a separate `RenameEntityCommand`.
- No changes to Delete key or context menu "Delete" flow.
- No changes to `CommandStack` interface — existing `undo()`/`redo()`/`execute()` signatures are unchanged.
- No changes to `Command` base class interface.
- No changes to entity name validation (empty/same-name rejection unchanged).
- No changes to the Properties Panel rename field.
- No changes to selection behavior beyond auto-select on create.
- No changes to the `Entity`, `World`, or engine APIs.
- No new dependencies.

## Relevant ADRs

- **ADR-027** (Editor Architecture): `CreateEntityCommand` lives in `src/editor/commands/` — part of the `buddd_editor` static library. The `Editor::command_stack()` accessor is used for execution.
- **ADR-029** (Editor UX Decisions, Decision 7): Entity creation as child of selected (or root if no anchor) — unchanged by this feature. The auto-rename flow only affects the post-creation UX, not the parent placement logic.
- **ADR-026** (Dear ImGui Integration): ImGui InputText with `EnterReturnsTrue` and `IsItemDeactivatedAfterEdit()` are the standard patterns for rename; the auto-rename uses the same mechanisms.

## Files to inspect

- `src/editor/panels/scene_panel.h` — current ScenePanel state members and method signatures
- `src/editor/panels/scene_panel.cpp` — current implementation of `draw_ui()`, `execute_create_entity()`, `confirm_rename()`, `cancel_rename()`, `start_rename()`, the F2 handler, the context menu handler, and the Delete key handler
- `src/editor/commands/create_entity_command.h` — current `CreateEntityCommand` (header-only, all inline)
- `src/editor/command_stack.h` — `CommandStack` API for `execute()`, `undo()`, `redo()`
- `src/editor/editor.h` — `Editor::command_stack()` accessor used by ScenePanel
- `src/editor/editor_selection.h` — `EditorSelection::select(id, SelectionModifier::Replace)` used for auto-select
- `tests/editor/entity_operations_tests.cpp` — existing test patterns for fixture `EntityTestCtx` and command execution/verification

## Files allowed to change

- `src/editor/commands/create_entity_command.h` — add `created_entity_id()` getter, `set_post_creation_name()` setter, `post_creation_name_` member, modify `execute()`
- `src/editor/panels/scene_panel.h` — add `#include "commands/create_entity_command.h"`, add private members `pending_create_command_`, `auto_rename_entity_id_`, `pending_undo_creation_`
- `src/editor/panels/scene_panel.cpp` — modify `draw_ui()`, `execute_create_entity()`, `confirm_rename()`, `cancel_rename()`, and the context menu "Create Empty" handler
- `tests/editor/entity_operations_tests.cpp` — add new test cases for `CreateEntityCommand` with `post_creation_name_`, auto-selection, and undo/redo consistency

## Files forbidden to change

- `src/editor/command_stack.h`
- `src/editor/editor.h`
- `src/editor/editor_selection.h`
- `src/editor/panels/properties_panel.h` / `properties_panel.cpp`
- `src/editor/commands/rename_entity_command.h`
- `src/editor/commands/delete_entity_command.h`
- `src/editor/command.h`
- `src/engine/` (any file)
- Any CMakeLists.txt
- `docs/` (wiki updates are handled by wiki-agent in a separate step)

## Existing conventions to follow

1. **Namespace**: All editor types are in `namespace buddd::editor`. Engine types use `buddd::engine::`.
2. **Logging**: Use `BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel", ...)` for debug logs (see existing pattern in `scene_panel.cpp` lines 102, 129, 188, 315).
3. **Command pattern**: Commands are header-only (see `create_entity_command.h`). All methods defined inline in the class body. Use `std::make_unique<Command>()` for construction and `command_stack().execute(std::move(cmd), ctx)` for execution.
4. **Naming**: Members use `snake_case_` trailing underscore (e.g., `renaming_entity_`, `rename_buffer_`). Local variables use `snake_case`.
5. **No trailing return types with `->` when not required**: Use `auto ... -> void` syntax for method declarations, matching existing code style.
6. **Entity lookup pattern**: The codebase traverses the entity tree recursively using a generic lambda `find_entity` (see `create_entity_command.h` lines 50-69, `scene_panel.cpp` lines 346-356) — use the same pattern when needed.
7. **Test fixture pattern**: `EntityTestCtx` struct in `tests/editor/entity_operations_tests.cpp` provides `world()`, `selection()`, `command_stack()`, `editor_ctx`, `add_root_entity()`, `add_child_entity()`. Use this same fixture for new tests.
8. **ImGui patterns**:
   - `ImGui::InputText("##rename", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)` for inline rename.
   - `ImGui::IsKeyPressed(ImGuiKey_Escape)` for Escape detection.
   - `ImGui::IsItemDeactivatedAfterEdit()` for focus-loss detection.
   - `ImGui::SetKeyboardFocusHere()` before InputText for auto-focus.
   - `ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x)` for full-width input.
   - `ImGui::IsWindowFocused()` for keyboard shortcut gating.
9. **Namespace alias in tests**: `namespace ed = buddd::editor;` and `namespace be = buddd::engine;` are used in the test file. New tests should follow the same convention.
10. **Comment style**: Section comments use `// ── Title ──` pattern with em-dash characters (see `scene_panel.cpp` lines 25-26, 32-33, 248-249).

## Required implementation behavior

### 1. Modify `CreateEntityCommand` (`src/editor/commands/create_entity_command.h`)

#### 1a. Add public getter for created entity ID

After `name()` method (line 133), add:
```cpp
[[nodiscard]] auto created_entity_id() const -> buddd::engine::EntityId {
    return created_entity_id_;
}
```

#### 1b. Add public setter for post-creation name

Before `private:` section (line 135), add:
```cpp
auto set_post_creation_name(std::string name) -> void {
    post_creation_name_ = std::move(name);
}
```

#### 1c. Add private member `post_creation_name_`

In the `private:` section (line 135), add after existing members:
```cpp
std::optional<std::string> post_creation_name_;
```

#### 1d. Modify `execute()` to use `post_creation_name_`

After the line `created_entity_id_ = new_entity.id();` (line 83) and BEFORE the logging line (line 85), add:
```cpp
if (post_creation_name_.has_value() && new_entity.id() != buddd::engine::EntityId::none()) {
    new_entity.set_name(*post_creation_name_);
}
```

#### 1e. No change to `undo()`

The `undo()` method is unchanged. It destroys the entity regardless of whether a name was set. On `redo()`, `execute()` runs again and re-applies `post_creation_name_` if it has a value.

### 2. Modify `ScenePanel` header (`src/editor/panels/scene_panel.h`)

#### 2a. Add include

After the existing `#include "editor_context.h"` (line 4), add:
```cpp
#include "commands/create_entity_command.h"
```

#### 2b. Add private members for auto-rename state

In the `private:` section, after the `rename_buffer_` member (line 27), add:
```cpp
// ── Post-creation auto-rename state ──
CreateEntityCommand* pending_create_command_ = nullptr;
std::optional<buddd::engine::EntityId> auto_rename_entity_id_;
bool pending_undo_creation_ = false;
```

### 3. Modify `ScenePanel` implementation (`src/editor/panels/scene_panel.cpp`)

#### 3a. Modify `execute_create_entity()`

Replace the entire implementation (lines 252-256) with:

```cpp
auto ScenePanel::execute_create_entity(EditorContext const& ctx,
                                       std::optional<buddd::engine::EntityId> parent) -> void {
    auto cmd = std::make_unique<CreateEntityCommand>(parent);
    pending_create_command_ = cmd.get();
    ctx.editor.command_stack().execute(std::move(cmd), ctx);

    auto created_id = pending_create_command_->created_entity_id();
    if (created_id != buddd::engine::EntityId::none()) {
        // Auto-select the new entity (Replace)
        ctx.editor.selection().select(created_id, SelectionModifier::Replace);
        // Start inline rename
        start_rename(ctx, created_id);
        auto_rename_entity_id_ = created_id;
        BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel",
            "Auto-rename started for entity {} post-creation", created_id.index);
    } else {
        // Creation failed — clear pending command pointer
        pending_create_command_ = nullptr;
    }
}
```

#### 3b. Modify the "Create Empty" handler in `draw_ui()`

Replace the existing `if (ImGui::MenuItem("Create Empty"))` block (lines 201-207) with:

```cpp
if (ImGui::MenuItem("Create Empty")) {
    // If rename is active on another entity, confirm it first
    if (renaming_entity_.has_value()) {
        confirm_rename(ctx);
    }
    if (on_entity) {
        execute_create_entity(ctx, context_menu_entity_);
    } else {
        execute_create_entity(ctx);
    }
}
```

#### 3c. Modify `confirm_rename()`

Add an early-return branch at the top of `confirm_rename()` (after the `if (!renaming_entity_.has_value()) return;` guard on line 370). The full modified method:

```cpp
auto ScenePanel::confirm_rename(EditorContext const& ctx) -> void {
    if (!renaming_entity_.has_value()) return;
    auto id = *renaming_entity_;
    renaming_entity_.reset();

    // ── Auto-rename (post-creation) path ──
    if (auto_rename_entity_id_.has_value() && *auto_rename_entity_id_ == id) {
        std::string new_name(rename_buffer_);
        rename_buffer_[0] = '\0';
        // Find entity and set name
        auto& world = ctx.editor.world();
        auto find_entity = [&](auto& self, buddd::engine::Entity e) -> bool {
            if (e.id() == id) {
                if (!new_name.empty()) {
                    e.set_name(new_name);
                }
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
            if (root.id() == id) {
                if (!new_name.empty()) {
                    root.set_name(new_name);
                }
                found = true;
                break;
            }
            if (find_entity(find_entity, root)) {
                found = true;
                break;
            }
        }
        if (found && !new_name.empty() && pending_create_command_ != nullptr) {
            pending_create_command_->set_post_creation_name(std::move(new_name));
            BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel",
                "Auto-rename confirmed: entity {} named \"{}\"", id.index, pending_create_command_->created_entity_id().index);
        } else if (found && !new_name.empty()) {
            BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel",
                "Auto-rename confirmed: entity {} named \"{}\" (no pending cmd)", id.index, new_name);
        }
        pending_create_command_ = nullptr;
        auto_rename_entity_id_.reset();
        return;
    }

    // ── Regular rename path (unchanged from F-04) ──
    std::string current_name;
    auto& world = ctx.editor.world();
    // ... existing code: find entity, read current_name ...
    // (Copied verbatim from the existing implementation lines 374-403)
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

    std::string new_name(rename_buffer_);
    if (new_name.empty() || new_name == current_name) {
        rename_buffer_[0] = '\0';
        return;
    }

    auto cmd = std::make_unique<RenameEntityCommand>(id, std::move(current_name), std::move(new_name));
    ctx.editor.command_stack().execute(std::move(cmd), ctx);
    rename_buffer_[0] = '\0';
}
```

**Important**: The "regular rename path" code after the early-return is copied verbatim from the existing implementation (lines 374-403). The implementer must preserve this exact behavior.

#### 3d. Modify the Escape handler in `draw_ui()`

In the `draw_ui()` method, find the Escape key handler inside the inline rename block (lines 81-82: `else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) { cancel_rename(); }`).

Replace these two lines with:

```cpp
} else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    if (auto_rename_entity_id_.has_value() && renaming_entity_.has_value()
        && *auto_rename_entity_id_ == *renaming_entity_) {
        // Auto-rename Escape: cancel rename, defer undo of creation
        cancel_rename();
        pending_undo_creation_ = true;
        pending_create_command_ = nullptr;
        auto_rename_entity_id_.reset();
        BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel",
            "Auto-rename cancelled via Escape: entity {} discarded", renaming_entity_.has_value());
    } else {
        cancel_rename();
    }
}
```

**Note**: The `renaming_entity_` has just been reset by `cancel_rename()` at line 83, so the log after `cancel_rename()` must NOT use `renaming_entity_` (it's already cleared). Either capture the entity ID in a local variable before calling `cancel_rename()`, or use the already-captured `entity.id()` from the outer scope (the lambda captures `entity`). The implementer should capture the entity ID before cancel.

**Clarification**: The log message should reference the entity ID that was being renamed. Since the code is inside the render_entity lambda where `entity` is available, the implementer should capture the entity ID before calling `cancel_rename()`:
```cpp
auto cancelled_id = entity.id();
cancel_rename();
// ... then use cancelled_id in log ...
```

#### 3e. Add deferred undo at end of `draw_ui()`

At the very end of `draw_ui()` (after the F2 key handler block at line 244, before the closing brace at line 246), add:

```cpp
// ── Deferred undo for Escape-during-auto-rename ──
if (pending_undo_creation_) {
    pending_undo_creation_ = false;
    ctx.editor.command_stack().undo(ctx);
}
```

#### 3f. Clear auto-rename state when user starts a non-auto rename

In the F2 handler (lines 237-244), add before the existing logic:

Currently the F2 handler is:
```cpp
if (ImGui::IsWindowFocused() && ctx.editor.selection().size() == 1) {
    if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
        if (renaming_entity_.has_value()) {
            confirm_rename(ctx);
        }
        start_rename(ctx, ctx.editor.selection().first().value());
    }
}
```

Modify to:
```cpp
if (ImGui::IsWindowFocused() && ctx.editor.selection().size() == 1) {
    if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
        // Abandon any pending auto-rename
        if (auto_rename_entity_id_.has_value()) {
            auto_rename_entity_id_.reset();
            pending_create_command_ = nullptr;
        }
        if (renaming_entity_.has_value()) {
            confirm_rename(ctx);
        }
        start_rename(ctx, ctx.editor.selection().first().value());
    }
}
```

#### 3g. Clear auto-rename state on left-click on another entity

In the left-click handler (line 138-163), before the existing cancel_rename call, add abandonment of auto-rename:

Currently lines 139-142:
```cpp
// If rename is active on a different entity, cancel it
if (renaming_entity_.has_value() && *renaming_entity_ != entity.id()) {
    cancel_rename();
}
```

Modify to:
```cpp
// If rename is active on a different entity, cancel it
if (renaming_entity_.has_value() && *renaming_entity_ != entity.id()) {
    if (auto_rename_entity_id_.has_value() && *auto_rename_entity_id_ == *renaming_entity_) {
        // Abandon auto-rename: confirm it (stores name in pending command)
        confirm_rename(ctx);
    } else {
        cancel_rename();
    }
}
```

#### 3h. Clear auto-rename state on empty-area left-click

In the empty-area left-click handler (lines 223-228), add abandonment of auto-rename before the `cancel_rename()`:

Currently:
```cpp
if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
    if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift) {
        cancel_rename();
        ctx.editor.selection().clear();
    }
}
```

Modify to:
```cpp
if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
    if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift) {
        if (renaming_entity_.has_value()) {
            confirm_rename(ctx);
        }
        // If no rename was active (renaming_entity_ was empty), also clear auto-rename state
        if (auto_rename_entity_id_.has_value()) {
            auto_rename_entity_id_.reset();
            pending_create_command_ = nullptr;
        }
        ctx.editor.selection().clear();
    }
}
```

**Note**: The empty-area click handler should confirm the rename before clearing selection, because the auto-rename's InputText loses focus when the click happens, triggering `IsItemDeactivatedAfterEdit()` which calls `confirm_rename(ctx)`. However, the `confirm_rename` may have already been called by the focus-loss handler. To avoid double-confirmation, the implementer should check whether `renaming_entity_` still has a value before calling `confirm_rename` in the empty-area click handler. The existing code only calls `cancel_rename()` which doesn't push a command. The safest approach: call `confirm_rename(ctx)` only if `renaming_entity_.has_value()`, to ensure the auto-rename name is recorded if the user typed something.

### 4. Observability (debug logging)

Add the following debug log calls as specified in the spec:

| Signal | Location | Log call |
|---|---|---|
| **Auto-rename started** | In `execute_create_entity()`, after starting rename | `BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel", "Auto-rename started for entity {} post-creation", created_id.index)` |
| **Auto-rename confirmed with name** | In `confirm_rename()`, auto-rename branch, after `set_post_creation_name` | `BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel", "Auto-rename confirmed: entity {} named \"{}\"", id.index, name)` |
| **Auto-rename cancelled (Escape)** | In Escape handler, after setting `pending_undo_creation_` | `BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel", "Auto-rename cancelled via Escape: entity {} discarded", cancelled_id.index)` |

Existing log calls in `confirm_rename()` for the regular rename path are unchanged.

## Required tests

### Unit tests

All tests use the existing `EntityTestCtx` fixture from `tests/editor/entity_operations_tests.cpp`. New tests are added to the existing file.

| Test ID | Description | Verification | Traces to |
|---|---|---|---|
| **T-01** | `CreateEntityCommand::created_entity_id()` returns valid ID after `execute()` | Create root entity via `CreateEntityCommand`, call `execute()`, verify `cmd->created_entity_id() != EntityId::none()` | AC-001, AC-002 |
| **T-02** | `CreateEntityCommand` with `set_post_creation_name("Player")` creates entity with name "Player" | Set name via `set_post_creation_name("Player")`, execute, find entity in world, verify `entity.name() == "Player"` | AC-004, AC-007 |
| **T-03** | `CreateEntityCommand` with `set_post_creation_name("Player")`: undo destroys entity | Execute with name, undo, flush destroyed, verify entity count unchanged from pre-execution | AC-007 |
| **T-04** | `CreateEntityCommand` with `set_post_creation_name("Player")`: redo re-creates entity with name | Execute→undo→flush→redo, verify entity exists with name "Player" | AC-007 |
| **T-05** | `CreateEntityCommand` with empty `post_creation_name_` (std::nullopt) creates unnamed entity | Do NOT call `set_post_creation_name()`, execute, verify entity name is empty | AC-009 |
| **T-06** | `CreateEntityCommand` with `set_post_creation_name("")` (empty string): entity remains unnamed | Set empty string via `set_post_creation_name("")`, execute, verify entity name is empty | AC-009 |
| **T-07** | `CreateEntityCommand::created_entity_id()` returns `EntityId::none()` before `execute()` | Call `created_entity_id()` before `execute()`, verify returns `EntityId::none()` | Regression |
| **T-08** | Auto-select: after execute, selection contains created entity ID | Execute command, get `created_entity_id()`, verify `selection().contains(created_id)` | AC-001, AC-002 |

### E2E / Integration verification

| Method | Description | Traces to |
|---|---|---|
| **Manual smoke test** | Run `buddd edit`. Right-click empty area → "Create Empty" → type "Player" → Enter. Verify: entity selected, InputText appeared, entity named "Player" in tree, selection shows the new entity. | AC-001, AC-002, AC-003, AC-004 |
| **Manual Escape test** | Create entity → press Escape. Verify: entity disappears, selection restored to pre-create state. Check that Edit > Undo is disabled (or undoes previous action, not the discarded creation). | AC-005, AC-006 |
| **Manual undo test** | Create entity named "Test" → Ctrl+Z. Verify: entity destroyed, selection restored. Ctrl+Y → entity reappears with name "Test". | AC-007 |
| **Manual F2 compatibility** | Create entity (name "A") → confirm → F2 → rename to "B" → Ctrl+Z. Verify: name reverts to "A". Second Ctrl+Z → entity destroyed. | AC-008, AC-013 |
| **Manual empty name** | Create entity → press Enter immediately (no typing). Verify: entity stays unnamed "(unnamed)", InputText gone, entity NOT discarded. | AC-009 |
| **Manual focus loss** | Create entity → type "Test" → click another entity. Verify: "Test" confirmed, other entity selected. | AC-010 |
| **Manual rename conflict** | F2-rename "Cat" to "Dog" (don't press Enter) → right-click empty area → "Create Empty". Verify: "Cat" renamed to "Dog", new entity created with auto-rename. | AC-011 |
| **Manual double create** | Create entity → type "First" → right-click empty area → "Create Empty". Verify: "First" confirmed, second entity created with auto-rename. | AC-012 |
| **Clean build** | Run `cmake --build --preset debug`. Verify zero new warnings from `src/editor/` and `tests/`. | AC-015 |
| **Existing tests pass** | Run `buddd_tests` with tags `[editor][commands]`. All existing tests pass. | AC-014 |

## Edge cases

| Case | Expected behavior | Where handled |
|---|---|---|
| **Empty name on Enter during auto-rename** | Entity stays unnamed (rendered as "(unnamed)"). Name is NOT recorded in `CreateEntityCommand`. The entity is NOT discarded. | `confirm_rename()` auto-rename branch: skips `set_post_creation_name()` when `new_name.empty()` |
| **Existing rename (F2) is active when Create is triggered** | Pending rename confirmed first (same behavior as focus-loss), then new entity created with auto-rename. | `draw_ui()` "Create Empty" handler: calls `confirm_rename(ctx)` before `execute_create_entity()` |
| **Auto-rename already active when Create is triggered** | First auto-rename confirmed (name recorded), then second entity created with new auto-rename. | Same as above — `confirm_rename()` handles auto-rename case, then new `execute_create_entity()` |
| **Escape during auto-rename** | Entity creation undone via `CommandStack::undo()`. Selection restored to pre-create state. No command on undo stack. | Escape handler: sets `pending_undo_creation_` flag; deferred at end of `draw_ui()` |
| **Focus loss during auto-rename** | Name confirmed (same as Enter). Name recorded in `CreateEntityCommand`. | `IsItemDeactivatedAfterEdit()` handler calls `confirm_rename()` |
| **Left-click on another entity during auto-rename** | Auto-rename confirmed (focus loss), then click processed normally. | Left-click handler: calls `confirm_rename()` for auto-rename, then processes click |
| **Empty-area left-click during auto-rename** | Auto-rename confirmed (focus loss), then selection cleared. | Empty-area click handler: calls `confirm_rename()` before clearing selection |
| **F2 during auto-rename** | Auto-rename state abandoned (no command pushed for the name), F2 starts normal rename on selected entity. | F2 handler: clears `auto_rename_entity_id_` and `pending_create_command_` before existing logic |
| **Rapid create → create (two "Create Empty" clicks)** | First creates auto-rename. Second confirms first (same as Enter), creates second with new auto-rename. Both entities exist. | First create's InputText loses focus → `IsItemDeactivatedAfterEdit()` → `confirm_rename()`; MenuItem handler also calls `confirm_rename()` which is safe to call twice (second call returns early because `renaming_entity_` is already reset) |
| **Undo after create+name** | Single undo destroys the entity (selection restored). Redo recreates entity with name. | `CreateEntityCommand::undo()` unchanged; `redo()` calls `execute()` which re-applies `post_creation_name_` |
| **Redo after undo of create+name** | Entity recreated with the same name. | `CreateEntityCommand::execute()` with `stored_parent_id_` and `post_creation_name_` |
| **Create entity, close app before confirming** | Entity exists unnamed (creation already executed). Unsaved state handled by existing save-prompt. | No special handling needed — `pending_create_command_` is a raw pointer, lifetime ends with app close |
| **Create entity, Escape (discard), then Ctrl+Z** | Create was undone at creation time. Undo stack has no entry for it. Ctrl+Z undoes previous operation. | `pending_undo_creation_` flag calls `undo()` which removes the command from the undo stack |
| **`Entity::set_name()` fails (engine error)** | `set_name` is void — assume always succeeds. No error handling needed. | Spec assumption A-07 |
| **Out-of-memory during entity creation** | `World::add_entity()` may throw `std::bad_alloc`. Command fails to execute, is not pushed to undo stack. No auto-rename occurs. `pending_create_command_` remains nullptr. | `execute_create_entity()` checks `created_id != EntityId::none()` |
| **Selection restore with stale EntityIds** | Same as F-04 — entities that no longer exist remain in selection set with no visual effect. | No changes needed |

## Security impact

None. Entity creation and naming are in-memory operations only. Entity names are plain strings; no sanitisation is required.

## Data and migration impact

None. No schema changes, data migrations, or seed data changes.

## API compatibility impact

- `CreateEntityCommand` gains two new public methods (`created_entity_id()` and `set_post_creation_name()`). Existing code that constructs and executes `CreateEntityCommand` continues to compile and work unchanged (backward-compatible).
- No changes to `Command` base class or `CommandStack` interface.

## Documentation impact

- README: None
- Wiki pages: `docs/wiki/editor/editor-panels.md` — The "Entity Operations" table at line 344 currently says "Not auto-selected" for Create Empty. This must be updated to reflect the new auto-select + auto-rename behavior. The F-04 description paragraph at line 416 should be updated. A cross-reference to this spec (auto-rename-on-create) should be added.
- Other specs: None — F-04 spec remains as a historical snapshot.

## ADR impact

None. No new architectural decisions are needed. The changes are backward-compatible extensions to existing classes (`CreateEntityCommand` gains new methods) and ScenePanel state management — no new subsystems, no changes to patterns established by ADR-027 or ADR-029.

## Done criteria

- [ ] `CreateEntityCommand` has a public `[[nodiscard]] auto created_entity_id() const -> EntityId` getter.
- [ ] `CreateEntityCommand` has a public `auto set_post_creation_name(std::string name) -> void` setter.
- [ ] `CreateEntityCommand` has a private `std::optional<std::string> post_creation_name_` member.
- [ ] `CreateEntityCommand::execute()` calls `new_entity.set_name(*post_creation_name_)` after entity creation if `post_creation_name_` has a value.
- [ ] `CreateEntityCommand::execute()` with `post_creation_name_` = `std::nullopt` creates unnamed entity (unchanged from current behavior).
- [ ] `CreateEntityCommand::undo()` destroys entity regardless of name state (unchanged).
- [ ] `scene_panel.h` includes `"commands/create_entity_command.h"`.
- [ ] `scene_panel.h` has private members: `CreateEntityCommand* pending_create_command_`, `std::optional<EntityId> auto_rename_entity_id_`, `bool pending_undo_creation_`.
- [ ] `scene_panel.cpp` "Create Empty" handler calls `confirm_rename(ctx)` before `execute_create_entity()` if `renaming_entity_` has a value.
- [ ] `scene_panel.cpp` `execute_create_entity()` stores raw pointer, executes command, auto-selects, starts rename, sets `auto_rename_entity_id_`.
- [ ] `scene_panel.cpp` `confirm_rename()` checks for auto-rename mode and handles it separately (sets name directly + records in `CreateEntityCommand`, no `RenameEntityCommand`).
- [ ] `scene_panel.cpp` `confirm_rename()` regular path (non-auto-rename) is unchanged — still pushes `RenameEntityCommand`.
- [ ] `scene_panel.cpp` Escape handler in `draw_ui()`: if `auto_rename_entity_id_` matches `renaming_entity_`, cancels rename and sets `pending_undo_creation_ = true`.
- [ ] `scene_panel.cpp` Escape handler in `draw_ui()`: for non-auto-rename Escape, calls `cancel_rename()` (unchanged).
- [ ] `scene_panel.cpp` end of `draw_ui()` has deferred undo check: `if (pending_undo_creation_) { undo creation; }`.
- [ ] `scene_panel.cpp` F2 handler abandons auto-rename state (`auto_rename_entity_id_.reset()`, `pending_create_command_ = nullptr`) before existing logic.
- [ ] `scene_panel.cpp` left-click handler for different entity: if auto-rename active, calls `confirm_rename(ctx)` instead of `cancel_rename()`.
- [ ] `scene_panel.cpp` empty-area left-click handler confirms rename if active before clearing selection.
- [ ] Debug log calls added for: auto-rename started, auto-rename confirmed, auto-rename cancelled.
- [ ] All 8 new unit tests (T-01 through T-08) pass.
- [ ] All existing entity operation tests pass (`[editor][commands]` tags).
- [ ] Zero new compiler warnings from `src/editor/` and `tests/`.
