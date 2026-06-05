# Governance Review — Free Camera Interactive Demo

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Wiki data-flow.md contradicts spec/implementation on Escape exit message**: `docs/wiki/architecture/data-flow.md` line 32 previously stated that free-camera "exit[s] with `"Demo aborted by user"` on Escape." However, SPEC-015 §Output specifies that Escape-pressed exit prints `"Demo complete: free-camera (interactive)\n"` to `std::cerr`, and only window-close (`poll_events() == false`) prints `"Demo aborted by user\n"`. The implementation (confirmed by code review DC-020) follows the spec, not the wiki. **RESOLVED**: wiki-agent corrected data-flow.md — line 32 now correctly documents Escape → "Demo complete: free-camera (interactive)" and window close → "Demo aborted by user".

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`. No new items for this re-verification cycle.

## Constitution violations

Checks against `docs/constitution/**`:

- [ ] **CONST-001 (Architecture Boundaries)**: No violation. The demo header forward-declares only `Platform` and `RenderDevice`. Grep verification confirms zero SDL3/OpenGL/GLM includes in the demo files. The `Platform::delta_time()` addition follows the existing abstraction pattern (same approach as `poll_events()` per ADR-003).
- [ ] **CONST-002 (Testing Policy)**: No violation. The free-camera demo is interactive (requires a display) and cannot run in the headless test suite — the constitution-agent and impl-contract-critic both verified this. The `Platform::delta_time()` addition is a trivial getter exercised via existing SDL3 backend tests.
- [ ] **CONST-003 (Documentation Policy)**: No violation. Rule is TODO — no actionable requirements yet.
- [ ] **CONST-004 (Security Policy)**: No violation. Rule is TODO — no actionable requirements yet. Feature has no elevated privileges, network access, or filesystem I/O.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-001 (Result<T> / Error Pattern)**: No new `Result<T>` APIs introduced. `delta_time()` returns `float` (non-fallible getter) — follows ADR-001's exception for functions that cannot logically fail.
- [x] **ADR-003 (Render Pipeline Architecture)**: Adding `Platform::delta_time()` follows the same pattern established by ADR-003 (adding abstract methods to Platform to keep demo code platform-independent). Consistent.
- [x] **ADR-004 (Demo System Architecture)**: Follows the mandated pattern: `.h`/`.cpp` pair in `src/cmd/demo/`, single free function in `buddd::cmd::demo` namespace, `else if` dispatch in `demo_command.cpp`.
- [x] **ADR-005 (std::optional<T&> for Component Lookup)**: Camera obtained via `get_component<CameraComponent>()->camera()` and stored as `auto&` (reference, not copy). Consistent.
- [x] **ADR-010 (No Raw Pointers in Public API)**: Public API uses `Platform&` and `RenderDevice&` — references, not raw pointers. Consistent.
- [x] **ADR-011 (Ownership/Nullability/Lifetime)**: ADR file is empty (0 lines) — no actionable requirements. No violations.

No new ADRs are required. The `Platform::delta_time()` addition follows the precedent of ADR-003 (adding virtual methods to Platform), and the demo follows ADR-004 exactly. The adr-agent correctly determined no new ADR is needed.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **module-map.md**: Accurately documents `free_camera_demo.h/.cpp`, `Platform::delta_time()` virtual method, SDL3/headless backend delta_time implementations, and subcommand listing. All accurate.
- [x] **data-flow.md**: Previously contained an error — line 32 stated Escape exits with `"Demo aborted by user"`, but spec and implementation specify Escape prints `"Demo complete: free-camera (interactive)"`. **RESOLVED**: wiki-agent corrected the line — now correctly documents both Escape (complete) and window close (aborted) behavior.
- [x] **overview.md**: Accurately describes the free-camera demo behavior. No incorrect output messages mentioned.

## Warnings

Non-blocking concerns for awareness:

- **data-flow.md Escape message error is blocking** (listed under Cross-document coherence) — not a warning.
- **ADR-011 is empty**: The file `docs/adr/011-owner-ship-nullability-lifetime-nodiscard.md` contains zero content. This does not affect this feature but the project may want to populate or remove it.
- **Extra include in code vs contract**: `free_camera_demo.cpp` includes `"input/input_system.h"` which was not listed in the implementation contract's required includes. The code review flagged this as correct and necessary — the contract's include list was slightly incomplete. Minor non-blocking gap.
- **AC-007 verification method tension**: Spec says "Unit test" for pitch clamp verification; implementation contract says code review is sufficient (since clamp is in application demo code). The divergence was reviewed and accepted in the contract-critic cycle. Not blocking.
- **SC-004 line count relaxation**: Spec says ≤120 lines; contract relaxed to ≤150 per spec-critic. Actual implementation is 118 lines, satisfying both. No practical impact.

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

- [x] **data-flow.md line 32**: Change the escape behavior description from:
  `"and exit with \"Demo aborted by user\" on Escape"`
  to:
  `"and exit with \"Demo complete: free-camera (interactive)\" on Escape (or \"Demo aborted by user\" on window close)"`
  to match SPEC-015 §Output and the verified implementation.
  **RESOLVED**: wiki-agent applied this exact correction.
