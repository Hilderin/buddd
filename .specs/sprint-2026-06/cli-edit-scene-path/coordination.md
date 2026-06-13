# Workflow Coordination: cli-edit-scene-path

## Orchestrator

**Feature**: `cli-edit-scene-path`
**Status**: completed
**Current step**: completed
**Initial instructions**: Permettre de démarrer `buddd edit [scenepath]` pour ouvrir une scène en mode édition directement depuis le CLI. Si un chemin de fichier YAML est fourni, l'éditeur doit charger la scène au démarrage. Si le fichier n'existe pas, erreur et exit code 1.
**Notes**:
- Sprint: 2026-06
- SPEC_DIR: `.specs/sprint-2026-06/cli-edit-scene-path/`
- Décisions prises avec l'humain :
  - `buddd edit scene.yaml` → ouvre l'éditeur avec la scène chargée
  - `buddd edit` → éditeur vide inchangé
  - Fichier inexistant → erreur + exit code 1
  - YAML auto-detection par extension `.yaml`/`.yml` (comme `run`)
  - Flags `--frame`/`--capture` conservés
  - Chemin relatif/absolu passé tel quel
  - Tests d'intégration à écrire (existait, inexistant, corrompu)
- Réponses aux questions du spec-critic (2026-06-13) :
  - Répertoire en argument → `is_regular_file()` check, erreur + exit 1 (pas d'ouverture éditeur)
  - Chaîne vide → traitée comme "pas d'argument", éditeur vide
  - Arg vs flag (`somearg` vs `--capture`) → vérifier argv[2][0] == '-'. Si flag → continue parse_running_args, sinon → exit 1

## spec-author

**Status**: completed
**Summary**:
Resolved 3 spec-critic blocking issues via targeted edits:
1. **Directory path handling**: Changed from `std::filesystem::exists()` to `is_regular_file()` for pre-opening validation. Directories now consistently produce error + exit 1 (no window). Updated user-visible behaviour, edge-case table, error cases, permissions, and assumptions.
2. **Empty string dispatch**: `buddd edit ''` unambiguously treated as "no argument" — editor opens empty. Updated edge-case entry and added explicit check in dispatch logic.
3. **Dispatch logic for positional args**: Added new "Dispatch logic for `edit` subcommand" section with 4-step flow: (1) no arg/empty → empty editor, (2) YAML extension → `is_regular_file()` check, (3) starts with `-` → flags only, (4) otherwise → unknown arg error + exit 1. Updated AC-005 to cover both `somearg` (exit 1) and `--frame 2` (exit 0). Also added "Existing documentation that must be updated" section (data-flow.md, module-map.md, business-rules.md) and added window title verification to AC-002.
**Artifacts**:
- `.specs/sprint-2026-06/cli-edit-scene-path/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
Re-review: PASSED. All 3 previous blocking issues resolved by spec-author. The spec now satisfies all Definition of Ready criteria. Directory path handling uses `is_regular_file()` consistently. Empty string treated as "no argument." 4-step dispatch logic added. Wiki documentation pages listed. AC-002 enhanced with window title verification. No new issues found.
**Artifacts**:
- `.specs/sprint-2026-06/cli-edit-scene-path/spec-critic.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
Fixed 1 blocking issue and 3 warnings identified by the implementation-contract-critic:
- Added `#include <optional>` to `editor_app.h` (blocking issue) — added to Files allowed to change, Required behavior section, and Done criteria.
- Fixed `is_yaml_path` → `is_yaml_file` lambda naming to match spec convention (warning W1).
- Fixed `to_string()` → `be::to_string()` namespace prefix in code snippet (warning W2).
- Consolidated T1/T4 test redundancy by making T4 use `--capture` instead of `--frame`.
- Clarified T7/T8 exit code expectations for both `BUDDD_HAS_DISPLAY=ON` and `=OFF` builds.
**Artifacts**:
- `.specs/sprint-2026-06/cli-edit-scene-path/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
Re-review: PASSED. The sole previous blocking issue (missing `#include <optional>`) is resolved — the contract now explicitly specifies the include in the Files allowed to change, Required implementation behavior, and Done criteria sections. All four previous warnings (T1/T4 redundancy, `be::to_string` namespace prefix, T7/T8 exit code ambiguity, `is_yaml_path` naming) are also fixed. No new issues found. Contract is accepted.
**Artifacts**:
- `.specs/sprint-2026-06/cli-edit-scene-path/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-13 10:21
**Notes**: Human approved implementation. Plan accepted as-is.

## code-implementer

**Status**: completed
**Summary**:
Extended `buddd edit` to accept an optional scene YAML file path. When a `.yaml`/`.yml` path is provided and the file exists, the editor opens with that scene loaded during `setup()`. When no path is provided, the editor opens with an empty untitled scene (existing behaviour unchanged). Non-existent or non-regular files produce an error on stderr and exit code 1 without opening the editor. Flags `--frame` and `--capture` continue to work after the scene path. Added 8 integration tests covering all dispatch scenarios.
**Artifacts**:
- `src/cmd/apps/editor_app.h` — Added `#include <optional>`, parameterised constructor with `std::optional<std::string>`, added `scene_path_` member
- `src/cmd/apps/editor_app.cpp` — Changed constructor to store scene path; added `open_scene()` call in `setup()` after successful `editor_->setup(ctx)`
- `src/cmd/main.cpp` — Replaced edit dispatch with 4-step logic (no arg/YAML/flag/unknown), uses `is_regular_file()` for validation, `flags_start=3` when scene path present
- `src/cmd/commands/help_command.h` — Updated edit line to `"Open the editor (optionally with a scene file)"`
- `tests/cmd/cli_app_tests.cpp` — Added 8 test cases: basic edit, nonexistent file, unknown arg, --capture flag, .yaml/.yml extension-only, valid YAML path, case-insensitive .YML
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**:
Review: PASSED. The implementation fully satisfies the spec (SPEC-035) and implementation contract (IMPL-035). All acceptance criteria covered, all 582 tests pass with zero regressions, build produces zero warnings. Architecture boundaries respected (no SDL3/OpenGL/GLM in src/cmd/). All 8 forbidden file categories remain unchanged. Minor non-blocking suggestions noted for test robustness, but nothing blocking acceptance.
**Artifacts**:
- `.specs/sprint-2026-06/cli-edit-scene-path/code-review.md`
**Questions for human**:
none
**Warnings**:
- Test T4 does not assert exit code for the `--capture` flag path (only checks absence of error string).
- Test T1 depends on `BUDDD_HAS_DISPLAY=ON` build configuration for its specific stderr assertion.
**Blocking issues**:
none

## adr-agent

**Status**: completed
**Summary**: Not needed — implementation contract explicitly states "This implementation does not warrant a new ADR" (backward-compatible changes within ADR-014 and ADR-027).
**Artifacts**:
- none
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**:
Updated 3 wiki pages to document the optional scene path for `buddd edit`:
- **data-flow.md**: Replaced simple `EditorApp → run_app()` with detailed 4-step edit dispatch diagram; added 3 new rows to CLI output table (`edit <path.yaml>`, `edit <nonexistent>.yaml`, `edit <unknown>`).
- **module-map.md**: Updated EditorApp file description with `scene_path` constructor parameter; expanded Subcommand behavior section for edit scene path; updated CLI integration to mention optional `[<scene>]`.
- **business-rules.md**: Added 3 entries to CLI output behavior table for edit variants; added 2 exit codes (nonexistent scene file, unknown argument edit); added 2 observability message rows for edit errors.
**Artifacts**:
- `docs/wiki/architecture/data-flow.md`
- `docs/wiki/architecture/module-map.md`
- `docs/wiki/domain/business-rules.md`
**Changes made**:
- `data-flow.md`: Expanded edit dispatch from 1-line to 4-step logic diagram; added table rows for edit with YAML path, nonexistent file, unknown arg.
- `module-map.md`: Updated EditorApp docs (optional `scene_path`), Subcommand behavior (edit with YAML path validation), CLI integration (optional scene arg).
- `business-rules.md`: Added edit variants to CLI behavior table, exit codes, and observability messages.
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
PASSED — All cross-document coherence checks pass. Spec, implementation contract, code, and wiki are fully consistent. ADR-014, ADR-027, ADR-019, and ADR-001 are all respected. Architecture boundaries preserved (no SDL3/OpenGL/GLM in src/cmd/). All 5 allowed files modified correctly; all 8 forbidden file categories untouched. Wiki accurately reflects current state. No blocking issues.
**Artifacts**:
- `.specs/sprint-2026-06/cli-edit-scene-path/governance-review.md`
**Questions for human**:
none
**Warnings**:
- Test T1 asserts for editor layout file log which requires `BUDDD_HAS_DISPLAY=ON` (default build config) — acceptable.
- Test T4 does not assert exit code for the `--capture` flag path — minor robustness gap.
- T9 and T10 (capture with scene) are manual/E2E only — acceptable per contract design.
**Blocking issues**:
none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **`{{SPRINT}}` must be replaced** with the actual sprint folder (e.g. `sprint-2026-06`) when the orchestrator creates coordination.md from this template.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
