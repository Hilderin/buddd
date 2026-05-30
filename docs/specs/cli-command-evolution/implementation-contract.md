# IMPL-007 — CLI Command Evolution: Demo System & Empty Run

## Status

`Accepted`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|---|
| Approved by | Guillaume (user) |
| Date | 2026-05-29 |
| Time | ~16:00 UTC |

## Source spec

`docs/specs/cli-command-evolution/spec.md` (SPEC-007, Status: `Accepted`)

## Goal

Rename `TestCommand` → `DemoCommand`, create a per-file demo dispatch system under `src/cmd/demos/`, extract the triangle 120-frame loop into `src/cmd/demos/triangle_demo.cpp`, simplify `RunCommand` to an empty framebuffer-clear loop (no triangle), update help text to replace `test` with `demo`, update `main.cpp` dispatch so `"test"` becomes an unknown command, and update the build system glob to pick up `demos/*.cpp`.

## Non-goals

- No changes to `src/engine/`, its public API, or its file structure.
- No changes to the render pipeline, draw calls, shader compilation, or rendering behaviour (the triangle appearance in `demo triangle` is unchanged from the old `buddd test`).
- No project/module system — `RunCommand` prepares for it (empty window) but does not implement project loading.
- No changes to `VersionCommand` or its behaviour.
- No dynamic demo discovery, plugin system, reflection, or runtime registration.
- No CLI framework or third-party argument parsing library.
- No subcommand aliases (e.g., `buddd d` for `buddd demo`).
- No tab-completion or shell integration.
- No changes to the test infrastructure directory structure (`tests/`).
- No unit tests for `DemoCommand` in isolation beyond the `[cli]` integration tests.
- No new dependencies.

## Relevant constitution rules

- **CONST-001** (`docs/constitution/rules/CONST-001-architecture-boundaries.md`): No SDL3, OpenGL, or GLM headers outside `src/engine/`. All access to platform/graphics goes through engine abstractions. Must be verified by running `grep -rnE '(SDL3|GL/|glad|glm)' src/cmd/` — zero matches.
- **AMEND-2026-001**: SDL3 test file exception — does NOT apply to `src/cmd/`. `src/cmd/` may never include `<SDL3/...>`.
- **CONST-002** (`docs/constitution/rules/CONST-002-testing-policy.md`): All testable code added or modified must have corresponding unit tests. The unconditionally testable paths (`buddd demo` no name, `buddd demo unknownname`, `buddd test` unknown) must have `[cli]` tests.

## Relevant ADRs

- **ADR-003** (`docs/adr/003-render-pipeline-architecture.md`): Documents that `Platform::poll_events()` was added to avoid a CONST-001 exception. Draw calls return `void` (precondition-based contracts). The render loop is an application-level concern owned by the command code.

## Files to inspect

| File | What to look for |
|---|---|
| `src/cmd/main.cpp` | Current dispatch: includes `test_command.h`, has `if (cmd == "test")` branch. To be updated to include `demo_command.h`, use `if (cmd == "demo")`, and let `"test"` fall through to unknown. |
| `src/cmd/CMakeLists.txt` | Current glob includes `*.cpp` and `commands/*.cpp`. Must add `demos/*.cpp`. |
| `src/cmd/demo_helpers.h` | Current `buddd::cmd::setup_triangle()` declaration. Contents unchanged; will be moved to `src/cmd/demos/`. |
| `src/cmd/demo_helpers.cpp` | Current `setup_triangle()` implementation. Contents unchanged; will be moved to `src/cmd/demos/`. |
| `src/cmd/commands/test_command.h` | Current `TestCommand` declaration. To be removed. |
| `src/cmd/commands/test_command.cpp` | Current 120-frame test render loop. To be removed. Its per-frame rendering body becomes `triangle_demo.cpp`. |
| `src/cmd/commands/run_command.h` | Current doc comment says "renders a coloured triangle". Must be updated to "framebuffer clear only". |
| `src/cmd/commands/run_command.cpp` | Current includes `demo_helpers.h`, calls `setup_triangle()`, calls `draw()`. Must remove all three. |
| `src/cmd/commands/help_command.h` | Current `k_usage_text` has `test` line. Must be replaced with `demo` line matching updated spec output. |
| `src/cmd/commands/version_command.h` | Unchanged. Inspect for reference. |
| `src/cmd/commands/version_command.cpp` | Unchanged. Inspect for reference. |
| `tests/version_test.cpp` | Current `[cli]` tests. Must update help text assertions (`"test"` → `"demo"`) and add new tests. |
| `src/engine/render/render_device.h` | Engine API: `begin_frame()`, `end_frame()`, `draw()`, etc. Used by demo and run commands. |
| `src/engine/platform/platform.h` | Engine API: `Platform::create(Backend)`, `poll_events()`, `create_window()`. |
| `src/engine/render/primitive_topology.h` | `PrimitiveTopology::Triangles` enum. |

## Files allowed to change

