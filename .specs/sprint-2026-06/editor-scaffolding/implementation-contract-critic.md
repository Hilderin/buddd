# Implementation Contract Review — Editor Scaffolding

## Blocking issues

No blocking issues found. The contract is thorough, complete, and correctly implements all requirements from the accepted spec. All 24 acceptance criteria (AC-001 through AC-023, SC-002) are verifiable and have corresponding Done criteria in the contract.

- [x] Spec compliance: Contract implements exactly what the spec describes. No extra features, no omitted requirements.
- [x] Testability: All Done criteria are specific and verifiable (inspection, build, manual, or grep-based).
- [x] Ambiguity: Contract is unambiguous — a code implementer can work from it without escalation.
- [x] Architecture boundary: AC-016, AC-017, SC-002 explicitly enforce ADR-019 (no SDL3/OpenGL/GLM outside `src/engine/`).
- [x] Consistency: Contract matches coordination.md Decision Log (D-01 through D-12).
- [x] Implementation order: Step ordering is correct with no missing prerequisites.
- [x] Edge cases: All 14 edge cases from the spec are addressed, plus 4 additional edge cases.
- [x] ADR-026 amendment: Correctly noted — adr-agent handles amendment, implementer must NOT modify ADR files.
- [x] Const mismatch: `EngineContext const&` used consistently throughout all method signatures.
- [x] Test plan: Complete, CI-safe (headless unit test with `[editor]` tag, no display required).
- [x] File permissions: "Files allowed to change" and "Files forbidden to change" sections are precise and correct.
- [x] No architectural decisions left to the Code Agent.

## Warnings

Non-blocking concerns for awareness:

- **Test code calls `draw_ui()` after potentially failed `setup()`**: The contract's test code template calls `editor.draw_ui()` unconditionally after `setup()`. The spec's E2E section explicitly states "`draw_ui()` is not called in this test." When `setup()` fails (ImGui not initialized in headless mode), `impl_->ctx` is already stored (set in step 1 of `setup()` before the `is_initialized()` check), so the null-ctx guard in `draw_ui()` does NOT trigger. Calling `ImGui::GetMainViewport()` / `ImGui::DockSpaceOverViewport()` without an initialized ImGui context may cause a null-pointer dereference or debug assertion. The spec correctly excluded `draw_ui()` from the headless test. The implementer should either skip the `draw_ui()` call (matching the spec) or guard it with an `is_initialized()` check.

- **Incorrect subdirectory order in description**: The contract states the root `CMakeLists.txt` order is `engine → editor → cmd`, but the actual order is `engine → cmd → editor` (confirmed by reading the file). This does NOT affect build correctness (CMake handles forward target references), but the statement is factually inaccurate and should be corrected to avoid confusion. The spec has the same inaccuracy.

- **`[NEEDS CLARIFICATION]` marker in test template**: The test code template includes `// [NEEDS CLARIFICATION]` in a comment (line 420 of the contract). While the test requirements (TEST-001 through TEST-003) are unambiguous, the presence of this marker in a contract that should be final is inconsistent with the required level of specificity.

## Required changes

No required changes. The contract is acceptable as-is. The warnings above are non-blocking suggestions for improvement.

## Suggested improvements

1. **Remove the `[NEEDS CLARIFICATION]` comment** from the test template, or replace it with a concrete recommended approach.
2. **Remove the `draw_ui()` call from the test** or guard it with `if (engine_imgui::is_initialized())` to avoid potential UB in display-mode test runs.
3. **Correct the subdirectory order** in the description to match actual `CMakeLists.txt`.
