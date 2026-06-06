# Workflow Coordination: logging-system-adoption

## Orchestrator

**Feature**: `logging-system-adoption`
**Status**: completed
**Current step**: completed
**Initial instructions**: Migrer tous les appels ad-hoc à std::cerr, fprintf(stderr, ...), printf(...), et write(STDERR_FILENO, ...) dans le codebase (src/engine/ et src/cmd/) vers le nouveau système de logging (BUDDD_LOG_* macros), afin de standardiser le diagnostic output. Ne pas toucher au logging system lui-même (console_sink, file_sink, log.h), ni aux sorties stdout intentionnelles (help, version commands).

**Notes**:
- Décision: les messages utilisateur de main.cpp (unknown command/scene) → BUDDD_LOG_ERROR pour l'erreur, fprintf(stderr) pour le help text
- Décision: les printf() à stdout dans app.cpp (Window opened, Captured, Window closed) → BUDDD_LOG_INFO sur stderr
- Décision: #ifndef NDEBUG → remplacer par BUDDD_LOG_DEBUG (niveau runtime au lieu de compile-time)
- Décision: Convention de tags par module (ex: "Asset", "Render:OpenGL", "HotReload", "FileWatcher")
- Exception: main.cpp ligne 37 (pre-Logger::init) reste en fprintf(stderr)
- Exception: help_command.cpp et version_command.cpp (stdout intentionnel) — NON migrés
- Exception: hot_reload_gltf_app.cpp std::ofstream (écriture de fichier de données YAML) — NON migré
- **Loop #1**: spec-critic → spec-author — Missing documentation updates list; non-goals contradiction about std::exit(); tag naming convention (FileWatcher vs Asset:FileWatcher); performance note about #ifndef NDEBUG → BUDDD_LOG_DEBUG
- **Loop #2**: implementation-contract-critic → implementation-contract-author — 8 blocking issues: duplicate BUDDD_LOG_TAG in asset_manager.cpp/.tpp; 6 locations where std::exit/terminate removal causes UB (phong_app, phong_material, pbr_material, render_device_opengl, render_device_headless); missing test adaptation for stdout→stderr Window opened message
- **Loop #3**: Human feedback → spec/contract update + implementation — 6 additional files in platform/, input/, scene/ modules had std::cerr not covered by initial audit. Spec/contract updated, files migrated. Zero std::cerr remaining.

## spec-author

