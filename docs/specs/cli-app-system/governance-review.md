# Governance Review — CLI App System

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec (SPEC-008) vs Implementation Contract (IMPL-008)**: Fully consistent. Both define the same `App` interface (`AppConfig`, `CaptureSpec`, `RunningArgs`, `App` base class with `setup()`/`render()`/`shutdown()`), the same 7 `App` subclasses, the same unified CLI, the same `RenderSystem::render_scene()` extraction, and the same file create/modify/delete list.
- [x] **Spec vs Spec-Critic**: All 5 blocking issues resolved. SC-004 removed (infeasible pixel comparison), error message inconsistency fixed, `render_scene()` clarified as member function, driver quirk silence accepted per human decision, extra-arg warning format added.
- [x] **Implementation Contract vs Impl-Contract-Critic**: All 6 blocking issues resolved. Capture save timing fixed, temporary `RunApp{}` lvalue reference fixed, extra-args warning clarified, capture failure exit code added, `argc == 0` defensive case added, driver quirk logic (`effective_frame`) verified correct.
- [x] **Implementation vs Code Review**: 1 blocking issue (0-based frame number in abort message) — **FIXED**: `app.cpp` line 100 now uses `frame + 1`. Confirmed by source inspection.
- [x] **ADR-014 vs Spec/Contract**: ADR-014 correctly documents the decision to replace ADR-004's per-demo pattern with the `App` lifecycle, references SPEC-008 and IMPL-008, and records which parts of ADR-004 are superseded. No contradictions.
- [x] **Wiki vs Current State**: All 7 wiki files updated by wiki-agent reflect the current architecture (App lifecycle, unified `run` command, no demo/capture commands, `app_config.h`/`app.cpp`, etc.). No contradictions found.
- [x] **Spec numbering collision (SPEC-008)**: Pre-existing project issue — two specs share the number `008` (`cli-app-system` and `scene-graph`). Not introduced by this workflow. Wiki `module-map.md` correctly references both in its Reference section. Not a blocking issue but noted for future governance.

## Constitution violations

Checks against `docs/constitution/**`:

- [x] **CONST-001 (Architecture Boundaries)**: **No violation**. Verified by `grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/` — zero matches. All new `App` subclasses and `run_app()` code access platform/graphics exclusively through engine abstractions (`RenderDevice`, `Platform`, `Window`). The constitution-agent confirmed compliance. The `is_running()` accessor and App lifecycle do not introduce any new dependency risks.
- [x] **CONST-002 (Testing Policy)**: **Satisfied**. `parse_running_args()` has dedicated tests (AP-01–AP-10 via CLI integration), `RenderSystem::render_scene()` has test RS-01, and CLI integration tests (CT-01–CT-06) cover all error paths and command dispatch. 7 App subclasses are not unit-tested in isolation but are exercised via subprocess-based CLI integration tests. This is a pre-existing acknowledged gap from the spec-critic review — the CONST-002 requirement is met via indirect integration testing.
- [x] **CONST-003 (Documentation Policy)**: Status is `TODO` in the constitution file. No violation can be evaluated against an empty rule.
- [x] **CONST-004 (Security Policy)**: Status is `TODO` in the constitution file. No violation can be evaluated against an empty rule.
- [x] **Engineering Principles**: Prefer explicit contracts (satisfied), prefer small scoped changes (satisfied — refactoring is well-scoped), prefer existing conventions (App pattern follows existing C++ class conventions while improving upon the per-demo free-function pattern), prefer testable requirements (satisfied — non-testable items are acknowledged as manual visual inspection), governance documents must not contradict each other (satisfied — all documents are coherent).

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-014 (`docs/adr/014-cli-app-system.md`)**: Exists and is `Accepted`. Documents the decision to centralise the render loop, adopt the `App` lifecycle, unify the CLI, and remove `demo`/`capture` commands. Lists alternatives considered (free-function callback, template-based, keep three commands, keep per-demo loops, runtime registry, sleep_for limiting). Documents consequences (positive and negative) and which parts of ADR-004 are superseded. Fully consistent with the spec and implementation contract.
- [x] **ADR-004 (`docs/adr/004-demo-system-architecture.md`)**: Remains `Accepted` but is partially superseded by ADR-014. The wiki and ADR-014 correctly document which parts are superseded (per-demo files, per-demo free functions, if/else-if dispatch in DemoCommand, each-demo-owns-render-loop) and which are retained (demo_helpers co-location, CMake glob pattern).
- [x] **No new ADR required**: ADR-014 was created as part of this workflow. All design decisions were resolved during the spec phase and recorded.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/architecture/module-map.md`**: Correctly documents the App lifecycle pattern, `app.h`/`app.cpp`/`app_config.h`/`app_config.cpp` files, App subclasses table, updated CMake glob, demo helpers, and subcommand behavior (no demo/capture). References ADR-014.
- [x] **`docs/wiki/architecture/data-flow.md`**: Correctly shows CLI dispatch without DemoCommand/CaptureCommand, unified `run` command flow, scene dispatch sub-diagram, and updated output table. References ADR-014 and SPEC-008/IMPL-008.
- [x] **`docs/wiki/architecture/overview.md`**: Key behaviors updated to use `buddd run <scene>` syntax. `demo`/`capture`/`test` commands correctly listed as removed.
- [x] **`docs/wiki/engineering/testing.md`**: CLI integration tests table updated to match new CLI (no demo/capture tests, new run scene tests with `--frame` and `--capture`).
- [x] **`docs/wiki/decisions/adr-index.md`**: ADR-014 entry added with correct title and status notes.
- [x] **`docs/wiki/domain/glossary.md`**: `EngineService` description updated — "Used by tests and `run_app()`" instead of "Used by tests and `demo_command.cpp`".
- [x] **`docs/wiki/architecture/dependency-map.md`**: Updated reference from `demo_command.cpp` to `run_app()` in the `EngineService` section.

## Warnings

Non-blocking concerns for awareness:

- **`is_running()` public accessor not in spec**: The implementation adds `[[nodiscard]] auto is_running() const noexcept -> bool` as a public accessor for `running_` (app.h line 42). The spec only shows `running_` as `protected`. This is necessary because `run_app()` is a free function (not a member) and cannot access `protected` members. The addition is well-designed and does not change the external contract, but the spec does not document it. Consider updating the spec's `App` interface listing to include `is_running()`.
- **5 ACs rely on manual visual inspection**: AC-013, AC-021, AC-022, AC-023, AC-024 require manual visual verification (rendering correctness). These are acknowledged in the spec and cannot be automated at this stage. Deferred to human acceptance testing.
- **Frame numbering dualism persists**: `App::render()` receives 0-based frame index, but CLI flags (`--capture`, observability messages) use 1-based numbering. The conversion is handled in `run_app()` (`spec.frame == frame + 1`). This is a known persistent off-by-one risk documented in the spec, ADR-014, spec-critic, and impl-critic.
- **`demo_helpers` `std::exit()` bypasses `App::shutdown()`**: `setup_triangle()` and `setup_cube()` call `std::exit(EXIT_FAILURE)` on fatal errors, which bypasses `App::shutdown()`. This is a pre-existing limitation inherited from old code, documented as a non-goal. Not introduced by this workflow.
- **SPEC-008 numbering collision**: Two specs share the number `008` (`docs/specs/cli-app-system/spec.md` and `docs/specs/scene-graph/spec.md`). This is a pre-existing project issue not introduced by this workflow. Consider renumbering to avoid confusion in future work.
- **No direct unit tests for `parse_running_args()` start parameter**: Tests AP-01 through AP-10 exercise parsing logic but may not cover non-default `start` indices. The code-implementer noted tests are exercised indirectly via subprocess CLI tests. Risk is low but worth noting.
- **App subclass rendering correctness via integration tests only**: The 7 App subclasses' individual rendering behavior is tested only through subprocess CLI invocations, not isolated unit tests. CONST-002 is satisfied (tests exist and pass), but there is a gap in granularity.

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

- **None required.** ADR-014 has been created and is consistent. The wiki has been updated by the wiki-agent. No constitution changes are needed. No ADR changes are needed. The implementation is complete and consistent with all governance documents.

## Blocking issues

<ul>
<li>none</li>
</ul>
