# SPEC-007 — CLI Command Evolution: Demo System & Empty Run

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

## Problem

SPEC-006 introduced a Command pattern with four subcommands (`run`, `test`, `version`, `help`). Experience with the current implementation has revealed several gaps:

1. **The `test` subcommand conflates two concerns**: running an automated render verification and running a named demo. Future demos (e.g., texture demo, animation demo) would each require a new top-level subcommand, cluttering the command namespace.
2. **No extensible demo system**: Adding a new visual demonstration requires modifying the command dispatch table and adding a new command class. There is no convention for per-demo files.
3. **`RunCommand` duplicates the triangle render setup**: The interactive `run` mode currently renders a coloured triangle, which is the same code as the `test` mode. This means the "empty project" case is never tested. `run` should prepare an empty canvas for project-based content, not duplicate demo logic.
4. **Naming inconsistency**: "Test" is a developer-internal term (CI verification), not a user-facing concept. "Demo" better communicates the purpose of named visual demonstrations.

SPEC-007 addresses these gaps by introducing a `demo` subcommand with a per-file demo system, renaming `TestCommand` to `DemoCommand`, and simplifying `RunCommand` to an empty window.

## Goals

- **G-01**: Rename `buddd test` → `buddd demo <name>` with a dispatch table inside `DemoCommand`.
- **G-02**: Create a `src/cmd/demo/` directory where each demo lives in its own `.h`/`.cpp` pair.
- **G-03**: Extract the triangle 120-frame loop from `demo_helpers` into `src/cmd/demo/triangle_demo.h/.cpp` as a self-contained function.
- **G-04**: Simplify `RunCommand` to open an empty window (1024×768) that clears the framebuffer each frame, with no triangle setup.
- **G-05**: Update all help text, error messages, and CLI output from "test" to "demo" language.
- **G-06**: Make `buddd test` an unknown command (old subcommand removed with no alias).
- **G-07**: Ensure the architecture boundary (CONST-001) is preserved: no SDL3, OpenGL, or GLM headers from `src/cmd/`.

## Supersedes

This spec supersedes the following parts of SPEC-006 (CLI Command System):

| SPEC-006 element | Superseded by |
|---|---|
| **TestCommand** (class, files, behavior) | Replaced by `DemoCommand`. The `test` subcommand is removed; use `buddd demo <name>`. SPEC-006 AC-003, AC-008, AC-009, AC-018 are superseded. |
| **RunCommand triangle rendering** | RunCommand no longer renders a triangle or calls `setup_triangle()`. It opens an empty window with a framebuffer clear loop. SPEC-006 AC-006, AC-007 (triangle rendering verifications) are superseded. SPEC-006 SC-003 (render behavior parity across commands) is superseded — RunCommand and DemoCommand now have divergent render behavior. |
| **Help text (test reference)** | The `help` command output and the unknown-command usage block replace the `test` line with a `demo` line. SPEC-006 AC-011 is superseded. |
| **Unknown command text (test reference)** | The usage block embedded in the unknown-command error message is updated to replace `test` with `demo`. |
| **File structure** | `test_command.h/.cpp` → `demo_command.h/.cpp`. `demo_helpers.h/.cpp` moves to `src/cmd/demo/`. New file `triangle_demo.h/.cpp` added. SPEC-006 AC-001, AC-003 (file structure requirements) are superseded. |
| **Observability signals** | Test-mode signals are replaced by demo-mode signals. |

SPEC-007 also inherits SPEC-006's supersession of SPEC-001 (Project Setup) regarding CLI behavior: SPEC-006 already superseded SPEC-001's `--version` flag behavior and no-args greeting message, and SPEC-007 carries that forward unchanged.

The following SPEC-006 elements remain in effect and are **not** superseded:
- VersionCommand and its behavior (AC-002, AC-010, AC-016).
- HelpCommand structure (AC-005, AC-017) — only the text content changes.
- Unknown-command dispatch rule (AC-012, AC-013).
- No-args default to `run` (AC-006 default behavior, minus the triangle).
- `RunCommand` window dimensions and lifecycle messages (part of AC-006, AC-007).
- The `setup_triangle()` helper function signature remains in `demo_helpers` (though the function is now only used by demos).
- CMakeLists.txt glob pattern (AC-015), extended to include `demo/*.cpp`.

## Non-goals

- No changes to the engine library (`src/engine/`), its public API, or its file structure.
- No changes to the render pipeline, draw calls, shader compilation, or rendering behavior beyond removing the triangle from RunCommand.
- No project/module system — `RunCommand` prepares for it (empty window) but does not implement project loading.
- No changes to VersionCommand or its behavior.
- No dynamic demo discovery, plugin system, or reflection.
- No CLI framework or third-party argument parsing library (C++26 standard library only).
- No subcommand aliases (e.g., `buddd d` for `buddd demo`).
- No tab-completion or shell integration.
- No internationalisation or localised text.
- No changes to the test infrastructure directory structure (`tests/`).

## Actors

