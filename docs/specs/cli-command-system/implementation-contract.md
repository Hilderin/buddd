# IMPL-006 — CLI Command System

## Status

`Accepted`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|---|
| Approved by | Guillaume (user) |
| Date | 2026-05-29 |
| Time | ~15:30 UTC |

## Source spec

`docs/specs/cli-command-system/spec.md` (SPEC-006, Status: `Accepted`)

## Goal

Refactor `src/cmd/main.cpp` from a monolithic entry point with ad-hoc `--flag` dispatch into a proper subcommand structure. Extract each subcommand (`version`, `test`, `run`, `help`) into its own `.h`/`.cpp` pair under `src/cmd/` with a public `run()` method. Make `main.cpp` a thin dispatcher (single if/else-if chain). Extract the shared `setup_triangle()` helper into a shared utility file. Update `CMakeLists.txt` to use glob-based source discovery.

## Non-goals

- No changes to `src/engine/`, its public API, or its file structure.
- No changes to render pipeline, draw calls, shader compilation, or rendering behavior.
- No changes to `tests/`, test infrastructure, or test coverage. No unit tests for command classes (the integration-level ACs from the spec suffice — this contract replicates them in Done criteria).
- No dynamic command loading, plugin system, reflection, or runtime discovery.
- No third-party argument parsing library (C++26 standard library only).
- No subcommand aliases, tab-completion, shell integration, or i18n.
- No support for old `--test` or `--version` flags — they are dropped entirely.
- No changes to the root `CMakeLists.txt` (`add_subdirectory(src/cmd)` already exists at line 9).
- No new dependencies.

## Relevant constitution rules

- **CONST-001** (`docs/constitution/rules/CONST-001-architecture-boundaries.md`): No SDL3, OpenGL, or GLM headers outside `src/engine/`. All access to platform/graphics goes through engine abstractions. Violation must be caught in review.
- **AMEND-2026-001**: SDL3 test file exception — does NOT apply to `src/cmd/`. `src/cmd/` may never include `<SDL3/...>`.

## Relevant ADRs

- **ADR-003** (`docs/adr/003-render-pipeline-architecture.md`): Documents that `Platform::poll_events()` was added to avoid a CONST-001 exception in `main.cpp`. The render loop is an application-level concern owned by `main.cpp`. Draw calls (`draw()`, `draw_indexed()`) return `void` (precondition-based contracts, not `Result<void>`).

## Files to inspect

| File | What to look for |
|---|---|
| `src/cmd/main.cpp` | Current monolithic entry point — 217 lines containing `setup_triangle()` (static), `run_test_mode()` (static), `run_interactive()` (static), and `main()`. Old `--test`/`--version` flag dispatch. All code to be refactored. |
| `src/cmd/CMakeLists.txt` | Current CMake: manually lists `main.cpp`. To be replaced with `file(GLOB_RECURSE ...)`. |
| `src/engine/version.h` / `.cpp` | `buddd::engine::version() -> std::string_view` returns `"0.1.0"`. Used by `VersionCommand`. |
| `src/engine/error.h` | `buddd::engine::Error`, `Result<T>`, `to_string()`, `make_error()`. Commands use these for engine error reporting. |
| `src/engine/platform/platform.h` | `Platform::create(Backend) -> Result<unique_ptr<Platform>>`, `poll_events() -> bool`, `create_window(WindowConfig) -> Result<unique_ptr<Window>>`. |
| `src/engine/window/window.h` | `WindowConfig{title, width, height}`, `Window` abstract class with `width()`, `height()`. |
| `src/engine/render/render_device.h` | `RenderDevice::create(Window&) -> Result<unique_ptr<RenderDevice>>`, `create_shader()`, `create_material()`, `create_vertex_buffer()`, `begin_frame()`, `end_frame()`, `draw()`, `draw_indexed()`. |
| `src/engine/render/shader.h` | `ShaderType::{Vertex, Fragment}`, `Shader` abstract class. |
| `src/engine/render/material.h` | `Material` abstract class, `set_uniform()` overloads. |
| `src/engine/render/vertex_buffer.h` | `VertexBuffer` abstract class. |
| `src/engine/render/vertex_format.h` | `VertexAttributeType::Float3`, `VertexFormat{stride, attributes}`, `VertexAttribute{location, type, offset, normalized}`. |
| `src/engine/render/primitive_topology.h` | `PrimitiveTopology::Triangles` enum value. |
| `src/engine/CMakeLists.txt` | Shows include directory set to `${CMAKE_CURRENT_SOURCE_DIR}` (i.e., `src/engine/`) — already `PUBLIC`, so `buddd` target inherits it. All `#include` paths from `src/cmd/` are relative to `src/engine/`. |
| `CMakeLists.txt` | Root CMake — already has `add_subdirectory(src/cmd)` (line 9). No changes needed. |
| `docs/constitution/rules/CONST-001-architecture-boundaries.md` | Full boundary enforcement text; see Relevant constitution rules above. |

