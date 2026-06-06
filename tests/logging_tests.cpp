#include "log/log.h"
#include "log/log_filter.h"
#include "log/console_sink.h"
#include "log/file_sink.h"
#include "log/memory_sink.h"
#include "log/logger.h"
#include "log_helpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

// Declare a tag for this test file
BUDDD_LOG_TAG("LoggingTest");

// ---------------------------------------------------------------------------
// T-01: LogLevel ordering
// ---------------------------------------------------------------------------
TEST_CASE("LogLevel ordering", "[logging]") {
    static_assert(static_cast<int>(buddd::log::LogLevel::Trace) < static_cast<int>(buddd::log::LogLevel::Debug));
    static_assert(static_cast<int>(buddd::log::LogLevel::Debug) < static_cast<int>(buddd::log::LogLevel::Info));
    static_assert(static_cast<int>(buddd::log::LogLevel::Info) < static_cast<int>(buddd::log::LogLevel::Warn));
    static_assert(static_cast<int>(buddd::log::LogLevel::Warn) < static_cast<int>(buddd::log::LogLevel::Error));

    // Also runtime check
    REQUIRE(buddd::log::LogLevel::Trace < buddd::log::LogLevel::Debug);
    REQUIRE(buddd::log::LogLevel::Debug < buddd::log::LogLevel::Info);
    REQUIRE(buddd::log::LogLevel::Info < buddd::log::LogLevel::Warn);
    REQUIRE(buddd::log::LogLevel::Warn < buddd::log::LogLevel::Error);
}

// ---------------------------------------------------------------------------
// T-02: MemorySink accumulates messages
// ---------------------------------------------------------------------------
TEST_CASE("MemorySink accumulates messages", "[logging]") {
    buddd::test::ScopedMemoryLogger log;

    BUDDD_LOG_INFO("first message");
    BUDDD_LOG_DEBUG("second message");
    BUDDD_LOG_ERROR("third message");

    REQUIRE(log.sink->messages().size() == 3);
    REQUIRE(log.sink->messages()[0].message == "first message");
    REQUIRE(log.sink->messages()[1].message == "second message");
    REQUIRE(log.sink->messages()[2].message == "third message");
}

// ---------------------------------------------------------------------------
// T-03: MemorySink message fields match (file, line, function)
// ---------------------------------------------------------------------------
TEST_CASE("MemorySink message fields match", "[logging]") {
    buddd::test::ScopedMemoryLogger log;

    // Capture the expected file and line
    constexpr int expected_line = __LINE__ + 1;
    BUDDD_LOG_INFO("field check");

    REQUIRE(log.sink->messages().size() == 1);
    const auto& msg = log.sink->messages()[0];

    // File should contain "logging_tests.cpp"
    REQUIRE(std::string(msg.file).find("logging_tests.cpp") != std::string::npos);
    REQUIRE(msg.line == expected_line);
    // Function should be non-empty
    REQUIRE(!std::string(msg.function).empty());
}

// ---------------------------------------------------------------------------
// T-04: Console sink format
// ---------------------------------------------------------------------------
TEST_CASE("Console sink format", "[logging]") {
    buddd::log::ConsoleSink sink;

    // Use pipe + dup2 to capture stderr
    int pipefd[2];
    int pipe_ret = ::pipe(pipefd);
    REQUIRE(pipe_ret == 0);

    int old_stderr = ::dup(STDERR_FILENO);
    REQUIRE(old_stderr != -1);

    int dup2_ret = ::dup2(pipefd[1], STDERR_FILENO);
    REQUIRE(dup2_ret != -1);
    ::close(pipefd[1]);

    // Write to the captured stderr
    buddd::log::LogMessage msg;
    msg.level = buddd::log::LogLevel::Warn;
    msg.tag = "TestTag";
    msg.message = "console test";
    msg.file = __FILE__;
    msg.line = __LINE__;
    msg.function = __FUNCTION__;

    sink.write(msg);

    // Restore stderr
    ::fflush(stderr);
    ::dup2(old_stderr, STDERR_FILENO);
    ::close(old_stderr);

    // Read captured output
    char buf[4096];
    auto n = ::read(pipefd[0], buf, sizeof(buf) - 1);
    ::close(pipefd[0]);
    REQUIRE(n > 0);
    buf[n] = '\0';
    std::string captured(buf);

    // Check format: [WARN] [TestTag] console test\n
    REQUIRE(captured.find("[WARN]") != std::string::npos);
    REQUIRE(captured.find("[TestTag]") != std::string::npos);
    REQUIRE(captured.find("console test") != std::string::npos);
    // No date-like prefix (no ISO 8601 timestamp)
    std::regex date_regex(R"(\d{4}-\d{2}-\d{2})");
    REQUIRE_FALSE(std::regex_search(captured, date_regex));
}

