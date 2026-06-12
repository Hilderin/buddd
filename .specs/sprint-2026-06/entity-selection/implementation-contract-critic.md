# Implementation Contract Review — F-03 Entity Selection with Multi-Select

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **B-01: `std::minmax` dangling reference UB in `collect_range` (Step 4)** — RESOLVED in loop-back #1. Contract now uses manual `(std::min)`/`(std::max)` on named `size_t` variables with an explanatory comment (lines 492–498).

- [x] **B-02: Empty-area click does not respect modifier-key no-op (violates AC-27)** — RESOLVED in loop-back #1. Contract now includes `!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift` guard with AC-27 reference (lines 466–470).
  ```cpp
  if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
      ctx.editor.selection().clear();
  }
  ```
  The spec explicitly requires (AC-27, edge cases, and the multi-select interaction table) that **Ctrl+click empty area** and **Shift+click empty area** are **no-ops** — the modifier is ignored on empty clicks, meaning the click itself does nothing. The code above clears selection regardless of modifier state.  
  **Fix**: Add a guard that Ctrl and Shift are NOT held:
  ```cpp
  if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
      if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift) {
          ctx.editor.selection().clear();
      }
  }
  ```

## Warnings

Non-blocking concerns for awareness:

- **Anchor asymmetry documented but subtle**: `clear()` clears the anchor; `set_selection({})` (empty span) leaves the anchor unchanged. The contract documents this asymmetry clearly in the edge cases table, which is good. However, the asymmetry could lead to bugs if future code calls `set_selection({})` expecting it to behave like `clear()`.

- **`EntityId::none()` guard inconsistency between DC and inline implementation**: DC-01 requires `select()` and `set_selection()` to silently ignore `EntityId::none()` entries, and the implementation notes say the same. However, the inline pseudocode for `select(id, Replace)`, `select(id, Toggle)`, and `set_selection(ids)` does NOT show the guard check. The DC requirement is binding, so a diligent Code Agent will add the guard, but the inconsistency may cause confusion or omission. Recommend either adding the guard to the inline pseudocode or adding a clarifying note.

- **No const overload of `Editor::selection()`**: The spec-critic suggested adding `auto selection() const -> EditorSelection const&;` for const-correct callers. The contract does not add this. This is acceptable for F-03 scope (no const consumers yet), but may need to be added in a future sprint when query-only use sites emerge.

- **Callback always fires on no-op**: The contract explicitly documents that callbacks fire on every mutation regardless of net state change. This was flagged by the spec-critic and the contract adopts the simple approach. Acceptable for F-03, but F-05+ may need change-detection gating to avoid unnecessary callback churn.

- **Missing `#include <algorithm>` in `editor_selection.h` note**: The contract includes `<algorithm>` for `std::find_if` used in `remove_on_change()`, but `remove_on_change` is not shown with inline implementation in the header. The DCs don't specify where `remove_on_change` is defined. If implemented in a `.cpp` file (contrary to the "all inline in header" convention stated), `<algorithm>` might need to be in the `.cpp` instead. Minor — the Code Agent will resolve this based on chosen implementation location.

## Required changes

Concrete, actionable changes requested:

1. **Fix `std::minmax` UB in `collect_range`** — See blocking issue B-01.
2. **Fix empty-area click modifier guard** — See blocking issue B-02.

## Suggested improvements

Optional ideas (not required):

1. **Consider `std::vector` for `Selection` backing store**: The spec (A-11) allows an alternative container. For scenes with <10K entities, `std::unordered_set` provides O(1) `contains()` which is needed per-frame in `draw_ui()` (checking `selection().contains(entity.id())` for every entity). `std::vector` would be O(n) for `contains()` and could degrade performance. The `std::unordered_set` choice is optimal given the usage pattern.

2. **Add `reserve()` to `all_ids` in Ctrl+A callback**: The Ctrl+A lambda in `editor.cpp` calls `all_ids.reserve(w.entity_count())`, which is good. However, `collect_all()` in `scene_panel.h` does NOT call `reserve()` before building the vector. Consider adding `ids.reserve(world.entity_count())` to `collect_all()` for consistency and slightly better performance.

3. **Add a brief note about `selection()` accessor thread safety**: Since `EditorSelection` is used from the main thread only (ImGui event processing in `draw_ui()` and shortcut callbacks in `update()`), the contract could explicitly document that no synchronization is needed. This is consistent with existing code conventions but would prevent future questions.

## Review summary

The implementation contract is **well-structured, highly prescriptive, and faithfully implements the spec**. It addresses all spec-critic warnings (callback churn, anchor asymmetry, double-click edges, Command integration path, container choice). The test plan covers 19 unit-testable ACs. The code snippets are accurate and consistent with existing architecture.

**Re-review (loop-back #1): Both blocking issues are RESOLVED.**

1. **B-01**: `std::minmax` dangling reference replaced with safe manual `(std::min)`/`(std::max)`.
2. **B-02**: Empty-area click now guarded with `!KeyCtrl && !KeyShift` per AC-27.

Additional improvement: `EntityId::none()` guard is now explicit in every relevant inline pseudocode block (previous warning addressed).

No new issues found. The contract is ready for implementation.

**Verdict: Accepted**
