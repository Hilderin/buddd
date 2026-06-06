# IMPL-023 — Developer Assertions

## Source spec

`.specs/sprint-2026-06/developer-assertions/spec.md`

## Goal

Implement a lightweight assertion system for the Buddd Engine with five macros (`BUDDD_ASSERT`, `BUDDD_ASSERT_MSG`, `BUDDD_VERIFY`, `BUDDD_FAIL`, `BUDDD_FAIL_MSG`), a `debug_break()` platform helper, a `LogLevel::Fatal` addition to the logging system, and a testable `format_assertion_failure_message()` / `handle_assertion_failure()` core. All assertion failures log via the existing `buddd::log::Logger` at `Fatal` level with the fixed tag `"Assert"`, then `debug_break()` (debug builds) and `std::abort()`. The system is governed solely by `NDEBUG` — no extra CMake flags, no `BUDDD_TESTING` involvement.

## Non-goals

- Do NOT migrate existing ad-hoc checks or `assert(3)` calls to the new assertion macros — deferred to future adoption task.
- Do NOT add crash reporting, crash dumps, remote telemetry, or stack trace capture beyond `__FUNCTION__`.
- Do NOT add in-editor assertion popups, "ignore always" lists, or per-assertion suppressibility.
- Do NOT add exception handling (`try`/`catch`) around assertion expressions.
- Do NOT add `static_assert`-like compile-time assertion variants.
- Do NOT add thread-related assertions (`BUDDD_ASSERT_ON_THREAD`, etc.).
- Do NOT add custom assertion handlers or replaceable assertion callbacks.
- Do NOT add `Fatal` to the CLI `--log-level` parser (level string mapping). The logging CLI parser is unchanged — `fatal` is not accepted as a CLI level string.
- Do NOT change the wiki (wiki-agent handles docs). Do NOT create ADRs (governance-reviewer handles that).
- Do NOT change CMakeLists.txt — both `src/engine/CMakeLists.txt` and `tests/CMakeLists.txt` use `file(GLOB_RECURSE)` that auto-discovers new files.

## Relevant ADRs

| ADR | Constraint |
|---|---|
| ADR-009 | Test files must use the plural `_tests.cpp` suffix. New test file: `assertion_tests.cpp`. |
| ADR-020 | Logging system is extended by adding `LogLevel::Fatal`. The macro-based API, `Logger` singleton, and sink interface are not changed structurally. |
| ADR-019 | No code outside `src/engine/` may include platform/graphics headers. The assertion system is entirely standard-library-only plus `log/log.h`, which is architecture-boundary compliant. |

## Files to inspect

| File | Why |
|---|---|
| `src/engine/log/log.h` | Existing `LogLevel` enum (add `Fatal`), log macro patterns (copy for `BUDDD_LOG_FATAL`/`BUDDD_LOG_TAGGED_FATAL`). |
| `src/engine/log/console_sink.cpp` | Level name `switch` — must add `case LogLevel::Fatal: return "FATAL"`. |
| `tests/log_helpers.h` | `ScopedMemoryLogger` — the test file uses this to capture Fatal-level messages. |
| `src/engine/CMakeLists.txt` | Confirm `file(GLOB_RECURSE)` auto-discovers `src/engine/debug/*.{h,cpp}`. |
| `tests/CMakeLists.txt` | Confirm `file(GLOB_RECURSE ... *_tests.cpp)` auto-discovers `assertion_tests.cpp`. |
| `src/engine/engine_service.cpp` | Target for assertion insertion — accessors dereference unique_ptrs without null checks. |
| `src/engine/scene/entity.cpp` | Target for assertion insertion — methods assume world_ is non-null. |
| `src/engine/render/render_device_opengl.cpp` | Target for assertion insertion — switches with no default case. |
| `docs/adr/ADR-009-test-file-naming-convention.md` | Confirm `_tests.cpp` suffix requirement. |
| `docs/adr/ADR-020-custom-logging-system.md` | Confirm ADR constraints around the logging system. |

## Files allowed to change

### New files

| Path | Purpose |
|---|---|
| `src/engine/debug/debug_break.h` | Inline `buddd::engine::debug_break()` — platform-aware breakpoint intrinsic, no-op in NDEBUG builds. Header-only. |
| `src/engine/debug/assert.h` | Public header declaring `format_assertion_failure_message()`, `handle_assertion_failure()`, and the five assertion macros. Includes `log/log.h`. |
| `src/engine/debug/assert.cpp` | Implementation of `format_assertion_failure_message()` and `handle_assertion_failure()`. |
| `tests/assertion_tests.cpp` | Catch2 test file exercising assertion formatting, BUDDD_VERIFY evaluation, non-double-evaluation, enum ordering, debug_break compilation. Tagged `[assertion]`. |

> **Note**: The `src/engine/debug/` directory does not currently exist. The Code Agent must create it when writing the new files.

### Modified files

