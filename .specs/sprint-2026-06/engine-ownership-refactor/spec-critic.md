# Spec Review — engine-ownership-refactor (Re-review #1 — 2026-06-06)

## Summary

**Verdict: ACCEPTED** — All 5 previous blocking issues are resolved. DoR criteria satisfied. One minor warning remains (see below).

The spec-author addressed all feedback from the initial review:
- Added Documentation Updates section listing 5 affected files (wiki + ADRs)
- Resolved all 5 [NEEDS CLARIFICATION] items with human decisions (recorded in Resolved design decisions §)
- Added error handling for `render_scene()` failure (Error Cases table)
- Fixed AC-013 with exact frame numbers (30/60 for HotReloadApp and HotReloadGltfApp)
- Fixed AC-015 with concrete criteria (no crash, empty World, no-op, no assertions)
- Added migration order (13 apps in recommended sequence)
- Added ADR references (Related Documents section)
- Added default no-op clarifications
- Removed optional trace-logging

## Previous blocking issues (all resolved)

- [x] **Missing documentation update list (DoR — Documentation)**: ✅ RESOLVED — Spec now has a "Documentation Updates" section listing `module-map.md`, `data-flow.md`, `glossary.md`, `ADR-023`, and `ADR-014`. All 5 files exist and are correctly identified.

- [x] **Unresolved [NEEDS CLARIFICATION] items**: ✅ RESOLVED — All 5 open questions have human decisions recorded in the "Resolved design decisions" section:
  - `App::shutdown()` signature: no parameters (confirmed)
  - `run_app()` namespace: stays in `buddd::cmd` (confirmed)
  - HotReloadGltfApp frame counter: uses `ctx.frame` directly from `on_frame_begin(ctx)` (confirmed)
  - `setup()` failure handling: `shutdown()` called systematically; World/RenderSystem valid during shutdown (confirmed)
  - HotReloadGltfApp `reload_model()` AssetManager: stores `AssetManager&` reference member from `ctx.services.assets()` (confirmed — see **new warning** below for C++ mechanics)

- [x] **Missing error handling for `render_system.render_scene()` in `run_app()`**: ✅ RESOLVED — Error Cases table now documents: `render_scene()` logs a warning internally and continues; if it throws unexpectedly, the exception propagates and `run_app()` exits (existing behaviour preserved).

- [x] **AC-013 is ambiguous**: ✅ RESOLVED — Now specifies exact frame numbers:
  - **HotReloadApp**: texture swap at frame 30; capture frame 30 shows pre-swap texture; capture frame 60 shows post-swap texture
  - **HotReloadGltfApp**: model scale change 1.0→2.0 at frame 30; capture frame 30 shows scale 1.0; capture frame 60 shows scale 2.0

- [x] **AC-015 is vaguely stated**: ✅ RESOLVED — Now specifies concrete behaviour: "RunApp completes its full frame loop without crashing; World is empty (no entities); render_scene() on empty World is a no-op (no assertions fire, no crashes); window opens and closes normally via Escape → request_exit()"

## Previous warnings (all addressed)

- [x] **Frame ordering risk** — Now a documented Edge Case (#1) with per-app instructions for the 7 affected apps
- [x] **HotReloadApp frame-30 timing** — Documented as Edge Case #5
- [x] **No migration order** — Now provided in "Recommended migration order" section
- [x] **Per-app split verification** — Covered implicitly by E2E visual capture comparison (AC-012)
- [x] **Trace-logging** — Removed from spec
- [x] **on_frame_begin() default no-op** — Now explicitly stated
- [x] **Early frame-loop exit safety** — Error Cases table confirms breaking before `begin_frame()` is safe
- [x] **No ADR references** — Related Documents section now references ADR-023 and ADR-014

## New warnings from re-review

- **HotReloadGltfApp `AssetManager&` reference member (C++ mechanics)**: The spec states (line 424) "store `AssetManager& asset_manager_` as a reference member, obtained from `ctx.services.assets()` during `setup()`". In C++, reference members MUST be initialized in the constructor's initializer list, not in `setup()` (which is called after construction). Apps are currently constructed via `std::make_unique<bc::app::HotReloadGltfApp>()` — no constructor arguments. The implementation contract should resolve this, e.g.:
  - Use `AssetManager* asset_manager_` (raw non-owning pointer) initialized to `nullptr`, set in `setup()`
  - Or use `std::optional<std::reference_wrapper<AssetManager>>`
  - Or change the app construction pattern to pass the reference through the constructor
  - Or pass `AssetManager&` as a parameter to `reload_model()` from `on_frame_begin(ctx)`

  **Impact**: Minor — the design intent (non-owning shared access to EngineService's AssetManager) is clear and sound. The C++ mechanism just needs adjustment. This does NOT block spec acceptance.

## Blocking issues

None. All 5 previous blocking issues are resolved.

## Required changes from previous review — verification

| # | Requested change | Status | Evidence |
|---|---|---|---|
| 1 | Documentation updates section | ✅ Done | Lines 492–503 list 5 files |
| 2 | Resolve [NEEDS CLARIFICATION] items | ✅ Done | Lines 629–641 record all human decisions |
| 3 | Error handling for render_scene() | ✅ Done | Error Cases table line 598 |
| 4 | AC-013 specific frame numbers | ✅ Done | Line 154 with exact 30/60 frames |
| 5 | AC-015 concrete criteria | ✅ Done | Line 156 with specific behaviour |
| 6 | Frame-loop exit safety note | ✅ Done | Error Cases table line 599 |

## Definition of Ready — full check

| Criterion | Status |
|---|---|
| Scope clearly defined (in/out) | ✅ Goals + Non-goals + Out of scope |
| Dependencies identified | ✅ Modified files tables |
| Edge cases and errors described | ✅ Edge Cases § + Error Cases § |
| Expected behavior unambiguous | ✅ User stories + ACs |
| E2E verification defined | ✅ Visual captures + unit tests |
| ACs specific, measurable, verifiable | ✅ All 15 ACs concrete |
| Success/failure states described | ✅ Success criteria + Error Cases |
| Interface changes documented | ✅ Changes per component |
| Existing docs to update listed | ✅ Documentation Updates section |
| Technical constraints identified | ✅ No new deps/build changes |
| Risks/unknowns surfaced | ✅ Edge Cases + Assumptions |
| Performance/resource implications | ✅ Empty World ~1KB noted |