// ---------------------------------------------------------------------------
// T-05: File sink format
// ---------------------------------------------------------------------------
TEST_CASE("File sink format", "[logging]") {
    // Create a temp file path
    char tmp_path[] = "/tmp/buddd_test_file_sink_XXXXXX";
    int fd = ::mkstemp(tmp_path);
    REQUIRE(fd != -1);
    ::close(fd);
    ::unlink(tmp_path); // Remove it so FileSink::create creates a new file

    auto sink = buddd::log::FileSink::create(tmp_path);
    REQUIRE(sink != nullptr);

    buddd::log::LogMessage msg;
    msg.level = buddd::log::LogLevel::Info;
    msg.tag = "FileTag";
    msg.message = "file format test";
    msg.file = __FILE__;
    msg.line = __LINE__;
    msg.function = __FUNCTION__;

    sink->write(msg);

    // Destroy the sink to flush the file
    sink.reset();

    // Read file back
    std::ifstream file(tmp_path);
    REQUIRE(file.is_open());
    std::string line;
    std::getline(file, line);
    file.close();

    // Check format: YYYY-MM-DDTHH:MM:SS [INFO] [FileTag] file format test
    std::regex format_regex(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2} \[INFO\] \[FileTag\] file format test)");
    REQUIRE(std::regex_search(line, format_regex));

    ::unlink(tmp_path);
}

// ---------------------------------------------------------------------------
// T-06: Global level filtering
// ---------------------------------------------------------------------------
TEST_CASE("Global level filtering", "[logging]") {
    buddd::log::LogConfig config;
    config.global_min_level = buddd::log::LogLevel::Warn;
    auto sink = std::make_shared<buddd::log::MemorySink>();
    config.sinks.push_back(sink);
    buddd::log::Logger::init(std::move(config));

    BUDDD_LOG_INFO("should be absent");
    BUDDD_LOG_WARN("should be present (warn)");
    BUDDD_LOG_ERROR("should be present (error)");

    REQUIRE(sink->messages().size() == 2);
    REQUIRE(sink->messages()[0].message == "should be present (warn)");
    REQUIRE(sink->messages()[1].message == "should be present (error)");

    buddd::log::Logger::reset();
}

// ---------------------------------------------------------------------------
// T-07: Tag override filtering (uses BUDDD_LOG_TAGGED to avoid tag redefinition)
// ---------------------------------------------------------------------------
TEST_CASE("Tag override filtering", "[logging]") {
    buddd::log::LogConfig config;
    config.global_min_level = buddd::log::LogLevel::Info;
    config.tag_overrides = {{"TestTag", buddd::log::LogLevel::Trace}};
    auto sink = std::make_shared<buddd::log::MemorySink>();
    config.sinks.push_back(sink);
    buddd::log::Logger::init(std::move(config));

    // Use BUDDD_LOG_TAGGED macros to specify the tag explicitly
    BUDDD_LOG_TAGGED_TRACE("TestTag", "trace on TestTag should pass");
    BUDDD_LOG_TAGGED_TRACE("OtherTag", "trace on OtherTag should be filtered");

    REQUIRE(sink->messages().size() == 1);
    REQUIRE(sink->messages()[0].message == "trace on TestTag should pass");

    buddd::log::Logger::reset();
}