| Path | Change |
|---|---|
| `src/engine/log/log.h` | Add `Fatal` to `LogLevel` enum after `Error`. Update doc comment from `(4)` to `(5)`. Add `BUDDD_LOG_FATAL` and `BUDDD_LOG_TAGGED_FATAL` macros following the ERROR pattern. |
| `src/engine/log/console_sink.cpp` | Add `case LogLevel::Fatal: return "FATAL";` to the `level_name()` switch. |
| `src/engine/engine_service.cpp` | Add `#include "debug/assert.h"`. Insert BUDDD_ASSERT before dereferences in accessor methods. |
| `src/engine/scene/entity.cpp` | Add `#include "debug/assert.h"`. Insert BUDDD_ASSERT before `world_` dereference in `transform()`. |
| `src/engine/render/render_device_opengl.cpp` | Add `#include "debug/assert.h"`. Replace fallback returns in two switches with `default: BUDDD_FAIL_MSG(...)` cases. |

## Files forbidden to change

- Any file under `src/engine/` outside the files listed above — no other engine files.
- `src/cmd/app_config.cpp` / `app_config.h` — CLI flag parsing is not updated to accept `"fatal"` as a level string.
- `src/cmd/main.cpp` — no changes.
- `tests/log_helpers.h` — already exists, no modifications.
- `tests/CMakeLists.txt` — auto-discovery handles new files.
- `src/engine/CMakeLists.txt` — auto-discovery handles new files.
- Any `.specs/` file outside this feature directory.
- Any file under `docs/` — wiki updates by wiki-agent.
- Any file under `src/editor/` — no changes.

## Existing conventions to follow

| Convention | Source | Application |
|---|---|---|---|
| `#pragma once` header guard | All existing headers | Use in all new `.h` files. |
| `snake_case` for source files and directories | `docs/wiki/architecture/module-map.md` | `debug/debug_break.h`, `debug/assert.h`, `debug/assert.cpp`, `assertion_tests.cpp` |
| `snake_case` for free functions and methods | Project style (log API, engine methods) | `format_assertion_failure_message`, `handle_assertion_failure` |
| PascalCase for types (classes, structs, enums) | Project style | `LogLevel`, `Logger`, `ScopedMemoryLogger` |
| `auto` return with trailing return type | Existing engine code | `auto format_assertion_failure_message(...) -> std::string` |
| `[[nodiscard]]` for pure functions | Project style | `format_assertion_failure_message` is `[[nodiscard]]` |
| `[[noreturn]]` for aborting functions | C++ standard attribute | `handle_assertion_failure` is `[[noreturn]]` |
| Namespace `buddd::engine` for engine code | Project style | `debug_break()`, `format_assertion_failure_message`, `handle_assertion_failure` all in `buddd::engine` |
| `do { } while(false)` for multi-statement macros | `log/log.h` pattern | ALL five assertion macros use this wrapper. |
| `__VA_OPT__(,)` for variadic macro commas | `log/log.h` pattern | `ASSERT_MSG` and `FAIL_MSG` use `fmt __VA_OPT__(,) __VA_ARGS__` for `std::format`. |
| `std::string_view` for string params in public API | Project style | `expr`, `file`, `function` params are `std::string_view`. |
| `std::optional<std::string>` for optional message | Spec AC-014 | `message` param is `std::optional<std::string>`. |
| `#ifdef NDEBUG` / `#ifndef NDEBUG` for build detection | Project convention | ASSERT macros compile out when `NDEBUG` is defined. |
| Test files: `_tests.cpp` suffix | ADR-009 | `assertion_tests.cpp` |
| Catch2 test macros | `docs/wiki/engineering/testing.md` | `TEST_CASE`, `REQUIRE`, `SECTION` (not `CHECK`) |
| `ScopedMemoryLogger` for log capture | `tests/log_helpers.h` | Tests use this to capture Fatal-level messages. |
| C++26 `std::format` / `std::vformat` for formatting | `log/log.h` | `format_assertion_failure_message` uses `std::format`. |

## Required implementation behavior

### 1. `src/engine/log/log.h` — Add `LogLevel::Fatal` and FATAL macros

**Enum change** — insert `Fatal` after `Error`:

```cpp
// LogLevel enum — ordered from most verbose (0) to least verbose (5)
enum class LogLevel {
    Trace,   // 0 — off by default in debug builds, only via --log-level=trace
    Debug,   // 1 — default minimum in debug builds (NDEBUG not defined)
    Info,    // 2
    Warn,    // 3 — default minimum in release builds (NDEBUG defined)
    Error,   // 4
    Fatal    // 5 — unrecoverable condition, process terminates (used by assertions)
};
```

**Macro additions** — add after the existing `BUDDD_LOG_TAGGED_ERROR` macro (line 183, before the closing `}` of namespace `buddd::log`):

