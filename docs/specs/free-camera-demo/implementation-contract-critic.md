# Implementation Contract Review — SPEC-015 Free Camera Interactive Demo

> **Re-review (Cycle 2 — Engine change verification):** The contract was updated to add
> `Platform::delta_time()` engine changes (5 platform files, new DC-022 through DC-026).
> This review verifies the engine changes against actual source files and governance rules.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

None. All source-file and governance checks pass for the updated contract.

## Warnings

Non-blocking concerns for awareness:

- **Transitive `Model` include**: The contract lists `#include "render/mesh_renderer.h"` but not `"render/model.h"`. `Model` is available transitively through `mesh_renderer.h` → `model.h`, which matches the existing `cube_scene_demo.cpp` pattern. This is safe but relies on a transitive dependency. *(Unchanged from Cycle 1.)*
- **Function definition style not specified**: The contract gives the exact algorithm but does not specify whether to use a qualified name (`auto buddd::cmd::demo::run_free_camera_demo(...)`) or a namespace block. The pattern from `cube_scene_demo.cpp` uses qualified names. Unambiguous for a C++ developer but worth noting. *(Unchanged from Cycle 1.)*
- **Wiki module-map line numbers approximate**: The contract references `docs/wiki/architecture/module-map.md` lines 179–187 for the demo files table. The actual table starts at line 181. This is handled by the wiki-agent, not the Code Agent, so precision is not critical. *(Unchanged from Cycle 1.)*
- **`Uint64` vs `uint64_t` type choice**: The contract uses `Uint64` (SDL3 type alias) for the local variable in `poll_events()` but `uint64_t` (C++ std type) for the member variable. Since both resolve to the same underlying type (`unsigned long long`), this is functionally correct — but there's a minor style inconsistency. Not a blocking issue.

## Required changes

Concrete, actionable changes requested:

None.

## Suggested improvements

Optional ideas (not required):

- Consider adding `"render/model.h"` to the includes list for explicitness, even though it's transitively available.
- Consider explicitly noting in the conventions section that the function implementation should use a fully qualified return type (matching `cube_scene_demo.cpp` style) to remove any ambiguity.
- Consider using `uint64_t` instead of `Uint64` in `poll_events()` for consistency with the member variable type — both are the same underlying type, but using the C++ standard type throughout avoids SDL3 type leakage into the codebase beyond what's already there.

## Review details

### Contract-completeness check

| Area | Verdict |
|------|---------|
| **Allowed files scope** | ✅ Precisely scoped — 2 create + 6 modify (5 platform files + 1 dispatch), all others forbidden |
| **Forbidden files** | ✅ Comprehensive list covering engine (except the 5 platform files), existing demos, tests, CMake, docs, editor |
| **Missing tests** | ✅ Correctly notes no tests required (interactive demo, CONST-002) — engine change is a trivial addition to existing abstractions |
| **Conventions** | ✅ `#pragma once`, `buddd::cmd::demo` namespace, `[[nodiscard]]`, `[[maybe_unused]]`, engine include paths, `namespace be = buddd::engine;`, `std::cerr` output, frame-rate limiting pattern — all match existing codebase |
| **Hidden architecture decisions** | ✅ None found — every choice is explicitly specified, including the exact `poll_events()` reordering |
| **New dependencies without justification** | ✅ None — `<cstdint>` is C++ stdlib; `SDL_GetTicks()` is already available via existing `<SDL3/SDL.h>` include |
| **Missing migration/data impact** | ✅ Correctly identified as "none" |
| **Security impact** | ✅ Correctly identified as "none", CONST-001 enforcement specified |
| **Documentation impact** | ✅ Wiki module-map update delegated to wiki-agent |
| **ADR impact** | ✅ Correctly follows ADR-004, ADR-005, ADR-001 without modifying them |
| **Constitution impact** | ✅ Correctly identifies none — adding `delta_time()` to `Platform` reinforces the abstraction layer (CONST-001 intent), it does not violate it |

### Source-file validation results — Existing demo/engine APIs (Cycle 1 — unchanged)