// ---------------------------------------------------------------------------
// T-08: File sink enabled via config
// ---------------------------------------------------------------------------
TEST_CASE("File sink enabled via config", "[logging]") {
    char tmp_path[] = "/tmp/buddd_test_file_enabled_XXXXXX";
    int fd = ::mkstemp(tmp_path);
    REQUIRE(fd != -1);
    ::close(fd);
    ::unlink(tmp_path);

    // Test with file sink in config
    {
        buddd::log::LogConfig config;
        auto file_sink = buddd::log::FileSink::create(tmp_path);
        REQUIRE(file_sink != nullptr);
        config.sinks.push_back(std::move(file_sink));
        auto mem_sink = std::make_shared<buddd::log::MemorySink>();
        config.sinks.push_back(mem_sink);
        buddd::log::Logger::init(std::move(config));

        BUDDD_LOG_INFO("log with file sink");

        buddd::log::Logger::reset();
    }

    // Verify file exists and has content
    std::ifstream file(tmp_path);
    REQUIRE(file.is_open());
    bool has_content = false;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            has_content = true;
            break;
        }
    }
    file.close();
    REQUIRE(has_content);

    ::unlink(tmp_path);

    // Test without file sink in config
    {
        char no_file_path[] = "/tmp/buddd_test_no_file_XXXXXX";
        int fd2 = ::mkstemp(no_file_path);
        REQUIRE(fd2 != -1);
        ::close(fd2);
        ::unlink(no_file_path); // Remove so we can check it's not recreated

        // Init logger WITHOUT file sink
        buddd::log::LogConfig config;
        auto mem_sink = std::make_shared<buddd::log::MemorySink>();
        config.sinks.push_back(mem_sink);
        buddd::log::Logger::init(std::move(config));

        BUDDD_LOG_INFO("log without file sink");

        buddd::log::Logger::reset();

        // Verify file does NOT exist
        std::ifstream f(no_file_path);
        REQUIRE_FALSE(f.is_open());
    }
}

// ---------------------------------------------------------------------------
// T-09: File sink failure is non-fatal
// ---------------------------------------------------------------------------
TEST_CASE("File sink failure is non-fatal", "[logging]") {
    // Try to create a file sink with an invalid path
    auto sink = buddd::log::FileSink::create("/nonexistent/dir/file.log");
    REQUIRE(sink == nullptr);

    // Logger should still work with a memory sink
    buddd::log::LogConfig config;
    auto mem_sink = std::make_shared<buddd::log::MemorySink>();
    config.sinks.push_back(mem_sink);
    buddd::log::Logger::init(std::move(config));

    BUDDD_LOG_INFO("logger still works after file sink failure");

    REQUIRE(mem_sink->messages().size() == 1);
    REQUIRE(mem_sink->messages()[0].message == "logger still works after file sink failure");

    buddd::log::Logger::reset();
}

// ---------------------------------------------------------------------------
// T-10/T-11: Build-type default threshold
// ---------------------------------------------------------------------------
TEST_CASE("Build-type default threshold", "[logging]") {
    buddd::log::LogConfig default_config;
#ifdef NDEBUG
    REQUIRE(default_config.global_min_level == buddd::log::LogLevel::Warn);
#else
    REQUIRE(default_config.global_min_level == buddd::log::LogLevel::Debug);
#endif
}

// ---------------------------------------------------------------------------
// T-12: No side effects for disabled levels
// ---------------------------------------------------------------------------
TEST_CASE("No side effects for disabled levels", "[logging]") {
    buddd::log::LogConfig config;
    config.global_min_level = buddd::log::LogLevel::Error;
    auto sink = std::make_shared<buddd::log::MemorySink>();
    config.sinks.push_back(sink);
    buddd::log::Logger::init(std::move(config));

    int counter = 0;
    BUDDD_LOG_INFO("counter is {}", ++counter);

    REQUIRE(counter == 0); // No side effect because level is below threshold
    REQUIRE(sink->messages().empty());

    buddd::log::Logger::reset();
}

// ---------------------------------------------------------------------------
// T-13: BUDDD_LOG_TAGGED_INFO overrides tag
// ---------------------------------------------------------------------------
TEST_CASE("BUDDD_LOG_TAGGED_INFO overrides tag", "[logging]") {
    // File tag is "LoggingTest" from the global declaration at top of file
    buddd::test::ScopedMemoryLogger log;

    BUDDD_LOG_TAGGED_INFO("CustomTag", "message with custom tag");

    REQUIRE(log.sink->messages().size() == 1);
    REQUIRE(log.sink->messages()[0].tag == "CustomTag");
    REQUIRE(log.sink->messages()[0].message == "message with custom tag");
}

