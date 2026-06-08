# Governance Review — Editor Scaffolding

## Summary

**Re-review: All 3 blocking issues resolved.** Cross-document governance validation confirms all previously blocking issues have been fixed:
1. ✅ ADR-026 Decision 2 now carries an amendment note referencing ADR-027.
2. ✅ ADR-027 Decision 4 now shows direct member variables instead of PIMPL.
3. ✅ Wiki dependency-map now correctly shows `buddd_editor ──PUBLIC──► buddd_engine`.

No new blocking issues found. Two non-blocking warnings remain (overview.md key behaviors stale, EditorApp constructor/destructor undocumented in spec).

## Reviews read

| Document | Path |
|---|---|
| Spec | `.specs/sprint-2026-06/editor-scaffolding/spec.md` |
| Implementation contract | `.specs/sprint-2026-06/editor-scaffolding/implementation-contract.md` |
| Spec critic | `.specs/sprint-2026-06/editor-scaffolding/spec-critic.md` |
| Implementation contract critic | `.specs/sprint-2026-06/editor-scaffolding/implementation-contract-critic.md` |
| Code review | `.specs/sprint-2026-06/editor-scaffolding/code-review.md` |
| Coordination | `.specs/sprint-2026-06/editor-scaffolding/coordination.md` |
| ADR-027 | `docs/adr/ADR-027-editor-architecture.md` |
| ADR-026 | `docs/adr/ADR-026-imgui-integration.md` |
| ADR-019 | `docs/adr/ADR-019-architecture-boundaries.md` |
| Wiki overview | `docs/wiki/architecture/overview.md` |
| Wiki module-map | `docs/wiki/architecture/module-map.md` |
| Wiki dependency-map | `docs/wiki/architecture/dependency-map.md` |
| Code | `src/editor/editor.h`, `src/editor/editor.cpp` |
| Code | `src/cmd/apps/editor_app.h`, `src/cmd/apps/editor_app.cpp` |
| Code | `src/cmd/main.cpp` |
| Code | `src/editor/CMakeLists.txt`, `src/cmd/CMakeLists.txt` |
| Tests | `tests/editor_tests.cpp` |

## Cross-document coherence checks

### ADR-026 amendment (spec → ADR)

**Verdict: BLOCKING.** The spec (line 33), implementation contract (line 27), and ADR-027 (line 97-113, Decision 5) all state that "ImGui init failure is fatal in display mode" and that this **amends ADR-026 Decision 2**. However, `docs/adr/ADR-026-imgui-integration.md` line 56 still reads: **"Init failure is non-fatal"** with no mention of any amendment. The ADR-026 file has not been updated.

- ADR-027 Decision 5 (lines 97-113) explicitly states "This decision amends ADR-026, Decision 2 ('Init failure is non-fatal')."
- ADR-026 Decision 2 (lines 56-57) still says: "Init failure is non-fatal: If `engine_imgui::init()` returns an error, a warning is logged and the engine continues without ImGui."
- The coordination.md adr-agent section flagged this as a warning ("ADR-026 should be updated"), but it was not resolved.

**Action**: Amend ADR-026 to add a note that Decision 2 is amended by ADR-027 (or update the text directly), reflecting that init failure is now fatal in display mode.

### ADR-027 PIMPL vs direct members

**Verdict: BLOCKING.** ADR-027 Decision 4 (lines 77-96) documents the Editor class with the PIMPL pattern (`struct EditorImpl; std::unique_ptr<EditorImpl> impl_;`). However, the spec (lines 88-91), implementation contract (line 86), and actual code (`src/editor/editor.h` lines 29-32) all use **direct member variables** (`bool initialized_`, `EngineService* engine_`, `Window* window_`).

The code-implementer changed from PIMPL to direct members during implementation, but ADR-027 was never amended to reflect this. ADR-027 is now out of sync with the implementation.

**Action**: Update ADR-027 Decision 4 to reflect direct member variables, or document the decision to use PIMPL as superseded and record the new approach in an ADR amendment.

### Wiki dependency-map link type (PUBLIC vs PRIVATE)

**Verdict: BLOCKING.** The wiki `dependency-map.md` (line 14, line 20, line 88) states that `buddd_editor` links `buddd_engine` as **PRIVATE**. However:
- `src/editor/CMakeLists.txt` (line 5-8): `target_link_libraries(buddd_editor PUBLIC buddd_engine)`
- The spec (line 145): `target_link_libraries(buddd_editor PUBLIC buddd_engine)`
- The implementation contract (line 130-138): same, PUBLIC.