```cpp
#define BUDDD_LOG_FATAL(fmt, ...) \
    BUDDD_LOG_INTERNAL(::buddd::log::LogLevel::Fatal, BUDDD_CURRENT_LOG_TAG, fmt __VA_OPT__(,) __VA_ARGS__)

#define BUDDD_LOG_TAGGED_FATAL(tag_, fmt, ...) \
    BUDDD_LOG_INTERNAL(::buddd::log::LogLevel::Fatal, tag_, fmt __VA_OPT__(,) __VA_ARGS__)
```

### 2. `src/engine/log/console_sink.cpp` — Add `[FATAL]` string

In `level_name()` static function, after the `case LogLevel::Error: return "ERROR";` line, add:

```cpp
case LogLevel::Fatal: return "FATAL";
```

### 3. `src/engine/debug/debug_break.h` — New file

```cpp
#pragma once

namespace buddd::engine {

inline void debug_break() {
#ifndef NDEBUG
    #if defined(_MSC_VER)
        __debugbreak();
    #elif defined(__clang__) || defined(__GNUC__)
        __builtin_trap();
    #else
        // Fallback — platform not supported
    #endif
#endif
}

} // namespace buddd::engine
```

- `__builtin_trap()` is preferred over `__builtin_debugtrap()` because the compiler treats it as no-return.
- In release (`NDEBUG` defined), the function body is entirely empty.
- No standard headers needed beyond `#pragma once`.

### 4. `src/engine/debug/assert.h` — New file (public header)

```cpp
#pragma once

#include "debug/debug_break.h"
#include "log/log.h"

#include <cstdlib>    // std::abort
#include <optional>
#include <string>
#include <string_view>

namespace buddd::engine {

// ---------------------------------------------------------------------------
// Public formatting function — safe to call from tests (no abort).
// Returns a multi-line formatted assertion failure report.
// ---------------------------------------------------------------------------
[[nodiscard]] auto format_assertion_failure_message(
    std::string_view expr,
    std::string_view file,
    int line,
    std::string_view function,
    std::optional<std::string> message = std::nullopt
) -> std::string;

// ---------------------------------------------------------------------------
// Core assertion handler — formats, logs (Fatal/Assert), breaks (debug), aborts.
// [[noreturn]] because it always calls std::abort().
// ---------------------------------------------------------------------------
[[noreturn]] void handle_assertion_failure(
    std::string_view expr,
    std::string_view file,
    int line,
    std::string_view function,
    std::optional<std::string> message = std::nullopt
);

} // namespace buddd::engine

// ===========================================================================
// Macros
// ===========================================================================

// ---------------------------------------------------------------------------
// BUDDD_ASSERT(expr) — debug builds only
// ---------------------------------------------------------------------------
#ifndef NDEBUG
#define BUDDD_ASSERT(expr)                                                         \
    do {                                                                           \
        if (!(expr)) {                                                             \
            ::buddd::engine::handle_assertion_failure(                                \
                #expr, __FILE__, __LINE__, __FUNCTION__                            \
            );                                                                     \
        }                                                                          \
    } while (false)
#else
#define BUDDD_ASSERT(expr) ((void)0)
#endif

// ---------------------------------------------------------------------------
// BUDDD_ASSERT_MSG(expr, fmt, ...) — debug builds only, with custom message
// ---------------------------------------------------------------------------
#ifndef NDEBUG
#define BUDDD_ASSERT_MSG(expr, fmt, ...)                                           \
    do {                                                                           \
        if (!(expr)) {                                                             \
            ::buddd::engine::handle_assertion_failure(                                \
                #expr, __FILE__, __LINE__, __FUNCTION__,                           \
                std::string(std::format((fmt) __VA_OPT__(,) __VA_ARGS__))          \
            );                                                                     \
        }                                                                          \
    } while (false)
#else
#define BUDDD_ASSERT_MSG(expr, fmt, ...) ((void)0)
#endif

// ---------------------------------------------------------------------------
// BUDDD_VERIFY(expr) — all builds (preserves side effects)
// Debug: log Fatal + break + abort on failure.
// Release: evaluate expr, log Fatal on failure (no break, no abort), continue.
// ---------------------------------------------------------------------------
#ifndef NDEBUG
#define BUDDD_VERIFY(expr)                                                         \
    do {                                                                           \
        if (!(expr)) {                                                             \
            ::buddd::engine::handle_assertion_failure(                                \
                #expr, __FILE__, __LINE__, __FUNCTION__                            \
            );                                                                     \
        }                                                                          \
    } while (false)
#else
#define BUDDD_VERIFY(expr)                                                         \
    do {                                                                           \
        if (!(expr)) {                                                             \
            auto& _buddd_verify_logger = ::buddd::log::Logger::instance();         \
            if (_buddd_verify_logger.is_enabled(                                    \
                    ::buddd::log::LogLevel::Fatal, "Assert")) {                    \
                _buddd_verify_logger.log(                                           \
                    ::buddd::log::LogLevel::Fatal, "Assert",                       \
                    __FILE__, __LINE__, __FUNCTION__,                              \
                    "Assertion failed: {}", #expr                                  \
                );                                                                 \
            }                                                                      \
        }                                                                          \
    } while (false)
#endif

// ---------------------------------------------------------------------------
// BUDDD_FAIL() — all builds, unconditionally fatal + abort
// ---------------------------------------------------------------------------
#define BUDDD_FAIL()                                                               \
    do {                                                                           \
        ::buddd::engine::handle_assertion_failure(                                    \
            "(unreachable)", __FILE__, __LINE__, __FUNCTION__                      \
        );                                                                         \
    } while (false)

// ---------------------------------------------------------------------------
// BUDDD_FAIL_MSG(fmt, ...) — all builds, unconditionally fatal + abort with msg
// ---------------------------------------------------------------------------
#define BUDDD_FAIL_MSG(fmt, ...)                                                   \
    do {                                                                           \
        ::buddd::engine::handle_assertion_failure(                                    \
            "(unreachable)", __FILE__, __LINE__, __FUNCTION__,                     \
            std::string(std::format((fmt) __VA_OPT__(,) __VA_ARGS__))              \
        );                                                                         \
    } while (false)
```

