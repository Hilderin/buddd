# Workflow Coordination: remove-buddd-testing

## Orchestrator

**Feature**: `remove-buddd-testing`
**Status**: completed
**Current step**: completed
**Initial instructions**: Remove BUDDD_TESTING compile-time define entirely. Three test-only accessors on AssetManager (`get_dependency_map`, `testing_shader_programs`, `testing_inject_file_event`) become regular public API (last one renamed to `reload(path)`). MemorySink becomes always-compiled. No engine code references BUDDD_TESTING anymore.
**Notes**: 
- Human confirmed: MemorySink can always be compiled ("on sait jamais"), so BUDDD_TESTING can be completely removed.
- Human confirmed: go through full workflow (spec → contract → implement → review).
- Scout report confirms: only 3 code files and 2 CMakeLists.txt files use BUDDD_TESTING in src/engine/ and tests/.
- `reload(path)` returns `void` (consistent with `poll_file_events()`).
- `testing_handle()` on `ShaderProgram` is removed entirely — tests use `handle()` directly instead (they return the same value).
- Wiki and ADR documentation will be updated by wiki-agent and governance-reviewer.

## spec-author

**Status**: completed
**Summary**: Updated spec.md to document the dispatch_file_event() extraction. Added §3.1 documenting the new private method with signature `dispatch_file_event(const std::string& path, FileEventType type)`. Updated the reload() implementation sketch and rationale to show delegation to dispatch_file_event. All existing acceptance criteria remain valid — this is a pure internal refactoring.
**Artifacts**:
- `.specs/sprint-2026-06/remove-buddd-testing/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**: The spec is thorough, well-structured, and satisfies all Definition of Ready criteria. All API changes are precisely described (before/after code blocks), build changes specify exact CMake lines, edge cases and error scenarios are systematically covered. No blocking issues found. Four non-blocking warnings were raised (unstable test-number references in AC-008/AC-009, missing assertions.md doc update, ambiguous wiki target, missing unified file list).
**Artifacts**:
- `.specs/sprint-2026-06/remove-buddd-testing/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- AC-008 and AC-009 reference unstable test case numbers "Test 26/Test 27" instead of specific TEST_CASE names.
- `docs/wiki/domain/assertions.md` line 40 references `BUDDD_TESTING` — consider updating despite the statement remaining factually correct.
- Wiki target `docs/wiki/domain/asset-manager.md (or equivalent)` is ambiguous; the wiki uses `docs/wiki/domain/business-rules.md` for AssetManager rules.
- No single "all files to modify" summary table — changes are distributed across sections, increasing risk of oversight.
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**: Wrote implementation contract covering all 10 files to modify (memory_sink.h, asset_manager.h/.cpp, shader_program.h/.cpp/headless.h, 2 CMakeLists.txt, 2 test files). Each change is specified with exact before/after code blocks. The dispatch logic extraction into `dispatch_file_event()` is precisely defined. Edge cases (11 total) and error cases from the spec are carried forward. Done criteria include grep checks, compile checks, and full test suite pass.
**Artifacts**:
- `.specs/sprint-2026-06/remove-buddd-testing/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
- The spec's test-change table omits `asset_manager_tests.cpp` line 483 (Test 22), which also has a `testing_inject_file_event` call that must be replaced with `reload()`. The implementation contract explicitly includes this change.
- The spec's test-change table describes the `model_asset_tests.cpp` replacement using `FileEvent` struct construction which no longer exists in the new API. The contract specifies removing the `FileEvent` variable entirely and calling `reload()` directly.
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**: The implementation contract is thorough, complete, and precise. All spec-level gaps (line 483 in asset_manager_tests.cpp, model_asset_tests.cpp FileEvent construction) were caught by the contract author. The dispatch logic extraction into `dispatch_file_event()` is correctly specified. No blocking issues found — the contract is ready for implementation.
**Artifacts**:
- `.specs/sprint-2026-06/remove-buddd-testing/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- Section J replacement range (lines 737-740) would leave stale "FileEvent" comments on lines 735-736 after replacement. The implementer should manually clean up or widen the replacement range.
- Section A uses "surrounding blank lines" for line 12 in memory_sink.h, but the actual file has no blank lines adjacent to line 12. The intent is clear; the implementer will produce a clean result regardless.
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-06
**Warnings**:
<none>
**Notes**: Plan approved by human. Go ahead with implementation.

