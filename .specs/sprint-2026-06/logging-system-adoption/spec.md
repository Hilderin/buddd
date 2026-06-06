# SPEC-022 — Logging System Adoption (Ad-hoc stdout/stderr Migration)

## Problem

The codebase has approximately 190 diagnostic output statements spread across ~50 files in `src/engine/` and `src/cmd/`, using four different ad-hoc mechanisms:

- `std::cerr << ...` (most common)
- `fprintf(stderr, ...)`
- `printf(...)` (stdout)
- `write(STDERR_FILENO, ...)`

These calls bypass the structured logging system (`buddd::log::Logger`, `BUDDD_LOG_*` macros, `ConsoleSink`, `FileSink`) that was introduced in SPEC-021. The result is:

- **No level filtering** — all diagnostic output is mixed together regardless of severity.
- **No source tagging** — messages lack a consistent module tag, making it hard to identify origin.
- **No runtime control** — verbosity cannot be controlled via `--log-level` or `--log-filter`.
- **No file sink** — diagnostic output is lost in automated/headless runs unless explicitly redirected.
- **Inconsistent format** — some messages use `[Tag]` prefixes by convention, most do not.
- **Thread-unsafe output** — `std::cerr` and `fprintf` are not guaranteed atomic under concurrent writes.

Migrating all diagnostic output to the logging system solves all of these problems and ensures a single, consistent, controllable output path.

## Goals

- Migrate all diagnostic `std::cerr`, `fprintf(stderr, ...)`, `printf(...)`, and `write(STDERR_FILENO, ...)` calls in `src/engine/` and `src/cmd/` to the equivalent `BUDDD_LOG_*` macro.
- Assign a consistent `BUDDD_LOG_TAG` to each translation unit based on its module identity.
- Map each existing ad-hoc statement to the correct log level (DEBUG, INFO, WARN, or ERROR) based on its semantics.
- Ensure `Logger::init()` is called before any `BUDDD_LOG_*` macro fires (true for all migrated files — `main.cpp` already calls `Logger::init()` before any subsystem init code runs).
- Preserve the pre-`Logger::init` bootstrap error on line 37 of `main.cpp` as raw `fprintf(stderr)` — it cannot use the logger because the logger has not been initialized yet.
- Preserve help/usage text blocks in `main.cpp` as `fprintf(stderr)` — they are user-facing instructional text, not diagnostic messages.

## Non-goals

- Do NOT modify the logging system implementation (`log/console_sink.cpp`, `log/file_sink.cpp`, `log/log.h`, `log/logger.cpp`, `log/log_filter.cpp`).
- Do NOT modify `help_command.cpp` or `version_command.cpp` — their output to stdout is intentional, not diagnostic.
- Do NOT modify `Image::save()` — that's intentional data output, not diagnostic.
- Do NOT modify data file writes (`hot_reload_gltf_app.cpp` `std::ofstream` YAML output).
- Do NOT change user-facing behavior or message content — only the output mechanism changes. Exception: `std::exit()`/`std::terminate()` calls following fatal error logging are replaced with normal control flow (the error is logged via `BUDDD_LOG_ERROR` and execution continues) — this is an intentional, documented behavioral change (see Story 4, AC-016).
- Do NOT add new log messages or restructure code.
- Do NOT touch any file outside `src/engine/` and `src/cmd/`.
- Do NOT introduce any new dependencies or libraries.

## Actors

| Actor | Interaction |
|---|---|
| **Engine developer** | Reads log output during development. Benefits from level filtering, source tags, and runtime verbosity control. |
| **CI/CD pipeline** | Runs headless captures with `--log-level` / `--log-file` to capture diagnostic output for post-mortem analysis. |
| **End-user (CLI operator)** | Runs engine demos with controlled verbosity. Error messages go to stderr via the logging system; usage/help text remains on stderr as raw `fprintf`. |
| **Code reviewer** | Verifies that each migration preserves original semantics (same message, same conditions, appropriate log level). |

## User-visible behavior

