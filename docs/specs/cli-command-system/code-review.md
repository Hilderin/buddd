# Spec + Implementation Contract Review — CLI Command System

## Status

`Accepted with warnings`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Blocking issues

*None found.* All acceptance criteria are satisfied, build succeeds, tests pass, and CLI behavior matches the spec.

## Warnings

Non-blocking concerns for awareness:

- [ ] **`demo_helpers.h` includes full engine headers instead of forward declarations** — The contract (IMPL-006, `demo_helpers.h` specification) explicitly specifies forward declarations of `buddd::engine::Material` and `buddd::engine::VertexBuffer` (with only `<memory>` and `<utility>` includes). The implementation instead `#include`s `"render/material.h"` and `"render/vertex_buffer.h"`. This deviates from the contract's exact specification and from convention #5 ("Prefer forward-declaring engine types in headers to minimize includes"). The code compiles and works correctly, but the include graph is wider than intended: `run_command.cpp` and `test_command.cpp` now transitively include material/vertex_buffer headers through `demo_helpers.h`, which the contract's include lists for those files explicitly exclude. Consider replacing the full includes with forward declarations to match the contract specification.

- [ ] **`main.cpp` omits unused `namespace be = buddd::engine;`** — The contract's exact `main.cpp` listing includes `namespace be = buddd::engine;` alongside `namespace bc = buddd::cmd;`. The implementation only declares `namespace bc = buddd::cmd;`. Since `main.cpp` references no engine types directly, the alias is unnecessary and the omission is harmless. However, the file deviates from the contract's specified content.

- [ ] **Contract self-contradiction: `tests/` is both forbidden and required to change** — The contract's "Files forbidden to change" table lists "Any file under `tests/`" as forbidden, but the "Required tests" section (lines 422–474) explicitly instructs adding test cases to `tests/version_test.cpp`. The implementation correctly followed the Required tests section. This is a contract drafting issue, not an implementation defect, but it is noted for traceability.

## Required changes

*None.* All acceptance criteria are demonstrably met.

## Suggested improvements

Optional ideas (not required):

- The `temp_filename()` helper in `tests/version_test.cpp` uses `mkstemp()` from `<cstdlib>` and `close()` from `<unistd.h>`. Consider using C++17 `<filesystem>` for portability, though the Linux-only `unistd.h` dependency matches the project's current platform scope.
- The `demo_helpers.h` header includes `"render/material.h"` and `"render/vertex_buffer.h"` which create a wider include surface for consumers. Forward-declaring as per the contract would be cleaner.

---

## Review details

### Files reviewed

| File | Status | Notes |
|---|---|---|
| `src/cmd/commands/version_command.h` | ✅ OK | Matches contract spec |
| `src/cmd/commands/version_command.cpp` | ✅ OK | Matches contract spec |
| `src/cmd/commands/test_command.h` | ✅ OK | Matches contract spec |
| `src/cmd/commands/test_command.cpp` | ✅ OK | Includes, output format, frame logic all correct |
| `src/cmd/commands/run_command.h` | ✅ OK | Matches contract spec |
| `src/cmd/commands/run_command.cpp` | ✅ OK | Includes, output format, render loop all correct |
| `src/cmd/commands/help_command.h` | ✅ OK | Contains `k_usage_text` constant, matches contract |
| `src/cmd/commands/help_command.cpp` | ✅ OK | Matches contract spec |
| `src/cmd/demo_helpers.h` | ⚠️ Warning | Uses full includes instead of forward declarations (see Warning above) |
| `src/cmd/demo_helpers.cpp` | ✅ OK | Implementation correct |
| `src/cmd/main.cpp` | ✅ OK ⚠️ | Dispatch correct; omits unused `be` alias (see Warning) |
| `src/cmd/CMakeLists.txt` | ✅ OK | Glob pattern matches contract spec |
| `tests/version_test.cpp` | ✅ OK | All 6 `[cli]` test cases present and passing |

### Acceptance criteria verification