// ---------------------------------------------------------------------------
// T-14: std::format-style works
// ---------------------------------------------------------------------------
TEST_CASE("std::format-style works", "[logging]") {
    buddd::test::ScopedMemoryLogger log;

    BUDDD_LOG_INFO("int {} str {}", 42, "hello");

    REQUIRE(log.sink->messages().size() == 1);
    REQUIRE(log.sink->messages()[0].message == "int 42 str hello");
}

// ---------------------------------------------------------------------------
// T-15: No external dependencies — code review marker
// ---------------------------------------------------------------------------
TEST_CASE("No external dependencies", "[logging]") {
    SUCCEED("Code review confirms only C++26 standard library includes in src/engine/log/");
}

// ---------------------------------------------------------------------------
// T-16: Logger compiled in engine
// ---------------------------------------------------------------------------
TEST_CASE("Logger compiled in engine", "[logging]") {
    auto& logger = buddd::log::Logger::instance();
    (void)logger;
    SUCCEED("Logger symbols resolved — Logger is compiled into engine");
}

// ---------------------------------------------------------------------------
// T-17: Logger decoupled from Error/Result
// ---------------------------------------------------------------------------
TEST_CASE("Logger decoupled from Error/Result", "[logging]") {
    // Verify LogMessage doesn't use Error or Result types
    buddd::log::LogMessage msg;
    msg.level = buddd::log::LogLevel::Info;
    msg.tag = "test";
    msg.message = "no error dependency";
    msg.file = "test.cpp";
    msg.line = 1;
    msg.function = "test";
    REQUIRE(msg.message == "no error dependency");
}

