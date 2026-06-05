# Workflow Coordination: free-camera-demo

## Orchestrator

**Feature**: `free-camera-demo`
**Status**: completed
**Current step**: completed
**Initial instructions**: Add a demo free camera that moves the camera around with WASD, mouse to move camera, and Space/Left Control for up/down.
**Notes**:
- Clarification answers received:
  - Mouse look: always active (no button hold required)
  - Up/down keys: Space = up, Left Control = down
  - Movement speed: fixed (no configurable parameter)
  - Exit: Escape closes the demo window
- This is a demo (application-level code under `src/cmd/demo/`), not an engine feature
- Follows existing demo patterns: new .h/.cpp pair + registration in demo_command.cpp
- Loop 1 (spec-critic → spec-author): Escape key handling — poll_events() does NOT return false on Escape; need explicit input.is_down(KeyCode::Escape) check in the demo loop
- Loop 2 (human-validation → spec-author): Human approved with modifications — add `Platform::delta_time()` engine method, demo uses it instead of manual chrono

## spec-author

**Status**: completed
**Summary**:
Fixed contradiction between A-15 (delta_time guaranteed > 0) and edge case documenting zero delta_time. Removed "returns zero" edge case entirely and updated A-15 to specify the contract: "returns a positive float representing seconds... always > 0 under normal operation."
**Artifacts**:
- `.specs/sprint-2026-05/free-camera-demo/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
Re-review (round 4 — A-15/edge-case fix verification): The contradiction between A-15 (delta_time always > 0) and the edge case (delta_time returns zero) has been fixed and verified. The zero-delta edge case was removed; A-15 now correctly states "under normal operation, always > 0"; the large-delta edge case notes the normal contract. Both are now consistent. No remaining blocking issues across all review rounds.
**Artifacts**:
- `.specs/sprint-2026-05/free-camera-demo/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- Pseudocode: `input` used before assignment in loop (line 131-132 before line 135) — trivially fixable, intent unambiguous.
- AC-007 unit test feasibility: pitch clamp is in demo-level code, not engine test suite. Consider clarifying verification method.
- SC-004 (≤120 lines) may be tight for the added input/timing logic vs existing simpler demos.
- Numerical stability of XZ forward projection at extreme pitch — minor, acceptable for a demo.
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
Updated the implementation contract to incorporate the `Platform::delta_time()` engine change from the updated spec. Added: (1) engine platform files (platform.h, platform_sdl3.h/.cpp, platform_headless.h/.cpp) to Files allowed to change; (2) exact code for `Platform::delta_time()` pure virtual, SDL_GetTicks-based computation in PlatformSDL3, and fixed 1/60s return in PlatformHeadless; (3) updated demo implementation to use `platform.delta_time()` instead of manual `std::chrono::steady_clock`; (4) 5 new Done Criteria (DC-022 through DC-026) for the engine changes; (5) removed the now-contradictory "Frame delta time is zero" edge case per spec update.
**Artifacts**:
- `.specs/sprint-2026-05/free-camera-demo/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
Re-review (Cycle 2 — engine delta_time change verification). Verified the updated contract's Platform::delta_time() changes against actual source files, constitution rules, and ADRs. The SDL_GetTicks-based computation is correct (Uint64 ms → float s), the headless implementation (fixed 1/60f) is appropriate, and the poll_events() reordering (delta before begin_frame) is safe. No blocking issues found. The contract is complete, accurate, and leaves no architectural decisions to the Code Agent.
**Artifacts**:
- `.specs/sprint-2026-05/free-camera-demo/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- Transitive `Model` include via `mesh_renderer.h` → `model.h` (follows existing pattern, safe but implicit)
- Function definition style not explicitly specified (same-pattern qualified name vs namespace block — unambiguous from `cube_scene_demo.cpp`)
- Wiki module-map line numbers approximate (179-187 vs actual 181-186) — handled by wiki-agent
- `Uint64` (SDL3 type) used for local variable in `poll_events()`, `uint64_t` (C++ std type) for member — functionally identical, minor style inconsistency
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-05-31
**Time**: 08:09 EDT
**Warnings**:
<none, or a bullet list of non-blocking concerns>
**Notes**: Human approved with modifications — all modifications incorporated and verified: (1) `Platform::delta_time()` engine change added to spec and contract. (2) Demo uses `platform.delta_time()` instead of manual chrono. (3) SDL3 Escape behavior verified correct. Final approval granted after spec-critic and impl-contract-critic re-reviews passed.

## code-implementer

