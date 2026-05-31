# Implementation Contract Review — Input System (IMPL-013) — Re-review: static_cast simplification

## Summary

This is a **re-review** of IMPL-013 (`docs/specs/input-system/implementation-contract.md`) after the static_cast simplification. The contract correctly eliminates all lookup-table references, simplifies event processing to inline `static_cast` + bounds check, simplifies test helpers, and removes the sync requirement.

**However, a new critical blocking issue was discovered**: the `KeyCode` enum code block in section 1 uses **implicit sequential values** (`A=1, B=2, ...`) instead of the required **SDL_Scancode-matching values** (`A=4, B=5, ...`). This contradicts the contract's own text ("Values are fixed to SDL scancode positions"), the accepted spec (which sets `A=4, B=5, ...`), and the event processing code in section 5 which casts SDL scancode values directly to array indices. A Code Agent following this contract literally would create a broken input system.

**Verdict: Reject** — 1 blocking issue (B-01) must be resolved before acceptance.

---

## Static_cast simplification verification

### 1. Consistency — leftover "mapping table" / "lookup table" references

- [x] No leftover references to a mapping function or lookup table as something that *exists*.
- [x] The 3 occurrences of "lookup table" are all correct negations: "no lookup table needed", "no lookup table or switch statement needed" — describing the absence of a table.
- [x] Line 574: `No separate \`sdl_scancode_to_key_code()\` function exists.`
- [x] Line 819: `"no table to maintain or sync"`.
- [x] The coordination.md reference to `sdl_scancode_to_key_code()` is historical context (what was removed).

**Status: CLEAN** ✓

### 2. KeyCode enum values — do they match SDL_Scancode?

- [ ] **ISSUE**: The `KeyCode` enum code block in section 1 (lines 172–219) uses **implicit sequential values** without explicit SDL_Scancode-matching initializers. For example:

```cpp
enum class KeyCode : uint8_t {
    Unknown = 0,
    A, B, C, ...   // implicit: A=1, B=2, C=3, ...
};
```

This would produce `A=1, B=2, ..., Z=26, Digit0=27, Digit1=28, ...` — **NOT** the required `A=4, B=5, ..., Z=29, Digit1=30, ...` that match `SDL_SCANCODE_A` through `SDL_SCANCODE_Z`.

- [ ] The doc comment says "Values are fixed to SDL scancode positions" but the actual code doesn't implement this.
- [ ] The event processing code in section 5 (lines 518–521, 579–583) uses `scancode` values directly as array indices via `static_cast<size_t>(scancode)` — this ONLY works if `KeyCode` values numerically equal `SDL_Scancode` values.
- [ ] The bounds check `scancode < static_cast<SDL_Scancode>(KeyCode::_Count)` breaks if `_Count` is not the correct sentinel value (> all defined SDL scancodes).
- [ ] The spec (lines 74–117) correctly defines the enum with **explicit** SDL_Scancode-matching values (`A = 4, B = 5, ..., SuperRight = 231`). The contract must either set these same explicit values or delegate to the spec with a clear requirement.

**Status: BLOCKING** — see B-01 below.

### 3. Event processing — inline static_cast + bounds check

- [x] Line 518: `"Convert via static_cast — KeyCode values match SDL_Scancode values."`
- [x] Lines 572–574: `"No separate \`sdl_scancode_to_key_code()\` function exists."` and `"Conversion is done inline in \`on_sdl_event()\` via \`static_cast<KeyCode>(scancode)\` with a bounds check"`
- [x] Lines 510–570: `on_sdl_event()` implementation uses inline bounds-check pattern.
- [x] Lines 579–583: The bounds check pattern is explicit (`scancode > SDL_SCANCODE_UNKNOWN && scancode < static_cast<SDL_Scancode>(KeyCode::_Count)`).
- [x] No switch statement or lookup table for keyboard scancode conversion.
- [x] The mouse button helper (`sdl_button_to_mouse_button`) uses a switch — this is correct because SDL mouse button constants (SDL_BUTTON_LEFT=1, etc.) don't match `MouseButton` values (Left=0, etc.).

**Status: CORRECT** ✓ (except the enum values issue in check 2 affects the correctness of the bounds check)

### 4. Test helpers — static_cast + REQUIRE

- [x] Lines 750–754: `key_code_to_sdl_scancode()` is a simple static_cast + REQUIRE.
- [x] Line 749: `"No lookup table needed because KeyCode values equal SDL_Scancode values."`
- [x] Line 819: `"The \`key_code_to_sdl_scancode\` function uses \`static_cast\` — no table to maintain or sync."`
- [x] No lookup table or switch in the test helper for keyboard scancodes.
- [x] The mouse button helper (`mouse_button_to_sdl`) uses a switch — correct because the values don't match 1:1.

**Status: CORRECT** ✓

### 5. T-17 — simplified to static_cast round-trip

- [x] Line 1036: T-17 is `"KeyCode static_cast round-trip for representative keys"`.
- [x] Tests `static_cast<KeyCode>(static_cast<SDL_Scancode>(k)) == k` for representative keys (A, Z, Digit0, Digit9, Space, Escape, ArrowUp, F12, Grave).
- [x] Clarifies: `"No lookup table needed — the static_cast-based compile-time assertion in AC-015 covers all values."`
- [x] AC linkage: AC-015 (correct).
- [x] No reference to "mapping completeness" or "unknown scancode" in T-17.

