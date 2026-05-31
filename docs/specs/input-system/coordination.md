# Workflow Coordination: input-system

## Orchestrator

**Feature**: input-system
**Status**: completed
**Current step**: completed
**Initial instructions**: Implémenter un InputSystem basé sur SDL3 avec un layer d'abstraction — clavier + souris, frame-based state, intégré via Platform::input_system().

**Notes**:
- Human a validé l'architecture proposée :
  - Modèle frame-based (is_down / is_pressed / is_released)
  - Périmètre v1 : Clavier + Souris
  - Intégration : Platform possède InputSystem, y route les événements SDL
- InputSystem suit le pattern existant : interface abstraite + backends SDL3/Headless + factory statique
- KeyCode enum engine-level (pas de SDL scancodes dans l'API publique) — CONST-001
- Double-buffered state avec begin_frame() pour les transitions pressed/released
- Platform.h gagne `virtual auto input_system() -> InputSystem& = 0`
- Décisions Questions for human (2026-05-30) :
  - Q-01: Factory incluse avec Backend param — Oui ✅
  - Q-02: Mouse wheel en float (preciseX/preciseY) — Oui ✅
  - Q-03: Error::Category InputInitFailed ajouté — Oui ✅
  - Q-04: KeyCode inclut LeftSuper/RightSuper — Oui ✅
  - Q-05: begin_frame() avant la boucle SDL_PollEvent — Oui ✅
  - W-05: Étendre AMEND-2026-001 pour SDL_PushEvent dans les tests — Oui ✅
  - B-01: Corriger "10 files" → "8 files" dans le contrat — Oui ✅
  - W-02: Corriger "replacing InitFailed" → "replacing Unsupported" — Oui ✅
  - B-03: Ajouter test automatisé T-18 pour vérifier le mapping — Oui ✅
  - B-02: Supprimer T-17 (unknown scancode test redondant avec T-18) — Oui ✅
  - KeyCode: Utiliser les mêmes valeurs que SDL_Scancode pour les touches supportées. Conversion = static_cast, pas de table de mapping. Simplification majeure — Oui ✅

## spec-author

**Status**: completed
**Summary**:
- Updated KeyCode enum to use SDL_Scancode numeric values (e.g., A=4, B=5, ..., SuperRight=231).
- Removed all references to lookup tables / mapping tables from the spec.
- Updated AC-001 to describe "values matching SDL_Scancode".
- Updated AC-006 to describe static_cast-based conversion instead of mapping table enumeration.
- Updated AC-015 to describe compile-time assertion of value equality instead of "mapping completeness".
- KeyCode enum is now `static_cast`-compatible with SDL_Scancode — no mapping table needed.
- Fixed B-03: A-02 now describes static_cast+bounds-check instead of switch/lookup table.
- Fixed B-04: Observability section "mapping table" reference replaced with "enum entries".
**Artifacts**:
- `docs/specs/input-system/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
Re-review 3 (2026-05-30) — verified two requested fixes from Re-review 2. Both confirmed resolved: (1) B-03: A-02 now describes `static_cast<KeyCode>(scancode)` with bounds check, no "switch/lookup table". (2) B-04: Observability line 496 now says "Add corresponding enum entries", no "mapping table". Verdict: **Accepted** — no new issues. Spec ready for implementation-contract authoring.
**Artifacts**:
- `docs/specs/input-system/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- W-01: Missing include dependency in InputSystem class code example (no `#include "platform/platform.h"` for `Backend` enum)
- W-02: `InputInitFailed` added to `Error::Category` but never used in any error path
- W-03: AC-012/AC-013 verification of `begin_frame()` being called is imprecise (hand-wavy)
- W-05: AC-006–AC-009 use `SDL_PushEvent()` which requires `<SDL3/SDL.h>` in test files — may require expanding AMEND-2026-001 beyond hint-setting, or using an engine-side test helper
- W-06: AC-015 compile-time assertions compare KeyCode to SDL_SCANCODE_* macros — must be placed in `.cpp` or test file (not `key_code.h`) to avoid CONST-001 violation
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
- Updated contract for KeyCode enum now using SDL_Scancode values with static_cast approach instead of mapping table.
- Goal section: "mapping via lookup table" → "converting via static_cast with bounds check" (line 38).
- W-04 resolution: updated to describe static_cast verification, no mapping function.
- Removed entire `sdl_scancode_to_key_code()` mapping function (switch table with ~53 cases) — replaced with inline static_cast+bounds check in on_sdl_event().
- Removed `key_code_to_sdl_scancode()` lookup table from test helpers — replaced with simple static_cast+REQUIRE.
- Removed sync requirement between two tables (only one mapping now, nothing to sync).
- Simplified T-17: "mapping is complete" test → "static_cast round-trip for representative keys".
- Updated edge case "Unknown SDL scancode received" to describe ignored-not-mapped behavior.
- All "mapping table" / "lookup table" references updated to describe static_cast approach.
**Artifacts**:
- `docs/specs/input-system/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
Re-review of B-01 fix: KeyCode enum now correctly uses explicit SDL_Scancode-matching values (A=4 through SuperRight=231). ✓ Static_cast approach works. No new issues. Verdict: **Accept** — ready for human validation.
**Artifacts**:
- docs/specs/input-system/implementation-contract-critic.md
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none
  - The bounds check `scancode < static_cast<SDL_Scancode>(KeyCode::_Count)`, which breaks if `_Count` is not a sentinel greater than all defined SDL scancodes.

  **Impact**: A Code Agent implementing from this contract would produce a broken input system where key events are mapped to wrong array indices, and valid SDL scancodes are rejected by the bounds check.

  **Fix required**: Either (a) add explicit SDL_Scancode-matching initializers to all `KeyCode` enumerators in the contract's code block (matching the spec), or (b) add a clear, testable requirement that enumerator values must match `SDL_Scancode` values and reference the spec for the exact mapping. Option (a) is strongly preferred for precision.

## Human Validation

**Status**: approved
**Approver**: human
**Date**: 2026-05-30
**Time**: (approved during session)
**Notes**: Human approved spec and implementation contract as-is. Proceeding to implementation.

## code-implementer

**Status**: completed
**Summary**:
Implemented the Input System for the Buddd Engine: KeyCode enum (uint8_t, values matching SDL_Scancode), abstract InputSystem interface with frame-based double-buffered state, SDL3 backend (event processing for keyboard, mouse-motion, mouse-button, mouse-wheel via on_sdl_event()), headless backend (all queries return false/0), factory pattern with Result<T>, integrated into Platform abstraction (virtual input_system(), embedded members in SDL3/Headless platforms), updated poll_events() to call begin_frame() and route events, added InputInitFailed to Error::Category. All 17 test cases pass (9 headless always-run + 8 SDL3 conditional).
**Artifacts**:
- Created: `src/engine/input/key_code.h`
- Created: `src/engine/input/input_system.h`
- Created: `src/engine/input/input_system.cpp`
- Created: `src/engine/input/input_system_sdl3.h`
- Created: `src/engine/input/input_system_sdl3.cpp`
- Created: `src/engine/input/input_system_headless.h`
- Created: `src/engine/input/input_system_headless.cpp`
- Created: `tests/input_tests.cpp`
- Modified: `src/engine/error.h` — added InputInitFailed to Category enum and to_string()
- Modified: `src/engine/platform/platform.h` — added forward decl + virtual input_system() = 0
- Modified: `src/engine/platform/platform_sdl3.h` — added InputSystemSDL3 member + override
- Modified: `src/engine/platform/platform_sdl3.cpp` — updated poll_events() with begin_frame() + event routing
- Modified: `src/engine/platform/platform_headless.h` — added InputSystemHeadless member + override
- Modified: `src/engine/platform/platform_headless.cpp` — updated poll_events() with begin_frame()
**Questions for human**:
none
**Warnings**:
- The `tests/CMakeLists.txt` filter regex `sdl3_backend_test\\.cpp$` (missing 's' before '.cpp') does not actually match the file `sdl3_backend_tests.cpp`, but this is benign since that file uses `#ifdef BUDDD_HAS_DISPLAY` guards. Not addressed as it's pre-existing and outside the contract scope.
- Added `friend class PlatformSDL3` and `friend class PlatformHeadless` to the backend classes (not in the original contract's explicit code blocks) because the embedded member pattern requires constructor access. This is architecturally consistent.
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**:
Code review of the Input System implementation completed. All 17 test cases pass (9 headless + 8 SDL3), full suite passes (230 tests, 12359 assertions), no build warnings, no SDL3 types in public headers (CONST-001), forbidden files unchanged. The implementation matches the accepted spec and implementation contract in all substantive respects. No blocking issues found. Two non-blocking warnings noted: (1) T-02 placed inside `#ifdef BUDDD_HAS_DISPLAY` despite contract listing it as always-run, and (2) `Platform::input_system()` lacks `[[nodiscard]]`.
**Artifacts**:
- docs/specs/input-system/code-review.md
**Questions for human**:
none
**Warnings**:
- W-01: T-02 ("Factory creates SDL3 InputSystem") placed inside `#ifdef BUDDD_HAS_DISPLAY` instead of always-run as the contract specifies. This is pragmatic (requires InputSystemSDL3 header with SDL3 types) and works in the primary build config (BUDDD_HAS_DISPLAY=ON).
- W-02: `Platform::input_system()` lacks `[[nodiscard]]` (carried forward from contract-critic W-04). Consistent with the contract's own code block but inconsistent with the convention table that says all non-void queries should be `[[nodiscard]]`.
**Blocking issues**:
none

## adr-agent

**Status**: completed
**Summary**:
Assessed the Input System feature for new ADR requirements. The implementation is a pure extension of three existing ADR patterns: (1) ADR-001 `Result<T>`/`Error` pattern — followed by `InputSystem::create()` factory; (2) ADR-003 `Platform::poll_events()` pattern — extended to call `begin_frame()` and route SDL events to the input system; (3) ADR-007 SDL3 FetchContent dependency — reused without modification. The `KeyCode` enum matching `SDL_Scancode` values is an implementation detail (static_cast conversion), not an architectural decision. The friend declarations on concrete backends (`friend class PlatformSDL3`/`friend class PlatformHeadless`) follow the same dual-construction pattern (factory + embedded member) already established by Platform backends. No new architectural decisions are introduced. Conclusion: **No new ADR needed**.
**Artifacts**:
- none
**Decisions needed**:
none
**Questions for human**:
none
**Warnings**:
- `Platform::input_system()` lacks `[[nodiscard]]` (carried forward from code-review W-02). This is a minor convention inconsistency, not a blocking issue or a new ADR-worthy decision.
**Blocking issues**:
none

## constitution-agent

**Status**: completed
**Summary**:
AMEND-2026-001 expansion applied as approved by human — exception broadened from hint-only to general SDL3 backend testing, file pattern generalised from `tests/*_sdl3*.cpp` to `tests/*.cpp`.
**Artifacts**:
- `docs/constitution/rules/CONST-001-architecture-boundaries.md`
**Changes needed**:
none (changes applied)
**Questions for human**:
none
**Warnings**:
- The existing test file `tests/sdl3_backend_tests.cpp` also benefits from this expansion (no change needed there — it already works, but the constitutional justification becomes more accurate).
- No other constitution rules are affected by this feature. CONST-002 (testing policy), CONST-003 (documentation policy), and CONST-004 (security policy) are all consistent with the Input System implementation.
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**:
Updated the operational wiki to reflect the implemented Input System feature. Added the `input/` submodule to directory layout and engine structure in architecture overview, created input submodule documentation in module-map, expanded architecture boundary/AMEND-2026-001 descriptions to match constitutional expansion, added `InputInitFailed` to Error::Category across all relevant pages, updated Platform lifecycle in data-flow to describe begin_frame()/event routing, added input tests section to testing docs, and added SPEC-013 references throughout.
**Artifacts**:
- `docs/wiki/architecture/overview.md`
- `docs/wiki/architecture/module-map.md`
- `docs/wiki/architecture/data-flow.md`
- `docs/wiki/domain/business-rules.md`
- `docs/wiki/engineering/testing.md`
- `docs/wiki/engineering/troubleshooting.md`
**Changes made**:
- `overview.md`: Added `input/` to directory layout tree and engine library structure with all 7 files; expanded AMEND-2026-001 exception description to match constitution (general SDL3 testing, not just hint-setting); added SPEC-013 reference
- `module-map.md`: Added `InputInitFailed` to Error::Category; updated Platform submodule with InputSystem integration details; added complete Input submodule section (7 files, roles, patterns); added `input_tests.cpp` to test file table; added SPEC-013 reference
- `data-flow.md`: Added `InputInitFailed` to Error::Category list; added InputSystem frame loop description (begin_frame + event routing) to Platform lifecycle section; added SPEC-013 reference
- `business-rules.md`: Added `InputInitFailed` to Error::Category enum; updated architecture boundary description to mention InputSystem abstractions; added SPEC-013 reference
- `testing.md`: Updated AMEND-2026-001 description from "hint-setting only" to "general SDL3 testing"; added complete input tests section (17 test cases: 9 headless + 8 SDL3) with table
- `troubleshooting.md`: Updated AMEND-2026-001 description and file pattern (`tests/*_sdl3*.cpp` → `tests/*.cpp`); added InputInitFailed and input-not-working troubleshooting entries; added SPEC-013 reference
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
Governance validation complete. All cross-document checks pass: spec aligns with contract, contract aligns with code, tests prove acceptance criteria, constitution is not violated (CONST-001 architecture boundary maintained, AMEND-2026-001 expanded with human approval), ADRs are consistent, and wiki reflects current state. No blocking issues found. Four non-blocking warnings carried forward from previous reviews: (1) T-02 placed inside BUDDD_HAS_DISPLAY despite contract listing it as always-run, (2) Platform::input_system() lacks [[nodiscard]], (3) input_system.h lacks explicit <cstdint> include, (4) minor wiki test table numbering drift.
**Artifacts**:
- `docs/specs/input-system/governance-review.md`
**Questions for human**:
none
**Warnings**:
- W-01: T-02 placed inside `#ifdef BUDDD_HAS_DISPLAY` (contract lists as always-run). Pragmatic deviation — test requires InputSystemSDL3 header.
- W-02: `Platform::input_system()` lacks `[[nodiscard]]` — minor convention inconsistency.
- W-03: `input_system.h` lacks explicit `#include <cstdint>` for `uint8_t` — relies on transitive include.
- W-04: Wiki testing.md input test table has minor numbering/description drift from contract's T-IDs.
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
