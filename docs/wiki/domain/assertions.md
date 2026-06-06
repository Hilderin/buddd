# Assertion System

> **API reference, behaviour matrix, and usage guide for the Buddd Engine assertion system.**
>
> Reference: [SPEC-023](/.specs/sprint-2026-06/developer-assertions/spec.md), [IMPL-023](/.specs/sprint-2026-06/developer-assertions/implementation-contract.md), [ADR-021](/docs/adr/ADR-021-developer-assertions.md)

## Quick start

```cpp
// 1. Include the public header
#include "debug/assert.h"

// 2. Assert an invariant (debug builds only)
BUDDD_ASSERT(ptr != nullptr);

// 3. Assert with a custom message
BUDDD_ASSERT_MSG(entity.IsValid(), "Expected a valid entity, id={}", entity.id());

// 4. Verify with side effects preserved in all builds
BUDDD_VERIFY(ref_count_--);

// 5. Unreachable code path
BUDDD_FAIL_MSG("Unsupported shader type: {}", static_cast<int>(type));
```

## Header location

All assertion macros and APIs are available via a single header:

```cpp
#include "debug/assert.h"   // engine debug module
```

This header includes `debug/debug_break.h` and `log/log.h` automatically — no additional includes are needed.

> **No `BUDDD_LOG_TAG` needed**: Assertion macros hardcode the tag `"Assert"` internally. Files that only use assertion macros do not need a `BUDDD_LOG_TAG` declaration.

## Behaviour matrix

All assertion behaviour is governed solely by `NDEBUG` — no additional CMake flags or `BUDDD_TESTING` involvement.

| Macro | Debug (`NDEBUG` not defined) | Release (`NDEBUG` defined) |
|---|---|---|
| `BUDDD_ASSERT(expr)` | Evaluates `expr`. If false: log `Fatal` + `debug_break()` + `std::abort()` | Expands to `((void)0)` — expression **not** evaluated |
| `BUDDD_ASSERT_MSG(expr, fmt, ...)` | Same as `BUDDD_ASSERT` + formatted custom message | Expands to `((void)0)` — expression **not** evaluated |
| `BUDDD_VERIFY(expr)` | Evaluates `expr`. If false: log `Fatal` + `debug_break()` + `std::abort()` | Evaluates `expr` (preserves side effects). If false: log `Fatal` **only** (no break, no abort) |
| `BUDDD_FAIL()` | Log `Fatal` + `debug_break()` + `std::abort()` unconditionally | Log `Fatal` + `std::abort()` unconditionally (no break) |
| `BUDDD_FAIL_MSG(fmt, ...)` | Same as `BUDDD_FAIL` + formatted custom message | Same as `BUDDD_FAIL` + formatted custom message |

### Key rules

- `BUDDD_ASSERT` / `BUDDD_ASSERT_MSG` expressions are **never** evaluated in release builds — use `BUDDD_VERIFY` if side effects must be preserved.
- `BUDDD_VERIFY` evaluates the expression in **all** builds but only aborts in debug builds. In release builds, a falsy expression logs `Fatal` and continues.
- `BUDDD_FAIL` / `BUDDD_FAIL_MSG` are **unconditional** — they always fire regardless of build type. Use for unreachable code paths (e.g., exhaustive `switch` `default` cases).
- All macros use a `do { } while(false)` wrapper for multi-statement safety.

## API reference

### `BUDDD_ASSERT(expr)`

Debug-only invariant check. Evaluates `expr` and, if false, triggers a fatal assertion failure.

```cpp
BUDDD_ASSERT(window != nullptr);
BUDDD_ASSERT(next() != nullptr);   // evaluated at most once — no double evaluation
```

### `BUDDD_ASSERT_MSG(expr, fmt, ...)`

Same as `BUDDD_ASSERT` but includes a `std::format`-style formatted message in the failure report.

```cpp
BUDDD_ASSERT_MSG(entity.IsValid(), "Tried to access destroyed entity, id={}", entity.id());
```

The `"Message:"` line is omitted entirely if the format string is empty or produces an empty string.

### `BUDDD_VERIFY(expr)`

Side-effect-preserving check. Evaluates `expr` in all builds. In debug builds, behaves like `BUDDD_ASSERT`. In release builds, logs `Fatal` on failure but does **not** abort — execution continues.

```cpp
// ref_count is decremented even in release builds
BUDDD_VERIFY(ref_count_-- > 0);
```

### `BUDDD_FAIL()`

Unconditionally triggers a fatal assertion failure in all builds.

```cpp
switch (type) {
    case Type::A: ...
    case Type::B: ...
    default:
        BUDDD_FAIL();  // unreachable
}
```

### `BUDDD_FAIL_MSG(fmt, ...)`

Same as `BUDDD_FAIL` but includes a formatted custom message.

