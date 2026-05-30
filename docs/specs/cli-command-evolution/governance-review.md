# Governance Review — CLI Command Evolution: Demo System & Empty Run (SPEC-007 / IMPL-007)

## Status

`Accepted`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec-critic B-01 resolved: Window title case** — Spec-critic identified that Story 1 / AC-007 showed the title `"Buddd Engine — Demo: Triangle"` (capital T) while the User-visible behavior rule said case-preserving. The spec was corrected to consistently use lowercase `triangle` throughout (spec lines 111, 127–131). Contract, code, and tests all match: `demo_command.cpp` constructs the title with the case-preserved demo name. ✓ Resolved.

- [x] **Spec-critic B-02 resolved: CONST-002 / tests optional** — The original spec labelled new tests as "optional" contradicting CONST-002. The spec was corrected (lines 496–514) to state these tests are required by CONST-002, and the "optional" label was removed. The contract (§Required tests) lists all required tests with verification criteria. Four new `[cli]` tests were added to `tests/version_test.cpp`. ✓ Resolved.

- [x] **Spec-critic B-03 resolved: Per-demo argv contract** — The original spec had a contradiction between the per-demo function doc comment (claiming `argv[0]` is the demo name) and the dispatch pseudocode (passing full `argc`/`argv`). The spec was corrected: `DemoCommand` now passes `argc - 2, argv + 2` to per-demo functions (spec line 587, contract line 265), so `argv[0]` is the demo name as documented. Code (`demo_command.cpp` line 83) matches. ✓ Resolved.

- [x] **Spec-critic B-04 resolved: Headless backend contradiction** — The spec's edge case table and Assumption A-10 described the old SDL3-fails-without-display behavior, contradicting the body text which said the headless backend works. The spec was corrected: edge cases 433–434 now state the headless backend succeeds at platform creation, and A-10 now correctly describes the compile-time backend selection. Contract and code match this headless-works behavior. ✓ Resolved.

- [x] **Spec-critic W-07 resolved: CONST-001 grep regex refined** — The AC-018 verification command was refined from `grep -rnE '(SDL3|GL/|glad|glm)' src/cmd/` to `grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/` to avoid false positives from `be::Backend::SDL3` enum values. Contract (CONST-001 compliance section and Done criteria AC-018) was updated accordingly (contract-critic W-04). ✓ Resolved.

- [x] **Spec-critic W-08 resolved: "Display-dependent" → "Backend-sensitive"** — Line 514 (now line 515) was updated from "Display-dependent paths" to "Backend-sensitive paths" to accurately reflect that these paths work with both SDL3 and headless backends. ✓ Resolved.

- [x] **Spec-critic W-01 resolved: Framebuffer clear mechanism** — The spec now documents that `begin_frame()` clears the colour buffer to black as an implementation detail of the OpenGL backend (spec lines 173, 594). Contract (§Required implementation behavior step 10) and code (`run_command.cpp` lines 43–45) follow this: the loop is just `begin_frame()` / `end_frame()` with no draw calls. ✓ Resolved.

- [x] **Spec-critic W-02 resolved: Extra-arguments warning order** — The spec was corrected to place the extra-arguments warning after resource creation (spec §Required implementation behavior, steps 3–4). Contract (§demo_command.cpp lines 252–260) and code (`demo_command.cpp` lines 71–78) both show the warning after resource creation. ✓ Resolved.

- [x] **Spec-critic W-03 resolved: `buddd run extra_arg` edge case** — Added to the edge case table (spec line 437). Contract §Edge cases also lists it. ✓ Resolved.

- [x] **Spec-critic W-04 resolved: `run_command.h` comment updated** — The contract (§Update `run_command.h`, lines 422–440) and the actual header (`src/cmd/commands/run_command.h`) now say "framebuffer clear only (no draw calls)". ✓ Resolved.

- [x] **Spec-critic W-05 resolved: Case-sensitivity note** — Demo usage text now includes `"Demo names are case-sensitive."` (spec lines 146, 161). Contract's `k_demo_usage` constant (line 218) and the actual `demo_command.cpp` (lines 21–27) both include this line. ✓ Resolved.