| File | Change description |
|---|---|
| `src/cmd/main.cpp` | Replace `#include "commands/test_command.h"` with `#include "commands/demo_command.h"`. Replace `if (cmd == "test")` branch with `if (cmd == "demo")` branch calling `DemoCommand`. The `"test"` string falls through to unknown-command handler with no special handling. |
| `src/cmd/CMakeLists.txt` | Add `demos/*.cpp` glob pattern alongside the existing `*.cpp` and `commands/*.cpp` patterns. |
| `src/cmd/commands/run_command.h` | Update doc comment to reflect empty-window behaviour (no triangle). |
| `src/cmd/commands/run_command.cpp` | Remove `#include "demo_helpers.h"`, remove `setup_triangle()` call and `draw()` call, remove `#include "render/primitive_topology.h"`. The render loop becomes `begin_frame()` / `end_frame()` only. |
| `src/cmd/commands/help_command.h` | Update `k_usage_text` constant: replace the `test` line with `demo` line, remove `(default)` from `run` line, update descriptions to match spec. |
| `tests/version_test.cpp` | Update help text assertions (`"test"` → `"demo"` in test bodies). Add new `[cli]` tests for `buddd demo` no name, `buddd demo unknownname`, `buddd test` unknown, and `buddd demo triangle` (guarded by `BUDDD_HAS_DISPLAY`). |

## Files to create

| File | Content summary |
|---|---|
| `src/cmd/commands/demo_command.h` | Declaration of `buddd::cmd::DemoCommand` with `run(int, const char* const*) -> int`. |
| `src/cmd/commands/demo_command.cpp` | Implementation: parses `argv[2]` as demo name, creates platform/window/device (800×600, title "Buddd Engine — Demo: \<name\>"), dispatches via chain on demo name to per-demo function with `argc - 2, argv + 2`. If `argc < 3`, prints demo usage to stderr and returns `EXIT_FAILURE`. If unknown demo, prints `"Unknown demo: '<name>'"` + usage to stderr and returns `EXIT_FAILURE`. If `argc > 3`, prints warning to stderr. |
| `src/cmd/demos/demo_helpers.h` | **Moved** from `src/cmd/demo_helpers.h`. Same `#pragma once`, same includes, same `setup_triangle()` declaration. Namespace changed from `buddd::cmd` to `buddd::cmd::demo`. |
| `src/cmd/demos/demo_helpers.cpp` | **Moved** from `src/cmd/demo_helpers.cpp`. Same includes, same `setup_triangle()` implementation. Namespace changed from `buddd::cmd` to `buddd::cmd::demo`. |
| `src/cmd/demos/triangle_demo.h` | Declaration of `buddd::cmd::demo::run_triangle_demo()` — free function taking `Platform&`, `RenderDevice&`, `int argc`, `const char* const* argv`. |
| `src/cmd/demos/triangle_demo.cpp` | Implementation: 120-frame render loop with coloured triangle, extracted from the old `TestCommand::run()`. Uses `#include "demos/demo_helpers.h"` for `setup_triangle()`. Prints demo diagnostic messages ("Demo started/hey", etc.) |

## Files to remove

| File | Reason |
|---|---|
| `src/cmd/commands/test_command.h` | Replaced by `demo_command.h`. The old `test` subcommand is removed. |
| `src/cmd/commands/test_command.cpp` | Replaced by `demo_command.cpp`. The triangle render loop content is extracted into `triangle_demo.cpp`. |
| `src/cmd/demo_helpers.h` | Moved to `src/cmd/demos/demo_helpers.h` with unchanged contents. |
| `src/cmd/demo_helpers.cpp` | Moved to `src/cmd/demos/demo_helpers.cpp` with unchanged contents. |

## Files forbidden to change

| File | Reason |
|---|---|
| Any file under `src/engine/` | SPEC-007 non-goal: no changes to the engine library. |
| `CMakeLists.txt` (root) | Already has `add_subdirectory(src/cmd)`. No changes needed. |
| `src/cmd/commands/version_command.h` | Unchanged by this spec. |
| `src/cmd/commands/version_command.cpp` | Unchanged by this spec. |
| `src/cmd/commands/help_command.cpp` | Unchanged — its `run()` reads `k_usage_text` from the `.h` file, which is updated. |
| Any file under `docs/` (except this contract) | Documentation updates tracked separately. |

## Existing conventions to follow

1. **File naming**: `snake_case` for all files (`demo_command.h`, `triangle_demo.h`, etc.).
2. **Namespace naming**: `buddd::cmd` for commands and demos, `buddd::engine` for engine. Use namespace aliases `namespace be = buddd::engine;` and `namespace bc = buddd::cmd;` in `.cpp` files.
3. **Namespace closing comments**: `} // namespace buddd::cmd` after closing brace of namespace.
4. **Include guard**: `#pragma once` in all headers.
5. **Forward declarations**: Prefer forward-declaring engine types in headers to minimise includes.
6. **Error handling**: Use `be::to_string(error)` for engine error formatting. Use `std::fprintf(stderr, ...)` and `std::cerr` for error output. Fatal errors (shader/material/vb creation failure) call `std::exit(EXIT_FAILURE)` (existing `setup_triangle()` behaviour).
7. **Include paths**: Engine headers are relative to `src/engine/` (e.g., `#include "platform/platform.h"`, `#include "render/render_device.h"`). Files under `src/cmd/` that include `demo_helpers.h` now use `#include "demos/demo_helpers.h"` since `target_include_directories(buddd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})` sets `src/cmd/` as the include root.
8. **CMake style**: Follow the existing glob pattern in `src/cmd/CMakeLists.txt`.
9. **If/else-if chain style**: Use bare `if` statements with early `return` (no `else`), matching existing `main.cpp` style.
10. **Constexpr string constants**: Use `inline constexpr std::string_view` for help/demo usage text, matching the existing `k_usage_text` pattern.

