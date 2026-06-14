# SPEC-NNNN — Scene Panel Right-Click Selection

## Problem

When a user right-clicks on a non-selected entity in the Scene Panel, the entity is **not** selected. The context menu's "Delete" item operates on whatever was previously selected (which may be a different entity or empty), not on the entity the user right-clicked. This is confusing and error-prone — the user intuitively expects the context menu to operate on the entity under the cursor.

Additionally, right-clicking a selected entity does not change the selection (this is the correct behavior for operations like the context menu), but the current implementation has no distinction: **all** right-clicks leave selection unchanged, meaning "Delete" may refer to an unrelated entity if the user right-clicked something new.

## Goals

| ID | Goal |
|---|---|
| G-01 | **Right-click selects before context menu**: A plain right-click on a non-selected entity changes the selection to that entity (Replace) before the context menu appears. |
| G-02 | **Right-click on already-selected entity is a no-op**: If the right-clicked entity is already in the selection, the selection is unchanged. |
| G-03 | **Ctrl+right-click adds to selection**: Ctrl+right-click on a non-selected entity toggles it **into** the selection only (never removes). |
| G-04 | **Shift+right-click range-selects**: Shift+right-click on a non-selected entity selects the range from the anchor to the clicked entity (same as left-click Shift+click). |
| G-05 | **Never deselect on right-click**: No right-click + modifier combination should ever remove an entity from the selection. |
| G-06 | **Empty-area right-click unchanged**: Right-click on empty area still opens context menu with "Create Empty" only — no selection change. |
| G-07 | **Existing left-click selection behavior unchanged**: All left-click selection interactions (Replace, Toggle, Range, Ctrl+A) continue to work as specified in F-03. |
| G-08 | **Existing spec alignment**: F-03 (Entity Selection) NG-10 is removed and F-04 (Entity Operations) AC-32 is updated to reflect the new right-click behavior. |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | **No changes to left-click selection behavior** — left-click (plain, Ctrl, Shift) remains exactly as specified in F-03. |
| NG-02 | **No changes to context menu items** — the existing "Create Empty", "Delete", "Rename" items are unchanged. |
| NG-03 | **No changes to keyboard shortcuts** — Delete, F2, Enter, Escape, Ctrl+A, Ctrl+Z, Ctrl+Y all remain unchanged. |
| NG-04 | **No changes to inline rename behavior** — F2 and rename confirm/cancel are unchanged. |
| NG-05 | **No confirmation dialog changes** — delete confirmation dialog behavior is unchanged. |
| NG-06 | **No changes to the EditorSelection API** — `select(id, Replace)`, `select(id, Toggle)`, `set_selection()`, `clear()`, `contains()`, `anchor()` all remain the same. |
| NG-07 | **No new modifier keys** — only plain, Ctrl, and Shift are handled for right-click. |
| NG-08 | **No middle-click or other mouse button interactions** — only left and right mouse buttons are relevant. |
| NG-09 | **No changes to World or Entity classes** — engine APIs are consumed as-is. |

## Actors

| Actor | Description |
|---|---|
| **Editor user** | Opens the editor, selects entities in the Scene Panel, right-clicks to open the context menu. Expects "Delete" in the context menu to delete the entity they right-clicked on. |
| **Editor developer** | Implements the right-click selection logic in `ScenePanel::draw_ui()`. Ensures the selection change happens before the popup is opened. |

## User-visible behavior

### Right-click Selection Rules

| Input | Entity NOT in selection | Entity already in selection |
|---|---|---|
| **Plain right-click** | **Replace**: clear selection, select this entity (same as left-click). Anchor set to this entity. | **No-op**: selection unchanged. Anchor unchanged. |
| **Ctrl+right-click** | **Toggle-add only**: add entity to selection (if not already present). Anchor unchanged. Never removes. | **No-op**: selection unchanged. Anchor unchanged. |
| **Shift+right-click** | **Range**: select all entities from anchor to clicked entity in depth-first tree order (same as left-click Shift+click). If no anchor exists, degrade to Replace (select only clicked entity, set as anchor). | **No-op**: selection unchanged. Anchor unchanged. |
| **Empty-area right-click** | **No selection change**: same as current behavior — opens context menu with "Create Empty" only. | N/A |