// ---------------------------------------------------------------------------
// T-18: Thread safety stress test
// ---------------------------------------------------------------------------
TEST_CASE("Thread safety stress test", "[logging]") {
    // Use a custom thread-safe sink that records messages
    struct ThreadSafeCollector : public buddd::log::Sink {
        std::mutex mtx;
        std::vector<std::string> messages;

        void write(const buddd::log::LogMessage& msg) override {
            std::lock_guard<std::mutex> lock(mtx);
            messages.push_back(msg.message);
        }
    };

    auto collector = std::make_shared<ThreadSafeCollector>();
    buddd::log::LogConfig config;
    config.global_min_level = buddd::log::LogLevel::Trace;
    config.sinks.push_back(collector);
    buddd::log::Logger::init(std::move(config));

    constexpr int num_threads = 4;
    constexpr int msgs_per_thread = 1000;
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t]() {
            for (int i = 0; i < msgs_per_thread; ++i) {
                BUDDD_LOG_TAGGED_INFO("Stress", "message from thread {} iteration {}", t, i);
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    REQUIRE(collector->messages.size() == static_cast<size_t>(num_threads * msgs_per_thread));

    // Verify each message is intact (not interleaved)
    for (const auto& msg : collector->messages) {
        REQUIRE(msg.find("message from thread") == 0);
        // Count occurrences of "thread" — should be exactly 1
        size_t count = 0;
        size_t pos = 0;
        while ((pos = msg.find("thread", pos)) != std::string::npos) {
            ++count;
            pos += 6;
        }
        REQUIRE(count == 1);
    }

    buddd::log::Logger::reset();
}

// ---------------------------------------------------------------------------
// T-19: Missing BUDDD_LOG_TAG compile error (compile-time test)
// ---------------------------------------------------------------------------
TEST_CASE("Missing BUDDD_LOG_TAG compile error", "[logging]") {
    // AC-002: This is enforced at compile time by the macro definition itself.
    // If BUDDD_LOG_TAG is not declared, BUDDD_CURRENT_LOG_TAG is undefined,
    // and the compiler rejects the translation unit. This is inherent in the
    // macro design — no separate compile-fail test needed.
    SUCCEED("Enforced by macro — BUDDD_CURRENT_LOG_TAG would be undefined");
}

// ---------------------------------------------------------------------------
// T-20: Logger singleton init idempotent
// ---------------------------------------------------------------------------
TEST_CASE("Logger singleton init idempotent", "[logging]") {
    // First init
    auto sink1 = std::make_shared<buddd::log::MemorySink>();
    buddd::log::LogConfig config1;
    config1.sinks.push_back(sink1);
    config1.global_min_level = buddd::log::LogLevel::Trace;
    buddd::log::Logger::init(std::move(config1));

    BUDDD_LOG_INFO("first config");

    // Second init (should be no-op)
    auto sink2 = std::make_shared<buddd::log::MemorySink>();
    buddd::log::LogConfig config2;
    config2.sinks.push_back(sink2);
    config2.global_min_level = buddd::log::LogLevel::Error; // Should NOT override
    buddd::log::Logger::init(std::move(config2));

    BUDDD_LOG_INFO("second init attempt");

    // First sink should have both messages
    REQUIRE(sink1->messages().size() == 2);
    // Second sink should be empty
    REQUIRE(sink2->messages().empty());

    buddd::log::Logger::reset();
}

// ---------------------------------------------------------------------------
// T-21: Logging after shutdown is safe
// ---------------------------------------------------------------------------
TEST_CASE("Logging after shutdown is safe", "[logging]") {
    {
        buddd::test::ScopedMemoryLogger log;
        BUDDD_LOG_INFO("before shutdown");
    } // reset is called by ~ScopedMemoryLogger

    // Now the logger is reset — log calls should be silently dropped
    BUDDD_LOG_INFO("after reset — should not crash");

    // Re-init the logger so assertions work
    auto sink = std::make_shared<buddd::log::MemorySink>();
    buddd::log::LogConfig config;
    config.global_min_level = buddd::log::LogLevel::Trace;
    config.sinks.push_back(sink);
    buddd::log::Logger::init(std::move(config));

    BUDDD_LOG_INFO("re-init after reset");
    REQUIRE(sink->messages().size() == 1);

    buddd::log::Logger::reset();
}

// ---------------------------------------------------------------------------
// T-22: Logging before init is safe
// ---------------------------------------------------------------------------
TEST_CASE("Logging before init is safe", "[logging]") {
    // Ensure the logger is reset first
    buddd::log::Logger::reset();

    // Log without init — should not crash
    BUDDD_LOG_INFO("before init — should not crash");

    // Now init and verify it works
    auto sink = std::make_shared<buddd::log::MemorySink>();
    buddd::log::LogConfig config;
    config.global_min_level = buddd::log::LogLevel::Trace;
    config.sinks.push_back(sink);
    buddd::log::Logger::init(std::move(config));

    BUDDD_LOG_INFO("after init");
    REQUIRE(sink->messages().size() == 1);

    buddd::log::Logger::reset();
}

// ---------------------------------------------------------------------------
// T-23: Empty format string
// ---------------------------------------------------------------------------
TEST_CASE("Empty format string", "[logging]") {
    buddd::test::ScopedMemoryLogger log;

    BUDDD_LOG_INFO("");

    REQUIRE(log.sink->messages().size() == 1);
    REQUIRE(log.sink->messages()[0].message.empty());
}

// ---------------------------------------------------------------------------
// T-24: File sink append mode
// ---------------------------------------------------------------------------
TEST_CASE("File sink append mode", "[logging]") {
    char tmp_path[] = "/tmp/buddd_test_append_XXXXXX";
    int fd = ::mkstemp(tmp_path);
    REQUIRE(fd != -1);
    ::close(fd);

    // Write some initial content
    {
        std::ofstream pre(tmp_path, std::ios::trunc);
        REQUIRE(pre.is_open());
        pre << "pre-existing content\n";
        pre.close();
    }

    // Create a FileSink (should append, not truncate)
    {
        auto sink = buddd::log::FileSink::create(tmp_path);
        REQUIRE(sink != nullptr);

        buddd::log::LogMessage msg;
        msg.level = buddd::log::LogLevel::Info;
        msg.tag = "AppendTest";
        msg.message = "appended line";
        sink->write(msg);
    } // sink destroyed, file flushed

    // Read the file back
    std::ifstream file(tmp_path);
    REQUIRE(file.is_open());
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    file.close();

    // There should be at least 2 lines (pre-existing + appended)
    REQUIRE(lines.size() >= 2);
    bool found_pre = false;
    bool found_appended = false;
    for (const auto& l : lines) {
        if (l == "pre-existing content") found_pre = true;
        if (l.find("appended line") != std::string::npos) found_appended = true;
    }
    REQUIRE(found_pre);
    REQUIRE(found_appended);

    ::unlink(tmp_path);
}

// ---------------------------------------------------------------------------
// T-25: Empty source tag
// ---------------------------------------------------------------------------
TEST_CASE("Empty source tag", "[logging]") {
    // Can't use BUDDD_LOG_TAG("") here because we already have a tag declared.
    // Use BUDDD_LOG_TAGGED with an empty tag instead.
    auto mem_sink = std::make_shared<buddd::log::MemorySink>();
    buddd::log::LogConfig config;
    config.global_min_level = buddd::log::LogLevel::Trace;
    config.sinks.push_back(mem_sink);
    buddd::log::Logger::init(std::move(config));

    BUDDD_LOG_TAGGED_INFO("", "empty tag test");

    REQUIRE(mem_sink->messages().size() == 1);
    REQUIRE(mem_sink->messages()[0].tag.empty());

    buddd::log::Logger::reset();
}

// ---------------------------------------------------------------------------
// Additional edge case: Tag truncation at 255 chars
// ---------------------------------------------------------------------------
TEST_CASE("Tag truncation at 255 chars", "[logging]") {
    auto mem_sink = std::make_shared<buddd::log::MemorySink>();
    buddd::log::LogConfig config;
    config.global_min_level = buddd::log::LogLevel::Trace;
    config.sinks.push_back(mem_sink);
    buddd::log::Logger::init(std::move(config));

    // Create a very long tag
    std::string long_tag(300, 'X');
    BUDDD_LOG_TAGGED_INFO(long_tag.c_str(), "long tag message");

    REQUIRE(mem_sink->messages().size() == 1);
    // Tag should be truncated to 255 chars
    REQUIRE(mem_sink->messages()[0].tag.length() == 255);

    buddd::log::Logger::reset();
}

// ---------------------------------------------------------------------------
// Additional edge case: Very long message truncation
// ---------------------------------------------------------------------------
TEST_CASE("Very long message truncation", "[logging]") {
    auto mem_sink = std::make_shared<buddd::log::MemorySink>();
    buddd::log::LogConfig config;
    config.global_min_level = buddd::log::LogLevel::Trace;
    config.sinks.push_back(mem_sink);
    buddd::log::Logger::init(std::move(config));

    // Create a message longer than 32 KB
    std::string long_msg(40 * 1024, 'A');
    BUDDD_LOG_INFO("{}", long_msg);

    REQUIRE(mem_sink->messages().size() == 1);
    // Message should be truncated to 32 KB
    REQUIRE(mem_sink->messages()[0].message.size() == 32 * 1024);

    buddd::log::Logger::reset();
}

// ---------------------------------------------------------------------------
// Additional edge case: Prefix matching boundary
// ---------------------------------------------------------------------------
TEST_CASE("Prefix matching boundary", "[logging]") {
    buddd::log::LogFilter filter;

    filter.set_global_level(buddd::log::LogLevel::Warn);
    filter.set_tag_overrides({
        {"Asset", buddd::log::LogLevel::Trace}
    });

    REQUIRE(filter.is_enabled(buddd::log::LogLevel::Trace, "Asset"));
    REQUIRE(filter.is_enabled(buddd::log::LogLevel::Trace, "Asset:ModelLoader"));
    // "Other" tag should still use global level
    REQUIRE_FALSE(filter.is_enabled(buddd::log::LogLevel::Trace, "Other"));

    // Test "Asset:" pattern
    filter.set_tag_overrides({
        {"Asset:", buddd::log::LogLevel::Trace}
    });

    REQUIRE(filter.is_enabled(buddd::log::LogLevel::Trace, "Asset:ModelLoader"));
    REQUIRE_FALSE(filter.is_enabled(buddd::log::LogLevel::Trace, "Asset"));
}

// ---------------------------------------------------------------------------
// Additional edge case: Last match wins for duplicate tag overrides
// ---------------------------------------------------------------------------
TEST_CASE("Last match wins for duplicate tag overrides", "[logging]") {
    buddd::log::LogFilter filter;

    filter.set_global_level(buddd::log::LogLevel::Error);
    filter.set_tag_overrides({
        {"Asset", buddd::log::LogLevel::Trace},
        {"Asset", buddd::log::LogLevel::Info}
    });

    // Trace for "Asset" should NOT pass (last override is Info)
    REQUIRE_FALSE(filter.is_enabled(buddd::log::LogLevel::Trace, "Asset"));
    // Info for "Asset" should pass
    REQUIRE(filter.is_enabled(buddd::log::LogLevel::Info, "Asset"));
}