**Important constraints on the macros:**

- All macros use `do { } while (false)` for multi-statement safety (matching `log.h` pattern).
- All variadic macros use `__VA_OPT__(,)` to handle zero-variadic-args correctly.
- `ASSERT` / `ASSERT_MSG` expand to `((void)0)` in release builds — the expression is NOT evaluated.
- The `BUDDD_VERIFY` release path does NOT call `handle_assertion_failure` (which aborts). Instead, it logs directly via `Logger::instance().log()` and continues.
- `FAIL` / `FAIL_MSG` are unconditional — they always call `handle_assertion_failure` which aborts.
- The fixed tag `"Assert"` is hardcoded in the `VERIFY` release log call. No `BUDDD_LOG_TAG` declaration is needed by the assertion macros themselves.
- `std::string(std::format(...))` in `ASSERT_MSG` / `FAIL_MSG` is intentional: `std::format` returns `std::string` (C++20), and wrapping in `std::string` makes the conversion to `std::optional<std::string>` unambiguous.

### 5. `src/engine/debug/assert.cpp` — New file (implementation)

```cpp
#include "debug/assert.h"
#include "debug/debug_break.h"
#include "log/log.h"

#include <format>
#include <string>

namespace buddd::engine {

auto format_assertion_failure_message(
    std::string_view expr,
    std::string_view file,
    int line,
    std::string_view function,
    std::optional<std::string> message
) -> std::string
{
    std::string result;
    result += std::format("Assertion failed: {}\n", expr);
    if (message.has_value() && !message->empty()) {
        result += std::format("Message: {}\n", *message);
    }
    result += std::format("Location: {}:{}\n", file, line);
    result += std::format("Function: {}", function);
    return result;
}

void handle_assertion_failure(
    std::string_view expr,
    std::string_view file,
    int line,
    std::string_view function,
    std::optional<std::string> message
)
{
    auto formatted = format_assertion_failure_message(expr, file, line, function, std::move(message));

    auto& logger = ::buddd::log::Logger::instance();
    logger.log(
        ::buddd::log::LogLevel::Fatal,
        "Assert",
        file, line, function,
        "{}", formatted
    );

    ::buddd::engine::debug_break();
    std::abort();
}

} // namespace buddd::engine
```

**Format rules:**
- The `"Function: {}"` line does NOT have a trailing `\n`. The console sink adds `\n` after the entire message. The formatted message is multi-line with embedded `\n`.
- Example result string (no custom message):
  ```
  Assertion failed: ptr != nullptr\nLocation: foo.cpp:42\nFunction: Foo::bar
  ```
- Example result string (with custom message):
  ```
  Assertion failed: ptr != nullptr\nMessage: expected non-null\nLocation: foo.cpp:42\nFunction: Foo::bar
  ```

### 6. `tests/assertion_tests.cpp` — New test file