| Contract assumption | Actual source | Verdict |
|---------------------|---------------|---------|
| `Entity::create(world)` | `static auto create(World& world) -> Entity` (entity.h:64) | ✅ |
| `get_component<T>() -> std::optional<T&>` | entity.h:44-45 (delegates to world.h:56) | ✅ |
| `CameraComponent::camera() -> Camera&` | camera_component.h:13 (`auto camera() noexcept -> math::Camera&`) | ✅ |
| `Camera` has `set_position`, `set_orientation`, `position()`, `orientation()` | camera.cpp:19-23 | ✅ |
| `Camera` has `set_perspective(fov, aspect, near, far)` | camera.cpp:49-54 | ✅ |
| Default `Camera` aspect is 16:9, not 4:3 | camera.h:47 (`float aspect_{16.0f / 9.0f}`) | ✅ — confirms `set_perspective` is needed |
| `Quat::from_euler(pitch, yaw, roll)` exists | quat.h:64, 85-87 | ✅ |
| `Quat * Vec3` rotates vector by quaternion | quat.h:39 (`friend auto operator*(Quat q, Vec3 v)`) | ✅ |
| `InputSystem::mouse_delta() -> pair<float,float>` | input_system.h:45 | ✅ |
| `InputSystem::is_down(KeyCode)` exists | input_system.h:37 | ✅ |
| `KeyCode::W`, `A`, `S`, `D`, `Space`, `Escape`, `ControlLeft`, `ControlRight` | key_code.h:16-17, 28, 25, 55-56 | ✅ |
| `Vec3::unit_y()` exists | vec3.h:104 | ✅ |
| `Vec3::length_squared()` exists | vec3.h:77-79 | ✅ |
| `Vec3::normalize()` exists | vec3.h:80-83 | ✅ |
| `math::radians()` exists | math.h:23 | ✅ |
| `math::epsilon` exists | math.h:20 (`inline constexpr float epsilon = 1.0e-6f`) | ✅ |
| `RenderSystem(device, world)` constructor | render_system.h:10 | ✅ |
| `RenderSystem::render()` calls begin/end_frame internally | render_system.cpp:17,44 | ✅ |
| `MeshRenderer(shared_ptr<Model>)` constructor | mesh_renderer.h:12 | ✅ |
| `World` is default-constructible | world.h:21 | ✅ |
| `CameraComponent` auto-registers via `on_attach()` | camera_component.cpp:20-21 | ✅ |
| `setup_cube(device)` returns `CubeResources` with `.model` (moveable `Model`) | demo_helpers.h:28-31, model.h:67 | ✅ |
| CMake GLOB_RECURSE picks up `src/cmd/demo/*.cpp` | CMakeLists.txt:4 | ✅ |
| `std::optional<T&>` compiles (ADR-005) | Used throughout world.h/camera_component.h | ✅ |

### Source-file validation results — Engine delta_time changes (Cycle 2 — new)

| Contract assumption | Actual source | Verdict |
|---------------------|---------------|---------|
| `Platform` abstract class exists with `virtual auto input_system() -> InputSystem& = 0` at line 33 | platform.h:33 — exactly as described | ✅ |
| Deleted copy constructors at lines 35–38 | platform.h:35-38 — `Platform(const Platform&) = delete` etc. | ✅ |
| Slot for `delta_time()` after `input_system()` and before deleted copy ctors | Lines 33-35 — empty line, perfect insertion point | ✅ |
| `PlatformSDL3` has `InputSystemSDL3 input_system_` member | platform_sdl3.h:25 | ✅ |
| `PlatformSDL3` includes `<SDL3/SDL.h>` (available for `SDL_GetTicks`) | platform_sdl3.cpp:4 — `#include <SDL3/SDL.h>` | ✅ |
| `SDL_Init(SDL_INIT_VIDEO)` called before `PlatformSDL3` construction (so `SDL_GetTicks()` is available) | platform.cpp:13 — `SDL_Init(SDL_INIT_VIDEO)` | ✅ |
| `InputSystemSDL3::begin_frame()` does not depend on timing | input_system_sdl3.cpp:24-39 — only copies key state and resets delta/wheel | ✅ — reordering (delta before begin_frame) is safe |
| `PlatformHeadless` has `InputSystemHeadless input_system_` member | platform_headless.h:25 | ✅ |
| Headless platform has no SDL dependency | platform_headless.cpp — no `<SDL3/SDL.h>` include | ✅ |

### Consistency with spec

| Spec element | Contract handling | Verdict |
|-------------|-------------------|---------|
| Function signature | Matches spec exactly | ✅ |
| Constants (k_move_speed=5.0, k_mouse_sensitivity=0.002) | Matches spec | ✅ |
| Camera initial position (0,2,5), identity orientation | Matches spec | ✅ |
| set_perspective(60°, 4:3, 0.1, 100) | Matches spec | ✅ |
| Mouse look uses mouse_delta(), NOT dt-scaled | Explicit constraint | ✅ |
| Pitch clamped to ±89° | Explicit code with `std::clamp + radians()` | ✅ |
| Yaw unbounded | Explicitly stated | ✅ |
| WASD XZ-projected forward, right, world Y up/down | Code matches | ✅ |
| Both ControlLeft and ControlRight | Both checked | ✅ |
| poll_events() before Escape is_down() | Explicit ordering + rationale | ✅ |
| "Demo started/complete/aborted" messages | Match spec | ✅ |
| Frame-rate limiting ~16ms | Matches cube_scene_demo pattern | ✅ |
| 13 edge cases from spec | Carried forward verbatim | ✅ |
| AC-007 (pitch clamp unit test) | Resolved: code-review verification | ✅ |
| SC-004 (120-line limit) | Relaxed to 150 lines (per spec-critic warning) | ✅ |
| **Spec A-15**: `delta_time()` returns seconds since last `poll_events()`, always > 0 | Contract: SDL_GetTicks ms / 1000 → float; first-frame default 1/60f; always positive under normal operation | ✅ |
| **Spec §Framerate independence**: movement scaled by `platform.delta_time()` | Contract DC-012: `movement * k_move_speed * dt` where dt = `platform.delta_time()` | ✅ |
| **Spec §Demo loop pseudocode**: `dt ← platform.delta_time()` | Contract line 248: `float dt = platform.delta_time();` | ✅ |
| **Spec A-17**: no manual chrono for delta-time | Contract DC-026: chrono used only for frame-rate limiting, not delta computation | ✅ |
| **Spec §Engine changes**: `delta_time()` on Platform, SDL_GetTicks in SDL3, 1/60f in headless | Contract §4a–4e: exact code for all three backends | ✅ |
| All 26 Done Criteria (DC-001 through DC-026) | Specified, verifiable | ✅ |

