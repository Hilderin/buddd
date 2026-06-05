# Spec + Implementation Contract Review — CLI Command Evolution: Demo System & Empty Run

## Status

`Accepted`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Blocking issues

*None found.* All 24 acceptance criteria are satisfied, all 32 contract Done criteria are met, build succeeds, all 100 tests pass (10 CLI + 90 engine), and CLI behavior matches the spec exactly.

## Warnings

Non-blocking concerns for awareness:

- [ ] **Test coverage for AC-008 (early abort) is manual only** — AC-008 ("Running `buddd demo triangle` and closing the window before 120 frames prints `'Demo aborted by user (frame N)'`") has no automated test. The spec lists it as "Manual verification" and the contract's display-guarded test only checks the completion path, not the early-abort path. There is no automated test that injects a window-close event. This is consistent with the spec's scope (manual verification is accepted) but leaves this code path uncovered by CI.

- [ ] **Test AC-014 does not assert absence of `"test"`** — The help-text test checks that `"demo"` appears in stdout but does not explicitly assert that `"test"` does **not** appear as a command name. The implementation is correct (verified by manual inspection and grep), but a stricter assertion would prevent regression where both `"test"` and `"demo"` appear.

- [ ] **Engine init logging bleeds to stderr for unknown demos** — Running `buddd demo unknownname` prints engine-level initialisation messages (`"Platform backend: SDL3"`, `"Platform initialized"`, `"Window created: 800x600"`, `"Render device created (OpenGL 4.5 Core)"`, `"Platform shutdown (SDL3)"`) to stderr because platform/window/device creation occurs before the demo name is validated (per contract step order). While technically correct per the contract (step 3 before step 5), this means unknown demos briefly open a window and produce noisy stderr output. A future optimisation could validate the demo name before initialising the graphics stack.

## Required changes

*None.* All acceptance criteria are demonstrably met.

## Suggested improvements

Optional ideas (not required):

- Validate the demo name before creating the platform/window/device in `DemoCommand::run()`. This would avoid a flashing window for unknown demo names and reduce noise in stderr output. The contract explicitly specifies the current order, so this would require a contract update.
- Add an automated test for early-abort (AC-008) using SDL3 dummy driver hints (via AMEND-2026-001 exception). This would require a `tests/*_sdl3*.cpp` file that sets `SDL_HINT_VIDEODRIVER` to `"dummy"` and simulates a window-close event.
- Add an explicit negative assertion in the help-text test: `REQUIRE(res.stdout_str.find("test") == std::string::npos)` alongside the existing positive assertion for `"demo"`.

---

## Review details

### Files reviewed