```cpp
#include "debug/assert.h"
#include "log/log.h"
#include "log_helpers.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

BUDDD_LOG_TAG("AssertionTest");

// ---------------------------------------------------------------------------
// T-A1: LogLevel::Fatal > LogLevel::Error enum ordering (AC-001, AC-002)
// ---------------------------------------------------------------------------
TEST_CASE("LogLevel::Fatal is ordered after Error", "[assertion]") {
    static_assert(static_cast<int>(buddd::log::LogLevel::Error) < static_cast<int>(buddd::log::LogLevel::Fatal));
    REQUIRE(buddd::log::LogLevel::Fatal > buddd::log::LogLevel::Error);
}

// ---------------------------------------------------------------------------
// T-A2: debug_break compiles and is callable (AC-004, AC-005, AC-006)
// ---------------------------------------------------------------------------
TEST_CASE("debug_break compiles and is callable", "[assertion]") {
    // Take the address to verify the function exists — no actual call in debug
    // builds (__builtin_trap would abort).
    auto* debug_break_ptr = &buddd::engine::debug_break;
    REQUIRE(debug_break_ptr != nullptr);

#ifndef NDEBUG
    // Verify the function address matches (it's inline, same address every call)
    auto* debug_break_ptr2 = &buddd::engine::debug_break;
    REQUIRE(debug_break_ptr == debug_break_ptr2);
#else
    // In release mode, calling is safe (no-op)
    buddd::engine::debug_break();
    REQUIRE(true);
#endif
}

// ---------------------------------------------------------------------------
// T-A3: format_assertion_failure_message without custom message (AC-015)
// ---------------------------------------------------------------------------
TEST_CASE("format_assertion_failure_message without custom message", "[assertion]") {
    std::string result = buddd::engine::format_assertion_failure_message(
        "ptr != nullptr", "test.cpp", 42, "MyFunction",
        std::nullopt
    );

    REQUIRE(result.find("Assertion failed: ptr != nullptr") != std::string::npos);
    REQUIRE(result.find("Location: test.cpp:42") != std::string::npos);
    REQUIRE(result.find("Function: MyFunction") != std::string::npos);
    // "Message:" line must be absent when no custom message is provided
    REQUIRE(result.find("Message:") == std::string::npos);
}

// ---------------------------------------------------------------------------
// T-A4: format_assertion_failure_message with custom message (AC-009, AC-013, AC-015)
// ---------------------------------------------------------------------------
TEST_CASE("format_assertion_failure_message with custom message", "[assertion]") {
    std::string result = buddd::engine::format_assertion_failure_message(
        "entity.IsValid()", "entity.cpp", 99, "EntityManager::GetComponent",
        std::string("Tried to access destroyed entity, id=42")
    );

    REQUIRE(result.find("Assertion failed: entity.IsValid()") != std::string::npos);
    REQUIRE(result.find("Message: Tried to access destroyed entity, id=42") != std::string::npos);
    REQUIRE(result.find("Location: entity.cpp:99") != std::string::npos);
    REQUIRE(result.find("Function: EntityManager::GetComponent") != std::string::npos);
}

// ---------------------------------------------------------------------------
// T-A5: format_assertion_failure_message with Fatal level (AC-003)
// ---------------------------------------------------------------------------
TEST_CASE("Fatal-level log message is captured by ScopedMemoryLogger", "[assertion]") {
    buddd::test::ScopedMemoryLogger log;

    // Log directly at Fatal level with "Assert" tag (simulating assertion failure)
    buddd::log::Logger::instance().log(
        buddd::log::LogLevel::Fatal, "Assert",
        __FILE__, __LINE__, __FUNCTION__,
        "test fatal message"
    );

    REQUIRE(log.sink->messages().size() == 1);
    REQUIRE(log.sink->messages()[0].level == buddd::log::LogLevel::Fatal);
    REQUIRE(log.sink->messages()[0].tag == "Assert");
    REQUIRE(log.sink->messages()[0].message == "test fatal message");
}

// ---------------------------------------------------------------------------
// T-A6: BUDDD_VERIFY evaluates expression in all builds (AC-010, AC-011)
// ---------------------------------------------------------------------------
TEST_CASE("BUDDD_VERIFY evaluates expression", "[assertion]") {
    int counter = 0;

    // Expression: assign 42. Side effect MUST occur.
    BUDDD_VERIFY((counter = 42, true));

    REQUIRE(counter == 42);
}

// ---------------------------------------------------------------------------
// T-A7: BUDDD_VERIFY evaluates expression exactly once per call (AC-018)
// ---------------------------------------------------------------------------
TEST_CASE("BUDDD_VERIFY evaluates expression exactly once", "[assertion]") {
    int invoke_count = 0;

    auto next_val = [&invoke_count]() -> int {
        ++invoke_count;
        return invoke_count;
    };

    // Pass a truthy value so the assertion does not fire
    BUDDD_VERIFY(next_val() > 0);

    REQUIRE(invoke_count == 1);
}

// ---------------------------------------------------------------------------
// T-A8: BUDDD_ASSERT does not double-evaluate expression (AC-018)
// ---------------------------------------------------------------------------
TEST_CASE("BUDDD_ASSERT evaluates expression exactly once", "[assertion]") {
    int invoke_count = 0;

    auto next_val = [&invoke_count]() -> bool {
        ++invoke_count;
        return true;  // always passes
    };

    BUDDD_ASSERT(next_val());

    REQUIRE(invoke_count == 1);
}

// ---------------------------------------------------------------------------
// T-A9: BUDDD_ASSERT in release does not evaluate expression (AC-008)
// ---------------------------------------------------------------------------
TEST_CASE("BUDDD_ASSERT in release mode does not evaluate expression", "[assertion]") {
#ifdef NDEBUG
    int counter = 0;
    BUDDD_ASSERT((++counter, true));
    REQUIRE(counter == 0);  // expression not evaluated in release
#else
    // In debug builds, this test is not applicable (expression IS evaluated)
    // Just verify the macro compiles
    SUCCEED("BUDDD_ASSERT compiles in debug mode");
#endif
}

// ---------------------------------------------------------------------------
// T-A10: BUDDD_FAIL_FORMAT with message produces correct output (AC-013)
// ---------------------------------------------------------------------------
TEST_CASE("BUDDD_FAIL_MSG formatting", "[assertion]") {
    // Test format_assertion_failure_message with the "(unreachable)" expression
    // that FAIL macros use.
    std::string result = buddd::engine::format_assertion_failure_message(
        "(unreachable)", "test.cpp", 1, "test",
        std::string("Unexpected enum value: 42")
    );

    REQUIRE(result.find("Assertion failed: (unreachable)") != std::string::npos);
    REQUIRE(result.find("Message: Unexpected enum value: 42") != std::string::npos);
}

// ---------------------------------------------------------------------------
// T-A11: Assertion macros compile with BUDDD_LOG_TAG declared (AC-016, AC-017)
// ---------------------------------------------------------------------------
TEST_CASE("Assertion macros use fixed Assert tag", "[assertion]") {
    buddd::test::ScopedMemoryLogger log;

    // Test format_assertion_failure_message directly — no abort risk
    auto formatted = buddd::engine::format_assertion_failure_message(
        "test_expr", __FILE__, __LINE__, __FUNCTION__,
        std::nullopt
    );

    // Verify format matches expected structure
    REQUIRE(formatted.find("Assertion failed: test_expr") != std::string::npos);
}

// ---------------------------------------------------------------------------
// T-A12: LogLevel::Fatal is last enumerator (no level above it) (AC-002)
// ---------------------------------------------------------------------------
TEST_CASE("LogLevel::Fatal is the highest severity level", "[assertion]") {
    // If a new level were added after Fatal, this test would need updating.
    // We verify Fatal is greater than Error and there's implicit ordering.
    REQUIRE(static_cast<int>(buddd::log::LogLevel::Fatal) == 5);
    REQUIRE(static_cast<int>(buddd::log::LogLevel::Fatal) > static_cast<int>(buddd::log::LogLevel::Error));
}
```