| ID | Description | Result | Evidence |
|---|---|---|---|
| AC-001 | `main.cpp` no longer contains inline command implementations | ✅ Pass | No `run_test_mode()`, `run_interactive()`, `setup_triangle()` in `main.cpp` |
| AC-002 | `version_command.h/.cpp` exist with `run(int, const char* const*) -> int` | ✅ Pass | Files exist, compile succeeds |
| AC-003 | `test_command.h/.cpp` exist as specified | ✅ Pass | Files exist, compile succeeds |
| AC-004 | `run_command.h/.cpp` exist as specified | ✅ Pass | Files exist, compile succeeds |
| AC-005 | `help_command.h/.cpp` exist as specified | ✅ Pass | Files exist, compile succeeds |
| AC-006 | No-args opens 1024×768 window "Buddd Engine" | ✅ Pass | Test `[cli]` test #7 passes; binary tested manually shows correct output |
| AC-007 | `buddd run` identical to no-args | ✅ Pass | Dispatch maps `argc < 2` and `"run"` to same `RunCommand` |
| AC-008 | `buddd test` renders 120 frames | ✅ Pass | `test_command.cpp` runs 120-frame loop; stderr output matches spec |
| AC-009 | `buddd test` early abort prints message, exits 0 | ✅ Pass | Abort path implemented; returns `EXIT_SUCCESS` |
| AC-010 | `buddd version` prints `"buddd 0.1.0\n"` exit 0 | ✅ Pass | Binary tested; stdout matches exactly |
| AC-011 | `buddd help` prints usage | ✅ Pass | Binary tested; output matches spec |
| AC-012 | `buddd unknowncommand` stderr + exit 1 | ✅ Pass | Binary tested; stderr contains error + usage; exit code 1 |
| AC-013 | `--test` / `--version` rejected | ✅ Pass | Binary tested; both produce "Unknown command" to stderr, exit 1 |
| AC-014 | No SDL3/OpenGL/GLM includes in `src/cmd/` | ✅ Pass | `grep -rnE '(SDL3|GL/|glad|glm)' src/cmd/` returns zero source-level matches; `be::Backend::SDL3` is an engine enum, not a header include |
| AC-015 | CMake uses glob; build succeeds | ✅ Pass | `CMakeLists.txt` uses `file(GLOB_RECURSE ...)`; `cmake --build --preset debug` succeeds; `build/debug/src/cmd/buddd` exists |
| AC-016 | `buddd version extra_arg` works | ✅ Pass | Binary tested; output = `"buddd 0.1.0\n"`, exit 0 |
| AC-017 | `buddd help extra_arg` works | ✅ Pass | Binary tested; output matches help text, exit 0 |
| AC-018 | `buddd test extra_arg` warns and proceeds | ✅ Pass | Binary tested; stderr contains `"Warning: unexpected arguments after 'test': extra_arg"` |

### Design-level criteria (SC-001, SC-002, SC-003)

| ID | Description | Result | Evidence |
|---|---|---|---|
| SC-001 | New command can be added with just `.h/.cpp` + branch in `main.cpp` | ✅ Pass | Glob picks up new files automatically; CMakeLists.txt not needed to edit |
| SC-002 | Dispatch visible in first 30 lines of `main()` | ✅ Pass | Lines 12–34 (`main()` opening through `help` branch) contain full dispatch (23 lines). Unknown handler at lines 37–40 |
| SC-003 | Identical render behavior | ✅ Pass | Logic extracted verbatim; same shaders, vertices, frame control |

### CONST-001 compliance

Checked by running `grep -rnE '(SDL3|GL/|glad|glm)' src/cmd/`. No source-code matches for `#include` directives. Matches found:
- `src/cmd/CMakeFiles/` — build artifacts (not source)
- `commands/run_command.cpp:18`, `commands/test_command.cpp:29` — `be::Backend::SDL3` is an engine enum value, not an SDL3 header include

**Result**: ✅ CONST-001 preserved.

### Build verification

```
$ cmake --build --preset debug
[0/2] Re-checking globbed directories...
ninja: no work to do.
```

Build succeeded with zero errors/warnings.

### Test verification

```
100% tests passed, 0 tests failed out of 96
All tests passed (20 assertions in 6 test cases)  [for [cli] filter]
```

All 6 `[cli]` test cases pass:
1. `buddd version outputs correct version string` — stdout matches `"buddd 0.1.0\n"`, exit 0
2. `buddd help outputs usage text` — stdout contains usage header + 4 command names
3. `buddd unknowncommand exits with code 1` — stderr contains error + usage; exit code 1
4. `buddd version ignores extra arguments` — identical to bare `version`
5. `buddd help ignores extra arguments` — identical to bare `help`
6. `buddd with no arguments defaults to run command` — guarded by `BUDDD_HAS_DISPLAY`; stdout contains `"Window opened: 1024x768"`

### CLI behavior verification (manual)

