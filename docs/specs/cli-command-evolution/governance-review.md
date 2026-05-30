# Governance Review — CLI Command Evolution: Demo System & Empty Run (SPEC-007 / IMPL-007)

## Status

`Accepted`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec-critic B-01 resolved: Window title case** — Spec-critic identified that Story 1 / AC-007 showed the title `"Buddd Engine — Demo: Triangle"` (capital T) while the User-visible behavior rule said case-preserving. The spec was corrected to consistently use lowercase `triangle` throughout (spec lines 111, 127–131). Contract, code, and tests all match: `demo_command.cpp` constructs the title with the case-preserved demo name. ✓ Resolved.

- [x] **Spec-critic B-02 resolved: CONST-002 / tests optional** — The original spec labelled new tests as "optional" contradicting CONST-002. The spec was corrected (lines 496–514) to state these tests are required by CONST-002, and the "optional" label was removed. The contract (§Required tests) lists all required tests with verification criteria. Four new `[cli]` tests were added to `tests/version_test.cpp`. ✓ Resolved.

- [x] **Spec-critic B-03 resolved: Per-demo argv contract** — The original spec had a contradiction between the per-demo function doc comment (claiming `argv[0]` is the demo name) and the dispatch pseudocode (passing full `argc`/`argv`). The spec was corrected: `DemoCommand` now passes `argc - 2, argv + 2` to per-demo functions (spec line 582, contract line 265), so `argv[0]` is the demo name as documented. Code (`demo_command.cpp` line 83) matches. ✓ Resolved.

- [x] **Spec-critic W-01 resolved: Framebuffer clear mechanism** — The spec now documents that `begin_frame()` clears the colour buffer to black as an implementation detail of the OpenGL backend (spec lines 172, 594). Contract (§Required implementation behavior step 10) and code (`run_command.cpp` lines 43–45) follow this: the loop is just `begin_frame()` / `end_frame()` with no draw calls. ✓ Resolved.

- [x] **Spec-critic W-02 resolved: Extra-arguments warning order** — The spec was corrected to place the extra-arguments warning after resource creation (spec §Required implementation behavior, steps 3–4). Contract (§demo_command.cpp lines 252–260) and code (`demo_command.cpp` lines 71–78) both show the warning after resource creation. ✓ Resolved.

- [x] **Spec-critic W-03 resolved: `buddd run extra_arg` edge case** — Added to the edge case table (spec line 436). Contract §Edge cases also lists it. ✓ Resolved.

- [x] **Spec-critic W-04 resolved: `run_command.h` comment updated** — The contract (§Update `run_command.h`, lines 422–440) and the actual header (`src/cmd/commands/run_command.h`) now say "framebuffer clear only (no draw calls)". ✓ Resolved.

- [x] **Spec-critic W-05 resolved: Case-sensitivity note** — Demo usage text now includes `"Demo names are case-sensitive."` (spec lines 141–145). Contract's `k_demo_usage` constant (line 203) and the actual `demo_command.cpp` (lines 21–27) both include this line. ✓ Resolved.

- [x] **Spec-critic W-06 resolved: Title/message case alignment** — All diagnostic messages and the title example consistently use lowercase `triangle` throughout spec, contract, and code. ✓ Resolved.

