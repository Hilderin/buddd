# IMPL-2026-06-scene-panel-right-click — Scene Panel Right-Click Selection

## Source spec

`.specs/sprint-2026-06/scene-panel-right-click/spec.md`

## Goal

Modify the right-click detection block in `ScenePanel::draw_ui()` to apply selection changes **before** opening the context menu. Specifically: if the right-clicked entity is NOT already in the selection, apply the same modifier-aware selection logic as left-click (Replace for plain, Toggle for Ctrl, Range for Shift). If the entity IS already in the selection, do nothing (no-op). This ensures the context menu's "Delete" item always operates on the entity the user right-clicked, while preserving the invariant that right-click never deselects.

## Non-goals

- No changes to left-click selection behavior (lines 108–134 of `scene_panel.cpp`).
- No changes to `EditorSelection` API, `Selection`, `SelectionModifier`, or any method signatures in `editor_selection.h`.
- No changes to context menu rendering, menu items, or popup enable/disable logic (lines 168–191 of `scene_panel.cpp`).
- No changes to empty-area right-click (lines 156–161 of `scene_panel.cpp`).
- No changes to keyboard shortcuts (Delete, F2, Ctrl+A, Ctrl+Z, Ctrl+Y).
- No changes to inline rename behavior.
- No changes to delete confirmation dialog.
- No changes to `Editor`, `EditorContext`, `World`, `Entity`, or any engine APIs.
- No new dependencies, no new files, no new public API surface.

## Relevant ADRs

| ADR | Relevance |
|---|---|
| **ADR-027** (Editor Architecture) | The editor is a static library in `src/editor/`. This contract modifies only `scene_panel.cpp` within that library. No architecture boundaries are crossed. |
| **ADR-026** (ImGui Integration) | The right-click detection uses `ImGui::IsItemHovered` / `ImGui::IsMouseReleased` per existing convention. No changes to the ImGui integration layer. |
| **ADR-029** (Editor UX Decisions) | Entity selection flow is defined here. Right-click selection extends the existing left-click multi-select UX with the invariant that right-click never deselects. |

No other ADRs constrain this implementation.

## Files to inspect

| File | Purpose |
|---|---|
| `src/editor/panels/scene_panel.cpp` | The file to modify. Lines 99–106 contain the right-click detection block to replace. Lines 108–134 contain the left-click selection logic (pattern reference). Lines 386–408 contain `collect_range()` (used as-is). |
| `src/editor/panels/scene_panel.h` | Verify no new members or methods are needed (none are). |
| `src/editor/editor_selection.h` | Verify the API surface: `contains(EntityId)`, `select(EntityId, SelectionModifier)`, `set_selection(span)`, `anchor()`. All used as-is. |
| `.specs/sprint-2026-06/entity-selection/spec.md` | Reference for existing left-click selection behavior and the `EditorSelection` API contract. |
| `tests/editor/entity_selection_tests.cpp` | Reference for testing patterns (not modified by this contract). |

## Files allowed to change

| File | Allowed changes |
|---|---|
| `src/editor/panels/scene_panel.cpp` | **Only lines 99–106** (the right-click detection block). Replace the entire block with new code that includes selection logic before setting `context_menu_entity_` and `open_context_menu`. |
| `.specs/sprint-2026-06/entity-selection/spec.md` | Remove NG-10 from the Non-goals section. Remove the "Right-click selection behaviour (context menu deferred)." bullet from the Out of scope section. |
| `.specs/sprint-2026-06/entity-operations/spec.md` | Update AC-32 text from "Context menu does not change selection — right-click alone does not select." to "Right-click on a non-selected entity selects it (Replace) before the context menu opens. Right-click on an already-selected entity does not change the selection." |

All other files are forbidden to change.

## Files forbidden to change