| Actor | Description |
|---|---|
| Developer | A human running the `buddd` CLI binary from a terminal. Uses subcommands to invoke different engine modes. |
| Demo author | A developer who adds new visual demonstrations to the engine. Creates `.h`/`.cpp` files in `src/cmd/demo/` and adds a dispatch entry in `DemoCommand`. |
| CLI maintainer | A developer who maintains the binary entry point, command dispatch, and build system integration. |

## User-visible behavior

### Command dispatch rules

1. The first positional argument after `buddd` selects the command.
2. If no positional argument is given (`argc == 1`), the default command is `run`.
3. Each command receives the full `argc`/`argv` and is responsible for its own argument parsing beyond the command name.
4. Commands are case-sensitive lowercase.
5. The old `--test` and `--version` flags are **not** supported. Passing them as the first positional argument produces an "unknown command" error.
6. The old `test` subcommand is **removed**. Passing `test` as the first positional argument produces an "unknown command" error.

### Command behaviors

#### `buddd version` (VersionCommand)

Unchanged from SPEC-006. Prints the version string to stdout and exits with code 0.

**Output format (exact)**:
```
buddd 0.1.0
```

**Extra arguments**: silently ignored.

#### `buddd demo <name>` (DemoCommand)

Opens an SDL3 window (800×600, title `"Buddd Engine — Demo: <name>"`), runs the named demo, and exits.

**Behavior details**:
- Platform created with `be::Platform::create(be::Backend::SDL3)`.
- Window dimensions: 800×600.
- Window title: `"Buddd Engine — Demo: <name>"` where `<name>` is the demo name as typed (preserving case).
- The demo name is `argv[2]`. If `argc < 3` (no demo name provided), prints usage to stderr and exits with code 1 (see below).
- Dispatches to the matching demo function via a switch/case on the demo name.
- Render loop runs exactly the number of frames defined by the demo (e.g., 120 frames for `triangle`).
- Between frames, sleeps to maintain ~60 FPS (16 ms per frame).
- If the user closes the window during the demo, the loop is aborted early, a message is printed to stderr, and the command exits with code 0 (this is not an error).
- **Extra arguments after the demo name**: if `argc > 3` and the demo name matches a known demo, prints a warning to stderr but proceeds with the demo.

##### `buddd demo triangle`

Replicates the current `buddd test` behavior with updated messaging.

**Output strings (exact)**:
- At start, stderr: `"Demo started: triangle (120 frames)\n"`
- On successful completion, stderr: `"Demo complete: triangle (120 frames rendered)\n"`
- On early abort, stderr: `"Demo aborted by user (frame N)\n"` where N is the current frame number.
- On extra arguments (e.g., `buddd demo triangle extra1 extra2`), stderr: `"Warning: unexpected arguments after 'demo triangle': extra1 extra2\n"`

##### `buddd demo` with no name

Prints the following to **stderr** and exits with code 1:

```
Usage: buddd demo <demo>

Available demos:
  triangle     Run the triangle demo (120 frames)

Demo names are case-sensitive.
```

##### Unknown demo name

If the demo name does not match any known demo, prints the following to **stderr** and exits with code 1:

```
Unknown demo: '<name>'

Usage: buddd demo <demo>

Available demos:
  triangle     Run the triangle demo (120 frames)

Demo names are case-sensitive.
```

#### `buddd run` (RunCommand)

Opens an SDL3 window (1024×768, title `"Buddd Engine"`), creates a render device, and runs a render loop that clears the framebuffer each frame but draws nothing. Continues until the user closes the window.

**Behavior details**:
- Platform created with `be::Platform::create(be::Backend::SDL3)`.
- Window dimensions: 1024×768.
- Window title: `"Buddd Engine"`.
- Prints `"Window opened: 1024x768"` to stdout.
- Render loop runs until `(*platform)->poll_events()` returns `false` (window close).
- Each frame: `begin_frame()` clears the colour buffer to black (as an implementation detail of the OpenGL backend, which calls `glClear(GL_COLOR_BUFFER_BIT)` with the default clear colour). No draw calls, no triangle setup, no `setup_triangle()` call, no include of `demo_helpers.h`.
- Prints `"Window closed, shutting down."` to stdout on exit.
- **Extra arguments**: silently ignored (reserved for future project-path argument).

#### `buddd help` (HelpCommand)

Prints usage information to stdout and exits with code 0.

**Output format (exact)**:
```
Usage: buddd <command> [<args>]

Commands:
  run       Run the engine in interactive mode (empty window)
  demo      Run a demo by name (try 'buddd demo triangle')
  version   Print version information
  help      Show this help message
```

**Extra arguments**: silently ignored.

#### Unknown command

If the first positional argument does not match any known command (`run`, `demo`, `version`, `help`), the CLI prints an error message to **stderr** and exits with code 1.

**Output format (exact)**:
```
Unknown command: '<cmd>'

Usage: buddd <command> [<args>]

Commands:
  run       Run the engine in interactive mode (empty window)
  demo      Run a demo by name (try 'buddd demo triangle')
  version   Print version information
  help      Show this help message
```

Where `<cmd>` is the unrecognised argument. The usage block is identical to the `help` command output.