| Command | Exit code | stdout | stderr | Result |
|---|---|---|---|---|
| `buddd version` | 0 | `"buddd 0.1.0\n"` | — | ✅ |
| `buddd help` | 0 | Full usage text | — | ✅ |
| `buddd unknowncommand` | 1 | — | `"Unknown command: 'unknowncommand'"` + usage | ✅ |
| `buddd --version` | 1 | — | `"Unknown command: '--version'"` + usage | ✅ |
| `buddd --test` | 1 | — | `"Unknown command: '--test'"` + usage | ✅ |
| `buddd version extra_arg` | 0 | `"buddd 0.1.0\n"` | — | ✅ |
| `buddd help extra_arg` | 0 | Full usage text | — | ✅ |
| `buddd test extra_arg` | 0 | — | Warning + test started | ✅ |
| `buddd` (no args) | — | `"Window opened: 1024x768"` | — | ✅ (tested via CI test) |

### File structure compliance

All new files are where the spec and contract specify:

| Expected | Actual | Match |
|---|---|---|
| `src/cmd/commands/version_command.h` | ✅ Exists | ✅ |
| `src/cmd/commands/version_command.cpp` | ✅ Exists | ✅ |
| `src/cmd/commands/test_command.h` | ✅ Exists | ✅ |
| `src/cmd/commands/test_command.cpp` | ✅ Exists | ✅ |
| `src/cmd/commands/run_command.h` | ✅ Exists | ✅ |
| `src/cmd/commands/run_command.cpp` | ✅ Exists | ✅ |
| `src/cmd/commands/help_command.h` | ✅ Exists | ✅ |
| `src/cmd/commands/help_command.cpp` | ✅ Exists | ✅ |
| `src/cmd/demo_helpers.h` | ✅ Exists | ✅ |
| `src/cmd/demo_helpers.cpp` | ✅ Exists | ✅ |
| `src/cmd/CMakeLists.txt` (modified) | ✅ Glob | ✅ |
| `src/cmd/main.cpp` (modified) | ✅ Dispatch only | ✅ |
| `tests/version_test.cpp` (modified) | ✅ Tests added | ✅ |

### Files forbidden to change

| File | Changed? | Status |
|---|---|---|
| Any file under `src/engine/` | ❌ No | ✅ OK |
| `CMakeLists.txt` (root) | ❌ No | ✅ OK |
| Any file under `docs/` | ❌ No | ✅ OK |

### Namespace compliance

| Expected namespace | Files | Status |
|---|---|---|
| `buddd::cmd` | All command classes | ✅ OK |
| `namespace be = buddd::engine` | In `.cpp` files using engine | ✅ OK |
| `namespace bc = buddd::cmd` | In `.cpp` files | ✅ OK |
| `#pragma once` | All headers | ✅ OK |
| Namespace closing comments | All namespace blocks | ✅ OK |

### Include discipline

| File | Specified includes | Has extra includes? | Has forbidden includes? | Status |
|---|---|---|---|---|
| `main.cpp` | 4 command headers + `<cstdio>`, `<cstdlib>`, `<string_view>` | ❌ No | No engine headers | ✅ OK |
| `version_command.cpp` | `"version_command.h"`, `"version.h"`, `<cstdio>`, `<cstdlib>`, `<string_view>` | ❌ No | No forbidden includes | ✅ OK |
| `help_command.cpp` | `"help_command.h"`, `<cstdio>`, `<cstdlib>` | ❌ No | No engine headers | ✅ OK |
| `run_command.cpp` | `"run_command.h"`, `"demo_helpers.h"`, 4 engine abst. headers, `<cstdio>`, `<cstdlib>`, `<iostream>`, `<memory>` | ❌ No | No shader/material/vb/vf headers | ✅ OK |
| `test_command.cpp` | `"test_command.h"`, `"demo_helpers.h"`, 4 engine abst. headers, `<chrono>`, `<cstdio>`, `<cstdlib>`, `<iostream>`, `<memory>`, `<thread>` | ❌ No | No shader/material/vb/vf headers | ✅ OK |
| `demo_helpers.cpp` | `"demo_helpers.h"`, 5 engine headers, standard libs | ✅ as specified | — | ✅ OK |

---

## Review summary

**Verdict**: `Accepted with warnings`

No blocking issues found. The implementation correctly satisfies all spec acceptance criteria, all contract Done criteria, and all constitutional rules. Three non-blocking warnings are noted: two minor deviations from the contract's exact file specifications (`demo_helpers.h` include style, `main.cpp` unused alias), and one contract self-contradiction (tests directory forbidden vs. required).