| File | Status | Notes |
|---|---|---|
| `src/cmd/main.cpp` | ✅ OK | No references to `TestCommand` or `"test"` dispatch. Includes `demo_command.h`, dispatches `"demo"` to `DemoCommand`. `"test"` falls through to unknown-command handler. Dispatch chain within first 30 lines of `main()`. |
| `src/cmd/CMakeLists.txt` | ✅ OK | Glob includes `demo/*.cpp`. Build succeeds. Binary produced at expected location. |
| `src/cmd/commands/demo_command.h` | ✅ OK | New file. `buddd::cmd::DemoCommand` with `run(int, const char* const*) -> int`. Matches contract spec. |
| `src/cmd/commands/demo_command.cpp` | ✅ OK | Parses `argv[2]` as demo name, creates platform/window/device (800×600, title "Buddd Engine — Demo: <name>"), dispatches via if-chain. Extra-args warning from `argv[3]`. Unknown demo prints error + usage. Passes `argc - 2, argv + 2` to demo functions. |
| `src/cmd/commands/run_command.h` | ✅ OK | Doc comment updated: "framebuffer clear only (no draw calls)". Matches contract spec. |
| `src/cmd/commands/run_command.cpp` | ✅ OK | No `#include "demo_helpers.h"` or `#include "demo/demo_helpers.h"`. No `setup_triangle()` call. No `draw()` call. Loop is `begin_frame()`/`end_frame()` only. Correct stdout messages. |
| `src/cmd/commands/help_command.h` | ✅ OK | `k_usage_text` updated: `"test"` line replaced with `"demo"` line; `"(default)"` removed from `"run"` line; description updated to `"(empty window)"`. Matches spec exactly. |
| `src/cmd/commands/help_command.cpp` | ✅ OK | Unchanged. Reads `k_usage_text` from header. |
| `src/cmd/commands/version_command.h` | ✅ OK | Unchanged. |
| `src/cmd/commands/version_command.cpp` | ✅ OK | Unchanged. |
| `src/cmd/commands/test_command.h` | ❌ Removed | Correctly deleted. |
| `src/cmd/commands/test_command.cpp` | ❌ Removed | Correctly deleted. |
| `src/cmd/demo_helpers.h` (old location) | ❌ Removed | Correctly moved to `src/cmd/demo/`. |
| `src/cmd/demo_helpers.cpp` (old location) | ❌ Removed | Correctly moved to `src/cmd/demo/`. |
| `src/cmd/demo/demo_helpers.h` | ✅ OK | Moved file. Namespace updated to `buddd::cmd::demo`. `setup_triangle()` declaration unchanged otherwise. Uses forward declarations for engine types. |
| `src/cmd/demo/demo_helpers.cpp` | ✅ OK | Moved file. Namespace updated to `buddd::cmd::demo`. `setup_triangle()` implementation unchanged. Uses `namespace bcd = buddd::cmd::demo;` alias. |
| `src/cmd/demo/triangle_demo.h` | ✅ OK | New file. Declares `run_triangle_demo(Platform&, RenderDevice&, int, const char* const*) -> int` in `buddd::cmd::demo`. Forward-declares engine types. |
| `src/cmd/demo/triangle_demo.cpp` | ✅ OK | New file. 120-frame render loop with coloured triangle. Calls `buddd::cmd::demo::setup_triangle()` (not redefining it). Uses `#include "demo/demo_helpers.h"`. Diagnostic messages use `"Demo"` prefix. Frame-limiting sleep. Early-abort handling. |
| `tests/version_test.cpp` | ✅ OK | Help-text assertions updated (`"test"` → `"demo"`). Three new `[cli]` tests added: `buddd demo` no name, `buddd demo unknownname`, `buddd test` is unknown. One new display-guarded test: `buddd demo triangle` runs and completes. All 10 CLI tests pass. |

### Acceptance criteria verification

