# Implementation Contract Review — SPEC-029 (Editor Scene State + [[nodiscard]] Fixes)

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [ ] None found.

## Warnings

Non-blocking concerns for awareness:

- **Include ordering justification is factually incorrect**: The contract states that `#include "scene/world.h"` should go after `#include "shortcut_registry.h"` and justifies it as "alphabetical order among project includes: `scene/world.h` comes after `shortcut_registry.h`." Alphabetically, `scene/` < `shortcut_` (c < h), so `scene/world.h` should come *before* `shortcut_registry.h`. However, the explicit placement instruction (between `shortcut_registry.h` and `<memory>`) is unambiguous and will produce a correct build regardless. The Code Agent should follow the explicit instruction, not the inaccurate justification.
- **Test Case 3 (`world() valid after setup failure`) does not call `shutdown()` explicitly**, relying entirely on the Editor destructor. This is correct behavior (the destructor calls `shutdown()`), but the test could be made more explicit by calling `shutdown()` before the editor goes out of scope, matching the pattern in Test Case 2.
- **No explicit Done criterion for "all existing tests continue to pass"** — The contract has DC-08 ("All `[editor][scene_state]` tests pass") but does not have a DC verifying no regression in pre-existing tests (e.g., the 8 `[editor][command]` tests and the existing `[editor]` lifecycle test). This is implied by the build/CI verification (DC-07, DC-08) but could be made explicit.

## Required changes

None. The contract is complete and implementable as-is. The single warning (include ordering justification) does not require a change — the explicit placement instruction is correct and unambiguous.

## Suggested improvements

Optional ideas (not required):

- Add a Done criterion: `[ ] **DC-11**: All pre-existing tests (including `[editor][command]` and `[editor]` tagged tests) continue to pass with no regressions.`
- In Test Case 3, add `editor.shutdown();` before the implicit destructor call to be more explicit about the lifecycle phase being tested, or add a comment clarifying that the destructor handles it.
- Fix the parenthetical justification on line 90 of the contract: change `(alphabetical order among project includes: scene/world.h comes after shortcut_registry.h)` to `(project include grouping: scene/world.h placed after shortcut_registry.h to keep editor infrastructure includes contiguous)`.

## Summary

**Verdict: ACCEPTED** — No blocking issues. The contract covers all 10 acceptance criteria (AC-001 through AC-010), all spec goals (G-01 through G-05), all edge cases, and the zero-warnings requirement. Implementation steps are ordered, specific, and unambiguous. Files-allowed and files-forbidden are correctly scoped. Done criteria (DC-01 through DC-10) are verifiable. Conventions match existing code patterns. ADR references are accurate. The single warning (inaccurate alphabetical justification) does not affect implementability.

### AC coverage verification

| AC | Covered by | Status |
|---|---|---|
| AC-001 (unique_ptr<World> in constructor) | Step 2, DC-02 | ✅ |
| AC-002 (world() valid before setup) | Step 3, Test 1, DC-06 | ✅ |
| AC-003 (World destroyed with Editor) | DC-09 (ASan/Valgrind) | ✅ |
| AC-004 (World empty on construction) | Step 3, Test 1 + Test 4, DC-06 | ✅ |
| AC-005 (World outlives shutdown) | Step 3, Test 2, DC-05, DC-06 | ✅ |
| AC-006 (Zero warnings build) | Step 4, DC-07 | ✅ |
| AC-007 ([[nodiscard]] on world()) | Step 1, DC-01 | ✅ |
| AC-008 (world() always safe, no assertions) | Step 1 + Step 2 (constructor design), Test 1+2 | ✅ |
| AC-009 (World persists through setup failure) | Step 3, Test 3, DC-06 | ✅ |
| AC-010 (shutdown() does not reset world) | Step 2 (item 4), DC-05, Test 2 | ✅ |

### Consistency checks

| Reference | Result |
|---|---|
| ADR-027 (Editor Architecture) | ✅ — Direct member pattern, namespace, static library boundary respected |
| ADR-029 (Editor UX Decisions) | ✅ — Foundation for future Play mode; no UX changes in this phase |
| ADR-011 (No discard conventions) | ✅ — world() declared [[nodiscard]]; existing fixes verified |
| ADR-001 (Result/Error pattern) | ✅ — Constructor throws bad_alloc (consistent with OOM handling); setup() returns Result |
| ADR-019 (Architecture Boundaries) | ✅ — No engine changes; world.h include is permitted engine abstraction |
| Existing editor.h/editor.cpp code | ✅ — All conventions (namespace, include style, logging, member order, lifecycle) respected |
| World class (world.h) | ✅ — Used as-is; entity_count(), root_entity_count(), default constructor confirmed |
| Existing test patterns | ✅ — Catch2, [editor][scene_state] tags, REQUIRE/REQUIRE_FALSE consistent |
| Wiki (module-map.md, editor-panels.md, scene-management.md) | ✅ — Wiki updates identified as documentation impact for wiki-agent; no contradictions |
