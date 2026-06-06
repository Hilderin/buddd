# Implementation Contract Review — Console Timestamps, FreeCameraMovement Refactoring & Helmet Investigation

**Re-review (2026-06-06)**: Re-reviewed the updated contract that adds the Updatable architectural system (auto-registration in World, cleanup in flush_destroyed/remove_component, integration in app.h/app.cpp, FreeCameraMovement multiple inheritance, app refactorings). All Updatable system changes correctly implement spec requirements. See updated coverage below.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

<none>

## Warnings

Non-blocking concerns for awareness:

- **Ambiguous test file path (Required tests, lines 617–618)**: The contract says "Add a test in the existing logging test file (e.g., `tests/logging_tests.cpp` or create a small test)". The "e.g." makes the target path ambiguous. The existing test file at `tests/logging_tests.cpp` has T-04 (line 85) which tests the ConsoleSink format — the new timestamp test should be added there. The contract should specify the exact path rather than offering alternatives. Not blocking because a competent Code Agent would default to the existing test file.

- **Incorrect file path for App base class in "Files to inspect" (line 51)**: Lists `src/cmd/apps/app.h` but the actual file is at `src/cmd/app.h`. The existing code uses `#include "app.h"` which resolves correctly because the build system includes `src/cmd/` in the search paths. The code examples in the contract are correct (they use `#include "app.h"`), but the inspection path is wrong. This does not affect implementation correctness but is imprecise.

- **Convention 9 mentions `Camera::from_euler(pitch, yaw, roll)` (line 102)**: The `Camera` class in `src/engine/math/camera.h` has **no** `from_euler()` static method. Camera orientation is set via `cam.set_orientation(Quat::from_euler(...))`. The contract's implementation code correctly uses `Quat::from_euler()` throughout. The convention description should read `Quat::from_euler(pitch, yaw, roll)` instead.

- **Minor behavioral difference on ESC exit (Sections 3–4)**: In the original `free_camera_app.cpp` and `phong_app.cpp`, pressing ESC triggers an early `return;` from `render()` (lines 106–108 and 328–331 respectively), which skips `render_system_->render_scene()` on the exit frame. After refactoring, `FreeCameraMovement::update()` returns `false`, the `each` callback returns `false` to stop iteration, but the code after the `each` block (i.e., `render_system_->render_scene()`) still executes for the frame. This means the exit frame renders one extra scene vs the original. This is unlikely to be user-visible (app exits immediately after), but if behavioral identity is critical for AC-015/AC-018, the contract should add an early `return;` after the `each` block when `running_` was set to false, matching the original behavior.

- **Line number references are fragile (Sections 3–4)**: The contract references absolute line numbers for `free_camera_app.cpp` ("lines 90–146") and `phong_app.cpp` ("lines 309–368"). These were verified correct against the current codebase, but edits from other concurrent work could shift them. The semantic anchors (`"from auto& input = ... to just before // ── Render ──"`) correctly accompany the line numbers, mitigating the risk.

- **`ConsoleSink` namespace context omitted in code snippet (Section 1)**: The contract's code block for `ConsoleSink::write()` (lines 114–131) shows only the function body without the enclosing `namespace buddd::log { ... }` block. The existing file uses `namespace buddd::log`. The contract relies on the Code Agent preserving the existing namespace, which is reasonable but not explicit.

- **App header include phong_app.h not checked for remaining dependencies**: The contract removes `yaw_`, `pitch_`, `prev_right_click_` from `phong_app.h` but does not verify whether the removed includes (`<chrono>`, `<algorithm>`) are still needed. `start_time_` is `std::chrono::steady_clock::time_point`, so `<chrono>` must remain. The optional check for `<algorithm>` is noted but should be a definite instruction: "Check if `std::clamp` or other `<algorithm>` functions are still used; if not, remove the include."

- **AC numbering mismatch in Section 7 header (Helmet investigation)**: Section 7 title says "Helmet investigation (AC-023 through AC-027)" and the Deformation subsection says "AC-024a–e, AC-025, AC-026, AC-027". These AC numbers are incorrect for the current spec — the spec numbers them AC-029a–e (vertex/index counts, min/max bounds, visual verification), AC-030 (quaternion conversion), AC-031 (TRS application), and AC-032 (single traversal). The same content is correctly addressed in the Done criteria checklist (lines 914–915) and the investigation body, but the section headers reference wrong AC IDs. Content correctness is unaffected, but the mapping is imprecise and could confuse cross-referencing.

- **AC-028 reference for system_clock::now() is wrong (Section 1)**: Line 144 (comment) says "Use `std::chrono::system_clock::now()` for wall-clock time (AC-028)." In the spec, AC-028 is "DamagedHelmet loads in under 3 seconds". The correct AC for system_clock::now() is AC-033. The behavior specified (use system_clock) is correct, but the AC cross-reference is wrong.

