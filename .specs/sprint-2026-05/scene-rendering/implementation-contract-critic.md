# Implementation Contract Review — Scene-Based Rendering (SPEC-011 / IMPL-011)

## Status

`Accepted with warnings`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MAY proceed when the status is `Accepted` or `Accepted with warnings`.

## Summary

**Final review verdict: Accepted with warnings.** All `std::optional<std::reference_wrapper<CameraComponent>>` → `std::optional<CameraComponent&>` changes have been correctly applied to the core implementation pseudo-code (sections 3–10). The contract is ADR-010 compliant and all seven behavioral changes from the human-approved API redesign are correctly integrated.

However, **9 stale patterns from the old `reference_wrapper` era remain** in the contract's documentation, test instructions, and checklist (listed below). These are non-blocking — the implementing Code Agent will write correct code by following the pseudo-code in sections 7–10 — but should be cleaned up for consistency.

### Changes verified as correct

| Change | Status |
|---|---|
| `std::optional<std::reference_wrapper<CameraComponent>>` → `std::optional<CameraComponent&>` in all API signatures | ✓ Lines 403, 408, 425 |
| `active_camera_ = camera` instead of `std::ref(camera)` | ✓ Line 415 |
| `&*active_camera_ == &camera` instead of `&active_camera_.value().get() == &camera` | ✓ Line 420 |
| RenderSystem uses `*cam_opt` instead of `cam_opt->get()` | ✓ Line 638 |
| `world.h` includes `<optional>` (already present) instead of `<functional>` | ✓ Actual `world.h` already has `<optional>` (line 4), no `<functional>` needed |
| ADR-010 compliance: nullable references use `std::optional<T&>`, non-null parameters use `T&`, no raw pointers in public API | ✓ Full compliance |
| All ADR references updated: ADR-005, ADR-010 consistency | ✓ Line 57 |

## Blocking issues

None.

## Warnings

### W-01: Stale `#include <functional>` instruction in world.h Files-to-inspect table

**File**: `.specs/sprint-2026-05/scene-rendering/implementation-contract.md`, line 68

The "Files to inspect" table for `world.h` says *"include `<functional>`"*. With `std::optional<CameraComponent&>`, `<functional>` is no longer needed. `<optional>` is already present in the actual `world.h`.

### W-02: Stale `Add #include <functional>` instruction in world.h section

**File**: `.specs/sprint-2026-05/scene-rendering/implementation-contract.md`, line 384

The contract instructs: *"Add `#include <functional>` in the include block at the top of the file."* This should be removed since `<functional>` is not needed for `std::optional<CameraComponent&>`.

### W-03: Stale `.get()` pattern in test case description

**File**: `.specs/sprint-2026-05/scene-rendering/implementation-contract.md`, line 830

Test description says *"its `.get()` references the component"*. `std::optional<CameraComponent&>` does not have `.get()`. Should read *"its `operator*` / `value()` references the component"* or simply *"references the component"*.

### W-04: Stale `->get()` pattern in test setup notes

**File**: `.specs/sprint-2026-05/scene-rendering/implementation-contract.md`, line 852

Says: *"Verify by comparing addresses via `&active_camera()->get()`"*. With `std::optional<CameraComponent&>`, `operator->` returns `CameraComponent*`, so `.get()` is invalid. The correct pattern is `&*active_camera()` or `&active_camera().value()`.

### W-05: Non-compiling example code in test notes

**File**: `.specs/sprint-2026-05/scene-rendering/implementation-contract.md`, lines 853–858

The example block shows:
```cpp
REQUIRE(&world.active_camera()->get() == &cam);
```
This code would NOT compile with `std::optional<CameraComponent&>` because `.get()` is not a member of `CameraComponent*` (which is what `operator->` returns). The correct pattern is:
```cpp
REQUIRE(&*world.active_camera() == &cam);
```

### W-06: Stale `.get()` in edge case description

**File**: `.specs/sprint-2026-05/scene-rendering/implementation-contract.md`, line 873

Says: *"destructor of CameraComponent A checks `&camera == &active_camera_.get()`"*. `std::optional<CameraComponent&>` does not have `.get()`. Should be `&*active_camera_`.

### W-07: Stale `#include <functional>` in Done criteria

**File**: `.specs/sprint-2026-05/scene-rendering/implementation-contract.md`, line 915

The Done criteria includes *`#include <functional>` added* for `world.h`. This should be removed since `<functional>` is not needed.

### W-08: Unnecessary `#include <memory>` in `camera_component.h`

**File**: `.specs/sprint-2026-05/scene-rendering/implementation-contract.md`, line 451

`camera_component.h` includes `<memory>` with the comment *"std::addressof (if needed for safe address-of)"*, but `std::addressof` is not used in the header. The header uses only a `math::Camera` value member. Remove or justify.

### W-09: Unnecessary `#include <memory>` in `render_system.h`

**File**: `.specs/sprint-2026-05/scene-rendering/implementation-contract.md`, line 587

`render_system.h` includes `<memory>` but only uses raw pointers (`RenderDevice*`, `World*`). No smart pointers or `std::addressof` are needed. Remove.

## Required changes (before next revision)

None blocking. The 9 warnings above should be addressed in a cleanup pass before the contract is finalized.

## Previously resolved items (history)

The following issues from earlier review cycles have been confirmed resolved:

- [x] **B-01 (Missing includes in `camera_component.cpp`)**: `#include "scene/entity.h"` and `#include "scene/world.h"` are present.
- [x] **W-02/W-03 (Debug logging in CameraComponent)**: `#ifndef NDEBUG`-guarded `std::cerr` output is present in `on_attach()` and destructor.
- [x] **W-04 (Missing `#include <type_traits>`)**: Present in `world.h` modifications.
- [x] **W-05 (Missing `#include "error.h"`)**: Present in `render_system.cpp`.
- [x] **W-09 (Demo material discard comment)**: Present in `cube_scene_demo.cpp`.
- [x] **API redesign**: All seven human-approved changes correctly integrated.

## ADR-010 compliance audit

| Requirement | Status |
|---|---|
| No raw pointers in public API signatures (`T*`) | ✓ Compliant. No raw pointer returns/params in any new public API. |
| `std::optional<T&>` used for nullable references | ✓ `active_camera() -> std::optional<CameraComponent&>`, `active_camera_` stored as `std::optional<CameraComponent&>`. |
| `T&` used for guaranteed non-null parameters | ✓ `register_camera(CameraComponent&)`, `unregister_camera(const CameraComponent&)`, `RenderSystem(RenderDevice&, World&)`. |
| No `const_cast` needed | ✓ Confirmed. `std::optional<CameraComponent&>` is natively const-correct. |
| Internal raw pointers (private impl) allowed | ✓ `RenderDevice* device_`, `World* world_` are private members, not public API. |

## Overall assessment

The contract is **ready for implementation**. All pseudo-code compiles correctly with `std::optional<CameraComponent&>` patterns. The 9 remaining stale-pattern warnings are documentation cleanup items — they affect test instructions and commentary but not the actual implementation guidance. The Code Agent should proceed with implementation, addressing the stale patterns in the test wording as they write the test file.
