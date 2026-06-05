# Workflow Coordination: capture-frame-validation

## Orchestrator

**Feature**: `capture-frame-validation`
**Status**: completed
**Current step**: done
**Initial instructions**: Ajuster l'interaction entre `--capture` et `--frame` dans la CLI : (1) si `--capture` sans `--frame`, auto-mettre `--frame` au plus grand frame effectif de capture ; (2) si `--frame` < plus grand frame effectif de capture, exit avec erreur ; (3) extraire la constante du driver quirk (effective_frame) dans `CaptureSpec` pour la réutiliser.
**Notes**: L'utilisateur a confirmé que le calcul du "plus grand frame" doit utiliser le **frame effectif** (max(spec.frame, 2) à cause du driver quirk OpenGL). L'auto-set quand `--capture` est seul a été confirmé. L'erreur quand `--frame` < max frame effectif a été confirmé comme "exit 1 avec message d'erreur".
**Loop history**: 
  - Loop #1 (impl-contract-author): blocking issue from impl-contract-critic — row 928 in IMPL-008 edge case table incorrectly says "no update needed" when IMPL-009 changes the behavior fundamentally (frame > frame_limit no longer silently produces nothing; either auto-set guarantees capture or error is returned).

## spec-author

**Status**: completed
**Summary**:
Created SPEC-009 amendment spec for capture-frame validation. Covers: (1) `CaptureSpec::effective_frame()` method, (2) auto-set `--frame` when `--capture` is given without `--frame`, (3) error when explicit `--frame` is smaller than max capture effective frame, (4) preserved driver quirk, (5) testable acceptance criteria with 12 AC items.
**Artifacts**:
- `docs/specs/capture-frame-validation/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
SPEC-009 reviewed against DoR, SPEC-008, constitution, and wiki. Spec is well-structured, internally consistent, and covers all three requested behaviors. No blocking issues found. Two warnings: (1) DoR criterion "existing docs to update" not explicitly listed, (2) AC-011 relies on manual code inspection. Verdict: accepted.
**Artifacts**:
- `docs/specs/capture-frame-validation/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- DoR: existing documentation updates not explicitly listed (wiki business-rules.md is already consistent, but no formal list of docs to update)
- AC-011 relies on manual code inspection; consider compile-time or CI check
- Minor consistency: AC-005 uses substring match while SC-002 uses full-string contains (compatible, implementation contract should be explicit)
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
Fixed 4 issues identified by critic: (1) Row 928 behavior description now correctly states implicit/explicit scenarios rather than "no change needed", (2) test file location pinned to `tests/capture_frame_tests.cpp`, (3) removed reference to non-existent AP test patterns, (4) integration tests use `temp_filename()` with unique prefixes to avoid path collision.
**Artifacts**:
- `docs/specs/capture-frame-validation/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
Re-review #2: All previous issues resolved. Row 928 fix is correct — contract now states it must be updated. Test file pinned to `tests/capture_frame_tests.cpp`. AP test patterns removed. Integration tests use unique `temp_filename()` prefixes. Full re-validation against SPEC-009, CONST-002, wiki, and IMPL-008 finds no new issues. Verdict: accepted.
**Artifacts**:
- `docs/specs/capture-frame-validation/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-05 13:26:43 EDT
**Warnings**:
none
**Notes**: Human approved via question tool. All 5 changes accepted as specified.

## code-implementer