- [x] **Spec-critic W-06 resolved: Title/message case alignment** — All diagnostic messages and the title example consistently use lowercase `triangle` throughout spec, contract, and code. ✓ Resolved.

- [x] **Contract-critic B-01 resolved: Double-dereference bug** — The contract originally used `*platform, *device` (single dereference) for the call to `run_triangle_demo()`. Fixed to `**platform, **device` (contract §demo_command.cpp lines 265–266). Actual code at `demo_command.cpp` line 83 uses `**platform, **device`. ✓ Resolved.

- [x] **Contract-critic W-01 resolved: Unnecessary include** — `demo_command.cpp` originally included `demo/demo_helpers.h` unnecessarily. Removed from contract and actual code (`demo_command.cpp` includes only `demo/triangle_demo.h`). ✓ Resolved.

- [x] **Contract-critic W-02 resolved: WindowConfig::title comment** — The misleading comment about `WindowConfig::title` being `std::string_view` was corrected to accurately state it is `std::string` (contract lines 255–256, actual `demo_command.cpp` lines 50–51). ✓ Resolved.

- [x] **Contract-critic W-04 resolved: CONST-001 grep pattern** — The contract's CONST-001 compliance section and Done criteria AC-018 were updated from bare regex `(SDL3|GL/|glad|glm)` to `#include.*(SDL3|GL/|glad|glm)` to align with the spec fix (spec-critic W-07). ✓ Resolved.

- [x] **Spec-to-contract-to-code coherence: File structure** — Spec §File structure (lines 265–292), contract §Files to create/remove/modify (lines 52–111), and actual filesystem all match exactly: `test_command.h/.cpp` removed, `demo_command.h/.cpp` created, `demo_helpers.h/.cpp` moved to `src/cmd/demo/`, `triangle_demo.h/.cpp` created, `main.cpp`, `run_command.h/.cpp`, `help_command.h`, `CMakeLists.txt` modified. ✓ Aligned.

- [x] **Spec-to-contract-to-code coherence: Output strings** — All exact output strings (`k_usage_text`, `k_demo_usage`, unknown command, unknown demo, demo started/complete/abort, run stdout messages) match exactly between spec, contract, actual code, and verified CLI output (code review §Output format correctness). ✓ Aligned.

- [x] **Spec-to-contract-to-code coherence: Dispatch logic** — Spec §Required implementation behavior (main.cpp), contract §Update main.cpp (lines 369–415), actual `main.cpp` (lines 1–41), and wiki `data-flow.md` (lines 7–24) all describe the same dispatch: `"run"`, `"demo"`, `"version"`, `"help"` with `"test"` falling through to the unknown-command handler. ✓ Aligned.

- [x] **Spec-to-contract-to-code coherence: Demo dispatch** — `DemoCommand::run()` parses `argv[2]` as demo name, creates platform/window/device (800×600, title "Buddd Engine — Demo: <name>"), warns if `argc > 3`, dispatches to `run_triangle_demo()` for `"triangle"`, prints error + usage for unknown names. Contracts, code, and spec all match. ✓ Aligned.

- [x] **Spec-to-contract-to-code coherence: RunCommand** — No triangle rendering, no `demo_helpers.h` include, no `setup_triangle()` call, loop is `begin_frame()`/`end_frame()` only. Window 1024×768, title "Buddd Engine". All matched. ✓ Aligned.

- [x] **Spec-to-contract-to-code coherence: CMakeLists.txt** — Glob includes `demo/*.cpp` alongside existing `*.cpp` and `commands/*.cpp`. Build succeeds. ✓ Aligned.

- [x] **Spec-to-contract-to-code coherence: CONST-001 compliance** — `grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/` returns zero matches. The only SDL3 references are `be::Backend::SDL3` enum values (engine abstractions, not header includes). ✓ Aligned.

- [x] **Spec-to-contract-to-code coherence: Backend selection** — Both `demo_command.cpp` and `run_command.cpp` use compile-time `#ifdef BUDDD_HAS_DISPLAY` to select backend (`be::Backend::SDL3` when defined, `be::Backend::Headless` when undefined). CMakeLists.txt propagates the define to the `buddd` target (line 624–626). This matches the spec (lines 115–116, 168–169, 580–583) and contract (lines 207–213, 477–483). ✓ Aligned.

