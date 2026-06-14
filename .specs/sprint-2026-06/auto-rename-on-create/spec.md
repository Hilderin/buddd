# SPEC-2026-06 — Scene Panel — Auto-Rename on Create

## Problem

Currently, creating an entity via "Create Empty" in the Scene Panel results in an unnamed entity with an empty selection state. To name it, the user must:
1. Click to select it
2. Press F2 or right-click → Rename
3. Type the name
4. Press Enter

This three-step interaction (create → select → rename) breaks the flow of scene authoring. Users creating multiple entities waste time on extra clicks. The entity appears in the tree as "(unnamed)" until manually renamed, polluting the hierarchy with placeholder names.

## Goals

| ID | Goal |
|---|---|
| G-01 | **Auto-select**: Creating an entity via "Create Empty" immediately selects the new entity (replaces current selection). |
| G-02 | **Auto-rename**: After creation, the entity tree node enters inline rename mode automatically (InputText focused, keyboard ready). |
| G-03 | **Single undo step**: Create + rename is recorded as a single undo step — one Ctrl+Z undoes both entity creation and name assignment atomically. |
| G-04 | **Escape discards**: Pressing Escape during the post-creation auto-rename discards the entity entirely (undoes the creation). |
| G-05 | **No change to explicit rename**: F2 and context menu "Rename" continue to work unchanged (separate RenameEntityCommand). |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | **No toolbar buttons** — no "+" or similar in the Scene Panel header or toolbar. |
| NG-02 | **No change to F2 behavior** — F2 still triggers RenameEntityCommand. |
| NG-03 | **No change to context menu "Rename"** — still triggers RenameEntityCommand. |
| NG-04 | **No change to Delete flow** — Delete key and context menu "Delete" unchanged. |
| NG-05 | **No change to entity name validation** — empty name rejection, same-name rejection unchanged. |
| NG-06 | **No multi-entity operations** — only single entity creation (as it already is). |
| NG-07 | **No change to confirm-on-focus-loss behavior** — focus loss during auto-rename still confirms (same as regular rename). |
| NG-08 | **No change to selection behavior** — only the auto-select-on-create is added. |
| NG-09 | **No change to CommandStack interface** — existing `undo()`/`redo()`/`execute()` unchanged. |

## Actors

| Actor | Description |
|---|---|
| **Editor user** | Opens the editor, creates entities in the Scene Panel via context menu. Types a name immediately without extra steps. |
| **Command implementer** | Modifies `CreateEntityCommand` to support a `post_creation_name` that can be set after construction but before undo. |

## User-visible behavior

### Create Empty (overrides F-04)

The "Create Empty" flow changes as follows:

**Before (F-04):**
1. Right-click → "Create Empty" → entity created
2. Entity is NOT auto-selected (previous selection persists)
3. User must click to select, then F2 to rename

**After (this feature):**
1. Right-click → "Create Empty" → entity created
2. Entity IS auto-selected (previous selection replaced)
3. Tree node enters inline rename mode immediately (InputText with keyboard focus)
4. User types a name and presses Enter (confirm) — name is recorded as part of the create operation
5. Escape during rename discards the entity entirely (creation undone)

### Auto-rename state

- The auto-rename state behaves identically to a regular F2-initiated rename in terms of InputText rendering (same inline `ImGui::InputText` with `EnterReturnsTrue`, `AutoSelectAll`, auto-focus).
- The buffer is pre-filled with the entity's **current (empty) name** — same as F2 behavior.
- If the user presses **Enter** (or the InputText loses focus), the rename is **confirmed**:
  - If the name is non-empty: the entity name is set, and the name is recorded in the `CreateEntityCommand` for undo/redo consistency.
  - If the name is empty: the entity remains unnamed (same as "(unnamed)" display), no name is recorded in the command. The auto-rename state ends but the entity is NOT discarded.
- If the user presses **Escape**, the entity creation is undone entirely. The entity is destroyed and the undo stack reflects the create as if it never happened.

### Undo/redo consistency

- The create + rename is a **single undo step** (one entry on the `CommandStack`).
- **Undo**: The entity is destroyed (same as `CreateEntityCommand::undo()` today). The selection is restored to what it was before creation.
- **Redo**: The entity is re-created with its assigned name (same as `CreateEntityCommand::execute()` but now names the entity).
- If the user renames the entity later via F2 or context menu "Rename", that is a **separate** undo step (via `RenameEntityCommand`).

### Interaction with existing rename state