## Files allowed to change

| File | Change description |
|---|---|
| `src/cmd/main.cpp` | Rewrite: remove all inline command logic (`setup_triangle()`, `run_test_mode()`, `run_interactive()`, old `--test`/`--version` handling). Replace with thin if/else-if dispatch chain. Must not contain any static helper functions beyond the dispatch. The includes reference `commands/` subdirectory for command headers. |
| `src/cmd/CMakeLists.txt` | Replace `add_executable(buddd main.cpp)` with `file(GLOB_RECURSE CONFIGURE_DEPENDS ...)` glob pattern covering both `src/cmd/*.cpp` and `src/cmd/commands/*.cpp`. |

## Files to create

| File | Content summary |
|---|---|
| `src/cmd/commands/version_command.h` | Declaration of `buddd::cmd::VersionCommand` with `run(int, const char* const*) -> int`. |
| `src/cmd/commands/version_command.cpp` | Implementation: prints `"buddd <version>\n"` to stdout. |
| `src/cmd/commands/test_command.h` | Declaration of `buddd::cmd::TestCommand` with `run(int, const char* const*) -> int`. |
| `src/cmd/commands/test_command.cpp` | Implementation: 120-frame test render loop. |
| `src/cmd/commands/run_command.h` | Declaration of `buddd::cmd::RunCommand` with `run(int, const char* const*) -> int`. |
| `src/cmd/commands/run_command.cpp` | Implementation: interactive render loop (runs until window closed). |
| `src/cmd/commands/help_command.h` | Declaration of `buddd::cmd::HelpCommand` with `run(int, const char* const*) -> int`. Also declares shared `k_usage_text` constant. |
| `src/cmd/commands/help_command.cpp` | Implementation: prints usage text to stdout. |
| `src/cmd/demo_helpers.h` | Declaration of `buddd::cmd::setup_triangle()` (shared helper, not a command — lives directly in `src/cmd/` root). |
| `src/cmd/demo_helpers.cpp` | Implementation of `setup_triangle()` — extracted verbatim from current `main.cpp` (lines 26–93) with namespace change. |

## Files forbidden to change

| File | Reason |
|---|---|
| Any file under `src/engine/` | SPEC-006 goal: no changes to engine library. |
| `CMakeLists.txt` (root) | Already has `add_subdirectory(src/cmd)`. No changes needed. |
| Any file under `tests/` | Out of scope for this contract. |
| Any file under `docs/` (except this contract and files allowed to change) | Documentation updates for the new commands are tracked separately (see Documentation impact). |

## Existing conventions to follow

