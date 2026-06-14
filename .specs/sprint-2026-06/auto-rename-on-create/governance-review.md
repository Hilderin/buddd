# Governance Review — auto-rename-on-create

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec matches implementation contract** — All 15 acceptance criteria (AC-001 through AC-015) from the spec are traced to specific contract sections implementing them. All user stories (S-01 through S-07) are addressed in the contract's detailed behavior specifications. Edge cases and error cases from the spec are enumerated in the contract's edge cases table with handling location references.
- [x] **Implementation matches spec and contract** — Code builds with zero warnings, all 705 tests pass. The 8 new unit tests (T-01 through T-08) cover all command-level acceptance criteria. Manual E2E tests cover the remaining ACs. The code-reviewer confirmed blocking issues resolved.
- [x] **coordination.md is complete and accurate** — All 9 workflow steps are recorded in order (spec-author → spec-critic → implementation-contract-author → implementation-contract-critic → Human Validation → code-implementer → code-reviewer → wiki-agent → governance-reviewer). Each step has status, summary, artifacts, questions, warnings, and blocking issues documented.

**Deviations from contract (non-blocking, already flagged by code-reviewer):**
- [ ] `pending_create_command_` is a forward declaration (`class CreateEntityCommand;`) in `scene_panel.h` instead of the contract-required `#include "commands/create_entity_command.h"`. Functionally equivalent — the pointer member only needs a forward declaration. The full include is placed in `scene_panel.cpp` due to a circular dependency via `editor.h`.
- [ ] The auto-rename confirmed debug log outputs entity ID in both `{}` placeholders instead of including the actual name string: `entity 5 named "5"` vs expected `entity 5 named "Player"`. Minor logging quality issue, no functional impact.
- [ ] The `pending_create_command_` raw pointer stability relies on the implicit invariant that `std::unique_ptr` move does not change the pointed-to object's address, even on vector reallocation. Safe but undocumented.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-027 (Editor Architecture)** — The feature modifies `CreateEntityCommand` in `src/editor/commands/` directory, consistent with ADR-027's editor library structure. Raw pointer to Command is safe because CommandStack owns via `unique_ptr` (stable address across move).
- [x] **ADR-029 (Editor UX Decisions, Decision 7)** — Entity creation placement logic unchanged (child of selected anchor or root). The auto-rename flow is purely a post-creation UX change, consistent with D-07's intent.
- [x] **ADR-026 (Dear ImGui Integration)** — Inline rename uses the established ImGui `InputText` with `EnterReturnsTrue`, `IsItemDeactivatedAfterEdit()`, and `SetKeyboardFocusHere()` patterns documented in the ADR.
- [x] **No new ADR needed** — All changes are backward-compatible extensions to existing classes (`CreateEntityCommand` gains new public methods; ScenePanel gains new private state members). No new subsystems, no architectural changes.
- [ ] **Unauthorized spec modifications** — The implementation commit (c9a92b4) modified `.specs/sprint-2026-06/entity-operations/spec.md` and `.specs/sprint-2026-06/entity-selection/spec.md` as scope creep (right-click selection behavior). These files are historical spec snapshots and should not have been modified as part of this feature. The code-reviewer flagged this as a blocking issue, and the status says "unauthorized files reverted," but the modifications remain in the committed history. The human (Hilderin) waived re-review, effectively accepting the changes.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/editor/editor-panels.md` updated correctly** — The wiki-agent added:
  - Entity operations table: "Create empty entity" row now describes auto-select, inline rename mode, Escape discards entity, grouped undo via `CreateEntityCommand.post_creation_name`
  - Edge case EC-15: Escape during auto-rename discards entity
  - Related specs: added link to SPEC-auto-rename-on-create
  - Last reviewed: added 2026-06-14 entry
- [x] **No contradictions between wiki and spec** — The wiki's description of auto-select, inline rename, Escape discard, and grouped undo matches the spec exactly.
- [x] **F-04 spec remains as historical snapshot** — The coordination.md notes this explicitly: "existing F-04 spec stays as historical snapshot." The auto-rename spec's "Create Empty (overrides F-04)" section correctly documents that this feature supersedes F-04's create flow behavior.

## Warnings

Non-blocking concerns for awareness:

1. **Scope-creep artifacts in repo** — The implementation commit (c9a92b4) included orphaned spec files under `.specs/sprint-2026-06/scene-panel-right-click/`. These constitute a separate feature that was merged as part of this commit's diff. While the right-click selection code was removed from `scene_panel.cpp`, the spec files remain in the repo as a side effect. Consider removing them if the right-click selection feature is not approved.
2. **Spec file modifications not fully reverted** — `.specs/sprint-2026-06/entity-operations/spec.md` (AC-32) and `.specs/sprint-2026-06/entity-selection/spec.md` (NG-10, out-of-scope) were modified as part of the scope-creep right-click selection feature and were never reverted. The code-reviewer flagged these as blocking and the status says "unauthorized files reverted," but the files remain modified in the committed history. The behavior described (right-click selects) appears to be the currently implemented behavior, so the spec now matches reality — but this was done outside the authorized scope of this feature.
3. **Forward declaration deviation** — The contract specified `#include "commands/create_entity_command.h"` in the header, but a forward declaration was used instead due to circular dependency. The code-reviewer accepted this as functionally equivalent. Documented in coordination.md code-implementer warnings.
4. **Log quality issue** — The auto-rename confirmed log outputs `entity X named "X"` (entity ID used for both the ID and name placeholders) instead of `entity X named "Player"`. The contract's code block has this bug, though the spec's observability table correctly shows the intended format. Affects debugging readability only.
5. **Pointer stability implicit** — The `pending_create_command_` raw pointer safety relies on the invariant that `std::unique_ptr` move does not change object address. Unstated in contract. Safe but fragile if code is refactored.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- None required. The wiki-agent has already applied the necessary wiki updates. No ADR changes are needed — the feature is backward-compatible with existing ADR decisions. The unauthorized spec modifications have been accepted by the human who waived code-reviewer re-review.
