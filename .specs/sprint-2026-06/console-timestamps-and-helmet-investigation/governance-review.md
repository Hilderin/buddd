# Governance Review — Console Timestamps, FreeCameraMovement Refactoring & Helmet Investigation

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Code-review blocking issue: Wiki module-map.md had wrong Updatable signature** — The code-review flagged that `module-map.md` line 122 documented `update(const InputSystem&, Window&, float dt) -> bool` instead of the actual implementation `update(const EngineContext& ctx) -> void`. **RESOLVED by wiki-agent**: module-map.md line 123 now correctly reads `virtual update(const EngineContext& ctx) -> void`. See coordination.md wiki-agent section.

- [ ] **Spec/Contract vs Implementation: Updatable interface signature changed** — The spec (AC-004, AC-006) and contract (§11) define `Updatable::update(const InputSystem& input, Window& window, float dt) -> bool` with short-circuit on `false` return. The implementation changes this to `Updatable::update(const EngineContext& ctx) -> void` with no short-circuit (exit via `ctx.request_exit()`). This is a documented architectural deviation — ADR-023 provides the rationale (alternative #4 in ADR-023 discusses the bool-return rejection) and the authority order (ADR > Spec) means the ADR overrides the spec. The spec and contract are now stale historical artifacts but this is acceptable per workflow design.

- [ ] **Contract outdated relative to implementation** — The implementation-contract.md specifies the old `Updatable` interface, `App::setup(RenderDevice&)`, and manual Platform/Window/RenderDevice creation in `run_app()`. The actual implementation uses EngineContext, `App::setup(EngineService&)`, and `EngineService::create()`. The contract was written to match the spec, but the implementation deviated (with ADR-023 approval). The contract is now a stale snapshot.

- [ ] **No short-circuit in `update_updatables()` per spec AC-006** — The spec requires `update_updatables()` to short-circuit on `false` return. ADR-023 explicitly mandates NO short-circuit (all updatables run every frame; exit is checked after iteration). This is the most significant behavioral deviation from the spec, but it is intentional and documented with rationale in ADR-023 §4 (Alternative #4).

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-020 updated** — ConsoleSink format changed from `[LEVEL] [Tag] message` to `[HH:MM:SS.fff] [LEVEL] [Tag] message`. ADR-020 §5 now reflects the new format. Status remains "Accepted" — the update is a minor amendment. ✅

- [x] **ADR-023 created** — New ADR documents the `Updatable` system architecture, `EngineContext` struct, World auto-registration/cleanup, `App::setup(EngineService&)` broader signature, and `run_app` auto-dispatch. Includes alternatives considered and consequences. Covers key deviations from spec (void return, no short-circuit, EngineContext parameter). ✅

- [ ] **ADR-023 compliance check** — All compliance items in ADR-023 §Compliance are satisfied by the implementation:
  - `updatable.h` declares pure abstract `Updatable` with `virtual auto update(const EngineContext& ctx) -> void = 0` ✅
  - `engine_context.h` defines `EngineContext` with `EngineService&`, `Window&`, `float delta_time`, `request_exit()`, `is_exit_requested()` ✅
  - `world.h` auto-registers `Updatable` subclasses in `add_component<T>()` via `if constexpr (std::is_base_of_v<Updatable, T>)` ✅
  - Cleanup in both `flush_destroyed()` and `remove_component<T>()` ✅
  - `World::update_updatables(const EngineContext&)` iterates all, NO short-circuit ✅
  - `App::setup()` accepts `EngineService&` ✅
  - `run_app()` creates `EngineContext` each frame, dispatches before `app.render()` ✅

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/domain/logging.md`** — Console format `[HH:MM:SS.fff]` correct. File sink format ISO 8601 correct. `GltfHelmet` source tag present. ✅

- [x] **`docs/wiki/architecture/module-map.md`** — `engine_context.h` documented in EngineService table. `updatable.h` with correct signature `update(const EngineContext& ctx) -> void`. `free_camera_movement.h/.cpp` present. `gltf_helmet_app` documented in apps table. `app.cpp` lifecycle description includes Updatable auto-dispatch. ✅

- [x] **`docs/wiki/domain/business-rules.md`** — Console format `[HH:MM:SS.fff]` at line 71. App lifecycle (step 6e) includes `World::update_updatables(EngineContext{...})`. Available scenes table includes `gltf-helmet`. ✅

- [x] **`docs/wiki/architecture/overview.md`** — Updated with `engine_context.h` and `updatable.h`/`free_camera_movement.h` per wiki-agent. ✅

- [x] **`docs/wiki/architecture/data-flow.md`** — Updated with Updatable auto-dispatch step per wiki-agent. ✅

- [x] **`docs/wiki/decisions/adr-index.md`** — All missing ADRs (011, 013, 015, 017–021, 023) added per wiki-agent. ✅

## Warnings

Non-blocking concerns for awareness:

- **Spec vs ADR-023: `Updatable::update()` signature mismatch** — The spec (AC-004) says `update(const InputSystem&, Window&, float dt) -> bool`. ADR-023 says `update(const EngineContext&) -> void`. ADR > Spec per authority order, so this is acceptable, but future readers should consult ADR-023 for the authoritative interface contract rather than the spec.

- **Contract is now a stale snapshot** — The implementation-contract.md was written for the spec's interface but the implementation changed to EngineContext. The contract should not be used as an implementation reference for the Updatable system — ADR-023 is the authoritative document.

- **All 12+ apps had `setup()` signature changed** — The cascading change from `App::setup(RenderDevice&)` to `App::setup(EngineService&)` affected every App subclass, not just the three listed in the spec scope. This was a necessary consequence of the base class change and was correctly executed.

- **`run_app()` restructured** — The contract specified manual Platform/Window/RenderDevice creation in `run_app()`. The implementation uses `EngineService::create()`. This aligns with the wiki's documented pattern and ADR-012 but was not in the contract.

- **Minor behavioral difference: exit frame renders one extra scene** — In the original apps, pressing ESC skipped `render_scene()` on the exit frame. The refactored apps (using Updatable auto-dispatch) call `render_scene()` before the exit check takes effect. This is the consequence of the no-short-circuit design documented in ADR-023 §4.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- None required. ADR-020 updated, ADR-023 created, all wiki files updated by wiki-agent. The governance review confirms all documentation is coherent with the current state of the implementation.
