# SPEC-006 — CLI Command System

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

## Problem

The CLI binary (`src/cmd/main.cpp`) currently has no subcommand structure:

- Arguments are ad-hoc flags (`--test`, `--version`) with no positional command dispatch.
- Adding a new mode (e.g., `buddd benchmark`, `buddd scene`, `buddd info`) requires modifying `main.cpp` directly, adding new `if`/`else if` branches and growing the entry point.
- The test and interactive modes are implemented as local `static` functions inside `main.cpp`, making them invisible to testing and impossible to compose.
- There is no `buddd help` command — new users cannot discover available subcommands from the CLI itself.

## Goals

- Introduce a Command pattern where each subcommand corresponds to the first positional argument after `buddd`.
- Define four commands: `version`, `test`, `run`, `help`.
- Extract each command into its own `.h`/`.cpp` pair under `src/cmd/` with a public `run()` method that receives `argc`/`argv` and returns an exit code.
- Make `main.cpp` a thin dispatcher: parse the first positional argument, instantiate the matching command, invoke it, and return its exit code.
- Default to `run` when no positional argument is given (preserving the existing interactive behavior).
- Drop support for the old `--test` and `--version` flags entirely.
- Print a usage message for unknown commands to stderr and exit with code 1.
- Ensure the architecture boundary (CONST-001) is preserved: no SDL3, OpenGL, or GLM headers are included from `src/cmd/`.

## Supersedes

This spec supersedes the following acceptance criteria from earlier specs:

- **SPEC-001 AC-005** (running `buddd --version` prints version): Replaced by `buddd version` subcommand. The `--version` flag is dropped.
- **SPEC-001 AC-006** (running `buddd` with no arguments prints a greeting): Replaced by defaulting to `run` command (interactive window). The greeting message is no longer printed.

## Non-goals

- No changes to the engine library (`src/engine/`), its public API, or its file structure.
- No changes to the render pipeline, draw calls, shader compilation, or any rendering behavior.
- No changes to test infrastructure, `tests/` directory, or test coverage for commands. The existing integration-level tests (process invocation with specific arguments) validate command behavior. Dedicated unit tests for individual command classes are deferred — the dispatch logic consists of a simple if/else-if chain that is tested implicitly via the integration ACs (AC-006 through AC-018 cover all command paths). This is consistent with the principle that integration-level verification of CLI behavior suffices given the simplicity of the dispatch.
- No dynamic command loading, plugin system, or reflection.
- No CLI framework or third-party argument parsing library (C++26 standard library only).
- No subcommand aliases (e.g., `buddd v` for `buddd version`).
- No tab-completion or shell integration.
- No internationalisation or localised help text.

## Actors

| Actor | Description |
|---|---|
| Developer | A human running the `buddd` CLI binary from a terminal. Uses subcommands to invoke different engine modes. |
| CLI maintainer | A developer who adds new subcommands to the binary. Benefits from the Command pattern to avoid touching `main.cpp`. |

## User-visible behavior

### Command dispatch rules

1. The first positional argument after `buddd` selects the command.
2. If no positional argument is given (`argc == 1`), the default command is `run`.
3. Each command receives the full `argc`/`argv` and is responsible for its own argument parsing beyond the command name.
4. Commands are case-sensitive lowercase.
5. The old `--test` and `--version` flags are **not** supported. Passing them as the first positional argument produces an "unknown command" error (see below).

### Command behaviors

#### `buddd version` (VersionCommand)

Prints the version string to stdout and exits with code 0.

**Output format (exact)**:
```
buddd 0.1.0
```

The version string comes from `buddd::engine::version()`.

**Extra arguments**: silently ignored. All `argv` beyond the command name are ignored — version is always printed.

#### `buddd test` (TestCommand)

Opens an SDL3 window (800×600, title "Buddd Engine — Render Test"), renders a coloured triangle using the full render pipeline for 120 frames at approximately 60 FPS, then exits automatically.

**Behavior details**:
- Platform created with `be::Platform::create(be::Backend::SDL3)`.
- Window dimensions: 800×600.
- Window title: `"Buddd Engine — Render Test"`.
- Render loop runs exactly 120 frames.
- Between frames, sleeps to maintain ~60 FPS (16 ms per frame).
- If the user closes the window during the test, the loop is aborted early, a message is printed to stderr, and the command exits with code 0 (this is not an error).
- Prints `"Render test started: 120 frames"` to stderr at the start.
- Prints `"Render test complete: 120 frames rendered"` to stderr on success.
- Prints `"Render test aborted by user (frame N)"` to stderr if aborted early.
- **Extra arguments**: if any positional arguments follow `test`, the command prints a warning to stderr (`"Warning: unexpected arguments after 'test': ..."`) but still proceeds with the test.