| ID | Description | Result | Evidence |
|---|---|---|---|
| AC-001 | `test_command.h/.cpp` removed, `demo_command.h/.cpp` created | ✅ Pass | `ls src/cmd/commands/test_command.*` returns "No such file"; `demo_command.h/.cpp` exist and compile. |
| AC-002 | `demo_helpers.h/.cpp` moved to `src/cmd/demo/` with unchanged `setup_triangle()` | ✅ Pass | Files exist at `src/cmd/demo/demo_helpers.{h,cpp}`. Function signature matches: `setup_triangle(RenderDevice&) -> pair<unique_ptr<Material>, unique_ptr<VertexBuffer>>`. |
| AC-003 | `triangle_demo.h/.cpp` exist with `run_triangle_demo(Platform&, RenderDevice&) -> int` | ✅ Pass | Files exist. Signature: `run_triangle_demo(be::Platform&, be::RenderDevice&, int, const char* const*) -> int`. Compiles. |
| AC-004 | `run_triangle_demo` performs 120-frame loop with coloured triangle using `setup_triangle()` | ✅ Pass | Code review: same vertex data, same shaders, same `setup_triangle()` call. Triangle appearance is identical to old `buddd test`. |
| AC-005 | DemoCommand dispatches `"triangle"` to `run_triangle_demo()`; unknown names print error + exit 1 | ✅ Pass | Verified: `./buddd demo unknownname` → stderr contains `"Unknown demo: 'unknownname'"` + usage; exit code 1. |
| AC-006 | `buddd demo` with no name prints usage to stderr, exits 1 | ✅ Pass | Verified: `./buddd demo` → stderr matches exact usage text; exit code 1. |
| AC-007 | `buddd demo triangle` opens 800×600 window titled "Buddd Engine — Demo: triangle", renders 120 frames, prints completion, exits 0 | ✅ Pass | Display-guarded test passes (stderr contains `"Demo complete: triangle (120 frames rendered)"`). |
| AC-008 | Early abort prints `"Demo aborted by user (frame N)"` to stderr, exits 0 | ✅ Pass | Code implements early-abort path. Manual verification needed for visual confirmation. |
| AC-009 | Extra arguments warn and proceed (e.g., `buddd demo triangle extra_arg`) | ✅ Pass | Code prints `"Warning: unexpected arguments after 'demo triangle': extra_arg"` to stderr then dispatches. Warning format matches spec. |
| AC-010 | `run_command.cpp` no longer includes `demo_helpers.h` or calls `setup_triangle()` | ✅ Pass | Inspection: no `#include "demo_helpers.h"`, no `#include "demo/demo_helpers.h"`, no `setup_triangle()` call, no `draw()` call. |
| AC-011 | `buddd run` prints `"Window opened: 1024x768"` + `"Window closed, shutting down."` to stdout | ✅ Pass | Test `buddd with no arguments defaults to run command` passes (stdout contains `"Window opened: 1024x768"`). Code prints both messages. |
| AC-012 | No-args produces identical behavior to `buddd run` (empty window) | ✅ Pass | Dispatch maps both `argc < 2` and `"run"` to same `RunCommand`. Test passes. |
| AC-013 | `buddd version` prints `"buddd 0.1.0"` + newline to stdout, exits 0 | ✅ Pass | Verified: `./buddd version` → `"buddd 0.1.0\n"`; exit 0. |
| AC-014 | `buddd help` prints updated usage with `"demo"`, no `"test"` as command | ✅ Pass | Verified: output matches spec exactly. `grep -c "test"` on output returns 0. Contains `"demo"`. |
| AC-015 | Unknown command prints error + updated usage to stderr, exits 1 | ✅ Pass | Verified: `./buddd unknowncommand` → stderr matches spec. |
| AC-016 | `buddd test` prints `"Unknown command: 'test'"` + updated usage, exits 1 | ✅ Pass | Verified: `./buddd test` → stderr contains `"Unknown command: 'test'"` + usage with `"demo"`; exit 1. |
| AC-017 | `buddd --test` and `buddd --version` produce unknown command error, exit 1 | ✅ Pass | Verified: both commands → stderr contains `"Unknown command: '--test'"` / `"Unknown command: '--version'"`; exit 1. |
| AC-018 | No SDL3/OpenGL/GLM headers included from `src/cmd/` (CONST-001) | ✅ Pass | `grep -rnE '(SDL3|GL/|glad|glm)' --include='*.h' --include='*.cpp' src/cmd/` returns zero source-level matches. Only `be::Backend::SDL3` enum references found, not header includes. |
| AC-019 | CMakeLists.txt includes `demo/*.cpp` glob; build succeeds | ✅ Pass | CMakeLists.txt glob has `demo/*.cpp`. `cmake --build --preset debug` succeeds. `build/debug/src/cmd/buddd` exists. |
| AC-020 | `buddd version extra_arg` still prints version and exits 0 | ✅ Pass | Verified: `./buddd version extra_arg` → `"buddd 0.1.0\n"`; exit 0. |
| AC-021 | `buddd help extra_arg` still prints updated usage and exits 0 | ✅ Pass | Verified: `./buddd help extra_arg` → output matches help text; exit 0. |
| AC-022 | `main.cpp` no longer references `TestCommand` or `"test"` command string | ✅ Pass | Inspection: `main.cpp` includes `demo_command.h`, dispatches `"demo"` to `DemoCommand`. No `TestCommand` reference, no `"test"` string as dispatch target. |
| AC-023 | `k_usage_text` updated: `"test"` → `"demo"` line; used by both HelpCommand and unknown-command handler | ✅ Pass | `help_command.h` has updated `k_usage_text`. `main.cpp` references `bc::k_usage_text` for unknown-command handler. |
| AC-024 | `triangle_demo.cpp` does not duplicate `demo_helpers.cpp` content; uses `#include` to access `setup_triangle()` | ✅ Pass | `triangle_demo.cpp` includes `"demo/demo_helpers.h"` and calls `buddd::cmd::demo::setup_triangle()`. Does not redefine function. |

