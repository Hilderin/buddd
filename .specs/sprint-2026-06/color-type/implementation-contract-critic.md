# Implementation Contract Review — Color Type

## Re-review summary (2026-06-14)

All 3 issues identified in the previous review have been verified as fixed:

1. **Blocking issue**: `tests/engine/component_color_registry_tests.cpp` added as 4th Create file (item 4). Integration test requirement now points to this new file. **Resolved.**
2. **HSV reference**: Changed from `"AC-008 (via Story 7)"` to `"Story 7"`. **Resolved.**
3. **from_string placeholders**: All `{ /* error */ }` replaced with concrete `make_error()` calls. **Resolved.**

**Verdict: Approved.** No remaining blocking issues. Contract is ready for code implementation.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **`tests/engine/component_registry_tests.cpp` not listed in "Files allowed to change" but required to be modified**: RESOLVED. Changed to a new separate file `tests/engine/component_color_registry_tests.cpp` listed as item 4 (Create) in "Files allowed to change". The "Required tests" section now references this new file. No editing of the existing test file required.

## Warnings

Non-blocking concerns for awareness:

- [x] **HSV roundtrip test references wrong AC**: RESOLVED. Changed from `"AC-008 (via Story 7)"` to `"Story 7"`.
- [x] **`from_string` error handling pseudocode incomplete**: RESOLVED. All `{ /* error */ }` placeholders replaced with concrete `make_error()` calls.
- [x] **`component_registry_tests.cpp` absent from both "allowed" and "forbidden" lists**: RESOLVED. Integration test moved to new file `tests/engine/component_color_registry_tests.cpp`; `component_registry_tests.cpp` no longer needs modification.
- **Hardcoded line numbers in model_loader.cpp references**: Still present. The contract also describes changes by content pattern, so the line numbers are supplementary. Non-blocking.
- **Color editor tag-based switching not yet exercisable**: Still true, but documented as a non-goal in the spec. Non-blocking.

## Required changes

Concrete, actionable changes requested:

- [x] Add `tests/engine/component_registry_tests.cpp` to the "Files allowed to change" list (after item 3, before the edit section), OR change the integration test to a new separate file (e.g., `tests/engine/color_component_tests.cpp`) listed as a 4th create. **RESOLVED**: New file `tests/engine/component_color_registry_tests.cpp` added as item 4 (Create).

## Suggested improvements

Optional ideas (not required):

- [x] Fix the HSV roundtrip test AC reference from `"AC-008 (via Story 7)"` to `"Story 7"` or `"Spec"`. **Done.**
- [x] Replace `{ /* error */ }` comments in `from_string` with the actual `make_error()` return, matching the Vec4 pattern shown in `register_all_components.cpp`. **Done.**
- [x] Add `component_registry_tests.cpp` to the "Files forbidden to change" list (if the integration test is moved to a separate file) to avoid ambiguity. **Rendered unnecessary** — the file is simply not modified, and the new integration test file makes this explicit.