1. **File naming**: `snake_case` for all new files (`version_command.h`, `demo_helpers.h`, etc.).
2. **Namespace naming**: `buddd::cmd` for commands, `buddd::engine` for engine. Use namespace aliases `namespace be = buddd::engine;` and `namespace bc = buddd::cmd;` in `.cpp` files.
3. **Namespace closing comments**: `} // namespace buddd::cmd` after closing brace of namespace.
4. **Include guard**: `#pragma once` in all headers.
5. **Forward declarations**: Prefer forward-declaring engine types in headers to minimize includes (e.g., `namespace buddd::engine { class RenderDevice; }`).
6. **Error handling**: Use `be::to_string(error)` for engine error formatting. Use `std::fprintf(stderr, ...)` and `std::cerr` for error output consistently with existing code. Fatal errors (shader/material/vb creation failure) call `std::exit(EXIT_FAILURE)` as in current `setup_triangle()`.
7. **Include paths**: Relative to `src/engine/` (e.g., `#include "version.h"`, `#include "platform/platform.h"`, `#include "render/render_device.h"`). Do NOT use `engine/` prefix — the include directory is already `src/engine/`.
8. **CMake style**: Follow the existing pattern in `src/engine/CMakeLists.txt` for `file(GLOB_RECURSE CONFIGURE_DEPENDS ...)`.

## Required implementation behavior

### 1. Exact file contents

#### `src/cmd/CMakeLists.txt`

Replace the entire file content with:

```cmake
file(GLOB_RECURSE CMD_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/commands/*.cpp
)

add_executable(buddd ${CMD_SOURCES})

target_include_directories(buddd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(buddd PRIVATE buddd_engine)
```

The `target_include_directories(buddd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})` line ensures that `#include` directives in command files (e.g., `#include "demo_helpers.h"` from `commands/run_command.cpp`) resolve relative to `src/cmd/`. This allows `demo_helpers.h` in the root of `src/cmd/` to be included with a bare `"demo_helpers.h"` path from any file under `src/cmd/`.

The globs automatically pick up `.cpp` files from both `src/cmd/` (for `main.cpp` and shared helpers like `demo_helpers.cpp`) and `src/cmd/commands/` (for command files). The `CONFIGURE_DEPENDS` flag ensures re-configuration on file addition/removal. When adding a new command, just create the `.h`/`.cpp` pair in `src/cmd/commands/` and add an `else if` branch to `main.cpp` — no CMakeLists.txt change needed.

#### `src/cmd/demo_helpers.h`

Create file with:

```cpp
#pragma once

#include <memory>
#include <utility>

namespace buddd::engine {
class Material;
class RenderDevice;
class VertexBuffer;
} // namespace buddd::engine

namespace buddd::cmd {

/// Creates a coloured triangle by compiling shaders and creating a material +
/// vertex buffer.
///
/// @param device  The render device to create resources from.
/// @return        A pair of (material, vertex_buffer).
///
/// On failure, prints a FATAL error to stderr and calls std::exit(EXIT_FAILURE).
auto setup_triangle(buddd::engine::RenderDevice& device)
    -> std::pair<
        std::unique_ptr<buddd::engine::Material>,
        std::unique_ptr<buddd::engine::VertexBuffer>>;

} // namespace buddd::cmd
```

#### `src/cmd/demo_helpers.cpp`

Create file. The implementation is the `setup_triangle()` function from the current `src/cmd/main.cpp` lines 26–93, with these modifications:
- Namespace is `buddd::cmd` (not anonymous).
- Includes: `"demo_helpers.h"`, `"render/render_device.h"`, `"render/shader.h"`, `"render/material.h"`, `"render/vertex_buffer.h"`, `"render/vertex_format.h"`, `<cstdio>`, `<cstdlib>`, `<cstring>`, `<memory>`, `<span>`, `<string_view>`, `<utility>`.
- Remove the `static` keyword.
- Add `namespace bc = buddd::cmd;` alias at file scope.
- The shader source code, vertex struct, VertexFormat setup, error handling (fatal → `std::exit`), and return type remain identical to the current code.

#### `src/cmd/commands/version_command.h`

```cpp
#pragma once

namespace buddd::cmd {

class VersionCommand {
public:
    /// Prints the engine version string and exits with code 0.
    /// Extra arguments are silently ignored.
    [[nodiscard]] auto run(int argc, const char* const* argv) -> int;
};

} // namespace buddd::cmd
```

#### `src/cmd/commands/version_command.cpp`

