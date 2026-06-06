# ADR-021-developer-assertions - Developer Assertions System

## Status

Accepted

## Context

The Buddd Engine had no structured way to check internal invariants during development. Programming errors (null pointer dereferences, invalid state transitions, violated preconditions) propagated silently, producing hard-to-debug symptoms far from the root cause. Existing engine code used ad-hoc `assert(3)` calls, manual null checks, or simply assumed correctness without validation.

A lightweight assertion system was needed to:

- Catch programming errors early in debug/development builds.
- Provide consistent, readable failure reports (expression, location, custom message).
- Integrate with the existing `buddd::log` logging system (ADR-020) instead of introducing a separate reporting channel.
- Be compilable out of release builds so there is zero overhead in shipped code.
- Support both fatal (always-active) and verify (side-effect-preserving) patterns.

The logging system (ADR-020) defined five log levels (`Trace` through `Error`). Assertion failures require a severity higher than `Error` to signal an unrecoverable condition that terminates the process, necessitating a new `LogLevel::Fatal`.

## Decision

### 1. `LogLevel::Fatal` added above `Error`

A new `LogLevel::Fatal` enumerator is inserted after `LogLevel::Error` in `buddd::log::LogLevel`, raising the level count from 5 to 6:

```
Trace(0) < Debug(1) < Info(2) < Warn(3) < Error(4) < Fatal(5)
```

`Fatal` represents an unrecoverable condition that terminates the process. It is the highest severity level and is always enabled in all builds. The console sink renders it as `[FATAL]`. It is **not** exposed as a CLI `--log-level` option — users cannot specify `--log-level=fatal`.

This extends ADR-020's level design without changing the macro-based API, singleton `Logger`, or sink interface.

### 2. Five-macro assertion API

The primary API surface is five preprocessor macros, all defined in `src/engine/debug/assert.h`:

| Macro | Debug (`NDEBUG` not defined) | Release (`NDEBUG` defined) |
|---|---|---|
| `BUDDD_ASSERT(expr)` | Evaluate `expr`; if false: log `Fatal` + break + abort | Expands to `((void)0)` — expression NOT evaluated |
| `BUDDD_ASSERT_MSG(expr, fmt, ...)` | Same as `BUDDD_ASSERT` + formatted custom message | Expands to `((void)0)` — expression NOT evaluated |
| `BUDDD_VERIFY(expr)` | Evaluate `expr`; if false: log `Fatal` + break + abort | Evaluate `expr` (preserves side effects); if false: log `Fatal` only (no break, no abort) |
| `BUDDD_FAIL()` | Log `Fatal` + break + abort unconditionally | Log `Fatal` + abort unconditionally (no break) |
| `BUDDD_FAIL_MSG(fmt, ...)` | Same as `BUDDD_FAIL` + formatted custom message | Same as `BUDDD_FAIL` + formatted custom message |

