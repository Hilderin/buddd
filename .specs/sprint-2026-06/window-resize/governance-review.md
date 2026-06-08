# Governance Review — Window Resize for All Apps

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] Spec AC-006 requires automated SDL event injection test, but E2E verification lists SDL3 as manual. Contract and code faithfully follow the E2E section's manual approach. This is a spec-level inconsistency, already flagged in contract-critic and code-review. Non-blocking.
- [x] Spec Observability section calls for debounced `BUDDD_LOG_INFO` on final resize size. Contract implements `BUDDD_LOG_DEBUG` per event (deferred). Documented as a deviation. Non-blocking.
- [x] Spec's Key Entities implies `WindowSDL3` self-registers in constructor; contract registers in `PlatformSDL3::create_window()` after construction. Documented as a deviation in contract "Notable deviations from spec". Behavior is equivalent.
- [x] Spec log message `"Window created: {}x{}"` changed to `"Window created (resizable): {}x{} (windowID={})"`. Documented as a deviation. Cosmetic improvement.
- [ ] No unresolved cross-document contradictions.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-010 (No raw pointers in public API)**: `Window*` in `window_map_` is a private member of `PlatformSDL3` (internal implementation detail), not public API. Per ADR-010 exceptions, "non-owning observer pointers in strictly private implementation" is permitted. `register_window()`/`unregister_window()` take `Window*` but are declared in a private engine header — acceptable.
- [x] **ADR-019 (Architecture boundaries)**: No SDL3 types leak outside `src/engine/`. `PlatformSDL3` is a private header inside the engine. Public API uses abstract `Window` interface only. Compliant.
- [x] **ADR-003 (Render pipeline)**: `RenderDevice::begin_frame()` and `RenderDevice::size()` unchanged. No modifications to draw methods or render pipeline. Compliant.
- [x] **ADR-014 (CLI App System)**: No changes to `App` base class, `run_app()`, or any `App` subclass. App lifecycle unchanged. Compliant.
- [x] **ADR-026 (ImGui integration)**: `SDL_EVENT_WINDOW_RESIZED` events flow through existing `engine_imgui::on_sdl_event()` path. Window cache is updated before ImGui sees the event. No new ImGui code. Compliant.
- [x] **ADR-012 (Navigable Object Graph)**: `EngineService` ownership chain unchanged. `Window` base class already stores `Platform&`. Compliant.
- [x] **ADR-020 (Custom Logging System)**: All new logging uses `BUDDD_LOG_DEBUG`/`BUDDD_LOG_INFO`/`BUDDD_LOG_WARN` macros. Compliant.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] `docs/wiki/architecture/module-map.md` — Updated with `on_resize()`, `window_map_`, `register_window()`/`unregister_window()`, `SDL_WINDOW_RESIZABLE`, `SDL_SetWindowMinimumSize()`, resize event handling in `poll_events()`. Accurate.
- [x] `docs/wiki/architecture/data-flow.md` — Updated with resize event flow (SDL resize event → windowID map lookup → `Window::on_resize()` → cache update → ImGui reflow). Accurate.
- [x] `docs/wiki/domain/glossary.md` — Updated Window entry with `on_resize()`, SDL3 backend entry with resize flags, window map, and event handling. Accurate.
- [x] `docs/wiki/engineering/testing.md` — Added Window resize tests section with 3 headless test cases. Accurate.

## Warnings

Non-blocking concerns for awareness:

- **AC-006 spec inconsistency**: AC-006 requires an automated SDL event injection test, but E2E section lists SDL3 as manual. Carried forward from contract-critic and code-review. Spec-level issue, not implementation.
- **Deferred final-size INFO logging**: Spec Observability calls for debounced `BUDDD_LOG_INFO` when resize events settle. Current implementation uses `BUDDD_LOG_DEBUG` per event. Adequate for initial implementation; can be added as follow-up.
- **Visual verification not performed**: SDL3 manual verification (Method 2) requires a display and was not performed in this headless environment. Headless integration tests cover the automated verification path (Method 1).
- **Two spec deviations documented**: Log message format and registration responsibility — both documented in contract's "Notable deviations from spec" section. Both are equivalent behavior.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- None required. All wiki files have been updated by the wiki-agent. No ADRs need modification.