```cpp
#include "version_command.h"
#include "version.h"

#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace be = buddd::engine;
namespace bc = buddd::cmd;

auto bc::VersionCommand::run([[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv) -> int {
    const auto ver = be::version();
    std::fwrite("buddd ", 1, 6, stdout);
    std::fwrite(ver.data(), 1, ver.size(), stdout);
    std::fwrite("\n", 1, 1, stdout);
    return EXIT_SUCCESS;
}
```

Must not include any engine header other than `"version.h"`.

#### `src/cmd/commands/help_command.h`

```cpp
#pragma once

#include <string_view>

namespace buddd::cmd {

/// Shared usage text constant, used by both HelpCommand and main.cpp's unknown-
/// command handler.
inline constexpr std::string_view k_usage_text =
    "Usage: buddd <command> [<args>]\n"
    "\n"
    "Commands:\n"
    "  run       Run the engine in interactive mode (default)\n"
    "  test      Run automated render test (120 frames, then exit)\n"
    "  version   Print version information\n"
    "  help      Show this help message\n";

class HelpCommand {
public:
    /// Prints the usage message to stdout and exits with code 0.
    /// Extra arguments are silently ignored.
    [[nodiscard]] auto run(int argc, const char* const* argv) -> int;
};

} // namespace buddd::cmd
```

Must not include any engine headers. Must not reference `buddd::engine` at all.

#### `src/cmd/commands/help_command.cpp`

```cpp
#include "help_command.h"

#include <cstdio>
#include <cstdlib>

namespace bc = buddd::cmd;

auto bc::HelpCommand::run([[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv) -> int {
    std::fwrite(k_usage_text.data(), 1, k_usage_text.size(), stdout);
    return EXIT_SUCCESS;
}
```

Must not include any engine headers. Must not reference `buddd::engine` at all.

#### `src/cmd/commands/run_command.h`

```cpp
#pragma once

namespace buddd::cmd {

class RunCommand {
public:
    /// Opens an interactive window (1024×768, title "Buddd Engine") and renders
    /// a coloured triangle until the user closes the window.
    /// Extra arguments are silently ignored.
    [[nodiscard]] auto run(int argc, const char* const* argv) -> int;
};

} // namespace buddd::cmd
```

#### `src/cmd/commands/run_command.cpp`

Implementation. Must contain the interactive render loop taken from the current `run_interactive()` function (`main.cpp` lines 160–198) with these changes:
- Namespace is `buddd::cmd` (not anonymous).
- Uses `setup_triangle()` from `demo_helpers.h` instead of a local static function.
- Includes minimally: `"run_command.h"`, `"demo_helpers.h"`, `"platform/platform.h"`, `"window/window.h"`, `"render/render_device.h"`, `"render/primitive_topology.h"`, `<cstdio>`, `<cstdlib>`, `<iostream>`, `<memory>`.
- Must NOT include `"render/shader.h"`, `"render/material.h"`, `"render/vertex_buffer.h"`, `"render/vertex_format.h"` (these are only needed by `demo_helpers.cpp`).
- `Platform::create(be::Backend::SDL3)` — backend argument must be `be::Backend::SDL3`.
- Window config: `.title = "Buddd Engine"`, `.width = 1024`, `.height = 768`.
- Prints `"Window opened: 1024x768\n"` (matching current) via `std::printf`.
- Render loop: `while ((*platform)->poll_events())` — identical to current.
- Uses `be::PrimitiveTopology::Triangles`.
- Prints `"Window closed, shutting down.\n"` via `std::printf` on exit.
- Returns `EXIT_SUCCESS` on success, `EXIT_FAILURE` on any engine init failure.
- Error messages use the same format as current code (e.g., `"FATAL: "` prefix for platform error).
- Must not accept extra arguments or warn about them — silently ignore per spec.

#### `src/cmd/commands/test_command.h`

```cpp
#pragma once

namespace buddd::cmd {

class TestCommand {
public:
    /// Opens a test window (800×600, title "Buddd Engine — Render Test") and
    /// renders a coloured triangle for 120 frames (~2 seconds at 60 FPS).
    /// If extra arguments follow "test", prints a warning to stderr but
    /// proceeds. If the user closes the window early, prints an abort message
    /// and exits with code 0.
    [[nodiscard]] auto run(int argc, const char* const* argv) -> int;
};

} // namespace buddd::cmd
```