- [x] **Spec-to-contract-to-code coherence: Early demo name validation** — `demo_command.cpp` validates the demo name before `Platform::create()` (contract lines 237–241, code lines 51–55). This matches the spec requirement (lines 114–115) to fail fast on CI without attempting display initialization. ✓ Aligned.

- [x] **Test-to-spec coherence** — All 10 CLI tests (6 existing updated + 4 new) map directly to spec ACs: AC-005 (unknown demo), AC-006 (demo no name), AC-007 (demo triangle completes), AC-012 (no-args = run), AC-013 (version), AC-014 (help), AC-015 (unknown command), AC-016 (`buddd test` is unknown), AC-020 (version extra args), AC-021 (help extra args). All tests pass (100/100, including 90 engine tests). ✓ Aligned.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Constitution violations

Checks against `docs/constitution/**`:

- [x] **CONST-001 (Architecture Boundaries)** — SATISFIED. Code review (lines 141–149) and re-verification confirm: `grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/` returns zero matches. Only `be::Backend::SDL3` enum values found — these are engine abstraction constants, not header includes. No SDL3, OpenGL, or GLM headers are included from any file under `src/cmd/`. The existing exception AMEND-2026-001 (SDL3 test files) is unaffected and does not apply to `src/cmd/`. The architecture boundary is preserved.

- [x] **CONST-002 (Testing Policy)** — SATISFIED. All unconditionally testable code paths have corresponding `[cli]` tests:
  - `buddd version` → version test ✓
  - `buddd help` → help test (checks for `"demo"` not `"test"`) ✓
  - `buddd unknowncommand` → unknown command test ✓
  - `buddd version extra_arg` → ignores extra args test ✓
  - `buddd help extra_arg` → ignores extra args test ✓
  - `buddd demo` (no name) → demo no name test ✓
  - `buddd demo unknownname` → unknown demo test ✓
  - `buddd test` is unknown → `buddd test` unknown test ✓
  - `buddd demo triangle` (display-guarded) → display-guarded test ✓
  - `buddd` no-args (display-guarded) → no-args test ✓
  
  All 100 tests (10 CLI + 90 engine) pass. The spec-critic's B-02 (CONST-002 violation from "optional" tests) was fully resolved.

- [x] **CONST-003 (Documentation Policy)** — NOT VIOLATED. This rule remains a TODO placeholder with no substantive rules to enforce. The project's documentation (spec, contract, code review, ADRs, wiki) is comprehensive and internally consistent for SPEC-007/IMPL-007.

- [x] **CONST-004 (Security Policy)** — NOT VIOLATED. This rule remains a TODO placeholder with no substantive rules to enforce. The spec's §Permissions and security and the contract's §Security impact document that no elevated privileges, network access, or credentials are required. No new attack surface is introduced.

- [x] **Engineering Principles** — No contradictions found. Explicit contracts are preferred (spec and contract are detailed with exact output strings). Changes are scoped (CLI refactor only; no engine changes). Existing conventions are followed (snake_case, namespaces, include paths, if/else-if dispatch style). Requirements are testable (all ACs and SCs are verifiable). Governance documents do not contradict each other (verified in this review).

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-001 (Result/Error Pattern)** — ALIGNED. Engine APIs that can fail return `Result<T>`. Command code handles errors via `.error()` with `be::to_string()`. Draw methods return `void` per the ADR-003 exception — this is respected (commands do not call `draw()` with error checking; `draw()` precondition-based contract is followed). The `setup_triangle()` helper uses `std::exit(EXIT_FAILURE)` on fatal errors, which is existing behaviour documented in the spec error cases.

- [x] **ADR-002 (GLM Wrapper)** — ALIGNED. No GLM headers are included outside `src/engine/`. All rendering goes through engine abstractions (`RenderDevice`, `Material`, `VertexBuffer`). The architecture boundary (CONST-001) is preserved. The demo code under `src/cmd/demo/` uses only engine types, not GLM directly.