## code-implementer

**Status**: completed
**Summary**: Removed `BUDDD_TESTING` define from build system. Promoted `MemorySink` to always-compiled. Renamed `get_dependency_map()` → `dependency_map()`, `testing_shader_programs()` → `shader_programs()`, `testing_inject_file_event()` → `reload(std::string_view)`. Extracted dispatch logic into private `dispatch_file_event()`. Removed `ShaderProgram::testing_handle()` entirely. Updated all tests to use renamed API. Refactored `poll_file_events()` to use `dispatch_file_event()`. All 419 tests pass. Debug and release builds compile cleanly.
**Artifacts**:
- `src/engine/log/memory_sink.h` — removed `#ifdef` guard
- `src/engine/asset/asset_manager.h` — added promoted public methods, removed old guarded declarations, added private `dispatch_file_event()`
- `src/engine/asset/asset_manager.cpp` — refactored to use `dispatch_file_event()`, removed old guarded section, added new methods
- `src/engine/render/shader_program.h` — removed `testing_handle()` declaration
- `src/engine/render/shader_program.cpp` — removed `testing_handle()` implementation
- `src/engine/render/shader_program_headless.h` — removed `testing_handle()` override
- `src/engine/CMakeLists.txt` — removed `target_compile_definitions(buddd_engine PRIVATE BUDDD_TESTING)`
- `tests/CMakeLists.txt` — removed `target_compile_definitions(buddd_tests PRIVATE BUDDD_TESTING)`
- `tests/asset_manager_tests.cpp` — replaced all old API calls with new names
- `tests/model_asset_tests.cpp` — replaced `testing_inject_file_event()` with `reload()`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**: Full re-review confirms the implementation is correct. All 10 allowed files were properly modified (confirmed via git diff). BUDDD_TESTING define removed from both CMakeLists.txt files. All grep checks pass (zero BUDDD_TESTING, zero testing_handle, zero old API names in tests). Dispatch extraction into dispatch_file_event() is semantically correct. All acceptance criteria (AC-001 through AC-016) satisfied. Full test suite passes 419/419. No files outside the allowed list were modified.
**Artifacts**:
- `.specs/sprint-2026-06/remove-buddd-testing/code-review.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**: Updated three wiki pages to reflect removal of `BUDDD_TESTING`. `MemorySink` is now documented as always-compiled (not test-only). `AssetManager` test-only accessors are now regular public API (already reflected in the code, no wiki changes needed for those since the module map already lists the current API). The `#ifdef BUDDD_TESTING` references in `assertions.md` were removed.
**Artifacts**:
- `docs/wiki/architecture/module-map.md` — 2 edits
- `docs/wiki/domain/logging.md` — 1 edit
- `docs/wiki/domain/assertions.md` — 1 edit
**Changes made**:
- `module-map.md`: Removed `(test-only, #ifdef BUDDD_TESTING)` note from `MemorySink` in the `log/log.h` table entry; replaced `Guarded by #ifdef BUDDD_TESTING` with `Always compiled` in the `log/memory_sink.h` table entry.
- `logging.md`: Changed "Memory sink (test only)" heading to "Memory sink (always compiled)" and replaced "Compiled only in test builds (`#ifdef BUDDD_TESTING`)" with "Always compiled. Accumulates messages ... for unit test assertions and diagnostic use."
- `assertions.md`: Removed reference to `BUDDD_TESTING` from the behaviour matrix description — now reads "no additional CMake flags are involved".
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**: All 4 ADR updates verified correct. Cross-document coherence confirmed (spec, contract, code, tests, wiki, ADRs all aligned). Zero BUDDD_TESTING references remain in source code, tests, or wiki. ADR-020/ADR-021 updated correctly. Human decided ADR-022 is unnecessary. Governance loop complete.
**Artifacts**:
- `.specs/sprint-2026-06/remove-buddd-testing/governance-review.md`
**Questions for human**:
none
**Warnings**:
none
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
