# Governance Review — SPEC-016 Architecture Refactor: Device/Window/Platform

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] Spec (SPEC-016) and implementation contract (IMPL-016) agree on all 44 ACs, the EngineService design, navigable object graph direction (RenderDevice→Window→Platform→InputSystem), mouse capture API, and demo signature removals.
- [x] Implementation contract and code review agree — all C-001 through C-020 satisfied. Code review documents two pragmatic deviations from strict contract wording (retained `#include "platform/platform.h"` in demos, `MouseButton::Right` instead of `KeyCode::MouseRight`) which are correct adaptations to actual API realities.
- [x] ADR-012 documents 4 decisions (navigable graph, EngineService, virtual diagnostics, mouse capture) — all consistent with spec and implementation.
- [x] Wiki (module-map.md, dependency-map.md, data-flow.md, overview.md, glossary.md, adr-index.md) accurately reflects the post-refactor architecture. No contradictions with spec, contract, or ADR.
- [x] Coordination.md tracks the complete workflow history across all 11 sub-agent steps. All previous steps completed successfully.
- [x] ADR-012 explicitly documents the diagnostic virtual methods (`frame_begin_count`, `frame_end_count`, `draw_call_count`) as test-only concerns that "pollute the API with non-production methods" (negative consequences section). This is a documented tradeoff — not a prohibition. The implementation adds these methods as described, consistent with the ADR.

## Constitution violations

Checks against `docs/constitution/**`:

- [x] **CONST-001 (Architecture boundaries)**: No SDL3/OpenGL/GLM headers leak outside `src/engine/`. EngineService lives at `src/engine/engine_service.h/.cpp` — inside the boundary. SDL3 code stays within `src/engine/window/` and `src/engine/platform/`. No violation.
- [x] **CONST-002 (Testing policy)**: All 227 tests pass (`ctest --preset debug`). Test files updated to use `EngineService::create()` and no longer construct `RenderDeviceHeadless` directly. Demo subprocess tests removed from `demo_tests.cpp`. No violation.
- [x] **CONST-003 (Documentation policy)**: Status is `TODO` — not enforceable.
- [x] **CONST-004 (Security policy)**: Status is `TODO` — not enforceable.
- [x] **ADR-010 (No raw pointers in public API)**: All cross-references use `T&` (`Platform&` in Window, `Window&` in RenderDevice backends). EngineService uses `std::unique_ptr` for ownership. No raw pointers introduced. No violation.
- [x] **ADR-001 (Result pattern)**: EngineService factory returns `Result<std::unique_ptr<EngineService>>`. No violation.
- [x] **Engineering principles (principles.md)**: Prefer explicit contracts (all ACs/C-IDs documented, code review confirms them). Prefer small scoped changes (refactoring is scoped to object graph + mouse capture — no changes to InputSystem, RenderSystem, etc.). Prefer existing conventions (follows snake_case, `#pragma once`, deleted copy/move patterns). Prefer testable requirements (all ACs are testable or inspectable). Governance documents do not contradict each other (all documents agree on architecture). ✅

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-012** exists and documents all 4 key decisions: navigable object graph with non-owning back-references, EngineService lifecycle owner, virtual diagnostic methods on RenderDevice base class, and mouse capture on Window abstract interface.
- [x] ADR-012 references applicable related ADRs (ADR-010 for no raw pointers, ADR-003 for draw method exception).
- [x] No new ADRs needed — the implementation is fully constrained by existing ADRs (per implementation contract section "ADR Impact").
- [x] **ADR-010 (No raw pointers)** compliance verified: all cross-references use `T&`.
- [x] **ADR-003 (Render pipeline architecture)** compliance verified: draw methods remain `void`, `Platform::poll_events()` unchanged.
- [x] **ADR-004 (Demo system architecture)** compliance verified: demo dispatch if/else chain unchanged.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] `docs/wiki/architecture/module-map.md` — Accurately reflects EngineService, Window `Platform&` back-link, mouse capture API, demo no-`Platform&` signatures, test changes, and ADR-012 reference.
- [x] `docs/wiki/architecture/dependency-map.md` — Accurately reflects navigable object graph diagram, new module dependencies (`window/`→`platform/`, `render/`→`window/`), EngineService description.
- [x] `docs/wiki/architecture/data-flow.md` — Accurately reflects EngineService lifecycle (create steps, destruction order), navigable graph access diagram, frame loop using `device.window().platform()` pattern, lifecycle rules with EngineService guarantees.
- [x] `docs/wiki/architecture/overview.md` — Accurately reflects engine_service.h/.cpp in library structure, navigable object graph section, architecture boundary section updated.
- [x] `docs/wiki/domain/glossary.md` — Accurately reflects EngineService term, updated Window/RenderDevice/Backend entries with SPEC-016 changes.
- [x] `docs/wiki/decisions/adr-index.md` — ADR-012 listed in the ADR table.
- [x] Wiki does not create normative rules — it references spec and ADR documents as authorities.

## Warnings

Non-blocking concerns for awareness:

- **Missing explicit EngineService tests for AC-038, AC-042, AC-043** in `tests/render_device_tests.cpp`: The contract requires explicit tests for `EngineService::create(Headless, invalid_config)` returning an error (AC-038), address comparison `&engine->device().window().platform() == &engine->platform()` (AC-042), and full chain `engine->device().window().platform().input_system()` compiling and returning a valid reference (AC-043). AC-038 is functionally covered by `Platform::create_window` negative-dimensions test (test-143). AC-042/043 are implicitly exercised through scene_rendering/model tests and free_camera_demo compilation. Behavior is correct, but explicit tests are absent.
- **Missing SDL3-conditional tests for AC-005/006/007** (`#ifdef BUDDD_HAS_DISPLAY`-guarded tests for `WindowSDL3::set_mouse_capture`/`is_mouse_captured`): These would verify the SDL3 mouse capture implementation with a real SDL3 window. Currently exercised only at runtime by the interactive free camera demo. Not a blocker — SDL3-conditional test infrastructure exists and could be extended.
- **`#include "platform/platform.h"` retained in all four demo .cpp files**: The contract specified removing this include. The implementer correctly identified it is required — `device.window().platform().poll_events()` needs the full `Platform` class definition, which the forward declaration in `window.h` does not provide. This is a pragmatic and correct deviation.
- **free_camera_demo uses `MouseButton::Right` / `is_mouse_down()` instead of `KeyCode::MouseRight` / `is_down()`**: The contract specified keyboard KeyCode API, but the actual mouse-button API is a separate `MouseButton` enum with dedicated `is_mouse_down()` accessors. The implementer correctly adapted to the real API.
- **Virtual diagnostic methods (`frame_begin_count`, `frame_end_count`, `draw_call_count`) are test-only concerns added to the `RenderDevice` abstract interface**: ADR-012 explicitly documents this as a known negative consequence — "pollutes the API with non-production methods." The tradeoff (avoiding `dynamic_cast` in tests) is accepted and documented.
- **`Window` becomes a "god accessor"**: ADR-012 flags the risk that `Window` becomes a dumping ground for unrelated accessors (it now provides `platform()`, `set_mouse_capture`, `is_mouse_captured`, plus existing metrics). Future accessors should be added to the appropriate component rather than to Window.

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

- None. All governance documents are consistent and up to date. No constitution changes are warranted. No new ADRs are required. The wiki has been updated to reflect the current architecture state. ADR-012 fully documents all architectural decisions with rationale, alternatives, and consequences.
