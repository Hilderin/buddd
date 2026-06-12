# SPEC-F-04 — Scene Panel — Entity Operations

## Problem

The Scene Panel now renders the entity hierarchy (F-02) and supports selection with multi-select (F-03), but there is no way to create, delete, or rename entities from within the editor. Users must resort to programmatic scene setup or external tools to perform basic entity lifecycle operations. Without these operations, the editor is read-only for entity data — a critical gap for any interactive scene authoring workflow.

Additionally, no undo/redo mechanism exists for entity operations. Any accidental deletion or creation forces a scene reload.

## Goals

| ID | Goal |
|---|---|
| G-01 | **Create Empty entity**: User can create a new root or child entity via right-click context menu. |
| G-02 | **Delete entity**: User can delete one or more selected entities via right-click context menu or Delete key. Confirmation dialog shown when children will also be deleted. |
| G-03 | **Rename entity**: User can rename a single selected entity via right-click context menu or F2 key, using an inline edit field. |
| G-04 | **Undo/redo**: All three operations (Create, Delete, Rename) support undo and redo via the Command pattern. Selection is snapshot before each operation and restored on undo/redo. |
| G-05 | **Context menu**: Right-click on an entity opens a context menu with applicable actions. Right-click on empty area opens a context menu with "Create Empty" only. |
| G-06 | **Keyboard shortcuts**: Delete key triggers deletion (gated by Scene Panel focus). F2 triggers rename (gated by Scene Panel focus). |
| G-07 | **Inline rename UX**: Rename shows an ImGui `InputText` overlay on the entity's tree node, with Enter to confirm, Escape to cancel. |
| G-08 | **Command interface change**: `Command::execute()` and `Command::undo()` accept `EditorContext const&` so commands can access the Editor and World. `CommandStack` passes the context through. |
| G-09 | **flush_destroyed()**: Called once per frame in `Editor::update()` after command processing, ensuring destroyed entities are removed before panel rendering. |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | **No toolbar buttons** — no "+" or "−" buttons in the Scene Panel header or any toolbar. |
| NG-02 | **No drag-and-drop parenting** — reparenting via drag-and-drop is deferred to a future feature. |
| NG-03 | **No duplicate entity** — no "Duplicate" or "Copy/Paste" entity operation. |
| NG-04 | **No prefab operations** — no "Create Prefab", "Unpack Prefab", or similar. |
| NG-05 | **No undo history visualization** — no undo stack panel or graphical undo history. |
| NG-06 | **No change to panel registration** — existing `Editor::add_panel()` pattern unchanged. |
| NG-07 | **No component editing** — entity operations only; component add/remove/modify is deferred. |
| NG-08 | **No entity search/filter** — no search bar in Scene Panel. |
| NG-09 | **No Ctrl+Z/Ctrl+Y shortcut binding** — undo/redo shortcut binding is existing infrastructure (ShortcutRegistry); this feature provides the Command implementations. |
| NG-10 | **No confirmation dialog for leaf entity deletion** — only shown when at least one entity being deleted has children. |

## Actors

| Actor | Description |
|---|---|
| **Editor user** | Opens the editor, selects entities in the Scene Panel, right-clicks or presses keys to create/delete/rename entities. Uses Ctrl+Z/Ctrl+Y to undo/redo. Sees confirmation dialogs when deletion affects children. |
| **Command author** | Implements `CreateEntityCommand`, `DeleteEntityCommand`, `RenameEntityCommand`. Each command snapshots selection before mutation, restores it on undo. |
| **Future feature developer** | Builds on the Command pattern and `EditorContext` plumbing for future entity operations (duplicate, reparent, prefab). Relies on the modified `Command`/`CommandStack` signatures. |

## User-visible behavior

### Context Menu

| Trigger | Location | Menu items |
|---|---|---|
| Right-click on an entity | `ImGui::BeginPopupContextItem` on the entity's tree node | "Create Empty", "Delete", "Rename" — in that order |
| Right-click on empty area | `ImGui::BeginPopupContextWindow` (or equivalent) on the Scene Panel content area | "Create Empty" only |

Menu items that are not applicable to the current selection state are greyed out (disabled):
- **Delete**: disabled when selection is empty.
- **Rename**: disabled when selection is empty, or when more than one entity is selected.

### Create Empty

- Selecting "Create Empty" executes a `CreateEntityCommand`.
- **With selection anchor**: The new entity is created as the last child of `EditorSelection::anchor()` (the entity set by the most recent Replace-click).
- **Without selection anchor / empty selection**: The new entity is created as a root entity (appended as the last root).
- The new entity has an empty name (rendered as "(unnamed)" in the tree).
- The new entity is **not** automatically selected after creation (the previous selection persists).
- **Undo**: The created entity is destroyed, previous selection restored.
- **Redo**: The entity is re-created, previous selection restored.