#### `src/cmd/commands/test_command.cpp`

Implementation. Must contain the 120-frame test render loop taken from the current `run_test_mode()` function (`main.cpp` lines 98–155) with these changes:
- Namespace is `buddd::cmd` (not anonymous).
- Uses `setup_triangle()` from `demo_helpers.h`.
- Includes minimally: `"test_command.h"`, `"demo_helpers.h"`, `"platform/platform.h"`, `"window/window.h"`, `"render/render_device.h"`, `"render/primitive_topology.h"`, `<chrono>`, `<cstdio>`, `<cstdlib>`, `<iostream>`, `<memory>`, `<thread>`.
- Must NOT include `"render/shader.h"`, `"render/material.h"`, `"render/vertex_buffer.h"`, `"render/vertex_format.h"`.
- `Platform::create(be::Backend::SDL3)`.
- Window config: `.title = "Buddd Engine — Render Test"`, `.width = 800`, `.height = 600`.
- If `argc > 2` (i.e., at least one extra positional argument beyond `test`), print to stderr: `"Warning: unexpected arguments after 'test': "` followed by all arguments from `argv[2]` onward space-separated, followed by `"\n"`. Then proceed with the test.
  - Example: `buddd test foo bar` → prints `"Warning: unexpected arguments after 'test': foo bar\n"` to stderr.
- Prints `"Render test started: 120 frames\n"` to stderr via `std::cerr`.
- Loop runs for `target_frames = 120` iterations.
- Each iteration: check `(*platform)->poll_events()`, if false → print abort message, return `EXIT_SUCCESS` early.
- Between frames: `std::this_thread::sleep_for(frame_duration - elapsed)` with `frame_duration = std::chrono::milliseconds(16)`.
- On completion: prints `"Render test complete: 120 frames rendered\n"` to stderr.
- Abort message: `"Render test aborted by user (frame N)\n"` to stderr where `N` is the current frame index (0-based).
- Returns `EXIT_SUCCESS` on success or user abort, `EXIT_FAILURE` on engine init failure.

### 2. `src/cmd/main.cpp` — exact dispatch logic

The new `main.cpp` must contain ONLY the following:

```cpp
#include "commands/help_command.h"
#include "commands/run_command.h"
#include "commands/test_command.h"
#include "commands/version_command.h"

#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace be = buddd::engine;
namespace bc = buddd::cmd;

auto main(int argc, char* argv[]) -> int {
    // No positional argument -> default to run
    if (argc < 2 || argv[1] == nullptr) {
        return bc::RunCommand{}.run(argc, argv);
    }

    const std::string_view cmd{argv[1]};

    if (cmd == "run") {
        return bc::RunCommand{}.run(argc, argv);
    }

    if (cmd == "test") {
        return bc::TestCommand{}.run(argc, argv);
    }

    if (cmd == "version") {
        return bc::VersionCommand{}.run(argc, argv);
    }

    if (cmd == "help") {
        return bc::HelpCommand{}.run(argc, argv);
    }

    // Unknown command
    std::fprintf(stderr, "Unknown command: '%s'\n\n",
                 argv[1]);
    std::fwrite(bc::k_usage_text.data(), 1, bc::k_usage_text.size(), stderr);
    return EXIT_FAILURE;
}
```

Rules for the dispatch:
- **Default**: `argc < 2` or `argv[1] == nullptr` dispatches to `RunCommand`. This also handles the defensive `argc == 0` case.
- **Order**: if/else-if chain in the order: `run`, `test`, `version`, `help`, then unknown command fallthrough.
- **No `else` keyword**: the early `return` makes `else` redundant; bare `if` statements are preferred for clarity.
- **No engine headers**: `main.cpp` must NOT include any `src/engine/` headers directly (no `"version.h"`, `"platform/platform.h"`, `"window/window.h"`, `"render/..."`, `"error.h"`). The only includes are the four command headers and `<cstdio>`, `<cstdlib>`, `<string_view>`.
- **No shared constants beyond `k_usage_text`**: The unknown command handler uses `bc::k_usage_text` from `help_command.h`.
- **No old flag handling**: The if/else-if chain must NOT check for `"--test"` or `"--version"`. These strings will be caught by the unknown command handler.
- **No static helper functions**: `main.cpp` must not contain any function definition other than `main()`.