### Key Design Principle

Right-click selection **always precedes** the context menu popup opening, so that the context menu "Delete" item operates on the selection that includes the right-clicked entity. The right-click selection logic runs before `ImGui::OpenPopup("scene_ctx")` and before the contextual menu items are evaluated.

### Summary of "Never Deselect on Right-click"

The following behaviors are explicitly **forbidden** on right-click:
- Removing an entity from the selection (Ctrl+right-click on a selected entity does nothing — it does NOT toggle it out).
- Clearing the selection (plain right-click on empty area does not clear selection).
- Changing the selection when the right-clicked entity is already selected.

## Impact on Existing Specs

### F-03 — Entity Selection (`.specs/sprint-2026-06/entity-selection/spec.md`)

The following changes must be made to the Entity Selection spec:

| Section | Change |
|---|---|
| **Non-goals — NG-10** | **Remove** the following entry: `NG-10 | No right-click behaviour for selection — right-click does not change selection (context menu deferred).` |
| **Out of scope** | Remove the bullet: `- Right-click selection behaviour (context menu deferred).` |

### F-04 — Entity Operations (`.specs/sprint-2026-06/entity-operations/spec.md`)

| Section | Change |
|---|---|
| **Acceptance criteria — AC-32** | **Update** from: `Context menu does not change selection — right-click alone does not select.` to: `Right-click on a non-selected entity selects it (Replace) before the context menu opens. Right-click on an already-selected entity does not change the selection.` |

## Key Entities

### ScenePanel — Right-click Handling Change

The right-click detection code in `ScenePanel::draw_ui()` (currently lines 99–106 in `scene_panel.cpp`) must be extended to perform selection changes **before** storing the `context_menu_entity_` and setting `open_context_menu = true`.

Current code (lines 99–106):
```cpp
// ── Detect right-click on this entity for shared context menu ──
if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel", "Right-click on entity {}", name);
        context_menu_entity_ = entity.id();
        open_context_menu = true;
    }
}
```

New code logic (pseudocode):
```cpp
// ── Detect right-click on this entity ──
if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel", "Right-click on entity {}", name);
        
        // Apply right-click selection BEFORE opening the context menu
        auto& selection = ctx.editor.selection();
        if (!selection.contains(entity.id())) {
            // Entity NOT in selection — apply modifier behavior
            if (ImGui::GetIO().KeyShift) {
                // Shift+right-click: range select (same as left-click Shift)
                auto anchor = selection.anchor();
                if (anchor.has_value()) {
                    auto range = collect_range(ctx.editor.world(), *anchor, entity.id());
                    selection.set_selection(range);
                } else {
                    // No anchor: degrade to Replace
                    selection.select(entity.id(), SelectionModifier::Replace);
                }
            } else if (ImGui::GetIO().KeyCtrl) {
                // Ctrl+right-click: toggle-add only (never remove)
                // Toggle modifier with Ctrl normally toggles in/out.
                // Here we want only add — but EditorSelection::select(id, Toggle)
                // will remove if already present. Since we checked !contains(id),
                // it will only add. So plain Toggle is safe here.
                selection.select(entity.id(), SelectionModifier::Toggle);
                // Note: this will add since we know entity is NOT in selection
            } else {
                // Plain right-click: replace selection
                selection.select(entity.id(), SelectionModifier::Replace);
            }
        }
        // If entity IS in selection: no-op (selection unchanged)
        
        context_menu_entity_ = entity.id();
        open_context_menu = true;
    }
}
```

