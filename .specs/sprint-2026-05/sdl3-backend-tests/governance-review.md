# Governance Review — SDL3 Backend Tests (SPEC-003 / IMPL-003)

## Status

`Accepted with warnings`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Cross-document consistency

Analysis of document alignment between spec, contract, code, reviews, and constitution.

### Spec vs Implementation Contract

| Check | Result | Notes |
|---|---|---|
| All 18 ACs (AC-001–AC-018) mapped in contract | ✅ | Every acceptance criterion has a corresponding implementation requirement |
| Video driver strategy matches | ✅ | Both spec and contract use `"offscreen"` for all tests. No fallback/retry logic. |
| T-13 removal specified and implemented | ✅ | Spec non-goals (line 49), contract section 4, and code all agree: T-13 deleted from `platform_abstraction_test.cpp` |
| CMake option location agrees | ✅ | Root `CMakeLists.txt` before `enable_testing()`, verified in implementation |
| Conditional compilation guard agrees | ✅ | `#ifdef BUDDD_HAS_DISPLAY` wrapping entire test file; `target_compile_definitions` in `if(BUDDD_HAS_DISPLAY)` block |
| CI workflow matches spec description | ✅ | Single job with `BUDDD_HAS_DISPLAY=OFF`, `--preset debug`, `--output-on-failure` |
| Constant name mismatch (contract typo) | ⚠️ | Contract sections 3f/3g use `SDL_HINT_VIDEODRIVER` (no underscore). Implementation correctly uses `SDL_HINT_VIDEO_DRIVER` per spec. The code review flagged this as a non-blocking contract typo. The implementation is correct. |

### Spec Internal Contradiction

| Issue | Detail |
|---|---|
| Out of scope vs non-goals conflict | The spec Out of scope section (line 229) states *"No changes to existing test files (`tests/platform_abstraction_test.cpp`)"* while the non-goals (line 49) explicitly state *"T-13 is removed from `tests/platform_abstraction_test.cpp`"*. These directly contradict. The contract correctly follows the non-goals (which have higher authority within the spec). **This is a pre-existing spec issue, not a contract or implementation defect.** |

### Implementation Contract vs Code

| Check | Result | Notes |
|---|---|---|
| Exact file contents match | ✅ | `tests/sdl3_backend_test.cpp` matches contract sections 3a–3h (with only the intentional constant name fix) |
| CMakeLists.txt structure | ✅ | Root: `option(BUDDD_HAS_DISPLAY ...)` before `enable_testing()`. Tests: conditional block with status messages, `target_compile_definitions`, `target_link_libraries` outside conditional. |
| CI YAML content | ✅ | Matches contract section 3. Includes `-DCMAKE_CXX_COMPILER=g++-14` (resolves contract-critic W-11). |
| T-13 removal clean | ✅ | `tests/platform_abstraction_test.cpp` no longer contains T-13. No other changes to the file. Verified by code review. |
| Option description wording | ✅ | Root `CMakeLists.txt` line 20: `"Enable SDL3 backend tests (requires display or offscreen driver)"`. Matches contract. (The code review flagged that this said "dummy driver" — it has since been corrected.) |

### Code Review Verdict

| Aspect | Result |
|---|---|
| Code review status | `Accepted with warnings` (not `Rejected`) |
| Blocking issues | None — all 18 ACs satisfied, builds succeed, tests pass in both ON/OFF configurations |
| Non-blocking issues | 2 found, both resolved or acceptable: (1) CMake option description stale wording — now fixed; (2) contract constant name typo — implementation correct, code review notes it |
| Review confirms no `CHECK` macros, proper tags, proper guards | ✅ |

- [x] **Contract typo in sections 3f/3g**: Uses `SDL_HINT_VIDEODRIVER` instead of `SDL_HINT_VIDEO_DRIVER`. Implementation correctly uses the spec's constant. Non-blocking — the implementation is correct.

## Constitution compliance

### CONST-001 — Architecture Boundaries (with AMEND-2026-001)

| Requirement | Status | Evidence |
|---|---|---|
| No code outside `src/engine/` may include platform/graphics/windowing library headers | ✅ Exception applied | AMEND-2026-001 is ratified (2026-05-29) and appended to CONST-001 |
| Exception applies only to SDL3 test files (`tests/*_sdl3*.cpp` or similar) | ✅ | `tests/sdl3_backend_test.cpp` is covered by the "or similar" clause (A-11 in spec) |
| Exception applies only when `BUDDD_HAS_DISPLAY=ON` | ✅ | Entire test file is wrapped in `#ifdef BUDDD_HAS_DISPLAY` |
| SDL3 include used only for `SDL_SetHint()` | ✅ | All 6 test cases call `SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")`. No other SDL3 APIs are called directly. |
| No other SDL3 headers or APIs used | ✅ | Only `<SDL3/SDL.h>` is included. The test uses engine abstractions (`Platform`, `Window`, `RenderDevice`) for all other operations. |
| Code review verified compliance | ✅ | Code review confirms "Constitution-compliant SDL3 include" |