- `src/editor/editor_selection.h` — No API changes per NG-06.
- `src/editor/editor_selection.cpp` — No implementation changes.
- `src/editor/editor.h` / `src/editor/editor.cpp` — No editor lifecycle changes.
- `src/editor/editor_context.h` — No context changes.
- `src/editor/panels/scene_panel.h` — No header changes (no new members, no new methods).
- `src/editor/editor_panel.h` — No base class changes.
- `src/cmd/apps/editor_app.h` / `editor_app.cpp` — No app lifecycle changes.
- `src/engine/**` — No engine changes.
- `tests/**` — No test changes (the feature is manually verified per spec ACs). If automated tests are added, they must be in a new file `tests/editor/scene_panel_right_click_tests.cpp`.
- `docs/wiki/**` — No wiki changes unless the `Entity Selection` wiki page documents right-click behavior, in which case the wiki page may be updated to document right-click selection.

## Existing conventions to follow

1. **Coding style**: `snake_case` for variables and functions, `PascalCase` for classes and enums, `UPPER_CASE` for enum values. Braces on the same line as control flow. Use `auto` consistently.
2. **Debug logging**: All editor debug logs use `BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel", "message")`. Follow the same pattern for any new debug log lines.
3. **Selection modifier pattern**: The left-click block (lines 108–134) uses `ImGui::GetIO().KeyShift`, `ImGui::GetIO().KeyCtrl`, `SelectionModifier::Replace`, `SelectionModifier::Toggle`, `selection.select()`, `selection.set_selection()`, `collect_range()`, and `selection.anchor()` — the right-click block must follow the same API usage pattern.
4. **Anchor handling**: `SelectionModifier::Replace` sets the anchor internally. `SelectionModifier::Toggle` leaves anchor unchanged. `selection.set_selection(range)` leaves anchor unchanged. Do not manually set/reset anchor in the right-click block.
5. **Defensive guard**: `EditorSelection::select()` already ignores `EntityId::none()` — no need to guard against it in the right-click block. The right-click only fires on valid entity rows.
6. **ImGui order**: The right-click detection (lines 99–106) is inside the `else { bool expanded = ImGui::TreeNodeEx(...); ... }` branch (the non-renaming branch). It's after `TreeNodeEx` creates the item, so `IsItemHovered` works correctly.

## Required implementation behavior

### Exact code change in `scene_panel.cpp` (lines 99–106)

**BEFORE** (current code):

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

**AFTER** (replace with):

```cpp
                // ── Detect right-click on this entity for shared context menu ──
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
                                // We already know entity is NOT in selection (the !contains() check above),
                                // so Toggle will only add, never remove.
                                selection.select(entity.id(), SelectionModifier::Toggle);
                            } else {
                                // Plain right-click: replace selection
                                selection.select(entity.id(), SelectionModifier::Replace);
                            }
                        }
                        // If entity IS in selection: no-op (selection unchanged)

                        BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel", "Right-click selection: {}",
                            selection.contains(entity.id()) ? "set" : "unchanged");

                        context_menu_entity_ = entity.id();
                        open_context_menu = true;
                    }
                }
```

### Behavioral specification

| Condition | Behavior |
|---|---|
| **Plain right-click on non-selected entity** | `selection.select(entity.id(), SelectionModifier::Replace)` — clears selection, selects entity, sets anchor. |
| **Ctrl+right-click on non-selected entity** | `selection.select(entity.id(), SelectionModifier::Toggle)` — adds entity to selection (Toggle would normally also remove if present, but the `!contains()` guard means it will only add). Anchor unchanged. |
| **Shift+right-click on non-selected entity, anchor exists** | `selection.set_selection(collect_range(world, *anchor, entity.id()))` — selects depth-first range. Anchor unchanged. |
| **Shift+right-click on non-selected entity, no anchor** | Degrade to `selection.select(entity.id(), SelectionModifier::Replace)` — selects only clicked entity (acts as new anchor). |
| **Any right-click on already-selected entity** | No-op — the entire `if (!selection.contains(entity.id()))` block is skipped. The context menu still opens. |
| **Empty-area right-click** | Unchanged — code at lines 156–161 handles this separately. Sets `context_menu_entity_ = EntityId::none()`, no selection change. |
| **Alt/Super/other modifier keys** | Fall through to the else branch (plain right-click → Replace). Same as left-click behavior for unhandled modifiers. |