**Key invariant**: The selection change must happen before `context_menu_entity_` is stored and before `open_context_menu = true`, because the context menu popup (`ImGui::BeginPopup("scene_ctx")`) evaluates `ctx.editor.selection().empty()` for the "Delete" menu item's enabled state. If we changed selection after the context menu displays, the Delete item would still reference the old selection.

### No other code changes

- No changes to `EditorSelection` API.
- No changes to `Editor` or `EditorContext`.
- No changes to context menu rendering or menu items.
- No changes to left-click selection handling (lines 108–134).
- No changes to empty-area right-click (lines 157–161).

## User stories

### Story 1 — Right-click on non-selected entity selects it before context menu (Priority: P1)

As an editor user, I want to right-click a non-selected entity and have the "Delete" menu item delete that entity, so that the context menu always operates on the entity I clicked.

**Given** the Scene Panel shows entities "Player" (not selected) and "Camera" (selected)
**When** I right-click "Player"
**Then** "Player" becomes selected (highlighted)
**And** "Camera" is no longer highlighted
**And** the context menu appears with "Delete" enabled
**When** I click "Delete"
**Then** "Player" is marked for destruction

**Given** the Scene Panel shows entities "Player" and "Camera"
**And** neither entity is selected (selection is empty)
**When** I right-click "Player"
**Then** "Player" becomes selected
**And** the context menu appears with "Delete" enabled
**When** I click "Delete"
**Then** "Player" is marked for destruction

### Story 2 — Right-click on already-selected entity does not change selection (Priority: P1)

As an editor user, I want to right-click on an entity that is already selected without changing the selection, so that I can open the context menu without altering my multi-select state.

**Given** "Player" and "Camera" are both selected (multi-select)
**When** I right-click "Player" (which is already selected)
**Then** both "Player" and "Camera" remain selected
**And** the context menu appears with "Delete" enabled

**Given** "Player" is selected (single selection)
**When** I right-click "Player"
**Then** "Player" remains selected
**And** `EditorSelection::size()` remains 1

### Story 3 — Ctrl+right-click adds entity to selection (Priority: P1)

As an editor user, I want to Ctrl+right-click a non-selected entity to add it to my existing selection, so that I can build a multi-select while also opening the context menu.

**Given** "Player" is selected
**When** I Ctrl+right-click "Camera" (not selected)
**Then** both "Player" and "Camera" are selected
**And** `EditorSelection::size()` is 2
**And** the context menu appears

**Given** "Player" and "Camera" are both selected
**When** I Ctrl+right-click "Player" (already selected)
**Then** both "Player" and "Camera" remain selected (no-op)
**And** `EditorSelection::size()` remains 2

**Given** "Player" is selected
**When** I Ctrl+right-click "Camera" (not selected), then Ctrl+right-click "Camera" again
**Then** both "Player" and "Camera" remain selected after the second right-click
**And** `EditorSelection::size()` remains 2 (entity was NOT removed)

### Story 4 — Shift+right-click range-selects (Priority: P2)

As an editor user, I want to Shift+right-click to select a range and open the context menu, so that I can quickly delete a contiguous group of entities.

**Given** the Scene Panel shows entities in depth-first order: "RootA", "ChildA1", "ChildA2", "RootB", "RootC"
**And** "RootA" is currently selected (anchor = "RootA")
**When** I Shift+right-click "RootC"
**Then** all 5 entities are selected ("RootA", "ChildA1", "ChildA2", "RootB", "RootC")
**And** the context menu appears
**And** "Delete" is enabled

**Given** no entities are selected (no anchor)
**When** I Shift+right-click "Player"
**Then** only "Player" is selected (degrade to Replace)
**And** the anchor is set to "Player"

### Story 5 — Right-click on entity in empty selection (Priority: P2)

As an editor user, I want to right-click an entity even when nothing is selected, so that I can start working immediately.

**Given** selection is empty
**When** I right-click "Player"
**Then** "Player" becomes selected
**And** the context menu appears with "Delete" enabled

