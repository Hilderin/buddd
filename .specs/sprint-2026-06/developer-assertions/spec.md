# SPEC-023 — Developer Assertions

## Problem

The Buddd Engine has no structured way to check internal invariants during development. Without assertions, invalid states propagate silently, producing hard-to-debug symptoms far from the root cause. Existing engine code uses ad-hoc checks or assumes correctness without validation. A lightweight assertion system is needed to:

- Catch programming errors early in debug/development builds.
- Provide consistent, readable failure reports (expression, location, custom message).
- Integrate with the existing `buddd::log` logging system instead of introducing a separate reporting channel.
- Be compilable out of release builds so there is zero overhead in shipped code.
- Support both fatal (always-active) and verify (side-effect-preserving) patterns.

## Goals

- Provide five assertion macros (`BUDDD_ASSERT`, `BUDDD_ASSERT_MSG`, `BUDDD_VERIFY`, `BUDDD_FAIL`, `BUDDD_FAIL_MSG`) as the primary API.
- Evaluate `BUDDD_ASSERT` / `BUDDD_ASSERT_MSG` expressions in debug builds only; compile them out entirely (including expression evaluation) in release builds.
- Evaluate `BUDDD_VERIFY` expressions in all builds (to preserve side effects), but only abort on failure in debug builds.
- Evaluate `BUDDD_FAIL` / `BUDDD_FAIL_MSG` unconditionally in all builds.
- Log assertion failures at `Fatal` level via the existing `buddd::log::Logger`, using the fixed tag `"Assert"`.
- Include expression text, source file, line number, function name, and optional formatted message in the failure report.
- Trigger a debugger break via `buddd::engine::debug_break()` when a debugger is attached (in non-NDEBUG builds).
- Call `std::abort()` after logging and break in debug builds; in release builds, Fatal log + abort for `FAIL`/`FAIL_MSG`, Fatal log only (no abort) for `VERIFY`.
- Add `LogLevel::Fatal` to the logging system (above `Error`) to support assertion failure reporting.
- Provide a `handle_assertion_failure()` function whose formatting/reporting logic can be unit tested without triggering `std::abort()`.

## Non-goals

- Crash reporting / minidump generation (Breakpad, etc.).
- Remote crash or error telemetry.
- In-editor assertion popup or modal dialog.
- Per-assertion "ignore always" or assertion ignore lists.
- Crash dump file creation or upload.
- Symbolication or stack trace generation beyond `__FUNCTION__`.
- Exception handling framework or `try`/`catch` recovery from assertion failures.
- Migration of existing ad-hoc checks to the new assertion macros (deferred to future adoption task).
- Any mechanism to suppress assertions at runtime (assertions are always-on in debug builds).

## Actors

| Actor | Interaction |
|---|---|
| **Engine developer** | Writes `BUDDD_ASSERT` / `BUDDD_ASSERT_MSG` / `BUDDD_VERIFY` / `BUDDD_FAIL` / `BUDDD_FAIL_MSG` in engine source code to check invariants. |
| **Debugger operator** | Attaches a debugger; assertion failures trigger a break at the failure site for interactive inspection. |
| **Test author** | Writes Catch2 tests that verify assertion formatting, `BUDDD_VERIFY` expression evaluation, and non-double-evaluation of `BUDDD_ASSERT`. |
| **CI pipeline** | Runs test builds (both debug and release) to confirm assertion macros compile, behave correctly, and do not cause abort in test-safe code paths. |

## User-visible behavior

1. Any engine code that includes `src/engine/debug/assert.h` gains access to the five assertion macros.
2. `BUDDD_ASSERT(expr)` in a debug build: evaluates `expr`. If false, logs a `Fatal`-level message tagged `[Assert]`, calls `debug_break()`, then `std::abort()`.
3. `BUDDD_ASSERT(expr)` in a release build (`NDEBUG` defined): the macro expands to nothing — `expr` is **not** evaluated.
4. `BUDDD_ASSERT_MSG(expr, fmt, ...)`: same as `BUDDD_ASSERT` but includes a formatted custom message after the expression text.
5. `BUDDD_VERIFY(expr)` in a debug build: evaluates `expr`. If false, logs `Fatal` + break + abort (same as ASSERT).
6. `BUDDD_VERIFY(expr)` in a release build: evaluates `expr` (for side effects). If false, logs `Fatal` — no break, no abort, execution continues.
7. `BUDDD_FAIL()` always: unconditionally logs `Fatal` + break (debug only) + abort regardless of build type.
8. `BUDDD_FAIL_MSG(fmt, ...)`: same as `BUDDD_FAIL` with a formatted custom message.
9. `LogLevel::Fatal` is added to the logging system as a level above `Error`. The console sink formats it as `[FATAL]`.
10. `debug_break()` triggers a debugger interrupt if a debugger is attached; if no debugger is attached, it is effectively a no-op (the process continues to `std::abort()` immediately after).

