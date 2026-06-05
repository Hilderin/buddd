# Governance Review — capture-frame-validation (SPEC-009 / IMPL-009)

## Summary

Cross-document governance validation for the capture-frame-validation feature. All gates have been passed (spec-critic, implementation-contract-critic, human approval, code-reviewer, adr-agent, constitution-agent, wiki-agent). The implementation faithfully implements SPEC-009, matches IMPL-009, complies with all constitution rules (CONST-001, CONST-002), aligns with ADR-014, and the wiki has been updated to reflect the changes. No blocking issues found. **Verdict: passed.**

## Documents reviewed

| # | Document | Status |
|---|---|---|
| 1 | `docs/specs/capture-frame-validation/coordination.md` | Workflow coordination, all gates checked |
| 2 | `docs/specs/capture-frame-validation/spec.md` (SPEC-009) | Spec: auto-set, validation, effective_frame() |
| 3 | `docs/specs/capture-frame-validation/spec-critic.md` | Reviewed, accepted — no blocking issues |
| 4 | `docs/specs/capture-frame-validation/implementation-contract.md` (IMPL-009) | Contract: detailed implementation instructions |
| 5 | `docs/specs/capture-frame-validation/implementation-contract-critic.md` | Reviewed, accepted — all issues resolved |
| 6 | `docs/specs/capture-frame-validation/code-review.md` | Code review — all 12 AC satisfied, 366 tests pass |
| 7 | `docs/specs/cli-app-system/spec.md` (SPEC-008) | Parent spec — edge cases 524/525 superseded |
| 8 | `docs/specs/cli-app-system/implementation-contract.md` (IMPL-008) | Parent contract — rows 928/929 updated |
| 9 | `src/cmd/app_config.h` | Added `CaptureSpec::effective_frame()` ✅ |
| 10 | `src/cmd/app_config.cpp` | Auto-set and validation logic ✅ |
| 11 | `src/cmd/app.cpp` | Uses `spec.effective_frame()`, no inline quirk ✅ |
| 12 | `tests/capture_frame_tests.cpp` | 18 test cases, all pass ✅ |
| 13 | `docs/wiki/domain/business-rules.md` | Updated: `N>=0`, exit codes, auto-set section ✅ |
| 14 | `docs/adr/014-cli-app-system.md` | Consistent — no new ADR needed ✅ |
| 15 | `docs/constitution/rules/CONST-001-architecture-boundaries.md` | No header violations ✅ |
| 16 | `docs/constitution/rules/CONST-002-testing-policy.md` | All testable code has tests ✅ |
| 17 | `docs/constitution/rules/CONST-003-documentation-policy.md` | TODO rule — not applicable |
| 18 | `docs/constitution/rules/CONST-004-security-policy.md` | TODO rule — no security impact |
| 19 | `docs/constitution/principles.md` | Engineering principles respected ✅ |

## Spec-Contract alignment (SPEC-009 ↔ IMPL-009)

| Requirement | Spec | Contract | Match |
|---|---|---|---|
| `CaptureSpec::effective_frame()` method | SPEC-009 §1 | IMPL-009 §1 | ✅ |
| Auto-set `--frame` when captures present | SPEC-009 §2 | IMPL-009 §2 | ✅ |
| Error when explicit `--frame < max_effective` | SPEC-009 §3 | IMPL-009 §2 | ✅ |
| Driver quirk preserved (frame 1 → frame 2) | SPEC-009 §4 | IMPL-009 §1 | ✅ |
| `--frame 0` with captures succeeds (AC-008) | SPEC-009 AC-008 | IMPL-009 CF-09 | ✅ |
| Error message exact format | SPEC-009 §3 | IMPL-009 §2 | ✅ |
| 12 AC items mapped to tests | SPEC-009 AC table | IMPL-009 test linkage table | ✅ |

**Result:** IMPL-009 faithfully implements SPEC-009 in all aspects. ✅

## Code-Contract alignment (Code ↔ IMPL-009)

| Contract requirement | Code implementation | Match |
|---|---|---|
| `CaptureSpec::effective_frame()` in `app_config.h` | `app_config.h` line 17: `[[nodiscard]] int effective_frame() const` returning `(frame < 2) ? 2 : frame` | ✅ |
| `bool frame_explicit` tracking in `app_config.cpp` | `app_config.cpp` line 13: `bool frame_explicit = false;` set to true when `--frame` parsed (line 34) | ✅ |
| Auto-set when `!frame_explicit` and captures non-empty | Lines 79-80: `args.frame_limit = max_effective;` | ✅ |
| Error when `frame_explicit && frame_limit > 0 && frame_limit < max_effective` | Lines 81-87: returns error with exact format | ✅ |
| Replace inline quirk in `app.cpp` with `spec.effective_frame()` | Line 119: `int effective_frame = spec.effective_frame();` | ✅ |
| EF-01 to EF-05 unit tests | 5 test cases in `capture_frame_tests.cpp` lines 36-54 | ✅ |
| CF-01 to CF-13 validation tests | 13 test cases in `capture_frame_tests.cpp` lines 60-162 | ✅ |
| Update IMPL-008 rows 928-929 | Verified in IMPL-008 lines 928-929 | ✅ |