- **Old Required change not resolved**: The first review's Required change #1 ("Specify exact test file path instead of `e.g.`") was not addressed. The Required tests section (lines 836–838) still says "e.g., `tests/logging_tests.cpp` or create a small test" rather than specifying the exact file and insertion point. This does not block implementation (a competent Code Agent will pick the correct file) but the requested fix was not applied.

## Required changes

1. **Specify exact test file path**: Replace the ambiguous path in "Required tests" (lines 617–618) with `tests/logging_tests.cpp` and specify the exact insertion point (e.g., "after the existing `Console sink format` test case at line 131").

## Suggested improvements

- Consider adding an early `return;` after the `each<FreeCameraMovement>` block in both refactored apps when `running_` was set to false, to preserve the original ESC exit behavior exactly.
- Consider adding a brief note in the `free_camera_app.cpp` refactoring section about `#include "input/input_system.h"` removal: the include is no longer needed in the `.cpp` because input is accessed through `FreeCameraMovement::update()`.
- The `phong_app.cpp` refactoring (Section 4, step 4) removes `auto& input = ...` and `float dt = ...` lines. Note that `delta_time()` and `input_system()` are still obtained inside the `each` callback. Also note that `start_time_` and `elapsed` (lines 314–315) are unrelated to camera movement and must be preserved — the contract correctly keeps them, but a cross-reference would help.
- Consider explicitly stating the unit test assertion pattern for the ConsoleSink format test using a regex, e.g.: `REQUIRE(std::regex_search(captured, std::regex(R"(\[\d{2}:\d{2}:\d{2}\.\d{3}\] \[INFO\] \[TestTag\] test message)")));`

## Coverage verification

### All 34 ACs mapped (original AC-001 through AC-028, plus new AC-029a–AC-034)

| AC | Description | Contract section | Status |
|----|------------|-----------------|--------|
| AC-001 | ConsoleSink prepends `[HH:MM:SS.fff]` | §1 | ✅ |
| AC-002 | FileSink unchanged | §1 Non-goals, Files forbidden | ✅ |
| AC-003 | No CLI flag to disable timestamps | §1 Non-goals | ✅ |
| AC-004 | `Updatable` interface in `updatable.h` | §11.1 (lines 646–676) | ✅ |
| AC-005 | `World::add_component<T>()` auto-registers Updatable via `if constexpr` | §11.2 (lines 694–697) | ✅ |
| AC-006 | `update_updatables()` short-circuits on `false` return | §11.3 (lines 726–736) | ✅ |
| AC-007 | `App::set_running(bool)` and `virtual world()` accessor | §11.4 (lines 762–776) | ✅ |
| AC-008 | `run_app()` calls `update_updatables()` before `app.render()` | §11.5 (lines 790–798) | ✅ |
| AC-009 | `FreeCameraMovement` inherits from **both** `Component` and `Updatable` | §2 header (line 174) | ✅ |
| AC-010 | Public fields: move_speed, mouse_sensitivity, pitch_clamp_degrees, invert_yaw, invert_pitch | §2 header (lines 181–185) | ✅ |
| AC-011 | `update(InputSystem&, Window&, float dt) -> bool` with override | §2 header (line 192) | ✅ |
| AC-012 | `update()` returns false on ESC, true otherwise | §2 impl (lines 238–240) | ✅ |
| AC-013 | Right-click mouse capture toggle | §2 impl (lines 254–261) | ✅ |
| AC-014 | Mouse look from mouse_delta (yaw/pitch) | §2 impl (lines 269–278) | ✅ |
| AC-015 | WASD + Space/Ctrl movement when captured | §2 impl (lines 281–299) | ✅ |
| AC-016 | Auto-finds CameraComponent via `entity().get_component<>()` | §2 impl (line 243) | ✅ |
| AC-017 | Private state: yaw, pitch, prev_right_click | §2 header (lines 195–197) | ✅ |
| AC-018 | `free_camera_app` overrides `world()`, no manual `each<>` in render | §3 | ✅ |
| AC-019 | `free_camera_app.h` removes yaw/pitch/prev_right_click | §3 (lines 339–344) | ✅ |
| AC-020 | free_camera_app behaviour identical to before | §3 behaviour preservation | ✅ |
| AC-021 | `phong_app` overrides `world()`, no manual `each<>` in render | §4 | ✅ |
| AC-022 | `phong_app.h` removes yaw/pitch/prev_right_click | §4 (lines 399–405) | ✅ |
| AC-023 | phong_app behaviour identical (camera start, orbiting lights) | §4 behaviour preservation | ✅ |
| AC-024 | `gltf_helmet_app.h/.cpp` exist in `src/cmd/apps/` | §5 (lines 415–569) | ✅ |
| AC-025 | `gltf-helmet` scene dispatches in `main.cpp` | §6 (lines 571–584) | ✅ |
| AC-026 | Window title "Buddd Engine — glTF Helmet", 1280×720 | §5 config (lines 441–443) | ✅ |
| AC-027 | Loads DamagedHelmet via AssetManager, FreeCameraMovement, directional light | §5 setup (lines 499–561) | ✅ |
| AC-028 | Load time < 3s | §7 performance (lines 602–613) | ✅ |
| AC-029a | Vertex count 14556 verified | §7 deformation step 4 | ✅ |
| AC-029b | Index count 46356 verified | §7 deformation step 4 | ✅ |
| AC-029c | Position min/max bounds ±0.001 tolerance | §7 deformation step 4 | ✅ |
| AC-029d | Node rotation `[0.7071,0,-0,0.7071]` → correct engine Quat | §7 deformation step 1 | ✅ |
| AC-029e | No visual artifacts (missing triangles, inverted faces) | §7 deformation step 5 | ✅ |
| AC-030 | Quaternion conversion verified in `build_node()` | §7 deformation step 1 | ✅ |
| AC-031 | TRS application verified correct (single-pass, no double) | §7 deformation step 2 | ✅ |
| AC-032 | Single traversal of hierarchy (no double traversal) | §7 deformation step 3 | ✅ |
| AC-033 | `std::chrono::system_clock::now()` used for wall-clock time | §1 (lines 124–128) | ✅ |
| AC-034 | Destroyed/removed Updatable unregistered from `updatables_` (no dangling ptr) | §11.6 (lines 823–831), §11.2 (remove_component), §11.3 (flush_destroyed) | ✅ |