### Delete

- Deleting one or more entities executes a `DeleteEntityCommand`.
- **Single entity, no children**: No confirmation dialog. Entity is destroyed immediately.
- **Single entity with children**: Confirmation dialog is shown: "Delete [entity_name] and its [N] children?" with "Delete" and "Cancel" buttons.
- **Multiple entities (any with children)**: Single confirmation dialog: "Delete [N] entities? ([M] have children that will also be deleted.)" with "Delete" and "Cancel" buttons.
- **Multiple entities, all leaves**: No confirmation dialog. All entities destroyed immediately.
- After deletion, the selection is **cleared** (`EditorSelection::clear()`).
- The entities are marked for destruction via `Entity::destroy()` — they are not physically removed until `World::flush_destroyed()` is called in `Editor::update()`.
- **Undo**: All deleted entities are restored with their identity, name, and hierarchy (parent-child relationships). Component state is NOT preserved in v1 — entities are restored with default components. Selection is restored via snapshot.
- **Redo**: Entities are destroyed again, selection cleared.

### Rename

- Rename operates on the **first selected entity** (from `EditorSelection::current()` — the first in iteration order).
- **Trigger**: F2 key (when Scene Panel is focused and exactly one entity is selected) or right-click context menu → "Rename".
- **Inline edit behavior**:
  1. The entity's tree node label is replaced with an `ImGui::InputText` widget.
  2. The InputText is auto-focused via `ImGui::SetKeyboardFocusHere()`.
  3. The InputText spans the available node width (`ImGui::CalcTextSize` + padding or using the tree node's content width).
  4. The buffer is pre-filled with the entity's current name.
- **Confirmation** (Enter key or InputText loses focus): A `RenameEntityCommand` is created and executed with the new name.
  - If the new name is empty → the command is **not** executed. The name reverts to the original.
  - If the new name equals the old name → the command is **not** executed (no-op). Inline edit closes without pushing a command.
- **Cancellation** (Escape key): Inline edit closes, name reverts to original. No command is created.
- **Focus loss**: If the InputText loses focus (user clicks elsewhere), behavior is the same as Enter — rename is confirmed.
- While rename is active on one entity, clicking another entity first confirms the pending rename (via focus loss), then processes the new click.
- Only one entity can be in rename state at a time.

### Keyboard Shortcuts

| Key | Behavior | Gate |
|---|---|---|
| **Delete** | Delete selected entities (same behavior as context menu "Delete") | `ImGui::IsWindowFocused()` on Scene Panel |
| **F2** | Start inline rename on first selected entity | `ImGui::IsWindowFocused()` on Scene Panel, exactly one entity selected |
| **Enter** (during rename) | Confirm rename | Inline edit is active |
| **Escape** (during rename) | Cancel rename | Inline edit is active |

### flush_destroyed() Lifecycle

```
Editor::update(frame_ctx):
    // ... other update logic ...
    for each panel: panel.update(editor_ctx)
    world().flush_destroyed()        // ← destroyed entities physically removed here
    // ... remaining update logic ...

Editor::draw_ui(frame_ctx):
    // panels render; entity tree reads world() which no longer contains flushed entities
```

This ensures that:
- Commands that call `Entity::destroy()` during `draw_ui()` (e.g., from context menu) leave entities in "pending destroy" state until the next frame's `update()`.
- Panels in `draw_ui()` never iterate over freshly flushed entities (they are removed before the next `draw_ui()`).

### Undo/Redo

- `CommandStack` is accessed via `Editor::command_stack()` (or existing private member with accessor).
- `CommandStack::undo()` and `CommandStack::redo()` accept `EditorContext const&` and forward it to the command's `undo()`/`execute()`.
- Selection is snapshot in each command's constructor (before mutation) and restored in `undo()`. On `redo()`, the command re-snapshots the current selection, performs the operation, then restores the snapshot.

### Command Signature Change

Current `Command` base class:
```cpp
virtual auto execute() -> void = 0;
virtual auto undo() -> void = 0;
```

New `Command` base class:
```cpp
virtual auto execute(EditorContext const& ctx) -> void = 0;
virtual auto undo(EditorContext const& ctx) -> void = 0;
```

`CommandStack` gains `EditorContext const&` parameters:
- `execute(std::unique_ptr<Command>, EditorContext const& ctx)` — forwards `ctx` to `command->execute(ctx)`.
- `undo(EditorContext const& ctx)` — forwards `ctx` to top-of-undo `command->undo(ctx)`.
- `redo(EditorContext const& ctx)` — forwards `ctx` to top-of-redo `command->execute(ctx)`.

## Key entities

### Command Interface Changes

**`src/editor/command.h`** — `Command` base class:
- `execute()` and `undo()` gain `EditorContext const& ctx` parameter.

**`src/editor/command_stack.h`** — `CommandStack`:
- `execute(unique_ptr<Command>)` → `execute(unique_ptr<Command>, EditorContext const& ctx)`.
- `undo()` → `undo(EditorContext const& ctx)` — returns bool, forwards context.
- `redo()` → `redo(EditorContext const& ctx)` — returns bool, forwards context.

### New Command Files

**`src/editor/commands/create_entity_command.h`** — `CreateEntityCommand`:
- Stores snapshot of selection before creation.
- Stores the created `EntityId` after `execute()` (for undo).
- `execute()`: Create entity as child of anchor (or root if no anchor). Save created EntityId.
- `undo()`: Call `Entity::destroy()` on created entity. Restore selection snapshot.
- `name()`: Returns `"Create Entity"`.

**`src/editor/commands/delete_entity_command.h`** — `DeleteEntityCommand`:
- Stores snapshot of selection before deletion.
- Stores serialized state of all deleted entities (EntityIds, names, hierarchy, parent info).
- `execute()`: Call `Entity::destroy()` on each entity. Clear selection.
- `undo()`: Recreate all entities from saved state (restoring names, hierarchy, parents). Restore selection snapshot.
- `name()`: Returns `"Delete Entity"`.

**`src/editor/commands/rename_entity_command.h`** — `RenameEntityCommand`:
- Stores entity ID, old name, new name.
- Stores snapshot of selection before rename.
- `execute()`: Call `Entity::set_name(new_name)`.
- `undo()`: Call `Entity::set_name(old_name)`. Restore selection snapshot.
- `name()`: Returns `"Rename Entity"`.

### ScenePanel Changes

**`src/editor/panels/scene_panel.h`**:
- Add state tracking for inline rename: `std::optional<buddd::engine::EntityId> renaming_entity_` and `std::string rename_buffer_`.
- Add context menu handling via `ImGui::BeginPopupContextItem()` after each entity tree node.
- Add empty-area context menu via `ImGui::BeginPopupContextWindow()` or equivalent.
- Add Delete key handling: `if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete))`.
- Add F2 key handling: `if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_F2))`.
- Add inline rename rendering: when `renaming_entity_` matches the current entity, render `ImGui::InputText` instead of the label.
- Add confirmation dialog for hierarchical deletion via `ImGui::OpenPopup` / `ImGui::BeginPopupModal`.
- Integrate with `Editor::command_stack()` for executing commands.

### Editor Changes

**`src/editor/editor.h`** (`Editor`):
- Add `[[nodiscard]] auto command_stack() -> CommandStack&;` public accessor (or keep private and expose via friend, but accessor is simpler).

**`src/editor/editor.cpp`** (`Editor`):
- In `Editor::update()`: call `world().flush_destroyed()` after menu/panel updates and after any command processing.

**`src/editor/panels/menu_bar.h`** (`MenuBar`):
- `MenuBar::draw_ui()` calls `command_stack_.undo(ctx)` and `command_stack_.redo(ctx)` when the corresponding menu items are selected. These call-sites must be updated to pass `EditorContext const& ctx` to match the new `CommandStack` signatures.

## User stories

### Story 1 — Create a new entity via context menu (Priority: P1)

As an editor user, I want to create a new empty entity from the context menu so that I can add objects to my scene.

**Given** the Scene Panel shows a root entity "Player" (selected)
**When** I right-click "Player" and select "Create Empty" from the context menu
**Then** a new entity with empty name is created as the last child of "Player"
**And** the selection remains unchanged (still contains "Player")

**Given** no entity is selected (empty selection)
**When** I right-click empty area in the Scene Panel and select "Create Empty"
**Then** a new entity with empty name is created as a root entity
**And** the selection remains empty

**Given** Ctrl+click multi-select (no anchor) with entities "A", "B", "C" selected
**When** I right-click any entity and select "Create Empty"
**Then** the new entity is created as a root entity (no anchor exists, degrade to root)

### Story 2 — Delete a single entity without children (Priority: P1)

As an editor user, I want to delete a selected entity without children so that I can remove unneeded objects.

**Given** entity "Cube" is selected and has `child_count() == 0`
**When** I press the Delete key (with Scene Panel focused)
**Then** "Cube" is marked for destruction
**And** the selection becomes empty
**And** no confirmation dialog is shown

**Given** entity "Cube" is selected and has `child_count() == 0`
**When** I right-click "Cube" and select "Delete"
**Then** "Cube" is marked for destruction
**And** no confirmation dialog is shown

### Story 3 — Delete an entity with children (Priority: P1)

As an editor user, I want to see a confirmation dialog when deleting an entity that has children, so that I don't accidentally delete an entire subtree.

**Given** entity "Group" is selected and has 3 children
**When** I press the Delete key
**Then** a confirmation dialog appears: "Delete Group and its 3 children?"
**And** no entities are destroyed yet
**When** I click "Delete" in the confirmation dialog
**Then** "Group" and all its children are marked for destruction
**And** the selection becomes empty

**Given** entity "Group" is selected and has 3 children
**When** I press the Delete key and click "Cancel" in the confirmation dialog
**Then** "Group" and its children are NOT destroyed
**And** the selection remains unchanged

### Story 4 — Delete multiple entities with single confirmation (Priority: P1)

As an editor user, I want a single confirmation for multi-select delete, so that I can efficiently remove several entities at once.

**Given** "A" (no children) and "B" (2 children) are both selected
**When** I press the Delete key
**Then** a confirmation dialog appears: "Delete 2 entities? (1 has children that will also be deleted.)"
**When** I click "Delete"
**Then** "A" and "B" are both marked for destruction
**And** the selection becomes empty

**Given** "A" and "B" are both selected, both with `child_count() == 0`
**When** I press the Delete key
**Then** no confirmation dialog is shown
**And** "A" and "B" are destroyed immediately

### Story 5 — Rename an entity via F2 (Priority: P1)

As an editor user, I want to rename a selected entity by pressing F2 so that I can give meaningful names to my entities.

**Given** entity "Cube" is selected (exactly one entity selected)
**When** I press F2 (with Scene Panel focused)
**Then** the tree node label for "Cube" is replaced by an editable text field pre-filled with "Cube"
**And** the text field is auto-focused
**When** I type "PlayerCube" and press Enter
**Then** the entity name changes to "PlayerCube"
**And** the tree displays "PlayerCube"

**Given** F2 rename is active on entity "Cube"
**When** I type a new name and press Escape
**Then** the entity name reverts to "Cube"
**And** the tree displays "Cube"
**And** no command is pushed to the undo stack

**Given** F2 rename is active on entity "Cube"
**When** I clear the text and press Enter
**Then** the entity name remains "Cube" (empty name rejected)
**And** the tree displays "Cube"
**And** no command is pushed to the undo stack

### Story 6 — Rename an entity via context menu (Priority: P1)

As an editor user, I want to rename an entity from the right-click context menu, as an alternative to F2.

**Given** entity "Cube" is selected (exactly one entity selected)
**When** I right-click "Cube" and select "Rename"
**Then** the same inline rename UI appears as when pressing F2

### Story 7 — Undo and redo entity operations (Priority: P1)

As an editor user, I want to undo and redo entity create/delete/rename operations so that I can recover from mistakes.

**Given** I have created an entity named "NewEntity"
**When** I press Ctrl+Z
**Then** "NewEntity" is destroyed
**And** the selection is restored to what it was before creation
**When** I press Ctrl+Y (or Ctrl+Shift+Z)
**Then** "NewEntity" is re-created
**And** the selection is restored

**Given** I have renamed entity "Cube" to "Sphere"
**When** I press Ctrl+Z
**Then** the entity's name reverts to "Cube"
**And** the selection is restored
**When** I press Ctrl+Y
**Then** the entity's name changes back to "Sphere"
**And** the selection is restored

**Given** I have deleted entity "Cube" (no children)
**When** I press Ctrl+Z
**Then** "Cube" is recreated with its original name, transform, and parent
**And** the selection is restored to what it was before deletion
**When** I press Ctrl+Y
**Then** "Cube" is destroyed again
**And** the selection is cleared

### Story 8 — Context menu adapts to selection state (Priority: P2)

As an editor user, I want only applicable context menu items to be available, so that I don't attempt invalid operations.

**Given** no entity is selected
**When** I right-click on empty area in the Scene Panel
**Then** the context menu shows only "Create Empty"
**And** "Delete" and "Rename" are not present

**Given** multiple entities are selected
**When** I right-click on one of them
**Then** "Create Empty" and "Delete" are enabled
**And** "Rename" is disabled (greyed out)

**Given** exactly one entity is selected
**When** I right-click on it
**Then** "Create Empty", "Delete", and "Rename" are all enabled

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-01 | `Command::execute()` and `undo()` accept `EditorContext const&` parameter. | Inspect `command.h` — verify signature change. Compiles without errors. |
| AC-02 | `CommandStack::execute()` accepts `EditorContext const&` and forwards it to `Command::execute()`. | Inspect `command_stack.h` — verify `execute(unique_ptr<Command>, EditorContext const&)`. |
| AC-03 | `CommandStack::undo()` and `redo()` accept `EditorContext const&` and forward to `Command::undo()`/`execute()`. | Inspect `command_stack.h` — verify signatures. |
| AC-04 | Existing `QuitCommand` (the only existing Command subclass) compiles with the new `Command::execute(EditorContext const&)`/`undo(EditorContext const&)` signatures. | Full build succeeds with zero errors. |
| AC-05 | Right-click on an entity shows a context menu with "Create Empty", "Delete", "Rename". | Manual: right-click entity, see three items in order. |
| AC-06 | Right-click on empty area shows a context menu with only "Create Empty". | Manual: right-click empty area, see only "Create Empty". |
| AC-07 | "Delete" context menu item is disabled when selection is empty. | Manual: no selection, right-click entity → "Delete" greyed out. |
| AC-08 | "Rename" context menu item is disabled when selection is empty or multi-select. | Manual: empty selection → "Rename" greyed out. Select 2 entities → "Rename" greyed out. |
| AC-09 | Selecting "Create Empty" creates entity as last child of selection anchor. | Unit test: set anchor to entity A, execute CreateEntityCommand, verify new entity is A's last child. |
| AC-10 | Selecting "Create Empty" with no anchor creates root entity. | Unit test: clear selection, execute CreateEntityCommand, verify new entity is a root. |
| AC-11 | Deleting single entity without children shows no confirmation. | Manual: select leaf entity, press Delete, no dialog appears. |
| AC-12 | Deleting single entity with children shows confirmation dialog. | Manual: select entity with children, press Delete, confirmation appears. |
| AC-13 | Confirmation dialog contains "Delete" and "Cancel" buttons. Clicking "Delete" destroys entities; clicking "Cancel" does nothing. | Manual: confirm / cancel — verify behavior. |
| AC-14 | Deleting multiple entities with at least one having children shows a single confirmation. | Manual: multi-select entities including one with children, press Delete, see single confirmation. |
| AC-15 | Deleting multiple entities where all are leaves shows no confirmation. | Manual: multi-select all-leaf entities, press Delete, no dialog. |
| AC-16 | After deletion, selection is empty. | Unit test: execute DeleteEntityCommand, verify `selection().empty()` is true. |
| AC-17 | F2 with exactly one entity selected triggers inline rename. | Manual: select one entity, press F2, see editable text field. |
| AC-18 | F2 with no selection or multi-select is a no-op. | Manual: no selection → F2 does nothing. Multi-select → F2 does nothing. |
| AC-19 | Delete key when Scene Panel is not focused is a no-op (does not trigger deletion). | Manual: focus another panel, press Delete, nothing happens. |
| AC-20 | F2 when Scene Panel is not focused is a no-op. | Manual: focus another panel, press F2, nothing happens. |
| AC-21 | Inline rename: Enter confirms (pushes RenameEntityCommand). | Unit test: simulate rename of "Old"→"New", verify world entity name changes. |
| AC-22 | Inline rename: Escape cancels (no command pushed, name unchanged). | Unit test: simulate rename with Escape, verify name unchanged, command stack size unchanged. |
| AC-23 | Inline rename: empty name on Enter is rejected (name reverts, no command). | Manual: rename entity, clear text, press Enter, name reverts to original. |
| AC-24 | Inline rename: same name on Enter is a no-op (no command pushed). | Manual: rename to same name, press Enter, no undo stack entry created. |
| AC-25 | RenameEntityCommand::undo() restores the old name. | Unit test: execute rename "A"→"B", undo, verify name is "A". |
| AC-26 | CreateEntityCommand::undo() destroys the created entity. | Unit test: execute create, undo, verify entity no longer exists. |
| AC-27 | DeleteEntityCommand::undo() recreates all deleted entities with original state (name, parent, children). | Unit test: delete entity with children, undo, verify all entities restored with correct names and hierarchy. |
| AC-28 | DeleteEntityCommand::undo() restores pre-delete selection. | Unit test: save selection, delete, undo, verify selection matches saved. |
| AC-29 | All three command types support redo. | Unit test: execute → undo → redo, verify state matches post-execution. |
| AC-30 | `Editor::update()` calls `world().flush_destroyed()` each frame. | Inspect `editor.cpp` — verify `flush_destroyed()` is called in `update()`. |
| AC-31 | Selection is cleared immediately on delete (before `flush_destroyed()`). | Unit test: after `Entity::destroy()` (before flush), verify `selection().empty()`. |
| AC-32 | Context menu does not change selection — right-click alone does not select. | Manual: right-click a non-selected entity, verify selection unchanged. |
| AC-33 | All existing tests still pass. | Run `buddd_tests`. |
| AC-34 | Zero new warnings from `src/editor/` and `tests/`. | Build with `cmake --build --preset debug`. |

## E2E Verification

| Method | Description |
|---|---|
| **Headless unit test (CI)** | Build with `BUDDD_HAS_DISPLAY=OFF`. Run `buddd_tests`. Verify tagged tests pass: `[editor][commands][create]`, `[editor][commands][delete]`, `[editor][commands][rename]`, `[editor][commands][undo_redo]`. |
| **Manual smoke test (display)** | Run `buddd edit` with a scene loaded. Verify: right-click entity → context menu with three items; Create Empty creates child entity; Delete with confirmation; F2 inline rename; Delete key gated by focus; Ctrl+Z undo/redo. |
| **Clean build verification (CI)** | Run `cmake --build --preset debug` and verify zero warnings from `src/editor/` and `tests/`. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A user can create, rename, and delete entities entirely from the Scene Panel (no code changes). | Manual: create entity, rename it, delete it — all via Scene Panel alone. |
| SC-002 | Accidental deletions with children are prevented by a confirmation dialog. | Manual: delete entity with children, see confirmation. |
| SC-003 | All three entity operations can be undone and redone without data loss of entity identity, name, and hierarchy. Component state preservation is deferred to a future feature (entities restored with default components only). | Manual: create → undo (entity gone) → redo (entity back); same for rename and delete. Undo delete does not restore component data in v1. |
| SC-004 | Inline rename feel is responsive: InputText appears immediately on F2, Enter/Esc close it. | Manual: select entity, press F2, see edit field. Press Enter, see name update instantly. |

## Edge cases

| Case | Expected behavior |
|---|---|
| **Create with multi-select (no anchor)** | New entity is created as a root entity (no anchor = degrade to root). |
| **Create with Ctrl+click multi-select (Toggle-only, no Replace)** | No anchor (Toggle does not set anchor). New entity created as root. |
| **Delete all entities in World** | All entities destroyed. Selection cleared. If root entities with children exist, confirmation dialog shown ("Delete N entities? ..."). |
| **Delete entity that is both selected and being renamed** | While inline rename is active (InputText focused), the Delete key is consumed by the InputText widget (deletes a character). To delete the entity, the user must first confirm (Enter) or cancel (Escape) the rename, then press Delete. The ScenePanel's Delete key handler is only active when no inline rename is in progress. |
| **F2 while rename is already active on another entity** | The active rename is confirmed first (Enter-equivalent), then F2 triggers rename on the new entity. |
| **F2 on an entity that is currently collapsed** | Rename works regardless of expansion state. The tree node is visible (selected entities are expanded if their parent is collapsed... actually how? ImGui requires parent to be expanded to render children). F2 only works on entities visible in the tree. If a selected entity has a collapsed ancestor, the entity is not rendered — F2 is a no-op (the entity is technically not focusable). |
| **Rename empty name (Enter with empty buffer)** | Rejected — name reverts to original, no command pushed. |
| **Rename to same name** | Accepted (no validation against identity). But no command is pushed — no-op. (If the name equals the old name, the command is skipped entirely.) |
| **Rename with special characters (Unicode, emoji)** | `Entity::set_name()` accepts any string. ImGui `InputText` handles UTF-8 input. No special handling needed. |
| **Delete entity that is pending destroy (should not happen)** | Not possible — `Entity::destroy()` sets `pending_destroy_ = true`. A second call is a no-op. Selection is already cleared. |
| **Undo after scene load** | Command stack is cleared on `new_scene()` / `open_scene()`. No stale commands reference destroyed entities. |
| **Redo with empty redo stack** | `CommandStack::redo()` returns false. No-op. |
| **Undo with empty undo stack** | `CommandStack::undo()` returns false. No-op. |
| **Context menu on entity that is not visible (scrolled off)** | Not possible — the user must right-click on a visible item for `BeginPopupContextItem` to fire. |
| **Rapid F2 presses** | Each F2 press when no rename is active starts rename. If rename is active, second F2 first confirms (Enter), then triggers new rename. |
| **Delete key while confirmation dialog is open** | Confirmation dialog is a modal — it captures keyboard input. Delete key is not processed by Scene Panel while modal is open. |
| **Multiple entities selected, some with children, some without — delete confirmation** | Single confirmation dialog summarizing total entities and count of those with children. |
| **Entity with 1000+ children — delete confirmation** | Dialog counts children correctly. No performance issue (child count is O(1) from Entity::child_count()). |
| **EditorContext not yet available during Command construction** | Commands store `EditorContext const&` as a reference or pointer and use it only in `execute()`/`undo()`. Construction does not require the context. |
| **Selection snapshot in DeleteEntityCommand with already-destroyed entities** | The snapshot is taken before `Entity::destroy()` is called. All entities are alive at snapshot time. |

## Error cases

| Case | Expected behavior |
|---|---|
| **`Entity::set_name()` fails (engine error)** | No mechanism for failure — `set_name` is void. Assume always succeeds. |
| **`Entity::destroy()` called on already pending_destroy entity** | No-op (safe to call multiple times). |
| **World::flush_destroyed() called while a rename InputText is active** | Rename state is per-panel (ScenePanel), not per-entity. The entity being renamed is still alive (it hasn't been destroyed). No conflict. |
| **Out-of-memory during entity creation** | `World::add_entity()` may throw `std::bad_alloc`. The command fails to execute. The command is not pushed to the undo stack. |
| **Selection restore with stale EntityIds (entity already destroyed by external means)** | `EditorSelection::restore()` accepts any `Selection`. Entities that no longer exist simply remain in the selection set — they have no visual effect (no tree node to highlight). This is consistent with F-03 error handling. |
| **Delete confirmation dialog dismissed by clicking outside (ImGui modal behaviour)** | Standard ImGui modal: clicking outside the modal closes it (equivalent to Cancel). No entities destroyed. |
| **Corner case: rename buffer overflows InputText max length (ImGui default is 256 by default, can be larger)** | ImGui `InputText` handles truncation. No crash. `InputText` supports configurable buffer sizes via the `size` parameter in its constructor. The rename buffer should be large enough (e.g., 256 chars). |
| **Delete undo restores entity without component data** | In v1, undo delete restores entity identity, name, and hierarchy but restores only default components. Any component data (Transform, Mesh, etc.) that existed before deletion is lost. This is a documented limitation — not an error condition, but users should be aware that undo delete is not a full state restore. |

## Permissions and security

- No changes to permissions or security posture.
- Entity operations are in-memory only — no file I/O, no network access, no sensitive data.
- No authentication or authorisation boundaries are crossed.
- Entity names are plain strings; no sanitisation is required (the engine already handles this).

## Observability

| Signal | Source |
|---|---|
| **Command executed** | Log at debug level: `BUDDD_LOG_DEBUG("[Command] {}: executed", command->name())` — added in `CommandStack::execute()`. |
| **Command undone** | `BUDDD_LOG_DEBUG("[Command] {}: undone", command->name())` — added in `CommandStack::undo()`. |
| **Command redone** | `BUDDD_LOG_DEBUG("[Command] {}: redone", command->name())` — added in `CommandStack::redo()`. |
| **Entity created** | Debug log in `create_entity_command.cpp`: `BUDDD_LOG_DEBUG("CreateEntity: entity {} created under {}", id.index, parent_id.index_or_none)` |
| **Entity deleted** | Debug log in `delete_entity_command.cpp`: `BUDDD_LOG_DEBUG("DeleteEntity: {} entities destroyed", count)` |
| **Entity renamed** | Debug log in `rename_entity_command.cpp`: `BUDDD_LOG_DEBUG("RenameEntity: {} -> {}", old_name, new_name)` |
| **Delete confirmation shown** | Debug log: `BUDDD_LOG_DEBUG("Delete confirmation dialog shown ({} entities, {} with children)", total, with_children)` |
| **Inline rename started** | Debug log: `BUDDD_LOG_DEBUG("Rename started for entity {}", id.index)` |

## Documentation impact

| Document | Reason for update |
|---|---|
| `docs/wiki/editor/editor-panels.md` | Add entity operations section describing Create Empty, Delete, Rename behavior, context menu, keyboard shortcuts, and confirmation dialogs. Update the Scene Panel feature table. Document the Command signature change (execute/undo now accept `EditorContext const&`). Note new command files in `src/editor/commands/`. |
| `docs/wiki/editor/cross-panel-communication.md` | Update to reflect that Commands now use `EditorContext` to access the World and selection state. Document that entity operations interact with selection via snapshot/restore. |
| `docs/wiki/editor/scene-management.md` | No changes expected — flush_destroyed() lifecycle is internal and does not affect scene management API. |

These updates are the responsibility of the implementation phase and will be tracked in the implementation contract.

## Out of scope

- Toolbar buttons for create/delete — deferred (never, per decision).
- Drag-and-drop reparenting — deferred to future feature.
- Duplicate / Copy-Paste entity — deferred to future feature.
- Prefab operations — deferred to future feature.
- Undo history visualization — deferred.
- Ctrl+Z/Ctrl+Y shortcut binding (uses existing ShortcutRegistry — not part of this spec).
- Entity search/filter in Scene Panel.
- Component operations (add/remove/modify).
- Multi-entity rename (only single-entity rename).
- Delete confirmation customisation (always shown for hierarchical deletes, never for leaf deletes).
- Changes to how panels register with Editor (existing pattern unchanged).
- Any changes to World, Entity, or engine APIs beyond those consumed.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `Editor::command_stack()` accessor will be added (returning `CommandStack&`). It currently exists as an unnamed private member. |
| A-02 | `World::add_entity()` returns a new root entity. `Entity::create_child()` returns a new child entity. Both create entities that are immediately queryable (no flush needed). |
| A-03 | `Entity::destroy()` sets `pending_destroy_` flag; the entity remains in the tree until `flush_destroyed()` is called. `Entity::id()` and `Entity::name()` remain valid until flush. |
| A-04 | `ImGui::IsWindowFocused()` returns true when the Scene Panel window is focused (has keyboard focus). This gates Delete and F2 key handling. |
| A-05 | `ImGui::BeginPopupContextItem()` requires the previous ImGui item to be the entity tree node. The popup is `End()`'d after rendering menu items. |
| A-06 | `ImGui::InputText` with `ImGuiInputTextFlags_EnterReturnsTrue` flag returns true when Enter is pressed, allowing the panel to detect confirmation. |
| A-07 | Inline rename uses an `InputText` that is positioned over the tree node label area via `ImGui::SetNextItemWidth()` matching the available content width. |
| A-08 | Focus loss on `InputText` triggers `ImGui::IsItemDeactivatedAfterEdit()` which can be used to detect confirmation. If this flag is not available, the panel checks `ImGui::IsItemDeactivated()` and waits for Enter/Escape. This is an implementation detail. |
| A-09 | **v1 scope**: The DeleteEntityCommand saves entity identity, name, and hierarchy in a serializable form. For v1, this means storing at minimum: EntityId, name, parent EntityId (or sentinel for root), and an ordered list of child EntityIds. Full component state preservation is deferred to a future feature — entities are restored with default components. When A-09 says "exact pre-delete state" in user-visible behavior, this means entity identity, name, and hierarchy (not component state). |
| A-10 | `Entity::set_name()` accepts any string (including empty). Empty-name validation is handled at the UI layer (spec), not the engine layer. |
| A-11 | The CommandStack's max history of 128 entries applies to entity operations as well. Old commands are evicted when the stack exceeds 128. |
| A-12 | Existing `Command` subclasses (if any) will need their `execute()`/`undo()` signatures updated. This is tracked as a breaking change. |
| A-13 | The context menu is created fresh each frame using ImGui's popup API. The "OpenPopup" / "BeginPopup" pattern follows standard ImGui convention. |
| A-14 | Multi-select rename is not supported. "Rename" is disabled when more than one entity is selected. |
| A-15 | When no `EditorSelection::anchor()` exists (selection built entirely via Ctrl+click Toggle), "Create Empty" creates a root entity. This is a design choice documented here, not a limitation. |

## Open questions

| ID | Question | Resolution |
|---|---|---|
| Q-01 | **DeleteEntityCommand: what entity state must be preserved for undo?** The minimum viable approach saves the entity's name and parent EntityId, and recreates the entity as a child of that parent (or root). Children are restored by re-running the command with the same logic — but this is complex. A pragmatic approach: save the entire subtree (EntityId, name, parent EntityId, ordered child list) and reconstruct on undo. For v1, assume that component state is NOT restored (entities are recreated with default Transform only). If full component preservation is needed, this must be revisited. | **No clarification needed for v1.** Documented in Assumptions (A-09). |
| Q-02 | **Should inline rename confirm on focus loss or wait for Enter/Escape?** The common editor pattern (Unity, Unreal) confirms on Enter and on focus loss. Focus loss is detected via `ImGui::IsItemDeactivatedAfterEdit()`. This is the recommended approach; Escape still cancels. | **Focus loss confirms.** Documented in user-visible behavior. |
| Q-03 | **Should the context menu appear on right-click down or right-click up?** ImGui's `BeginPopupContextItem` handles this with `ImGui::IsMouseReleased(ImGuiMouseButton_Right)` by default (popup appears on right-click release). This is the standard ImGui behavior and acceptable. | **Standard ImGui behavior (release).** No change needed. |