### Story 6 — Empty-area right-click unchanged (Priority: P2)

As an editor user, I want to right-click empty space and see only "Create Empty" in the context menu, without any selection change.

**Given** "Player" is selected
**When** I right-click empty area in the Scene Panel
**Then** "Player" remains selected
**And** the context menu shows only "Create Empty"

**Given** no entities are selected
**When** I right-click empty area
**Then** the selection remains empty
**And** the context menu shows only "Create Empty"

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-01 | Plain right-click on non-selected entity selects it (Replace) — entity becomes selected, previous selection cleared, anchor set. | Manual: select "Camera", right-click "Player" (not selected), verify "Player" highlighted, "Camera" not highlighted, context menu appears |
| AC-02 | Plain right-click on already-selected entity does not change selection. | Manual: select "Player", right-click "Player", verify "Player" still selected, anchor unchanged |
| AC-03 | Plain right-click on entity when selection is empty selects it. | Manual: clear selection, right-click "Player", verify "Player" selected |
| AC-04 | Ctrl+right-click on entity not in selection adds it to selection. | Manual: select "Player", Ctrl+right-click "Camera", verify both selected |
| AC-05 | Ctrl+right-click on entity already in selection is a no-op (does not remove it). | Manual: select "Player", Ctrl+right-click "Player", verify "Player" still selected |
| AC-06 | Ctrl+right-click on entity not in selection, then Ctrl+right-click same entity again — second click is a no-op (entity not removed). | Manual: select "Player", Ctrl+right-click "Camera", Ctrl+right-click "Camera" again, verify both still selected |
| AC-07 | Shift+right-click on entity not in selection performs range selection (same as left-click Shift+click). | Manual: select "RootA", Shift+right-click "RootC", verify entire range selected |
| AC-08 | Shift+right-click with no anchor degrades to Replace (select only clicked entity). | Manual: clear selection, Shift+right-click "Player", verify only "Player" selected |
| AC-09 | Shift+right-click on entity already in selection is a no-op. | Manual: select "RootA" and "RootB", Shift+right-click "RootA", verify both still selected |
| AC-10 | Empty-area right-click does not change selection. | Manual: select "Player", right-click empty area, verify "Player" still selected, context menu shows only "Create Empty" |
| AC-11 | Right-click selection happens before context menu opens — "Delete" in context menu refers to the right-clicked entity (not previous selection). | Manual: select "Camera", right-click "Player", click "Delete", verify "Player" is deleted (not "Camera") |
| AC-12 | Context menu "Delete" is enabled after right-click on an entity (selection is non-empty). | Manual: right-click any entity, verify "Delete" is not greyed out |
| AC-13 | Context menu "Rename" is enabled after right-click on an entity (single entity selected). | Manual: right-click any entity when only that entity is selected, verify "Rename" enabled |
| AC-14 | Left-click selection behavior is completely unchanged (non-regression). | Manual: verify plain left-click, Ctrl+click, Shift+click, Ctrl+A, empty-area left-click all work as before |
| AC-15 | All existing tests still pass. | Run `buddd_tests` |
| AC-16 | Zero new warnings from `src/editor/`. | Build with `cmake --build --preset debug` |

## E2E Verification

| Method | Description |
|---|---|
| **Manual smoke test (display)** | Run `buddd edit` with a scene loaded. Verify each right-click + modifier combination in the user-visible behavior table. Verify that "Delete" removes the right-clicked entity (not the previously selected one). Verify all left-click interactions still work as expected. |
| **Clean build verification (CI)** | Run `cmake --build --preset debug` and verify zero new warnings. Run `buddd_tests` and verify all tests pass. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | Right-clicking any non-selected entity immediately selects it before the context menu appears — "Delete" always refers to the entity the user right-clicked. | Manual: right-click entity A with entity B selected, click Delete, verify A is deleted. |
| SC-002 | Right-clicking a selected entity never changes the selection (multi-select preserved). | Manual: multi-select 3 entities, right-click one of them, verify all 3 still selected. |
| SC-003 | Ctrl+right-click and Shift+right-click never deselect. | Manual: Ctrl+right-click a selected entity — it stays selected. Ctrl+right-click twice — entity remains selected. |
| SC-004 | No regressions in left-click selection or entity operations. | Run full test suite. |