**Status**: completed
**Summary**:
Implemented IMPL-009: added `CaptureSpec::effective_frame()` method to `app_config.h`, updated `parse_running_args()` in `app_config.cpp` with frame-explicit tracking and auto-set/validation logic, replaced inline quirk expression in `app.cpp` with `spec.effective_frame()`, created new `tests/capture_frame_tests.cpp` with 18 test cases (EF-01–EF-05, CF-01–CF-13), and updated IMPL-008 edge case table rows 928–929. Also relaxed `--frame 0` validation (changed `n < 1` to `n < 0`) to support SPEC-009's explicit-interactive-with-captures scenario. All 366 test cases pass.
**Artifacts**:
- Modified: `src/cmd/app_config.h`
- Modified: `src/cmd/app_config.cpp`
- Modified: `src/cmd/app.cpp`
- Created: `tests/capture_frame_tests.cpp`
- Modified: `tests/CMakeLists.txt`
- Modified: `docs/specs/cli-app-system/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
- Changed `--frame` validation from `n < 1` to `n < 0` to allow `--frame 0` (explicit interactive with captures), per SPEC-009 AC-008. This is a behavioral change from SPEC-008 which rejected `--frame 0` as invalid. The error message was also updated from "positive integer" to "non-negative integer".
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**:
Implementation accepted. All 12 AC items satisfied. 18 new tests pass, all 366 tests pass. Build succeeds. Render tests confirm auto-set and error behavior. Constitution CONST-002 satisfied. No blocking issues.
**Artifacts**:
- `docs/specs/capture-frame-validation/code-review.md`
**Questions for human**:
none
**Warnings**:
- `tests/CMakeLists.txt` modified (not in allowed file list) but necessary for test compilation — minor scope expansion, non-blocking.
- `--frame` validation changed from `n < 1` to `n < 0` allowing `--frame 0`, per SPEC-009 AC-008. Behavioral change from SPEC-008.
**Blocking issues**:
none

## adr-agent

**Status**: completed
**Summary**:
No new ADR needed. The four changes (effective_frame() method, auto-set --frame, error on small --frame, relaxed --frame 0 validation) are implementation-level refinements within the existing architecture established by ADR-014. None touch architecture boundaries, module structure, cross-cutting policies, or any other ADR trigger.
**Artifacts**:
- none
**Decisions needed**:
No new architectural decisions required. Reasoning: (1) effective_frame() extracts an inline constant already documented in ADR-014's "Frame numbering dualism" consequence — pure refactoring within bounds. (2) Auto-set and error validation are CLI argument parsing logic within the app_config.h/cpp module that ADR-014 already established. (3) Relaxed --frame validation (n<1→n<0) is a minor input rule change, not an architectural decision. All changes are refinements wholly inside the existing architecture.
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## constitution-agent

**Status**: completed
**Summary**:
Reviewed CONST-001 (architecture boundaries), CONST-002 (testing policy), CONST-003 (documentation policy), CONST-004 (security policy), and the engineering principles against the capture-frame-validation changes. The changes add `CaptureSpec::effective_frame()`, auto-set/validation logic in `parse_running_args()`, replace an inline quirk expression in `app.cpp`, and add 18 new tests. No architecture boundaries are crossed (all changes stay within `src/cmd/` and `tests/`; no new platform/graphics/windowing includes). The testing policy is satisfied (all new code is tested). No constitution rule needs to change.
**Artifacts**:
- none
**Changes needed**:
none — the changes are entirely additive within existing `src/cmd/` boundaries, comply with CONST-001 (no platform header leaks), CONST-002 (18 new tests cover all new code), and existing engineering principles. No new durable rule is warranted: the behavior is scoped to CLI argument parsing, which is already governed by the existing testing policy and architecture boundaries. Creating a constitutional rule for CLI argument validation logic would violate the principle "Do not turn every ADR into a constitutional rule" / "Do not create rules from preferences."
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**:
Reviewed the capture-frame-validation feature (SPEC-009 / IMPL-009) against the wiki. The wiki's business-rules.md needed four changes: (1) `--frame` description updated from "N >= 1" to "N >= 0", (2) exit codes table gained a row for the new validation error, (3) driver quirk section updated to reference `CaptureSpec::effective_frame()` method, (4) new subsection added documenting the auto-set and validation behavior. The module map and other wiki pages were already consistent.
**Artifacts**:
- `docs/wiki/domain/business-rules.md` (modified)
**Changes made**:
- Changed `--frame N` flag description from `N >= 1` to `N >= 0` (SPEC-009 allows `--frame 0`)
- Added `| --frame too small for captures | EXIT_FAILURE (1) |` row to exit codes table
- Updated driver quirk section: `effective_frame` → `CaptureSpec::effective_frame()`, formula explicitly names the method
- Added new "Capture-frame auto-set and validation" subsection documenting the auto-set and error behaviors with reference to SPEC-009
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
Cross-document governance validation completed. All gates passed (spec-critic, impl-contract-critic, human approval, code-reviewer, adr-agent, constitution-agent, wiki-agent). No blocking issues found. Implementation faithfully implements SPEC-009 and matches IMPL-009. All constitution rules respected. Verdict: passed.
**Artifacts**:
- `docs/specs/capture-frame-validation/governance-review.md`
**Questions for human**:
none
**Warnings**:
- `tests/CMakeLists.txt` modified outside allowed file list (necessary scope expansion for test compilation)
- `--frame` validation relaxed from `n < 1` to `n < 0` (intentional per SPEC-009 AC-008, behavioral change from SPEC-008)
- SPEC-009 E2E Verification mentions `tests/cmd_tests.cpp` but actual tests are in `tests/capture_frame_tests.cpp` (planning inconsistency resolved by IMPL-009, non-blocking)
**Blocking issues**:
none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## adr-agent` and `## constitution-agent` sections must include their extra fields (`**Decisions needed**` and `**Changes needed**` respectively).
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
