# Data Flow

## CLI data flow

At the bootstrap stage, the CLI binary uses a Command pattern dispatch:

```
User invocation
      │
      ▼
main(int argc, char* argv[])
      │
      ├── argc < 2 or argv[1] == nullptr ?
      │       └── YES ──► RunCommand.run(argc, argv) ← default
      │
      ├── argv[1] == "run"     ──► RunCommand.run(argc, argv)
      ├── argv[1] == "demo"    ──► DemoCommand.run(argc, argv)
      ├── argv[1] == "capture" ──► CaptureCommand.run(argc, argv)
      ├── argv[1] == "version" ──► VersionCommand.run(argc, argv)
      ├── argv[1] == "help"    ──► HelpCommand.run(argc, argv)
      │
      └── Unknown command ──► fprintf(stderr, "Unknown command: '%s'\n", argv[1])
                              fwrite(k_usage_text, stderr)
                              return EXIT_FAILURE
```

Each command produces its own output:

| Command | stdout | stderr |
|---|---|---|
| `run` / (default) | `"Window opened: 1024x768"` then `"Window closed, shutting down."` | — |
| `demo <name>` | — | `"Demo started: <name> (N frames)"` then `"Demo complete: <name> (N frames rendered)"` (or abort: `"Demo aborted by user (frame N)"`). Interactive demos (`free-camera`) print `"Demo started: free-camera (interactive)"`. On Escape, they exit with `"Demo complete: free-camera (interactive)"` via `std::cerr`. On window close, they exit with `"Demo aborted by user"` via `std::cerr`. If no name: demo usage text. If unknown name: `"Unknown demo: '<name>'"` + usage. |
| `version` | `"buddd 0.1.0"` | — |
| `help` | Usage text (5 commands: `run`, `demo`, `capture`, `version`, `help`) | — |
| `capture <scenario> [--frame N] [path]` | `"Captured: <path>"` | `"Capturing: <scenario> (N frame(s))"` then error or success. If no scenario: `"Usage: buddd capture <scenario>"` + scenario list. If unknown scenario: `"Unknown capture scenario: '<name>'"` + usage. If extra args: `"Warning: unexpected arguments..."`. |
| Unknown (including `test`) | — | `"Unknown command: '<cmd>'"` + usage text |

The old `--test` and `--version` flags are removed — they are caught by the unknown-command handler.

## Test data flow

```
Catch2 test runner
      │
      ▼
TEST_CASE("engine version is non-empty", "[sanity]")
      │
      └── REQUIRE_FALSE(buddd::engine::version().empty())
              │
              └──► Calls version() → returns "0.1.0" → .empty() is false → test passes
```

## Version string source

The version string `"0.1.0"` is defined in a single location:

```
src/engine/version.cpp  ──►  return "0.1.0";
```

It is consumed by:
- `src/cmd/commands/version_command.cpp` (via `buddd::engine::version()`)
- `tests/version_test.cpp` (via `buddd::engine::version()`)

The version in `CMakeLists.txt` (`project(buddd VERSION 0.1.0 ...)`) must be kept in sync with `version.cpp` manually — no automation is introduced at bootstrap.

## Platform abstraction lifecycle

The platform abstraction layer follows a linear three-phase lifecycle:

```
Platform::create(Backend)
        │
        ▼
  [Platform initialized]
  - SDL3 backend: SDL_Init(SDL_INIT_VIDEO) called
  - Headless backend: no-op initialization
        │
        ▼
platform->create_window(WindowConfig{title, width, height})
        │
        ├── Valid config (width>0, height>0)
        │       │
        │       ▼
        │   [Window created]
        │   - SDL3 backend: SDL_CreateWindow with SDL_WINDOW_OPENGL flag
        │   - Headless backend: in-memory width/height storage
        │
        └── Invalid config (width≤0 or height≤0)
                │
                ▼
            Error{WindowCreationFailed, "Invalid window dimensions"}
        │
        ▼
RenderDevice::create(window)
        │
        ├── native_handle() != nullptr (SDL3 backend)
        │       │
        │       ▼
    │   [OpenGL 4.5 Core context created with 24-bit depth buffer]
    │   - SDL_GL_SetAttribute for Core profile 4.5
    │   - SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)
    │   - SDL_GL_CreateContext
    │   - SDL_GL_MakeCurrent
    │   - RenderDeviceOpenGL constructor enables GL_DEPTH_TEST (GL_LESS)
        │       │
        │       ▼
│   device->begin_frame() → glClearColor(0.02f, 0.02f, 0.05f, 1.0f); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
│   device->end_frame()   → SDL_GL_SwapWindow()
│   device->read_pixels() → glReadBuffer(GL_BACK); glReadPixels(...)
│                          (must be called before end_frame() to read the
│                           freshly rendered back buffer before the swap)
│
└── native_handle() == nullptr (Headless backend)
                │
                ▼
            [Headless render device]
            - begin_frame() and end_frame() are no-ops
            - size() returns stored dimensions
        │
        ▼
    [Frame loop: poll_events() orchestrates input and rendering]
    - Each poll_events() call:
        1. Computes delta_time from SDL_GetTicks () — time since the previous
           poll_events() call in seconds. First call after construction returns 1/60.
        2. Calls InputSystem::begin_frame() — copies current→previous state, resets
           accumulated mouse delta/wheel to zero.
        3. Processes SDL events — routes non-quit events to InputSystemSDL3::on_sdl_event()
           (keyboard, mouse-motion, mouse-button, mouse-wheel).
    - Application queries input state via platform.input_system() and delta time via
      platform.delta_time() between poll_events() and render/update logic.

    [Destruction order: RenderDevice → Window → Platform]
    - ~RenderDeviceOpenGL: SDL_GL_DestroyContext
    - ~WindowSDL3: SDL_DestroyWindow
    - ~PlatformSDL3: SDL_Quit()
```

### Error propagation

All factory methods (`Platform::create`, `create_window`, `RenderDevice::create`) return `Result<T>` (`std::expected<T, Error>`). On failure they return `std::unexpected<Error>` constructed via `make_error()`. The `Error` struct carries:
- `Category`: `InitFailed`, `WindowCreationFailed`, `RenderDeviceCreationFailed`, `ShaderCompilationFailed`, `LinkingFailed`, `ResourceCreationFailed`, `InvalidArgument`, `UniformNotFound`, `ReadbackFailed`, `IoFailed`, `Unsupported`, `InputInitFailed`, `Unknown`
- `code`: backend-specific numeric error code (defaults to 0)
- `message`: human-readable description

### Lifecycle rules

- `Platform` must outlive any `Window` and `RenderDevice` created from it.
- `Window` must outlive the `RenderDevice` that was created from it.
- Violating these rules is undefined behavior at the abstract level.
- The backend is fixed for the lifetime of a `Platform` instance — no runtime switching.

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — User-visible behavior, User stories 1-3
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — section 7 (`main.cpp` behavior)
- Spec: [SPEC-002](/docs/specs/platform-abstraction/spec.md) — User stories 1-5, Edge cases, Error cases
- Implementation contract: [IMPL-002](/docs/specs/platform-abstraction/implementation-contract.md) — Required implementation behavior
- Spec: [SPEC-006](/docs/specs/cli-command-system/spec.md) — CLI Command System: dispatch rules, command behaviors, output contracts
- Implementation contract: [IMPL-006](/docs/specs/cli-command-system/implementation-contract.md) — Dispatch logic, output format correctness, edge cases
- Spec: [SPEC-007](/docs/specs/cli-command-evolution/spec.md) — CLI Command Evolution: Demo System & Empty Run
- Implementation contract: [IMPL-007](/docs/specs/cli-command-evolution/implementation-contract.md) — Demo dispatch, RunCommand simplification, output text changes
- Spec: [SPEC-009](/docs/specs/3d-cube-demo/spec.md) — Model Utility & 3D Cube Demo
- Implementation contract: [IMPL-009](/docs/specs/3d-cube-demo/implementation-contract.md) — Cube demo dispatch integration, output messages
- Spec: [SPEC-010](/docs/specs/capture/spec.md) — Framebuffer Capture (ImageBuffer, Image, read_pixels, capture command, cube capture scenario)
- Implementation contract: [IMPL-010](/docs/specs/capture/implementation-contract.md)
- Spec: [SPEC-012](/docs/specs/depth-handling/spec.md) — Depth Buffer Support (24-bit depth allocation, GL_DEPTH_TEST, per-frame depth clear)
- Implementation contract: [IMPL-012](/docs/specs/depth-handling/implementation-contract.md)
- Spec: [SPEC-013](/docs/specs/input-system/spec.md) — Input System (KeyCode, InputSystem, frame-based state model, Platform integration)