### Invariant enforcement

The selection change **must** happen before `context_menu_entity_ = entity.id()` and `open_context_menu = true`, because:
- The context menu (line 182) evaluates `ctx.editor.selection().empty()` for the "Delete" item's enabled state.
- The selection change must be visible to the popup rendering that happens later in the same `draw_ui()` call (lines 163–191).

### Edge cases carried forward from spec

See `Edge cases` section below.

## Required tests

No automated unit tests are required for this contract. All 16 acceptance criteria in the spec are verified manually (see spec AC-01 through AC-16). The existing test suite must continue to pass (AC-15).

### Manual test checklist (mapped to spec acceptance criteria)

| Spec AC | Manual test step |
|---|---|
| AC-01 | Select "Camera". Right-click "Player" (not selected). Verify Player becomes selected (highlighted), Camera is not highlighted. |
| AC-02 | Select "Player". Right-click "Player". Verify Player remains selected (no change). |
| AC-03 | Clear selection. Right-click "Player". Verify Player becomes selected. |
| AC-04 | Select "Player". Ctrl+right-click "Camera" (not selected). Verify both selected. |
| AC-05 | Select "Player". Ctrl+right-click "Player". Verify Player still selected. |
| AC-06 | Select "Player". Ctrl+right-click "Camera", then Ctrl+right-click "Camera" again. Verify both still selected. |
| AC-07 | Select "RootA". Shift+right-click "RootC". Verify entire range selected. |
| AC-08 | Clear selection. Shift+right-click "Player". Verify only "Player" selected. |
| AC-09 | Select "RootA" and "RootB". Shift+right-click "RootA". Verify both still selected. |
| AC-10 | Select "Player". Right-click empty area. Verify Player still selected, context menu shows only "Create Empty". |
| AC-11 | Select "Camera". Right-click "Player". Click "Delete" in context menu. Verify Player is deleted (not Camera). |
| AC-12 | Right-click any entity. Verify "Delete" is not greyed out. |
| AC-13 | Right-click any entity (single selection). Verify "Rename" is enabled. |
| AC-14 | Verify plain left-click, Ctrl+click, Shift+click, Ctrl+A, empty-area left-click all still work. |
| AC-15 | Run `buddd_tests` — all pass. |
| AC-16 | Build with `cmake --build --preset debug` — zero new warnings in `src/editor/`. |

## Edge cases

From the spec (all must be handled correctly):

| Edge case | Expected behavior | How the contract handles it |
|---|---|---|
| Right-click on entity when selection is empty | Entity becomes selected (Replace) | `!contains(id)` → plain else → `select(id, Replace)` |
| Right-click on the single selected entity (only one selected) | No-op — selection unchanged | `contains(id)` → skip entire block |
| Right-click on entity that is one of many in multi-select | No-op — all entities remain selected | `contains(id)` → skip entire block |
| Ctrl+right-click on entity not in selection, multiple entities already selected | Entity added to multi-select | `!contains(id) && KeyCtrl` → `select(id, Toggle)` adds it |
| Shift+right-click when anchor equals clicked entity | Range of length 1 — only that entity selected | `collect_range()` returns `{anchor}` (anchor == clicked, range is 1 element) |
| Shift+right-click with no anchor, entity not in selection | Degrade to Replace | `anchor.has_value()` is false → `select(id, Replace)` |
| Right-click on entity while inline rename is active on a different entity | No special handling — ImGui's ItemHovered will not fire on renamed node (it's an InputText, not a TreeNode) | No code change needed — right-click detection is inside `else` (non-renaming) block |
| Right-click on entity while confirmation dialog open | Modal captures input — right-click not processed | No code change needed — ImGui modal isolation |
| Rapid right-clicks on different entities | Each selects independently (Replace) | Each frame evaluates independently |
| Right-click on entity, then left-click empty area | Left-click clears selection normally | Cascade of right-click selection then left-click clear |
| Right-click on entity with 10K+ entities selected | Replace clears (O(1)), Range is O(n) | Using same `set_selection` and `collect_range` as left-click |
| Right-click on entity, press Delete without interacting with context menu | Delete executes on right-click-selected entity | Selection was already set before context menu appeared |
| Context menu dismissed without interaction | Right-click selection persists | No undo of selection — consistent with left-click behavior |
| `collect_range()` called with anchor or clicked entity not in the tree | Returns `{clicked}` (defensive fallback) | `collect_range()` already has this guard — no change needed |
| `EditorSelection::select()` called with `EntityId::none()` | Ignored — no-op | `EditorSelection::select()` already has this guard. Right-click only fires on valid entity rows, so this should never happen. |

