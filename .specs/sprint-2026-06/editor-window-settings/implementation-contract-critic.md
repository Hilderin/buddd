# Implementation Contract Review — IMPL-037: Editor Window Geometry Persistence

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **Contradiction: integration test file not in allowed changes list**: RESOLVED — `tests/editor/settings_integration_tests.cpp` is now listed as item 13 in the "Files allowed to change" list. The integration tests section correctly references appending to this file. No contradiction remains.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Warnings

Non-blocking concerns for awareness:

- **No explicit test for `PlatformSDL3::display_bounds()` out-of-range bounds check**: The contract's implementation (item 7) includes a bounds check returning `{0,0,0,0}` for `index < 0 || index >= count`, but no test exercise this defensive path. The test table for `platform_display_tests.cpp` only has a nominal offscreen test and a headless zero test. Consider adding a test case for out-of-range index (e.g., `display_bounds(-1)` and `display_bounds(999)` on `PlatformSDL3`) to verify the bounds guard.

- **AC-018 `resize()` immediate cache update tested only on `WindowHeadless`**: The contract tests AC-018's immediate-cache-update requirement only on `WindowHeadless`. While both `WindowSDL3` and `WindowHeadless` use the same `width_ = width; height_ = height;` pattern, the spec's verification suggests a unit test without specifying implementation. Consider adding an offscreen SDL3 test that verifies `WindowSDL3::resize()` also updates cached dimensions immediately, not relying on the subsequent `on_resize()` callback.

- **Spec-critic warnings not explicitly addressed in contract**: The spec-critic identified several non-blocking concerns (`save_all()` double-call risk, `resize()` vs `on_resize()` naming, AC-016 mock approach, `noexcept` conventions). The contract addresses most of them well (explicitly notes `save_all()` must not be called twice, adds `noexcept` rules, documents the `resize()`/`on_resize()` distinction). However, the `noexcept` rules in the contract (Section "Existing conventions to follow", item 1) are stated without referencing the spec-critic's recommendation — this is not a problem, but traceability would be improved by a brief note.

- **No test coverage for `window_state_to_string` fallback return**: The `window_state_to_string` switch has a `return "normal";` after the switch (dead-code if all enum values are covered, but serves as defensive fallback). No test exercises this fallback path. Minor.

- **Success criteria (SC-001 through SC-006) not explicitly traced**: The test cases map to AC IDs but have no direct traceability to the success criteria (SC-001: Window methods coverage, SC-002: Platform methods coverage, SC-003: size boundary tests, SC-004: position overlap tests, SC-005: state round-trip, SC-006: full round-trip). While coverage is implicitly present, explicit SC tracing would aid verification.

## Required changes

Concrete, actionable changes requested:

1. **Resolve the integration test file contradiction**: Either:
   - Add `tests/editor/settings_integration_tests.cpp` to the "Files allowed to change" list, OR
   - Move the integration test cases from "Append to `tests/editor/settings_integration_tests.cpp`" into the new `tests/editor/window_settings_tests.cpp` file (which is already listed as item 17), wrapped in `#ifdef BUDDD_HAS_DISPLAY` where needed.

## Suggested improvements

Optional ideas (not required):

- Add explicit test for `PlatformSDL3::display_bounds()` out-of-range index.
- Add offscreen SDL3 test for `WindowSDL3::resize()` immediate cache update (AC-018).
- Add SC-00X tags to test case descriptions for easier spec traceability.
- Consider adding a compile-time test or comment noting that `editor.cpp` has no SDL3 includes, to formally satisfy AC-023's code-review verification.

## Review summary

The implementation contract IMPL-037 is thorough, well-structured, and closely follows SPEC-037. It correctly maps all 23 acceptance criteria to concrete implementation steps and test cases. The type choices (`int32_t`, `std::string`) match `SettingsStore`'s explicit instantiations. The bootstrap ordering (window created before settings loaded) is correct. No SDL3 headers leak into editor code. The `noexcept` convention follows existing patterns. The save/load algorithms match the spec pseudocode. The overlap test is correctly transcribed. PlatformHeadless and WindowHeadless stubs are properly specified.

**Re-review (loop #2)**: The blocking issue from the first review has been resolved. `tests/editor/settings_integration_tests.cpp` is now listed as item 13 in the "Files allowed to change" list. Additionally, the author addressed several non-blocking warnings: out-of-range `display_bounds()` tests (-1 and 999) added, offscreen SDL3 `resize()` immediate cache update test (AC-018) added, and SC-00X traceability columns added to all test case tables. No new issues introduced.

**The contract is accepted. All issues resolved.**