#### `buddd run` (RunCommand)

Opens an SDL3 window (1024×768, title "Buddd Engine"), renders a coloured triangle using the full render pipeline, and continues until the user closes the window.

**Behavior details**:
- Platform created with `be::Platform::create(be::Backend::SDL3)`.
- Window dimensions: 1024×768.
- Window title: `"Buddd Engine"`.
- Prints `"Window opened: 1024x768"` to stdout (matching current behavior).
- Render loop runs until `(*platform)->poll_events()` returns `false` (window close).
- Prints `"Window closed, shutting down."` to stdout on exit.
- **Extra arguments**: currently ignored. The `run` command receives the full `argv` but does not validate or warn about extra positional arguments (reserved for future subcommand arguments).

#### `buddd help` (HelpCommand)

Prints usage information to stdout and exits with code 0.

**Output format (exact)**:
```
Usage: buddd <command> [<args>]

Commands:
  run       Run the engine in interactive mode (default)
  test      Run automated render test (120 frames, then exit)
  version   Print version information
  help      Show this help message
```

**Extra arguments**: silently ignored. Help is always shown regardless of extra arguments.

#### Unknown command

If the first positional argument does not match any known command, the CLI prints an error message to **stderr** and exits with code 1.

**Output format (exact)**:
```
Unknown command: '<cmd>'

Usage: buddd <command> [<args>]

Commands:
  run       Run the engine in interactive mode (default)
  test      Run automated render test (120 frames, then exit)
  version   Print version information
  help      Show this help message
```

Where `<cmd>` is the unrecognised argument. The usage block is identical to the `help` command output.

## Key entities

### Command interface

Each command is a class in namespace `buddd::cmd` (aliased as `bc` in `main.cpp` if needed). Every command class provides a public method with the following contract:

```cpp
/// Executes the command.
/// @param argc  Argument count (including the command name as argv[0]).
/// @param argv  Argument vector (argv[0] is the program name, argv[1] is the
///              command name, argv[2..argc-1] are per-command arguments).
/// @return      Exit code (0 for success, non-zero on error).
auto run(int argc, const char* const* argv) -> int;
```

Command classes do not require a common base class (the dispatch in `main.cpp` can use direct instantiation or a function pointer table). Each command is:

- **Self-contained**: all logic for that mode lives in its `.h`/`.cpp` pair.
- **Stateless** (no mutable state beyond local variables within `run()`).
- **Independent** (no shared mutable global state between commands).

### File structure

| File | Content |
|---|---|
| `src/cmd/commands/version_command.h` | Declaration of `buddd::cmd::VersionCommand` |
| `src/cmd/commands/version_command.cpp` | Implementation: prints `buddd <version>` from `be::version()` |
| `src/cmd/commands/test_command.h` | Declaration of `buddd::cmd::TestCommand` |
| `src/cmd/commands/test_command.cpp` | Implementation: 120-frame test render loop |
| `src/cmd/commands/run_command.h` | Declaration of `buddd::cmd::RunCommand` |
| `src/cmd/commands/run_command.cpp` | Implementation: interactive render loop |
| `src/cmd/commands/help_command.h` | Declaration of `buddd::cmd::HelpCommand` |
| `src/cmd/commands/help_command.cpp` | Implementation: prints usage text |
| `src/cmd/main.cpp` | Thin dispatcher: parse first arg, dispatch to command, return exit code |

Additional shared utility files (e.g., for the triangle setup helper used by both `RunCommand` and `TestCommand`) may be added with `snake_case` naming directly in `src/cmd/` (not in the `commands/` subdirectory), since they are not commands themselves.

## User stories

### Story 1 — Run interactive mode (Priority: P1)

As a developer, I want to run `buddd` with no arguments and see the interactive 3D window, so that I can verify the engine works out of the box.

**Given** the `buddd` binary is compiled
**When** I run `buddd` with no arguments
**Then** a window opens (1024×768, title "Buddd Engine") and renders a coloured triangle until I close it, then exits with code 0.

**Given** the `buddd` binary is compiled
**When** I run `buddd run`
**Then** the behavior is identical to running `buddd` with no arguments.