1. All previous `std::cerr` / `fprintf(stderr)` / `printf()` / `write(STDERR_FILENO)` diagnostic output now goes through `BUDDD_LOG_*` macros.
2. Output format changes from bare text to `[LEVEL] [Tag] message\n`.
3. Message content (text, variables, formatting) remains identical to the original.
4. `printf()` to stdout diagnostic output (window lifecycle, capture confirmation) moves to `BUDDD_LOG_INFO` on stderr — it is no longer on stdout.
5. Pre-`Logger::init` bootstrap error on `main.cpp:37` remains as `fprintf(stderr)`.
6. Help/usage text blocks in `main.cpp` (unknown command, unknown scene) remain as `fprintf(stderr)`. Only the error line (e.g., "Unknown command: 'foo'") becomes `BUDDD_LOG_ERROR`.
7. All migrated output is controllable via `--log-level`, `--log-filter`, and capturable via `--log-file`.
8. `#ifndef NDEBUG`-guarded `std::cerr` calls become unconditional `BUDDD_LOG_DEBUG` calls — the log level replaces compile-time gating.
9. Fatal errors that previously called `std::exit()` or `std::terminate()` after `std::cerr` now log via `BUDDD_LOG_ERROR` and continue without terminating (unless the error is truly unrecoverable).

## User stories

### Story 1 — Single diagnostic output path (Priority: P1)

As an engine developer, I want all diagnostic output to go through the logging system so that I can control verbosity and capture logs to a file.

**Given** the engine is running  
**When** any diagnostic event occurs (error, warning, info, debug)  
**Then** the output appears as a `[LEVEL] [Tag] message\n` line on stderr (or in the log file if `--log-file` is set).

**Given** the engine is running with `--log-level=error`  
**When** a warning-level diagnostic event occurs  
**Then** the warning is suppressed (does not appear on stderr or in the log file).

### Story 2 — Consistent module tags (Priority: P1)

As a developer debugging a specific subsystem, I want each log message to carry a consistent tag so that I can filter by module.

**Given** the engine is running  
**When** a message originates from `src/engine/asset/asset_manager.cpp`  
**Then** the output carries the tag `[Asset]`.

**Given** the engine is running with `--log-filter=Render:OpenGL=debug`  
**When** a `debug`-level message originates from `src/engine/render/render_device_opengl.cpp`  
**Then** the message appears (tag `[Render:OpenGL]` is above the filter threshold).

### Story 3 — Runtime level control replaces compile-time gating (Priority: P1)

As a developer, I want `#ifndef NDEBUG`-guarded diagnostics to be controlled at runtime so that I don't need to recompile to see them.

**Given** the engine is built in release mode (`NDEBUG` defined)  
**When** running with `--log-level=debug`  
**Then** previously `#ifndef NDEBUG`-guarded messages (now `BUDDD_LOG_DEBUG`) appear.

**Given** the engine is built in release mode  
**When** running with default settings (release default: `--log-level=warn`)  
**Then** `BUDDD_LOG_DEBUG` messages are suppressed.

### Story 4 — Fatal errors log without termination (Priority: P2)

As a developer, I want non-recoverable errors to be logged and let the application continue gracefully rather than terminating abruptly, except where truly unrecoverable.

**Given** a "FATAL" diagnostic event occurs  
**When** the original code would call `std::exit()` or `std::terminate()` after the message  
**Then** the message is logged via `BUDDD_LOG_ERROR` and execution continues (no forced termination).

### Story 5 — Pre-init bootstrap error survives (Priority: P1)

As an operator, I want to see parse errors for logging CLI flags even when the logger itself has not been initialized.

**Given** an invalid `--log-level` flag is passed  
**When** the engine starts  
**Then** the error message is written via raw `fprintf(stderr)` on line 37 of `main.cpp` (before `Logger::init()`).

### Story 6 — Help/usage text preserved (Priority: P1)

As a user, I want to see the full usage text when I type an invalid command or scene.

**Given** I type `buddd unknown`  
**When** the engine starts  
**Then** the error line appears via `BUDDD_LOG_ERROR`, and the usage/help text block appears via `fprintf(stderr)`.

## Tag mapping

Each file that currently uses ad-hoc diagnostic output must declare a `BUDDD_LOG_TAG` at file scope. The following tags are assigned by module:

