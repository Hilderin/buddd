# Governance Review — SPEC-029: Editor Scene State + [[nodiscard]] Fixes

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Include placement minor deviation**: The contract's explicit line-level instruction placed `#include "scene/world.h"` after `shortcut_registry.h`, but the implementer placed it before (alphabetically correct: `scene/` < `shortcut_`). This follows the contract's own alphabetical convention and produces correct, clean code. Already flagged as non-blocking by both implementation-contract-critic and code-reviewer. **Non-blocking — no action needed.**

No actual contradictions found. All other cross-document checks pass:

| Pair | Verdict | Notes |
|---|---|---|
| Spec ↔ Contract | ✅ Clean | All 10 ACs from spec are covered by Contract DC-01 to DC-10. All edge cases, non-goals, file changes match. |
| Contract ↔ Code | ✅ Clean | All 10 DCs satisfied (verified by code review). Only deviation: include ordering (alphabetically correct, non-blocking). |
| Code ↔ Tests | ✅ Clean | 4 test cases cover AC-001 through AC-010. All 507 tests pass, zero warnings. |
| Spec ↔ Tests | ✅ Clean | Each AC maps to a specific test case. Test names match spec's user stories. |
| Spec ↔ Wiki | ✅ Clean | 3 wiki files updated with content matching spec exactly. |

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-027 (Editor Architecture)** — Respected. The `world_` member is a direct member variable (no PIMPL, per Decision 4). Namespace `buddd::editor` used. No changes to `App` base class (Decision 2). Architecture boundary (Decision 6) preserved — no SDL3/OpenGL/GLM headers in editor code. The non-default constructor/destructor is an expected evolution of the Editor class (adding World lifecycle), not a violation of ADR-027.
- [x] **ADR-029 (Editor UX Decisions)** — Respected. SPEC-029 establishes the World ownership foundation (Decision 5's prerequisite). Play mode cloning (`World::clone()`) is correctly deferred to a future feature. No UX changes in this phase.
- [x] **ADR-019 (Architecture Boundaries)** — Respected. `world.h` include in `editor.h` is a permitted engine abstraction include. No engine code changes.
- [x] **ADR-011 (Ownership/Nullability/NoDiscard)** — Respected. `world()` declared `[[nodiscard]]`. `std::unique_ptr` ownership pattern used. All 11 existing `[[nodiscard]]` warning sites verified as fixed.
- [x] **ADR-001 (Result/Error Pattern)** — Respected. Constructor throws `std::bad_alloc` on OOM — consistent with existing patterns.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/architecture/module-map.md`** — Updated: `editor.h` row now documents `world_` member and `world()` accessor; `editor.cpp` row describes World creation/destruction and that `shutdown()` does not reset the World. Content matches spec exactly.
- [x] **`docs/wiki/editor/editor-panels.md`** — Updated: "Important conventions" section now includes the Editor World ownership convention (created in constructor, accessible via `editor.world()`, separate from `ctx.world`). Content matches spec.
- [x] **`docs/wiki/editor/scene-management.md`** — Updated: "Editor World" entry added to Domain Concepts table with full lifecycle description (constructor → always valid → destructor). Matching lifecycle note included. Content matches spec.

The wiki is a faithful operational record of the implementation. It does not introduce new requirements or contradict the spec.

## Warnings

Non-blocking concerns for awareness:

- **Include ordering deviation (previously flagged)**: The contract explicitly instructed placement of `#include "scene/world.h"` after `shortcut_registry.h`, but the alphabetical ordering convention (also in the contract) dictates `scene/` < `shortcut_`, so placing it before is correct. The implementer chose correct alphabetical order over the explicit instruction. This is a minor contract imprecision, not an implementation defect. Already flagged by implementation-contract-critic and code-reviewer. No action needed.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- None. No ADR amendments are needed. No wiki corrections are needed. No new ADRs are required. The implementation follows all existing ADRs. The wiki updates are complete and accurate.