**Result:** Code matches IMPL-009 precisely. ✅

**Minor scope expansion:** `tests/CMakeLists.txt` was modified (not in IMPL-009's allowed list) — necessary to compile the new test file. Non-blocking.

## ADR consistency

| ADR | Relevance | Assessment |
|---|---|---|
| ADR-014 (CLI App System) | Documents the driver quirk, frame numbering dualism, centralised render loop | All changes are implementation-level refinements within ADR-014's architecture. No new ADR needed. ✅ |
| ADR-001 (Result/Error pattern) | `parse_running_args()` returns `engine::Result<RunningArgs>` | Validation errors use the established pattern. ✅ |

The `effective_frame()` method formalises what ADR-014 documented as inline quirk logic. The auto-set and validation are CLI argument parsing refinements in the module ADR-014 already established. ✅

## Constitution compliance

| Rule | Assessment |
|---|---|
| CONST-001 (Architecture Boundaries) | No SDL3/OpenGL/GLM headers in `src/cmd/`. All changes stay within `app_config.h/.cpp`, `app.cpp`, and `tests/`. `effective_frame()` is pure computation. ✅ |
| CONST-002 (Testing Policy) | 18 new tests cover all new testable code: `effective_frame()` (EF-01–EF-05), auto-set/validation (CF-01–CF-13). All 366 tests pass. ✅ |
| CONST-003 (Documentation Policy) | Rule is TODO — not enforceable. No violation. |
| CONST-004 (Security Policy) | Rule is TODO — not enforceable. No security impact. |
| Engineering principles | Explicit contracts ✅, small scoped change ✅, existing conventions ✅, testable AC ✅, no contradictions ✅ |

## Wiki consistency

The wiki (`docs/wiki/domain/business-rules.md`) was updated by the wiki-agent with 4 changes:
1. `--frame N` description changed from `N >= 1` to `N >= 0`
2. Exit codes table: added `--frame too small for captures` row
3. Driver quirk section: references `CaptureSpec::effective_frame()` method
4. New "Capture-frame auto-set and validation" subsection

All changes are consistent with SPEC-009 and the implementation. The wiki references SPEC-009. ✅

## Cross-reference coherence

| Reference chain | Coherent? |
|---|---|
| coordination.md → spec.md → spec-critic.md | ✅ |
| coordination.md → implementation-contract.md → implementation-contract-critic.md | ✅ |
| implementation-contract.md → spec.md (source) | ✅ |
| implementation-contract.md → ADR-014, ADR-001 | ✅ |
| implementation-contract.md → CONST-002 | ✅ |
| code-review.md → SPEC-009 AC items → IMPL-009 | ✅ |
| wiki → SPEC-009 | ✅ |
| coordination.md → all sub-agents | ✅ |

## Workflow completeness

| Gate | Status |
|---|---|
| spec-author | ✅ completed |
| spec-critic | ✅ completed, accepted |
| Human Validation | ✅ approved (Hilderin, 2026-06-05) |
| implementation-contract-author | ✅ completed |
| implementation-contract-critic | ✅ completed, accepted |
| code-implementer | ✅ completed |
| code-reviewer | ✅ completed, accepted |
| adr-agent | ✅ completed, no new ADR |
| constitution-agent | ✅ completed, no changes needed |
| wiki-agent | ✅ completed, wiki updated |
| governance-reviewer | ✅ completed (this review) |

## Findings

### Blocking issues

None.

### Warnings

- [ ] **`tests/CMakeLists.txt` modified outside allowed file list**: IMPL-009 did not list `tests/CMakeLists.txt` as an allowed change, but it was modified to compile the new `capture_frame_tests.cpp` test file. This is necessary and correct (adding `app_config.cpp` to the test executable and `src/cmd` include directory). Non-blocking scope expansion, already flagged by the code-reviewer.

- [ ] **`--frame` validation relaxed from `n < 1` to `n < 0`**: The validation change (`--frame 0` now allowed) is intentional per SPEC-009 AC-008, but represents a behavioral change from SPEC-008 where `--frame 0` was rejected. The error message was also updated from "positive integer" to "non-negative integer". This was flagged by both the code-implementer and code-reviewer. Consistent across documents, but worth noting as a behavioral delta from the parent spec.

- [ ] **SPEC-009 E2E Verification mentions `tests/cmd_tests.cpp`**: SPEC-009 §E2E Verification says tests are in `tests/cmd_tests.cpp`, but the actual tests are in `tests/capture_frame_tests.cpp` (as specified by IMPL-009). This is a minor planning inconsistency in the spec that was correctly resolved in the implementation contract. Non-blocking.

### Overall Verdict

**PASSED** — All governance checks pass. The feature is coherent across all documents, respects the constitution, aligns with existing ADRs, and follows proper workflow gates. No blocking issues.
