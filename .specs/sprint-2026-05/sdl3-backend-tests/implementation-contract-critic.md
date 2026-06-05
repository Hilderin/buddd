# Implementation Contract Review — SDL3 Backend Tests

## Status

`Accepted with warnings`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Blocking issues

**No blocking issues found.** The contract is complete, internally consistent on all critical points, and faithfully implements SPEC-003 with the simplified video driver strategy.

### Previously reported (1st contract review — all still resolved)

- [x] **CONST-001 violation** — RESOLVED via AMEND-2026-001. The contract explicitly cites the amendment and confirms the file name falls under the `*_sdl3*.cpp` or similar glob pattern.
- [x] **SPEC-002 AC-015 contradiction** — RESOLVED. The contract does not introduce any new contradictions.
- [x] **Non-goal vs root CMakeLists.txt contradiction** — RESOLVED. The contract correctly modifies root `CMakeLists.txt` and acknowledges the (now-fixed) spec inconsistency.
- [x] **AC-011 vs A-05/Q-03 (wrong file for option definition)** — RESOLVED. The contract defines `option(BUDDD_HAS_DISPLAY ...)` in root `CMakeLists.txt`.

## Warnings

Non-blocking concerns for awareness. Some are carried forward from the 1st review; others are new findings in the updated contract.

### Previously reported (1st review — status noted)

- [x] **W-01 — `target_compile_definitions` not explicitly specified** — RESOLVED. Present in section 2.
- [x] **W-02 — Offscreen fallback mechanism is underspecified** — RESOLVED. Simplified approach with no fallback is now correctly reflected in sections 3f, 3g.
- [x] **W-03 — Tag naming conventions** — RESOLVED. All 6 test cases carry `[sdl3]` plus subsystem tags.
- [x] **W-04 — `SDL_SetHint` return value handling** — RESOLVED. Covered in edge cases section.
- [x] **W-05 — Render device tests without `[!mayfail]`** — NOT ADOPTED (acceptable per spec-critic verdict).
- [x] **W-06 — Multi-test file pattern** — NOT ADOPTED (acceptable).
- [x] **Spec Out of scope / Non-goals contradiction (root CMakeLists.txt)** — The contract acknowledged this in its "Source spec" section. However, the current spec version has *already fixed* this contradiction. See new finding W-08 below.

### Previously reported — carried forward (still applicable)

- [ ] **W-07 — File name pattern under AMEND-2026-001** — Still applicable. The contract's "Relevant constitution rules" section argues that `sdl3_backend_test.cpp` is "deemed 'similar'" to the `*_sdl3*.cpp` glob pattern via the amendment's "or similar" clause. This relies on a judgment call. A strict glob match would require `backend_sdl3_test.cpp` or similar. The contract faithfully documents the intended interpretation. _(Carried from 1st review.)_

- [ ] **W-08 — Include order convention vs actual code** — Still applicable. The conventions table (line 97) states "Standard library headers last; engine headers first; Catch2 header between them." The actual code in section 3a places `<SDL3/SDL.h>` (system-style header) before the engine headers (`"error.h"`, etc.). This is a minor convention inconsistency but acceptable because the contract provides exact verbatim code, so the Code Agent has no ambiguity. _(Carried from 1st review.)_

### New findings (2nd review)

- [ ] **W-09 — Stale line reference in "Source spec" section** — The contract's "Source spec" section (line 23) states: _"One non-blocking consistency fix remains (spec Out of scope section line 227 mentions 'No changes to the root `CMakeLists.txt`' which contradicts the non-goal that allows adding the `option()`)."_ However, the **current spec** (SPEC-003, lines 228–238) no longer contains this text. The spec was updated to fix this contradiction before the contract was written. The line reference "line 227" is stale and refers to an old spec version. The contract should either remove this note or update it to reflect the current spec state. The contract's behavior is correct (it modifies root `CMakeLists.txt` per the non-goals), so this does not affect implementation — it is a documentation accuracy issue.