| Directory / Module | Tag | Files |
|---|---|---|
| `src/cmd/app.cpp` | `App` | App lifecycle, capture diagnostics |
| `src/cmd/apps/hot_reload_app.cpp` | `HotReload` | Hot-reload demo app |
| `src/cmd/apps/hot_reload_gltf_app.cpp` | `HotReload` | Hot-reload glTF demo app |
| `src/cmd/apps/gltf_demo_app.cpp` | `GltfDemo` | glTF demo app |
| `src/cmd/apps/textured_cube_app.cpp` | `TexturedCube` | Textured cube demo app |
| `src/cmd/apps/asset_demo_app.cpp` | `AssetDemo` | Asset pipeline demo app |
| `src/cmd/apps/phong_app.cpp` | `Phong` | Phong lighting demo app |
| `src/cmd/apps/cube_app.cpp` | `Cube` | Cube demo app |
| `src/cmd/apps/multi_material_app.cpp` | `MultiMaterial` | Multi-material cube demo app |
| `src/engine/asset/asset_manager.cpp` | `Asset` | Asset manager |
| `src/engine/asset/asset_manager.tpp` | `Asset` | Asset manager template (same tag as .cpp) |
| `src/engine/asset/model_loader.cpp` | `Asset:ModelLoader` | glTF model loader |
| `src/engine/asset/file_watcher_inotify.cpp` | `Asset:FileWatcher` | Inotify file watcher |
| `src/engine/render/render_device.cpp` | `Render` | Render device factory |
| `src/engine/render/render_device_opengl.cpp` | `Render:OpenGL` | OpenGL render device |
| `src/engine/render/render_device_headless.cpp` | `Render:Headless` | Headless render device |
| `src/engine/render/render_system.cpp` | `Render` | Render system |
| `src/engine/render/material_opengl.cpp` | `Render:OpenGL` | OpenGL material |
| `src/engine/render/material_headless.cpp` | `Render:Headless` | Headless material |
| `src/engine/render/texture_opengl.cpp` | `Render:OpenGL` | OpenGL texture |
| `src/engine/render/shader_program_opengl.cpp` | `Render:OpenGL` | OpenGL shader program |
| `src/engine/render/shader_program_headless.cpp` | `Render:Headless` | Headless shader program |
| `src/engine/render/phong/phong_material.cpp` | `Render:Phong` | Phong material |
| `src/engine/render/pbr/pbr_material.cpp` | `Render:Pbr` | PBR material |
| `src/engine/platform/platform.cpp` | `Platform` | Platform factory |
| `src/engine/platform/platform_headless.cpp` | `Platform:Headless` | Headless platform |
| `src/engine/platform/platform_sdl3.cpp` | `Platform:SDL3` | SDL3 platform |
| `src/engine/input/input_system.cpp` | `Input` | Input system factory |
| `src/engine/input/input_system_sdl3.cpp` | `Input:SDL3` | SDL3 input system |
| `src/engine/scene/camera_component.cpp` | `Scene:ECSCamera` | Camera component (ECS) |

**Note for `main.cpp`**: This file already has a log tag declaration. Its existing `fprintf(stderr)` calls that become `BUDDD_LOG_ERROR` should use tag `App` (already declared).

## Level mapping

Each ad-hoc diagnostic pattern maps to a log level based on its semantics:

| Original pattern | Example | Log level | Rationale |
|---|---|---|---|
| `std::cerr << "FATAL: ..."` | `"FATAL: could not create window"` | `BUDDD_LOG_ERROR` | Recoverable failure that prevents an operation |
| `std::cerr << "Warning: ..."` | `"Warning: set_uniform failed"` | `BUDDD_LOG_WARN` | Non-fatal, operation continues with degraded behaviour |
| `std::cerr << "[Tag] Warn: ..."` | `"[Asset] Warn: data URI not supported"` | `BUDDD_LOG_WARN` | Explicitly labelled as warning |
| `std::cerr << "[Tag] <info>"` | `"[Asset] Texture created: foo"` | `BUDDD_LOG_INFO` | Informational status update |
| `std::cerr << "Shader created..."` | `"Shader created (type=...)"` | `BUDDD_LOG_DEBUG` | Detailed tracing useful during development |
| `std::cerr << "[Tag] Cache cleared..."` | `"[Asset] Cache cleared (N assets)"` | `BUDDD_LOG_DEBUG` | Internal housekeeping detail |
| `std::cerr << "Uniform cached: ..."` | `"Uniform cached: u_mvp (type=Mat4)"` | `BUDDD_LOG_DEBUG` | Very fine-grained tracing |
| `std::cerr << "[Tag] glTF error: ..."` | `"[Asset] glTF error: ..."` | `BUDDD_LOG_ERROR` | Error loading asset |
| `std::cerr << "[Tag] glTF warning: ..."` | `"[Asset] glTF warning: ..."` | `BUDDD_LOG_WARN` | Non-fatal glTF issue |
| `std::cerr << "RenderSystem: ..."` (no lights) | `"RenderSystem: collected N lights"` | `BUDDD_LOG_DEBUG` | Internal render system detail |
| `#ifndef NDEBUG std::cerr` | Cache hit, Asset:FileWatcher start/stop | `BUDDD_LOG_DEBUG` | Previously compile-time gated; now runtime-controlled |
| `std::printf("Window opened...")` | `"Window opened: 800x600"` | `BUDDD_LOG_INFO` | Informational lifecycle event (moves to stderr) |
| `std::printf("Captured: ...")` | `"Captured: /tmp/frame.png"` | `BUDDD_LOG_INFO` | Informational capture confirmation (moves to stderr) |
| `std::printf("Window closed...")` | `"Window closed, shutting down."` | `BUDDD_LOG_INFO` | Informational lifecycle event (moves to stderr) |
| `fprintf(stderr, "Scene started: ...")` | `"Scene started: MyApp (120 frames)"` | `BUDDD_LOG_INFO` | Informational lifecycle event |
| `fprintf(stderr, "Scene aborted...")` | `"Scene aborted by user"` | `BUDDD_LOG_INFO` | Informational lifecycle event |
| `fprintf(stderr, "Scene complete: ...")` | `"Scene complete: MyApp (120 frames rendered)"` | `BUDDD_LOG_INFO` | Informational lifecycle event |
| `fprintf(stderr, "Error: ...")` in `main.cpp` | `"Error: <parse error>"` | `BUDDD_LOG_ERROR` | Command-line parsing error (post-init) |
| `fprintf(stderr, "Unknown command: ...")` | `"Unknown command: 'foo'"` (error line only) | `BUDDD_LOG_ERROR` | CLI error (help text stays as fprintf) |
| `fprintf(stderr, "Unknown scene: ...")` | `"Unknown scene: 'bar'"` (error line only) | `BUDDD_LOG_ERROR` | CLI error (help text stays as fprintf) |
| `fprintf(stderr, "Warning: unexpected...")` | `"Warning: unexpected arguments..."` | `BUDDD_LOG_WARN` | Non-fatal CLI warning |

## Files to modify

### src/cmd/ (App lifecycle)

| File | Statements | Tag |
|---|---|---|
| `src/cmd/app.cpp` | 14 | `App` |
| `src/cmd/main.cpp` | 6 (post-init) | `App` (already declared) |

### src/cmd/apps/ (Demo apps)

| File | Statements | Tag |
|---|---|---|
| `src/cmd/apps/hot_reload_app.cpp` | 7 | `HotReload` |
| `src/cmd/apps/hot_reload_gltf_app.cpp` | 5 | `HotReload` |
| `src/cmd/apps/gltf_demo_app.cpp` | 2 | `GltfDemo` |
| `src/cmd/apps/textured_cube_app.cpp` | 7 | `TexturedCube` |
| `src/cmd/apps/asset_demo_app.cpp` | 3 | `AssetDemo` |
| `src/cmd/apps/phong_app.cpp` | 9 | `Phong` |
| `src/cmd/apps/cube_app.cpp` | 1 | `Cube` |
| `src/cmd/apps/multi_material_app.cpp` | 1 | `MultiMaterial` |

### src/engine/asset/ (Asset system — P0 priority)

| File | Statements | Tag |
|---|---|---|
| `src/engine/asset/asset_manager.cpp` | 42 | `Asset` |
| `src/engine/asset/asset_manager.tpp` | 1 | `Asset` |
| `src/engine/asset/model_loader.cpp` | 14 | `Asset:ModelLoader` |
| `src/engine/asset/file_watcher_inotify.cpp` | 6 | `Asset:FileWatcher` |