**Note**: `buddd test`, `buddd --test`, and `buddd --version` all produce this unknown-command error. The old `test` subcommand is removed with no alias or deprecation path.

## Key entities

### DemoCommand

- **File**: `src/cmd/commands/demo_command.h` / `src/cmd/commands/demo_command.cpp`.
- **Namespace**: `buddd::cmd`.
- **Public method**: `auto run(int argc, const char* const* argv) -> int;`
- **Behavior**:
  - Parses `argv[2]` as the demo name.
  - If `argc < 3`, prints usage to stderr and returns `EXIT_FAILURE`.
  - Uses a switch/case (or equivalent if/else chain) on the demo name to dispatch to the per-demo function.
  - If the demo name is unknown, prints `"Unknown demo: '<name>'"` followed by usage to stderr and returns `EXIT_FAILURE`.
  - Creates platform, window, and render device before dispatching.
  - Passes `(**platform)` and `(**device)` to the selected demo function.
  - If `argc > 3` and the demo name matches, prints a warning to stderr before spawning the demo.

### Per-demo files

Each demo is a pair of `.h`/`.cpp` files in `src/cmd/demo/`. Each file exposes a single free function in the `buddd::cmd::demo` namespace:

```cpp
namespace buddd::cmd::demo {

/// Runs the triangle demo: 120-frame render loop with a coloured triangle.
/// @param platform  The engine platform (for event polling).
/// @param device    The render device (for rendering).
/// @param argc      Argument count (argv[0] is the demo name, argv[1..] are extra args).
/// @param argv      Argument vector (argv[0] is the demo name).
/// @return          0 on success, non-zero on error.
auto run_triangle_demo(buddd::engine::Platform& platform,
                       buddd::engine::RenderDevice& device,
                       int argc, const char* const* argv) -> int;

} // namespace buddd::cmd::demo
```

**Note**: `DemoCommand` passes `argc - 2` and `argv + 2` to the per-demo function, so the demo function receives `argv[0]` as the demo name (matching the documented contract). The extra-arguments warning in `DemoCommand` iterates from `argv[3]` (original `argv` indices).

The demo function is responsible for:
- Running its own render loop (with per-frame `poll_events()`, `begin_frame()`, rendering, `end_frame()`).
- Maintaining its own frame count.
- Printing its own diagnostic messages to stderr.
- Printing its own extra-argument warning if applicable (though the general warning is printed by DemoCommand before dispatch).

### demo_helpers (moved)

- **Files**: `src/cmd/demo/demo_helpers.h` / `src/cmd/demo/demo_helpers.cpp` (moved from `src/cmd/`).
- **Namespace**: Moved into `buddd::cmd::demo` along with other demo code.
- **Content**: The `setup_triangle()` function unchanged. This function is used by triangle_demo and potentially by future demos.
- **Note**: The `#include` path in files that reference `demo_helpers.h` must be updated to `"demo/demo_helpers.h"` (since the include root is `src/cmd/`).

### File structure

| File | Content |
|---|---|
| `src/cmd/main.cpp` | Dispatch: `"test"` → unknown command; `"demo"` → DemoCommand instead of TestCommand. |
| `src/cmd/CMakeLists.txt` | Updated glob pattern to include `demo/*.cpp`. |
| `src/cmd/commands/run_command.h` | Declaration of `buddd::cmd::RunCommand` — no changes to interface, behavior changes in .cpp only. |
| `src/cmd/commands/run_command.cpp` | Empty render loop: no `#include "demo_helpers.h"`, no triangle setup, framebuffer clear only. |
| `src/cmd/commands/demo_command.h` | **NEW** (replaces `test_command.h`): Declaration of `buddd::cmd::DemoCommand`. |
| `src/cmd/commands/demo_command.cpp` | **NEW** (replaces `test_command.cpp`): Parses demo name, creates platform/window/device, dispatches to per-demo function. |
| `src/cmd/commands/version_command.h` | Unchanged. |
| `src/cmd/commands/version_command.cpp` | Unchanged. |
| `src/cmd/commands/help_command.h` | `k_usage_text` constant updated. |
| `src/cmd/commands/help_command.cpp` | Unchanged (reads from `k_usage_text`). |
| `src/cmd/demo/demo_helpers.h` | **MOVED** from `src/cmd/demo_helpers.h`. Contents unchanged. |
| `src/cmd/demo/demo_helpers.cpp` | **MOVED** from `src/cmd/demo_helpers.cpp`. Contents unchanged. |
| `src/cmd/demo/triangle_demo.h` | **NEW**: Declaration of `run_triangle_demo()`. |
| `src/cmd/demo/triangle_demo.cpp` | **NEW**: 120-frame triangle render loop (extracted from `test_command.cpp`). |

### File changes summary