## Edge cases

| Case | Expected behavior |
|---|---|
| **Right-click on entity when selection is empty** | Entity becomes selected (Replace). Context menu appears with "Delete" enabled. |
| **Right-click on the single selected entity (only one selected)** | No-op — selection unchanged. Context menu appears with "Delete" and "Rename" enabled. |
| **Right-click on entity that is one of many in multi-select** | No-op — all entities remain selected. Context menu appears with "Delete" enabled, "Rename" disabled (multi-select). |
| **Ctrl+right-click on entity not in selection, when multiple entities are already selected** | Entity is added to the multi-select. Context menu appears. |
| **Shift+right-click when anchor equals clicked entity** | Range of length 1 — only that entity selected. |
| **Shift+right-click with no anchor, entity not in selection** | Degrade to Replace — clicked entity selected, set as anchor. |
| **Right-click on entity while inline rename is active on a different entity** | The right-click selection and context menu opening happen first. If the rename InputText had focus, right-click may close it (ImGui default behavior for popup). The selection change is evaluated after the rename InputText loses focus. No special handling needed — standard ImGui behavior. |
| **Right-click on entity while confirmation dialog is open** | Modal dialog captures input — right-click on the panel behind is not processed until the modal is dismissed. |
| **Rapid right-clicks on different entities** | Each right-click independently selects the clicked entity (Replace). Standard frame-by-frame processing. |
| **Right-click on entity, then immediately left-click on empty area before context menu interaction** | Left-click on empty area closes the popup and clears selection (standard ImGui behavior). The right-click selection was already applied; clearing it afterward is the expected cascade. |
| **Right-click on entity with 10,000+ other entities selected** | Right-click on a non-selected entity replaces the entire large selection with just this entity. Clear + select one is O(1) for EditorSelection. Range select is O(n) — same as left-click. |
| **Right-click on entity, then press Delete key without interacting with context menu** | The right-click selection is already applied. Delete key checks `!selection.empty()` and executes deletion. This is valid — the user right-clicked to select and pressed Delete without using the menu. |
| **Right-click on entity that is pending-destroy (should not happen)** | Not possible — entities pending destroy are not rendered in the tree in subsequent frames. |
| **Context menu appears but user clicks outside (dismiss)** | Right-click selection persists. No undo of the selection change. This is consistent with left-click selection behavior — selection changes are not undoable. |
| **Right-click on entity while F2 rename is active on the same entity** | During inline rename, the tree node label is replaced by an InputText. Right-click on an InputText may trigger ImGui's default text context menu. The Scene Panel's `IsItemHovered` check on the entity tree node will not fire (because the node is not rendered). Right-click during rename is handled by ImGui's default InputText context menu, which does nothing destructive. |

## Error cases

| Case | Expected behavior |
|---|---|
| **Right-click on entity that is destroyed between frames** | Not possible — entity tree nodes only render live entities. |
| **`collect_range()` called with anchor or clicked entity not in the tree** | `collect_range()` returns `{clicked}` (single entity). This is the same defensive guard as left-click behavior. |
| **`EditorSelection::select()` called with `EntityId::none()`** | Ignored — no-op (defensive guard in `EditorSelection`). Not expected in practice since right-click only fires on valid entities. |

## Permissions and security

- No changes to permissions or security posture.
- Selection is entirely in-memory state — no file I/O, no network access, no sensitive data.
- No authentication or authorisation boundaries are crossed.

## Observability

