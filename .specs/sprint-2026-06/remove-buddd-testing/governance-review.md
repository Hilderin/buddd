# Governance Review — Remove BUDDD_TESTING

## Cross-document coherence

Cross-referencing spec, implementation-contract, code-review, wiki, and ADRs reveals no contradictions or gaps. All documents agree on:

- Removal of `BUDDD_TESTING` define from both CMakeLists.txt files.
- Promotion of `MemorySink` to always-compiled (no `#ifdef` guard).
- Three API renames: `get_dependency_map()` → `dependency_map()`, `testing_shader_programs()` → `shader_programs()`, `testing_inject_file_event()` → `reload(std::string_view)`.
- Extraction of duplicated dispatch logic into private `dispatch_file_event()`.
- Removal of `ShaderProgram::testing_handle()` entirely — tests use `handle()` directly.
- Only the 10 allowed files were modified (confirmed by `git diff --name-only` plus 3 wiki files by wiki-agent).

- [x] Spec intent matches human approval (Hilderin, 2026-06-06).
- [x] Implementation contract accurately reflects spec.
- [x] Code (10 files) matches implementation contract.
- [x] Tests (419/419 passing) prove acceptance criteria.
- [x] Wiki files updated by wiki-agent reflect current state.
- [x] Historical `.specs/` files from previous sprints were NOT modified (confirmed by `git diff HEAD -- .specs/` — zero output).

## ADR alignment

- [x] ADR-020: Line 56 updated — `MemorySink` description changed to "always compiled". ✓
- [x] ADR-021: Lines 56, 161–163, 218 updated — stale `BUDDD_TESTING` references replaced. Line 56 now reads "No test-specific define controls assertion behaviour". Lines 161–163 now frame `BUDDD_TESTING` as historical. Line 218 now reads "MUST NOT use any test-specific define". ✓
- [x] New ADR (ADR-022): Human decided it is unnecessary — no new ADR required. The decision is sufficiently documented by the spec, implementation contract, and updated ADR-020/ADR-021. ✓

## Wiki alignment

- [x] `docs/wiki/architecture/module-map.md` — Line 52 correctly reads "Always compiled" instead of "Guarded by `#ifdef BUDDD_TESTING`". Line 43 `(test-only, #ifdef BUDDD_TESTING)` removed.
- [x] `docs/wiki/domain/logging.md` — Line 233 heading changed to "Memory sink (always compiled)". Description updated to "Always compiled. Accumulates messages..."
- [x] `docs/wiki/domain/assertions.md` — Line 40 now reads "no additional CMake flags are involved" (reference to `BUDDD_TESTING` removed).
- [x] `docs/wiki/domain/business-rules.md` — No specific `BUDDD_TESTING` references; accurately describes AssetManager at a higher level. No update needed.

## Grep verification

- [x] No `BUDDD_TESTING` in `src/engine/` or `tests/` — zero matches (exit code 1 from grep).
- [x] No `testing_handle` in `src/engine/render/` — zero matches.
- [x] No `testing_handle` in `tests/` — zero matches.
- [x] No old API names (`testing_shader_programs`, `testing_inject_file_event`, `get_dependency_map`) in tests — zero matches.

## Required governance updates

All governance updates have been completed:

- [x] **ADR-020-custom-logging-system.md** line 56: Updated — "always compiled". ✓
- [x] **ADR-021-developer-assertions.md**: Lines 56, 161–163, 218 updated. ✓
- [x] **ADR-022**: Human decided it is unnecessary — not created. ✓

## Warnings

None. The implementation is clean, tests pass, wiki is updated, ADRs are updated, and grep checks pass.

## Blocking issues

None.