### Story 2 — Run automated test (Priority: P1)

As a developer, I want to run `buddd test` and have it render for a fixed number of frames then exit automatically, so that I can verify the render pipeline works without manual interaction.

**Given** the `buddd` binary is compiled
**When** I run `buddd test`
**Then** a window opens (800×600, title "Buddd Engine — Render Test"), renders for 120 frames, prints completion to stderr, and exits with code 0.

### Story 3 — Print version (Priority: P1)

As a developer, I want to run `buddd version` and see the current version string, so that I can confirm which build I am running.

**Given** the `buddd` binary is compiled
**When** I run `buddd version`
**Then** stdout contains `"buddd 0.1.0"` and the process exits with code 0.

### Story 4 — Show help (Priority: P1)

As a new developer, I want to run `buddd help` and see a list of available commands, so that I can discover how to use the CLI without reading documentation.

**Given** the `buddd` binary is compiled
**When** I run `buddd help`
**Then** stdout contains the usage message listing all four commands, and the process exits with code 0.

### Story 5 — Unknown command error (Priority: P1)

As a developer, I want to see a clear error message when I mistype a command, so that I know what went wrong and what commands are available.

**Given** the `buddd` binary is compiled
**When** I run `buddd unknowncommand`
**Then** stderr contains `"Unknown command: 'unknowncommand'"` followed by the usage block, and the process exits with code 1.

### Story 6 — Old flags rejected (Priority: P2)

As a developer who remembers the old `--test` or `--version` flags, I want the CLI to tell me those flags no longer work, so that I learn the new subcommand syntax.

**Given** the `buddd` binary is compiled
**When** I run `buddd --test` or `buddd --version`
**Then** stderr contains `"Unknown command: '--test'"` (or `"--version"`) followed by the usage block, and the process exits with code 1.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `main.cpp` no longer contains the inline implementation of test mode, interactive mode, or version printing. All three are in their own `.h`/`.cpp` command files under `src/cmd/commands/`. | Inspect `main.cpp` — it must only contain the dispatch logic and any shared helper includes. The `run_test_mode()`, `run_interactive()` static functions must not exist. |
| AC-002 | A file `src/cmd/commands/version_command.h` and `src/cmd/commands/version_command.cpp` exist declaring `buddd::cmd::VersionCommand` with a public `run(int, const char* const*) -> int` method. | Files exist and compile without error. |
| AC-003 | A file `src/cmd/commands/test_command.h` and `src/cmd/commands/test_command.cpp` exist declaring `buddd::cmd::TestCommand` with a public `run(int, const char* const*) -> int` method. | Files exist and compile without error. |
| AC-004 | A file `src/cmd/commands/run_command.h` and `src/cmd/commands/run_command.cpp` exist declaring `buddd::cmd::RunCommand` with a public `run(int, const char* const*) -> int` method. | Files exist and compile without error. |
| AC-005 | A file `src/cmd/commands/help_command.h` and `src/cmd/commands/help_command.cpp` exist declaring `buddd::cmd::HelpCommand` with a public `run(int, const char* const*) -> int` method. | Files exist and compile without error. |
| AC-006 | Running `buddd` with no arguments opens a 1024×768 window titled "Buddd Engine" with a coloured triangle. Closing the window exits with code 0. | Manual visual verification. |
| AC-007 | Running `buddd run` produces identical behavior to `buddd` with no arguments. | Manual visual verification. |
| AC-008 | Running `buddd test` opens an 800×600 window titled "Buddd Engine — Render Test", renders for 120 frames, prints completion message to stderr, and exits with code 0. | Manual visual verification; stderr contains `"Render test complete: 120 frames rendered"`. |
| AC-009 | Running `buddd test` and closing the window before 120 frames prints `"Render test aborted by user"` to stderr and exits with code 0. | Manual verification. |
| AC-010 | Running `buddd version` prints `"buddd 0.1.0"` to stdout and exits with code 0. | Run `buddd version`; stdout matches the expected output exactly. |
| AC-011 | Running `buddd help` prints the usage message to stdout and exits with code 0. | Run `buddd help`; stdout contains the usage block listing `run`, `test`, `version`, `help`. |
| AC-012 | Running `buddd unknowncommand` prints `"Unknown command: 'unknowncommand'"` followed by the usage block to stderr and exits with code 1. | Run `buddd unknowncommand`; stderr contains the error and usage. Exit code is 1. |
| AC-013 | Running `buddd --test` or `buddd --version` prints the unknown command error to stderr and exits with code 1. | Run both; stderr contains `"Unknown command: '--test'"` / `"Unknown command: '--version'"`. Exit code is 1. |
| AC-014 | No SDL3, OpenGL, or GLM headers are included from any file under `src/cmd/`. | Run `grep -rnE '(SDL3|GL/|glad|glm)' src/cmd/` — zero matches. |
| AC-015 | The `src/cmd/CMakeLists.txt` uses `file(GLOB_RECURSE CONFIGURE_DEPENDS ...)` to include all source files from both `src/cmd/` and `src/cmd/commands/`, and the `buddd` executable links successfully. | `cmake --build --preset debug` succeeds; `build/debug/src/cmd/buddd` is produced. |
| AC-016 | Running `buddd version extra_arg` still prints the version and exits 0 (extra args ignored). | Run `buddd version extra_arg`; output matches AC-010. |
| AC-017 | Running `buddd help extra_arg` still prints the usage message and exits 0 (extra args ignored). | Run `buddd help extra_arg`; output matches AC-011. |
| AC-018 | Running `buddd test extra_arg` prints a warning to stderr but still runs the test and exits 0. | Run `buddd test extra_arg`; stderr contains `"Warning: unexpected arguments after 'test': ..."` and the test completes normally. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A new command can be added by creating one `.h`/`.cpp` pair in `src/cmd/commands/` and adding an `else if` branch to `main.cpp`, without modifying any other command file or `CMakeLists.txt` (glob picks up new files automatically). | Create a skeleton `info_command.h/.cpp` in `src/cmd/commands/` with a `run()` that prints "info stub". Wire it into `main.cpp`. Build succeeds. |
| SC-002 | The command dispatch logic is a single if/else-if chain mapping command-name strings to command invocations, contained within the first 30 lines of `main()`. | Inspect `main.cpp` — the if/else-if chain is the top-level dispatch and is fully visible within the first 30 lines of `main()`. |
| SC-003 | All four commands show identical render behavior to the current implementation (same window dimensions, same frame count, same triangle appearance). | Manual visual comparison between old and new binary for `run` and `test` modes. |