- If a regular rename (F2) or auto-rename is already active when the user triggers "Create Empty":
  - The active rename is confirmed first (same behavior as regular rename's focus-loss confirm).
  - Then the new entity is created and enters auto-rename mode.
- If a delete confirmation dialog is open, "Create Empty" is not available (modal captures input).

### Context menu (no change to items, only to flow)

| Trigger | Menu items | Flow change |
|---|---|---|
| Right-click on entity | "Create Empty", "Delete", "Rename" | "Create Empty" now triggers auto-select + auto-rename |
| Right-click on empty area | "Create Empty" only | Same as above |

## User stories

### Story 1 — Create entity and name it in one flow (Priority: P1)

As an editor user, I want to create an entity and immediately give it a name so that the hierarchy is meaningful from the start.

**Given** no entity is selected
**When** I right-click empty area in the Scene Panel and select "Create Empty"
**Then** a new entity is created as a root entity
**And** the new entity is auto-selected (replaces the empty selection with a single-entity selection containing the new entity)
**And** the tree node enters inline rename mode, with the InputText auto-focused and prefilled with empty text
**When** I type "Player" and press Enter
**Then** the entity is named "Player"
**And** this create+rename is a single undo step
**And** the tree displays "Player"

### Story 2 — Create entity and press Escape to discard (Priority: P1)

As an editor user, I want to cancel entity creation if I change my mind about adding it.

**Given** the Scene Panel is showing any existing entities
**When** I right-click on an entity and select "Create Empty"
**Then** a new child entity is created and auto-rename starts
**When** I press Escape (instead of typing a name)
**Then** the entity is discarded (creation undone)
**And** the selection reverts to what it was before creation
**And** the undo stack has no entry for this creation (it was fully unwound)

### Story 3 — Single undo reverts create + rename (Priority: P1)

As an editor user, I want to undo both entity creation and its name in one step.

**Given** I created an entity named "AmmoPickup" via "Create Empty" and typed the name
**When** I press Ctrl+Z
**Then** the entity "AmmoPickup" is destroyed
**And** the selection is restored to what it was before creation
**When** I press Ctrl+Y (redo)
**Then** the entity is re-created with name "AmmoPickup"
**And** the selection is restored

### Story 4 — Auto-rename with empty name is accepted as unnamed (Priority: P2)

As an editor user, I want to create an entity without naming it (leave it unnamed) by confirming with an empty name.

**Given** a new entity is in auto-rename mode (InputText showing empty text)
**When** I press Enter (without typing anything)
**Then** the entity remains unnamed (rendered as "(unnamed)")
**And** the entity is NOT discarded
**And** no name is recorded in the CreateEntityCommand (undo still destroys the entity, redo creates it unnamed)
**And** the auto-rename state ends (tree node returns to normal)

### Story 5 — Auto-rename with focus loss confirms (Priority: P2)

As an editor user, I want to confirm the auto-rename by clicking elsewhere (same behavior as regular rename).

**Given** a new entity is in auto-rename mode (InputText showing empty text)
**When** I type "Bullet" and then click on another entity in the tree
**Then** the rename is confirmed (same as Enter)
**And** the entity is named "Bullet"
**And** the name is recorded in the CreateEntityCommand
**And** the click on the other entity processes normally (selects it)

### Story 6 — Auto-rename is overridden by F2 rename later (separate undo step) (Priority: P2)

As an editor user, I want auto-rename to behave as a single operation with create, while later renames are separate operations.

**Given** I created an entity named "Temp" via auto-rename
**When** I later press F2 on it and rename it to "Final"
**Then** "Temp" → "Final" is a separate undo step (RenameEntityCommand)
**And** Ctrl+Z first reverts "Final" back to "Temp"
**And** a second Ctrl+Z destroys the entity entirely

### Story 7 — Create after a regular rename is active (Priority: P2)

As an editor user, I want the pending rename to be confirmed first when I initiate a Create.

**Given** entity "Cube" is in rename mode (F2-initiated, partially edited to "Cub")
**When** I right-click empty area and select "Create Empty"
**Then** the pending rename on "Cube" is confirmed first (name changes to "Cub")
**And** then a new entity is created and enters auto-rename mode

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | "Create Empty" on an entity creates it as a child and auto-selects it. | Manual: right-click entity → "Create Empty" → new entity created as child, selected (highlighted), tree shows InputText. |
| AC-002 | "Create Empty" on empty area creates root entity and auto-selects it. | Manual: no selection → right-click empty area → root entity created, selected, InputText auto-focused. |
| AC-003 | After creation, the tree node shows an InputText widget with keyboard focus. | Manual: create entity → observe InputText appears with a blinking cursor. |
| AC-004 | Enter during auto-rename sets the entity name and ends rename mode. | Manual: create entity → type "Player" → Enter → tree shows "Player", InputText gone. |
| AC-005 | Escape during auto-rename discards the entity (creation undone). | Manual: create entity → press Escape → entity disappears from tree, selection returns to pre-create state. |
| AC-006 | Escape during auto-rename does NOT push a command onto the undo stack (no extra entry). | Manual: create → Escape → open Edit menu → "Undo" is disabled (or undo does nothing visible) assuming no prior action. Verify no stale entry. |
| AC-007 | Ctrl+Z after create+rename undoes both creation and naming in one step. | Manual: create entity → name it → Ctrl+Z → entity disappears, selection restored. Ctrl+Y → entity reappears with name. |
| AC-008 | Ctrl+Z after a later F2 rename only undoes the rename, not the creation. | Manual: create entity (name "A") → later F2 rename to "B" → Ctrl+Z → name reverts to "A". Second Ctrl+Z → entity destroyed. |
| AC-009 | Empty name on Enter during auto-rename leaves entity unnamed, not discarded. | Manual: create entity → press Enter without typing → entity stays, shows "(unnamed)", InputText gone. Undo destroys it. |
| AC-010 | Focus loss during auto-rename confirms the name (same as Enter). | Manual: create entity → type "Test" → click another entity → "Test" confirmed, other entity selected. |
| AC-011 | Create while regular rename is active confirms the pending rename first, then creates with auto-rename. | Manual: F2-rename "Cat" to "Dog" (don't press Enter yet) → right-click empty-area → "Create Empty" → "Cat" renamed to "Dog", new entity created with auto-rename. |
| AC-012 | Create while auto-rename is already active confirms the first auto-rename first, then creates a second. | Manual: create entity → type "First" → right-click empty area → "Create Empty" → "First" confirmed, second entity created with new auto-rename. |
| AC-013 | F2 and context menu "Rename" still push a separate RenameEntityCommand (no change). | Manual: create unnamed entity → confirm (Enter) → F2 → rename to "NewName" → Ctrl+Z → entity returns to unnamed (old name), not destroyed. |
| AC-014 | All existing entity operation tests still pass. | Run `buddd_tests` with `[editor][commands]` tags. |
| AC-015 | Zero new compiler warnings from `src/editor/` and `tests/`. | Build with `cmake --build --preset debug`. |

## E2E Verification

| Method | Description |
|---|---|
| **Headless unit test (CI)** | Build with `BUDDD_HAS_DISPLAY=OFF`. Run `buddd_tests`. Verify tagged tests pass: `[editor][commands][create]`, `[editor][commands][undo_redo]`, `[editor][auto_rename]` (if new tests are added). |
| **Manual smoke test (display)** | Run `buddd edit`. Test create+name flow, Escape discard, single undo step, interaction with F2 rename, empty name confirm. |
| **Clean build verification (CI)** | Run `cmake --build --preset debug` and verify zero warnings from `src/editor/` and `tests/`. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | User can create and name an entity in a single uninterrupted flow without extra clicks or keystrokes. | Manual: create entity → type name → Enter. Count steps: right-click → menu click → type → Enter. No extra selection or F2 step. |
| SC-002 | A mistaken creation can be discarded with a single Escape keystroke, with no lasting effect on the scene or undo stack. | Manual: create entity → Escape → entity gone, no undo stack entry for it. |
| SC-003 | Create+rename behaves as one undo step: one Ctrl+Z undoes both. | Manual: create+name "X" → Ctrl+Z → X gone. Ctrl+Y → X back with name. |

## Edge cases

| Case | Expected behavior |
|---|---|
| **Create entity with non-empty existing selection** | Selection replaced by new entity (auto-select). Previous selection lost but restorable via undo. |
| **Create entity with multi-select active** | Multi-select replaced by single selection containing the new entity. |
| **Create entity with rename active on another entity** | Pending rename confirmed first (via focus loss), then create with auto-rename. |
| **Create entity, type name, then Escape** | Escape is consumed by InputText. If Escape is pressed before name is typed (buffer empty), entity is discarded. If user typed a name first and then presses Escape, entity is discarded regardless of buffer contents. |
| **Create entity, type name, then click another entity** | Focus loss triggers confirm (same as Enter). Name is set and recorded. Click on other entity selects it. |
| **Create entity, type name, then click empty area** | Focus loss triggers confirm. Name is set. Empty-area click then clears selection (leaves new entity selected... actually empty-area click clears selection). The entity's name is set, selection cleared. |
| **Create entity, leave buffer empty, click empty area** | Focus loss triggers confirm with empty name. Entity stays unnamed. Empty-area click then clears selection. |
| **Rapid double-create (two "Create Empty" clicks in succession)** | First create triggers auto-rename. Second create confirms first rename (focus loss from first InputText), then creates second entity with auto-rename. Both entities exist, second is auto-renaming. |
| **Create entity, then close app before confirming or cancelling** | Unsaved state is handled by existing save-prompt. The entity exists unnamed (creation was already executed via command). No special handling needed. |
| **Create entity, press Escape (discard), then Ctrl+Z** | The create was undone at creation time. The undo stack has no entry for it. Ctrl+Z undoes whatever the previous operation was. |
| **Create entity, name it, undo (destroy), then create another entity** | Second creation is a fresh operation. The first entity's name is irrelevant (it's destroyed). No name collision issues. |

## Error cases

| Case | Expected behavior |
|---|---|
| **`Entity::set_name()` fails (engine error)** | No mechanism for failure — `set_name` is void. Assume always succeeds. |
| **Out-of-memory during entity creation** | `World::add_entity()` may throw `std::bad_alloc`. The command fails to execute and is not pushed to the undo stack. No auto-rename occurs. |
| **Selection restore with stale EntityIds** | Same as F-04 — entities that no longer exist remain in the selection set with no visual effect. |

## Permissions and security

- No changes to permissions or security posture.
- Entity creation and naming are in-memory operations only.
- Entity names are plain strings; no sanitisation is required.

## Observability

| Signal | Source |
|---|---|
| **Create+auto-rename started** | Debug log: `BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel", "Auto-rename started for entity {} post-creation", id.index)` |
| **Auto-rename confirmed with name** | Debug log: `BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel", "Auto-rename confirmed: entity {} named \"{}\"", id.index, name)` |
| **Auto-rename cancelled (Escape)** | Debug log: `BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel", "Auto-rename cancelled via Escape: entity {} discarded", id.index)` — followed by the existing command undo log. |
| **Existing confirm_rename changed** | The existing log for rename confirmation is unchanged for regular F2 renames. |
| **Entity created** | Existing debug log in `CreateEntityCommand::execute()` unchanged. |
| **Command undone** | Existing `BUDDD_LOG_DEBUG("[Command] ...: undone")` in `CommandStack::undo()`. |

## Documentation impact

| Document | Reason for update |
|---|---|
| `docs/wiki/editor/editor-panels.md` | The entity creation section currently states "Not auto-selected after creation". This must be updated to reflect the new auto-select + auto-rename behavior. The "Create Empty" flow description, selection post-creation behavior, and inline rename documentation should be revised. |
| `docs/wiki/editor/editor-panels.md` (Related specs) | Add a cross-reference to this spec (auto-rename-on-create) in the entity operations section. |

These updates are the responsibility of the implementation phase and will be tracked in the implementation contract.

## Out of scope

- Toolbar buttons for entity creation.
- Changes to F2 or context menu "Rename" behavior (still push `RenameEntityCommand`).
- Changes to Delete flow or confirmation dialog.
- Changes to entity name validation (empty/same-name rejection).
- Multi-entity operations.
- Changes to `CommandStack` or `Command` interfaces.
- Changes to how panels register with Editor.
- Drag-and-drop reparenting (deferred).
- Entity duplication or prefab operations.
- Component operations.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | The `CreateEntityCommand` can be modified to store a `post_creation_name_` optional string, exposed via a public setter. The command remains on the `CommandStack`'s internal vector long enough for the panel to hold a raw pointer to it (stable reference until evicted from 128-entry bounded stack). |
| A-02 | The existing `confirm_rename()` method can be extended to check whether it is operating in auto-rename mode (post-creation) vs. regular rename (F2-initiated). In auto-rename mode, it sets the name directly on the entity and calls `set_post_creation_name()` on the pending command, rather than pushing a new `RenameEntityCommand`. |
| A-03 | The deferred undo at end of `draw_ui()` (for Escape → discard) is safe: the entity is marked for destruction via `Entity::destroy()`, but not physically removed until `flush_destroyed()` in the next frame's `update()`. The entity tree has already been rendered for this frame. |
| A-04 | Auto-selection uses the same `EditorSelection::select(id, SelectionModifier::Replace)` mechanism as a plain click. The anchor is set to the new entity. |
| A-05 | The `create_entity_id` returned by `CreateEntityCommand` (via `created_entity_id_`) is stable and remains valid after `execute()` returns. The panel can use this ID to set the selection and initiate rename. |
| A-06 | Rename confirmation via focus loss (existing behavior) applies equally to auto-rename mode. The user clicking elsewhere triggers `IsItemDeactivatedAfterEdit()` or similar, which calls `confirm_rename()`. |
| A-07 | If the user confirms with an empty name during auto-rename, the entity stays unnamed. No name is recorded in the `CreateEntityCommand`. The `post_creation_name_` remains `std::nullopt`. On undo, the entity is destroyed (same as always). On redo, the entity is recreated with no name (same as original `CreateEntityCommand::execute()` without `post_creation_name_`). |

## Open questions

| ID | Question | Resolution |
|---|---|---|
| Q-01 | **Should empty-name confirm during auto-rename discard the entity?** The alternative is to keep the entity unnamed (no-op confirm). Keeping the entity unnamed matches existing behavior (regular rename rejects empty names). If the user wants to discard, they can press Escape. | **Keep entity unnamed.** Documented in Assumptions (A-07). |