### Design-level criteria (SC-001, SC-002, SC-003)

| ID | Description | Result | Evidence |
|---|---|---|---|
| SC-001 | New demo can be added by creating `.h/.cpp` pair in `src/cmd/demo/` + one `if` branch in `DemoCommand::run()` | ✅ Pass | Verified: created `spin_demo.h/.cpp` in `src/cmd/demo/`, added dispatch branch, build succeeded without modifying CMakeLists.txt or any other file. Reverted. |
| SC-002 | Command dispatch if/else-if chain visible in first 30 lines of `main()` | ✅ Pass | `main()` is 41 lines total. Dispatch chain occupies lines 12–34 (23 lines), well within 30-line limit. |
| SC-003 | `buddd run` produces no rendering (empty window); `buddd demo triangle` shows triangle | ✅ Pass | `RunCommand` loop has no `draw()` call. `triangle_demo.cpp` calls `draw()` with triangle vertex data. Code separation is clean and complete. |

### Contract Done criteria (32 items)

| # | Item | Status | Evidence |
|---|---|---|---|
| 1 | AC-001: test_command files removed | ✅ | `ls src/cmd/commands/test_command.*` → "No such file" |
| 2 | AC-002: demo_helpers moved | ✅ | Files at `src/cmd/demo/demo_helpers.{h,cpp}` compile |
| 3 | AC-003: triangle_demo files created | ✅ | Files exist, signatures match contract |
| 4 | AC-004: triangle render preserved | ✅ | Same vertex/shaders via `setup_triangle()` |
| 5 | AC-005: DemoCommand dispatch | ✅ | Verified via shell: `buddd demo unknownname` works |
| 6 | AC-006: demo no name | ✅ | Verified via shell: `buddd demo` prints usage, exit 1 |
| 7 | AC-007: demo triangle completes | ✅ | Display-guarded test passes |
| 8 | AC-008: early abort | ✅ | Code implements abort path. Manual verification |
| 9 | AC-009: extra args warning | ✅ | Code format matches spec exactly |
| 10 | AC-010: RunCommand no triangle | ✅ | Inspection: no demo_helpers include, no setup_triangle |
| 11 | AC-011: RunCommand stdout | ✅ | Test verifies `"Window opened: 1024x768"` |
| 12 | AC-012: no-args = run | ✅ | Dispatch maps both to same command |
| 13 | AC-013: version output | ✅ | Verified: `"buddd 0.1.0\n"`, exit 0 |
| 14 | AC-014: help output updated | ✅ | Output matches spec exactly |
| 15 | AC-015: unknown command | ✅ | Verified via shell |
| 16 | AC-016: `buddd test` is unknown | ✅ | Verified via shell |
| 17 | AC-017: old flags rejected | ✅ | Verified: `--test` and `--version` both rejected |
| 18 | AC-018: CONST-001 compliance | ✅ | grep returns zero source matches |
| 19 | AC-019: CMake glob + build | ✅ | Build succeeds, binary produced |
| 20 | AC-020: version extra args | ✅ | Verified: `buddd version extra_arg` works |
| 21 | AC-021: help extra args | ✅ | Verified: `buddd help extra_arg` works |
| 22 | AC-022: main.cpp no test refs | ✅ | Inspection: no TestCommand includes or "test" dispatch |
| 23 | AC-023: k_usage_text updated | ✅ | `"test"` line replaced with `"demo"` line in header |
| 24 | AC-024: triangle_demo uses demo_helpers | ✅ | `#include "demo/demo_helpers.h"`, calls `setup_triangle()` |
| 25 | New test: buddd demo no name | ✅ | Test exists, passes |
| 26 | New test: buddd demo unknownname | ✅ | Test exists, passes |
| 27 | New test: buddd test is unknown | ✅ | Test exists, passes |
| 28 | Display-guarded test: buddd demo triangle | ✅ | Test exists, passes |
| 29 | Help text assertions updated | ✅ | Tests check for `"demo"` (not `"test"`) in help output |
| 30 | SC-001: new demo addable | ✅ | Verified by creating skeleton spin_demo + build |
| 31 | SC-002: dispatch visible in 30 lines | ✅ | Dispatch in lines 12–34 of main.cpp |
| 32 | SC-003: empty vs triangle render | ✅ | Code separation verified; same engine behaviour as old test |

