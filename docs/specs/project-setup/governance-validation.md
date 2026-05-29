# Governance Validation — Project Setup: Buddd Engine Bootstrap

## Status

`Accepted with warnings`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Workflow gate verification

| Gate | Artifact | Status |
|------|----------|--------|
| 1. Spec authored | `docs/specs/project-setup/spec.md` (SPEC-001) | ✅ Accepted |
| 2. Spec critiqued | `docs/specs/project-setup/spec-critic.md` | ✅ Accepted with warnings (all blocking issues resolved) |
| 3. Contract authored | `docs/specs/project-setup/implementation-contract.md` (IMPL-001) | ✅ Accepted |
| 4. Contract critiqued | `docs/specs/project-setup/contract-critic.md` | ✅ Accepted (B-01 resolved, then accepted) |
| 5. Human approved | Approval section in spec and contract | ✅ Authorized by Guillaume on 2026-05-29 |
| 6. Code implemented | 14 files under `CMakeLists.txt`, `CMakePresets.json`, `src/`, `tests/`, `.clang-format`, `.vscode/` | ✅ All files exist with correct content |
| 7. Code reviewed | `docs/specs/project-setup/code-review.md` | ✅ Accepted with warnings |
| 8. Governance validated | This document | ✅ Complete |

All gates passed. The workflow was followed correctly.

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **GOV-001: `enable_testing()` added to `CMakeLists.txt` without contract authorization**  
  *Severity: Non-blocking.* The root `CMakeLists.txt` contains `enable_testing()` at line 20, which is **not present** in the implementation contract's template (section 1). The code review (W-01) identified this as a pragmatically necessary addition — without it, `catch_discover_tests()` (via `include(Catch)`) would register zero tests with CTest.  
  *Status:* The addition is correct and necessary. The contract should be updated to include `enable_testing()` in the next revision, as recommended by the code review. Not a blocking issue.

- [x] **GOV-002: Done criterion #9 (`buddd_editor` direct build) fails with Ninja**  
  *Severity: Non-blocking.* The implementation contract's Done criterion #9 requires `cmake --build --preset debug --target buddd_editor` to succeed, but INTERFACE-only libraries (no sources) do not produce Ninja build rules. The spec AC-009 (which requires the full build to succeed with no editor binary) is satisfied. Only the contract's own internal criterion is affected.  
  *Status:* Documented in code review (W-02). The target is correctly defined and serves its structural purpose. A future contract revision should update this criterion to reflect the Ninja limitation.

- [x] **GOV-003: Spec Q-02 remains `[NEEDS CLARIFICATION]` despite contract resolution**  
  *Severity: Non-blocking.* Spec open question Q-02 asks whether the version function should be `constexpr`/`consteval`/runtime/macro. The contract resolves it as `auto version() -> std::string_view` (runtime function), but the spec has not been updated from `[NEEDS CLARIFICATION]` to `[RESOLVED]`. Already flagged by spec-critic (W-02).  
  *Status:* The contract provides the binding constraint, so this is a documentation gap in the spec, not a workflow failure.

- [x] **GOV-004: `CMAKE_CXX_STANDARD` duplicated in `CMakeLists.txt` and `CMakePresets.json`**  
  *Severity: Non-blocking.* Both the root `CMakeLists.txt` (line 4) and `CMakePresets.json` (cache variables) set `CMAKE_CXX_STANDARD` to `26`. Both yield the same effective value, so there is no runtime conflict, but this creates a maintenance hazard. Already flagged by contract-critic (W-02).  
  *Status:* Acknowledged. Recommend deduplication in a future cleanup pass.

- [x] **GOV-005: PascalCase convention contradicts actual test case name**  
  *Severity: Non-blocking.* The contract's "Existing conventions to follow" section states "Use `PascalCase` for Catch2 test case names", but the required test case is `"engine version is non-empty"` (sentence case). The contract includes a note that the explicit code block takes precedence. Already flagged by contract-critic (W-01).  
  *Status:* Minor internal contradiction. No functional impact.

## Constitution violations

Checks against `docs/constitution/**`:

- [x] **CONST-001 (Architecture Boundaries)** — TODO placeholder with no active rule. ✅ No violation.
- [x] **CONST-002 (Testing Policy)** — "All testable code added or modified in this project must have corresponding unit tests. Those tests must pass."  
  ✅ Satisfied: One test (`"engine version is non-empty"`) exists for the only testable API (`buddd::engine::version()`), and it passes.
- [x] **CONST-003 (Documentation Policy)** — TODO placeholder with no active rule. ✅ No violation.
- [x] **CONST-004 (Security Policy)** — TODO placeholder with no active rule. ✅ No violation.
- [x] **Principles: Governance documents must not contradict each other**  
  The minor inconsistencies identified (GOV-003, GOV-004, GOV-005) are within individual documents or between contract and implementation, not between governance documents (constitution, ADRs). ✅ No violation.

**No constitution violations found.**

## ADR alignment

Required ADRs exist or are proposed:

- [x] No ADR is required for this bootstrap phase — confirmed by:
  - Implementation contract: *"No accepted ADRs exist. This contract does not require an ADR."*
  - Wiki ADR index: *"No Architecture Decision Records (ADRs) have been accepted yet."*
  - All bootstrap decisions (static library, FetchContent for Catch2, INTERFACE editor, CLI output format) are reversible and do not constrain future architecture irreversibly.

