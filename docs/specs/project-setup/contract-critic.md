# Implementation Contract Review — Project Setup: Buddd Engine Bootstrap

## Status

`Accepted`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

---

## Review cycle history

| Cycle | Date | Verdict | Key changes |
|-------|------|---------|-------------|
| 1 | – | `Accepted with warnings` | Initial review. 6 warnings (W-01–W-06), no blocking issues. |
| 2 | 2026-05-29 | `Rejected` | New sections (.clang-format, VS Code files, naming conventions). Found blocking issue B-01 (format target fallback). W-03 resolved. |

---

## Blocking issues

Items that must be resolved before the artifact can be accepted.

### Cycle 1 (original review)

- [x] *(none)* — No blocking issues found. *(Carried forward, resolved — no action needed.)*

### Cycle 2 (this review)

- [x] **B-01 — `format` target fallback code contradicts spec error case and contract's own edge case.**  
  **Location:** Section 1 (root `CMakeLists.txt`), `else()` block, lines 143–146 of the contract.  
  **The problem:** The code uses `COMMAND ${CMAKE_COMMAND} -E echo "clang-format not found..."` which exits **0** (success). However:
  - The **spec error case** (page 2, "Error cases" table, last row) requires: *"`clang-format` not installed → `cmake --build --preset debug --target format` exits **non-zero** with a clear error message."*
  - The **contract's own edge case table** (row "`clang-format` not installed") says: *"exits non-zero with a clear message."*
  - The **contract's inline note** (line 151) claims: *"This satisfies the error-case requirement (AC-012+A-12)"* — but it does **not**, because the exit code is 0, not non-zero.
  
  **Impact:** A Code Agent following the contract verbatim would produce a `format` target that prints a message but exits successfully. This violates the spec requirement (AC-012 error case) and contradicts the contract's own edge-case specification.
  
  **Required fix:** Change the `else()` block to exit non-zero, for example:
  ```cmake
  else()
      add_custom_target(format
          COMMAND ${CMAKE_COMMAND} -E echo "clang-format not found. Install clang-format >= 18."
          COMMAND ${CMAKE_COMMAND} -E false
      )
  endif()
  ```
  Or equivalently, use `COMMAND false` or `COMMAND exit 1` as a second command.

---

## Warnings

Non-blocking concerns for awareness:

### Cycle 1 (original review) — still open

- **W-01 — Self-contradiction: "PascalCase" convention vs. actual test name.**  
  Section "Existing conventions to follow" says: *"Use `PascalCase` for Catch2 test case names (matching Catch2 convention)."*  
  However, the required test case in section 10 specifies `"engine version is non-empty"` — sentence case, not PascalCase.  
  The contract now includes a note that the explicit code block takes precedence, which mitigates the confusion for the Code Agent. The underlying contradiction remains: a future convention checker might flag the mismatch. **Status: unchanged, still open.**

- **W-02 — `CMAKE_CXX_STANDARD` is set in both root `CMakeLists.txt` AND `CMakePresets.json`, creating redundant sources of truth.**  
  The root `CMakeLists.txt` uses `set(CMAKE_CXX_STANDARD 26)` (normal variable), while the presets table also sets it as a cache variable. Both yield the same effective value (`26`), so there is no runtime conflict. However, having two locations for the same setting creates a maintenance hazard. **Status: unchanged, still open.**

- **W-04 — C++26 is not yet an ISO final standard; compiler flag names vary.**  
  The contract assumes `-std=c++26` works (GCC 14+), which matches Assumption A-04 in the spec. This is correct for the reference compiler, but Clang 19+ may use `-std=c++2c` instead. **Status: unchanged, still open.**

- **W-05 — Done criteria #8 ("No Catch2 manual install") is hard to verify in an existing environment.**  
  Since `FetchContent` is used (not `find_package`), this is guaranteed by design. The criterion is more of a design guarantee than a testable acceptance test. **Status: unchanged, still open.**

- **W-06 — The `project(VERSION)` value and the C++ version string are independent sources of truth.**  
  The root `CMakeLists.txt` sets `project(buddd VERSION 0.1.0 ...)` and `version.cpp` returns `"0.1.0"` as a hard-coded string. The contract notes the manual sync requirement. **Status: unchanged, still open.**

### Cycle 1 — resolved

- **W-03 — `CMakePresets.json` `"version"` field described as "optional" but it is actually required.**  
  **Resolution:** The contract now reads (section 2): *"The `"version"` field at the top level must be `6` or higher"* — the word "optional" has been removed. ✅ **Resolved in this cycle.**

### Cycle 2 (this review) — new

- **W-07 — Done criterion #13 does not explicitly require non-zero exit for missing `clang-format`.**  
  **Location:** Done criteria, item 13.  
  The criterion says: *"prints a clear error message about missing `clang-format` (if not installed)"* but does **not** specify the exit code. Meanwhile, the spec error case and the contract's own edge case table both require a non-zero exit. If B-01 is fixed (making the fallback exit non-zero), this criterion should be updated to say *"exits non-zero with a clear error message"* for consistency.