## Edge cases

| Case | Expected behavior |
|---|---|
| `buddd` with trailing whitespace in shell (no extra args) | Treated as no arguments; default to `run`. |
| `buddd   ` (multiple spaces between args) | Shell normalises whitespace; same as no extra args. |
| `buddd ''` (empty string as command) | Empty string is not a valid command; treated as unknown command. |
| `buddd RUN` (uppercase) | Case-sensitive comparison fails; treated as unknown command. |
| `buddd run extra1 extra2` (multiple extra args) | RunCommand receives all args; extra args are silently ignored. |
| `buddd test extra` (extra arg to test) | Warning printed to stderr; test proceeds. |
| `buddd help --verbose` (extra flag-like arg) | HelpCommand ignores extra args; help is printed. |
| `buddd version --format json` (extra arg to version) | VersionCommand ignores extra args; version is printed. |
| Window closed during `run` mode before first frame renders | `poll_events()` returns `false` on first call; loop exits immediately; prints "Window closed, shutting down."; exits 0. |
| Window closed during `test` mode on frame 0 | Abort message printed; exits with code 0. |
| Build configured with `BUDDD_HAS_DISPLAY=OFF` | The `buddd` CLI binary still links and commands that use SDL3 backend will fail at runtime with a platform creation error. This is expected — the spec does not change this existing behavior. |
| `buddd --` (double dash) | `argv[1]` is `"--"`; treated as unknown command; unknown command error printed. |

## Error cases

| Case | Expected behavior |
|---|---|
| Unknown command | Print to stderr: `"Unknown command: '<cmd>'"` followed by the usage block. Exit code 1. |
| Engine initialisation failure (e.g., SDL3 not available) | The command's `run()` returns `EXIT_FAILURE`. Error message is printed to stderr by the command itself (same as current behavior). |
| Window creation failure | Command prints error to stderr and returns `EXIT_FAILURE`. |
| Render device creation failure | Command prints error to stderr and returns `EXIT_FAILURE`. |
| Shader compilation failure | Command prints error to stderr and calls `std::exit(EXIT_FAILURE)` (same as current fatal behaviour in `setup_triangle()`). |
| Material linking failure | Same as shader compilation failure — prints error and exits. |
| Vertex buffer creation failure | Same as shader compilation failure — prints error and exits. |
| `main.cpp` receives `argc == 0` (impossible on hosted implementations, but defensive) | No `argv[0]` to use as program name. Dispatch uses default `run` command. |
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
| Test mode progress | stderr: `"Render test started: 120 frames"` / `"Render test complete: ..."` |
| Test mode early abort | stderr: `"Render test aborted by user (frame N)"` |
| Test mode unexpected args | stderr: `"Warning: unexpected arguments after 'test': ..."` |
| Engine initialisation error | stderr: command-specific error messages (already implemented) |
| Unknown command | stderr: error message + usage block |
| Exit code | Shell variable `$?` after the process exits |