### src/engine/render/ (Render system — P0 priority)

| File | Statements | Tag |
|---|---|---|
| `src/engine/render/render_device.cpp` | 3 | `Render` |
| `src/engine/render/render_device_opengl.cpp` | 15 | `Render:OpenGL` |
| `src/engine/render/render_device_headless.cpp` | 13 | `Render:Headless` |
| `src/engine/render/render_system.cpp` | 3 | `Render` |
| `src/engine/render/material_opengl.cpp` | 9 | `Render:OpenGL` |
| `src/engine/render/material_headless.cpp` | 14 | `Render:Headless` |
| `src/engine/render/texture_opengl.cpp` | 1 | `Render:OpenGL` |
| `src/engine/render/shader_program_opengl.cpp` | 1 | `Render:OpenGL` |
| `src/engine/render/shader_program_headless.cpp` | 1 | `Render:Headless` |
| `src/engine/render/phong/phong_material.cpp` | 5 | `Render:Phong` |
| `src/engine/render/pbr/pbr_material.cpp` | 3 | `Render:Pbr` |

### src/engine/platform/ (Platform system)

| File | Statements | Tag |
|---|---|---|
| `src/engine/platform/platform.cpp` | 5 | `Platform` |
| `src/engine/platform/platform_headless.cpp` | 1 | `Platform:Headless` |
| `src/engine/platform/platform_sdl3.cpp` | 2 | `Platform:SDL3` |

### src/engine/input/ (Input system)

| File | Statements | Tag |
|---|---|---|
| `src/engine/input/input_system.cpp` | 2 | `Input` |
| `src/engine/input/input_system_sdl3.cpp` | 2 (debug-only) | `Input:SDL3` |

### src/engine/scene/ (Scene/ECS system)

| File | Statements | Tag |
|---|---|---|
| `src/engine/scene/camera_component.cpp` | 2 (debug-only) | `Scene:ECSCamera` |

### Total

Approximately 202 statements across 31 files.

## What NOT to modify

The following files and patterns are explicitly excluded:

| File / Pattern | Reason |
|---|---|
| `src/engine/log/console_sink.cpp` | Logging system implementation — not diagnostic output |
| `src/engine/log/file_sink.cpp` | Logging system implementation — not diagnostic output |
| `src/engine/log/log.h` | Logging system header — macro definitions |
| `src/engine/log/logger.cpp` | Logging system implementation — not diagnostic output |
| `src/engine/log/log_filter.cpp` | Logging system implementation — not diagnostic output |
| `src/cmd/commands/help_command.cpp` | Intentional stdout output (user-facing help text) |
| `src/cmd/commands/version_command.cpp` | Intentional stdout output (version string) |
| `src/engine/image/image.cpp` `Image::save()` | Intentional data output (file save) |
| `src/cmd/apps/hot_reload_gltf_app.cpp` `std::ofstream` | Intentional YAML data file write |
| `src/cmd/main.cpp` pre-`Logger::init` bootstrap error | The `fprintf(stderr, "Error: %s\n", ...)` call before `Logger::init()` — must stay as `fprintf(stderr)` |
| `src/cmd/main.cpp` usage/help text blocks | Usage text for unknown command/scene (after `parse_running_args` and scene dispatch) — user-facing instructional text, not diagnostic |

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | All `std::cerr` diagnostic calls in the listed files migrate to `BUDDD_LOG_*` equivalents with identical message content. | Diff review shows no remaining `std::cerr <<` for diagnostic purposes in modified files. |
| AC-002 | All `fprintf(stderr, ...)` diagnostic calls in the listed files migrate to `BUDDD_LOG_*` equivalents, except the explicitly excluded lines in `main.cpp`. | Diff review shows no remaining `fprintf(stderr)` for diagnostic purposes in modified files (excluding exemptions). |
| AC-003 | All `printf(...)` diagnostic calls (window opened, captured, window closed) in `app.cpp` migrate to `BUDDD_LOG_INFO`. | Diff review confirms `printf` calls replaced. |
| AC-004 | All `write(STDERR_FILENO, ...)` diagnostic calls migrate to `BUDDD_LOG_*` equivalents (if any exist in scope). | Grep search confirms no `write(STDERR_FILENO` in modified files. |
| AC-005 | Each modified `.cpp` file declares `BUDDD_LOG_TAG(...)` at file scope (outside any namespace) if it does not already have one. | Diff review confirms `BUDDD_LOG_TAG` added to each modified file that lacks one. |
| AC-006 | `main.cpp` pre-`Logger::init` bootstrap error (`fprintf(stderr, "Error: %s\n", ...)`) remains unchanged. | Diff review confirms this line is not modified. |
| AC-007 | `main.cpp` usage/help text blocks (after `parse_running_args` and scene dispatch) remain as `fprintf(stderr)`. Only the error lines (e.g., "Unknown command:", "Unknown scene:") become `BUDDD_LOG_ERROR`. | Diff review confirms usage/help text blocks are unchanged; error lines use `BUDDD_LOG_ERROR`. |
| AC-008 | `main.cpp` "Warning: unexpected arguments" block (after engine init) migrates to `BUDDD_LOG_WARN`. | Diff review confirms. |
| AC-009 | `main.cpp` post-init `fprintf(stderr, "Error: %s\n", ...)` calls (after `parse_running_args` and after log-level parse) migrate to `BUDDD_LOG_ERROR`. | Diff review confirms. |
| AC-010 | `#ifndef NDEBUG`-guarded `std::cerr` calls become unconditional `BUDDD_LOG_DEBUG` calls (not gated by `#ifndef NDEBUG`). | Diff review confirms the `#ifndef NDEBUG` guard is removed and `BUDDD_LOG_DEBUG` replaces the `std::cerr`. |
| AC-011 | Each migrated call uses the correct log level per the Level Mapping table. | Diff review samples >= 10 calls across different severity categories and verifies level assignment. |
| AC-012 | Each migrated call preserves the original format string and argument expressions (only the output mechanism changes). | Diff review confirms message content is preserved character-for-character (excluding format syntax changes from `<<` to `{}`). |
| AC-013 | All `printf(...)` diagnostic calls that output to stdout now go to stderr via `BUDDD_LOG_INFO`. | Build and run a capture session; verify "Window opened", "Captured:", "Window closed" messages appear on stderr, not stdout. |
| AC-014 | The project compiles and links successfully after all changes. | Full build (`cmake --build`) succeeds with zero errors. |
| AC-015 | Running all demo apps produces equivalent diagnostic output (same messages, same conditions) as before migration. | Manual smoke test: run each demo app and verify no missing/duplicated critical messages; log output is visibly formatted as `[LEVEL] [Tag] message`. |
| AC-016 | Fatal error messages (prefixed with "FATAL:") that previously preceded `std::exit()`/`std::terminate()` now log via `BUDDD_LOG_ERROR` and the function returns/continues without terminating (unless the error is truly unrecoverable, e.g., double-free). | Code review confirms no `std::exit()` or `std::terminate()` follows a migrated "FATAL" log call. |
| AC-017 | `help_command.cpp` and `version_command.cpp` are NOT modified. | Diff review confirms zero changes to these files. |
| AC-018 | Data file writes (`hot_reload_gltf_app.cpp` `std::ofstream`, `Image::save()`) are NOT modified. | Diff review confirms zero changes to these patterns. |
| AC-019 | Logging system implementation files (`log/console_sink.cpp`, `log/file_sink.cpp`, `log/log.h`, `log/logger.cpp`, `log/log_filter.cpp`) are NOT modified. | Diff review confirms zero changes to these files. |
| AC-020 | `BUDDD_LOG_*` macros use `std::format`-style `{}` placeholders; existing `<<` stream expressions are converted to format strings. | Diff review confirms stream-style `<<` concatenation is replaced with `std::format`-compatible string (e.g., `"message: {}"`). |

## E2E Verification

This feature will be verified through:

- **Diff review**: A comprehensive `git diff` of all modified files, verifying that no ad-hoc diagnostic output remains and that all exemptions are correctly preserved.
- **Build verification**: Full CMake build of `buddd_engine` (static lib) and all `buddd` CLI targets.
- **Smoke test**: Run each demo app (`triangle`, `cube`, `textured-cube`, `phong`, `asset-demo`, `hot-reload`, `gltf`, `hot-reload-gltf`, `multi-material`) with `--capture` to exercise diagnostic paths and observe formatted log output on stderr.
- **Log level test**: Run with `--log-level=error` and verify that info/warn/debug messages are suppressed.
- **Log file test**: Run with `--log-file=/tmp/buddd.log` and verify log file contains formatted output.
- **CI**: GitHub Actions build and test suite must pass.

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | Zero `std::cerr`, `fprintf(stderr)`, `printf` (diagnostic), or `write(STDERR_FILENO)` statements remain in the modified files, except explicitly listed exemptions. |
| SC-002 | All 20 acceptance criteria pass (verified by diff review + build + smoke test). |
| SC-003 | Full project builds with zero warnings (no new warnings introduced by the migration). |
| SC-004 | All existing demo apps produce equivalent log output (same volume, same conditions, same message content) after migration. |

## Edge cases

1. **Pre-init logger not available**: `main.cpp` line 37 (bootstrapping log config parse error) must stay as raw `fprintf(stderr)` because `Logger::init()` has not been called yet. This is explicitly exempted.

2. **`Logger::is_enabled()` returns false**: The `BUDDD_LOG_*` macros already handle this — they check `is_enabled()` before calling `Logger::log()`. No special handling needed.

3. **Empty format arguments**: If the original `std::cerr << "constant string"` has no variables, the migration uses `BUDDD_LOG_INFO("constant string")` — note the lack of `\n` (the sink adds the newline).

4. **Format string conversion**: `std::cerr << "x=" << x` becomes `BUDDD_LOG_INFO("x={}", x)`. The `<<` concatenation must be converted to `std::format`-style `{}` placeholders. All arguments must be reordered into the format string.

5. **`\n` in original output**: `std::cerr << "message\n"` must drop the trailing `\n` because the `ConsoleSink` appends its own `\n`. The message content becomes `"message"` without explicit newline.

6. **Unicode characters in original messages**: Passed through as-is; the logging system does not transform encoding.

7. **`BUDDD_LOG_TAG` already declared**: Some files (e.g., `main.cpp`, `hot_reload_gltf_app.cpp`) may already have `BUDDD_LOG_TAG`. Do NOT add a second declaration.

8. **`std::cerr` used for both diagnostic and intentional output**: Only diagnostic `std::cerr` calls are migrated. If a file uses `std::cerr` for both purposes, the implementer must distinguish by context. Currently no such ambiguous cases exist.

9. **Debug-only messages in `asset_manager.tpp`**: The `#ifndef NDEBUG` guarded `std::cerr` on line 31 becomes unconditional `BUDDD_LOG_DEBUG`, relying on runtime level control instead of compile-time gating. The `Logger::is_enabled()` check runs before any formatting, so the runtime cost is negligible when debug logging is disabled (no measurable impact).

10. **Multiple `std::cerr` statements on consecutive lines**: Each statement is independently migrated to its own `BUDDD_LOG_*` call. No combining.

11. **`fprintf(stderr, ...)` with complex format strings**: The format specifiers and argument list from `fprintf` must be converted to `std::format`-style. For example, `fprintf(stderr, "x=%d y=%s\n", x, y.c_str())` becomes `BUDDD_LOG_INFO("x={} y={}", x, y)`.

## Error cases

| Scenario | Behaviour |
|---|---|
| Logger not initialized at time of migrated call | The `BUDDD_LOG_*` macro calls `Logger::instance()`, which returns the singleton. If `init()` was never called, the internal check silently drops the message — no crash. All migrated calls occur after `Logger::init()` is called in `main.cpp`, so this should not happen. |
| Pre-init error on `main.cpp:37` | Remains as `fprintf(stderr)` — cannot use the logger because it has not been initialized. |
| Format string mismatch during migration | If a `std::cerr <<` concatenation is incorrectly converted to `{}` placeholders, the compiled code will either compile with wrong output or fail to compile (type mismatch in `std::format`). This is caught at compile time or during smoke testing. |
| Memory exhaustion during log formatting | `std::format` may throw `std::bad_alloc`; this is allowed to propagate. Original code had no such risk (stream `<<` does not throw on allocation failure in practice). The risk is inherent in the logging system and is documented in SPEC-021. |