## Required implementation behavior

### 1. `src/cmd/CMakeLists.txt` — add demos glob

Add `demos/*.cpp` to the existing glob:

```cmake
file(GLOB_RECURSE CMD_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/commands/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/demos/*.cpp
)

add_executable(buddd ${CMD_SOURCES})

target_include_directories(buddd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(buddd PRIVATE buddd_engine)
```

The `target_include_directories(buddd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})` line (already present) sets `src/cmd/` as the include root. This means `#include "demos/demo_helpers.h"` resolves to `src/cmd/demos/demo_helpers.h` from any file under `src/cmd/`.

### 2. Move `demo_helpers.h` and `demo_helpers.cpp`

Move:
- `src/cmd/demo_helpers.h` → `src/cmd/demos/demo_helpers.h` — **contents unchanged**.
- `src/cmd/demo_helpers.cpp` → `src/cmd/demos/demo_helpers.cpp` — **contents unchanged**.

The only modification is the namespace change: `buddd::cmd` → `buddd::cmd::demo` (both in the header declaration and the `.cpp` definition). All other content, includes, and function signatures remain unchanged. The include in `demo_helpers.h` (`#include "render/material.h"` and `#include "render/vertex_buffer.h"`) still resolves through the engine's public include directory.

### 3. Remove `src/cmd/commands/test_command.h` and `test_command.cpp`

Delete both files. They are replaced by `demo_command.h/.cpp` and `triangle_demo.h/.cpp`.

### 4. Create `src/cmd/commands/demo_command.h`

```cpp
#pragma once

namespace buddd::cmd {

class DemoCommand {
public:
    /// Parses argv[2] as a demo name, creates a platform/window/device (800×600,
    /// title "Buddd Engine — Demo: <name>"), and dispatches to the matching
    /// per-demo function. If no name is given, prints usage to stderr and exits 1.
    /// If the name is unknown, prints an error + usage to stderr and exits 1.
    /// Extra arguments after the demo name produce a warning to stderr.
    [[nodiscard]] auto run(int argc, const char* const* argv) -> int;
};

} // namespace buddd::cmd
```

### 5. Create `src/cmd/commands/demo_command.cpp`

```cpp
#include "demo_command.h"
#include "demos/triangle_demo.h"

#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

namespace be = buddd::engine;
namespace bc = buddd::cmd;

namespace {

/// Shared demo usage text constant.
inline constexpr std::string_view k_demo_usage =
    "Usage: buddd demo <demo>\n"
    "\n"
    "Available demos:\n"
    "  triangle     Run the triangle demo (120 frames)\n"
    "\n"
    "Demo names are case-sensitive.\n";

} // anonymous namespace

auto bc::DemoCommand::run(int argc, const char* const* argv) -> int {
    // No demo name provided
    if (argc < 3) {
        std::fwrite(k_demo_usage.data(), 1, k_demo_usage.size(), stderr);
        return EXIT_FAILURE;
    }

    const std::string_view demo_name{argv[2]};

    // Create platform, window, and render device
    auto platform = be::Platform::create(be::Backend::SDL3);
    if (!platform) {
        std::cerr << "Failed to create platform: "
                  << be::to_string(platform.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Build full title: "Buddd Engine — Demo: <name>"
    // WindowConfig::title is std::string, so concatenation creates a temporary
    // that is copied into the config.
    auto window_title = std::string("Buddd Engine \u2014 Demo: ") + std::string(demo_name);
    auto window = (*platform)->create_window({
        .title = window_title,
        .width = 800,
        .height = 600
    });
    if (!window) {
        std::cerr << "Failed to create window: "
                  << be::to_string(window.error()) << "\n";
        return EXIT_FAILURE;
    }

    auto device = be::RenderDevice::create(**window);
    if (!device) {
        std::cerr << "Failed to create render device: "
                  << be::to_string(device.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Warn about unexpected extra arguments (argv[3] and beyond)
    if (argc > 3) {
        std::fprintf(stderr, "Warning: unexpected arguments after 'demo %s':",
                     argv[2]);
        for (int i = 3; i < argc; ++i) {
            std::fprintf(stderr, " %s", argv[i]);
        }
        std::fprintf(stderr, "\n");
    }

    // Dispatch to per-demo function using if/else chain
    // Pass argc - 2, argv + 2 so the demo receives argv[0] == demo name
    if (demo_name == "triangle") {
        return buddd::cmd::demo::run_triangle_demo(**platform, **device, argc - 2, argv + 2);
    }

    // Unknown demo name
    std::fprintf(stderr, "Unknown demo: '%s'\n\n", argv[2]);
    std::fwrite(k_demo_usage.data(), 1, k_demo_usage.size(), stderr);
    return EXIT_FAILURE;
}
```