- **Remove**: `src/cmd/commands/test_command.h`, `src/cmd/commands/test_command.cpp`.
- **Remove**: `src/cmd/demo_helpers.h`, `src/cmd/demo_helpers.cpp` (moved, not deleted).
- **Create**: `src/cmd/commands/demo_command.h`, `src/cmd/commands/demo_command.cpp`.
- **Create**: `src/cmd/demo/triangle_demo.h`, `src/cmd/demo/triangle_demo.cpp`.
- **Move**: `src/cmd/demo_helpers.h` → `src/cmd/demo/demo_helpers.h`, `src/cmd/demo_helpers.cpp` → `src/cmd/demo/demo_helpers.cpp`.
- **Modify**: `src/cmd/main.cpp`, `src/cmd/CMakeLists.txt`, `src/cmd/commands/run_command.cpp`, `src/cmd/commands/help_command.h`.
- **Unchanged**: `src/cmd/commands/version_command.h`, `src/cmd/commands/version_command.cpp`, `src/cmd/commands/help_command.cpp`.

## User stories

### Story 1 — Run demo with name (Priority: P1)

As a developer, I want to run `buddd demo triangle` and see the triangle rendering for 120 frames, so that I can verify the render pipeline works without manual interaction.

**Given** the `buddd` binary is compiled with display support
**When** I run `buddd demo triangle`
**Then** a window opens (800×600, title "Buddd Engine — Demo: triangle"), renders for 120 frames, prints "Demo complete: triangle (120 frames rendered)" to stderr, and exits with code 0.

### Story 2 — Run demo with no name (Priority: P1)

As a developer, I want to see a usage message when I run `buddd demo` without a name, so that I can discover available demos.

**Given** the `buddd` binary is compiled
**When** I run `buddd demo`
**Then** stderr contains the demo usage text listing available demos, and the process exits with code 1.

### Story 3 — Run empty interactive window (Priority: P1)

As a developer, I want to run `buddd run` and see an empty window, so that I can verify the engine boots without rendering a specific scene.

**Given** the `buddd` binary is compiled with display support
**When** I run `buddd run`
**Then** a window opens (1024×768, title "Buddd Engine") with a cleared framebuffer, shows nothing, and closes cleanly when I close the window, printing "Window closed, shutting down." to stdout.

### Story 4 — Run with no arguments defaults to run (Priority: P1)

As a developer, I want `buddd` with no arguments to open the interactive window, so that the engine remains immediately launchable.

**Given** the `buddd` binary is compiled with display support
**When** I run `buddd` with no arguments
**Then** a window opens (1024×768, title "Buddd Engine") with the same empty-window behavior as `buddd run`.

### Story 5 — Print version (Priority: P1)

As a developer, I want to run `buddd version` and see the current version string, so that I can confirm which build I am running.

**Given** the `buddd` binary is compiled
**When** I run `buddd version`
**Then** stdout contains `"buddd 0.1.0"` and the process exits with code 0.

### Story 6 — Show help (Priority: P1)

As a new developer, I want to run `buddd help` and see a list of available commands, so that I can discover how to use the CLI without reading documentation.

**Given** the `buddd` binary is compiled
**When** I run `buddd help`
**Then** stdout contains the updated usage message listing `run`, `demo`, `version`, and `help`, and the process exits with code 0.

### Story 7 — Unknown demo name (Priority: P2)

As a developer, I want to see a clear error when I run `buddd demo unknownname`, so that I know what demos exist.

**Given** the `buddd` binary is compiled
**When** I run `buddd demo unknownname`
**Then** stderr contains `"Unknown demo: 'unknownname'"` followed by the demo usage block, and the process exits with code 1.

### Story 8 — Old `test` command rejected (Priority: P2)

As a developer who used `buddd test` previously, I want the CLI to tell me it's an unknown command, so that I learn the new `buddd demo triangle` syntax.

**Given** the `buddd` binary is compiled
**When** I run `buddd test`
**Then** stderr contains `"Unknown command: 'test'"` followed by the usage block, and the process exits with code 1.

### Story 9 — Old flags rejected (Priority: P2)

As a developer who remembers old flags, I want the CLI to reject `--test` and `--version` as unknown commands, so that I learn the subcommand syntax (same behavior as SPEC-006).

**Given** the `buddd` binary is compiled
**When** I run `buddd --test` or `buddd --version`
**Then** stderr contains `"Unknown command: '--test'"` (or `"--version"`) followed by the usage block, and the process exits with code 1.

### Story 10 — Demo with extra arguments (Priority: P3)

As a developer, I want extra arguments after the demo name to produce a warning but still run, so that I can pass future per-demo options without breaking current usage.

**Given** the `buddd` binary is compiled with display support
**When** I run `buddd demo triangle extra1 extra2`
**Then** stderr contains a warning about unexpected arguments, then the triangle demo runs normally and completes.

### Story 11 — Abort demo early (Priority: P3)

As a developer, I want to close the demo window early and see an abort message, so that I can cancel a running demo cleanly.

