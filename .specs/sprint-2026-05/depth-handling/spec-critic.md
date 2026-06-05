# Spec Review — Depth Buffer Support for OpenGL Renderer (SPEC-012)

## Status

`Accepted with warnings`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Summary

SPEC-012 proposes a minimal, well-scoped change to add depth buffer support to the OpenGL renderer: request a 24-bit depth buffer from SDL3 before context creation, enable `GL_DEPTH_TEST` with `GL_LESS`, and add `GL_DEPTH_BUFFER_BIT` to the per-frame clear. The problem is clearly motivated (current rendering is flat because fragments write in index-buffer order), and the solution is precisely the hardware depth buffer — the standard, correct approach for opaque 3D rendering. The spec explicitly respects all architecture boundaries (CONST-001), makes zero changes to the `RenderDevice` abstract interface, and confines all modifications to two files in `src/engine/render/`. Edge cases and error cases are thoroughly catalogued. No blocking issues were found, but several non-blocking concerns are noted below.

## Strengths

1. **Problem statement is crystal clear** — the spec identifies the exact root cause (no depth buffer allocated, no `glEnable(GL_DEPTH_TEST)`, no depth clear) with concrete evidence (individual draw calls contain both front and back faces in the same index buffer, so painter's algorithm cannot fix it).

2. **Exceptionally well-scoped** — the spec defines a comprehensive set of non-goals (lines 38–49) and out-of-scope items (lines 240–255) that clearly establish what is *not* being done. This prevents scope creep.

3. **All acceptance criteria are testable** — some via static inspection (code review), some via automated checks (`glGetError()`), some via manual visual verification (cube demos), and one via `diff` against pre-spec headers. This is appropriate for a graphics-level change where visual correctness matters.

4. **Thorough edge-case and error-case analysis** — 12 edge cases and 5 error cases are documented (lines 196–218), covering SDL3 fallback behavior, window resize, multiple device instances, missing depth buffer, multi-threading concerns, and more. The spec honestly acknowledges failure modes rather than hiding them.

5. **Excellent assumption documentation** — 11 assumptions (lines 259–274) cover SDL3 behavior, OpenGL defaults, camera coordinate system, state persistence, constructor ordering, window resize semantics, headless backend constraints, and more.

6. **Architecture boundary respected** — all changes stay inside `src/engine/render/`, no new public API surface, no `RenderDevice` interface changes. Full compliance with CONST-001.

7. **Cross-spec consistency** — no contradictions with SPEC-005 (Render Pipeline), SPEC-011 (Scene Rendering), SPEC-009 (3D Cube Demo), or any constitution rule. The changes are transparent to `RenderSystem`, `CameraComponent`, `MeshRenderer`, etc., as promised.

8. **Open questions resolved with clear rationale** — Q-01 (explicit `glDepthFunc(GL_LESS)` for clarity) and Q-02 (no `glGetError()` check, consistent with existing patterns) both have well-reasoned proposed resolutions.

## Weaknesses

1. **AC-010 (`glGetError()` returns `GL_NO_ERROR`) has no implementation vehicle** — The open question Q-02 explicitly decides *against* adding a `glGetError()` check to production code ("No — the existing code does not check `glGetError()` after state setup"). Yet AC-010 requires verifying this. The AC says "Verifiable via a debug build or ad-hoc test" but no test code, test file, or mechanism is proposed to make this verification repeatable. This creates a gap between an AC that demands `glGetError()` verification and the explicit decision to not implement it.

2. **Heavy reliance on "static inspection" for ACs** — AC-001 through AC-004, AC-011, and AC-014 are verified solely by "static inspection" (code review). While acceptable for a change of this magnitude, CONST-002 ("All testable code must have corresponding unit tests") would ideally see at least an automated check (e.g., grepping for `GL_DEPTH_BUFFER_BIT` in the clear call or verifying the `SDL_GL_DEPTH_SIZE` attribute in a test). The spec adds no new automated test cases for depth-specific behavior beyond the unimplemented AC-010.

3. **Minor ambiguity in "No changes to" list** — Line 129 states "`render_device.cpp` (headless path) — unchanged" but the file IS being modified (the OpenGL path within it gains a `SDL_GL_SetAttribute` call). The intent is clear (the headless *branch* is unchanged), but the phrasing could mislead a reader into thinking `render_device.cpp` as a whole is untouched.

4. **No discussion of test file creation** — The spec's Assumption A-10 lists only two modified files and "No new files are created." This means no new test file for depth-specific behavior. CONST-002 would normally require a test for the new behavior, but the spec relies entirely on existing tests passing and visual verification. A brief rationale for why no depth-specific test file is needed would strengthen the spec.

## Blocking issues

None identified.

## Non-blocking issues

- [ ] **AC-010 / Q-02 contradiction**: Open question Q-02 explicitly decides *against* adding `glGetError()` checking to production code ("consistent with pre-spec pattern"). Yet AC-010 requires verifying that `glGetError()` returns `GL_NO_ERROR` after depth state setup. No automated test or debug-build mechanism is proposed. The AC should either be removed or paired with a concrete verification mechanism (e.g., a test in an SDL3-conditional test file). If the AC is meant to be manually verified once during implementation, it should be reclassified (e.g., as a success criterion or development note) rather than an acceptance criterion.

- [ ] **Minor phrasing ambiguity about `render_device.cpp`**: Line 129 says "`render_device.cpp` (headless path) — unchanged" but the file is modified (the OpenGL path within it gains a `SDL_GL_SetAttribute` call). Recommend rewording to "`render_device.cpp` — the headless branch is unchanged" or "`render_device.cpp` — only the OpenGL/context-creation branch is modified."

- [ ] **No new automated test for depth buffer attribute**: All depth-specific ACs are verified by static inspection (AC-001, AC-002, AC-003, AC-004, AC-011, AC-014) or manual visual verification (AC-008, AC-009). While acceptable for the change's scale, the spec would benefit from a brief justification (in Assumptions or an inline note) explaining why no automated unit test is added — e.g., "headless backend has no depth concept; OpenGL depth test verification requires a display-backed test which is out of scope for this change" — to preempt questions about CONST-002 compliance.

- [ ] **Assumption A-04 is fragile**: "No existing code calls `glDisable(GL_DEPTH_TEST)` or changes `glDepthFunc` — the depth state set in the constructor persists for the lifetime of the `RenderDeviceOpenGL`." This is true *today* but could be silently broken by future code in the same constructor or elsewhere. Consider recommending an explicit state verification (or at least noting that future OpenGL state changes must be aware of this assumption).

- [ ] **The success metric SC-003 ("under 5 developer-minutes of additional code") is subjective and unnecessary**: This is the only success criterion with a time estimate. It adds no verification value and conflicts with the professional tone of the rest of the spec. Consider removing or replacing with a line-count constraint.

- [ ] **Accuracy of depth comparison description**: Line 101 says "Z value (after perspective divide, in normalized device coordinates [0,1])". Strictly speaking, the depth comparison happens in *window coordinates* after the viewport depth-range transform (default `glDepthRange(0,1)` maps NDC [-1,1] to window [0,1]). The practical behavior is correct (smaller Z = closer = passes `GL_LESS`), but the parenthetical "in normalized device coordinates [0,1]" is technically imprecise — this is a minor documentation nitpick.

## Required changes

None — the spec is complete and internally consistent. Only non-blocking items above.

## Suggested improvements

1. Resolve the AC-010 / Q-02 tension: either drop AC-010, add a concrete test mechanism, or reclassify it as a non-AC note.
2. Clarify the `render_device.cpp` phrasing in the "No changes to" list.
3. Add a brief rationale in Assumptions explaining why no new depth-specific automated test file is created.
4. Consider removing or rephrasing SC-003's "5 developer-minutes" metric.
5. Fix the minor precision issue in line 101's parenthetical about NDC vs. window coordinates.

## Questions for the human

None — all questions that arose during review are documented as non-blocking issues above and can be resolved by the specification author without further input.