### Governance check

| Rule | Status |
|------|--------|
| **CONST-001** (Architecture Boundaries) | ✅ No backend headers in demo code. Header only forward-declares `Platform`/`RenderDevice`. Grep verification specified. Adding `delta_time()` to `Platform` is an *extension* of the abstraction layer, not a violation — it keeps demo code platform-independent. |
| **CONST-002** (Testing Policy) | ✅ No test file required for interactive demo. Engine `delta_time()` change is trivial (accessor + simple computation) and covered by existing headless platform tests. |
| **CONST-003** (Documentation Policy) | ✅ Wiki module-map update delegated to wiki-agent. |
| **CONST-004** (Security Policy) | ✅ No elevated privileges, network, or I/O. |
| **ADR-004** (Demo System Architecture) | ✅ Follows .h/.cpp pattern, single free function, if/else-if dispatch. |
| **ADR-005** (std::optional<T&> for Component Lookup) | ✅ Camera obtained as `auto&` reference, not copy. |
| **ADR-001** (Result<T> / Error Pattern) | ✅ `setup_cube` handles errors via `std::exit`. No new error handling needed. |

### Engine change verification — detailed checks

| Check | Detail | Verdict |
|-------|--------|---------|
| **platform.h — virtual method placement** | Contract places `delta_time()` after `input_system()` (line 33) and before deleted copy constructors (line 35). Actual file has line 34 blank between them. | ✅ Correct insertion point |
| **platform.h — signature correctness** | `[[nodiscard]] virtual auto delta_time() const noexcept -> float = 0;` | ✅ Pure virtual, const (getter), noexcept (no throwing), returns float (seconds) |
| **platform_sdl3.h — member placement** | Contract adds `float delta_time_` and `uint64_t last_frame_ticks_` after `input_system_` in private section | ✅ Fits existing pattern |
| **platform_sdl3.h — override declaration** | Public section: after `input_system()` | ✅ |
| **platform_sdl3.h — `#include <cstdint>`** | Required for `uint64_t` | ✅ Correctly specified |
| **platform_sdl3.cpp — `SDL_GetTicks()` return type** | Returns `Uint64` (milliseconds, SDL3 type = `uint64_t`) | ✅ Correct |
| **platform_sdl3.cpp — ms-to-s conversion** | `static_cast<float>(now - last_frame_ticks_) / 1000.0f` | ✅ Correct (ms → s) |
| **platform_sdl3.cpp — first-frame guard** | `if (last_frame_ticks_ != 0)` — default is 0, so first frame gets `1.0f / 60.0f` | ✅ Correct |
| **platform_sdl3.cpp — reordering rationale** | Delta computed BEFORE `begin_frame()`, which is named `input_system_.begin_frame()` | ✅ Verified: `InputSystemSDL3::begin_frame()` is timing-independent (only resets input state) |
| **platform_headless.h/cpp** | Override declaration + `return 1.0f / 60.0f;` | ✅ Appropriate for headless mode |
| **DC-026 — no manual chrono for dt** | Demo uses `platform.delta_time()`; chrono only for frame-rate limiting | ✅ Verified against `free_camera_demo.cpp` algorithm in the contract |

### Edge case coverage

All 13 edge cases from SPEC-015 §Edge cases are carried forward in the contract's explicit edge-case table, with correct required behavior for each. No edge cases from the spec are missing. The `platform.delta_time()` edge case (very large value from debugger breakpoint) is correctly documented.

**Summary:** The contract is complete, accurate, unambiguous, feasible, and consistent with all governance documents. It leaves no architectural decisions to the Code Agent and covers all edge cases. The engine `delta_time()` changes are correctly specified, match the existing Platform architecture, and do not violate any constitution rules.