## Security impact

None. Selection is entirely in-memory state — no file I/O, no network access, no sensitive data. No authentication or authorisation boundaries are crossed. No input validation issues — the `EntityId` comes from the entity tree iteration, not from external input.

## Data and migration impact

None. No schema changes, no data migrations, no seed data changes, no data loss risks.

## API compatibility impact

**No API changes.** The `EditorSelection` API is unchanged (confirmed by NG-06). The `ScenePanel` public interface is unchanged (no new methods or members). The `EditorContext` is unchanged.

Downstream consumers of `ctx.editor.selection()` (Inspector panel, Viewport, future panels) continue to work without modification — they already observe the selection state, which will now reflect right-click actions.

## Documentation impact

- **Wiki page `docs/wiki/editor/entity-selection.md`**: The "Multi-Select Interactions" table currently documents only left-click interactions. It should be updated to include a row for right-click interactions, documenting the same modifier-aware behavior with the "never deselect" invariant. Or, a separate "Right-click Selection" subsection should be added. The wiki-agent will handle this.
- **Specs to update** (the Code Agent must apply these changes):
  - `.specs/sprint-2026-06/entity-selection/spec.md`:
    - Remove NG-10 from Non-goals section: `NG-10 | No right-click behaviour for selection — right-click does not change selection (context menu deferred).`
    - Remove "Right-click selection behaviour (context menu deferred)." from the Out of scope section.
  - `.specs/sprint-2026-06/entity-operations/spec.md`:
    - Update AC-32 from: `"Context menu does not change selection — right-click alone does not select."` to: `"Right-click on a non-selected entity selects it (Replace) before the context menu opens. Right-click on an already-selected entity does not change the selection."`

## ADR impact

None. No existing ADR is contradicted or amended by this change. Selection is already documented as non-undoable (ADR-029), and the right-click selection follows the same rules as left-click with the additional "never deselect" invariant. No new ADR is warranted.

## Done criteria

The implementation is complete when all of the following are verifiable:

- [ ] **DC-1** — `src/editor/panels/scene_panel.cpp` lines 99–106 have been replaced with the new right-click selection logic (the exact code block in "Required implementation behavior" above).
- [ ] **DC-2** — The right-click selection change happens BEFORE `context_menu_entity_` and `open_context_menu` are set. Verified by reading lines ~99–138 of `scene_panel.cpp`.
- [ ] **DC-3** — `BUDDD_LOG_TAGGED_DEBUG` for right-click selection is present after the selection block.
- [ ] **DC-4** — `.specs/sprint-2026-06/entity-selection/spec.md` has NG-10 removed from the Non-goals section and "Right-click selection behaviour" removed from Out of scope.
- [ ] **DC-5** — `.specs/sprint-2026-06/entity-operations/spec.md` has AC-32 updated with the new text.
- [ ] **DC-6** — All 16 manual acceptance criteria (AC-01 through AC-16) from the spec pass when tested manually. At minimum, the developer confirms AC-01, AC-02, AC-05, AC-10, and AC-11 are working.
- [ ] **DC-7** — `cmake --build --preset debug` produces zero new warnings from `src/editor/`.
- [ ] **DC-8** — `buddd_tests` passes (all existing tests still pass).
- [ ] **DC-9** — No changes to any file outside the allowed list (see "Files allowed to change").
