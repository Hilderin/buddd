# Implementation Contract Review — Scene Panel Right-Click Selection

## Verdict: **approved**

## Summary

The implementation satisfies all review criteria. The right-click detection block in `ScenePanel::draw_ui()` correctly applies selection changes (Replace, Toggle-add, Range, or No-op) **before** opening the context menu, enforcing the "never deselect on right-click" invariant. Only the 3 allowed files were modified. The build produces zero new warnings, all 697 existing tests pass, and visual verification confirms the editor renders correctly. The F-03 and F-04 spec updates (NG-10 removal, AC-32 update) match the spec requirements exactly.

## Review checks

### 1. Right-click selection logic matches spec

| Input | Entity NOT in selection | Entity already in selection |
|---|---|---|
| **Plain right-click** | `select(entity.id(), SelectionModifier::Replace)` ✅ | No-op (skips entire `if (!contains())` block) ✅ |
| **Ctrl+right-click** | `select(entity.id(), SelectionModifier::Toggle)` — guarded by `!contains()`, so only adds ✅ | No-op ✅ |
| **Shift+right-click (anchor exists)** | `set_selection(collect_range(...))` ✅ | No-op ✅ |
| **Shift+right-click (no anchor)** | Degrade to `select(entity.id(), SelectionModifier::Replace)` ✅ | No-op ✅ |
| **Empty-area right-click** | Unchanged (separate code block at lines 185–190) ✅ | N/A |

**Result: PASS**

### 2. Never deselect on right-click invariant

- The `if (!selection.contains(entity.id()))` guard ensures that any right-click on an already-selected entity is a no-op.
- Ctrl+right-click uses `SelectionModifier::Toggle`, which would normally also remove if the entity were already selected. But the `!contains()` guard ensures Toggle is only called when the entity is NOT in the selection, so it can only add.
- No code path removes an entity from selection on right-click.

**Result: PASS**

### 3. Selection change happens BEFORE context menu popup

The selection logic (lines 104–130) executes before both:
- `context_menu_entity_ = entity.id();` (line 132)
- `open_context_menu = true;` (line 133)

The deferred `ImGui::OpenPopup("scene_ctx")` (line 194) runs later in the same `draw_ui()` call, so the updated selection is visible to the context menu's "Delete" item enabled check (`!ctx.editor.selection().empty()` on line 211).

**Result: PASS**

### 4. Uses existing APIs correctly

- `EditorSelection::contains(entity.id())` ✅
- `EditorSelection::select(entity.id(), SelectionModifier::Replace/Toggle)` ✅
- `EditorSelection::set_selection(range)` ✅
- `EditorSelection::anchor()` ✅
- `ScenePanel::collect_range(world, *anchor, entity.id())` ✅

All APIs are used with the same signatures and semantics as the existing left-click block.

**Result: PASS**

### 5. No changes to left-click behavior

Lines 137–163 (left-click handling) are completely unchanged.

**Result: PASS**

### 6. No changes to empty-area right-click behavior

Lines 185–190 (empty-area right-click) are completely unchanged.

**Result: PASS**

### 7. No changes to engine, Entity, or World classes

Only `src/editor/panels/scene_panel.cpp` was modified (plus two spec files). No engine headers, no Entity/World classes, no `EditorSelection` API were touched.

**Result: PASS**

### 8. F-03 spec update: NG-10 removal

`git diff` confirms:
- `NG-10 | No right-click behaviour for selection — right-click does not change selection (context menu deferred).` **removed** from Non-goals ✅
- `NG-11 | No changes to World or Entity classes — engine APIs are consumed as-is.` **renumbered to NG-10** ✅
- `- Right-click selection behaviour (context menu deferred).` **removed** from Out of scope section ✅

**Result: PASS**

### 9. F-04 spec update: AC-32 update

`git diff` confirms AC-32 changed from:
> `Context menu does not change selection — right-click alone does not select.`

To:
> `Right-click on a non-selected entity selects it (Replace) before the context menu opens. Right-click on an already-selected entity does not change the selection.`

**Result: PASS**

### 10. Builds without warnings

```
cmake --build --preset debug
```
Completed successfully with zero warnings from `src/` or `tests/`. All warnings (if any) are from `_deps/` dependencies only.

**Result: PASS**

### 11. All existing tests pass

```
./build/debug/tests/buddd_tests
→ All tests passed (22658 assertions in 697 test cases)
```

**Result: PASS**

### 12. No forbidden file changes

Only the 3 allowed files were modified:
- `src/editor/panels/scene_panel.cpp` ✅
- `.specs/sprint-2026-06/entity-selection/spec.md` ✅
- `.specs/sprint-2026-06/entity-operations/spec.md` ✅

No changes to:
- `src/editor/editor_selection.h` ✅
- `src/editor/panels/scene_panel.h` ✅
- `src/engine/**` ✅
- `tests/**` ✅
- Any other files ✅

**Result: PASS**

### 13. Visual verification

The editor was launched with a demo scene (4 entities) and a screenshot captured at frame 1. The image shows:
- Scene Panel with entity tree rendered correctly ("main_camera", "box_via_directive", "(unnamed)", "box_via_component", and "light")
- No visual artifacts or rendering issues
- Normal ImGui editor layout

Right-click behavior is inherently interactive and cannot be verified through static screenshots. All 16 acceptance criteria (AC-01 through AC-16) are manual test items per the spec.

**Result: PASS (with caveat that manual testing is required for interactive behavior)**

## Blocking issues

None.

## Warnings

- Manual testing of all 16 acceptance criteria (AC-01 through AC-16) is required to fully verify right-click behavior. The automated checks confirm the code compiles, matches the spec structure, and doesn't break existing tests, but interactive behavior (right-click → selection change → context menu → Delete) must be verified by a human.
- The debug log on right-click selection uses a simplified message (`"set"` or `"unchanged"`) rather than the richer format suggested in the spec's Observability section (which recommended including modifier and entity ID). This is non-blocking — existing debug logging is adequate for operational purposes.

## Suggested improvements

None.

## Questions for human

- AC-12 (Context menu "Delete" is enabled after right-click on an entity) and AC-13 (Context menu "Rename" is enabled after right-click on an entity) depend on F-04's context menu implementation reading `ctx.editor.selection().empty()` and `context_menu_entity_` correctly. The code change is correct, but manual testing should confirm the context menu respects the new selection state.