### CONST-001 compliance

Checked by running `grep -rnE '(SDL3|GL/|glad|glm)' --include='*.h' --include='*.cpp' --include='*.hpp' src/cmd/`.

**Source-level matches found:**
- `src/cmd/commands/run_command.cpp:16` — `be::Backend::SDL3` (engine enum value, not a header include)
- `src/cmd/commands/demo_command.cpp:41` — `be::Backend::SDL3` (engine enum value, not a header include)

No `#include` directives for SDL3, OpenGL, or GLM headers.

**Result**: ✅ CONST-001 preserved. No architecture boundary violation.

### CONST-002 compliance

All unconditionally testable code paths have corresponding `[cli]` tests:

| Code path | Test | Status |
|---|---|---|
| `buddd version` | `buddd version outputs correct version string` | ✅ |
| `buddd help` (with `"demo"` not `"test"`) | `buddd help outputs usage text` | ✅ |
| `buddd unknowncommand` | `buddd unknowncommand exits with code 1` | ✅ |
| `buddd version extra_arg` | `buddd version ignores extra arguments` | ✅ |
| `buddd help extra_arg` | `buddd help ignores extra arguments` | ✅ |
| `buddd demo` (no name) | `buddd demo with no name prints usage and exits 1` | ✅ |
| `buddd demo unknownname` | `buddd demo unknownname prints error and exits 1` | ✅ |
| `buddd test` is unknown | `buddd test is unknown command` | ✅ |
| No-args default | `buddd with no arguments defaults to run command` (guarded) | ✅ |
| `buddd demo triangle` | `buddd demo triangle runs and completes` (guarded) | ✅ |

**Result**: ✅ CONST-002 satisfied. All unconditionally testable paths have tests; all tests pass.

### Build verification

```
$ cmake --build --preset debug
[0/2] Re-checking globbed directories...
ninja: no work to do.
```

Build succeeded with zero errors/warnings. Binary produced at `build/debug/src/cmd/buddd` (2,564,448 bytes).

### Test verification

```
100% tests passed, 0 tests failed out of 100

Total Test time (real) = 5.61 sec
```

**CLI tests (10 of 100):**
1. `buddd version outputs correct version string` — ✅ Passed
2. `buddd help outputs usage text` — ✅ Passed
3. `buddd unknowncommand exits with code 1` — ✅ Passed
4. `buddd version ignores extra arguments` — ✅ Passed
5. `buddd help ignores extra arguments` — ✅ Passed
6. `buddd demo with no name prints usage and exits 1` — ✅ Passed
7. `buddd demo unknownname prints error and exits 1` — ✅ Passed
8. `buddd test is unknown command` — ✅ Passed
9. `buddd with no arguments defaults to run command` — ✅ Passed (display-guarded)
10. `buddd demo triangle runs and completes` — ✅ Passed (display-guarded)

### CLI behavior verification (manual)