- [ ] **W-10 — Unacknowledged Out of scope contradiction (platform_abstraction_test.cpp)** — The contract's "Source spec" section acknowledges the (now-fixed) root `CMakeLists.txt` contradiction but does **not** acknowledge a remaining spec-level inconsistency: the Out of scope section (line 229–230) states _"No changes to existing test files (`tests/platform_abstraction_test.cpp`, `tests/version_test.cpp`)"_ while the non-goals (line 49) explicitly state _"T-13 is removed from `tests/platform_abstraction_test.cpp`"_. The Out of scope section also includes (line 238) _"T-13 is replaced by the equivalent test in `tests/sdl3_backend_test.cpp`"_ which contradicts the "no changes" statement on the same list. This is a spec-level inconsistency; the contract correctly follows the non-goals (which have higher authority within the spec). The contract should document this known inconsistency for clarity, similar to how it documented the now-fixed root `CMakeLists.txt` issue.

- [ ] **W-11 — CI YAML: compiler installed but not selected** — The CI workflow (section 3) installs `g++-14` via `apt` but does **not** set the compiler for CMake. Specifically, the configure step runs `cmake -DBUDDD_HAS_DISPLAY=OFF --preset debug` without `CXX=g++-14` or `-DCMAKE_CXX_COMPILER=g++-14`. The CMake presets (`CMakePresets.json`) do not specify a compiler either. On `ubuntu-latest`, the default `g++` is typically GCC 13 or older, which may not fully support `-std=c++26` (as required by the root `CMakeLists.txt` line 4: `set(CMAKE_CXX_STANDARD 26)`). This could cause the CI job to fail at the build step. The fix is straightforward: either pass `-DCMAKE_CXX_COMPILER=g++-14` to the cmake configure command or set the `CXX` environment variable. The spec-critic (line 55) flagged this same concern during the spec review.

- [ ] **W-12 — Wiki out of date** — The wiki page `docs/wiki/engineering/testing.md` (section "SDL3/OpenGL tests (require display)") still references T-12 and T-13 as tests requiring a display and mentions the `[!mayfail]` pattern. After this contract is implemented, T-13 will be removed and replaced with the dummy-driver test in `sdl3_backend_test.cpp`. The contract correctly excludes wiki changes from scope (Non-goals: "No modification of wiki pages"), so this is not a contract defect — but the wiki will become stale after implementation. This should be addressed in a follow-up or as part of the implementation verification.

## Required changes

No blocking changes required. Implementation may proceed.

The following changes are strongly recommended before the contract moves to `Accepted` (non-blocking but important):

1. **Fix CI compiler selection (W-11):** Add `-DCMAKE_CXX_COMPILER=g++-14` to the cmake configure step in `.github/workflows/ci.yml` (section 3), or set `env: { CXX: g++-14 }` on the configure step. Without this, the CI build may fail due to an older default compiler that does not support C++26.

2. **Update stale "Source spec" reference (W-09):** Remove or update the "Source spec" paragraph about the root `CMakeLists.txt` Out of scope contradiction, since the current spec no longer has this issue. This avoids confusion for reviewers reading the spec alongside the contract.

3. **Document platform_abstraction_test.cpp Out of scope contradiction (W-10):** Add a note in the "Source spec" section acknowledging that the spec's Out of scope section (line 229–230) says "No changes to existing test files" while the non-goals change `platform_abstraction_test.cpp`, and that the contract aligns with the non-goals.

## Suggested improvements

Optional ideas (not required):

1. **Consider strict glob pattern match** — Rename `tests/sdl3_backend_test.cpp` to `tests/backend_sdl3_test.cpp` or similar to exactly match the `*_sdl3*.cpp` pattern in AMEND-2026-001, avoiding reliance on the "or similar" clause. _(Carried from 1st review.)_