### Assertion failure log format

When an assertion fails, the logger receives a single `Fatal`-level message with the fixed tag `"Assert"`. The formatted message body contains:

```
Assertion failed: <expression>
Message: <custom message>                    (only if BUDDD_ASSERT_MSG or BUDDD_FAIL_MSG)
Location: <file>:<line>
Function: <function>
```

Example output:
```
[FATAL] [Assert] Assertion failed: entity.IsValid()
Message: Tried to access destroyed entity
Location: src/ecs/EntityManager.cpp:123
Function: EntityManager::GetComponent
```

When no custom message is provided (plain `BUDDD_ASSERT` / `BUDDD_FAIL`), the "Message:" line is omitted entirely.

## Key entities

| Entity | Role |
|---|---|
| `buddd::log::LogLevel::Fatal` | New log level above `Error`, used for assertion failures. Represents an unrecoverable condition that terminates the process. |
| `buddd::engine::debug_break()` | Platform-aware inline function that triggers a debugger breakpoint interrupt. Active only in non-NDEBUG builds. |
| `buddd::engine::handle_assertion_failure()` | Public core function that formats and logs the failure message, calls `debug_break()`, and calls `std::abort()`. Can be tested indirectly (formatting) without triggering abort by NOT calling it. |
| `BUDDD_ASSERT(expr)` | Macro: evaluates expression in debug builds; compiles out in release. |
| `BUDDD_ASSERT_MSG(expr, fmt, ...)` | Macro: same as ASSERT with a formatted custom message. |
| `BUDDD_VERIFY(expr)` | Macro: always evaluates expression (preserves side effects); fatal in debug, log-only in release. |
| `BUDDD_FAIL()` | Macro: unconditionally fatal + abort in all builds. |
| `BUDDD_FAIL_MSG(fmt, ...)` | Macro: unconditionally fatal + abort with formatted message in all builds. |

## User stories

### Story 1 — Basic assertion for debug-only invariants (Priority: P1)

As an engine developer, I want to assert that a pointer is non-null before dereferencing it, with the check compiled out in release builds for zero overhead.

**Given** a debug build of the engine
**When** `BUDDD_ASSERT(window != nullptr)` is reached with `window == nullptr`
**Then** a `[FATAL]` message is logged, the debugger breaks (if attached), and the process aborts.

**Given** a release build of the engine
**When** `BUDDD_ASSERT(window != nullptr)` is reachable
**Then** no assertion code is emitted — the expression is not evaluated and no log/abort occurs.

**Given** a debug build
**When** `BUDDD_ASSERT(window != nullptr)` is reached with `window != nullptr`
**Then** execution continues normally with no side effects.

### Story 2 — Assertion with custom message (Priority: P1)

As an engine developer, I want to provide additional context when an assertion fails, so that the failure report includes actionable information.

**Given** `BUDDD_ASSERT_MSG(entity.IsValid(), "Expected a valid entity, id={}", entity.id())`
**When** the assertion fails in a debug build
**Then** the logged message contains both `Assertion failed: entity.IsValid()` and `Message: Expected a valid entity, id=42`.

### Story 3 — Verify macro for side-effect-preserving checks (Priority: P1)

As an engine developer, I want a check that evaluates the expression in all builds (to preserve needed side effects) but only terminates in debug builds.

**Given** a release build
**When** `BUDDD_VERIFY(some_ref_count--)` is reached
**Then** `some_ref_count` is decremented regardless of build type.

**Given** a release build
**When** `BUDDD_VERIFY(some_ref_count--)` evaluates to `0` (falsy)
**Then** a `[FATAL]` message is logged and execution continues (no break, no abort).

