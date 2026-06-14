# Implementation Contract Review — Properties Panel UX Polish (IMPL-F-06)

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **Contradiction: `draw_axis_widget()` dirty marking behavior is inconsistent across the contract.** Section A (Required implementation behavior, step 5) states: during drag handling, "Call `ctx.editor.mark_dirty()`." However, Section B (Dirty marking convention, immediately after the Vec2 code sample) explicitly states: "The `draw_axis_widget()` function does NOT call `mark_dirty()` — dirty marking is the parent editor's responsibility." The done criteria (criterion #7) also says "does NOT call `mark_dirty()`." This is a direct contradiction: an implementer cannot determine whether `draw_axis_widget()` should call `mark_dirty()` during drag or leave it to the parent editor. **Fix**: Remove the `ctx.editor.mark_dirty()` call from Section A step 5 (and update the `@param ctx` doc if it is not used for dirty marking), OR add the call if the design intent changed and update the convention in Section B accordingly. The key architectural question is: does `mark_dirty()` fire every frame during a drag (inside the widget) or once per drag session when the parent copies values back? The current Vec2/Vec3/Vec4/Quat code samples all show `mark_dirty()` in the parent only, suggesting Section A step 5 is the erroneous part.
    **Resolution (loop-back iteration 2)**: Contract author removed `ctx.editor.mark_dirty()` from Section A step 5 and updated the `@param ctx` doc to "reserved for future use". The parent-only dirty-marking model is now consistent throughout the contract. **Verified resolved.**

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Warnings

Non-blocking concerns for awareness:

- ~~**Vec2 and Vec4 code samples lack explicit clamping logic** (sections B and D).~~ **RESOLVED in loop-back iteration 2**: Vec2 and Vec4 code samples now include `std::clamp()` calls matching the edge cases clarification. Done criteria #11 updated to cover Vec2+Vec3+Vec4. Section G now checks for `#include <algorithm>`.

- **Vec3 Section C code sample still lacks explicit `std::clamp()` calls**, while Vec2 and Vec4 samples were updated to include them. The clamping logic is correctly specified in the edge cases section (lines 430–441) and done criteria #11 covers it, but the main Vec3 code block is inconsistent with Vec2 and Vec4. An implementer reading only Section C would not see the clamping. Recommendation: add `std::clamp()` calls to the Vec3 Section C code sample for consistency with Vec2 and Vec4.

- ~~**`draw_axis_widget()` receives `ctx` parameter documented "for mark_dirty()" but the design says it doesn't call it.**~~ **RESOLVED in loop-back iteration 2**: Contract author updated the doc from "for mark_dirty()" to "reserved for future use". The parameter is kept unused but correctly documented.

- **F-05 Scale minimum-value constraint (0.001) was never implemented in the actual codebase.** The current `draw_transform_section()` at `properties_panel.cpp:150` passes `EditorFlags{}` for Scale (no constraint). This contract introduces `EditorFlags{min_value=0.001f}` for Scale and adds clamping in Vec3/Vec2/Vec4 editors. This is architecturally correct per the F-05 spec intent, but it is technically a *new behavior* being introduced in F-06. The contract-author already flagged this. Ensure human validation is aware: if retaining backward compatibility with F-05's actual (unconstrained) behavior is desired, the scale_flags should remain default.

- **The `draw_axis_widget()` modifies `*value` directly during drag (through the pointer).** The parent copies values from its temp array back to the entity field on `changed`. This is correct and works, but it is an unusual pattern (the widget mutates the parent's stack-local array). The implementer must understand that `draw_axis_widget` is not pure — it writes back to `*value` every frame during drag. The code samples are clear, but this should be noted during code review.

- **No explicit mention of `#include <unordered_map>` in the includes check section (G).** The `static std::unordered_map<const void*, float>` inside `draw_axis_widget()` requires `<unordered_map>`. It is transitively available via `inspector_editors.h` (which includes it), so this is not an error — but an explicit check in section G would improve clarity.

## Required changes (loop-back iteration 2 — all resolved)

Both previously requested changes have been addressed:

1. **[REMOVED ✓] `mark_dirty()` contradiction**: The `ctx.editor.mark_dirty()` call was removed from Section A step 5. The parent-only dirty-marking model is now consistent across the entire contract (Section A implementation details, Section B convention, and done criteria #7).

2. **[REMOVED ✓] Clamping in Vec2/Vec4 code samples**: `std::clamp()` calls were added to Vec2 (Section B) and Vec4 (Section D) code samples. Section G now checks for `#include <algorithm>`. Done criteria #11 updated to cover all three Vec editors.

**Remaining consistency note**: Vec3 Section C code sample still lacks clamping in its main code block (though correctly specified in edge cases section and done criteria). Consider aligning the Vec3 sample with Vec2/Vec4 for consistency.

## Suggested improvements

Optional ideas (not required):

- Document the `draw_axis_widget()` signature's `ctx` parameter as "unused — reserved for future dirty-marking" or simply remove it, to avoid confusion.
- Consider using `ImGui::IsItemActivated()` (not `IsItemActive()`) to detect the first frame of drag, which is the idiomatic ImGui approach. The current description "if active just became true" is accurate but the code is not shown; `IsItemActivated()` is cleaner than checking map membership.
- The edge cases table mentions gimbal lock (pitch ≈ ±90°) but does not specify how the composite widget behaves when the Euler angle conversion produces near-gimbal-lock values. The contract inherits F-05 behavior which is acceptable, but a brief note would help the implementer.