**ADR alignment: ✅ Confirmed — no ADR needed.**

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] `docs/wiki/engineering/setup.md` — Accurate prerequisites, quick start, build presets, formatting, and VS Code integration. References SPEC-001 and IMPL-001 correctly.
- [x] `docs/wiki/architecture/overview.md` — Directory layout, build system, CMake targets, and key behaviors match implementation exactly.
- [x] `docs/wiki/architecture/module-map.md` — Target descriptions, file roles, and naming conventions match contract.
- [x] `docs/wiki/architecture/dependency-map.md` — Dependencies, toolchain requirements, and constraints match contract.
- [x] `docs/wiki/architecture/data-flow.md` — CLI behavior and test flow match spec and implementation.
- [x] `docs/wiki/engineering/testing.md` — Framework, running tests, current tests, and adding tests match spec/contract.
- [x] `docs/wiki/engineering/troubleshooting.md` — Error/symptom tables match spec edge/error cases.
- [x] `docs/wiki/domain/business-rules.md` — CLI output behavior, version API contract, and conventions match spec.
- [x] `docs/wiki/domain/glossary.md` — All terms match spec/contract definitions.
- [x] `docs/wiki/decisions/adr-index.md` — Correctly states no ADRs exist.
- [x] `docs/wiki/decisions/constitution-index.md` — Correctly lists constitution rules and their status.
- [x] `docs/wiki/engineering/deployment.md` — Correctly states deployment is not applicable at bootstrap.
- [x] `docs/wiki/README.md` — Explicitly states "It is not a source of mandatory rules." ✅ Correct.

**Wiki alignment: ✅ Fully aligned. Wiki accurately reflects current state without creating law.**

## Approval date consistency

| Document | Date | Time | Match? |
|----------|------|------|--------|
| SPEC-001 (spec) | 2026-05-29 | ~10:30 UTC | — |
| IMPL-001 (contract) | 2026-05-29 | ~10:30 UTC | ✅ Matches spec |
| Spec-critic | (not dated) | — | N/A (review artifact) |
| Contract-critic | Cycle 2: 2026-05-29 | — | ✅ Consistent |
| Code review | (not dated) | — | N/A (review artifact) |

**Approval dates: ✅ Consistent across all dated documents.**

## Implementation contract deviation: `enable_testing()` addition

The code review (W-01) documents that `enable_testing()` was added to the root `CMakeLists.txt` without appearing in the implementation contract's template. This deviation is:

- **Verified**: Present at line 20 of `CMakeLists.txt`, absent from contract section 1.
- **Pragmatically necessary**: `catch_discover_tests()` requires `enable_testing()` in the root `CMakeLists.txt`. Without it, CTest reports "No tests were found!!!" — a silent failure.
- **Documented**: The code review (W-01) explicitly captures this deviation and recommends accepting it.
- **Recommended action**: Update the implementation contract's root `CMakeLists.txt` template (section 1) to include `enable_testing()` before `add_subdirectory(tests)` in the next revision.

## Warnings

Non-blocking concerns for awareness:

1. **`enable_testing()` gap**: The implementation contains a pragmatically necessary function call that the contract template does not authorize. Recommended for contract update.
2. **`buddd_editor` Ninja limitation**: Done criterion #9 is technically unverifiable with Ninja because INTERFACE-only libraries produce no build rules. Documented in code review (W-02).
3. **Spec Q-02 unclosed**: The spec still lists the version function API question as `[NEEDS CLARIFICATION]` even though the contract resolved it. Should be updated to `[RESOLVED]` for future readers.
4. **`CMAKE_CXX_STANDARD` duplication**: Set in both `CMakeLists.txt` and `CMakePresets.json`. Maintenance hazard but no functional conflict.
5. **PascalCase convention mismatch**: The conventions section states PascalCase for test names, but the actual test uses sentence case. Minor internal contradiction.

None of these are blocking. They are documentation polish items.

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

- None required at this time. The constitution has no active rules beyond CONST-002 (Testing Policy), which is satisfied. No ADRs are needed. The wiki is fully aligned.

**Suggested (non-blocking) updates for next revision cycle:**

- Update the implementation contract's root CMakeLists.txt template (section 1) to include `enable_testing()` before `add_subdirectory(tests)`.
- Update the implementation contract's Done criterion #9 to acknowledge the Ninja INTERFACE-library limitation.
- Close Q-02 in the spec by marking it `[RESOLVED]` and noting the contract's decision (`auto version() -> std::string_view`).
- Either deduplicate `CMAKE_CXX_STANDARD` or add a comment in `CMakeLists.txt` noting the dual-source-of-truth.
- Either update the PascalCase convention or rename the test case to match the stated convention.

## Final verdict

**Approved with warnings.**

The project bootstrap (SPEC-001 / IMPL-001) is complete, coherent, and constitution-compliant. All 14 required files exist with correct content. The build system configures, builds, and runs correctly in both Debug and Release presets. The single unit test passes via CTest. The CLI outputs match the spec exactly. The wiki has been comprehensively updated and is fully aligned.

The two warnings from the code review (enable_testing, buddd_editor Ninja limitation) are non-blocking and documented. The remaining concerns are minor documentation inconsistencies that do not affect correctness or constitutionality.

The implementation is ready for commit and subsequent feature development.