| Signal | Source |
|---|---|
| **Right-click on entity** | Existing debug log: `BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel", "Right-click on entity {}", name)` — already present on line 102 of `scene_panel.cpp`. |
| **Right-click selection change** | Add debug log after right-click selection mutation: `BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel", "Right-click selection: {} selected entity {} (modifier={})", modifier_string, id.index, action)` — where action is "Replace", "Toggle-add", "Range", or "No-op (already selected)". |
| **Empty-area right-click** | Existing debug log: `BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel", "Empty-area right-click")` — already present on line 159 of `scene_panel.cpp`. |

## Out of scope

- Changes to left-click selection behavior.
- Changes to context menu items (Create Empty, Delete, Rename).
- Changes to keyboard shortcut handling (Delete, F2, Ctrl+A, Ctrl+Z, Ctrl+Y).
- Changes to inline rename behavior.
- Changes to delete confirmation dialog behavior.
- Changes to the EditorSelection API or Selection class.
- Changes to World, Entity, or engine APIs.
- New modifier keys or mouse buttons for selection.
- Selection persistence across editor sessions.
- Undo for selection changes (selection is not undoable — consistent with F-03).

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | The `EditorSelection::contains(id)` method exists and correctly reports whether an entity is in the current selection. Confirmed by F-03 spec. |
| A-02 | The `EditorSelection::select(id, Replace)` clears selection, adds the entity, and sets the anchor. Confirmed by F-03 spec. |
| A-03 | The `EditorSelection::select(id, Toggle)` adds the entity if not present, removes it if present. Confirmed by F-03 spec. When we use it after confirming `!contains(id)`, the Toggle will only add (never remove). |
| A-04 | The `EditorSelection::set_selection(span)` replaces the entire selection with the given list and does not change the anchor. Confirmed by F-03 spec. |
| A-05 | The `ScenePanel::collect_range()` helper exists and returns a depth-first ordered range. Confirmed by existing code (lines 386–408). |
| A-06 | `ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)` and `ImGui::IsMouseReleased(ImGuiMouseButton_Right)` are the correct pattern for right-click detection. Confirmed by existing code (lines 100–101). |
| A-07 | The right-click selection code runs before `ImGui::OpenPopup("scene_ctx")` because both are in the same `draw_ui()` call and the selection change happens in the right-click detection block (lines 99–106) which is evaluated before the deferred popup open (lines 163–166). |
| A-08 | `ImGui::GetIO().KeyCtrl` and `ImGui::GetIO().KeyShift` are available and correctly report modifier key state during right-click release. Confirmed by existing left-click code. |
| A-09 | The context menu's "Delete" item reads `ctx.editor.selection().empty()` at the time the popup is rendered (same frame as the right-click), which will reflect the updated selection because the right-click selection change happened earlier in the same frame. |
| A-10 | The existing F-03 and F-04 specs will be updated to reflect the new right-click behavior as part of this feature's implementation. These updates are documented in the "Impact on Existing Specs" section above. |

## Open questions

| ID | Question | Resolution |
|---|---|---|
| Q-01 | **What if `EntityId::none()` is right-clicked?** The Scene Panel only renders entities with valid `EntityId`. Empty-area right-click is handled separately and sets `context_menu_entity_ = EntityId::none()`. Right-click on a valid entity always produces a valid `EntityId`. The `select()` call is guarded by the `contains()` check — if somehow an invalid id is passed, `EditorSelection::select()` should ignore it (defensive guard already documented in F-03). | **No clarification needed.** |
| Q-02 | **Should right-click selection be undoable?** No — selection changes are not undoable (consistent with F-03 NG-04). The right-click selection change is a UI navigation action, not a data mutation. | **No clarification needed.** |
| Q-03 | **Should the anchor be updated on plain right-click Replace?** Yes — plain right-click on a non-selected entity is semantically equivalent to left-click Replace: it clears the selection, selects the new entity, and sets the anchor. This is consistent with how Shift+click range depends on the anchor. | **No clarification needed.** |
