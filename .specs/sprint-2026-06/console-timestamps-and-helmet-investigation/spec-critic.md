# Spec Review — Console Timestamps, FreeCameraMovement Refactoring & Helmet Investigation

## Re-review verdict (2026-06-06 — Updatable review)

**Previous 3 issues remain resolved. The Updatable dangling-pointer cleanup issue is now resolved. Re-review passed — all blocking issues are closed.**

The original loop-back successfully addressed:
1. **ADR-020 contradiction** → Added "Documentation impact" section with rationale.
2. **Subjective helmet AC** → Replaced with 5 objective sub-criteria.
3. **Ambiguous EC-005** → Committed to propagate, no catch.
4. **All 4 warnings** from v1 resolved (exit wiring, doc list, source tag, helmet risk note).

However, the new `Updatable` architectural content introduces a **critical dangling pointer risk** that must be resolved before the spec can be accepted (see blocking issues below).

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **ADR-020 contradiction not addressed (Documentation — Interface changes documented / Clarity — Dependencies identified)**.
    **RESOLVED**: Spec now includes a "Documentation impact" section (lines 246–266) that references ADR-020, explains the rationale for overriding ("V1 simplicity" deferral), lists ADR-020 as requiring an update, and provides a full document-update table.

- [x] **Subjective acceptance criterion for helmet geometry (Verification — Acceptance criteria are specific, measurable, and verifiable)**.
    **RESOLVED**: AC-024 replaced with five objective sub-criteria (AC-024a–e): verifiable vertex count (14556), index count (46356), position min/max bounds (±0.001 tolerance), quaternion conversion check, and specific visual artifacts check.

- [x] **Ambiguous exception handling approach in ConsoleSink (Clarity — Expected behavior is unambiguous and testable)**.
    **RESOLVED**: EC-005 now commits to a single unambiguous approach: NOT noexcept, NOT catch, let exceptions propagate naturally.

- [x] **Missing cleanup of `updatables_` when entities/components are destroyed (Clarity — Edge cases / Technical — Risks surfaced / Verification — Success and failure states)**: The spec introduces `std::vector<Updatable*> updatables_` in `World` as a private member storing raw pointers to `Updatable`-derived components. The spec defines auto-registration in `add_component<T>()` via `if constexpr`, but provides **no mechanism for un-registration** when:
  - A component is removed via `remove_component<T>()` (the component is destroyed but the raw pointer remains in `updatables_`)
  - An entity is destroyed via `destroy_entity()` + `flush_destroyed()` (all its components are destroyed but their `Updatable*` entries in `updatables_` are not removed)

  This creates dangling pointers in `updatables_`. The next call to `update_updatables()` would dereference a stale pointer, causing **use-after-free (undefined behaviour)**.

  **RESOLVED**: Added cleanup logic in both `flush_destroyed()` (entity destruction) and `remove_component<T>()` (component removal). Both paths now iterate the destroyed/removed component's `components_` and use `dynamic_cast<Updatable*>` + `std::erase` to remove matching raw pointers from `updatables_` before the component destructor runs. Documented in the `World` changes section, AC-034, and EC-010.

## Warnings

Non-blocking concerns for awareness:

- **ADR creation not mentioned for Updatable architecture (Documentation)** — The spec's "Documentation impact" section lists ADR-020 (timestamp format), wiki pages, and data-flow. It does not mention that the `Updatable` interface is a significant architectural change (new class hierarchy orthogonal to `Component`, new auto-registration in `World`, new render-loop semantics) that likely warrants a new ADR or an amendment to ADR-006 (component dispatch). The workflow includes an `adr-agent` step, so this is not blocking but should be noted.

- **ER-004 and EC-002 redundancy** — Both EC-002 (Edge case) and ER-004 (Error case) describe the exact same scenario: `FreeCameraMovement::update()` called without a `CameraComponent` on the entity. Both specify the same expected behaviour (log warning once, return `true`). Minor duplication — consider consolidating.

- **Missing `world.h` dependency note** — The spec introduces `std::is_base_of_v<Updatable, T>` in `World::add_component<T>()` (defined in `world.h`). This requires `Updatable` to be at least forward-declared in `world.h` (or included). The spec does not mention this include dependency. (Mitigation: `world.h` already includes `<type_traits>` and forward-declares scene types. A forward declaration of `Updatable` at the top of `world.h` would suffice.)

- **`run_app` code snippet scope ambiguity** — The spec's `run_app` code snippet inserts the `update_updatables()` call before `app.render()`, but does not specify whether it goes before or after `app.on_frame_begin()`. In the existing loop, `on_frame_begin()` is called before `render()`. This is an implementation-detail-level concern and can be resolved in the implementation contract, but should be clarified.

## Required changes

All concrete changes requested have been addressed:

1. **[RESOLVED] Dangling-pointer cleanup** — Added cleanup logic in both `flush_destroyed()` and `remove_component<T>()`. Documented in the `World` changes section (spec.md), AC-034, and EC-010.
2. Consider adding a forward-declaration note for `Updatable` in `world.h` (non-blocking, but good practice).
3. If not already planned in the `adr-agent` step, add a note that the `Updatable` architectural change may need an ADR.

## Suggested improvements

Optional ideas (not required):

- Consolidate EC-002 and ER-004 into a single entry.
- Clarify whether `update_updatables()` call goes before or after `on_frame_begin()` in the render loop.