The spec, contract, and actual code all agree on PUBLIC. The wiki is wrong.

**Action**: Update `dependency-map.md` diagram, table, and key constraints to say PUBLIC instead of PRIVATE.

### Overview.md key behaviors (3 commands vs 4)

**Verdict: Warning (non-blocking).** `docs/wiki/architecture/overview.md` line 178 says `"prints usage information listing three commands (run, version, help)"` — missing `edit`. However, `docs/wiki/architecture/module-map.md` line 339 correctly says `"prints usage information listing four commands (run, edit, version, help)"`. The overview.md was not fully updated.

### EditorApp constructor/destructor

**Verdict: Warning (non-blocking).** `src/cmd/apps/editor_app.h` declares `EditorApp()` and `~EditorApp() override` which are not shown in the spec (spec lines 104-128). These are practically required for `std::unique_ptr<Editor>` with a forward-declared Editor (the destructor must see the complete type). The code-review flagged this. Consider updating the spec for accuracy.

## ADR-019 compliance (architecture boundary)

**Verdict: PASS.** Verified by grep:
- `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/` — **zero matches** ✓
- `grep -rnE '#include.*(SDL3|GL/|glm/)' src/cmd/apps/editor_app.*` — **zero matches** ✓

## Implementation consistency (code matches spec)

| Check | Status |
|---|---|
| `Editor` in `namespace buddd::editor` | ✅ |
| `setup(EngineContext const&) -> Result<void>` | ✅ |
| `draw_ui(EngineContext const&) -> void` | ✅ |
| `shutdown() -> void` | ✅ |
| Direct member variables (no PIMPL) | ✅ |
| `EditorApp` extends `buddd::cmd::App` | ✅ |
| `config()` returns `{"Buddd Editor", 1280, 800}` | ✅ |
| `setup()` creates `Editor`, calls `editor_->setup(ctx)` | ✅ |
| `on_render()` calls `editor_->draw_ui(ctx)` | ✅ |
| `shutdown()` calls `editor_->shutdown()` | ✅ |
| `"edit"` dispatch in `main.cpp` | ✅ |
| `src/cmd/CMakeLists.txt` links `buddd_editor` | ✅ |
| `src/engine/render/render_device.cpp` propagates ImGui error | ✅ |
| `CMakeLists.txt` for `buddd_editor` is STATIC, PUBLIC link to engine | ✅ |
| Headless test in `tests/editor_tests.cpp` | ✅ |

## Decision log coverage

All decisions D-01 through D-12 from coordination.md are reflected in the spec, implementation contract, ADR-027, code, and/or wiki. No orphaned decisions found.

## Blocking issues

- [x] **ADR-026 not amended**: ADR-026 Decision 2 still states "Init failure is non-fatal" (line 56). ADR-027 Decision 5 claims to amend this, but the amendment was never applied to the ADR-026 file. ADR-026 must be updated to reflect that Decision 2 is amended by ADR-027, or the text must be changed to "fatal in display mode" with a note about the amendment.
- [x] **ADR-027 PIMPL/documentation mismatch**: ADR-027 Decision 4 documents the Editor class with the PIMPL pattern (`EditorImpl`, `std::unique_ptr<EditorImpl>`), but the implementation uses direct member variables (`bool initialized_`, `EngineService*`, `Window*`). ADR-027 must be updated to match the actual implementation.
- [x] **Wiki dependency-map link type wrong**: `dependency-map.md` shows `buddd_editor ──PRIVATE──► buddd_engine` and the table/key constraints say PRIVATE, but the actual CMakeLists.txt uses PUBLIC (matching spec and contract).

## Warnings

- **Overview.md key behaviors stale**: `docs/wiki/architecture/overview.md` line 178 still says "three commands (run, version, help)" — should be "four commands (run, edit, version, help)" to reflect the new `buddd edit` command (module-map.md already has the correct text).
- **EditorApp constructor/destructor undocumented in spec**: `editor_app.h` declares `EditorApp()` and `~EditorApp() override` which are required for `unique_ptr<Editor>` with forward-declared type but are not shown in the spec. Consider updating the spec for accuracy.

## Verdict

**Accepted** — All 3 previously blocking issues have been resolved:
1. ✅ ADR-026 Decision 2 amended with reference to ADR-027.
2. ✅ ADR-027 Decision 4 corrected to direct member variables (no PIMPL).
3. ✅ Wiki dependency-map corrected to PUBLIC.

No new blocking issues. Two non-blocking warnings remain (overview.md key behaviors, EditorApp constructor/destructor).