**Verdict**: CONST-001 is fully satisfied. The amendment is correctly applied and the implementation respects its narrow scope.

### CONST-002 — Testing Policy

| Requirement | Status | Evidence |
|---|---|---|
| All testable code must have corresponding unit tests | ✅ | SDL3 backend was previously untested (only T-12 enum check). Now 6 runtime tests cover the full lifecycle. |
| Those tests must pass | ✅ | All 19 tests pass with `BUDDD_HAS_DISPLAY=ON` (6 SDL3 + 12 headless + 1 sanity). All 13 pass with `OFF`. Verified by code review. |

**Verdict**: CONST-002 is fully satisfied.

### Engine code integrity

| Check | Result | Evidence |
|---|---|---|
| No `src/engine/` files modified | ✅ | `git diff --name-only` shows no `src/engine/` files. Verified by code review. |
| No forbidden files changed | ✅ | `CMakePresets.json`, `tests/version_test.cpp` unchanged. Verified. |
| No CC engine APIs added | ✅ | Tests use existing `Platform::create()`, `create_window()`, `RenderDevice::create()` APIs only. |

**Verdict**: Engine code boundaries fully respected.

## Workflow compliance

### Gate verification

| Gate | Status | Evidence |
|---|---|---|
| **Spec approved by human** | ✅ | Spec.md § Approval: Approved by Guillaume, 2026-05-29 17:52 UTC. Status: `Accepted`. |
| **Spec critic passed** | ✅ | spec-critic.md: 3rd review verdict `Accepted`. All 4 blocking issues resolved. No new issues. |
| **Contract approved by human** | ✅ | implementation-contract.md § Approval: Approved by Guillaume, 2026-05-29 17:52 UTC. Status: `Accepted`. |
| **Contract critic passed** | ✅ | implementation-contract-critic.md: 2nd review verdict `Accepted with warnings`. No blocking issues. 3 non-blocking warnings (W-09, W-10, W-11). W-11 (CI compiler selection) resolved in implementation. |
| **Code review passed** | ✅ | code-review.md: Verdict `Accepted with warnings`. No blocking issues. Builds and tests verified in both configurations. |

### Workflow rule compliance

| Authority Order Rule | Status | Evidence |
|---|---|---|
| Code implemented only from accepted contract | ✅ | Implementation matches IMPL-003 exact content. |
| Contract not ambiguous — no escalation needed | ✅ | Contract is precise (exact file contents specified). |
| No constitution violations | ✅ | See Constitution compliance section above. |
| No silent architecture changes | ✅ | All changes are documented in spec, contract, and reviews. |
| No direct modification of constitution | ✅ | Amendment was properly ratified before implementation. |
| No ADRs rewritten | ✅ | ADR-001 unchanged. No new ADRs needed (confirmed in contract § ADR impact). |
| Wiki not contradicted (but stale) | ⚠️ | See Wiki alignment section below. |

**Verdict**: Standard workflow was followed correctly. Spec → spec-critic → contract → contract-critic → code → code-review gates were all passed.

## ADR alignment

| ADR | Requirement | Status | Evidence |
|---|---|---|---|
| ADR-001 | `Result<T>`/`Error` pattern for all fallible APIs | ✅ | All tests use `REQUIRE(platform.has_value())`, `REQUIRE(window.has_value())`, `REQUIRE(device.has_value())`. No exceptions used. |
| ADR-001 | `make_error()`, `to_string()` helpers | ✅ | Not directly tested in SDL3 tests, but the pattern is used by the engine APIs being tested. |
| New ADRs required? | None | ✅ | Contract § ADR impact: "No architectural decision requires an ADR." Confirmed. |

**Verdict**: ADR-001 is fully respected. No new ADRs needed.

## Wiki alignment

| Check | Status | Detail |
|---|---|---|
| Wiki reflects current state | ⚠️ | `docs/wiki/engineering/testing.md` § "SDL3/OpenGL tests (require display)" still references T-13 as an existing test with `[!mayfail]` tag. After this implementation, T-13 is removed and replaced by 6 offscreen-driver-based tests. The wiki is stale. |
| Wiki contradicts constitution | ✅ | No contradictions found. |
| Wiki becomes law | ✅ | No — wiki is at authority level #4. The spec and constitution take precedence. |