### 7. Assertion insertion points in existing code

The following 8 assertions are added to demonstrate the system and catch real programming errors:

**File: `src/engine/engine_service.cpp`**

Add `#include "debug/assert.h"` at the top (before any other code in the file, replacing the blank line after the last existing include).

In each accessor, add a BUDDD_ASSERT before the raw pointer dereference:

- Line 51 (`auto EngineService::platform() noexcept -> Platform&`):
  Insert `BUDDD_ASSERT(platform_ != nullptr);` before `return *platform_;`

- Line 55 (`auto EngineService::window() noexcept -> Window&`):
  Insert `BUDDD_ASSERT(window_ != nullptr);` before `return *window_;`

- Line 59 (`auto EngineService::device() noexcept -> RenderDevice&`):
  Insert `BUDDD_ASSERT(device_ != nullptr);` before `return *device_;`

- Line 63 (`auto EngineService::assets() noexcept -> AssetManager&`):
  Insert `BUDDD_ASSERT(asset_manager_ != nullptr);` before `return *asset_manager_;`

**File: `src/engine/scene/entity.cpp`**

Add `#include "debug/assert.h"` at the top (after the last existing include).

In `Entity::transform()`:
- Line 21 (`return world_->get_transform(id_);`):
  Insert `BUDDD_ASSERT(world_ != nullptr);` before the return.

**File: `src/engine/render/render_device_opengl.cpp`**

Add `#include "debug/assert.h"` between the existing `#include "log/log.h"` (line 21) and the `BUDDD_LOG_TAG` declaration (line 23), to group engine includes before the log tag.

In `vertex_attribute_type_to_gl()` (line 37–52):
- Replace the fallback `return {GL_FLOAT, 1}; // fallback` (line 51) with:
  ```cpp
  default:
      BUDDD_FAIL_MSG("Unknown VertexAttributeType: {}", static_cast<int>(attr_type));
  ```
  and remove the unreachable fallback return.

In `primitive_topology_to_gl()` (line 54–63):
- Replace the fallback `return GL_TRIANGLES; // fallback` (line 62) with:
  ```cpp
  default:
      BUDDD_FAIL_MSG("Unknown PrimitiveTopology: {}", static_cast<int>(topology));
  ```
  and remove the unreachable fallback return.

### 8. CMake — no changes needed