**Status: CORRECT** ✓

### 6. W-04 resolution — static_cast verification

- [x] Lines 27–28: W-04 resolution says `"Clarified as: 'All defined KeyCode values match their corresponding SDL_Scancode numeric values — verified by static_cast, no mapping function needed.'"`
- [x] This accurately describes the static_cast approach and replaces the ambiguous "mapping must be complete" phrasing.
- [x] No reference to a mapping table or mapping function in the resolution.

**Status: CORRECT** ✓

### 7. Sync requirement — removed

- [x] No "sync" requirement exists in the contract (the only match is `"no table to maintain or sync"` on line 819, which correctly negates the need).
- [x] No two separate mappings to keep in sync — the single `KeyCode` ↔ `SDL_Scancode` mapping is inherent in the enum values.

**Status: CORRECT** ✓

### 8. No new issues introduced

- [x] No contradictions with the accepted spec (except the enum values issue).
- [x] No contradictions with CONST-001 (architecture boundary maintained).
- [x] No contradictions with ADR-001, ADR-003, ADR-007.
- [x] No hidden architecture decisions.
- [x] No missing migration, data, security, documentation, or ADR impact.
- [x] AMEND-2026-001 expansion correctly flagged as handled by constitution-agent (not Code Agent).
- [x] Forbidden files list correctly protects the constitution.

**Status:** One new issue: B-01 (enum values).

---

## Pre-existing warnings (carried forward from previous review)

These non-blocking warnings from the previous review were NOT addressed (they were outside the scope of the static_cast simplification fix request):

- [ ] **W-01**: T-07 (`"Double-buffered state transitions (standalone)"`) has AC linkage AC-016 but only tests that `begin_frame()` does not crash in headless mode. The actual AC-016 transition test is T-14 (SDL3). AC-016 should be removed from T-07's linkage.
- [ ] **W-02**: `sdl_button_to_mouse_button()` default silently returns `MouseButton::Left` without any debug diagnostic (unlike scancode mapping which prints a warning for unrecognised scancodes).
- [ ] **W-03**: Documentation impact section uses passive voice ("should be updated") — no owner specified for wiki updates. Should explicitly defer to wiki-agent.
- [ ] **W-04**: `Platform::input_system()` returns `InputSystem&` (non-void) but lacks `[[nodiscard]]`, inconsistent with the contract's own convention table ("All query methods (non-void return) must be marked `[[nodiscard]]`").
- [ ] **W-05**: `input_system.h` lacks explicit `#include <cstdint>` for `uint8_t` in `MouseButton` (relies on transitive include from `key_code.h` — fragile).
- [ ] **W-06**: `Backend` forward declaration (`enum class Backend;`) uses default underlying type (`int`) but could break if the definition later adds an explicit underlying type.
- [ ] **W-07**: Documentation impact section asks for wiki updates but "Files forbidden to change" blocks `docs/` — the wiki-agent should handle this, but no explicit handoff is documented.

Previous W-04 (raw C array in `key_code_to_sdl_scancode()`) is resolved — it's now a simple `static_cast`.

---

## Blocking issues

- [ ] **B-01 — KeyCode enum values do not match SDL_Scancode values in the contract's code block**: The `KeyCode` enum definition in section 1 (lines 172–219) uses implicit sequential values (`A=1, B=2, ...`) instead of explicit SDL_Scancode-matching values (`A=4, B=5, ...`). This contradicts:
  - The contract's own doc comment: "Values are fixed to SDL scancode positions" (line 170).
  - The accepted spec (SPEC-013), which defines `A = 4, B = 5, ..., SuperRight = 231` (spec.md lines 74–117).
  - The event processing code in section 5, which casts `scancode` values directly to `size_t` for array indexing — this only works if `KeyCode` values numerically equal `SDL_Scancode` values.
  - The bounds check `scancode < static_cast<SDL_Scancode>(KeyCode::_Count)`, which breaks if `_Count` is not a sentinel greater than all defined SDL scancodes.

  **Impact**: A Code Agent implementing from this contract would produce a broken input system where key events are mapped to wrong array indices, and valid SDL scancodes are rejected by the bounds check.

  **Fix required**: Either (a) add explicit SDL_Scancode-matching initializers to all `KeyCode` enumerators in the contract's code block (matching the spec), or (b) add a clear, testable requirement that enumerator values must match `SDL_Scancode` values and reference the spec for the exact mapping. Option (a) is strongly preferred for precision.

---

## Required changes

1. Fix the `KeyCode` enum in section 1 to use explicit SDL_Scancode-matching values (as the spec does: `A = 4, B = 5, ..., SuperRight = 231`), or add a binding requirement directing the Code Agent to set them accordingly.

---

## Suggested improvements

1. Consider adding `[[nodiscard]]` to `Platform::input_system()` for consistency with the convention table (W-04).
2. Consider adding `#include <cstdint>` explicitly in `input_system.h` for `uint8_t` usage in `MouseButton` (W-05).

---

## Questions for Human

None.

---

## Overall Verdict

**Reject** — the contract has been correctly simplified for the static_cast approach across all dimensions (event processing, test helpers, T-17, W-04 resolution, sync removal), but **B-01 (KeyCode enum values don't match SDL_Scancode in the code block)** is a blocking issue that must be fixed before a Code Agent can implement from this contract.