No additional logging or metrics infrastructure is required. Each command is responsible for its own diagnostic output.

## Out of scope

- Unit tests for individual commands (each command is testable via shell invocation; dedicated unit tests are a future concern).
- Integration tests that automate window/rendering verification.
- Tab-completion scripts or shell integration.
- Subcommand aliases or short forms (`v` for `version`, `h` for `help`, etc.).
- A common base class or virtual interface for Command (each command is standalone; dispatch uses if/else-if chain in `main.cpp`).
- CLI argument parsing library or framework (C++26 standard library features only).
- Dynamic command discovery or plugin loading.
- Colourised or styled terminal output.
- Cross-platform testing of CLI argument parsing (single platform at this stage).

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | The `be::version()` function returns `"0.1.0"` (current value). The version output format is `"buddd <version>\n"`. |
| A-02 | The engine's public API (`Platform::create()`, `Platform::poll_events()`, `Window`, `RenderDevice`, etc.) is unchanged by this spec. |
| A-03 | The command dispatch uses the first positional argument (`argv[1]`) as the command name. If `argc == 1`, the default command is `run`. `RunCommand` silently ignores extra arguments beyond the command name. |
| A-04 | C++26 standard library features (`std::string_view`, `std::array`, `std::span`) are available for the dispatch implementation. |
| A-05 | The existing `src/cmd/CMakeLists.txt` is modified to use `file(GLOB_RECURSE CONFIGURE_DEPENDS ...)` globbing from both `src/cmd/` (for `main.cpp` and shared helpers) and `src/cmd/commands/` (for command files), and adds a `target_include_directories` for the source directory so that includes resolve correctly across subdirectories. The root `CMakeLists.txt` already includes `add_subdirectory(src/cmd)` — no root-level CMake changes are needed. |
| A-06 | The `setup_triangle()` helper currently duplicated across command files (or extracted to a shared utility file) is acceptable as long as commands are self-contained. A shared utility file (e.g., `demo_helpers.h`/`.cpp`) is preferred over duplication. |
| A-07 | The old `--test` and `--version` flags are dropped with no deprecation period. Developers who used them will see the unknown command error and can switch to `buddd test` and `buddd version`. |
| A-08 | The command namespace is `buddd::cmd`, consistent with the directory structure (`src/cmd/`). The existing `be = buddd::engine` alias in `main.cpp` may be joined by `namespace bc = buddd::cmd`. |
| A-09 | The help text is an exact string embedded in `HelpCommand` (no i18n, no templating). If the set of commands changes, `HelpCommand` must be updated manually. |
| A-10 | When the CLI has no display (`BUDDD_HAS_DISPLAY=OFF` or no SDL3 at runtime), commands that try to open a window fail at the `Platform::create()` or `create_window()` stage with an engine error. The command's error handling is unchanged by this refactor. |

## Open questions

| ID | Question | Impact |
|---|---|---|
| Q-01 | [RESOLVED] `RunCommand` silently ignores extra arguments. Future subcommand-specific arguments can be parsed when they are actually needed, with no breaking change required. | **Resolution**: Silently ignore. See Assumption A-03. |
| Q-02 | [RESOLVED] Dispatch uses a simple `if`/`else if` chain in `main.cpp`. This is sufficient for four commands and keeps the entry point immediately readable. | **Resolution**: If/else-if chain. See User-visible behavior > Command dispatch rules. |
| Q-03 | [RESOLVED] `src/cmd/CMakeLists.txt` switches to `file(GLOB_RECURSE CONFIGURE_DEPENDS src/cmd/*.cpp)` to automatically pick up new command files, matching the engine's convention. | **Resolution**: Switch to glob. See AC-015 and required implementation behavior. |
