# Spec Review — SPEC-015: Free Camera Interactive Demo

## Review summary

**Re-review (round 2):** The previous blocking issue (Escape key — `poll_events()` does not return `false` on Escape) is fully resolved. All fixed issues have been verified: (1) Escape handling now uses explicit `input.is_down(KeyCode::Escape)` throughout — Goals, Controls table, pseudocode, User Story 3, AC-013, and edge cases are all consistent. (2) Camera variable now correctly dereferences through `CameraComponent`. (3) `set_perspective()` is present with correct 800×600 aspect ratio. (4) Registration instructions are precise about insertion point. 

One minor pseudocode ordering issue remains (non-blocking). No new issues introduced. 

**Recommendation:** Accept — no remaining blocking issues.

**Re-review (round 3 — delta_time engine change):** The `Platform::delta_time()` adoption is applied consistently throughout the spec — all 20 references to delta_time/dt now use `platform.delta_time()` with no remaining chrono/`steady_clock`/manual timing references. The "No engine core changes" non-goal and associated out-of-scope/permissions language have been properly removed. The pseudocode, framerate-independence section, AC-022, edge cases, and A-15 are all updated and self-consistent.

**One new issue found:** A contradiction between assumption A-15 (delta_time is guaranteed > 0) and the edge case (describes behavior when delta_time returns zero). These cannot both be true — see blocking issues below.

**Recommendation:** Accept with minor fix — resolve the A-15 / edge-case contradiction, then accept.

**Re-review (round 4 — A-15/edge-case fix verification):** The fix is verified as correct and consistent:
1. The "delta_time returns zero" edge case has been removed entirely.
2. The "very large delta_time" edge case now states: "Under normal operation `delta_time` is always > 0."
3. A-15 now states: "Under normal operation, this is always > 0."
Both A-15 and the edge case now agree on the contract — no contradiction remains.

No new issues introduced. All previous issues remain resolved.

**Recommendation:** Accept — no remaining blocking issues.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **Blocking (RESOLVED): Escape key does NOT cause `poll_events()` to return `false`** — The spec previously claimed `poll_events()` returns `false` on Escape. Now fixed: all references use explicit `input.is_down(KeyCode::Escape)` check + break. Goals (§"Exit on Escape"), Controls table, pseudocode (lines 149–150), User Story 3, AC-013 (line 314), and edge cases (line 351) are all consistent and correct.

- [x] **Blocking (RESOLVED): A-15 contradicts the delta_time edge case** — Assumption A-15 previously stated `Platform::delta_time()` is "guaranteed to be > 0" while the edge case documented behavior for "delta_time returns zero". The fix removed the zero edge case, updated the large-delta edge case to note the normal contract, and updated A-15 to say "always > 0 under normal operation." The contradiction is resolved — both now agree on the same contract.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Warnings

Non-blocking concerns for awareness:

- **Pseudocode: `input` used before assignment in loop** — The loop pseudocode (lines 131–132) checks `input.is_down(KeyCode::Escape)` before `input ← platform.input_system()` is assigned on line 135. On the first iteration, `input` would be uninitialized. The fix is trivial: move `input ← platform.input_system()` to immediately before the Escape check. The intent is unambiguous (any implementer will naturally store `auto& input = platform.input_system()` before the loop), so this is a non-blocking pseudocode ordering artifact.

- **AC-007 unit test feasibility** — AC-007 specifies "Unit test: with any delta above the threshold, pitch never exceeds the bound." The pitch clamp logic lives in `src/cmd/demo/` (application-level code), not in the engine core. The project's test suite targets engine code, not demo code. Consider clarifying whether the test refers to extraction into a testable helper or whether code review is sufficient.

- **SC-004 line count constraint (≤120 lines)** — The free-camera demo adds mouse look, keyboard movement, delta-time computation, constants, Escape handling, and timing. At 120 lines this may be tight compared to `cube_scene_demo.cpp` (91 lines) which has simpler logic. Consider relaxing to 150 lines.

- **Numerical stability of XZ forward projection at extreme pitch** — At ±89° pitch, the XZ-projected forward vector is very small (~0.017 units). `normalize()` on this near-zero vector is numerically unstable. Consider adding an edge case entry documenting this behavior (acceptable for a demo).

## Required changes

- ~~Resolve the A-15 vs edge case contradiction~~ — **Done.** Verified in round 4.

## Suggested improvements

Optional ideas (not required):

- Add `mouse_position()` to the acceptance criteria as a negative check (verify the demo uses `mouse_delta()`, NOT `mouse_position()`, for look) — partially covered by AC-019.
