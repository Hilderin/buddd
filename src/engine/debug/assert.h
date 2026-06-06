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