**Given** a debug build
**When** `BUDDD_VERIFY(some_ref_count--)` evaluates to `0` (falsy)
**Then** a `[FATAL]` message is logged, debugger breaks, and the process aborts.

### Story 4 — Unconditional fail macro (Priority: P1)

As an engine developer, I want a fatal failure that always fires regardless of build type, for unreachable code paths (e.g., `default` in exhaustive `switch` statements).

**Given** any build (debug or release)
**When** `BUDDD_FAIL_MSG("Unsupported shader type: {}", static_cast<int>(type))` is reached
**Then** a `[FATAL]` message is logged, the process aborts (and in debug builds, debugger breaks first).

### Story 5 — No double evaluation (Priority: P2)

As an engine developer, I want assurance that assertion macro arguments are never evaluated more than once.

**Given** `BUDDD_ASSERT(next() != nullptr)`
**When** `next()` has observable side effects
**Then** `next()` is called at most once in both debug and release builds.

### Story 6 — Assertion formatting can be tested (Priority: P2)

As a test author, I want to verify the failure report format without triggering `std::abort()`.

**Given** a test that calls a helper that formats the assertion report
**When** the helper produces the formatted string
**Then** the test can assert that the string contains the expected expression, file, line, function, and optional custom message.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `LogLevel::Fatal` exists in the `buddd::log::LogLevel` enum with a value greater than `Error`. | Unit test asserts `static_cast<int>(LogLevel::Fatal) > static_cast<int>(LogLevel::Error)`. |
| AC-002 | `LogLevel::Fatal` is the highest severity level (no level above it). | Unit test asserts Fatal is the last enumerator in `LogLevel`. |
| AC-003 | Console/formatted output for a Fatal-level message uses `[FATAL]` prefix. | Unit test with `ScopedMemoryLogger` logs at `Fatal` level with `"Assert"` tag and verifies the `LogMessage::level` field equals `LogLevel::Fatal`. |
| AC-004 | `buddd::engine::debug_break()` exists, is declared in `src/engine/debug/debug_break.h`, and compiles in both debug and release builds. | Compile a translation unit that calls `debug_break()` — link succeeds. |
| AC-005 | `debug_break()` uses `__builtin_trap()` on GCC/Clang and `__debugbreak()` on MSVC. | Code review of `debug_break.h` confirms platform-appropriate intrinsic. (No MSVC build available; the branch is present for completeness.) |
| AC-006 | `debug_break()` is a no-op (empty) when `NDEBUG` is defined (release build). | Code review confirms `#ifndef NDEBUG` guard around the intrinsic call. |
| AC-007 | `BUDDD_ASSERT(expr)` in debug (`NDEBUG` not defined) evaluates `expr` and, if false, logs `Fatal` + breaks + aborts. | Unit test verifies that a false assertion increments a counter set up in the expression (expression was evaluated). Testing abort is done by NOT calling the macro — instead, test the `handle_assertion_failure()` formatting separately. |
| AC-008 | `BUDDD_ASSERT(expr)` in release (`NDEBUG` defined) expands to nothing — `expr` is not evaluated. | Compile a test in release mode that uses `BUDDD_ASSERT` with an expression containing side effects; verify the side effect did not occur at runtime. |
| AC-009 | `BUDDD_ASSERT_MSG(expr, fmt, ...)` behaves identically to `BUDDD_ASSERT` but includes a formatted custom message in the failure report. | Verify via format/spy test that `handle_assertion_failure()` is called with the expected optional message content. |
| AC-010 | `BUDDD_VERIFY(expr)` in debug evaluates `expr` and, if false, logs `Fatal` + breaks + aborts. | Same approach as AC-007 — verify expression evaluation, verify log output via `ScopedMemoryLogger` (without triggering abort by using test-only code path). |
| AC-011 | `BUDDD_VERIFY(expr)` in release evaluates `expr` (side effects occur) but, if false, only logs `Fatal` — no break, no abort. | Release test verifies side effect occurred; release test with false expression verifies a `[FATAL]` log entry is created (process does not abort; test continues). |
| AC-012 | `BUDDD_FAIL()` unconditionally evaluates the failure path (logs Fatal + aborts) in all builds. | Test verifies that `BUDDD_FAIL()` triggers `handle_assertion_failure()` with appropriate data. Abort not tested directly. |
| AC-013 | `BUDDD_FAIL_MSG(fmt, ...)` unconditionally evaluates the failure path with a formatted custom message. | Test verifies custom message formatting matches expected output. |
| AC-014 | `handle_assertion_failure()` is declared in `src/engine/debug/assert.h` with signature `void handle_assertion_failure(std::string_view expr, std::string_view file, int line, std::string_view function, std::optional<std::string> message = std::nullopt)`. | Code review confirms the declaration. |
| AC-015 | The formatted assertion report includes the expression text, source file, line number, function name, and optional message in the expected format. | Unit test constructs a `handle_assertion_failure` call (but does not invoke it — test a string-building helper instead, or invoke in a test-safe path where abort is replaced). |
| AC-016 | All assertion macros use the fixed tag `"Assert"` — no per-file `BUDDD_LOG_TAG` declaration is needed. | Code review confirms `"Assert"` is hardcoded in the macro implementations. |
| AC-017 | `BUDDD_ASSERT` uses a `do { } while(false)` wrapper for multi-statement safety. | Code review of macro definitions confirms the wrapper pattern. |
| AC-018 | `BUDDD_ASSERT` expression is never evaluated twice in any code path. | Unit test with a stateful functor that tracks invocation count verifies exactly one call when assertion fails or passes. |
| AC-019 | The assertion system has zero external dependencies beyond C++26 standard library. | Code review of `#include` directives confirms no third-party headers. |
| AC-020 | `handle_assertion_failure()` logs via `buddd::log::Logger::instance().log()` and `debug_break()` and calls `std::abort()`. | Code review of the implementation confirms all three actions in sequence. |

