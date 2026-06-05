# Spec Review — Input System (SPEC-013) — Re-review 3 (verify B-03, B-04 fixes)

## Summary

Re-review 3 verifying the two requested fixes from Re-review 2.

**Fix #1 (B-03 / A-02):** ✅ CONFIRMED FIXED.
Assumption A-02 (line 520) now reads: *"`InputSystemSDL3` converts SDL scancodes to `KeyCode` via `static_cast<KeyCode>(scancode)` with a bounds check in `input_system_sdl3.cpp`. Values corresponding to defined `KeyCode` entries are accepted; all other scancodes map to `KeyCode::Unknown`."*
No mention of "switch statement or lookup table".

**Fix #2 (B-04 / Observability):** ✅ CONFIRMED FIXED.
Line 496 now reads: *"New `KeyCode` or `MouseButton` values added after v1 | Add corresponding enum entries and update observability if needed"*
No mention of "mapping table".

**Verdict: Accepted** — both blocking issues are cleanly resolved. No new issues introduced. The spec is ready for implementation-contract authoring.

---

## What Works Well

- **KeyCode → SDL_Scancode alignment:** All enum values correctly match SDL_Scancode values (verified against SDL3 header values: A=4 through Z=29, Digit1=30 through Digit0=39, Enter=40, Escape=41, Space=44, modifiers 224–231, etc.). The approach is clean and eliminates the need for a mapping table.

- **Consistent static_cast description throughout:** The spec consistently describes conversion as `static_cast<KeyCode>(scancode)` with bounds check in the KeyEntities section (line 69), AC-006, AC-015, and Story 6. No internal contradictions remain in the main sections.

- **Constitution compliance (CONST-001):** The public API headers (`key_code.h`, `input_system.h`) expose zero SDL3 types. AC-018 explicitly verifies this via grep. Only the private SDL3 backend includes `<SDL3/SDL.h>`.

- **Error-handling pattern (ADR-001):** Factory returns `Result<std::unique_ptr<InputSystem>>`. Query methods (non-fallible by nature) return plain values — consistent with ADR-001's "where this does not apply" guidance.

- **No-raw-pointers rule (ADR-010):** All public API signatures use references (`InputSystem&`), smart pointers (`unique_ptr`), value types (`bool`, `std::pair<float,float>`), or enums. No `T*` in public headers.

- **Platform integration (ADR-003):** The `poll_events()` integration is clean — `begin_frame()` is called before the event loop, and events are routed to the embedded InputSystem. The QUIT-signal return path remains intact. This extends the existing pattern without breaking it.

- **Edge-case coverage:** The Edge cases table (lines 447–458) is comprehensive and well-reasoned. Particularly strong: rapid key-down/up in one frame (both events processed before any query), focus-loss handling (SDL sends key-up for all held keys), and the "Platform destructor while InputSystem& held" safety note.

- **Non-goals discipline:** The non-goals section is thorough (15 items) and prevents scope creep. Each exclusion is clearly motivated.

- **Double-buffered state model:** The is_down/is_pressed/is_released semantics are clearly defined with formulas and worked examples. The distinction between key state (persistent across frames) and accumulated values (reset in begin_frame()) is well-explained.

---

## Issues by Severity

### Blocking Issues

These must be resolved before the spec can be accepted for implementation-contract authoring.

- [x] **B-01: AC-006 through AC-009 describe an untestable testing approach (testability).**  
  **RESOLVED in spec update (2026-05-30).**  
  AC-006 through AC-009 now describe a testable approach: construct `Platform` via `Platform::create(Backend::SDL3)`, push synthetic events via `SDL_PushEvent()`, call `Platform::poll_events()`, and verify through the abstract `InputSystem&` interface. No private methods or constructors are accessed. The approach is testable and respects the architecture boundary.

- [x] **B-02: AC-006 scancode mapping list is incomplete (inconsistency).**  
  **RESOLVED in spec update (2026-05-30).**  
  AC-006 now includes `SuperLeft/Right` in the scancode-mapping list. AC-001 also lists them. The `KeyCode` enum already included them. All three references are now consistent.

- [x] **B-03: Assumption A-02 contradicts the static_cast approach (inconsistency).**  
  **RESOLVED in Re-review 3 (2026-05-30).**  
  A-02 now reads: *"`InputSystemSDL3` converts SDL scancodes to `KeyCode` via `static_cast<KeyCode>(scancode)` with a bounds check in `input_system_sdl3.cpp`."* No mention of "switch statement or lookup table".

- [x] **B-04: Observability section references non-existent "mapping table" (inconsistency).**  
  **RESOLVED in Re-review 3 (2026-05-30).**  
  Line 496 now reads: *"Add corresponding enum entries and update observability if needed."* No mention of "mapping table".

---

### Warnings (Non-blocking)

- **W-01: Missing include dependency in `InputSystem` class code example (clarity).**  

  The `InputSystem::create(Backend)` method uses the `Backend` enum, which is defined in `src/engine/platform/platform.h`. However, the code example at lines 139–203 only shows:
  ```cpp
  #include "error.h"
  #include "input/key_code.h"
  ```
  It does not show the required `#include "platform/platform.h"` (or wherever `Backend` is obtained). The spec should document this include to avoid ambiguity during implementation.  

  Similarly, `Platform` (in `platform.h`) requires a forward declaration of `InputSystem` before the `virtual auto input_system() -> InputSystem& = 0;` line. The spec lists `platform.h` as modified but does not show the forward declaration; adding it to the modified-file description in A-11 would help.