- [x] **ADR-003 (Render Pipeline Architecture)** — ALIGNED.
  - **Decision 1 (draw methods return void)**: Both `RunCommand` (no draw calls) and `triangle_demo.cpp` (calls `device.draw(...)` which returns `void` per ADR-003) are consistent. No error checking on draw calls — correct per ADR-003's precondition-based contract.
  - **Decision 2 (Platform::poll_events() added)**: Both `RunCommand` and `DemoCommand` (via `triangle_demo.cpp`) use `platform.poll_events()` for the event loop. No SDL3 includes needed. CONST-001 is preserved.
  - **Render loop owned by command code**: `RunCommand` and `triangle_demo.cpp` each own their render loop, consistent with ADR-003's precedent.

- [x] **ADR-004 (Demo System Architecture)** — ALIGNED. Created as part of this work. Fully documents the architecture decisions that SPEC-007 realises:
  - Per-demo files in `src/cmd/demo/` → Implemented: `triangle_demo.h/.cpp`, `demo_helpers.h/.cpp`. ✓
  - Per-demo free functions in `buddd::cmd::demo` namespace → Implemented: `run_triangle_demo()`, `setup_triangle()`. ✓
  - If/else-if dispatch in `DemoCommand::run()` → Implemented: `demo_command.cpp` lines 82–84. ✓
  - Demo helpers co-located with demos → Implemented: `demo_helpers.h/.cpp` moved to `src/cmd/demo/`. ✓
  - CMake glob auto-discovers demo files → Implemented: `demo/*.cpp` in `CMakeLists.txt`. ✓
  - ADR-004's "Preservation of precedent" section correctly references ADR-003 and CONST-001. ✓
  - Backend selection via `BUDDD_HAS_DISPLAY` compile-time define → Implemented in both `demo_command.cpp` and `run_command.cpp`. ✓
  - Early demo name validation before resource creation → Implemented in `demo_command.cpp`. ✓