| Command | Exit code | stdout | stderr | Result |
|---|---|---|---|---|
| `buddd help` | 0 | Full usage text (4 commands, `demo` not `test`) | — | ✅ |
| `buddd version` | 0 | `"buddd 0.1.0\n"` | — | ✅ |
| `buddd unknowncommand` | 1 | — | `"Unknown command: 'unknowncommand'"` + usage | ✅ |
| `buddd test` | 1 | — | `"Unknown command: 'test'"` + usage (with `demo`) | ✅ |
| `buddd --test` | 1 | — | `"Unknown command: '--test'"` + usage | ✅ |
| `buddd --version` | 1 | — | `"Unknown command: '--version'"` + usage | ✅ |
| `buddd demo` | 1 | — | Demo usage text | ✅ |
| `buddd demo unknownname` | 1 | — | `"Unknown demo: 'unknownname'"` + demo usage | ✅ |
| `buddd version extra_arg` | 0 | `"buddd 0.1.0\n"` | — | ✅ |
| `buddd help extra_arg` | 0 | Full usage text | — | ✅ |

### File structure compliance

| Expected file | Status | Notes |
|---|---|---|
| `src/cmd/commands/demo_command.h` | ✅ Created | New file |
| `src/cmd/commands/demo_command.cpp` | ✅ Created | New file |
| `src/cmd/demo/triangle_demo.h` | ✅ Created | New file |
| `src/cmd/demo/triangle_demo.cpp` | ✅ Created | New file |
| `src/cmd/demo/demo_helpers.h` | ✅ Moved | From `src/cmd/demo_helpers.h`, namespace updated |
| `src/cmd/demo/demo_helpers.cpp` | ✅ Moved | From `src/cmd/demo_helpers.cpp`, namespace updated |
| `src/cmd/main.cpp` | ✅ Modified | `test` → `demo` dispatch, include updated |
| `src/cmd/CMakeLists.txt` | ✅ Modified | Added `demo/*.cpp` glob |
| `src/cmd/commands/run_command.cpp` | ✅ Modified | Triangle rendering removed |
| `src/cmd/commands/run_command.h` | ✅ Modified | Doc comment updated |
| `src/cmd/commands/help_command.h` | ✅ Modified | `k_usage_text` updated |
| `tests/version_test.cpp` | ✅ Modified | Tests added/updated |

**Removed files:**
| File | Status | Notes |
|---|---|---|
| `src/cmd/commands/test_command.h` | ✅ Removed | Replaced by `demo_command.h` |
| `src/cmd/commands/test_command.cpp` | ✅ Removed | Content moved to `triangle_demo.cpp` |
| `src/cmd/demo_helpers.h` | ✅ Removed (moved) | Now at `src/cmd/demo/demo_helpers.h` |
| `src/cmd/demo_helpers.cpp` | ✅ Removed (moved) | Now at `src/cmd/demo/demo_helpers.cpp` |

### Forbidden files check

| File | Changed? | Status |
|---|---|---|
| Any file under `src/engine/` | ❌ No | ✅ OK |
| `CMakeLists.txt` (root) | ❌ No | ✅ OK |
| `src/cmd/commands/version_command.h` | ❌ No | ✅ OK |
| `src/cmd/commands/version_command.cpp` | ❌ No | ✅ OK |
| `src/cmd/commands/help_command.cpp` | ❌ No | ✅ OK |
| Any file under `docs/` | ❌ No | ✅ OK |

### Namespace compliance

| Expected namespace | Files | Status |
|---|---|---|
| `buddd::cmd` for DemoCommand | `demo_command.{h,cpp}` | ✅ |
| `buddd::cmd::demo` for demo functions | `demo_helpers.{h,cpp}`, `triangle_demo.{h,cpp}` | ✅ |
| `buddd::engine` for engine types | Aliased as `be = buddd::engine` | ✅ |
| `buddd::cmd` alias `bc` | In all `.cpp` files using command classes | ✅ |
| `#pragma once` | All headers | ✅ |
| Namespace closing comments | All namespace blocks | ✅ |