**Status**: completed
**Summary**:
Implemented the free-camera interactive demo (`buddd demo free-camera`) with WASD + mouse look + Space/Control vertical movement. Added `Platform::delta_time()` virtual method with SDL_GetTicks-based implementation in `PlatformSDL3` and fixed 1/60s return in `PlatformHeadless`. Wired the demo into `demo_command.cpp` dispatch. All 230 existing tests pass and the demo builds with zero warnings.
**Artifacts**:
- `src/cmd/demo/free_camera_demo.h` — created
- `src/cmd/demo/free_camera_demo.cpp` — created
- `src/engine/platform/platform.h` — modified: added `delta_time()` pure virtual
- `src/engine/platform/platform_sdl3.h` — modified: added delta_time member + override
- `src/engine/platform/platform_sdl3.cpp` — modified: delta computation in poll_events
- `src/engine/platform/platform_headless.h` — modified: added delta_time override
- `src/engine/platform/platform_headless.cpp` — modified: implemented delta_time
- `src/cmd/commands/demo_command.cpp` — modified: registration + dispatch
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**:
Implementation review passed. All 26 Done Criteria (DC-001 through DC-026) are satisfied. The implementation matches the spec and contract exactly: demo files in `src/cmd/demo/`, registration in `demo_command.cpp`, Platform `delta_time()` engine change, WASD + mouse look + Space/Control controls, ECS-based rendering via `RenderSystem`, CONST-001 compliance confirmed. All 230 existing tests pass. Demo compiles with zero warnings and runs correctly (window opens, cube renders, input loop runs).
**Artifacts**:
- `.specs/sprint-2026-05/free-camera-demo/code-review.md`
**Questions for human**:
- Visual verification of mouse look and movement requires manual testing — please run `./build/debug/buddd demo free-camera` and confirm WASD, mouse look, Space/Control, and Escape exit all work correctly.
**Warnings**:
none
**Blocking issues**:
none

## adr-agent

**Status**: completed
**Summary**:
No ADR needed. `Platform::delta_time()` is an incremental extension of the existing Platform abstraction (same pattern as `poll_events()` and `input_system()`), not a new architectural pattern. ADR-003 already established the precedent for adding virtual methods to Platform, and ADR-004 already covers the demo architecture. The free-camera demo follows both patterns exactly.
**Artifacts**:
- none
**Decisions needed**:
none — no ADR decisions identified
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## constitution-agent

**Status**: completed
**Summary**:
Evaluated the free-camera demo feature against all four existing constitution rules (CONST-001 through CONST-004). No constitution changes are needed. CONST-001 is respected (Platform::delta_time() follows the established abstraction pattern, no backend headers in demo code). CONST-002 is respected (interactive demo, no test required). CONST-003 and CONST-004 are still TODO and unaffected.
**Artifacts**:
- none
**Changes needed**:
none
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**:
Fixed incorrect Escape exit message in data-flow.md: line 32 previously stated that Escape prints "Demo aborted by user", but the spec and implementation specify Escape prints "Demo complete: free-camera (interactive)" (window close prints "Demo aborted by user").
**Artifacts**:
- `docs/wiki/architecture/data-flow.md` — modified
**Changes made**:
- **data-flow.md**: Corrected the free-camera exit behavior description on line 32 — Escape now correctly documents "Demo complete: free-camera (interactive)" via `std::cerr` (not "Demo aborted by user"), and window close exit behavior ("Demo aborted by user") is now also documented.
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
Re-verification (wiki fix) completed. The blocking issue is resolved: `docs/wiki/architecture/data-flow.md` line 32 now correctly documents that Escape exits with "Demo complete: free-camera (interactive)" (window close exits with "Demo aborted by user"), matching SPEC-015 §Output and the verified implementation. No remaining blocking issues. All governance artifacts are coherent.

**Artifacts**:
- `.specs/sprint-2026-05/free-camera-demo/governance-review.md`
**Questions for human**:
none
**Warnings**:
- ADR-011 is an empty file (0 lines) — no impact on this feature but should be addressed
- Extra include `input/input_system.h` in implementation not listed in contract (non-blocking, code review noted it)
- AC-007 verification method: spec says "unit test", but contract says "code review" — resolved during review cycles, not a practical issue
- SC-004: spec says ≤120 lines, contract relaxed to ≤150 — actual code is 118 lines, both satisfied

**Blocking issues**:
none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## adr-agent` and `## constitution-agent` sections must include their extra fields (`**Decisions needed**` and `**Changes needed**` respectively).
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