**Given** the `buddd` binary is compiled with display support
**When** I run `buddd demo triangle` and close the window before 120 frames
**Then** stderr contains `"Demo aborted by user (frame N)"` and the process exits with code 0.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `src/cmd/commands/test_command.h` and `src/cmd/commands/test_command.cpp` no longer exist. They are replaced by `src/cmd/commands/demo_command.h` and `src/cmd/commands/demo_command.cpp`. | Check that old files are removed and new files exist. |
| AC-002 | `src/cmd/demo/demo_helpers.h` and `src/cmd/demo/demo_helpers.cpp` exist (moved from `src/cmd/`). The `setup_triangle()` function is present and unchanged. | Files exist and compile. Function signature matches SPEC-006 `demo_helpers.h`. |
| AC-003 | `src/cmd/demo/triangle_demo.h` and `src/cmd/demo/triangle_demo.cpp` exist, declaring `run_triangle_demo(be::Platform&, be::RenderDevice&) -> int`. | Files exist and compile without error. |
| AC-004 | `run_triangle_demo` performs a 120-frame render loop with a coloured triangle, mirroring the current `buddd test` render behavior (same vertex data, same shaders, same triangle). | Visual inspection: the triangle appears identical to the current `buddd test` rendering. |
| AC-005 | `DemoCommand::run()` dispatches `"triangle"` to `run_triangle_demo()`. Unknown demo names print `"Unknown demo: '<name>'"` to stderr and exit with code 1. | Run `buddd demo unknownname`; stderr contains the error. Exit code is 1. |
| AC-006 | Running `buddd demo` with no name prints the demo usage text to stderr and exits with code 1. | Run `buddd demo`; stderr matches the exact usage output. Exit code is 1. |
| AC-007 | Running `buddd demo triangle` opens an 800×600 window titled "Buddd Engine — Demo: triangle" (case-preserving), renders for 120 frames, prints `"Demo complete: triangle (120 frames rendered)"` to stderr, and exits with code 0. | Manual visual verification; stderr contains the completion message. |
| AC-008 | Running `buddd demo triangle` and closing the window before 120 frames prints `"Demo aborted by user (frame N)"` to stderr and exits with code 0. | Manual verification: close window early, check stderr, check exit code. |
| AC-009 | Running `buddd demo triangle extra_arg` prints a warning to stderr (`"Warning: unexpected arguments after 'demo triangle': extra_arg"`) but still runs the demo and exits 0. | Run the command; stderr contains the warning and the demo completion message. |
| AC-010 | `RunCommand` no longer includes `demo_helpers.h` and no longer calls `setup_triangle()`. It opens a 1024×768 window titled "Buddd Engine", clears the framebuffer to black each frame, and draws nothing. | Inspect `run_command.cpp` — no `#include "demo_helpers.h"` (or `"demo/demo_helpers.h"`), no call to `setup_triangle()`. Visual: window background shows black, no triangle. |
| AC-011 | Running `buddd run` opens a 1024×768 window, prints `"Window opened: 1024x768"` to stdout, and prints `"Window closed, shutting down."` on exit. | Run `buddd run` with timeout, verify stdout contains both messages. |
| AC-012 | Running `buddd` with no arguments produces identical behavior to `buddd run` (empty window, no triangle). | Run with no args, verify same stdout messages as AC-011. |
| AC-013 | Running `buddd version` prints `"buddd 0.1.0"` to stdout and exits with code 0. | Run `buddd version`; stdout matches exactly. |
| AC-014 | Running `buddd help` prints the updated usage message (with `demo` replacing `test`) to stdout and exits with code 0. | Run `buddd help`; stdout contains `"demo"` and does not contain `"test"` as a command name. |
| AC-015 | Running `buddd unknowncommand` prints `"Unknown command: 'unknowncommand'"` followed by the updated usage block to stderr and exits with code 1. | Run `buddd unknowncommand`; stderr matches. Exit code is 1. |
| AC-016 | Running `buddd test` prints `"Unknown command: 'test'"` followed by the updated usage block to stderr and exits with code 1. | Run `buddd test`; stderr matches. Exit code is 1. |
| AC-017 | Running `buddd --test` or `buddd --version` prints the unknown command error to stderr and exits with code 1. | Run both; stderr contains `"Unknown command: '--test'"` / `"Unknown command: '--version'"`. Exit code is 1. |
| AC-018 | No SDL3, OpenGL, or GLM headers are included from any file under `src/cmd/`. | Run `grep -rnE '(SDL3|GL/|glad|glm)' src/cmd/` — zero matches. Architecture boundary (CONST-001) is preserved. |
| AC-019 | The `src/cmd/CMakeLists.txt` glob includes `demo/*.cpp` and the build succeeds. | `cmake --build --preset debug` succeeds; `build/debug/src/cmd/buddd` is produced. |
| AC-020 | Running `buddd version extra_arg` still prints the version and exits 0 (extra args ignored). | Run `buddd version extra_arg`; output matches AC-013. |
| AC-021 | Running `buddd help extra_arg` still prints the updated usage message and exits 0 (extra args ignored). | Run `buddd help extra_arg`; output matches AC-014. |
| AC-022 | `main.cpp` no longer references `TestCommand` or the `"test"` command string. It references `DemoCommand` and `"demo"` instead. | Inspect `main.cpp` — no mention of `test_command.h` or `"test"` as a dispatch target. |
| AC-023 | The `k_usage_text` constant in `help_command.h` is updated to replace `"test"` with `"demo"`. The unknown-command handler in `main.cpp` uses this same constant. | Inspect `help_command.h` — the text matches the updated format. |
| AC-024 | `triangle_demo.cpp` does not duplicate `demo_helpers.cpp` content. It uses `#include "demo_helpers.h"` (or `"demo/demo_helpers.h"`) to access `setup_triangle()`. | Inspect `triangle_demo.cpp` — it includes the shared helper, does not redefine `setup_triangle()`. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A new demo can be added by creating one `.h`/`.cpp` pair in `src/cmd/demo/` and adding one `else if` branch in `DemoCommand::run()`, without modifying any other file. CMakeLists.txt picks up the new file automatically via glob. | Create a skeleton `spin_demo.h/.cpp` in `src/cmd/demo/` with a matching dispatch entry. Build succeeds. |
| SC-002 | The command dispatch in `main.cpp` is a single if/else-if chain (or equivalent lookup) mapping command-name strings to command invocations, contained within the first 30 lines of `main()`. | Inspect `main.cpp` — the dispatch is the top-level structure, fully visible in the first 30 lines of `main()`. |
| SC-003 | `buddd run` produces no rendering output (empty window) while `buddd demo triangle` produces the same triangle rendering as the old `buddd test`. | Manual visual comparison: `buddd run` shows an empty cleared window; `buddd demo triangle` shows the coloured triangle. |