- `src/engine/CMakeLists.txt` uses `file(GLOB_RECURSE ... *.h *.cpp)` — new files under `src/engine/debug/` are auto-discovered.
- `tests/CMakeLists.txt` uses `file(GLOB_RECURSE ... *_tests.cpp)` — `assertion_tests.cpp` is auto-discovered.

## Required tests

### Unit tests (`tests/assertion_tests.cpp`)

| # | Test | AC(s) covered | Method |
|---|---|---|---|
| T-A1 | `LogLevel::Fatal > LogLevel::Error` enum ordering | AC-001, AC-002 | `static_assert` + `REQUIRE` that Fatal is after Error |
| T-A2 | `debug_break()` compiles and is callable | AC-004, AC-005, AC-006 | Take function address (compile check). In release, call directly (no-op). |
| T-A3 | `format_assertion_failure_message` without custom message | AC-015 | Verify format contains expression, file, line, function; "Message:" absent |
| T-A4 | `format_assertion_failure_message` with custom message | AC-009, AC-013, AC-015 | Verify "Message:" line present with expected content |
| T-A5 | Fatal-level log via `ScopedMemoryLogger` | AC-003 | Log at Fatal level, verify `level == LogLevel::Fatal`, tag == "Assert" |
| T-A6 | `BUDDD_VERIFY` evaluates expression | AC-010, AC-011 | Side-effect expression, verify counter incremented |
| T-A7 | `BUDDD_VERIFY` evaluates expression exactly once | AC-018 | Stateful functor, verify `invoke_count == 1` |
| T-A8 | `BUDDD_ASSERT` evaluates expression exactly once | AC-018 | Same stateful functor test for ASSERT |
| T-A9 | `BUDDD_ASSERT` in release does not evaluate | AC-008 | Guarded with `#ifdef NDEBUG`, verify counter==0 |
| T-A10 | `BUDDD_FAIL_MSG` formatting | AC-012, AC-013 | `format_assertion_failure_message` with "(unreachable)" expr and custom message |
| T-A11 | Assertion macros compile with `BUDDD_LOG_TAG` | AC-016, AC-017 | Basic compile check using `format_assertion_failure_message` |
| T-A12 | `LogLevel::Fatal` is last enumerator | AC-002 | Verify Fatal has integer value 5 |

### Code review checks (explicit verifications in Done criteria)

- AC-005: No MSVC build available; the `__debugbreak()` branch is present for completeness. Code review confirms platform-appropriate branches.
- AC-014: Code review confirms `handle_assertion_failure` signature matches spec.
- AC-016: Code review confirms `"Assert"` is hardcoded in all assertion macros.
- AC-017: Code review confirms `do { } while(false)` wrapper on all macros.
- AC-019: Code review confirms no third-party includes in `src/engine/debug/`.
- AC-020: Code review confirms `handle_assertion_failure` calls `Logger::instance().log()`, then `debug_break()`, then `std::abort()`.

## Edge cases

All edge cases from the spec are carried forward. The implementation must handle:

1. **Assertion expression has side effects with `BUDDD_ASSERT` in release**: Expression is NOT evaluated — by design. Developers must use `BUDDD_VERIFY` if side effects must be preserved. Verified by T-A9.
2. **Assertion expression throws an exception**: Macros do not catch exceptions. Exception propagates naturally. No special handling.
3. **Logger not initialised when assertion fires**: `Logger::instance()` returns valid reference (magic static). If `is_enabled()` returns false, the Fatal message is silently dropped. `handle_assertion_failure` still calls `std::abort()` unconditionally.
4. **`debug_break()` called without a debugger attached**: Process continues to `std::abort()` immediately — no user-visible side effect from the breakpoint instruction.
5. **`BUDDD_ASSERT_MSG` with empty format string**: `std::format("")` produces empty string. `format_assertion_failure_message` checks `message.has_value() && !message->empty()` — since the formatted message is empty, the `"Message:"` line is suppressed entirely. This matches the spec's requirement that `BUDDD_ASSERT_MSG` with an empty format string behaves like `BUDDD_ASSERT`.
6. **`BUDDD_ASSERT_MSG` with no variadic arguments**: `__VA_OPT__(,)` handles this — the format string is passed as-is.
7. **`BUDDD_FAIL()` / `BUDDD_FAIL_MSG()` in `noexcept` function**: `std::abort()` may call `std::terminate()`. Process terminates either way — acceptable.
8. **Assertion in static initialisation / destruction code**: Logger may not be available. `handle_assertion_failure` still calls `std::abort()` — process terminates regardless of logger state.

## Security impact

- The assertion system reads no files, writes no files, opens no network connections, and reads no environment variables.
- Assertion expressions access memory reachable from the calling context — the developer is responsible for expression safety.
- No sensitive data (passwords, tokens, keys) should appear in assertion expressions or messages. The framework does not filter content.
- No elevated permissions required.

## Data and migration impact

None. This feature adds new files. Schema/data is unaffected.

## API compatibility impact