- **W-02: `InputInitFailed` added but never used (inconsistency).**  

  Assumption A-06 states that `Error::Category` gains a new value `InputInitFailed`. However, the Error cases table (line 465) uses `Error::Category::InitFailed` (the existing generic category) for the forward-compatibility error path, not `InputInitFailed`. This means the new category is added to `error.h` but never referenced in any code path. Either:
  - Use `InputInitFailed` in the error case (replacing `InitFailed`), or
  - Defer adding `InputInitFailed` until a real error path uses it, and document that the spec does not require adding it now.  

  The current state is not technically wrong but is confusing — the spec adds a code element that has no purpose in v1.

- **W-03: AC-012/AC-013 indirect verification of `begin_frame()` call is imprecise (testability).**  

  AC-012 says: *"verify that begin_frame() was called (e.g., via a test accessor or coverage)"*. This is hand-wavy. The spec should specify a concrete verification mechanism. Following the precedent of `RenderDeviceHeadless` (which has diagnostic counters like `frame_begin_count_` with public accessors), the `InputSystemHeadless` (or a test wrapper) could expose a counter incremented by `begin_frame()`. Alternatively, the indirect test (push event → poll → verify accumulators reset) should be described.

- [x] **W-04: AC-015 "mapping completeness" wording is ambiguous (clarity).**  
  **RESOLVED in spec update (KeyCode = SDL values).**  
  AC-015 now clearly states: *"Compile-time assertion or unit test verifies that `static_cast<uint8_t>(KeyCode::A) == SDL_SCANCODE_A`, `static_cast<uint8_t>(KeyCode::Escape) == SDL_SCANCODE_ESCAPE`, etc. for all defined `KeyCode` values."* This is unambiguous.

- **W-05: AC-006–AC-009 use `SDL_PushEvent()`, which may require expanding AMEND-2026-001 (constitutional compliance).**  

  AC-006 through AC-009 describe integration tests that push synthetic events via `SDL_PushEvent()`, which requires including `<SDL3/SDL.h>` in the test file. The current AMEND-2026-001 exception permits this include only *"for setting video driver hints (e.g., `SDL_SetHint(SDL_HINT_VIDEODRIVER, 'dummy')`)"* — the mitigation explicitly says "code review must verify that `#include <SDL3/SDL.h>` is used only for `SDL_SetHint()`." Using `SDL_PushEvent()` to inject synthetic events goes beyond this narrow scope.

  **This is not a spec defect** — the spec describes what to test and how to verify results through the public API. But the implementation contract will need to resolve this by either (a) expanding AMEND-2026-001 to cover `SDL_PushEvent()` for synthetic event injection in input system tests, or (b) creating an engine-side test helper in `src/engine/` (where SDL3 includes are unrestricted) that exposes `SDL_PushEvent()` to test code wrapped in `#ifdef BUDDD_HAS_DISPLAY`.

  The governance-reviewer step can assess the constitutional impact once the implementation contract proposes a concrete approach.

- **W-06: AC-015 compile-time assertion location and CONST-001 (clarity).**  
  **NEW.**  
  AC-015's verification states: *"Compile-time assertion or unit test verifies that `static_cast<uint8_t>(KeyCode::A) == SDL_SCANCODE_A` ... for all defined `KeyCode` values."*  
  The `SDL_SCANCODE_*` macros are defined in `<SDL3/SDL.h>` (or `SDL_scancode.h`). If the compile-time assertion is placed in the public header `key_code.h`, it would require including SDL3 headers in a public header, violating CONST-001.  
  The spec should clarify that the assertion must reside in an implementation file (`.cpp`) where SDL3 includes are private — for example, in `input_system_sdl3.cpp` or a dedicated test file. AC-015's verification cell should specify: *"Compile-time assertion in `input_system_sdl3.cpp` (or equivalent implementation file), or unit test in `tests/` that includes `<SDL3/SDL.h>` privately."*

---

## Required Changes

All required changes from Re-review 2 have been verified as resolved:

1. **[x] Fix B-03:** ✅ A-02 now describes `static_cast<KeyCode>(scancode)` with bounds check.
2. **[x] Fix B-04:** ✅ Observability section now says "Add corresponding enum entries".

---

## Suggested Improvements

- **S-01: Document the `input/` → `platform/` dependency.**  

  `input_system.h` needs to include `platform/platform.h` (or equivalent) for the `Backend` enum. This creates a dependency from the `input/` submodule to the `platform/` submodule. Within a single static library (`GLOB_RECURSE`) this is fine, but it should be documented in the spec's Assumptions or the wiki module map to avoid confusion during future refactoring.

- **S-02: Consider adding `Backend` include to the `InputSystem` code block.**  

  The class code example in section 1 should show `#include "platform/platform.h"` (or the include that provides `Backend`), so implementers know which header to add. This is a small fix that prevents a build-error-first approach.

- **S-03: Clarify AC-015 compile-time assertion location.**  

  Add a note to AC-015's verification cell stating that the static assertion must be placed in a `.cpp` file or test file, not in `key_code.h`, to avoid CONST-001 violations (see W-06).

---

## Questions for Human

- None. Both blocking issues have been resolved. Spec is ready for implementation-contract authoring.

---

## Overall Verdict

**Accepted** — Re-review 3 confirms both fixes are cleanly applied. B-03 (A-02 no longer says "switch statement or lookup table") and B-04 (Observability no longer says "mapping table") are both resolved. No new issues introduced. The spec is ready for implementation-contract authoring.
