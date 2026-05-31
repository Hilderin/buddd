# Governance Review — Input System (SPEC-013 / IMPL-013)

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec → Contract**: KeyCode enum values match SDL_Scancode (A=4 through SuperRight=231). Contract's code block (after fix) replicates spec's explicit values. Both use `static_cast` with bounds check, no mapping table.
- [x] **Contract → Code**: All 8 new files + 6 modified files match the contract's required implementation behavior. Code review confirms: all 17 test cases pass, architecture boundary maintained, forbidden files unchanged.
- [x] **Spec → Tests**: All 20 acceptance criteria (AC-001 through AC-020) are covered by the 17 test cases. AC-015 (compile-time assertion of KeyCode/SDL_Scancode value match) verified via T-17 static_cast round-trip test.
- [ ] **Contract test table → Code test placement**: T-02 ("Factory creates SDL3 InputSystem") is listed under "Headless tests (always runnable)" in the contract but is placed inside `#ifdef BUDDD_HAS_DISPLAY` in the actual test file. This is pragmatic (requires `InputSystemSDL3` header which includes `<SDL3/SDL.h>`) and non-blocking — the test runs and passes in the primary build config (`BUDDD_HAS_DISPLAY=ON`). See code-review W-01.
- [x] **Wiki testing.md → Actual tests**: Minor documentation drift in test numbering/descriptions (wiki T-03/T-08/T-09 describe tests that don't exist as named in the actual test file). The wiki is operational documentation (authority #4) and the core test content is accurate. Not blocking.
- [x] **Human validation**: All human decisions recorded in coordination.md `## Orchestrator` notes and `## Human Validation` section.

## Constitution violations

Checks against `docs/constitution/**`:

- [x] **CONST-001 (Architecture Boundaries)**: No SDL3 types in public headers. Verified via `grep -E '(SDL_|SDL3)' src/engine/input/key_code.h src/engine/input/input_system.h` — zero matches. Architecture boundary maintained.
- [x] **AMEND-2026-001 (SDL3 Test File Exception)**: Expanded by constitution-agent, approved by human. Now permits SDL3 API calls (`SDL_SetHint`, `SDL_PushEvent`, etc.) in `tests/*.cpp` under `#ifdef BUDDD_HAS_DISPLAY` for testing SDL3-dependent engine functionality.
- [x] **CONST-002 (Testing Policy)**: 17 test cases, all passing (230 total, 12359 assertions). Code is tested.
- [x] **CONST-003 (Documentation Policy)**: Placeholder (TODO) — not relevant.
- [x] **CONST-004 (Security Policy)**: Placeholder (TODO) — not relevant.
- [x] **No raw pointers in public API** (ADR-010): All public API uses `InputSystem&`, `Result<std::unique_ptr<InputSystem>>`, value types. Verified.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-001 (Result/Error pattern)**: `InputSystem::create()` returns `Result<std::unique_ptr<InputSystem>>`. Query methods return plain values. Pattern followed correctly.
- [x] **ADR-003 (poll_events pattern)**: `poll_events()` extended to call `begin_frame()` and route events. Pattern extended without breaking it.
- [x] **ADR-007 (SDL3 Release build)**: No changes needed — `GLOB_RECURSE` discovers new files automatically.
- [x] **ADR-009 (Test file naming)**: `input_tests.cpp` uses correct plural `_tests.cpp` suffix.
- [x] **ADR-010 (No raw pointers)**: Verified above. No `T*` in public headers.
- [x] **No new ADR needed**: Confirmed by adr-agent. All patterns are extensions of existing decisions.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **overview.md**: Added `input/` directory to directory layout and engine library structure. AMEND-2026-001 description expanded. SPEC-013 reference added.
- [x] **module-map.md**: Added complete Input submodule section (7 files, roles, patterns). Updated Platform submodule with InputSystem integration. Added `InputInitFailed` to Error::Category.
- [x] **data-flow.md**: Added `InputInitFailed` to Error::Category list. Added InputSystem frame loop description (`begin_frame()` + event routing) to Platform lifecycle section. SPEC-013 reference.
- [x] **business-rules.md**: Added `InputInitFailed` to Error::Category enum. Updated architecture boundary to mention InputSystem abstractions. SPEC-013 reference.
- [x] **testing.md**: Updated AMEND-2026-001 description. Added input system tests section (17 tests). Minor test numbering drift from contract (non-blocking — see cross-document coherence section).
- [x] **troubleshooting.md**: Updated AMEND-2026-001 description. Added `InputInitFailed` and input-not-working troubleshooting entries.
- [x] **No wiki content contradicts constitution** — wiki correctly describes AMEND-2026-001 expansion.

## Warnings

Non-blocking concerns for awareness:

- **W-01: T-02 placed inside `#ifdef BUDDD_HAS_DISPLAY` (carried forward from code-review)**. Contract lists T-02 as always-run headless test; code places it inside SDL3 conditional block. Pragmatic deviation — test requires `InputSystemSDL3` header which includes `<SDL3/SDL.h>`. Works correctly with `BUDDD_HAS_DISPLAY=ON`.
- **W-02: `Platform::input_system()` lacks `[[nodiscard]]` (carried forward from code-review/contract-critic)**. Consistent with contract's own code block but inconsistent with contract's conventions table ("All query methods (non-void return) must be marked `[[nodiscard]]`").
- **W-03: `input_system.h` lacks explicit `#include <cstdint>` (carried forward from contract-critic W-05)**. Relies on transitive include from `key_code.h` for `uint8_t` in `MouseButton` — fragile but compiling.
- **W-04: Wiki testing.md input test table has minor numbering/description drift** from the contract's test table (e.g., T-03 description in wiki doesn't match any always-run test in actual code; T-08/T-09 describe InputSystemSDL3-specific behaviors as headless tests). This is operational documentation, not a formal specification, so not blocking.

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

- **[COMPLETED]** AMEND-2026-001 expansion: constitution-agent applied expansion, human approved. No further action needed.
- **[COMPLETED]** Wiki updates: wiki-agent applied changes across 6 pages. No further action needed.
- **[OPTIONAL]** Wiki testing.md: The input test table could be realigned with the contract's T-IDs for consistency if desired, but this is not required for correctness.