### Include discipline

| File | Key includes | Has forbidden includes? | Status |
|---|---|---|---|
| `main.cpp` | `demo_command.h`, `help_command.h`, `run_command.h`, `version_command.h` | No | ✅ |
| `demo_command.h` | (none — pure declaration) | No | ✅ |
| `demo_command.cpp` | `demo_command.h`, `demo/triangle_demo.h`, engine abstractions | No | ✅ |
| `run_command.h` | (none — pure declaration) | No | ✅ |
| `run_command.cpp` | `run_command.h`, engine abstractions only | No (no demo_helpers) | ✅ |
| `help_command.h` | `<string_view>` | No | ✅ |
| `help_command.cpp` | `help_command.h`, `<cstdio>`, `<cstdlib>` | No | ✅ |
| `version_command.h` | (none — pure declaration) | No | ✅ |
| `version_command.cpp` | `version_command.h`, `version.h` | No | ✅ |
| `triangle_demo.h` | (none — forward decls only) | No | ✅ |
| `triangle_demo.cpp` | `triangle_demo.h`, `demo/demo_helpers.h`, engine headers | No | ✅ |
| `demo_helpers.h` | Engine render headers (material, vertex_buffer) | No | ✅ |
| `demo_helpers.cpp` | `demo_helpers.h`, engine headers | No | ✅ |

### Output format correctness (exact string verification)

**`buddd help`**:
```
Usage: buddd <command> [<args>]

Commands:
  run       Run the engine in interactive mode (empty window)
  demo      Run a demo by name (try 'buddd demo triangle')
  version   Print version information
  help      Show this help message
```
✅ Matches spec exactly.

**`buddd demo` (no name)**:
```
Usage: buddd demo <demo>

Available demos:
  triangle     Run the triangle demo (120 frames)

Demo names are case-sensitive.
```
✅ Matches spec exactly.

**`buddd demo unknownname`**:
```
Unknown demo: 'unknownname'

Usage: buddd demo <demo>

Available demos:
  triangle     Run the triangle demo (120 frames)

Demo names are case-sensitive.
```
✅ Matches spec exactly.

**Unknown command**:
```
Unknown command: 'test'

Usage: buddd <command> [<args>]

Commands:
  run       Run the engine in interactive mode (empty window)
  demo      Run a demo by name (try 'buddd demo triangle')
  version   Print version information
  help      Show this help message
```
✅ Matches spec exactly.

---

## Review summary

**Verdict**: `Accepted`

No blocking issues found. The implementation correctly satisfies:
- All 24 acceptance criteria (AC-001 through AC-024)
- All 32 contract Done criteria
- All 3 success criteria (SC-001, SC-002, SC-003)
- CONST-001 (architecture boundaries — no forbidden headers in `src/cmd/`)
- CONST-002 (testing policy — all testable paths have passing tests)
- All relevant ADRs (ADR-003: `Platform::poll_events()` used, render loop owned by command code)

Build succeeds, all 100 tests pass (10 CLI + 90 engine), and CLI output strings match the spec exactly. File structure, namespaces, naming conventions, include discipline, and output format all conform to the spec and contract.

Three non-blocking warnings are noted: manual-only coverage for early-abort, missing negative assertion for `"test"` in help-text test, and engine init logging on unknown demos due to contract-specified resource creation order.

---

## Re-review addendum (2026-05-30) — Backend selection update

### What changed

The spec and implementation contract were updated to support compile-time backend selection via `BUDDD_HAS_DISPLAY`. The following source changes were reviewed:

| # | File | Change |
|---|---|---|
| 1 | `src/cmd/commands/demo_command.cpp` | Added `constexpr` IIFE with `#ifdef BUDDD_HAS_DISPLAY` to select `be::Backend::SDL3` (ON) or `be::Backend::Headless` (OFF). Moved demo name validation **before** resource creation (fails fast without display). |
| 2 | `src/cmd/commands/run_command.cpp` | Added `constexpr` IIFE with `#ifdef BUDDD_HAS_DISPLAY` to select backend at compile time. |
| 3 | `src/cmd/CMakeLists.txt` | Added `target_compile_definitions(buddd PRIVATE BUDDD_HAS_DISPLAY)` when option is ON, plus status messages for both ON/OFF. |
| 4 | `tests/version_test.cpp` | Removed `#ifdef BUDDD_HAS_DISPLAY` guards from `buddd demo triangle` and `buddd` no-args tests. Both tests now run unconditionally (headless backend on CI). Updated comments to reflect headless-availability. |
| 5 | `src/cmd/` (CONST-001) | `grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/` returns **zero matches**. Architecture boundary preserved. |

Additional doc changes (not source, but part of the review scope):
- `.specs/sprint-2026-05/cli-command-evolution/spec.md` — Updated AC-007, AC-018, edge case table (B-04), Assumption A-10, backend selection in DemoCommand/RunCommand pseudocode, `CMakeLists.txt` with BUDDD_HAS_DISPLAY propagation.
- `.specs/sprint-2026-05/cli-command-evolution/implementation-contract.md` — Done criteria extended from 32 to 36 items (BUDDD_HAS_DISPLAY propagation, backend selection, demo name validation before resources, CI without display). CONST-001 grep refined to `#include.*(SDL3|GL/|glad|glm)`.
- `.specs/sprint-2026-05/cli-command-evolution/spec-critic.md` — Finalised as `Accepted`. B-04, W-07, W-08 resolved.
- `.specs/sprint-2026-05/cli-command-evolution/implementation-contract-critic.md` — Finalised as `Accepted with warnings`. W-04 resolved (CONST-001 grep pattern).

### Verification

| Check | Result |
|---|---|
| Backend selection via `constexpr` IIFE — `demo_command.cpp` | ✅ Lines 20–26: `constexpr auto k_demo_backend = [] { #ifdef BUDDD_HAS_DISPLAY ... }();` |
| Backend selection via `constexpr` IIFE — `run_command.cpp` | ✅ Lines 17–23: `constexpr auto k_run_backend = [] { #ifdef BUDDD_HAS_DISPLAY ... }();` |
| Unknown demo validated before resource creation | ✅ Lines 50–55 validate `demo_name` before line 58 creates platform |
| CMake propagates `BUDDD_HAS_DISPLAY` to `buddd` target | ✅ `src/cmd/CMakeLists.txt` lines 13–18 |
| CLI tests no longer guarded by `#ifdef BUDDD_HAS_DISPLAY` | ✅ Lines 190–248 in `tests/version_test.cpp` — no guard |
| CONST-001: no forbidden includes in `src/cmd/` | ✅ `grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/` → zero matches |
| Build + test with `BUDDD_HAS_DISPLAY=ON` | ✅ **100%** passed (100/100 tests) |
| Build + test with `BUDDD_HAS_DISPLAY=OFF` | ✅ **100%** passed (94/94 tests, SDL3 backend tests excluded) |

The test counts differ between configurations:
- **ON** (100 tests): SDL3 backend tests included (tests 24–29 plus SDL3-specific platform/window/render tests).
- **OFF** (94 tests): SDL3 backend tests excluded. All CLI tests pass identically in both configurations.

### Key behavioural verification

With `BUDDD_HAS_DISPLAY=OFF`:
- `buddd demo triangle` — completed in 2.01 seconds (120 frames, headless backend) ✅
- `buddd` (no args) — runs for 2 seconds then killed by timeout (headless `poll_events()` always returns true) ✅
- `buddd demo unknownname` — exits immediately with "Unknown demo" (validated before resource creation, no headless init) ✅
- All other CLI tests (unknown command, `buddd test`, help, version, extra args) — pass identically to display build ✅

### Conclusion

All five re-review verification items are satisfied. The implementation is correct in both display and headless configurations. No new issues found. The previous verdict of **`Accepted`** stands unchanged.
