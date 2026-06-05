# Implementation Contract Review — IMPL-016 (Architecture Refactor: Navigable Object Graph)

**Re-review (Loop #2):** All 4 blocking issues and 2 warnings from the previous review are confirmed resolved. No new issues found. The contract is now acceptable.

## Review scope

Evaluates `.specs/sprint-2026-05/architecture-refactor-device-window-platform/implementation-contract.md` against the accepted spec (SPEC-016), existing codebase conventions, and constitutional rules.

---

## Blocking issues

Items that must be resolved before the contract can be accepted.

- [x] **B-01: `tl::expected` / `tl::make_unexpected` do not exist in the project** — ✅ RESOLVED: Contract now uses `Result<std::unique_ptr<EngineService>>` and `std::unexpected(...)`. No `tl::expected` or `tl::make_unexpected` remains.

- [x] **B-02: Wrong include path for EngineService in test files** — ✅ RESOLVED: All test include directives use `"engine_service.h"` (not `"engine/engine_service.h"`).

- [x] **B-03: Incorrect removal of `#include "input/input_system.h"` from free_camera_demo.cpp** — ✅ RESOLVED: Contract now explicitly states to KEEP `#include "input/input_system.h"` with detailed explanation of `KeyCode` references.

- [x] **B-04: Incorrect migration pattern for `model_tests.cpp`** — ✅ RESOLVED: Contract now uses `engine.value()->device()` returning `RenderDevice&`, with comment noting `engine` stays alive for the scope.

---

## Warnings

Non-blocking concerns for awareness:

- [x] **W-01: Contradiction between "forbidden files" and "required tests"** — ✅ RESOLVED: `platform_abstraction_tests.cpp` no longer appears in the Required tests section. All new tests are directed to `tests/render_device_tests.cpp`.

- [x] **W-02: EC-012 code comment not in implementation behavior** — ✅ RESOLVED: EC-012 code comment is now inline in the `WindowSDL3::set_mouse_capture` implementation section (lines 186–194).

- **W-03: `model_tests.cpp` `create_test_material` helper still receives `RenderDevice&` correctly** — The `create_test_material(be::RenderDevice& device)` helper takes `RenderDevice&` which is compatible with `engine.value()->device()`. No issue here, but the contract should be explicit that this helper needs no changes.

- **W-04: Spec AC-038 "other invalid config" scope** — The contract tests AC-038 with only negative dimensions. This matches the only defined invalid config in the codebase (both `platform_sdl3.cpp` and `platform_headless.cpp` validate `width <= 0 || height <= 0`). Acceptable.

---

## Required changes

Concrete, actionable changes requested:

1. **EngineService.h**: Change return type from `tl::expected<std::unique_ptr<EngineService>, Error>` to `Result<std::unique_ptr<EngineService>>`. `Result` is already available via `#include "error.h"`.

2. **EngineService.cpp**: Change `tl::make_unexpected(...)` to `std::unexpected(...)` (C++23, available through `<expected>` included via `error.h`).

3. **Test file changes (items 25, 26, 27)**: Change `#include "engine/engine_service.h"` to `#include "engine_service.h"`.

4. **model_tests.cpp migration**: Change `auto& device = *engine.value()` to `auto& device = engine.value()->device()`.

5. **free_camera_demo.cpp**: Keep `#include "input/input_system.h"` (or replace with `#include "input/key_code.h"`). The code directly references `be::KeyCode` which is defined in `input/key_code.h`.

6. **Required tests table**: Remove `tests/platform_abstraction_tests.cpp` as an option since it is in the "Files forbidden to change" list.

---

## Suggested improvements

Optional ideas (not required):

- Add a note in the free camera demo behavior section to keep the `input/input_system.h` include for `KeyCode` references, or substitute with `input/key_code.h`.
- Add the EC-012 code comment instruction to the `WindowSDL3::set_mouse_capture` implementation section.
- Consider adding `#include "input/key_code.h"` to the free camera demo if `input/input_system.h` is removed, to document the minimal dependency.

---

## Full checklist verification

| Check | Status | Notes |
|-------|--------|-------|
| Allowed files too broad | ✅ | 28 files + 2 new — well-bounded |
| Missing forbidden files | ✅ | Comprehensive list |
| Missing tests | ✅ | All ACs mapped — no location ambiguity |
| Missing conventions | ✅ | Matches codebase: `snake_case_`, `auto` trailing return, `#pragma once`, etc. |
| Hidden architecture decisions | ✅ | All decisions explicit (rename to `sdl_window_`, virtual diagnostics, member order) |
| New dependencies without justification | ✅ | No new dependencies |
| Missing migration or data impact | ✅ | None |
| Missing security impact | ✅ | Covered |
| Missing documentation impact | ✅ | Covers coordination.md |
| Missing ADR impact | ✅ | ADR-010 referenced |
| Missing constitution impact | ✅ | CONST-001, CONST-002 referenced |
| Contradictions with spec | ✅ | No contradictions |
| Contradictions with constitution | ✅ | Compliant |
| Contradictions with codebase conventions | ✅ | Uses `Result<T>` and `std::unexpected` — correct |
| Test coverage completeness | ✅ | All 22 ACs mapped to tests |
| Include path correctness | ✅ | All test includes use `"engine_service.h"` |
| Edge case coverage | ✅ | All 12 ECs addressed |
| Lifetime correctness (EngineService) | ✅ | Member order guarantees correct destruction |
| Free camera demo behavior | ✅ | Mouse capture logic correct; include retained for `KeyCode` |