All macros use the `do { } while(false)` wrapper for multi-statement safety (consistent with ADR-020's `BUDDD_LOG_*` macro pattern). All variadic macros use `__VA_OPT__(,)` for correct C++20 zero-argument handling.

### 3. NDEBUG-only build detection (no separate CMake flags)

Assertion behavior is governed **solely** by the standard `NDEBUG` preprocessor symbol:

- `#ifndef NDEBUG` → debug build: assertions are active.
- `#ifdef NDEBUG` → release build: `BUDDD_ASSERT` / `BUDDD_ASSERT_MSG` compile out entirely; `BUDDD_VERIFY` preserves expression evaluation but does not abort; `BUDDD_FAIL` / `BUDDD_FAIL_MSG` remain unconditional.

No additional CMake option (e.g., `BUDDD_ENABLE_ASSERTIONS`) is introduced. No test-specific define controls assertion behaviour — tests verify formatting via `format_assertion_failure_message()` rather than a test-mode flag in the macros.

This decision avoids fragmentation of the build-type concept and is consistent with the project's existing use of `NDEBUG` for debug/release discrimination (ADR-020 uses the same convention for log level defaults).

### 4. Platform-specific debug break via `__builtin_trap()` / `__debugbreak()`

A header-only function `buddd::engine::debug_break()` is defined in `src/engine/debug/debug_break.h`:

```cpp
inline void debug_break() {
#ifndef NDEBUG
    #if defined(_MSC_VER)
        __debugbreak();
    #elif defined(__clang__) || defined(__GNUC__)
        __builtin_trap();
    #endif
#endif
}
```

- `__builtin_trap()` is preferred over `__builtin_debugtrap()` on GCC/Clang because the compiler treats it as no-return, enabling better codegen in the surrounding code.
- `__debugbreak()` on MSVC emits a hardware breakpoint (`int 3`) that the debugger catches.
- In release builds (`NDEBUG` defined), the function body is empty — no intrinsics are emitted.
- If no debugger is attached, the breakpoint instruction executes harmlessly and the process continues to `std::abort()`.

### 5. Fixed `"Assert"` tag for assertion log messages

All assertion macros hardcode the tag `"Assert"` when logging via the `Logger`. No per-file `BUDDD_LOG_TAG` declaration is required for assertion failure messages. The `BUDDD_VERIFY` release path logs directly via `Logger::instance().log()` with the hardcoded `"Assert"` tag, and `handle_assertion_failure()` uses the same fixed tag.

This ensures assertion failures are always identifiable in log output regardless of which translation unit fires the assertion, without imposing a `BUDDD_LOG_TAG` declaration requirement on files that only use assertion macros.

### 6. `handle_assertion_failure()` always calls `std::abort()` after logging

The core assertion handler `buddd::engine::handle_assertion_failure()` performs exactly three actions in sequence:

1. Formats the failure report via `format_assertion_failure_message()`.
2. Logs the formatted report at `LogLevel::Fatal` with the fixed tag `"Assert"` via `Logger::instance().log()`.
3. Calls `debug_break()` (no-op in release builds).
4. Calls `std::abort()`.

The function is declared `[[noreturn]]`. There is no graceful recovery — assertion failures always terminate the process in debug builds. The `BUDDD_VERIFY` release path is the only case where an assertion failure (falsy expression) does **not** lead to `std::abort()`; it logs at `Fatal` level and continues execution.

### 7. Test strategy: testable `format_assertion_failure_message()` avoids abort in tests

The formatting logic is extracted into a separate public function:

```cpp
[[nodiscard]] auto format_assertion_failure_message(
    std::string_view expr,
    std::string_view file,
    int line,
    std::string_view function,
    std::optional<std::string> message = std::nullopt
) -> std::string;
```

This function performs no logging, no debug break, and no abort — it is a pure string builder. Unit tests call this function directly to verify the format of assertion failure reports. They never invoke `handle_assertion_failure()` (which aborts).

The formatted output follows this structure:

```
Assertion failed: <expression>
Message: <custom message>                    (only if message is non-empty)
Location: <file>:<line>
Function: <function>
```

No trailing `\n` in the message body — the console sink appends it.

### 8. File location: `src/engine/debug/` module

The assertion system lives in a new `src/engine/debug/` subdirectory of the engine static library, containing:

| File | Role |
|---|---|
| `src/engine/debug/debug_break.h` | Header-only `debug_break()` inline function |
| `src/engine/debug/assert.h` | Public header: `format_assertion_failure_message()`, `handle_assertion_failure()`, five macros |
| `src/engine/debug/assert.cpp` | Implementation of formatting and handler functions |

This placement reflects the architectural boundary (ADR-019): no code outside `src/engine/` may include platform/graphics headers, but the assertion system is entirely standard-library-only plus `log/log.h`, which is architecture-boundary compliant.

### 9. Zero external dependencies beyond C++26 standard library

The assertion system uses only C++26 standard library headers: `<cstdlib>` (for `std::abort()`), `<format>` (for `std::format`), `<optional>`, `<string>`, `<string_view>`. The only project headers included are `"debug/debug_break.h"` and `"log/log.h"` (which itself is standard-library-only). No third-party headers are required.

### 10. Impact on ADR-020 (logging system extension)

This ADR extends the logging system defined in ADR-020 in the following ways:

- **LogLevel enum extended**: `Fatal` added after `Error` (6 levels instead of 5). The `Logger` and `Sink` interfaces are unchanged.
- **New log macros**: `BUDDD_LOG_FATAL` and `BUDDD_LOG_TAGGED_FATAL` added following the existing `ERROR` pattern.
- **Console sink updated**: `level_name()` switch extended with `case LogLevel::Fatal: return "FATAL";`.
- **No CLI change**: `Fatal` is NOT added to the `--log-level` parser — it is always enabled.
- **No structural change**: The macro-based API, singleton `Logger`, sink interface, and `LogConfig` / `Logger::init()` flow are unchanged from ADR-020.

## Alternatives considered

### 1. Separate assertion framework (CppUnit, Google Test assert, etc.)

Rejected because the assertion system needs to integrate with the engine's own logging infrastructure (ADR-020) and must work outside of test contexts. A testing framework's assertion macros would not be available in production engine code and would create a confusing dual-assertion landscape.

### 2. Custom CMake option (e.g., `BUDDD_ENABLE_ASSERTIONS`)

Rejected in favour of `NDEBUG` only. Adding a custom option fragments the build-type concept. The project already uses `NDEBUG` to distinguish debug/release builds for log level defaults (ADR-020). Using `NDEBUG` as the sole discriminator ensures assertions are always-on in debug builds and always-off (or reduced) in release builds without requiring extra CMake configuration or CI matrix expansion.

### 3. Test-specific define involvement in assertion behavior (historical: `BUDDD_TESTING`)

Rejected. A test-specific define (historically `BUDDD_TESTING`, since removed) must not control assertion behaviour. Using such a define to suppress `std::abort()` in tests would risk shipping code where assertions silently fail to abort. Instead, `format_assertion_failure_message()` is extracted as a testable helper, and tests never invoke `handle_assertion_failure()`. This keeps assertion behaviour cleanly governed by `NDEBUG` alone.

### 4. `abort()` with custom signal handler for graceful shutdown

Rejected. `std::abort()` raises `SIGABRT`, which can be caught with a signal handler, but the assertion system deliberately does not install one. If custom shutdown behaviour is desired (e.g., flushing logs before aborting), that is the responsibility of higher-level infrastructure outside the assertion system. The assertion system itself is intentionally simple: log, break, abort.

### 5. `raise(SIGTRAP)` for debug break

Rejected in favour of `__builtin_trap()` on GCC/Clang. `raise(SIGTRAP)` requires `<signal.h>`, involves a function call to `raise()`, and the compiler does not treat it as a no-return point. `__builtin_trap()` is recognised by the optimiser as a no-return intrinsic, producing smaller and more predictable codegen around assertion sites.

### 6. `std::abort()` vs `std::terminate()`

`std::abort()` is chosen over `std::terminate()` because `abort()` sends `SIGABRT`, which generates a core dump by default on most platforms, enabling post-mortem analysis. `std::terminate()` may call `std::abort()` anyway (depending on implementation) but does not guarantee a core dump and may invoke custom `std::terminate_handler` functions. For assertion failures, a core dump is the desired outcome.

### 7. Exception-based assertion failure (throw from within assertion)

Rejected. Assertions represent unrecoverable programming errors, not recoverable runtime conditions. Throwing an exception through potentially broken program state is undefined behaviour if the stack is corrupted. `std::abort()` is the correct response to an unrecoverable invariant violation.

### 8. Non-macro API (inline functions instead of macros)

Rejected because macros are the only portable way to automatically capture `__FILE__`, `__LINE__`, and `__FUNCTION__` at the call site, and to conditionally compile out expressions with `#ifdef NDEBUG`. The `do { } while(false)` wrapper pattern is already established by ADR-020's `BUDDD_LOG_*` macros.

### 9. Single `BUDDD_ASSERT` macro with format variants handled by overloads

Rejected because the behaviour matrix (debug vs release, always-evaluate vs. compile-out vs. side-effect-preserving) requires distinct macro implementations that cannot be expressed with a single function or overload set across `NDEBUG` boundaries. Five macros provide a clear, unambiguous API surface where each macro's contract is immediately visible from its name.

## Consequences

### Positive

- Consistent assertion API with clear behaviour matrix across debug and release builds.
- Seamless integration with the existing logging system — assertion failures appear in the same log stream, respecting the same sinks and format.
- `format_assertion_failure_message()` is directly testable without triggering `std::abort()`, enabling 12 unit tests that cover formatting, expression evaluation, and non-double-evaluation.
- Zero external dependencies beyond C++26 standard library — no third-party assertion framework to maintain.
- No additional CMake flags — `NDEBUG` is the sole discriminator, consistent with ADR-020.
- `BUDDD_ASSERT` expressions are fully dead-stripped in release builds (no overhead in shipped code).
- `BUDDD_VERIFY` preserves side effects in all builds, giving developers a safe pattern for assertions that guard side-effect-bearing expressions.
- Fixed `"Assert"` tag ensures all assertion failures are traceable regardless of which module fires them.

### Negative

- No graceful recovery from assertion failures — `std::abort()` terminates the process immediately after logging. This is intentional (assertions represent unrecoverable programming errors).
- `BUDDD_ASSERT` silently drops side effects in release builds — developers must remember to use `BUDDD_VERIFY` if expression side effects must be preserved. This is documented in the macro contracts.
- No MSVC CI build to verify the `__debugbreak()` branch — tested only on GCC/Clang. The MSVC branch is present for completeness and will require manual verification if MSVC support is added.
- Six log levels instead of five, extending the ADR-020 design. Existing code that enumerates levels or uses `switch` without a `default` case will produce compiler warnings until updated.

### Risks

- Low. The assertion system is additive (new files, new macros, one new enum value). No existing code is modified structurally. The 8 assertion insertion points in engine source files are low-risk precondition checks.
- Pre-existing release test failures in `cli_app_tests.cpp`, `cmd_tests.cpp`, and `logging_tests.cpp` (display-related and file-permission tests) are unrelated to assertion changes.

## Compliance

- `src/engine/debug/assert.h` MUST declare the five assertion macros with the `do { } while(false)` wrapper and the `NDEBUG`-based behaviour matrix documented above.
- `src/engine/debug/debug_break.h` MUST use `__builtin_trap()` on GCC/Clang and `__debugbreak()` on MSVC, guarded by `#ifndef NDEBUG`.
- `src/engine/debug/assert.h` MUST NOT use any test-specific define to control assertion behaviour.
- All assertion macros MUST hardcode the `"Assert"` tag — no per-file `BUDDD_LOG_TAG` declaration required.
- `src/engine/log/log.h` MUST add `LogLevel::Fatal` after `LogLevel::Error`.
- `src/engine/log/log.h` MUST add `BUDDD_LOG_FATAL` and `BUDDD_LOG_TAGGED_FATAL` macros.
- `src/engine/log/console_sink.cpp` MUST render `LogLevel::Fatal` as `"FATAL"`.
- `format_assertion_failure_message()` MUST be a public `[[nodiscard]]` function that does not call `std::abort()`.
- `handle_assertion_failure()` MUST be `[[noreturn]]` and MUST call `Logger::instance().log()`, `debug_break()`, and `std::abort()` in sequence.

## Related documents

- SPEC-023 — Developer Assertions specification (`.specs/sprint-2026-06/developer-assertions/spec.md`)
- IMPL-023 — Developer Assertions implementation contract (`.specs/sprint-2026-06/developer-assertions/implementation-contract.md`)
- ADR-020 — Custom Logging System (extended by this ADR with `LogLevel::Fatal`)
- ADR-019 — Architecture Boundaries (assertion system is standard-library-only, compliant)
- ADR-009 — Test File Naming Convention (`_tests.cpp` suffix followed by `assertion_tests.cpp`)
- `docs/wiki/domain/logging.md` — Logging system documentation (to be updated with `Fatal` level and assertion reference)

---

*Derived from SPEC-023, IMPL-023, and design decisions settled during the 2026-06-06 grill-me session.*
