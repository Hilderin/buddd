# Governance Review — CLI Command System (SPEC-006 / IMPL-006)

## Status

`Accepted`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **SPEC-006 supersedes SPEC-001 AC-005/AC-006** — SPEC-006 §Supersedes (lines 40–45) explicitly documents that `--version` flag behavior is replaced by `buddd version` subcommand, and that no-args behavior now defaults to `run` (interactive window) rather than printing a greeting message. The supersession note was added after spec-critic review (W-1). ✓ Resolved.

- [x] **Command structure consistency** — All four commands (version, test, run, help) are consistently specified in SPEC-006, implemented in IMPL-006, coded in `src/cmd/commands/`, verified in code review, and documented in wiki (`module-map.md` lines 117–124, `overview.md` lines 100–105, `data-flow.md` lines 28–35). ✓ Aligned.

- [x] **Dispatch logic consistency** — SPEC-006 §Command dispatch rules, IMPL-006 §Exact dispatch logic, the actual `src/cmd/main.cpp`, and wiki `data-flow.md` all describe the same if/else-if chain with default-to-run, case-sensitive comparison, and unknown-command fallthrough. ✓ Aligned.

- [x] **CONST-001 boundary enforcement** — SPEC-006 AC-014, IMPL-006 §CONST-001 compliance, code review (lines 89–95), and wiki `overview.md` §Architecture boundary all agree: `grep -rnE '(SDL3|GL/|glad|glm)' src/cmd/` returns zero source-level matches. ✓ Aligned.

- [x] **Output format consistency** — SPEC-006 §User-visible behavior, IMPL-006 §Output format correctness, code review (lines 122–135, CLI behavior verification table), and wiki `data-flow.md` all document and verify identical exact output strings. ✓ Aligned.

- [x] **CMake build integration** — SPEC-006 AC-015, IMPL-006 §Exact file contents (CMakeLists.txt), the actual `src/cmd/CMakeLists.txt`, and code review (line 76) all match: `file(GLOB_RECURSE CONFIGURE_DEPENDS)` covering `*.cpp` from both `src/cmd/` and `src/cmd/commands/`. Root `CMakeLists.txt` (line 9) already has `add_subdirectory(src/cmd)` — no root-level changes needed. ✓ Aligned.

