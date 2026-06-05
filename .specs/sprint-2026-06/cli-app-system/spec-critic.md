# Spec Review — SPEC-008: CLI App System — Centralised Render Loop with Scene Dispatch

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [ ] **SC-004 contradicts window dimension change (infeasible acceptance criterion)**. SC-004 requires "same pixel output" when comparing `buddd run cube --capture 120:/tmp/out.png` against the old `buddd capture cube --frame 120 /tmp/out.png`. However, the old capture command used an **800×600** window (see `src/cmd/commands/capture_command.cpp` line 151–153 and SPEC-010), while the new spec uses **1024×768** (spec.md line 88). Different viewport sizes produce different rasterised output; side-by-side pixel comparison will always fail. Either SC-004 must be removed / relaxed, or the window size must match the old capture size when `--capture` is used, or a note must explain why the comparison tolerates dimension differences.

- [ ] **Inconsistent `--capture` error message between AC-026 and Error Cases table**. AC-026 (line 470) says the error message is `"Error: --capture requires a frame number"`. The Error Cases table (line 525) says the error message is `"Error: --capture requires a frame number (format: N:path)"`. These must be a single, consistent string.

- [ ] **`RenderSystem::render_scene()` signature is ambiguous**. The spec (line 278–279) shows `auto render_scene() -> void;` under the `// In src/engine/render/render_system.h:` heading, but it is not clear whether this is a **public member function** of `RenderSystem` (taking no arguments, using the stored `device_`/`world_`) or a free function. The existing `render()` (in the actual `render_system.h`) is a no-argument member function. If `render_scene()` is the same pattern (member, no arguments), the spec should say so explicitly. If it takes additional parameters, those must be declared. Without this clarity, the implementation contract cannot authoritatively specify the interface.

- [ ] **Driver quirk silently captures frame 2 when frame 1 is requested**. Edge case (line 514) states that `--capture 1:path` is "quietly skipped and frame 2 is captured instead." The output file `path` would then contain frame 2 but the user asked for frame 1. The spec's existing capture messages (line 104: `"Captured: <path>"`) do not indicate which frame was actually captured. The spec must either (a) produce a warning on stderr when the frame number is adjusted (e.g., `"Warning: frame 1 adjusted to frame 2 (driver quirk workaround)"`), or (b) document that this silent substitution is intentional and acceptable. The current "quietly skipped" design is a usability defect.

- [ ] **No warning message format for extra arguments to `buddd run` (no scene)**. The spec (line 133) says "Extra arguments produce a warning but proceed" but does not specify the warning text or format. Compare SPEC-007 which defined exact warning format: `"Warning: unexpected arguments after 'demo triangle': extra1 extra2"`. The spec should define the exact warning message format for consistency.

## Warnings

Non-blocking concerns for awareness:

- **5 ACs rely on manual visual inspection**. AC-013 ("manual visual inspection"), AC-021 ("Manual visual: black empty window"), AC-022 ("Manual visual: triangle appearance matches"), AC-023 ("Manual visual: cube appearance matches") are not automatable. This is acceptable for a rendering spec but should be acknowledged. Consider adding a note that these ACs are verified during human acceptance testing, not in CI.

- **Frame numbering dualism**. `render()` receives a 0-based frame counter (App interface line 239: "The `frame` parameter is 0-based (0 = first frame rendered)") while `--capture N` uses 1-based frame numbering (line 163: "The frame number in `--capture N:path` is 1-based"). The observability messages (line 103: `"Scene aborted by user (frame N)"`) use 1-based numbers. This dualism is documented but is a persistent source of off-by-one bugs during implementation. The implementation contract should include explicit test cases that confirm the 0↔1 mapping is correct.

- **`buddd run --frame 10 triangle` silently fails**. The spec (line 85) says scene name is `argv[2]`. If a user writes `buddd run --frame 10 triangle`, then `--frame` becomes `argv[2]` (the scene name) and fails with `"Unknown scene: '--frame'"`. This design is internally consistent but will confuse users who expect GNU-style flag-before-positional-argument ordering. Consider documenting this limitation in the spec or implementing reordering.

- **CONST-002 (Testing Policy) tension**. The spec creates 7 new `App` subclasses (~14 files) with non-trivial rendering logic but explicitly defers unit tests for individual apps (line 566: "Unit tests for individual App subclasses" are out of scope). The ACs cover CLI error paths and the framework (`run_app()`), but the rendering behavior of each App subclass is only verified manually. This leaves a gap in automated coverage for the new code. The policy's "All testable code must have corresponding tests" may be technically satisfied by the CLI integration tests, but the rendering correctness of each App is not automatically verified.

- **RenderSystem::render_scene() testability gap**. AC-012 verifies that `render_scene()` contains the rendering logic without begin/end framing, and AC-013 verifies the output matches `render()`. However, there is no automated test that calls `render_scene()` in a begin/end pair and verifies the output is identical to `render()`. The implementation contract should include a headless test for this equivalence.

- **No automated exit-code test for `buddd run` with `--frame` and `--capture` on headless**. While AC-010 tests capture via `buddd run cube --frame 3 --capture 3:/tmp/test_ac10.png`, this requires display support. There is no headless-only test that exercises the `--frame` limit without a display. The spec assumes headless works (edge case line 510) but provides no AC for it.

## Required changes

Concrete, actionable changes requested:

- Resolve SC-004 contradiction: either remove the pixel-comparison requirement, re-scope it to accept dimension differences, or revert to 800×600 for capture scenarios.
- Align AC-026 error message with the Error Cases table (pick one format).
- Clarify `render_scene()` signature: member vs free, parameters, preconditions.
- Add warning message format for extra arguments to `buddd run` (no scene).
- Add a stderr warning when the driver quirk adjusts frame 1 to frame 2.
- Consider adding a headless `--frame` AC to verify frame-limited exit works without a display.

## Suggested improvements

Optional ideas (not required):

- Add a note to the `render()` method documentation that the frame parameter is 0-based, cross-referencing the 1-based `--capture` convention, to reduce implementation confusion.
- Document the `run` argument-ordering limitation (flags after scene name only) in the CLI usage text or spec.
- Add an AC that `run_app()` returns non-zero when all captures fail (edge case line 531).
- Consider adding a small tolerance note to SC-005 (under 3 seconds) acknowledging that VSync-dependent timing may vary.
