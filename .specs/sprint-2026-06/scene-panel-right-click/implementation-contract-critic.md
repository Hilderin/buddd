# Implementation Contract Review — Scene Panel Right-Click Selection

## Blocking issues

Items that must be resolved before the artifact can be accepted.

*None — no blocking issues identified.*

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Warnings

Non-blocking concerns for awareness:

- **Debug log less detailed than spec recommendation**: The spec (Observability section) recommends a debug log that includes the modifier type, entity ID, and action (`"Right-click selection: {} selected entity {} (modifier={})"`). The contract implements a simpler `"Right-click selection: {}"` with `"set"`/`"unchanged"` values. While still useful for debugging, the implementer may want to enrich this log to match the spec's recommendation for better traceability.
- **AC-12/AC-13 dependency on F-04 context menu implementation** (inherited from spec-critic): AC-12 and AC-13 verify context menu enabled/disabled state after right-click, which depends on F-04's context menu reading `selection().empty()`. The contract correctly assumes this F-04 behavior, but the implementer should verify the context menu implementation matches F-04's contract before testing these ACs.
- **No automated tests**: The contract correctly follows the spec's manual-only ACs. However, adding a unit test for the right-click selection logic (e.g., `tests/editor/scene_panel_right_click_tests.cpp`) would improve regression protection. The contract already permits this ("If automated tests are added, they must be in a new file...").

## Required changes

Concrete, actionable changes requested:

*None — no required changes.*

## Suggested improvements

Optional ideas (not required):

- **Enrich the debug log** to include modifier type (Shift/Ctrl/plain) and entity ID, as recommended by the spec's Observability section. This makes debugging right-click selection issues much easier.
- **Consider adding an automated test** in `tests/editor/scene_panel_right_click_tests.cpp` for the core selection logic (e.g., verify `!contains(id) → Toggle` never removes). The contract already allows adding tests.

## Per-criterion evaluation

| # | Criterion | Verdict | Rationale |
|---|-----------|---------|-----------|
| 1 | Matches spec intent | ✅ Pass | All 8 goals (G-01 through G-08) are addressed. The right-click selection rules table from the spec is faithfully reproduced in the contract's behavioral specification. All 16 acceptance criteria are mapped to manual test steps. |
| 2 | Specifies exact files and line ranges | ✅ Pass | `scene_panel.cpp` — lines 99–106 specifically identified. BEFORE/AFTER blocks match actual code content exactly (verified against source). The other two spec files have precise instructions (remove NG-10, update AC-32 text). |
| 3 | Provides precise code replacement blocks | ✅ Pass | The AFTER block is production-ready C++ code (not pseudocode). Uses correct API calls (`selection.contains()`, `selection.select()`, `selection.set_selection()`, `selection.anchor()`, `collect_range()`). Variable names (e.g., `selection`, `anchor`, `range`) are consistent with existing code style. |
| 4 | Handles all edge cases from the spec | ✅ Pass | 14 edge cases from the spec are each explicitly mapped to expected behavior and contract handling in the Edge cases table. This includes empty selection, multi-select, no anchor, anchor==clicked, rapid clicks, inline rename, modal dialogs, 10K+ entities, and more. |
| 5 | Never deselect on right-click invariant | ✅ Pass | For Ctrl+right-click: `!contains(id)` guard prevents the Toggle "remove" path. For Shift+right-click on selected entity: `contains(id)` → no-op. For plain right-click on selected entity: no-op. Empty-area right-click: unchanged. No code path can remove an entity from selection on right-click. |
| 6 | Selection change BEFORE context menu popup | ✅ Pass | The AFTER code block explicitly places `selection.select(...)` / `selection.set_selection(...)` BEFORE `context_menu_entity_ = entity.id()` and `open_context_menu = true`. The invariant is called out in a dedicated section. Verified against code flow: selection mutation → open_context_menu flag → `ImGui::OpenPopup("scene_ctx")` → `ImGui::BeginPopup("scene_ctx")` reads `selection().empty()`. |
| 7 | Uses existing APIs correctly | ✅ Pass | `EditorSelection::contains(EntityId)` → `bool` ✅; `EditorSelection::anchor()` → `std::optional<EntityId>` ✅; `EditorSelection::select(EntityId, SelectionModifier)` → `void` ✅; `EditorSelection::set_selection(std::span<const EntityId>)` → auto-converts from `std::vector` ✅; `SelectionModifier::Replace`/`Toggle` ✅; `collect_range(World&, EntityId, EntityId)` → `std::vector<EntityId>` ✅. `auto& selection = ctx.editor.selection()` binds correctly because `Editor::selection()` returns `EditorSelection&` and `EditorContext::editor` is a mutable reference. |
| 8 | No unused/unnecessary changes | ✅ Pass | Only the right-click detection block (lines 99–106) is modified. No changes to left-click handling, context menu, empty-area right-click, keyboard shortcuts, or any other code. |
| 9 | No changes to engine/Entity/World | ✅ Pass | Explicitly listed as non-goals and in forbidden files list. `collect_range()` is already a `ScenePanel` member helper — no engine changes needed. |
| 10 | No changes to left-click behavior | ✅ Pass | Left-click block (lines 108–134) is explicitly called out as unchanged in both non-goals and forbidden files. No overlap between right-click and left-click detection. |
| 11 | Impact on Existing Specs is accurate | ✅ Pass | F-03 NG-10 removal: precise instruction (remove NG-10 from Non-goals, remove bullet from Out of scope). F-04 AC-32 update: exact before/after text provided. Both are listed as allowed-to-change files with specific edits. Verified against spec-critic which confirmed these changes are correct. |
| 12 | Debug logging recommendation is appropriate | ✅ Pass (with warning) | The contract adds a debug log after the selection mutation, which is an improvement over the current no-log state. However, the log is simpler than the spec's recommendation (no modifier/entity ID info). See Warnings above. |

## Summary

**Verdict: ACCEPTED** — The implementation contract is complete, precise, and aligned with the spec. It provides exact code replacement blocks that match the actual source code, correctly handles all 14 edge cases and 16 acceptance criteria, respects all invariants (especially "never deselect on right-click"), and uses existing APIs correctly. The three warnings are non-blocking and concern debug log detail, a cross-feature dependency note, and optional testing considerations.