## Edge cases

| Case | Expected behavior |
|---|---|
| `buddd demo` (no name) | Prints demo usage to stderr; exits 1. |
| `buddd demo triangle` | Runs triangle demo; exits 0 on completion. |
| `buddd demo TRIANGLE` (uppercase) | Case-sensitive comparison fails; treated as unknown demo name. |
| `buddd demo triangle extra1 extra2` | Warning to stderr; demo proceeds. |
| `buddd demo triangle ` (trailing whitespace) | Shell normalises; treated as no extra args. |
| `buddd demo ''` (empty demo name) | Empty string is not a valid demo name; treated as unknown demo. |
| `buddd test` (old subcommand) | Unknown command error; exits 1. |
| `buddd run` with no display (`BUDDD_HAS_DISPLAY=OFF`) | Platform creation fails at runtime; error printed to stderr; exits non-zero. |
| `buddd demo triangle` with no display | Same — platform creation fails; error to stderr; exits non-zero. |
| Window closed during `buddd run` before first frame renders | `poll_events()` returns `false` on first call; loop exits immediately; prints "Window closed, shutting down."; exits 0. |
| Window closed during `buddd demo triangle` on frame 0 | Abort message printed for frame 0; exits with code 0. |
| `buddd run extra_arg` (extra arg to run) | RunCommand silently ignores extra args (reserved for future project path). |
| `buddd` with trailing whitespace (no extra args) | Treated as no arguments; defaults to `run`. |
| `buddd   ` (multiple spaces) | Shell normalises; same as no extra args. |
| `buddd RUN` (uppercase command) | Case-sensitive comparison fails; treated as unknown command. |
| `buddd --help` | `"--help"` is not a known command; unknown command error printed (which includes the usage block, so the user still sees available commands). |
| `buddd --` (double dash) | `argv[1]` is `"--"`; treated as unknown command. |
| `buddd help --verbose` (extra flag-like arg) | HelpCommand ignores extra args; help printed. |
| `buddd version --format json` (extra arg) | VersionCommand ignores extra args; version printed. |

## Error cases

| Case | Expected behavior |
|---|---|
| Unknown command (including `test`, `--test`, `--version`) | Print to stderr: `"Unknown command: '<cmd>'"` followed by the updated usage block. Exit code 1. |
| Unknown demo name | Print to stderr: `"Unknown demo: '<name>'"` followed by the demo usage block. Exit code 1. |
| `buddd demo` with no name | Print demo usage to stderr. Exit code 1. |
| Engine initialisation failure (e.g., SDL3 not available) | Command's `run()` returns `EXIT_FAILURE`. Error message printed to stderr by the command. |
| Window creation failure | Command prints error to stderr and returns `EXIT_FAILURE`. |
| Render device creation failure | Command prints error to stderr and returns `EXIT_FAILURE`. |
| Shader compilation failure (in triangle_demo via `setup_triangle()`) | Prints `FATAL` error to stderr and calls `std::exit(EXIT_FAILURE)` (same existing behaviour in `demo_helpers`). |
| Material linking failure | Same as shader compilation failure — prints error and exits. |
| Vertex buffer creation failure | Same as shader compilation failure — prints error and exits. |
| `main.cpp` receives `argc == 0` (impossible on hosted, but defensive) | No `argv[0]` to use as program name; dispatch defaults to `run`. |
| `argv[1]` is `nullptr` (defensive) | Treated as no arguments; defaults to `run`. |

## Permissions and security

- The CLI binary requires no elevated privileges (root/admin) to run.
- No network access is required at runtime.
- No secrets, credentials, or environment variables are consumed.
- The architecture boundary CONST-001 is preserved: no SDL3, OpenGL, or GLM headers are included from `src/cmd/`. All platform and graphics access goes through engine abstractions (`Platform`, `Window`, `RenderDevice`).
- The existing exception AMEND-2026-001 (SDL3 test files) is unaffected — it applies only to `tests/*_sdl3*.cpp`, not to `src/cmd/`.