### 3. CONST-001 compliance

- No file under `src/cmd/` may `#include` any header matching `<SDL3/*>`, `<GL/*>`, `<glad/*>`, or `<glm/*>`.
- This must be verified by running: `grep -rnE '(SDL3|GL/|glad|glm)' src/cmd/` — zero matches (the recursive `-r` flag covers the `commands/` subdirectory).
- The exception AMEND-2026-001 (SDL3 in test files) does NOT apply to `src/cmd/`.

### 4. Output format correctness

Every command's stdout/stderr output must match the spec exactly:

| Command | Output stream | Exact content |
|---|---|---|
| `buddd version` | stdout | `"buddd 0.1.0\n"` |
| `buddd help` | stdout | `"Usage: buddd <command> [<args>]\n\nCommands:\n  run       Run the engine in interactive mode (default)\n  test      Run automated render test (120 frames, then exit)\n  version   Print version information\n  help      Show this help message\n"` |
| `buddd run` | stdout | `"Window opened: 1024x768\n"` then on close `"Window closed, shutting down.\n"` |
| `buddd test` | stderr | `"Render test started: 120 frames\n"` then on completion `"Render test complete: 120 frames rendered\n"` |
| `buddd test` aborted | stderr | `"Render test aborted by user (frame 0)\n"` (or other frame number) |
| `buddd test extra` | stderr (first) | `"Warning: unexpected arguments after 'test': extra\n"` then test proceeds |
| Unknown command | stderr | `"Unknown command: '<cmd>'\n\n"` followed by usage block (same content as `buddd help` output) |

## Required tests

Unit tests are required for the CLI commands that do not require a display or GPU. These test the command dispatch logic and output correctness.

### Tests to add to `tests/version_test.cpp`

Add the following test cases (tagged `[cli]`):

```cpp
TEST_CASE("buddd version outputs correct version string", "[cli]") {
    // Process-level test: invoke the buddd binary with "version" argument
    // Expect: stdout contains "buddd 0.1.0", exit code 0
    // Implementation: use std::system() or popen() to run the binary
    // and capture stdout.
}

TEST_CASE("buddd help outputs usage text", "[cli]") {
    // Process-level test: invoke the buddd binary with "help" argument
    // Expect: stdout contains "Usage: buddd <command> [<args>]" and
    // all four command names, exit code 0
}

TEST_CASE("buddd unknowncommand exits with code 1", "[cli]") {
    // Process-level test: invoke the buddd binary with an unknown command
    // Expect: stderr contains "Unknown command: 'unknowncommand'",
    // exit code 1
}

TEST_CASE("buddd with no arguments defaults to run command", "[cli]") {
    // Process-level test: invoke the buddd binary with no arguments
    // Expect: stdout contains "Window opened: 1024x768" (run mode starts)
    // Note: this test requires a display or offscreen driver.
    // Guard with #ifdef BUDDD_HAS_DISPLAY or mark as skipped if no display.
}

TEST_CASE("buddd version ignores extra arguments", "[cli]") {
    // Process-level test: invoke the buddd binary with "version extra_arg"
    // Expect: stdout contains "buddd 0.1.0", exit code 0
}

TEST_CASE("buddd help ignores extra arguments", "[cli]") {
    // Process-level test: invoke the buddd binary with "help extra_arg"
    // Expect: stdout contains usage text, exit code 0
}
```

**Implementation guidance**:
- Use `std::system()` or `popen()` to invoke the built `buddd` binary from the build directory.
- The binary path should be determined relative to the test binary at runtime, or passed via a compile-time define.
- Tests tagged `[cli]` that require a display should be guarded with `#ifdef BUDDD_HAS_DISPLAY` to match the existing pattern in `sdl3_backend_test.cpp`.