- [x] **Contract-critic B-01 resolved: Double-dereference bug** — The contract originally used `*platform, *device` (single dereference) for the call to `run_triangle_demo()`. Fixed to `**platform, **device` (contract §demo_command.cpp lines 265–266, now line 265). Actual code at `demo_command.cpp` line 83 uses `**platform, **device`. ✓ Resolved. The root cause (spec's Required implementation behavior section also had the error) was fixed in the spec as well.

- [x] **Contract-critic W-01 resolved: Unnecessary include** — `demo_command.cpp` originally included `demos/demo_helpers.h` unnecessarily. Removed from contract and actual code (`demo_command.cpp` includes only `demos/triangle_demo.h`). ✓ Resolved.

- [x] **Contract-critic W-02 resolved: WindowConfig::title comment** — The misleading comment about `WindowConfig::title` being `std::string_view` was corrected to accurately state it is `std::string` (contract line 276–277, actual `demo_command.cpp` lines 50–51). ✓ Resolved.

- [x] **Spec-to-contract-to-code coherence: File structure** — Spec §File structure (lines 265–292), contract §Files to create/remove/modify (lines 52–111), and actual filesystem all match exactly: `test_command.h/.cpp` removed, `demo_command.h/.cpp` created, `demo_helpers.h/.cpp` moved to `src/cmd/demos/`, `triangle_demo.h/.cpp` created, `main.cpp`, `run_command.h/.cpp`, `help_command.h`, `CMakeLists.txt` modified. ✓ Aligned.

- [x] **Spec-to-contract-to-code coherence: Output strings** — All exact output strings (`k_usage_text`, `k_demo_usage`, unknown command, unknown demo, demo started/complete/abort, run stdout messages) match exactly between spec, contract, actual code, and verified CLI output (code review §Output format correctness). ✓ Aligned.

- [x] **Spec-to-contract-to-code coherence: Dispatch logic** — Spec §Required implementation behavior (main.cpp), contract §Update main.cpp (lines 369–415), actual `main.cpp` (lines 1–41), and wiki `data-flow.md` (lines 7–24) all describe the same dispatch: `"run"`, `"demo"`, `"version"`, `"help"` with `"test"` falling through to the unknown-command handler. ✓ Aligned.

- [x] **Spec-to-contract-to-code coherence: Demo dispatch** — `DemoCommand::run()` parses `argv[2]` as demo name, creates platform/window/device (800×600, title "Buddd Engine — Demo: <name>"), warns if `argc > 3`, dispatches to `run_triangle_demo()` for `"triangle"`, prints error + usage for unknown names. Contracts, code, and spec all match. ✓ Aligned.

- [x] **Spec-to-contract-to-code coherence: RunCommand** — No triangle rendering, no `demo_helpers.h` include, no `setup_triangle()` call, loop is `begin_frame()`/`end_frame()` only. Window 1024×768, title "Buddd Engine". All matched. ✓ Aligned.

- [x] **Spec-to-contract-to-code coherence: CMakeLists.txt** — Glob includes `demos/*.cpp` alongside existing `*.cpp` and `commands/*.cpp`. Build succeeds. ✓ Aligned.

- [x] **Spec-to-contract-to-code coherence: CONST-001 compliance** — `grep -rnE '(SDL3|GL/|glad|glm)' src/cmd/` returns zero source-level matches. The only matches are `be::Backend::SDL3` (engine enum values, not header includes). ✓ Aligned.

- [x] **Test-to-spec coherence** — All 10 CLI tests (6 existing updated + 4 new) map directly to spec ACs: AC-005 (unknown demo), AC-006 (demo no name), AC-007 (demo triangle completes), AC-012 (no-args = run), AC-013 (version), AC-014 (help), AC-015 (unknown command), AC-016 (`buddd test` is unknown), AC-020 (version extra args), AC-021 (help extra args). All tests pass (100/100, including 90 engine tests). ✓ Aligned.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Constitution violations

Checks against `docs/constitution/**`:

- [x] **CONST-001 (Architecture Boundaries)** — SATISFIED. Code review (lines 141–149) and my own verification confirm: `grep -rnE '(SDL3|GL/|glad|glm)' --include='*.h' --include='*.cpp' src/cmd/` returns zero source-level matches. Only `be::Backend::SDL3` enum values found — these are engine abstraction constants, not header includes. No SDL3, OpenGL, or GLM headers are included from any file under `src/cmd/`. The existing exception AMEND-2026-001 (SDL3 test files) is unaffected and does not apply to `src/cmd/`. The architecture boundary is preserved.

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

- [x] **CONST-003 (Documentation Policy)** — NOT VIOLATED. This rule is a TODO placeholder with no substantive rules to enforce. The project's documentation (spec, contract, code review, ADRs, wiki) is comprehensive and internally consistent for SPEC-007/IMPL-007.

- [x] **CONST-004 (Security Policy)** — NOT VIOLATED. This rule is a TODO placeholder with no substantive rules to enforce. The spec's §Permissions and security and the contract's §Security impact document that no elevated privileges, network access, or credentials are required. No new attack surface is introduced.

- [x] **Engineering Principles** — No contradictions found. Explicit contracts are preferred (spec and contract are detailed with exact output strings). Changes are scoped (CLI refactor only; no engine changes). Existing conventions are followed (snake_case, namespaces, include paths, if/else-if dispatch style). Requirements are testable (all ACs and SCs are verifiable). Governance documents do not contradict each other (verified in this review).

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-001 (Result/Error Pattern)** — ALIGNED. Engine APIs that can fail return `Result<T>`. Command code handles errors via `.error()` with `be::to_string()`. Draw methods return `void` per the ADR-003 exception — this is respected (commands do not call `draw()` with error checking; `draw()` precondition-based contract is followed). The `setup_triangle()` helper uses `std::exit(EXIT_FAILURE)` on fatal errors, which is existing behaviour documented in the spec error cases.

- [x] **ADR-002 (GLM Wrapper)** — ALIGNED. No GLM headers are included outside `src/engine/`. All rendering goes through engine abstractions (`RenderDevice`, `Material`, `VertexBuffer`). The architecture boundary (CONST-001) is preserved. The demo code under `src/cmd/demos/` uses only engine types, not GLM directly.

- [x] **ADR-003 (Render Pipeline Architecture)** — ALIGNED.
  - **Decision 1 (draw methods return void)**: Both `RunCommand` (no draw calls) and `triangle_demo.cpp` (calls `device.draw(...)` which returns `void` per ADR-003) are consistent. No error checking on draw calls — correct per ADR-003's precondition-based contract.
  - **Decision 2 (Platform::poll_events() added)**: Both `RunCommand` and `DemoCommand` (via `triangle_demo.cpp`) use `platform.poll_events()` for the event loop. No SDL3 includes needed. CONST-001 is preserved.
  - **Render loop owned by command code**: `RunCommand` and `triangle_demo.cpp` each own their render loop, consistent with ADR-003's precedent.

- [x] **ADR-004 (Demo System Architecture)** — ALIGNED. Created as part of this work. Fully documents the architecture decisions that SPEC-007 realises:
  - Per-demo files in `src/cmd/demos/` → Implemented: `triangle_demo.h/.cpp`, `demo_helpers.h/.cpp`. ✓
  - Per-demo free functions in `buddd::cmd::demo` namespace → Implemented: `run_triangle_demo()`, `setup_triangle()`. ✓
  - If/else-if dispatch in `DemoCommand::run()` → Implemented: `demo_command.cpp` lines 82–84. ✓
  - Demo helpers co-located with demos → Implemented: `demo_helpers.h/.cpp` moved to `src/cmd/demos/`. ✓
  - CMake glob auto-discovers demo files → Implemented: `demos/*.cpp` in `CMakeLists.txt`. ✓
  - ADR-004's "Preservation of precedent" section correctly references ADR-003 and CONST-001. ✓

- [x] **AMEND-2026-001 (SDL3 Test Exception) referenced correctly** — Spec §Permissions and security (lines 467–468) correctly notes the exception "applies only to `tests/*_sdl3*.cpp`, not to `src/cmd/`." Contract §Security impact (line 700) restates this. ✓ Consistent.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/architecture/overview.md`** — Updated with SPEC-007/IMPL-007 references (lines 138–139). "Key behaviors" section (lines 100–112) accurately describes the new behavior: `buddd run` opens empty window (1024×768, no triangle), `buddd demo triangle` opens 800×600 window with triangle render, `buddd test` is removed. The architecture boundary description (lines 114–122) is correct and includes the AMEND-2026-001 exception. ✓ Aligned.

- [x] **`docs/wiki/architecture/module-map.md`** — §buddd — CLI executable (lines 99–141) accurately describes the demo system structure: demos directory, per-demo files, DemoCommand dispatch, subcommand behavior with `demo` replacing `test`. The file structure table (lines 111–131) correctly lists `demo_command.h/.cpp`, `triangle_demo.h/.cpp`, `demo_helpers.h/.cpp` under `src/cmd/demos/`. The subcommand behavior table (lines 134–141) correctly states that `buddd test` is removed. References SPEC-007 and IMPL-007 (lines 177–178). ✓ Aligned.

- [x] **`docs/wiki/architecture/data-flow.md`** — CLI data flow diagram (lines 7–24) matches the actual `main.cpp` dispatch exactly, including the `"demo"` command branch. Output table (lines 28–35) matches all exact output strings including `demo <name>` stderr output and unknown command handling. Notes that old `--test`/`--version` flags are removed. References SPEC-007 and IMPL-007 (lines 143–144). ✓ Aligned.

- [x] **`docs/wiki/architecture/dependency-map.md`** — Shows `buddd` → `buddd_engine` (PRIVATE) dependency. Documents architecture boundary (lines 62–70). No CLI-command-specific content needed here. ✓ Aligned.

- [x] **`docs/wiki/engineering/testing.md`** — §CLI integration tests (lines 30–46) accurately lists all 10 CLI tests, including the 4 new ones (demo no name, demo unknownname, `buddd test` is unknown, demo triangle guarded). Correctly notes that the help-text tests now check for `"demo"` not `"test"`. References SPEC-007 and IMPL-007 (lines 108–109). ✓ Aligned.

- [ ] **`docs/wiki/decisions/adr-index.md` is stale** — Lists only ADR-001 and spec references through SPEC-005/IMPL-005. Missing ADR-002 (GLM Wrapper), ADR-003 (Render Pipeline Architecture), and the newly created ADR-004 (Demo System Architecture). This is a pre-existing gap not caused by SPEC-007, but it creates an inconsistency between the wiki index and the actual ADR directory. See Required governance updates.

- [ ] **`docs/wiki/decisions/constitution-index.md` is partially stale** — Lists CONST-001 as "TODO placeholder — Not yet applicable" but the actual CONST-001 file is fully populated with rules and two amendments (AMEND-2026-001 and AMEND-2026-002). This predates SPEC-007 and is not caused by it, but is a wiki discrepancy. See Required governance updates.

- [x] **Wiki does not become law** — All wiki files reference specs and implementation contracts as authoritative. The wiki is consistently at authority order #4 (per AGENTS.md), below constitution, specs/contracts, and ADRs. No wiki document contradicts the constitution, ADRs, or specs.

## Warnings

Non-blocking concerns for awareness:

- **Code review warnings carried forward** — The code review identified three non-blocking concerns:
  1. **AC-008 (early abort) has only manual test coverage** — No automated test injects a window-close event. Consistent with the spec's scope (manual verification accepted).
  2. **Help text test does not assert absence of `"test"`** — The test checks for `"demo"` but doesn't assert `"test"` is absent. A stricter assertion would prevent regression.
  3. **Engine init logging bleeds to stderr for unknown demos** — `DemoCommand` creates the platform/window/device before validating the demo name, so unknown demos briefly open a window and emit engine-level messages to stderr. Correct per the contract but noisy.

- **Wiki ADR index is stale** — `docs/wiki/decisions/adr-index.md` lists only ADR-001 and SPEC-001 through SPEC-005. ADR-002, ADR-003, and ADR-004 (created as part of this work) are missing. This is a pre-existing failure to keep the index up to date, not a SPEC-007-specific issue.

- **Wiki constitution index is stale** — `docs/wiki/decisions/constitution-index.md` says CONST-001 is "TODO placeholder — Not yet applicable" but the actual rule file is fully populated with content and two amendments.

- **CONST-003 and CONST-004 are still TODO stubs** — Both documentation policy and security policy rules have no substantive content. While this doesn't violate any rule (there are none to violate), the project's governance framework is incomplete. This does not block SPEC-007/IMPL-007 — the feature's security impact is explicitly documented as "none" (no network, no privileges, no secrets) and the documentation is comprehensive.

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

- **Wiki maintenance: Update `docs/wiki/decisions/adr-index.md`** — This index should list ADR-002, ADR-003, and ADR-004 alongside ADR-001 to reflect the current ADR state. Not blocking for SPEC-007 but needed for overall governance coherence.

- **Wiki maintenance: Update `docs/wiki/decisions/constitution-index.md`** — The status of CONST-001 should be corrected from "TODO placeholder — Not yet applicable" to reflect its actual content with amendments. Not blocking for SPEC-007 but needed for governance accuracy.

- **CONST-003 / CONST-004 completion** (non-blocking, project-level) — The project should fill in the TODO stubs for Documentation Policy and Security Policy at some point. This is not specific to SPEC-007/IMPL-007.

- **No constitution amendments required** — SPEC-007 does not require changes to any constitution rule. CONST-001 is preserved (verified by grep). The existing AMEND-2026-001 is correctly referenced and unaffected. CONST-002 is satisfied by the required tests.

- **No additional ADRs required** — ADR-004 was created as part of this work and documents the demo system architecture decisions. No other architectural decisions in SPEC-007 warrant a new ADR — decisions (extra-args warning order, framebuffer clear via `begin_frame()`, case-sensitivity in usage, `argc - 2`/`argv + 2` dispatch) were resolved in the spec's open questions and are consistent with existing patterns.