## Observability

| Signal | Source |
|---|---|
| Command name being executed | Visible from program arguments; can be logged if needed |
| Version output | `buddd version` stdout |
| Interactive mode window opened | stdout: `"Window opened: 1024x768"` |
| Interactive mode shutdown | stdout: `"Window closed, shutting down."` |
| Demo mode started | stderr: `"Demo started: triangle (120 frames)"` |
| Demo mode complete | stderr: `"Demo complete: triangle (120 frames rendered)"` |
| Demo mode early abort | stderr: `"Demo aborted by user (frame N)"` |
| Demo mode unexpected args | stderr: `"Warning: unexpected arguments after 'demo triangle': ..."` |
| Demo usage (no name) | stderr: `"Usage: buddd demo <demo>"` + available demos |
| Unknown demo name | stderr: `"Unknown demo: '<name>'"` + demo usage |
| Engine initialisation error | stderr: command-specific error messages (already implemented) |
| Unknown command | stderr: error message + updated usage block |
| Exit code | Shell variable `$?` after the process exits |

No additional logging or metrics infrastructure is required. Each command is responsible for its own diagnostic output.

## Test implications

The existing test file `tests/version_test.cpp` contains `[cli]` tests that invoke the `buddd` binary and verify its output. The following changes are required:

1. **`buddd help outputs usage text`** — currently checks that `"test"` appears in stdout. Must be updated to check for `"demo"` instead of `"test"`.
2. **`buddd help ignores extra arguments`** — same update: `"test"` → `"demo"`.
3. **Tests referencing `buddd test`** — no direct test for `buddd test` exists in the current file, so no removal is strictly needed. However, any future tests that invoke `buddd test` must be removed or changed to `buddd demo triangle`.

New tests (required by CONST-002 — all unconditionally testable paths must have tests):

| Test | Condition | Description |
|---|---|---|
| `buddd demo` with no name | Always runs (no display needed) | Verify stderr contains demo usage; exit code is 1. |
| `buddd demo unknownname` | Always runs (no display needed) | Verify stderr contains `"Unknown demo:"`; exit code is 1. |
| `buddd test` is unknown command | Always runs (no display needed) | Verify stderr contains `"Unknown command: 'test'"`; exit code is 1. |
| `buddd demo triangle` | Guarded by `BUDDD_HAS_DISPLAY` | Verify the demo window opens and completes (with timeout). |

CONST-002 (Testing Policy) requires tests for all testable code. The following code paths are testable without a display and should have corresponding tests:
- `buddd demo` with no name (exit code 1, stderr contains usage)
- `buddd demo unknownname` (exit code 1, stderr contains `"Unknown demo:"`)
- `buddd test` as unknown command (exit code 1, stderr contains `"Unknown command: 'test'"`)
- `buddd help` text now references `demo` instead of `test`
- Version, unknown-command, and extra-arg tests all pass unchanged (except help text assertion)

Display-dependent paths (demo mode with window, run mode with window) are guarded by `BUDDD_HAS_DISPLAY` and tested at the integration level.

## Out of scope

- Unit tests for `DemoCommand` in isolation beyond the `[cli]` integration tests.
- Integration tests that automate window/rendering verification for demo modes (manual verification for now).
- Additional demos beyond `triangle` (future spec).
- A project/module loading system for `RunCommand`.
- Tab-completion scripts or shell integration.
- Subcommand aliases or short forms.
- A common base class or virtual interface for commands.
- CLI argument parsing library or framework.
- Dynamic demo discovery or plugin loading.
- Colourised or styled terminal output.
- Cross-platform testing of CLI argument parsing.
- Changes to the triangle rendering itself (vertex data, shaders, appearance are unchanged).

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | The `be::version()` function returns `"0.1.0"` (current value). The version output format is `"buddd <version>\n"`. |
| A-02 | The engine's public API (`Platform::create()`, `Platform::poll_events()`, `Window`, `RenderDevice`, etc.) is unchanged by this spec. |
| A-03 | The command dispatch uses the first positional argument (`argv[1]`) as the command name. If `argc == 1`, the default command is `run`. |
| A-04 | C++26 standard library features (`std::string_view`, `std::array`, `std::span`) are available for the dispatch implementation. |
| A-05 | The existing `src/cmd/CMakeLists.txt` glob pattern is extended to include `demo/*.cpp` in addition to the existing `*.cpp` and `commands/*.cpp` patterns. No root-level CMake changes are needed. |
| A-06 | The `setup_triangle()` function in `demo_helpers` is unchanged and used by `triangle_demo.cpp` via `#include`. |
| A-07 | The old `test` subcommand is dropped with no deprecation period. Developers who used it will see the unknown-command error and can switch to `buddd demo triangle`. |
| A-08 | The command namespace is `buddd::cmd`, consistent with the directory structure. |
| A-09 | The help text is an exact string embedded in `HelpCommand` (no i18n, no templating). If the set of commands changes, `HelpCommand` must be updated manually. |
| A-10 | When the CLI has no display (`BUDDD_HAS_DISPLAY=OFF` or no SDL3 at runtime), commands that try to open a window fail at the `Platform::create()` or `create_window()` stage with an engine error. The command's error handling is unchanged. |
| A-11 | Previous `[cli]` tests for `buddd` no-args default (in `tests/version_test.cpp`) still pass because `RunCommand` still outputs `"Window opened: 1024x768"` on stdout. The test checks for this substring and is unaffected by the triangle removal. |