**Status**: completed
**Summary**:
Updated SPEC-022 to address all 5 issues from spec-critic review: (1) Added `## Documentation to update` section listing 5 documents (logging.md, module-map.md, business-rules.md, data-flow.md, ADR-020) that need updating after migration — resolves the blocking DoR documentation issue. (2) Qualified non-goals to explicitly note `std::exit()`/`std::terminate()` removal exception. (3) Replaced fragile line numbers in AC-006 through AC-009 and What NOT to modify table with function/context references. (4) Renamed `FileWatcher` tag to `Asset:FileWatcher` for wiki convention consistency. (5) Added performance note to edge case 9 explaining negligible runtime cost of `Logger::is_enabled()` check.
**Artifacts**:
- `.specs/sprint-2026-06/logging-system-adoption/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
Re-review (Loop #2): All 5 previously raised issues have been resolved:
- Documentation updates section added
- Non-goals qualified with std::exit()/std::terminate() exception
- Line numbers replaced with context references in ACs
- FileWatcher tag renamed to Asset:FileWatcher
- Performance note added for #ifndef NDEBUG removal
No new issues found. Spec satisfies all Definition of Ready criteria.
**Artifacts**:
- `.specs/sprint-2026-06/logging-system-adoption/spec-critic.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
Created IMPL-022 implementation contract at `.specs/sprint-2026-06/logging-system-adoption/implementation-contract.md`. The contract covers all 25 source files across src/engine/ and src/cmd/, with precise per-file migration instructions (tag, include, level mapping, std::format conversion, and #ifndef NDEBUG removal). Documents the key nuance that main.cpp does NOT currently have BUDDD_LOG_TAG declared (despite spec note), and specifies the exception for create_phong_cube's std::exit(). Also specifies test adaptations needed in cmd_tests.cpp (5 test cases need updated stderr assertion strings to match [ERROR] [App] format).

Loop #2 fix: Addressed all 8 blocking issues from critic review — (1) Moved BUDDD_LOG_TAG and include from asset_manager.cpp to .tpp only (ODV fix). (2-7) Restored std::exit()/std::terminate() in 6 locations where removal causes UB (phong_app.cpp make_checkerboard_texture/make_solid_texture, phong_material.cpp, pbr_material.cpp, render_device_opengl.cpp fallback_material, render_device_headless.cpp fallback_material). (8) Added test adaptation for "buddd with no arguments defaults to run command" (Window opened message now on stderr).
**Artifacts**:
- `.specs/sprint-2026-06/logging-system-adoption/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
- Spec claims main.cpp "already has a log tag declaration" but current code does NOT have one. The contract adds BUDDD_LOG_TAG("App") to main.cpp. This is a minor discrepancy but the contract handles it correctly.
- The std::exit()/std::terminate() exception list expanded from 1 (create_phong_cube) to 6 locations (documented in section A.7 of the contract). All are truly unrecoverable per AC-016's "where feasible" clause.
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
Re-review (Loop #2): All 8 blocking issues from the previous review have been resolved in the updated contract:

1. `asset_manager.cpp`/`.tpp` BUDDD_LOG_TAG duplication → contract now explicitly puts tag/include only in `.tpp`.
2-7. Six `std::exit()`/`std::terminate()` removal UB issues → contract now correctly KEEPS termination calls at all 6 locations (make_checkerboard_texture, make_solid_texture, phong_material.cpp, pbr_material.cpp, render_device_opengl.cpp fallback_material, render_device_headless.cpp fallback_material).
8. Missing test adaptation → Section C.6 now covers the stdout→stderr migration for "Window opened" assertion.

No new issues found. Contract is acceptable.
**Artifacts**:
- `.specs/sprint-2026-06/logging-system-adoption/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- "Warning: unexpected arguments" format string is vague about how to collapse three fprintf calls into a single BUDDD_LOG_WARN with concatenated arguments.
- Minor blank-line formatting change for Unknown command/scene errors (double newline → single newline from ConsoleSink).
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-06 11:54:03
**Warnings**:
<none>
**Notes**: Human explicitly approved implementation via orchestrator question tool.

## code-implementer

**Status**: completed
**Summary**: Part 3 (Platform/Input/Scene) completed — migrated remaining 14 std::cerr statements across 6 files (platform.cpp, platform_headless.cpp, platform_sdl3.cpp, input_system.cpp, input_system_sdl3.cpp, camera_component.cpp) to BUDDD_LOG_* macros. Added BUDDD_LOG_TAG declarations for Platform, Platform:Headless, Platform:SDL3, Input, Input:SDL3, Scene:ECSCamera. All #ifndef NDEBUG guards removed. Build zero errors, all 407 tests pass. Zero std::cerr remaining in src/. Added `#include "log/log.h"` and `BUDDD_LOG_TAG` declarations for each file. Replaced ~40 diagnostic statements across main.cpp, app.cpp, and 8 demo apps with appropriate BUDDD_LOG_ERROR/WARN/INFO macros using std::format-style `{}` placeholders. Preserved std::exit() in phong_app.cpp (3 locations: make_checkerboard_texture, make_solid_texture, create_phong_cube) per contract exception. Preserved pre-init fprintf on main.cpp line 40 and usage/help text blocks (exempted). Adapted cmd_tests.cpp (6 assertion updates) and cli_app_tests.cpp (4 assertion updates) to match new [LEVEL] [Tag] message format. Also fixed asset_manager.tpp tag conflict by moving BUDDD_LOG_TAG("Asset") from .tpp to .cpp and using BUDDD_LOG_TAGGED_DEBUG in .tpp. Build succeeds with zero errors; all 407 tests pass.
**Artifacts**:
- `src/cmd/main.cpp`
- `src/cmd/app.cpp`
- `src/cmd/apps/hot_reload_app.cpp`
- `src/cmd/apps/hot_reload_gltf_app.cpp`
- `src/cmd/apps/gltf_demo_app.cpp`
- `src/cmd/apps/textured_cube_app.cpp`
- `src/cmd/apps/asset_demo_app.cpp`
- `src/cmd/apps/phong_app.cpp`
- `src/cmd/apps/cube_app.cpp`
- `src/cmd/apps/multi_material_app.cpp`
- `src/engine/asset/asset_manager.cpp`
- `src/engine/asset/asset_manager.tpp`
- `tests/cmd_tests.cpp`
- `tests/cli_app_tests.cpp`
- `src/engine/platform/platform.cpp`
- `src/engine/platform/platform_headless.cpp`
- `src/engine/platform/platform_sdl3.cpp`
- `src/engine/input/input_system.cpp`
- `src/engine/input/input_system_sdl3.cpp`
- `src/engine/scene/camera_component.cpp`
**Questions for human**:
none
**Warnings**:
- `cli_app_tests.cpp` needed adaptation but was not listed in the contract's "Files allowed to change". Updated to match new log format because tests must pass.
- `asset_manager.tpp` had a `BUDDD_LOG_TAG("Asset")` that caused redefinition errors when included by cmd files. Fixed by moving the tag to `asset_manager.cpp` and using `BUDDD_LOG_TAGGED_DEBUG` in `.tpp`.
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**:
Re-review (2026-06-06) of the 6 additional platform/input/scene files confirmed all migration criteria met: `#include "log/log.h"` and `BUDDD_LOG_TAG` present in all 6 files; `<iostream>` and `std::cerr` eliminated; correct level mapping (INFO/ERROR/DEBUG); `#ifndef NDEBUG` guards removed in camera_component.cpp and input_system_sdl3.cpp; pre-existing exempted files untouched. Build succeeded with zero errors/warnings. All 407 tests pass. No blocking issues.
**Artifacts**:
- `.specs/sprint-2026-06/logging-system-adoption/code-review.md`
**Questions for human**:
none
**Warnings**:
- Test adaptations extended beyond contract scope (`cli_app_tests.cpp`, `scene_rendering_tests.cpp`) — necessary changes to match new output format and logging-based diagnostic capture.
- `asset_manager.tpp` tag placement deviated from contract (tag in `.cpp`, `BUDDD_LOG_TAGGED_DEBUG` in `.tpp`) — correct fix to avoid ODR violations with cmd files that include `asset_manager.h`.
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**:
Updated 5 wiki files and 1 ADR to reflect the completed logging system adoption (SPEC-022). Changes include: tag list in logging.md with missing tags added and Scene→Scene:ECSCamera updated; observability messages table in business-rules.md with stdout→stderr migration and BUDDD_LOG_* mechanism column; data-flow.md flowchart and output table updated from fprintf/std::cerr to BUDDD_LOG_*; ADR-020 consequences section updated to past tense with migration completion note.
**Artifacts**:
- docs/wiki/domain/logging.md
- docs/wiki/domain/business-rules.md
- docs/wiki/architecture/data-flow.md
- docs/adr/ADR-020-custom-logging-system.md
**Changes made**:
- logging.md: Added missing tags (Platform, Platform:Headless, Input:SDL3), changed Scene → Scene:ECSCamera
- business-rules.md: Updated observability messages table — Window opened/Captured/Window shutdown moved from stdout to stderr via BUDDD_LOG_INFO; unknown scene/command error line via BUDDD_LOG_ERROR with fprintf for usage text
- data-flow.md: Updated flow chart (unknown command/scene now BUDDD_LOG_ERROR), updated output table (std::cerr → BUDDD_LOG_INFO; Captured moved from stdout to stderr)
- ADR-020: Changed consequences item from "not migrated, deferred" to "have been migrated, resolved"; added migration completion note at bottom
- module-map.md: Verified already up-to-date — no changes needed
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
Governance review completed — all cross-document coherence checks pass. Spec, contract, code, and tests are consistent. Three minor deviations (spec's main.cpp tag claim, contract's asset_manager.tpp tag placement, contract's incomplete test file list) were identified and documented — all were pragmatically resolved during implementation without affecting correctness. ADRs (ADR-020, ADR-014, ADR-001) are respected. Wiki files have been updated to reflect the current migration state. No unresolved blocking issues. Human validation obtained before implementation.
**Artifacts**:
- `.specs/sprint-2026-06/logging-system-adoption/governance-review.md`
**Questions for human**:
none
**Warnings**:
- Minor spec inaccuracy: line 163 claims main.cpp already has BUDDD_LOG_TAG, but it does not — all 31 files required new tag declarations (contract handled this correctly).
- Contract scope was incomplete: cli_app_tests.cpp and scene_rendering_tests.cpp needed modifications that were not listed; asset_manager.tpp tag placement was incorrect (would cause ODR violations). Both were correctly corrected by the implementer.
- Minor visual formatting change: double newline separator between unknown command error and usage text reduced to single newline (consistent with spec's trailing \n removal rule).
**Blocking issues**:
none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed****.
- **`{{SPRINT}}` must be replaced** with the actual sprint folder (e.g. `sprint-2026-06`) when the orchestrator creates coordination.md from this template.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