### Other commands

`TestCommand` and `RunCommand` require a display/GPU and are tested via the spec's acceptance criteria (manual verification and process-level checks where possible). No additional unit tests are required for these.

## Edge cases

| Case | Expected behavior |
|---|---|
| `buddd` (no args) | Defaults to `RunCommand`. Same as `buddd run`. |
| `buddd` (argc < 2, e.g., argc == 1) | Defaults to `RunCommand`. |
| `buddd` (argv[1] is nullptr, defensive) | Defaults to `RunCommand`. |
| `buddd RUN` (uppercase) | Unknown command error. |
| `buddd ''` (empty string) | Unknown command error — `std::string_view{""}` is not `"" == "run"` etc. |
| `buddd --test` / `buddd --version` | Unknown command error (old flags dropped). |
| `buddd --` (double dash) | `argv[1]` is `"--"`; unknown command error. |
| `buddd run extra1 extra2` | RunCommand silently ignores extras. |
| `buddd test extra` | Warning printed to stderr; test proceeds normally. |
| `buddd version --format json` | VersionCommand silently ignores extras. |
| `buddd help --verbose` | HelpCommand silently ignores extras. |
| `buddd <unknown>` | Unknown command error + usage to stderr, exit code 1. |
| Window closed before frame 0 in test mode | Abort message printed with frame 0; exit code 0. |
| Window closed before frame 0 in run mode | `poll_events()` returns false immediately; prints shutdown message; exit code 0. |
| Engine init failure (SDL3 not available) | Command prints error to stderr, returns `EXIT_FAILURE`. |
| Shader compilation failure | `setup_triangle()` prints fatal error and calls `std::exit(EXIT_FAILURE)`. This is existing behavior preserved. |
| `BUDDD_HAS_DISPLAY=OFF` | Binary links. Commands that attempt SDL3 backend will fail at `Platform::create()` time — same as current behavior. |

## Security impact

- No elevated privileges required.
- No network access.
- No secrets, credentials, or environment variables consumed.
- No new attack surface: the refactor moves code between files but does not introduce new I/O, parsing, or execution paths.

## Data and migration impact

- No persistent state, configuration files, databases, or caches.
- No migration needed.
- The old `--test` and `--version` flags are removed with no deprecation period. Users who relied on them will see the unknown command error and the usage block, which will direct them to use `buddd test` and `buddd version`.

## API compatibility impact

- The engine library (`buddd_engine`) public API is unchanged.
- The `buddd` binary's CLI interface changes:
  - **Breaking**: `buddd --version` no longer works (replaced by `buddd version`).
  - **Breaking**: `buddd --test` no longer works (replaced by `buddd test`).
  - **Backward-compatible**: `buddd` with no arguments still opens the interactive window (same as `buddd run`).
  - **New**: `buddd help` subcommand.
  - **New**: `buddd version` subcommand.
  - **New**: `buddd run` subcommand (alias for default behavior).
- The command classes (`buddd::cmd::*Command`) are new public API. Each has a single public method `run(int, const char* const*) -> int`.

## Documentation impact

- `docs/specs/cli-command-system/spec.md` already documents the new CLI interface.
- No wiki or external docs need updating as part of this contract.
- Future command additions should update the help text in `help_command.h`.

## ADR impact

No new ADR is required. Decisions in SPEC-006 were resolved in the spec:
- Dispatch via if/else-if chain (Q-02 resolved).
- `RunCommand` silently ignores extra args (Q-01 resolved).
- CMake switches to glob (Q-03 resolved).
- No base class for commands (see spec: "Each command is standalone").
- Shared `setup_triangle()` extracted to `demo_helpers.h/.cpp` (A-06).

## Constitution impact

No constitution changes are required. CONST-001 is preserved (no SDL3/OpenGL/GLM headers in `src/cmd/`). AMEND-2026-001 (SDL3 test exception) is unaffected.

## Done criteria

The contract is done when ALL of the following are satisfied:

1. [ ] **AC-001 (extraction)**: `src/cmd/main.cpp` no longer contains inline implementations of test mode, interactive mode, or version printing. The old `run_test_mode()`, `run_interactive()` static functions, and `setup_triangle()` do not exist in `main.cpp`. Verified by review.

2. [ ] **AC-002 (version_command files)**: `src/cmd/commands/version_command.h` and `src/cmd/commands/version_command.cpp` exist, declaring `buddd::cmd::VersionCommand` with `run(int, const char* const*) -> int`. Compiles.

3. [ ] **AC-003 (test_command files)**: `src/cmd/commands/test_command.h` and `src/cmd/commands/test_command.cpp` exist as specified. Compiles.

4. [ ] **AC-004 (run_command files)**: `src/cmd/commands/run_command.h` and `src/cmd/commands/run_command.cpp` exist as specified. Compiles.

5. [ ] **AC-005 (help_command files)**: `src/cmd/commands/help_command.h` and `src/cmd/commands/help_command.cpp` exist as specified. Compiles.

6. [ ] **AC-006 (no-args opens window)**: Running `buddd` with no arguments opens a 1024×768 window titled "Buddd Engine" with a coloured triangle. Closing the window exits with code 0. Manual visual verification.

7. [ ] **AC-007 (`buddd run` identical)**: Running `buddd run` produces identical behavior to `buddd` with no arguments. Manual visual verification.

8. [ ] **AC-008 (`buddd test` renders 120 frames)**: Running `buddd test` opens an 800×600 window titled "Buddd Engine — Render Test", renders for 120 frames, prints `"Render test complete: 120 frames rendered"` to stderr, and exits with code 0. Manual visual verification; stderr checked.

9. [ ] **AC-009 (`buddd test` early abort)**: Running `buddd test` and closing the window before 120 frames prints `"Render test aborted by user"` to stderr and exits with code 0. Manual verification.

10. [ ] **AC-010 (`buddd version` output)**: Running `buddd version` prints `"buddd 0.1.0"` to stdout and exits with code 0. Verified via shell.

11. [ ] **AC-011 (`buddd help` output)**: Running `buddd help` prints the usage message to stdout and exits with code 0. Verified via shell.

12. [ ] **AC-012 (unknown command)**: Running `buddd unknowncommand` prints `"Unknown command: 'unknowncommand'"` followed by the usage block to stderr and exits with code 1. Verified via shell.

13. [ ] **AC-013 (old flags rejected)**: Running `buddd --test` or `buddd --version` prints the unknown command error to stderr and exits with code 1. Verified via shell.

14. [ ] **AC-014 (CONST-001 compliance)**: `grep -rnE '(SDL3|GL/|glad|glm)' src/cmd/` returns zero matches.

15. [ ] **AC-015 (CMake glob)**: `src/cmd/CMakeLists.txt` uses `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` globbing `*.cpp` files under `src/cmd/`. The exact content matches the specification in the Required implementation behavior section. `cmake --build` succeeds and produces `build/debug/src/cmd/buddd`.

16. [ ] **AC-016 (version extra args)**: Running `buddd version extra_arg` still prints the version and exits 0. Verified via shell.

17. [ ] **AC-017 (help extra args)**: Running `buddd help extra_arg` still prints the usage message and exits 0. Verified via shell.

18. [ ] **AC-018 (test extra args)**: Running `buddd test extra_arg` prints `"Warning: unexpected arguments after 'test': extra_arg"` to stderr but still runs the test and exits 0. Verified via shell and manual visual check.

19. [ ] **SC-001 (add new command)**: Create a skeleton `info_command.h/.cpp` in `src/cmd/commands/`, wire it into `main.cpp` dispatch chain. Build succeeds without modifying any other command file or `CMakeLists.txt`. Revert the skeleton after verification.

20. [ ] **SC-002 (dispatch visible in first 30 lines)**: The command dispatch if/else-if chain is contained within the first 30 lines of `main()` in `main.cpp`. Verified by inspection.

21. [ ] **SC-003 (identical render behavior)**: Manual visual comparison confirms same triangle appearance, window dimensions, and frame count between old and new binary for `run` and `test` modes.