### All 6 ECs mapped (EC-010 added in spec update)

| EC | Description | Contract coverage |
|----|------------|-------------------|
| EC-001 | `dt = 0` → no movement, no div-by-zero | §2 edge cases |
| EC-002 | No CameraComponent → warning once, return true | §2 impl + edge cases |
| EC-003 | No input → mouse_delta=(0,0), no drift | §2 edge cases |
| EC-004 | Rapid right-click toggle → edge detection | §2 impl + edge cases |
| EC-005 | `system_clock::now()` throws → propagate (no try/catch, not noexcept) | §1 (lines 147–148) |
| EC-006 | Helmet texture load failure → magenta fallback | §7, edge cases table |
| EC-007 | Helmet quaternion conversion | §7 deformation step 1 |
| EC-008 | Index type Uint16 with 46356 indices | §7 deformation step 4 |
| EC-009 | Multiple root nodes in glTF scene | §7, edge cases table |
| EC-010 | Entity/component with Updatable destroyed/removed → no dangling pointer | §11.2 (remove_component), §11.3 (flush_destroyed both branches), §11.6 |

### All 5 ERs mapped

| ER | Description | Contract coverage |
|----|------------|-------------------|
| ER-001 | glTF parse failure → error, exit | Edge cases table |
| ER-002 | Texture load failure → warning, magenta | Edge cases table |
| ER-003 | Window creation fails at 1280×720 → fallback | Edge cases table |
| ER-004 | Component before entity attachment → warning, no-op | §2 impl (missing_camera_warned_) |
| ER-005 | Non-existent textures → warning, magenta | Edge cases table |

### Updatable system — specific design correctness checks

| Check | Requirement | Contract evidence | Status |
|-------|------------|-------------------|--------|
| U-01 | `updatable.h` is pure abstract, no Component dependency | §11.1 — no include of component.h, class `Updatable` only | ✅ |
| U-02 | `updatables_` is a private `std::vector<Updatable*>` member | §11.2 — `std::vector<Updatable*> updatables_;` | ✅ |
| U-03 | `add_component<T>()` uses `if constexpr (std::is_base_of_v<Updatable, T>)` for registration | §11.2 — exact code shown | ✅ |
| U-04 | `remove_component<T>()` unregister via `dynamic_cast<Updatable*>` + `std::erase` before erase | §11.2 — full template shown with cleanup BEFORE `components_.erase(it)` | ✅ |
| U-05 | `flush_destroyed()` cleanup in **both** parent-linked and root-entity branches | §11.3 — code for both branches explicitly shown | ✅ |
| U-06 | `update_updatables()` iterates, short-circuits on false | §11.3 — code shown | ✅ |
| U-07 | `app.h` adds `virtual auto world() noexcept -> World*` (default nullptr) | §11.4 — exact code | ✅ |
| U-08 | `app.h` adds `void set_running(bool)` | §11.4 — exact code | ✅ |
| U-09 | `run_app()` calls `update_updatables()` before `app.render()` with false→`set_running(false)` | §11.5 — exact code segment | ✅ |
| U-10 | `world.h` includes `updatable.h` (need complete type for `std::is_base_of_v`) | §11.2 — explicit instruction to include | ✅ |
| U-11 | Dangling pointer safety for World destruction (no explicit cleanup needed) | §11.6 — reasoning: no subsequent update_updatables call possible | ✅ |
| U-12 | FreeCameraMovement uses multiple inheritance: `Component` + `Updatable` | §2 — `class FreeCameraMovement : public Component, public Updatable` | ✅ |