**Important notes on the implementation**:
- The window title is constructed as a `std::string` via concatenation. `WindowConfig::title` is `std::string`, so the local `std::string` variable is safely copied into the config. This is the same pattern used in the existing `run_command.cpp` where a string literal is passed directly.
- The demo-name extraction uses `argv[2]`. If the user types `buddd demo triangle`, then `argv[0]="buddd"`, `argv[1]="demo"`, `argv[2]="triangle"`.
- Extra-arguments warning iterates from `argv[3]` (original indices), i.e., only the arguments after the demo name.
- The per-demo function receives `argc - 2, argv + 2` so that inside `run_triangle_demo()`, `argv[0]` is the demo name (`"triangle"`) and `argv[1..]` are extra arguments.

### 6. Create `src/cmd/demos/triangle_demo.h`

```cpp
#pragma once

namespace buddd::engine {
class Platform;
class RenderDevice;
} // namespace buddd::engine

namespace buddd::cmd::demo {

/// Runs the triangle demo: 120-frame render loop with a coloured triangle.
/// @param platform  The engine platform (for event polling).
/// @param device    The render device (for rendering).
/// @param argc      Argument count (argv[0] is the demo name).
/// @param argv      Argument vector (argv[0] is the demo name).
/// @return          0 on success, non-zero on error.
[[nodiscard]] auto run_triangle_demo(buddd::engine::Platform& platform,
                                     buddd::engine::RenderDevice& device,
                                     int argc, const char* const* argv) -> int;

} // namespace buddd::cmd::demo
```

### 7. Create `src/cmd/demos/triangle_demo.cpp`

```cpp
#include "triangle_demo.h"
#include "demos/demo_helpers.h"

#include "platform/platform.h"
#include "render/render_device.h"
#include "render/primitive_topology.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>

namespace be = buddd::engine;

auto buddd::cmd::demo::run_triangle_demo(
    be::Platform& platform, be::RenderDevice& device,
    [[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv) -> int {

    auto [material, vb] = buddd::cmd::demo::setup_triangle(device);

    // Render loop: ~120 frames at 60 FPS (~2 seconds)
    constexpr int target_frames = 120;
    constexpr auto frame_duration = std::chrono::milliseconds(16); // ~60 FPS

    std::cerr << "Demo started: triangle (" << target_frames << " frames)\n";

    for (int frame = 0; frame < target_frames; ++frame) {
        auto frame_start = std::chrono::steady_clock::now();

        if (!platform.poll_events()) {
            std::cerr << "Demo aborted by user (frame " << frame << ")\n";
            return EXIT_SUCCESS;
        }

        device.begin_frame();
        device.draw(
            be::PrimitiveTopology::Triangles,
            *vb, *material, 3);
        device.end_frame();

        // Frame rate limiting
        auto elapsed = std::chrono::steady_clock::now() - frame_start;
        if (elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - elapsed);
        }
    }

    std::cerr << "Demo complete: triangle (" << target_frames << " frames rendered)\n";
    return EXIT_SUCCESS;
}
```

**Notes**:
- The includes for `"platform/platform.h"` and `"render/render_device.h"` are needed because the function body calls `platform.poll_events()`, `device.begin_frame()`, etc.
- The includes for `"render/primitive_topology.h"` is needed for the `be::PrimitiveTopology::Triangles` argument to `draw()`.
- `setup_triangle()` is called as `buddd::cmd::demo::setup_triangle()` (fully-qualified) since it is now in the `buddd::cmd::demo` namespace. A namespace alias like `namespace bcd = buddd::cmd::demo;` could also be used.
- Diagnostic messages use `"Demo"` prefix instead of the old `"Render test"` prefix, matching SPEC-007.
- The function receives `argc/argv` where `argv[0]` is the demo name (`"triangle"`) — this matches the spec's contract. The triangle demo currently ignores extra arguments.

### 8. Update `src/cmd/main.cpp`

Replace the current content with:

```cpp
#include "commands/demo_command.h"
#include "commands/help_command.h"
#include "commands/run_command.h"
#include "commands/version_command.h"

#include <cstdio>
#include <cstdlib>
#include <string_view>

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

    if (cmd == "demo") {
        return bc::DemoCommand{}.run(argc, argv);
    }

    if (cmd == "version") {
        return bc::VersionCommand{}.run(argc, argv);
    }

    if (cmd == "help") {
        return bc::HelpCommand{}.run(argc, argv);
    }

    // Unknown command (includes "test", "--test", "--version", etc.)
    std::fprintf(stderr, "Unknown command: '%s'\n\n",
                 argv[1]);
    std::fwrite(bc::k_usage_text.data(), 1, bc::k_usage_text.size(), stderr);
    return EXIT_FAILURE;
}
```

**Changes from current**:
- `#include "commands/test_command.h"` → `#include "commands/demo_command.h"`
- `if (cmd == "test")` branch removed entirely — `"test"` falls through to the unknown command handler.
- `if (cmd == "demo")` branch added, dispatching to `bc::DemoCommand{}`.

### 9. Update `src/cmd/commands/run_command.h`

Replace the class doc comment:

```cpp
#pragma once

namespace buddd::cmd {

class RunCommand {
public:
    /// Opens an interactive window (1024×768, title "Buddd Engine") and clears
    /// the framebuffer each frame until the user closes the window (no draw calls).
    /// Extra arguments are silently ignored.
    [[nodiscard]] auto run(int argc, const char* const* argv) -> int;
};

} // namespace buddd::cmd
```

### 10. Update `src/cmd/commands/run_command.cpp`

Remove `#include "demo_helpers.h"`, remove the `setup_triangle()` call, remove the `draw()` call, and remove `#include "render/primitive_topology.h"`:

```cpp
#include "run_command.h"

#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>

namespace be = buddd::engine;
namespace bc = buddd::cmd;

auto bc::RunCommand::run([[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv) -> int {
    auto platform = be::Platform::create(be::Backend::SDL3);
    if (!platform) {
        std::cerr << "FATAL: " << be::to_string(platform.error()) << "\n";
        return EXIT_FAILURE;
    }

    auto window = (*platform)->create_window({
        .title = "Buddd Engine",
        .width = 1024,
        .height = 768
    });
    if (!window) {
        std::cerr << "FATAL: " << be::to_string(window.error()) << "\n";
        return EXIT_FAILURE;
    }

    std::printf("Window opened: %dx%d\n", (*window)->width(), (*window)->height());

    auto device = be::RenderDevice::create(**window);
    if (!device) {
        std::cerr << "FATAL: " << be::to_string(device.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Render loop: runs until the window is closed by the user
    // Each frame clears the framebuffer (begin_frame does the clear in the
    // OpenGL backend via glClear) with no draw calls.
    while ((*platform)->poll_events()) {
        (*device)->begin_frame();
        (*device)->end_frame();
    }

    std::printf("Window closed, shutting down.\n");
    return EXIT_SUCCESS;
}
```

**Changes from current**:
- Removed `#include "demo_helpers.h"`
- Removed `#include "render/primitive_topology.h"`
- Removed `auto [material, vb] = bc::setup_triangle(**device);`
- Removed the `draw()` call inside the loop — now just `begin_frame()` / `end_frame()`

### 11. Update `src/cmd/commands/help_command.h`

Update `k_usage_text` to replace the `test` line with the `demo` line and remove `(default)` from the `run` line:

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
    "  run       Run the engine in interactive mode (empty window)\n"
    "  demo      Run a demo by name (try 'buddd demo triangle')\n"
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

**Changes from current**:
- `"  run       Run the engine in interactive mode (default)\n"` → `"  run       Run the engine in interactive mode (empty window)\n"` (removed `(default)`, added `(empty window)`)
- `"  test      Run automated render test (120 frames, then exit)\n"` → `"  demo      Run a demo by name (try 'buddd demo triangle')\n"`

### 12. Include path resolution

The existing `target_include_directories(buddd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})` sets `src/cmd/` as the include root. Therefore:

- From `src/cmd/commands/demo_command.cpp`:
  - `#include "demos/demo_helpers.h"` → resolves to `src/cmd/demos/demo_helpers.h` ✓
  - `#include "demos/triangle_demo.h"` → resolves to `src/cmd/demos/triangle_demo.h` ✓
  - `#include "platform/platform.h"` → resolves to `src/engine/platform/platform.h` (via engine's public include) ✓

- From `src/cmd/demos/triangle_demo.cpp`:
  - `#include "demos/demo_helpers.h"` → resolves to `src/cmd/demos/demo_helpers.h` ✓
  - `#include "platform/platform.h"` → resolves to `src/engine/platform/platform.h` ✓

- From `src/cmd/demos/demo_helpers.cpp`:
  - `#include "demo_helpers.h"` — this resolves first to the same directory (the moved file at `src/cmd/demos/demo_helpers.h`), which is correct ✓
  - Or equivalently: the include root `src/cmd/` plus `target_include_directories(buddd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})` means `#include "demos/demo_helpers.h"` also works.

Actually, `demo_helpers.cpp` now resides in `src/cmd/demos/` and includes `"demo_helpers.h"`. The compiler first searches relative to the including file's directory, so `"demo_helpers.h"` resolves to `src/cmd/demos/demo_helpers.h` (the sibling file). This is correct.

### 13. CONST-001 compliance

Run `grep -rnE '(SDL3|GL/|glad|glm)' src/cmd/` — zero matches. No file under `src/cmd/` includes any SDL3, OpenGL, or GLM headers.

## Required tests

### Tests to add/update in `tests/version_test.cpp`

#### Update existing help text assertions

In `TEST_CASE("buddd help outputs usage text", "[cli]")` and `TEST_CASE("buddd help ignores extra arguments", "[cli]")`, change:

```cpp
// Before:
REQUIRE(res.stdout_str.find("test") != std::string::npos);

// After:
REQUIRE(res.stdout_str.find("demo") != std::string::npos);
```

#### Add new test cases

Add the following after the existing `[cli]` tests (before the `#ifdef BUDDD_HAS_DISPLAY` block, since they don't need a display):

```cpp
TEST_CASE("buddd demo with no name prints usage and exits 1", "[cli]") {
    const auto res = run_buddd("demo");

    REQUIRE(res.exit_code == 1);
    // stderr must contain the demo usage text
    REQUIRE(res.stderr_str.find("Usage: buddd demo <demo>") != std::string::npos);
    REQUIRE(res.stderr_str.find("triangle") != std::string::npos);
    REQUIRE(res.stderr_str.find("Demo names are case-sensitive.") != std::string::npos);
}

TEST_CASE("buddd demo unknownname prints error and exits 1", "[cli]") {
    const auto res = run_buddd("demo unknownname");

    REQUIRE(res.exit_code == 1);
    // stderr must contain "Unknown demo: 'unknownname'"
    REQUIRE(res.stderr_str.find("Unknown demo: 'unknownname'") != std::string::npos);
    // Must also contain the demo usage
    REQUIRE(res.stderr_str.find("Usage: buddd demo <demo>") != std::string::npos);
}

TEST_CASE("buddd test is unknown command", "[cli]") {
    const auto res = run_buddd("test");

    REQUIRE(res.exit_code == 1);
    // stderr must contain "Unknown command: 'test'"
    REQUIRE(res.stderr_str.find("Unknown command: 'test'") != std::string::npos);
    // Must also contain the updated usage block (which has "demo" not "test")
    REQUIRE(res.stderr_str.find("Usage: buddd <command> [<args>]") != std::string::npos);
}
```

#### Add display-dependent test

Inside the `#ifdef BUDDD_HAS_DISPLAY` block (after or alongside the existing `buddd with no arguments defaults to run command` test):

```cpp
TEST_CASE("buddd demo triangle runs and completes", "[cli]") {
    // Run the demo with a 5-second timeout and verify the completion message.
    // This test requires a display (guarded by BUDDD_HAS_DISPLAY).
    const auto binary = buddd_binary_path();
    const auto out_file = temp_filename("buddd_demo_out");
    const auto err_file = temp_filename("buddd_demo_err");

    const std::string shell_cmd = "timeout 5 \"" + binary + "\" demo triangle > \""
                                  + out_file + "\" 2> \"" + err_file + "\" || true";

    const int sys_ret = std::system(shell_cmd.c_str());
    (void)sys_ret;

    auto read_file = [](const std::string& path) -> std::string {
        std::ifstream f(path, std::ios::binary);
        if (!f) return {};
        return std::string((std::istreambuf_iterator<char>(f)),
                           std::istreambuf_iterator<char>());
    };

    const auto stderr_str = read_file(err_file);
    std::remove(out_file.c_str());
    std::remove(err_file.c_str());

    // The demo should complete (120 frames ~2 seconds, well within 5s timeout)
    // If the display is available, we should see the completion message.
    // If the platform/window creation failed, we'll see an error message instead.
    // We check that either the demo completed or an engine init error occurred.
    REQUIRE( (stderr_str.find("Demo complete: triangle (120 frames rendered)") != std::string::npos
              || stderr_str.find("Failed to create") != std::string::npos) );
}
```

**Test linkage to spec acceptance criteria**:

| Test | ACs covered |
|---|---|
| `buddd demo with no name prints usage and exits 1` | AC-006 (demo no name → usage + exit 1) |
| `buddd demo unknownname prints error and exits 1` | AC-005 (unknown demo name → error + exit 1) |
| `buddd test is unknown command` | AC-016 (old `test` → unknown command error + exit 1) |
| `buddd demo triangle runs and completes` (display) | AC-007 (demo triangle completes) |
| Updated `buddd help outputs usage text` | AC-014 (help text updated with `demo`, no `test`) |
| Updated `buddd help ignores extra arguments` | AC-014, AC-021 |
| Existing `buddd unknowncommand exits with code 1` | AC-015 (unknown command + updated usage) |
| Existing `buddd version outputs correct version string` | AC-013 |
| Existing `buddd version ignores extra arguments` | AC-020 |
| Existing `buddd with no arguments defaults to run command` | AC-012 (no-args → empty window) |

## Edge cases

| Case | Expected behaviour |
|---|---|
| `buddd demo` (no name) | Prints demo usage to stderr; exits 1. |
| `buddd demo triangle` | Runs triangle demo; exits 0 on completion. |
| `buddd demo TRIANGLE` (uppercase) | Case-sensitive comparison fails; treated as unknown demo name. |
| `buddd demo triangle extra1 extra2` | Warning to stderr; demo proceeds. |
| `buddd demo ''` (empty demo name) | Empty string is not a valid demo name (`argc >= 3` but demo name is empty); treated as unknown demo. |
| `buddd test` (old subcommand) | Unknown command error; exits 1. |
| `buddd run` with no display (`BUDDD_HAS_DISPLAY=OFF`) | Platform creation fails at runtime; error printed to stderr; exits non-zero. |
| `buddd demo triangle` with no display | Same — platform creation fails; error to stderr; exits non-zero. |
| Window closed during `buddd run` before first frame | `poll_events()` returns `false` on first call; loop exits immediately; prints "Window closed, shutting down."; exits 0. |
| Window closed during `buddd demo triangle` on frame 0 | Abort message printed for frame 0; exits with code 0. |
| `buddd run extra_arg` (extra arg to run) | RunCommand silently ignores extra args. |
| `buddd` with trailing whitespace (no extra args) | Treated as no arguments; defaults to `run`. |
| `buddd RUN` (uppercase command) | Case-sensitive comparison fails; treated as unknown command. |
| `buddd --test` / `buddd --version` | Unknown command error; exits 1. |
| `buddd --help` | `"--help"` is not a known command; unknown command error printed (includes usage). |
| `buddd --` (double dash) | `argv[1]` is `"--"`; treated as unknown command. |
| `buddd help --verbose` (extra flag-like arg) | HelpCommand ignores extra args; help printed. |
| `buddd version --format json` (extra arg) | VersionCommand ignores extra args; version printed. |
| `buddd demo` with `argc == 2` (just "demo", no extra args) | Covered by `argc < 3` check; prints usage; exits 1. |
| `buddd demo triangle ` (trailing whitespace) | Shell normalises; treated as no extra args. |

## Security impact

- No elevated privileges required.
- No network access.
- No secrets, credentials, or environment variables consumed.
- No new attack surface: the refactor moves code between files and renames commands but does not introduce new I/O, parsing, or execution paths.
- CONST-001 is preserved: no SDL3/OpenGL/GLM headers in `src/cmd/`. The existing AMEND-2026-001 (SDL3 test files) is unaffected.

## Data and migration impact

None.

## API compatibility impact

- The engine library (`buddd_engine`) public API is unchanged.
- The `buddd` binary's CLI interface changes:
  - **Breaking**: `buddd test` no longer works (replaced by `buddd demo triangle`).
  - **Breaking**: `buddd test` now produces an unknown command error.
  - **New**: `buddd demo <name>` subcommand with named demo dispatch.
  - **Changed**: `buddd run` no longer renders a triangle (empty window only).
  - **Backward-compatible**: `buddd` with no arguments still opens the interactive window (now without triangle).
  - **Backward-compatible**: `buddd version` unchanged.
  - **Backward-compatible**: `buddd help` unchanged except for updated text.
- The command classes (`buddd::cmd::DemoCommand`) are new public API.
- `buddd::cmd::TestCommand` is removed.

## Documentation impact

- `docs/specs/cli-command-evolution/spec.md` already documents the new CLI interface.
- No wiki or external docs need updating as part of this contract.

## ADR impact

No new ADR is required. Decisions in SPEC-007 were resolved in the spec:
- Demo dispatch via if/else chain in `DemoCommand::run()`.
- Pass `argc - 2, argv + 2` to per-demo functions (B-03 resolution).
- Extra-arguments warning printed by `DemoCommand` before dispatch (Q-02 resolution).
- `begin_frame()` clears the colour buffer to black as an engine implementation detail (W-01 resolution).
- Demo usage text includes case-sensitivity note (W-05 resolution).

## Constitution impact

No constitution changes are required. CONST-001 is preserved (no SDL3/OpenGL/GLM headers in `src/cmd/`). AMEND-2026-001 (SDL3 test exception) is unaffected. CONST-002 is satisfied by the required tests.

## Done criteria

The contract is done when ALL of the following are satisfied:

1. [ ] **AC-001 (files removed)**: `src/cmd/commands/test_command.h` and `src/cmd/commands/test_command.cpp` no longer exist. Verified by `ls src/cmd/commands/test_command.*` returning no such file.

2. [ ] **AC-002 (demo helpers moved)**: `src/cmd/demos/demo_helpers.h` and `src/cmd/demos/demo_helpers.cpp` exist. The `setup_triangle()` function is present and its signature matches the original (same as SPEC-006). Verified by inspection and build.

3. [ ] **AC-003 (triangle demo files created)**: `src/cmd/demos/triangle_demo.h` and `src/cmd/demos/triangle_demo.cpp` exist, declaring `run_triangle_demo(be::Platform&, be::RenderDevice&, int, const char* const*) -> int`. Verified by inspection and build.

4. [ ] **AC-004 (triangle render behaviour preserved)**: `run_triangle_demo` performs a 120-frame render loop with a coloured triangle, using `setup_triangle()` from `demo_helpers.h` (not redefining it). Same vertex data, same shaders, same triangle appearance as the old `buddd test`. Verified by visual inspection.

5. [ ] **AC-005 (DemoCommand dispatch)**: `DemoCommand::run()` dispatches `"triangle"` to `run_triangle_demo()`. Unknown demo names print `"Unknown demo: '<name>'"` to stderr and exit with code 1. Verified by running `buddd demo unknownname`.

6. [ ] **AC-006 (demo no name)**: Running `buddd demo` with no name prints demo usage text (`"Usage: buddd demo <demo>"` + available demos + case-sensitivity note) to stderr and exits with code 1. Verified by shell.

7. [ ] **AC-007 (demo triangle completes)**: Running `buddd demo triangle` opens an 800×600 window titled "Buddd Engine — Demo: triangle", renders for 120 frames, prints `"Demo complete: triangle (120 frames rendered)"` to stderr, and exits with code 0. Manual visual verification; stderr checked.

8. [ ] **AC-008 (demo triangle early abort)**: Running `buddd demo triangle` and closing the window before 120 frames prints `"Demo aborted by user (frame N)"` to stderr and exits with code 0. Manual verification.

9. [ ] **AC-009 (demo triangle extra args)**: Running `buddd demo triangle extra_arg` prints `"Warning: unexpected arguments after 'demo triangle': extra_arg"` to stderr but still runs the demo and exits 0. Verified via shell with timeout.

10. [ ] **AC-010 (RunCommand no triangle)**: `run_command.cpp` no longer includes `demo_helpers.h` and no longer calls `setup_triangle()`. It opens a 1024×768 window titled "Buddd Engine", clears the framebuffer each frame, and draws nothing. Verified by inspection and visual check (window shows black, no triangle).

11. [ ] **AC-011 (RunCommand stdout)**: Running `buddd run` prints `"Window opened: 1024x768"` to stdout and `"Window closed, shutting down."` on exit. Verified via shell with timeout.

12. [ ] **AC-012 (no-args = run)**: Running `buddd` with no arguments produces identical behaviour to `buddd run` (empty window, no triangle). Same stdout messages as AC-011. Verified via shell with timeout.

13. [ ] **AC-013 (version output)**: Running `buddd version` prints `"buddd 0.1.0"` to stdout and exits with code 0. Verified via shell.

14. [ ] **AC-014 (help output)**: Running `buddd help` prints the updated usage message (with `demo` replacing `test`) to stdout and exits with code 0. Verified via shell. stdout contains `"demo"` and does NOT contain `"test"` as a command name.

15. [ ] **AC-015 (unknown command)**: Running `buddd unknowncommand` prints `"Unknown command: 'unknowncommand'"` followed by the updated usage block (with `demo`) to stderr and exits with code 1. Verified via shell.

16. [ ] **AC-016 (`buddd test` is unknown)**: Running `buddd test` prints `"Unknown command: 'test'"` followed by the updated usage block to stderr and exits with code 1. Verified via shell.

17. [ ] **AC-017 (old flags rejected)**: Running `buddd --test` or `buddd --version` prints the unknown command error to stderr and exits with code 1. Verified via shell.

18. [ ] **AC-018 (CONST-001 compliance)**: `grep -rnE '(SDL3|GL/|glad|glm)' src/cmd/` returns zero matches.

19. [ ] **AC-019 (CMake glob + build)**: `src/cmd/CMakeLists.txt` includes `demos/*.cpp` in the glob pattern. `cmake --build --preset debug` succeeds and produces `build/debug/src/cmd/buddd`.

20. [ ] **AC-020 (version extra args)**: Running `buddd version extra_arg` still prints the version and exits 0. Verified via shell.

21. [ ] **AC-021 (help extra args)**: Running `buddd help extra_arg` still prints the updated usage message and exits 0. Verified via shell.

22. [ ] **AC-022 (main.cpp no test references)**: `main.cpp` no longer references `TestCommand` or the `"test"` command string. It references `DemoCommand` and `"demo"` instead. Verified by inspection — no `test_command.h` include, no `"test"` string in the dispatch.

23. [ ] **AC-023 (k_usage_text updated)**: The `k_usage_text` constant in `help_command.h` replaces the `"test"` line with `"demo"` line. The unknown-command handler in `main.cpp` uses this same constant (same as before, no code change needed). Verified by inspection.

24. [ ] **AC-024 (triangle_demo uses demo_helpers)**: `triangle_demo.cpp` includes `"demos/demo_helpers.h"` (or equivalent) to access `setup_triangle()` and does not redefine it. Verified by inspection.

25. [ ] **New test: `buddd demo` no name**: `tests/version_test.cpp` has a test case that verifies `buddd demo` prints usage to stderr and exits 1. Test passes.

26. [ ] **New test: `buddd demo unknownname`**: `tests/version_test.cpp` has a test case that verifies `buddd demo unknownname` prints `"Unknown demo: 'unknownname'"` to stderr and exits 1. Test passes.

27. [ ] **New test: `buddd test` is unknown**: `tests/version_test.cpp` has a test case that verifies `buddd test` prints `"Unknown command: 'test'"` to stderr and exits 1. Test passes.

28. [ ] **Display-guarded test: `buddd demo triangle`**: `tests/version_test.cpp` has a test case guarded by `BUDDD_HAS_DISPLAY` that runs `buddd demo triangle` with a timeout. Test passes when a display is available.

29. [ ] **Help text assertions updated**: The existing `[cli]` tests for `buddd help` now check for `"demo"` instead of `"test"` in stdout. Tests pass.

30. [ ] **SC-001 (new demo addable)**: Create a skeleton `spin_demo.h/.cpp` in `src/cmd/demos/` with a matching `else if` branch in `DemoCommand::run()`. Build succeeds without modifying any other file. Revert the skeleton after verification.

31. [ ] **SC-002 (dispatch visible)**: The command dispatch if/else-if chain is contained within the first 30 lines of `main()` in `main.cpp`. Verified by inspection.

32. [ ] **SC-003 (empty vs triangle render)**: `buddd run` produces no rendering output (empty cleared window) while `buddd demo triangle` produces the coloured triangle. Manual visual comparison confirms the split.