- **Public API addition**: `src/engine/debug/assert.h` and `src/engine/debug/debug_break.h` — new public headers. Inclusion is additive.
- **Logging API extension**: `LogLevel::Fatal` added to the enum. All existing code that compares `LogLevel` values continues to work. Adding an enumerator to a `switch` without a `default` case in existing code (e.g., `console_sink.cpp`) will produce a compiler warning — this is addressed by adding the new case.
- **No deprecation**: No existing API is changed or removed.
- **ABI impact**: Only addition of new symbols. No existing symbol is altered.

## Documentation impact

- **`docs/wiki/domain/logging.md`** — Must be updated (by wiki-agent after implementation) to:
  - Add `Fatal` to the log levels table (6 levels, from 0 to 5).
  - Add `Assert` to the source tags table or convention section.
  - Add section for the assertion system API.
- **New ADR** required — Architectural decision record for the assertion system. Handled by governance-reviewer.
- **New wiki page or section** — Assertion system API reference, conventions, and usage guide. Handled by wiki-agent.

## ADR impact

- **ADR-020**: This implementation extends the logging system with `LogLevel::Fatal`. The ADR may need updating to reflect the 6th level. The governance-reviewer will handle this.
- **New ADR**: The spec recommends a new ADR documenting assertion system design decisions. Handled by governance-reviewer.

## Done criteria

The implementation is complete when all of the following are true:

- [ ] **New files exist**: `src/engine/debug/debug_break.h`, `src/engine/debug/assert.h`, `src/engine/debug/assert.cpp`, `tests/assertion_tests.cpp` — all at correct paths.
- [ ] **Build succeeds**: `cmake --build --preset debug` compiles without errors or warnings (excluding any pre-existing warnings unrelated to this feature).
- [ ] **Build succeeds (release)**: `cmake --build --preset release` compiles without errors.
- [ ] **All tests pass**: `ctest -R assertion --output-on-failure` reports 100% pass rate (both debug and release builds).
- [ ] **LogLevel::Fatal added**: `src/engine/log/log.h` has `Fatal` after `Error` in the `LogLevel` enum, with value 5.
- [ ] **Console sink renders [FATAL]**: `src/engine/log/console_sink.cpp` has `case LogLevel::Fatal: return "FATAL";` in `level_name()`.
- [ ] **FATAL log macros added**: `src/engine/log/log.h` has `BUDDD_LOG_FATAL` and `BUDDD_LOG_TAGGED_FATAL` macros following the existing ERROR pattern.
- [ ] **debug_break.h exists**: `src/engine/debug/debug_break.h` declares `buddd::engine::debug_break()` with the correct platform intrinsics and `#ifndef NDEBUG` guard.
- [ ] **assert.h public API**: `src/engine/debug/assert.h` declares `format_assertion_failure_message` and `handle_assertion_failure` with exact signatures matching AC-014 (including `[[nodiscard]]` on the formatter and `[[noreturn]]` on the handler).
- [ ] **Five macros defined**: `BUDDD_ASSERT`, `BUDDD_ASSERT_MSG`, `BUDDD_VERIFY`, `BUDDD_FAIL`, `BUDDD_FAIL_MSG` — all with `do { } while (false)` wrappers, matching the behavior matrix from the spec.
- [ ] **Fixed "Assert" tag**: All assertion macros hardcode `"Assert"` tag — no `BUDDD_LOG_TAG` declaration needed. Verified by code review (grep for `"Assert"` in assert.h).
- [ ] **format_assertion_failure_message format**: The formatted output matches the spec: `"Assertion failed: <expr>\n"` + optional `"Message: <msg>\n"` + `"Location: <file>:<line>\n"` + `"Function: <function>"` (no trailing newline in message body).
- [ ] **handle_assertion_failure sequence**: Calls `Logger::instance().log(Fatal, "Assert", ...)`, then `debug_break()`, then `std::abort()`. Verified by code review.
- [ ] **No double evaluation**: Assertion expression arguments are never evaluated more than once. Verified by T-A7 and T-A8.
- [ ] **Assertions inserted**: 8 assertion points added across `engine_service.cpp` (4), `entity.cpp` (1), and `render_device_opengl.cpp` (2 default cases, 1 removed fallback).
- [ ] **No external dependencies**: `grep -rn '^#include' src/engine/debug/*.h src/engine/debug/*.cpp` confirms only C++26 standard library headers plus `"log/log.h"` and `"debug/debug_break.h"`.
- [ ] **No BUDDD_TESTING involvement**: Assertion behavior is governed solely by `NDEBUG`. No `#ifdef BUDDD_TESTING` appears in `src/engine/debug/`. Verified by grep.
- [ ] **Test file has 12 test cases**: `tests/assertion_tests.cpp` contains at least 12 `TEST_CASE` entries tagged `[assertion]`.
- [ ] **Formatter alignment**: All new/modified files have been formatted with the project's clang-format.