## E2E Verification

This feature will be verified through:

- **Unit test suite** (`assertion_tests.cpp` or similar) covering:
  - `LogLevel::Fatal` enum value ordering and rendering
  - `handle_assertion_failure()` formatting/reporting logic (using a test helper that captures the formatted string without calling abort)
  - `BUDDD_VERIFY` expression evaluation in debug and release builds
  - Non-double-evaluation of `BUDDD_ASSERT` expressions
  - `debug_break()` compilation and presence
  - `BUDDD_FAIL` / `BUDDD_FAIL_MSG` custom message formatting
- **Compile-time verification**: A test binary compiled in release mode confirms that `BUDDD_ASSERT` expression side effects do not occur.
- **Integration**: The existing `ScopedMemoryLogger` captures `Fatal`-level log output for format assertions.

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | All 20 acceptance criteria pass in CI on every commit (debug and release builds). |
| SC-002 | `BUDDD_ASSERT(true)` in any build adds zero branch instructions when compiled with optimisations (confirmed via spot-check of generated assembly or compiler explorer for representative compilers). |
| SC-003 | `BUDDD_ASSERT(expr)` in release builds generates no code — the expression is dead-stripped by the compiler. |
| SC-004 | `BUDDD_FAIL()` failure path (log + abort) completes in under 1 ms wall-clock before the abort signal. |

## Edge cases

1. **Assertion expression has side effects with `BUDDD_ASSERT` in release**: The expression is not evaluated — this is by design. Developers must use `BUDDD_VERIFY` if side effects must be preserved.
2. **Assertion expression throws an exception**: The assertion macro does not catch exceptions. If the expression itself throws, the exception propagates naturally (assertion failure does not occur).
3. **Logger not initialised when assertion fires**: `Logger::instance()` returns a valid reference (magic static) but `is_enabled()` may return false if no sinks are configured. The `handle_assertion_failure()` function logs directly — if the logger is uninitialised, the Fatal message is silently dropped (same as any other log call before init).
4. **`debug_break()` called without a debugger attached**: The process continues to `std::abort()` immediately — no user-visible side effect from the breakpoint instruction itself.
5. **`BUDDD_ASSERT_MSG` with empty format string**: Behaves like `BUDDD_ASSERT` — no "Message:" line appears in the output.
6. **`BUDDD_ASSERT_MSG` with no variadic arguments**: The format string is used as-is (literal string), matching `std::format` behaviour.
7. **`BUDDD_FAIL()` / `BUDDD_FAIL_MSG()` in a noexcept function**: `std::abort()` may call `std::terminate()` depending on noexcept guarantees. This is acceptable — the process terminates either way.
8. **Assertion in static initialisation / destruction code**: `Logger::instance()` may not be available. The assertion macro still compiles — if the logger is not constructed yet, `is_enabled()` returns false and the abort path is still reached (via `handle_assertion_failure` which calls abort regardless of logger state).

