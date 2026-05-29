# Spec Review — Project Setup: Buddd Engine Bootstrap

## Status

`Accepted with warnings`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

### Previously resolved (carried forward from earlier review cycle)

- [x] **B-01 — AC-006: "does not indicate an error" is subjective and not objectively testable.**  
  AC-006 has been fixed. It now reads: *"process exits with code 0, stdout contains `Buddd Engine v0.1.0`, and stderr is empty."* All three conditions are objective and independently verifiable.

- [x] **B-02 — AC-009: Dual optionality makes the expected outcome ambiguous.**  
  AC-009 has been fixed. It now specifies a single outcome: an INTERFACE library target with no sources and no binary produced. The "or" clause has been removed.

- [x] **B-03 — AC-005 verification is inconsistent with Story 2's version format.**  
  AC-005 now requires the exact semver pattern `buddd <major>.<minor>.<patch>`, matching Story 2. The loose "contains `buddd` followed by a version string" wording has been removed.

- [x] **B-04 — AC-001 / AC-002 verification depends on CMake version-specific output text.**  
  Verification for both AC-001 and AC-002 now uses `cmake -L -N build/<preset>` to check `CMAKE_BUILD_TYPE:STRING=<Debug|Release>` in the generated cache, which is stable across CMake >= 3.28.

### New issues from this review

No new blocking issues found. All new acceptance criteria (AC-011 through AC-015) are testable; all new content is internally consistent with the existing spec sections.

## Warnings

Non-blocking concerns for awareness:

### Previously carried forward

- [x] **W-01 — AC-006 acceptance criterion was weaker than User-visible behavior section.**  
  *Resolved.* AC-006 now matches the User-visible behavior section exactly (`"Buddd Engine v0.1.0"`).

- [ ] **W-02 — Version function API is underspecified (Q-02).**  
  Q-02 remains `[NEEDS CLARIFICATION]` in the spec. The spec says `buddd::engine::version()` as an example but does not specify the return type or whether it is `constexpr`/`consteval`/runtime. The implementation contract has resolved this as `auto version() -> std::string_view` (runtime), but the spec itself has not been updated. This is not a spec defect since the implementation contract provides the constraint, but the spec should ideally reflect the resolution for future readers.

- [ ] **W-03 — No CMakePresets.json location or file structure specified.**  
  The spec repeatedly references presets (`debug`, `release`) but does not explicitly state that the presets file lives at `CMakePresets.json` in the repository root. This is a minor documentation gap — the location is industry convention and the implementation contract specifies it, but the spec could be more explicit.

- [ ] **W-04 — Test binary path not documented alongside CLI binary path.**  
  AC-004 and AC-005 hardcode `build/debug/src/cmd/buddd` for the CLI binary, but the test binary path (`build/debug/tests/buddd_tests`) is not documented in the spec. It is mentioned in A-09 (name `buddd_tests`) but the path is not given. This gap is carried forward from the previous review and is still present.

- [x] **W-05 — "No CI/CD" non-goal partially conflicts with Q-01 impact note.**  
  *Resolved.* Q-01 is now marked `[RESOLVED]` and the compiler question is addressed in A-04 (GCC 14+ reference compiler). No conflict remains.

### New warnings

- [ ] **W-06 — AC-015: "Debug buddd_tests" launch configuration program path is underspecified.**  
  The verification says the `"program"` field should point to *"the test executable"* without specifying a path. Unlike `"Debug buddd"` which has an explicit path (`build/debug/src/cmd/buddd`), the test binary path is left ambiguous. An implementation could use any valid path and claim compliance. The spec should document the expected path (e.g. `build/debug/tests/buddd_tests`) or use a CMake variable reference.  
  *Impact*: Minor ambiguity; the implementation contract resolves it to `build/debug/tests/buddd_tests`, but the spec itself does not.

- [ ] **W-07 — No error or edge case for missing `clang-format` when using the `format` target.**  
  Assumption A-12 states that if `clang-format` is not installed, the `format` target will *"produce a clear error message during build."* However, this behavior is not captured in the **Error cases** or **Edge cases** sections. A new error case should be added: *"`clang-format` not installed — `cmake --build --preset debug --target format` exits non-zero with a message indicating `clang-format` was not found."*  
  *Impact*: The behavior is defined (in A-12) but not tracked in the proper specification section.

- [ ] **W-08 — Minor inconsistency between Conventions (C++ source files) and Story 5 (includes `.c` files).**  
  The **Conventions** > **Code formatting** section says the `format` target applies to *"all C++ source files under `src/` and `tests/`."*  
  Story 5's `Then` clause lists *"all `.cpp`, `.hpp`, `.h`, and `.c` files"* — which includes `.c` (C source) files. In a C++26 project, `.c` files are unusual; if they are intentionally included, the Conventions section should be updated to say *"all source files"* or be explicit about the extension list. If not, Story 5 should drop `.c`.  
  *Impact*: Low — unlikely to cause confusion in practice, but the two sections disagree on scope.