- [x] **AMEND-2026-001 (SDL3 Test Exception) referenced correctly** — Spec §Permissions and security (lines 467–468) correctly notes the exception "applies only to `tests/*_sdl3*.cpp`, not to `src/cmd/`." Contract §Security impact (line 724) restates this. ✓ Consistent. The code does NOT use this exception — `src/cmd/` has no SDL3 includes.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/architecture/overview.md`** — Updated with SPEC-007/IMPL-007 references (lines 138–139). "Key behaviors" section (lines 98–112) accurately describes the new behavior: `buddd run` opens empty window (1024×768, no triangle), `buddd demo triangle` opens 800×600 window with triangle render, `buddd test` is removed. The architecture boundary description (lines 114–122) is correct and includes the AMEND-2026-001 exception. ✓ Aligned.

- [x] **`docs/wiki/architecture/module-map.md`** — §buddd — CLI executable (lines 99–141) accurately describes the demo system structure: demos directory, per-demo files, DemoCommand dispatch, subcommand behavior with `demo` replacing `test`. The file structure table (lines 111–131) correctly lists `demo_command.h/.cpp`, `triangle_demo.h/.cpp`, `demo_helpers.h/.cpp` under `src/cmd/demo/`. The subcommand behavior table (lines 134–141) correctly states that `buddd test` is removed. References SPEC-007 and IMPL-007 (lines 177–178). ✓ Aligned.

- [x] **`docs/wiki/architecture/data-flow.md`** — CLI data flow diagram (lines 7–24) matches the actual `main.cpp` dispatch exactly, including the `"demo"` command branch. Output table (lines 28–35) matches all exact output strings including `demo <name>` stderr output and unknown command handling. Notes that old `--test`/`--version` flags are removed. References SPEC-007 and IMPL-007 (lines 143–144). ✓ Aligned.

- [x] **`docs/wiki/architecture/dependency-map.md`** — Shows `buddd` → `buddd_engine` (PRIVATE) dependency. Documents architecture boundary (lines 62–70). No CLI-command-specific content needed here. ✓ Aligned.

- [x] **`docs/wiki/engineering/testing.md`** — §CLI integration tests (lines 30–46) accurately lists all 10 CLI tests, including the 4 new ones (demo no name, demo unknownname, `buddd test` is unknown, demo triangle guarded). Correctly notes that the help-text tests now check for `"demo"` not `"test"`. References SPEC-007 and IMPL-007 (lines 108–109). ✓ Aligned.

- [ ] **`docs/wiki/decisions/adr-index.md` is stale** — Lists only ADR-001 and spec references through SPEC-005/IMPL-005. Missing ADR-002 (GLM Wrapper), ADR-003 (Render Pipeline Architecture), and ADR-004 (Demo System Architecture). This is a pre-existing gap not caused by SPEC-007, and remains unresolved since the previous governance review. See Required governance updates.

- [ ] **`docs/wiki/decisions/constitution-index.md` is partially stale** — Lists CONST-001 as "TODO placeholder — Not yet applicable" but the actual CONST-001 file is fully populated with rules and two amendments (AMEND-2026-001 and AMEND-2026-002). This predates SPEC-007 and is not caused by it, but remains unresolved since the previous governance review. See Required governance updates.

- [x] **Wiki does not become law** — All wiki files reference specs and implementation contracts as authoritative. The wiki is consistently at authority order #4 (per AGENTS.md), below constitution, specs/contracts, and ADRs. No wiki document contradicts the constitution, ADRs, or specs.

## Warnings

Non-blocking concerns for awareness:

- **Code review warnings carried forward** — The code review identified three non-blocking concerns:
  1. **AC-008 (early abort) has only manual test coverage** — No automated test injects a window-close event. Consistent with the spec's scope (manual verification accepted).
  2. **Help text test does not assert absence of `"test"`** — The test checks for `"demo"` but doesn't assert `"test"` is absent. A stricter assertion would prevent regression.
  3. **Engine init logging bleeds to stderr for unknown demos** — `DemoCommand` creates the platform/window/device before validating the demo name, so unknown demos briefly open a window and emit engine-level messages to stderr. Correct per the contract but noisy.

- **Wiki ADR index is stale** — `docs/wiki/decisions/adr-index.md` lists only ADR-001 and SPEC-001 through SPEC-005. ADR-002, ADR-003, and ADR-004 (created as part of this work) are missing. This is a pre-existing failure to keep the index up to date, not a SPEC-007-specific issue.

- **Wiki constitution index is stale** — `docs/wiki/decisions/constitution-index.md` says CONST-001 is "TODO placeholder — Not yet applicable" but the actual rule file is fully populated with content and two amendments (AMEND-2026-001, AMEND-2026-002).

- **CONST-003 and CONST-004 are still TODO stubs** — Both documentation policy and security policy rules have no substantive content. While this doesn't violate any rule (there are none to violate), the project's governance framework is incomplete. This does not block SPEC-007/IMPL-007 — the feature's security impact is explicitly documented as "none" (no network, no privileges, no secrets) and the documentation is comprehensive.

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

- **Wiki maintenance: Update `docs/wiki/decisions/adr-index.md`** — This index should list ADR-002, ADR-003, and ADR-004 alongside ADR-001 to reflect the current ADR state. Not blocking for SPEC-007 but needed for overall governance coherence.

- **Wiki maintenance: Update `docs/wiki/decisions/constitution-index.md`** — The status of CONST-001 should be corrected from "TODO placeholder — Not yet applicable" to reflect its actual content with amendments. Not blocking for SPEC-007 but needed for governance accuracy.

- **CONST-003 / CONST-004 completion** (non-blocking, project-level) — The project should fill in the TODO stubs for Documentation Policy and Security Policy at some point. This is not specific to SPEC-007/IMPL-007.

- **No constitution amendments required** — SPEC-007 does not require changes to any constitution rule. CONST-001 is preserved (verified by grep). The existing AMEND-2026-001 is correctly referenced and unaffected. CONST-002 is satisfied by the required tests.

- **No additional ADRs required** — ADR-004 was created as part of this work and documents the demo system architecture decisions. No other architectural decisions in SPEC-007 warrant a new ADR — decisions (extra-args warning order, framebuffer clear via `begin_frame()`, case-sensitivity in usage, `argc - 2`/`argv + 2` dispatch, compile-time backend selection, early demo name validation) were resolved in the spec's open questions and are consistent with existing patterns.

---

## Re-validation addendum (2026-05-30)

This section records the results of a final cross-document governance re-validation performed after all SPEC-007 / IMPL-007 artifacts reached their final accepted states.

### Re-validation scope

Full cross-document scan of:
- `docs/specs/cli-command-evolution/spec.md` (Status: `Accepted`)
- `docs/specs/cli-command-evolution/implementation-contract.md` (Status: `Accepted`)
- `docs/specs/cli-command-evolution/spec-critic.md` (Status: `Accepted`)
- `docs/specs/cli-command-evolution/implementation-contract-critic.md` (Status: `Accepted with warnings`)
- `docs/specs/cli-command-evolution/code-review.md` (Status: `Accepted`)
- `docs/constitution/**` — all rules
- `docs/adr/**` — ADR-001 through ADR-004
- `docs/wiki/**` — all operational wiki pages

### Items verified

- [x] **All previously resolved cross-document coherence items remain resolved** — No regressions detected. All 25 cross-document coherence items marked `[x]` in the original review remain valid. The spec, contract, code, and tests continue to agree on all behaviors.

- [x] **Backend selection via `BUDDD_HAS_DISPLAY`** — Spec (lines 115–116, 168–169, 580–583), contract (lines 207–213, 477–483, 624–626), and code (`demo_command.cpp` IIFE, `run_command.cpp` IIFE, `CMakeLists.txt` define propagation) are consistent. The compile-time `#ifdef BUDDD_HAS_DISPLAY` pattern correctly selects `be::Backend::SDL3` when defined and `be::Backend::Headless` when undefined. Edge cases in both spec (§Edge cases) and contract (§Edge cases) correctly describe headless behavior. ✓

- [x] **Early demo name validation** — Spec (lines 114–115): "Demo name is validated **before** creating platform resources". Contract (lines 237–241): validates before `Platform::create()`. Code (`demo_command.cpp` lines 51–55): validates before resource creation. Tests (`buddd demo unknownname`): verifies error output without display initialization. ✓

- [x] **CONST-001 (Architecture Boundaries)** — RE-VERIFIED. `grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/` returns zero matches. The refined regex (from spec-critic W-07 / contract-critic W-04) correctly avoids false positives from `be::Backend::SDL3` enum values. ✓

- [x] **CONST-002 (Testing Policy)** — RE-VERIFIED. All 10 CLI tests pass. The 4 new tests (demo no name, demo unknownname, `buddd test` unknown, demo triangle complete) cover all unconditionally testable paths. ✓

- [x] **ADR-004 alignment** — The demo system architecture ADR correctly documents: per-demo files, `buddd::cmd::demo` namespace, if/else-if dispatch, co-located helpers, and CMake glob. All decisions are faithfully implemented. ✓

- [x] **Wiki alignment** — `overview.md`, `module-map.md`, `data-flow.md`, `testing.md` all accurately reflect the SPEC-007/IMPL-007 implementation state. The two stale indexes (`adr-index.md`, `constitution-index.md`) remain as pre-existing issues noted in the original review. ✓

- [x] **No new cross-document contradictions** — No contradictions were found between any pair of documents (spec↔contract, contract↔code, spec↔code, spec↔wiki, code↔tests, ADR↔spec, ADR↔code, constitution↔spec/contract/code). All documents tell the same story.

### Verdict

**Status remains: `Accepted`**

No new blocking issues, no new cross-document contradictions, no new constitution violations. The two pre-existing wiki index staleness items (`adr-index.md` and `constitution-index.md`) continue to be non-blocking awareness items. All SPEC-007 / IMPL-007 governance checks are satisfied.

### Change log

| Date | Action | Key findings |
|---|---|---|
| 2026-05-30 (initial) | Governance review | All cross-document coherence issues resolved. Status: `Accepted`. Pre-existing wiki index staleness noted. |
| 2026-05-30 (this) | Re-validation addendum | No new issues. All artifacts in final accepted states. Backend selection and early validation coherence confirmed. Status remains `Accepted`. |