## Error cases

| Scenario | Behavior |
|---|---|
| `BUDDD_ASSERT(expr)` with `expr` false in debug | Log `Fatal` via Logger, `debug_break()`, `std::abort()`. Process terminates. |
| `BUDDD_ASSERT(expr)` with `expr` false in release | Nothing — expression not evaluated, no code emitted. |
| `BUDDD_VERIFY(expr)` with `expr` false in release | Log `Fatal` via Logger, execution continues (no break, no abort). |
| `BUDDD_FAIL()` reached in any build | Log `Fatal`, debugger break (debug only), `std::abort()`. Process terminates. |
| Logger not available (static init order) | `handle_assertion_failure()` attempts to log (message may be dropped). `std::abort()` is still called — process terminates. |

## Permissions and security

- The assertion system does not read or write any files, network resources, or environment variables.
- Assertion expressions may reference any memory reachable from the calling context — the developer is responsible for ensuring the expression is safe to evaluate.
- No sensitive data (passwords, tokens, keys) should appear in assertion expressions or messages. The framework does not filter assertion message content.
- No elevated permissions are required.

## Observability

- All assertion failures are logged at `LogLevel::Fatal` with the fixed tag `"Assert"`, making them visible in any log sink (console, file, memory sink).
- `debug_break()` does not produce any log output — it is a silent interrupt that only affects attached debuggers.
- Unit tests use `ScopedMemoryLogger` to capture and assert on `[FATAL]` messages without process termination.
- Assertion failure rate is not tracked or metered — assertions are considered programming errors, not operational events.

## Out of scope

- Crash dump generation (minidump, core dump).
- Remote error/telemetry reporting.
- In-editor assertion popup UI.
- Assertion ignore lists or "ignore always" per assertion.
- Graceful recovery from assertion failures (assertions always abort or log-fatal).
- Stack trace capture beyond `__FUNCTION__`.
- Symbolication.
- `static_assert`-like compile-time assertion variants.
- Migration of existing ad-hoc checks in engine source code.
- Custom assertion handlers or replaceable assertion callbacks.
- Thread-related assertions (`BUDDD_ASSERT_ON_THREAD`, etc.).

## Documentation updates

The following documents must be updated when this feature is implemented:

- **`src/engine/log/log.h`** — Add `LogLevel::Fatal` to the enum.
- **`docs/wiki/domain/logging.md`** — Mention `Fatal` level in the log levels table and CLI flags.
- **`docs/wiki/domain/logging.md`** — Document the `"Assert"` fixed tag convention.
- **New ADR** required — Architectural decision record for the assertion system (design decisions, macro API, debug break, NDEBUG-only compilation).
- **New wiki page** (or section in `docs/wiki/domain/logging.md`) — Assertion system API reference, conventions, and usage guide.

## Assumptions

1. `NDEBUG` is the sole build-type discriminator. No additional CMake flags are needed to control assertion behaviour. `#ifdef NDEBUG` = release; `#ifndef NDEBUG` = debug.
2. The `buddd::log::Logger` singleton is available at the point any assertion fires (except during static initialisation — see edge case 8).
3. `std::abort()` is the correct process-termination mechanism for debug-build assertion failures. No custom signal handler or graceful shutdown is needed.
4. `__builtin_trap()` on GCC/Clang is preferable to `__builtin_debugtrap()` or `raise(SIGTRAP)` because it reliably triggers an interrupt in debug builds and is recognised by optimisers as a "no-return" point.
5. Assertion macros use the `do { } while(false)` pattern for multi-statement safety, consistent with project conventions.
6. The `"Assert"` tag does not need to be declared via `BUDDD_LOG_TAG` — assertion macros hardcode the tag internally.
7. `std::optional<std::string>` is available (C++17, which is a subset of C++26) for the optional message parameter in `handle_assertion_failure()`.
8. The `BUDDD_TESTING` define (present in both debug and release test builds) is NOT used to control assertion behaviour. Assertion behaviour is governed solely by `NDEBUG`. Tests that need to verify formatting without abort use helper techniques (e.g., testing a string-building function) rather than a test-mode flag in the macros themselves.

## Open questions

None. All architectural decisions were confirmed during the grill-me session with the human.