## Open questions

| ID | Question | Impact |
|---|---|---|
| Q-01 | [RESOLVED] Should `buddd demo` (no name) print to stdout or stderr? | **Resolution**: stderr. Usage messages are diagnostic output, not program results. See User-visible behavior. |
| Q-02 | [RESOLVED] Should the extra-argument warning be printed by `DemoCommand` before dispatch, or by each individual demo function? | **Resolution**: printed by `DemoCommand` before dispatch, to avoid duplication across demos. The demo function receives `argc`/`argv` and can parse its own additional arguments if future demos need them. |
| Q-03 | [RESOLVED] When `buddd demo triangle` is run, should the warning iterate from `argv[3]` (extra args only) or `argv[2]` (including demo name)? | **Resolution**: Iterate from `argv[3]` (extra args only). The demo name is already known from `argv[2]` and should not be repeated in the warning. This is cleaner than the old `test` behavior. |
| Q-04 | [RESOLVED] What is the exact framebuffer-clear behaviour for `RunCommand`? | **Resolution**: Clear to black (0, 0, 0, 1). The implementation should issue a clear-colour call or equivalent to present a black framebuffer each frame. |
| Q-05 | [RESOLVED] Should `DemoCommand` pass the full `argc`/`argv` to the per-demo function? | **Resolution**: Yes. The demo function receives `argc` and `argv` so future demos can parse per-demo arguments. The triangle demo currently ignores them. |

## Required implementation behaviour

### Main.cpp changes

1. Replace `#include "commands/test_command.h"` with `#include "commands/demo_command.h"`.
2. Replace the `"test"` dispatch branch:
   - **Old**: `if (cmd == "test") { return bc::TestCommand{}.run(argc, argv); }`
   - **New**: No branch for `"test"`. `"test"` falls through to the unknown-command handler.
   - **New**: `if (cmd == "demo") { return bc::DemoCommand{}.run(argc, argv); }`
3. The unknown-command handler now uses the updated `k_usage_text` (which references `demo` instead of `test`). No code change needed if it already references the constant.

### DemoCommand implementation

```
DemoCommand::run(int argc, const char* const* argv) -> int:
   1. If argc < 3:
      - Print demo usage to stderr
      - Return EXIT_FAILURE
   2. Extract demo_name = argv[2]
   3. Create platform, window (800×600, title "Buddd Engine — Demo: <name>"), device
   4. If argc > 3:
      - Print warning to stderr: "Warning: unexpected arguments after 'demo <name>':" followed by argv[3..argc-1] space-separated, then "\n"
   5. Switch/case on demo_name:
      - "triangle" → call bc::demo::run_triangle_demo(**platform, **device, argc - 2, argv + 2), return its result
      - default → print "Unknown demo: '<demo_name>'\n\n" + demo usage to stderr, return EXIT_FAILURE

**Note**: The demo function receives `argc - 2` and `argv + 2`, so `argv[0]` in the demo function is the demo name (e.g., `"triangle"`) and `argv[1..]` are any extra arguments after the demo name.
```

### RunCommand changes

In `run_command.h`:
- Update the class doc comment: remove "renders a coloured triangle" and replace with "framebuffer clear only".

In `run_command.cpp`:
1. Remove `#include "demo_helpers.h"`.
2. Remove the `auto [material, vb] = bc::setup_triangle(**device);` line.
3. In the render loop, replace the `draw()` call with a framebuffer clear operation (no draw calls):
   - `(*device)->begin_frame();`  — this already clears the colour buffer to black in the OpenGL backend
   - `(*device)->end_frame();`

### CMakeLists.txt changes

Add `demo/*.cpp` to the glob:

```cmake
file(GLOB_RECURSE CMD_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/commands/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/demo/*.cpp
)
```

### Include path considerations

Files under `src/cmd/demo/` that include `demo_helpers.h` must use a path that resolves. Two approaches:
- **Option A**: Add `target_include_directories(buddd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/demo)` so that `#include "demo_helpers.h"` resolves from both `src/cmd/` and `src/cmd/demo/`.
- **Option B**: Update includes to `#include "../demo/demo_helpers.h"` or `#include "demo/demo_helpers.h"` with the existing include directory.

The existing `target_include_directories(buddd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})` in CMakeLists.txt makes `src/cmd/` the include root. Therefore `#include "demo/demo_helpers.h"` works from any file under `src/cmd/`. Files in `commands/` would use `#include "../demo/demo_helpers.h"` or the include path can be adjusted. The implementation should choose the cleanest approach.
