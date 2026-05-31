# Troubleshooting

## CMake configuration failures

| Symptom | Likely cause | Solution |
|---|---|---|
| `CMake Error: Could not find CMAKE_ROOT` | CMake not installed | Install CMake >= 3.28 |
| `CMake Error: Could not create named generator Ninja` | Ninja not installed or not on PATH | Install Ninja >= 1.11 |
| `The compiler does not support C++26` | Compiler too old | Use GCC 14+ or Clang 19+ |
| `Failed to download Catch2` | Network unavailable during first configure | Ensure network access; after first successful configure, FetchContent is cached |
| `Unknown argument --preset invalid` | Typo in preset name | Run `cmake --list-presets` to see available presets |
| `Catch2 repository or tag not found` | Git tag `v3.7.0` was deleted or moved | Check the tag in `CMakeLists.txt` and update if needed |
| `SDL could not find X11 or Wayland development libraries` | SDL3 requires X11 or Wayland headers to build on Linux. CI runners or minimal containers may lack these. | Pass `-DSDL_UNIX_CONSOLE_BUILD=ON` to skip the X11/Wayland check (use when `BUDDD_HAS_DISPLAY=OFF` and display is not needed). To install the headers: `sudo apt install libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxfixes-dev libxss-dev` |

## Build failures

| Symptom | Likely cause | Solution |
|---|---|---|
| `ninja: error: ...` with compiler diagnostics | Compilation error in source code | Fix the error and rebuild |
| `clang-format: command not found` | `clang-format` not installed | Install clang-format >= 18, or skip the `format` target |
| Build fails after FetchContent error | First configure failed mid-way | Clean the build dir (`rm -rf build/debug`) and reconfigure |

## Test failures

| Symptom | Likely cause | Solution |
|---|---|---|
| `ctest` reports 0 tests | Tests not built, or `catch_discover_tests` not run | Run `cmake --build --preset debug` first |
| Test binary crashes | Linking issue or missing symbol | Verify `buddd_tests` links `buddd_engine` |
| SDL3 backend tests fail | Offscreen driver hint not set, or SDL3 initialization issue | Verify tests call `SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")` before `Platform::create()`; CI uses `BUDDD_HAS_DISPLAY=OFF` to exclude these tests entirely |
| SDL3 backend tests unexpectedly compiled in CI | `BUDDD_HAS_DISPLAY` default is ON | Pass `-DBUDDD_HAS_DISPLAY=OFF` explicitly in CI configuration |

## Binary behavior

| Observation | Explanation |
|---|---|
| `buddd --help` prints the greeting | `--help` is not handled at bootstrap — any unrecognized argument falls through to the greeting branch |
| `buddd arg1 arg2` prints the greeting | Only `--version` as the sole argument is special-cased; everything else prints the greeting |
| Incremental build says "no work to do" | No source files changed — this is correct behavior |

## Platform abstraction layer

| Symptom | Likely cause | Solution |
|---|---|---|
| `Platform::create(Backend::SDL3)` fails with `InitFailed` | No display server available (e.g., headless CI, SSH session without X11 forwarding) | Use `Platform::create(Backend::Headless)` for headless environments |
| `InputSystem::create()` returns `InputInitFailed` | SDL3 input backend initialisation error (forward-compatibility; no current path fails) | Check SDL3 availability; use `Backend::Headless` for testing |
| Input queries always return false/zero | Headless platform in use, or `begin_frame()` not called before queries | Verify platform type; ensure `poll_events()` is called each frame |
| `SDL_CreateWindow` returns null | Display unavailable, or window dimensions exceed desktop limits | Verify display is available; check window dimensions |
| `SDL_GL_CreateContext` fails | OpenGL 4.5 Core profile not available on the system | Ensure the GPU driver supports OpenGL 4.5; try updating graphics drivers |
| `find_package(OpenGL REQUIRED)` fails | OpenGL development headers not installed | Install `libgl-dev` (Debian/Ubuntu) or `mesa-libGL-devel` (Fedora) |
| `FetchContent` for SDL3 fails | Network unavailable during first configure | Ensure network access; after first successful configure, SDL3 is cached locally |
| `SDL_GL_SetAttribute` errors not reported | Return values are not checked individually — they may fail silently | If context creation fails shortly after, the `SDL_GL_CreateContext` failure will produce the error |
| Architecture boundary violation (SDL3/OpenGL includes outside `src/engine/`) | Code in `src/cmd/`, `src/editor/`, or `tests/` includes SDL3 or OpenGL headers | Use the abstract `Platform`/`Window`/`RenderDevice`/`InputSystem` interfaces instead. Exception: test files under `tests/*.cpp` conditionally compiled with `BUDDD_HAS_DISPLAY=ON` may include `<SDL3/SDL.h>` for testing SDL3-dependent engine functionality per AMEND-2026-001 |
| Multiple windows from a single Platform instance | Multiple `create_window()` calls on the same Platform | This is undefined behavior — only one window is supported at this stage |
| Window destroyed before RenderDevice | Incorrect lifecycle ordering | Ensure `Window` outlives the `RenderDevice` created from it |
| Platform destroyed before Window or RenderDevice | Incorrect lifecycle ordering | Ensure `Platform` outlives all `Window` and `RenderDevice` instances |

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — Edge cases and Error cases sections
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — Edge cases section
- Spec: [SPEC-002](/docs/specs/platform-abstraction/spec.md) — Error cases, Edge cases sections
- Implementation contract: [IMPL-002](/docs/specs/platform-abstraction/implementation-contract.md) — Edge cases section
- Spec: [SPEC-003](/docs/specs/sdl3-backend-tests/spec.md) — Error cases, Constraints
- Implementation contract: [IMPL-003](/docs/specs/sdl3-backend-tests/implementation-contract.md) — Edge cases
- Spec: [SPEC-013](/docs/specs/input-system/spec.md) — Input System (Error cases, Edge cases, Permissions and security)