## Permissions and security

- No elevated permissions are required for the migration — it is purely a source-level transformation.
- No new file I/O is introduced beyond what the logging system's `FileSink` already provides.
- No sensitive data is logged by the migrated calls (they contain only diagnostic info like file paths, shader compilation errors, and render state).
- No network access.

## Observability

- After migration, all diagnostic output is observable through the logging system's standard mechanisms:
  - Console sink (stderr) — always active.
  - File sink — enabled via `--log-file`.
  - Level filtering — via `--log-level` and `--log-filter`.
- During migration, the implementer should verify that no diagnostic messages are lost or duplicated.
- After migration, `grep -rn 'std::cerr\|fprintf(stderr\|printf(\|write(STDERR_FILENO' src/engine/ src/cmd/` should return zero relevant diagnostic lines (excluding exemptions).

## Documentation to update

The following existing documents must be updated when this feature is implemented:

- **`docs/wiki/domain/logging.md`** — Best practice #2 ("A future feature will migrate existing ad-hoc output") must be updated to past tense ("has been migrated"). Usage patterns and tag naming conventions (including the new `Asset:FileWatcher` tag) should reflect the current state.
- **`docs/wiki/architecture/module-map.md`** — References to `std::cerr` in `render_system.cpp` (line 230) and other per-file behavior notes should be updated to reflect `BUDDD_LOG_*` equivalents.
- **`docs/wiki/domain/business-rules.md`** — The table of "Current ad-hoc diagnostic output patterns" (lines 46–61) enumerates `fprintf(stderr)`, `std::cerr`, and `printf` calls; this must be updated to reflect the structured-logging equivalents or removed if the wiki now references the logging system directly.
- **`docs/wiki/architecture/data-flow.md`** — Lines 20 and 36 reference `fprintf(stderr)` for unknown command/scene output; these should be updated to reflect the final migration state of `main.cpp`.
- **`docs/adr/ADR-020-custom-logging-system.md`** — If the ADR references ad-hoc diagnostic output as a future concern, it may need a note that the migration has been completed.

## Out of scope

- Migration of files outside `src/engine/` and `src/cmd/` (e.g., tests, third-party code, build scripts).
- Adding new log messages or diagnostic output not present before migration.
- Modifying the logging system's output format or behaviour.
- Colour/ANSI output in console sink.
- Network/remote logging.
- Asynchronous logging.
- Log file rotation or management.
- Automatic error logging of `Error`/`Result<T>` types.

## Assumptions

1. `Logger::init()` is called in `main.cpp` (line 42) before any migrated `BUDDD_LOG_*` macro is reached. This holds for all files in scope — no subsystem or app code runs before `Logger::init()`.
2. Each file has a single, stable module identity that maps to exactly one `BUDDD_LOG_TAG`. Files that serve multiple roles (e.g., `render_device.cpp` which creates both OpenGL and Headless devices) use the module-level tag (`Render`).
3. All `#ifndef NDEBUG`-guarded `std::cerr` calls are purely diagnostic and suitable for `BUDDD_LOG_DEBUG`. None contain side effects that assume compile-time removal.
4. The `std::cerr <<` stream concatenation can be mechanically converted to `std::format`-style `{}` placeholders. Where the original uses `<<` with non-string types (int, float, enum), the format string uses `{}` directly.
5. Trailing `\n` characters in original messages should be removed (the sink appends its own newline). All original messages end with `\n` explicitly or implicitly via `std::endl`.
6. All `printf(...)` calls in the modified files are diagnostic (window lifecycle, capture confirmation) and should move to stderr via `BUDDD_LOG_INFO`. There are no intentional stdout outputs in these files.
7. `help_command.cpp` and `version_command.cpp` are intentional stdout output and contain no diagnostic output. No diagnostic output needs to be migrated from these files.
8. The `main.cpp` usage/help text blocks (fprintf calls that dump multi-line usage strings) are user-facing instructional text, not diagnostic messages. The distinction is clear: the error line identifies the problem, the usage block tells the user how to use the CLI.

## Open questions

None. All decisions were confirmed during the orchestrator-human conversation documented in `coordination.md`.