- [ ] **W-09 — AC-011 verification uses an "or" clause (two verification paths).**  
  The verification states: `head -1 .clang-format` contains `BasedOnStyle: LLVM` **or** the file is parseable by `clang-format -style=file -dump-config` without error. This is similar in form to the previously-rejected B-02 pattern. A `.clang-format` file could pass one check and fail the other (e.g., comments before `BasedOnStyle`). While this is a verification guidance issue rather than a spec outcome ambiguity, a single clear verification method is preferable.  
  *Impact*: Low — in practice both checks usually agree, but a strict tester could report a false failure.

- [ ] **W-10 — The `Conventions` section's `snake_case` rule for file names is not explicitly linked to the new AC-011–AC-015 files.**  
  The new requirements introduce files like `.clang-format`, `.vscode/settings.json`, `.vscode/tasks.json`, `.vscode/launch.json`. These are **dotfiles** and configuration files, not source files — so the `snake_case` convention does not technically apply. However, the spec should clarify whether the `snake_case` convention applies only to C++ source files, or to all project files (with dotfiles as exceptions).  
  *Impact*: Low — dotfiles conventionally use their tool-required names.

## Required changes

Concrete, actionable changes requested:

### Previously requested (all now verified as resolved)

1. [x] **Fix AC-006** to replace "does not indicate an error" with objective criteria. → **Resolved.**
2. [x] **Fix AC-009** to commit to one outcome (interface library, no binary). → **Resolved.**
3. [x] **Align AC-005 with Story 2** so verification requires semver pattern. → **Resolved.**
4. [x] **Fix AC-001 / AC-002** verification to use CMake cache check. → **Resolved.**

### Newly requested

5. [x] **Fix AC-015 to specify the test executable path.**  
   Replace *"pointing to the test executable"* with an explicit path (e.g., `${workspaceFolder}/build/debug/tests/buddd_tests` or a build-variable reference), consistent with how the `"Debug buddd"` path is given.

6. [x] **Add an error case for missing `clang-format`.**  
   Insert a row in the **Error cases** table: *"`clang-format` not installed — `cmake --build --preset debug --target format` exits non-zero with a clear error message indicating `clang-format` was not found."* The current assumption A-12 implies this behavior, but it must be in the Error cases section to be normative.

7. [x] **Resolve the `.c` vs. C++ scope inconsistency between Conventions and Story 5.**  
   Either:
   - Remove `.c` from Story 5's file extension list (since this is a C++26 project), or
   - Update the Conventions section to say *"all source files"* and list the same extensions as Story 5.

## Suggested improvements

Optional ideas (not required):

1. **Resolve Q-02 in the spec** to match the implementation contract's decision: `auto version() -> std::string_view` (runtime function). This would close the last open question.

2. **Add `.gitignore` entries** for `build/`, compiled binaries, and `.vscode/` user settings (if not already present). Common developer convenience that prevents accidental commits of build artifacts.

3. **Explicitly state the `CMakePresets.json` location** in the spec (repository root). This removes the minor ambiguity flagged in W-03.

4. **Document the test binary path** (`build/debug/tests/buddd_tests`) in the spec, matching how the CLI binary path is documented. This addresses the carried-forward W-04.

5. **Consider documenting the `clang-format` version requirement in the goals section.** Assumption A-12 requires `clang-format >= 18`, but the goals only mention the `.clang-format` file and the format target. Adding the version requirement to the goals (or to the format target description) would make it discoverable without reading the assumptions section.

6. **Consider adding a short "no formatting check in CI" non-goal** since CI is out of scope — this clarifies that the `format` target is for local developer use only at this stage.

## Cross-reference: New acceptance criteria (AC-011 through AC-015)

| ID | Summary | Testable? | Issues |
|---|---|---|---|
| AC-011 | `.clang-format` exists (LLVM style) | ✅ | W-09 (dual verification path) |
| AC-012 | CMake `format` target formats source files | ✅ | Requires unformatted test setup; W-07 (missing error case) |
| AC-013 | `.vscode/settings.json` with IntelliSense settings | ✅ | — |
| AC-014 | `.vscode/tasks.json` with configure/build/test tasks | ✅ | W-06 (task specification slightly vague but verifiable) |
| AC-015 | `.vscode/launch.json` with debug configurations | ✅ | W-06 (test path underspecified) |

All new ACs are testable. No blocking issues introduced.

## Summary

The spec update adds well-defined new requirements (clang-format integration and VS Code workspace configuration) with testable acceptance criteria. All four previously-identified blocking issues (B-01 through B-04) are confirmed resolved. The five new acceptance criteria (AC-011 through AC-015) are testable, and the new Conventions section provides useful developer guidance.

The remaining concerns are non-blocking: an underspecified launch configuration path (W-06), a missing error case for the format target (W-07), a minor scope inconsistency between sections (W-08), a dual verification path in AC-011 (W-09), and a clarification about the scope of naming conventions (W-10). The carried-forward warnings W-02, W-03, and W-04 remain unaddressed but are minor.

**Verdict: Accepted with warnings.** Implementation may proceed, but the three newly-requested required changes (items 5, 6, 7) should be addressed before the spec is finalized.
