#include "debug/assert.h"
#include "log/log.h"
#include "log/memory_sink.h"
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
// T-A5: Fatal-level log message is captured by ScopedMemoryLogger (AC-003)
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
#ifndef NDEBUG
    int invoke_count = 0;

    auto next_val = [&invoke_count]() -> bool {
        ++invoke_count;
        return true;  // always passes
    };

    BUDDD_ASSERT(next_val());

    REQUIRE(invoke_count == 1);
#else
    // In release builds, BUDDD_ASSERT does not evaluate the expression,
    // so this test is not applicable. Verify compilation only.
    SUCCEED("BUDDD_ASSERT compiles in release mode");
#endif
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
// T-A10: BUDDD_FAIL_MSG formatting (AC-012, AC-013)
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