- [ ] **Contract self-contradiction: `tests/` forbidden vs. required to change** — IMPL-006 §Files forbidden to change lists "Any file under `tests/`" as forbidden, but IMPL-006 §Required tests (lines 422–474) explicitly instructs adding test cases to `tests/version_test.cpp`. The implementation correctly followed the Required tests section. This is a contract drafting issue, not an implementation defect, but it is a cross-document contradiction within the contract itself. Code review (warning #3) already identified this. Not blocking because the constitution-required tests were correctly added.

- [ ] **`demo_helpers.h` include style deviates from contract** — IMPL-006 specifies forward declarations of `buddd::engine::Material` and `buddd::engine::VertexBuffer` (only `<memory>` and `<utility>` includes). The actual `demo_helpers.h` uses full `#include "render/material.h"` and `#include "render/vertex_buffer.h"` instead. This widens the include graph: `run_command.cpp` and `test_command.cpp` now transitively include material/vertex_buffer headers, which the contract's include lists for those files explicitly exclude. Minor deviation — code compiles and works correctly. Code review (warning #1) already identified this.

- [ ] **`main.cpp` omits unused `be` alias** — IMPL-006 §Exact dispatch logic includes `namespace be = buddd::engine;` alongside `namespace bc = buddd::cmd;`. The actual `main.cpp` only declares `namespace bc = buddd::cmd;`. Since `main.cpp` references no engine types directly, the omission is harmless. Code review (warning #2) already identified this.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Constitution violations

Checks against `docs/constitution/**`:

- [x] **CONST-001 (Architecture Boundaries)** — SATISFIED. No SDL3, OpenGL, or GLM headers are included from any file under `src/cmd/`. SPEC-006 AC-014, IMPL-006 §CONST-001 compliance, and code review §CONST-001 compliance all confirm zero matches. The `be::Backend::SDL3` enum value used in command `.cpp` files is an engine abstraction, not a header include.

- [x] **CONST-002 (Testing Policy)** — SATISFIED. All testable code added or modified has corresponding tests. The original spec-critic concern about deferred unit tests (W-2) was flagged as a blocking issue by the contract-critic review and resolved by adding 6 `[cli]` test cases to `tests/version_test.cpp`: version output, help output, unknown command, no-args default (guarded by `BUDDD_HAS_DISPLAY`), and extra-argument handling for both version and help. All 96 tests pass (20 assertions across 6 `[cli]` test cases). The display-dependent commands (`TestCommand`, `RunCommand`) cannot be unit-tested without a GPU — this is acknowledged and consistent with the integration-level ACs.

- [x] **CONST-003 (Documentation Policy)** — NOT VIOLATED. This rule is a TODO placeholder with no substantive rules to enforce. The project's documentation (spec, contract, code review, wiki, ADRs) is comprehensive and internally consistent.

- [x] **CONST-004 (Security Policy)** — NOT VIOLATED. This rule is a TODO placeholder with no substantive rules to enforce. The spec's §Permissions and security and the contract's §Security impact document that no elevated privileges, network access, or credentials are required. No new attack surface is introduced.

- [x] **Engineering Principles** — No contradictions found. Explicit contracts are preferred (spec and contract are detailed), changes are scoped (CLI refactor only), existing conventions are followed (snake_case, namespace conventions, include paths), and requirements are testable (all ACs are verifiable).

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-001 (Result/Error Pattern)** — ALIGNED. CLI commands use engine error handling (`be::to_string(error)` for engine error formatting; fatal errors call `std::exit(EXIT_FAILURE)`). The contract references `src/engine/error.h` and `Result<T>` for engine error reporting. The draw-methods-return-void exception documented in ADR-003 is respected (commands do not call `draw()` themselves; `setup_triangle()` is a resource creation helper, not a draw call).

- [x] **ADR-002 (GLM Wrapper)** — ALIGNED. No GLM headers are included outside `src/engine/`. All math access goes through engine abstractions. CONST-001 boundary is preserved.

- [x] **ADR-003 (Render Pipeline Architecture)** — ALIGNED.
  - **Decision 1 (draw methods return void)**: The `setup_triangle()` helper and render loops in `RunCommand`/`TestCommand` use `RenderDevice::draw()` and `draw_indexed()` which return `void` per ADR-003. This is correctly documented in the implementation contract's "Relevant ADRs" section.
  - **Decision 2 (Platform::poll_events())**: `RunCommand` and `TestCommand` use `(*platform)->poll_events()` for the event loop — no SDL3 includes needed. CONST-001 is preserved.
  - The implementation contract (line 46) correctly references ADR-003.

- [x] **No new ADR required** — Decisions made during SPEC-006 development were resolved in the spec's open questions: if/else-if dispatch (Q-02), silent ignore of extra args (Q-01), CMake glob (Q-03), no base class, shared `setup_triangle()` extracted to `demo_helpers`. No new architectural decisions were made that would warrant a new ADR. The contract's §ADR impact section confirms this.

- [x] **AMEND-2026-001 (SDL3 Test Exception) referenced correctly** — SPEC-006 §Permissions and security (line 313) correctly notes that the exception "applies only to `tests/*_sdl3*.cpp`, not to `src/cmd/`." IMPL-006 (line 42) restates this. CONST-001 file includes the amendment. ✓ Consistent.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/architecture/module-map.md`** — Accurately describes the CLI command structure (§buddd — CLI executable): subcommands, file structure, CMake build, and subcommand behavior table (lines 99–133). References SPEC-006 and IMPL-006 correctly. ✓ Aligned.

- [x] **`docs/wiki/architecture/overview.md`** — Lists all four CLI behaviors (lines 100–105). Documents architecture boundary (lines 114–121) including AMEND-2026-001 exception and GLM boundary. References SPEC-006 and IMPL-006. ✓ Aligned.

- [x] **`docs/wiki/architecture/data-flow.md`** — CLI data flow diagram (lines 7–24) matches the actual `main.cpp` dispatch exactly. Output table (lines 28–35) matches all spec exact strings. Notes that old `--test`/`--version` flags are removed. ✓ Aligned.

- [x] **`docs/wiki/architecture/dependency-map.md`** — Shows `buddd` → `buddd_engine` (PRIVATE) dependency. Documents architecture boundary (lines 64–70). No reference to CLI commands needed here — wiki scope is appropriate. ✓ Aligned.

- [x] **Wiki does not become law** — All wiki files reference specs and implementation contracts as authoritative. The wiki is consistently at authority order #4 (per AGENTS.md), below specs and ADRs. No wiki document contradicts the constitution, ADRs, or specs.

## Warnings

Non-blocking concerns for awareness:

- **Contract self-contradiction (`tests/` forbidden vs. required)** — IMPL-006 §Files forbidden to change lists `tests/` as forbidden, but §Required tests requires adding tests to `tests/version_test.cpp`. The implementation correctly resolved by following the Required tests section. Code review (warning #3) flagged this. The contract should be corrected in a future revision to either remove `tests/` from the forbidden list or add a note that the forbidden list defers to the Required tests section.

- **`demo_helpers.h` includes deviate from contract specification** — IMPL-006 specifies forward declarations for `buddd::engine::Material` and `buddd::engine::VertexBuffer`, but the actual `demo_helpers.h` uses full `#include` directives. This widens the transitive include graph for command `.cpp` files. Not a governance violation (CONST-001 is still intact, code compiles and works), but should be corrected in a future implementation pass to match the contract.

- **`main.cpp` omits `namespace be = buddd::engine;`** — The contract's exact listing includes this alias, but the actual implementation omits it because no engine types are referenced in `main.cpp`. Trivial and harmless, but a deviation from the contract's exact specification.

- **CONST-003 and CONST-004 are TODO stubs** — Both documentation policy and security policy rules have no substantive content. While this doesn't violate any rule (there are none to violate), the project's governance framework is incomplete. This does not block SPEC-006/IMPL-006 — the feature's security impact is explicitly documented as "none" (no network, no privileges, no secrets).

- **Spec-critic suggested improvement not fully addressed: extra-argument behavior inconsistency** — The spec defines three policies for extra arguments: silently ignored (`version`, `help`, `run`), warned-about-but-proceeded (`test`), and error (`unknown command`). The spec-critic review (suggestion #1) noted this inconsistency. The spec and contract's approach was accepted as-is after human review. This is a design preference, not a governance issue.

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

- **Contract correction** (non-blocking, future): IMPL-006 should either remove `tests/` from the "Files forbidden to change" table or add a note that the Required tests section takes precedence. This would resolve the self-contradiction identified by the code review.

- **Wiki maintenance** (non-blocking, ongoing): The `docs/wiki/architecture/module-map.md` section on test files (lines 139–148) lists `version_test.cpp` as having only a single `[sanity]` test. It should be updated to reflect the 6 `[cli]` test cases that were added. However, this is a documentation gap rather than a contradiction, and does not block the current review.

- **CONST-003 / CONST-004 completion** (non-blocking, project-level): The project should fill in the TODO stubs for Documentation Policy and Security Policy at some point. This is not specific to SPEC-006/IMPL-006.