```cpp
switch (attr_type) {
    case VertexAttributeType::Float:  return {GL_FLOAT, 1};
    // ...
    default:
        BUDDD_FAIL_MSG("Unknown VertexAttributeType: {}", static_cast<int>(attr_type));
}
```

## Failure report format

When an assertion fails, a single `Fatal`-level message is logged with the fixed tag `"Assert"`:

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

When no custom message is provided, the `"Message:"` line is omitted.

## Debug break behaviour

On assertion failure in a non-NDEBUG build, the engine calls `buddd::engine::debug_break()` before `std::abort()`:

- **GCC / Clang**: Uses `__builtin_trap()` — the compiler treats this as no-return, producing optimal codegen.
- **MSVC**: Uses `__debugbreak()` — emits a hardware breakpoint (`int 3`) that the attached debugger catches.
- **No debugger attached**: The breakpoint instruction executes harmlessly and the process continues to `std::abort()`.

In release builds (`NDEBUG` defined), `debug_break()` is an empty inline function — no code is emitted.

## Logging integration

Assertion failures are logged at `LogLevel::Fatal` via the existing `buddd::log::Logger` singleton. The `Fatal` level:

- Is always enabled in all builds — cannot be suppressed via `--log-level` or `--log-filter`.
- Renders as `[FATAL]` in the console sink.
- Represents an unrecoverable condition that terminates the process (except `BUDDD_VERIFY` in release builds).

## `handle_assertion_failure()`

The core handler function performs three actions in sequence:

1. **Logs** the formatted failure report at `LogLevel::Fatal` with tag `"Assert"` via `Logger::instance().log()`.
2. **Breaks** via `debug_break()` (no-op in release builds).
3. **Aborts** via `std::abort()`.

```cpp
[[noreturn]] void handle_assertion_failure(
    std::string_view expr,
    std::string_view file,
    int line,
    std::string_view function,
    std::optional<std::string> message = std::nullopt
);
```

This function is declared `[[noreturn]]` because it always calls `std::abort()`. It is called internally by the assertion macros; direct use is rarely needed.

## `format_assertion_failure_message()`

A public pure-string-builder function that formats the assertion report without logging, breaking, or aborting. Safe to call from unit tests.

```cpp
[[nodiscard]] auto format_assertion_failure_message(
    std::string_view expr,
    std::string_view file,
    int line,
    std::string_view function,
    std::optional<std::string> message = std::nullopt
) -> std::string;
```

## Testing

The assertion system is tested via `tests/assertion_tests.cpp` (tagged `[assertion]`), which covers:

- `LogLevel::Fatal` enum ordering (value 5, above Error)
- `debug_break()` compilation and callability
- `format_assertion_failure_message()` formatting (with and without custom message)
- Fatal-level log capture via `ScopedMemoryLogger`
- `BUDDD_VERIFY` expression evaluation in all builds
- Single evaluation (no double evaluation) of `BUDDD_ASSERT` and `BUDDD_VERIFY`
- Release-build expression omission for `BUDDD_ASSERT`
- `BUDDD_FAIL_MSG` formatting
- Fixed `"Assert"` tag convention

Tests use `format_assertion_failure_message()` directly (which does not abort) and `ScopedMemoryLogger` for log capture. They never invoke `handle_assertion_failure()` (which aborts).

## Edge cases

| Scenario | Behaviour |
|---|---|
| Expression has side effects with `BUDDD_ASSERT` in release | Expression not evaluated — use `BUDDD_VERIFY` instead |
| Expression throws an exception | Exception propagates naturally (assertion does not fire) |
| Logger not initialised | `Logger::instance()` returns valid ref; message silently dropped if no sinks; `std::abort()` still called |
| `debug_break()` without debugger | Harmless — process continues to `std::abort()` |
| `BUDDD_ASSERT_MSG` with empty format string | Behaves like `BUDDD_ASSERT` — no `"Message:"` line |
| `BUDDD_FAIL()` in `noexcept` function | `std::abort()` may call `std::terminate()` — process terminates either way |
| Assertion during static init/destruction | `Logger` may not be available; `std::abort()` still terminates the process |

## Reference

- Spec: [SPEC-023](/.specs/sprint-2026-06/developer-assertions/spec.md) — Full specification, user stories, acceptance criteria, edge cases
- Implementation contract: [IMPL-023](/.specs/sprint-2026-06/developer-assertions/implementation-contract.md) — Required implementation behavior, test requirements, API signatures
- ADR: [ADR-021](/docs/adr/ADR-021-developer-assertions.md) — Architectural decisions (Fatal level, five macros, debug break, NDEBUG-only build detection, fixed Assert tag)
- Logging system: [logging.md](logging.md) — Logging system documentation (`Fatal` level, console format, CLI flags)