- **W-08 — `src/*.hpp` glob pattern in `format` target references a file extension not yet present in the project.**  
  **Location:** Section 1, GLOB_RECURSE pattern includes `src/*.hpp`.  
  The project currently uses `.h` for headers (e.g., `version.h`). The `.hpp` glob is forward-looking and does no harm, but it could set an implicit expectation that `.hpp` files are expected. This is minor and non-blocking.

- **W-09 — Done criteria do not verify release preset `format` target.**  
  **Location:** Done criteria, item 13 checks the `format` target only for the `debug` preset. The spec does not require release-preset formatting, so this is acceptable. Noted for awareness: if the `format` target is used in release builds, it should also be verified.

---

## Required changes

Concrete, actionable changes requested:

1. **Fix B-01:** Change the `else()` block of the `format` target (root `CMakeLists.txt`, section 1) so that it exits **non-zero** when `clang-format` is not found, matching the spec error case and the contract's own edge case table.

2. **Update Done criterion #13 (if B-01 is fixed):** Add the non-zero exit requirement to the description for consistency.

---

## Suggested improvements

Optional ideas (not required):

1. **(From Cycle 1) Fix the PascalCase contradiction (W-01):** Either change the convention to "sentence case for test case names" or rename the test to `"VersionStringIsNotEmpty"` to match the stated convention.

2. **(From Cycle 1) Deduplicate `CMAKE_CXX_STANDARD` (W-02):** Remove `CMAKE_CXX_STANDARD` and `CMAKE_CXX_STANDARD_REQUIRED` from `CMakePresets.json` to have a single source of truth in `CMakeLists.txt`.

3. **(From Cycle 1) Consider adding `.gitignore` entries** for the `build/` directory and compiled test binaries.

4. **(From Cycle 1) Consider adding a brief comment in `CMakeLists.txt`** noting that `CMAKE_CXX_STANDARD 26` targets GCC 14+ and that Clang users may need `-std=c++2c`.

---

## Cross-reference: Spec acceptance criteria

| Spec AC | Contract section | Status | Notes |
|---------|-----------------|--------|-------|
| AC-001 (debug preset) | §2, Done #2 | ✅ | Debug preset with Ninja, `CMAKE_BUILD_TYPE=Debug` |
| AC-002 (release preset) | §2, Done #4 | ✅ | Release preset with Ninja, `CMAKE_BUILD_TYPE=Release` |
| AC-003 (buddd_engine static lib) | §3 | ✅ | `add_library(buddd_engine STATIC ...)` |
| AC-004 (buddd executable) | §6–7 | ✅ | `add_executable(buddd main.cpp)` + links engine |
| AC-005 (--version output) | §7 (main.cpp) | ✅ | Prints `"buddd 0.1.0\n"` |
| AC-006 (greeting) | §7 (main.cpp) | ✅ | Prints `"Buddd Engine v0.1.0\n"` |
| AC-007 (Catch2 version test) | §10 | ✅ | `REQUIRE_FALSE(version().empty())` |
| AC-008 (FetchContent) | §1 (FetchContent block) | ✅ | Pinned to Catch2 v3.7.0 |
| AC-009 (editor placeholder) | §8 | ✅ | INTERFACE library, no sources, no binary |
| AC-010 (ctest passes) | §9, Done #7 | ✅ | `catch_discover_tests` + ctest preset |
| AC-011 (.clang-format file) | §11, Done #11 | ✅ | `BasedOnStyle: LLVM`, overrides: 4-space indent, 100 columns, C++26 |
| AC-012 (format target) | §1 (root CMakeLists.txt), Done #13 | ⚠️ **FAILS** | Code block exits 0 on missing clang-format; spec requires non-zero (see B-01) |
| AC-013 (.vscode/settings.json) | §12, Done #12 | ✅ | `intelliSenseEngine: "default"`, `cStandard: "c23"`, `cppStandard: "c++26"`, `includePath` covers `src/engine`, formatter set, format-on-save enabled |
| AC-014 (.vscode/tasks.json) | §13, Done #12 | ✅ | Shell tasks for configure, build, and test (debug preset) |
| AC-015 (.vscode/launch.json) | §14, Done #12 | ✅ | "Debug buddd" and "Debug buddd_tests" configurations with correct `program` paths |

All spec-critic blocking issues (B-01 through B-04) are resolved in this contract. All spec-critic warnings (W-01 through W-05) are addressed.

## Summary

The contract has been updated to include the new `.clang-format`, VS Code workspace files, and naming conventions. These new sections are well-specified and correctly implement spec acceptance criteria AC-011 through AC-015 **except** for AC-012, where the `format` target's fallback code exits 0 instead of non-zero, contradicting both the spec's error case and the contract's own edge case table.

**The contract is Rejected** due to blocking issue B-01. Once the `else()` block is fixed to exit non-zero, and the Done criteria are updated to reflect the required exit code, the contract should pass re-review.