**Note**: The implementation contract explicitly excluded wiki changes (non-goal: "No modification of wiki pages"). This is correctly out of scope, but the wiki should be updated as a follow-up task to accurately describe the current test landscape.

**Recommendation**: Update `docs/wiki/engineering/testing.md` to:
- Replace the "SDL3/OpenGL tests (require display)" section with the new SDL3 offscreen-driver test structure
- Document the `BUDDD_HAS_DISPLAY` CMake option
- Remove references to T-13 and `[!mayfail]`
- Add the 6 new test cases and their tags

## Issues found

### Blocking issues

None. All previous blocking issues from earlier review cycles have been resolved.

### Non-blocking issues

- [ ] **SPEC-003 internal contradiction: Out of scope vs non-goals (W-10 from contract-critic)** — The spec's Out of scope section (line 229) states *"No changes to existing test files (`tests/platform_abstraction_test.cpp`)"* while the non-goals (line 49) state *"T-13 is removed from `tests/platform_abstraction_test.cpp`"*. These directly contradict. The implementation correctly follows the non-goals. The spec should be corrected to resolve this contradiction.

- [ ] **IMPL-003 contract typo: `SDL_HINT_VIDEODRIVER` in sections 3f/3g** — The contract specifies `SDL_HINT_VIDEODRIVER` (missing underscore) in the RenderDevice creation and frame cycle test cases. The spec and correct SDL3 API use `SDL_HINT_VIDEO_DRIVER`. The implementation correctly uses `SDL_HINT_VIDEO_DRIVER`. The contract should be corrected if re-reviewed.

- [ ] **IMPL-003 stale "Source spec" reference (W-09 from contract-critic)** — Contract § "Source spec" (line 23) references a spec contradiction about root `CMakeLists.txt` that was already fixed in the current spec version. The reference is stale and should be updated.

- [ ] **Wiki stale: `docs/wiki/engineering/testing.md`** — Still references T-13 and the old `[!mayfail]` / "requires display" paradigm. Should be updated to document: the 6 new offscreen-driver-based tests, the `BUDDD_HAS_DISPLAY` option, and the removal of T-13.

- [x] **CMake option description said "dummy driver" (code review finding)** — Root `CMakeLists.txt` originally had `"dummy driver"` instead of `"offscreen driver"`. **RESOLVED** — the current file shows the correct wording. Verified by file inspection.

- [x] **CI compiler not selected (contract-critic W-11)** — Contract-critic flagged that `g++-14` was installed but not selected. **RESOLVED** — the CI YAML implementation includes `-DCMAKE_CXX_COMPILER=g++-14` in the configure step. Verified by file inspection.

## Final verdict

**ACCEPTED with warnings.**

The implementation of SPEC-003 / IMPL-003 (SDL3 Backend Tests) is governance-compliant and ready for use.

### Why not `Rejected`?
- No blocking issues exist. All constitution rules are satisfied.
- All previous blocking issues (CONST-001 violation, SPEC-002 contradiction, non-goal conflicts) were resolved across the review cycle.
- The implementation matches both the spec and the accepted contract (with only one intentional correction — fixing a constant name typo in the contract).
- Builds and tests pass in both `BUDDD_HAS_DISPLAY=ON` and `OFF` configurations.
- No engine code was modified. No forbidden files were changed.
- The standard workflow (spec → spec-critic → contract → contract-critic → code → code-review → governance-review) was correctly followed.

### Why not `Accepted` without warnings?
Four non-blocking issues remain:
1. A spec-internal contradiction (Out of scope vs non-goals for `platform_abstraction_test.cpp`)
2. A contract typo (SD constant name in 2 sections)
3. A stale reference in the contract's "Source spec" paragraph
4. Wiki staleness (still references the old testing structure)

These issues do not affect correctness, safety, or compliance, but they should be resolved in a follow-up to maintain document quality.

### Recommended follow-up actions
1. Fix the spec-internal contradiction (Out of scope section vs non-goals).
2. Correct the contract typo (`SDL_HINT_VIDEODRIVER` → `SDL_HINT_VIDEO_DRIVER`).
3. Update the contract's stale "Source spec" reference.
4. Update `docs/wiki/engineering/testing.md` to reflect the new SDL3 backend test structure.

### Change log

| Review | Verdict | Key findings |
|---|---|---|
| 1st (this) | `Accepted with warnings` | No blocking issues. 4 non-blocking issues: spec internal contradiction, contract typo, stale contract reference, stale wiki. All constitution, workflow, and ADR checks pass. |