2. **Clarify include order convention** — Update the conventions table (line 97) to document the SDL3 exception: e.g., "SDL3 system header (`<SDL3/SDL.h>`) may appear before engine headers per AMEND-2026-001." _(Carried from 1st review.)_

3. **Add compile-time verification guard** — Consider adding a `#ifndef BUDDD_HAS_DISPLAY` `#error` guard in the test file to catch accidental compilation without the define. _(Carried from 1st review.)_

4. **Add CI step to verify T-13 removal** — The CI workflow currently only builds and runs tests. Consider adding a step like `grep -n 'T-13\|mayfail' build/debug/tests/buddd_tests --list-tests 2>/dev/null || echo "T-13 symbols absent"` to explicitly verify T-13 is no longer present in the test binary when `BUDDD_HAS_DISPLAY=OFF`. This was suggested in the spec-critic (line 53).

5. **Update wiki after implementation** — Plan a follow-up to update `docs/wiki/engineering/testing.md` to document the new `sdl3_backend_test.cpp` file, the `BUDDD_HAS_DISPLAY` option, and the removal of T-13 from the old test file. This is out of scope for the current contract but should not be forgotten.

## Review summary

| Check | Outcome |
|---|---|
| **Faithful implementation of SPEC-003** | ✅ All 18 ACs (AC-001 through AC-018) are covered. All 6 test cases match the spec's acceptance criteria mapping. |
| **Simplified video driver strategy** | ✅ Platform/window tests use `"dummy"` directly; render/frame tests use `"offscreen"` directly. No fallback logic. Matches spec "Video driver strategy by test type" (lines 116–123). |
| **T-13 removal & replacement** | ✅ T-13 deleted from `platform_abstraction_test.cpp` (section 4). Absorbed as test 3b in `sdl3_backend_test.cpp` with dummy driver, no `[!mayfail]`. Correct. |
| **CI workflow specification** | ✅ Single job with `BUDDD_HAS_DISPLAY=OFF`. (⚠️ Compiler selection issue flagged — see W-11.) |
| **File changes precisely specified** | ✅ Exact content for all 5 file changes (2 new, 3 modified). |
| **Edge cases from spec covered** | ✅ All 11 spec edge cases mapped in contract's edge case table. |
| **Constitution compliance** | ✅ AMEND-2026-001 cited. File name interpretation via "or similar" clause. CONST-002 testing policy respected. |
| **ADR compliance** | ✅ ADR-001 (`Result<T>` pattern) followed in all tests. |
| **Spec contradictions addressed** | ⚠️ Partially. The (now-fixed) root CMakeLists.txt contradiction has a stale reference. The remaining `platform_abstraction_test.cpp` Out of scope contradiction is not documented. See W-09, W-10. |
| **No undocumented decisions** | ✅ Every design choice traces to the spec or constitution amendment. |
| **Done criteria verifiable** | ✅ 14 concrete, measurable done criteria with copy-paste verification commands. |
| **CI compiler selection** | ⚠️ `g++-14` installed but not selected. See W-11. |
| **Previous warnings carried forward** | ⚠️ File name pattern (W-07) and include order (W-08) remain as non-blocking concerns. |

## Change log

| Review | Verdict | Key findings |
|---|---|---|
| 1st | `Accepted` | No blocking issues. Minor warnings about file name, include order. |
| 2nd (this) | `Accepted with warnings` | 3 new non-blocking findings: stale spec reference (W-09), unacknowledged Out of scope contradiction (W-10), CI compiler not selected (W-11). Wiki staleness noted (W-12). Previous warnings carried forward. |

**Verdict**: The implementation contract is thorough, complete, and faithfully implements SPEC-003 with the simplified video driver strategy. No blocking issues exist. Three new non-blocking warnings have been identified (stale spec reference, unacknowledged spec contradiction, CI compiler selection). The most actionable is the CI compiler selection (W-11), which could cause CI failures if left unaddressed. The contract is ready for implementation, with the recommended fixes applied before final `Accepted` status.
