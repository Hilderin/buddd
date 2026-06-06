#include "test_helpers.h"
#include "app_config.h"
#include "log/file_sink.h"
#include "log/memory_sink.h"
#include "log/logger.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// CLI command process-level tests (tagged [cli])
// ---------------------------------------------------------------------------

TEST_CASE("buddd version outputs correct version string", "[cli]") {
    const auto res = run_buddd("version");

    REQUIRE(res.exit_code == 0);
    REQUIRE(res.stdout_str == "buddd 0.1.0\n");
}

TEST_CASE("buddd help outputs usage text", "[cli]") {
    const auto res = run_buddd("help");

    REQUIRE(res.exit_code == 0);
    REQUIRE(res.stdout_str.find("Usage: buddd <command> [<args>]") != std::string::npos);
    REQUIRE(res.stdout_str.find("run") != std::string::npos);
    REQUIRE(res.stdout_str.find("version") != std::string::npos);
    REQUIRE(res.stdout_str.find("help") != std::string::npos);
    // demo was removed; capture should not appear either
    REQUIRE(res.stdout_str.find("demo") == std::string::npos);
    REQUIRE(res.stdout_str.find("capture") == std::string::npos);
}

TEST_CASE("buddd unknowncommand exits with code 1", "[cli]") {
    const auto res = run_buddd("unknowncommand");

    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("[ERROR] [App] Unknown command: 'unknowncommand'") != std::string::npos);
    REQUIRE(res.stderr_str.find("Usage: buddd <command> [<args>]") != std::string::npos);
}

TEST_CASE("buddd version ignores extra arguments", "[cli]") {
    const auto res = run_buddd("version extra_arg");

    REQUIRE(res.exit_code == 0);
    REQUIRE(res.stdout_str == "buddd 0.1.0\n");
}

TEST_CASE("buddd help ignores extra arguments", "[cli]") {
    const auto res = run_buddd("help extra_arg");

    REQUIRE(res.exit_code == 0);
    REQUIRE(res.stdout_str.find("Usage: buddd <command> [<args>]") != std::string::npos);
    REQUIRE(res.stdout_str.find("run") != std::string::npos);
    REQUIRE(res.stdout_str.find("version") != std::string::npos);
    REQUIRE(res.stdout_str.find("help") != std::string::npos);
}

TEST_CASE("buddd demo is unknown command", "[cli]") {
    // 'buddd demo' was removed; 'buddd demo unknown' should show unknown command.
    const auto res = run_buddd("demo unknown");

    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("[ERROR] [App] Unknown command: 'demo'") != std::string::npos);
}

TEST_CASE("buddd capture prints unknown command", "[cli]") {
    // Old 'capture' subcommand is removed; 'buddd capture' should produce unknown command error.
    const auto res = run_buddd("capture cube");

    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("[ERROR] [App] Unknown command: 'capture'") != std::string::npos);
    REQUIRE(res.stderr_str.find("Usage: buddd <command> [<args>]") != std::string::npos);
}

TEST_CASE("buddd run unknownscene prints error", "[cli]") {
    const auto res = run_buddd("run unknownscene");

    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("[ERROR] [App] Unknown scene: 'unknownscene'") != std::string::npos);
    REQUIRE(res.stderr_str.find("Usage: buddd run") != std::string::npos);
}

TEST_CASE("buddd test is unknown command", "[cli]") {
    const auto res = run_buddd("test");

    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("[ERROR] [App] Unknown command: 'test'") != std::string::npos);
    REQUIRE(res.stderr_str.find("Usage: buddd <command> [<args>]") != std::string::npos);
}

TEST_CASE("buddd with no arguments defaults to run command", "[cli]") {
    const auto binary = buddd_binary_path();
    const auto out_file = temp_filename("buddd_run_out");
    const auto err_file = temp_filename("buddd_run_err");

    // Use offscreen SDL driver to prevent a real window from popping up
    // during test runs (the run command opens a 1024x768 SDL3 window).
    static_cast<void>(setenv("SDL_VIDEO_DRIVER", "offscreen", 1));

    const std::string shell_cmd = "timeout 2 \"" + binary + "\" > \""
                                  + out_file + "\" 2> \"" + err_file + "\" || true";

    const int sys_ret = std::system(shell_cmd.c_str());

    unsetenv("SDL_VIDEO_DRIVER");
    (void)sys_ret;

    auto read_file = [](const std::string& path) -> std::string {
        std::ifstream f(path, std::ios::binary);
        if (!f) return {};
        return std::string((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
    };

    const auto stdout_str = read_file(out_file);
    const auto stderr_str = read_file(err_file);
    std::remove(out_file.c_str());
    std::remove(err_file.c_str());

    REQUIRE(stderr_str.find("[INFO] [App] Window opened: 1024x768") != std::string::npos);
}

// ---------------------------------------------------------------------------
// parse_logging_args() unit tests
// ---------------------------------------------------------------------------

// Helper: create a mutable argv array from a vector of strings
// (parse_logging_args needs char** not const char**)
struct Argv {
    std::vector<std::string> storage;
    std::vector<char*> ptrs;

    explicit Argv(std::initializer_list<std::string> args) {
        for (auto& s : args) {
            storage.push_back(s);
        }
        for (auto& s : storage) {
            ptrs.push_back(s.data());
        }
    }

    auto argc() const -> int { return static_cast<int>(ptrs.size()); }
    auto argv() -> char** { return ptrs.data(); }
};

TEST_CASE("parse_logging_args --log-level values", "[cli][logging]") {
    auto check_level = [](const char* level_str, buddd::log::LogLevel expected) {
        Argv args{"program", "--log-level", level_str};
        // Malformed: no '=' means the parser sees "--log-level" without "=value"
        // This is an unknown flag — silently ignored, so level stays default.
        // Use proper --log-level=<level> syntax instead.
        Argv args2{"program", std::string("--log-level=") + level_str};
        auto res = buddd::cmd::parse_logging_args(args2.argc(), args2.argv(), 1);
        REQUIRE(res.has_value());
        REQUIRE(res->global_min_level == expected);
    };

    check_level("trace", buddd::log::LogLevel::Trace);
    check_level("debug", buddd::log::LogLevel::Debug);
    check_level("info",  buddd::log::LogLevel::Info);
    check_level("warn",  buddd::log::LogLevel::Warn);
    check_level("error", buddd::log::LogLevel::Error);
}

TEST_CASE("parse_logging_args --log-level invalid returns error", "[cli][logging]") {
    Argv args{"program", "--log-level=invalid"};
    auto res = buddd::cmd::parse_logging_args(args.argc(), args.argv(), 1);
    REQUIRE_FALSE(res.has_value());
    auto err_msg = res.error().message;
    REQUIRE(err_msg.find("invalid") != std::string::npos);
}

TEST_CASE("parse_logging_args --log-file creates FileSink", "[cli][logging]") {
    char tmp_path[] = "/tmp/buddd_test_logfile_XXXXXX";
    int fd = ::mkstemp(tmp_path);
    REQUIRE(fd != -1);
    ::close(fd);
    ::unlink(tmp_path);

    std::string flag = std::string("--log-file=") + tmp_path;
    Argv args{"program", flag.c_str()};
    auto res = buddd::cmd::parse_logging_args(args.argc(), args.argv(), 1);
    REQUIRE(res.has_value());

    // Should have a FileSink in sinks
    bool has_file_sink = false;
    for (const auto& sink : res->sinks) {
        if (sink) {
            auto* fs = dynamic_cast<buddd::log::FileSink*>(sink.get());
            if (fs) {
                has_file_sink = true;
                break;
            }
        }
    }
    REQUIRE(has_file_sink);

    ::unlink(tmp_path);
}

TEST_CASE("parse_logging_args --log-file invalid path does not crash", "[cli][logging]") {
    Argv args{"program", "--log-file=/nonexistent/dir/log.txt"};
    auto res = buddd::cmd::parse_logging_args(args.argc(), args.argv(), 1);
    REQUIRE(res.has_value());

    // No FileSink should be added (factory returned nullptr)
    bool has_file_sink = false;
    for (const auto& sink : res->sinks) {
        if (dynamic_cast<buddd::log::FileSink*>(sink.get())) {
            has_file_sink = true;
        }
    }
    REQUIRE_FALSE(has_file_sink);
}

TEST_CASE("parse_logging_args --log-filter tag=level", "[cli][logging]") {
    Argv args{"program", "--log-filter=Asset:ModelLoader=trace"};
    auto res = buddd::cmd::parse_logging_args(args.argc(), args.argv(), 1);
    REQUIRE(res.has_value());

    REQUIRE(res->tag_overrides.size() == 1);
    REQUIRE(res->tag_overrides[0].first == "Asset:ModelLoader");
    REQUIRE(res->tag_overrides[0].second == buddd::log::LogLevel::Trace);
}

TEST_CASE("parse_logging_args --log-filter prefix match", "[cli][logging]") {
    Argv args{"program", "--log-filter=Asset=trace"};
    auto res = buddd::cmd::parse_logging_args(args.argc(), args.argv(), 1);
    REQUIRE(res.has_value());

    REQUIRE(res->tag_overrides.size() == 1);
    REQUIRE(res->tag_overrides[0].first == "Asset");
    REQUIRE(res->tag_overrides[0].second == buddd::log::LogLevel::Trace);
}

TEST_CASE("parse_logging_args --log-filter without level uses global level", "[cli][logging]") {
    Argv args{"program", "--log-level=info", "--log-filter=Asset"};
    auto res = buddd::cmd::parse_logging_args(args.argc(), args.argv(), 1);
    REQUIRE(res.has_value());

    REQUIRE(res->tag_overrides.size() == 1);
    REQUIRE(res->tag_overrides[0].first == "Asset");
    // Should use global level (info)
    REQUIRE(res->tag_overrides[0].second == buddd::log::LogLevel::Info);
}

TEST_CASE("parse_logging_args --log-filter invalid level returns error", "[cli][logging]") {
    Argv args{"program", "--log-filter=Asset=invalid"};
    auto res = buddd::cmd::parse_logging_args(args.argc(), args.argv(), 1);
    REQUIRE_FALSE(res.has_value());
    auto err_msg = res.error().message;
    REQUIRE(err_msg.find("invalid") != std::string::npos);
}

TEST_CASE("parse_logging_args multiple filters", "[cli][logging]") {
    Argv args{"program",
        "--log-filter=Render=trace",
        "--log-filter=Asset:ModelLoader=debug"
    };
    auto res = buddd::cmd::parse_logging_args(args.argc(), args.argv(), 1);
    REQUIRE(res.has_value());

    REQUIRE(res->tag_overrides.size() == 2);
    REQUIRE(res->tag_overrides[0].first == "Render");
    REQUIRE(res->tag_overrides[0].second == buddd::log::LogLevel::Trace);
    REQUIRE(res->tag_overrides[1].first == "Asset:ModelLoader");
    REQUIRE(res->tag_overrides[1].second == buddd::log::LogLevel::Debug);
}

TEST_CASE("parse_logging_args combination of all flags", "[cli][logging]") {
    Argv args{"program",
        "--log-level=debug",
        "--log-file=/tmp/buddd_test_combined.log",
        "--log-filter=Engine=warn"
    };
    auto res = buddd::cmd::parse_logging_args(args.argc(), args.argv(), 1);
    REQUIRE(res.has_value());
    REQUIRE(res->global_min_level == buddd::log::LogLevel::Debug);
    REQUIRE(res->tag_overrides.size() == 1);
    REQUIRE(res->tag_overrides[0].first == "Engine");
    REQUIRE(res->tag_overrides[0].second == buddd::log::LogLevel::Warn);
}

TEST_CASE("parse_logging_args unknown flags silently ignored", "[cli][logging]") {
    Argv args{"program", "--unknown-flag=42", "--log-level=warn"};
    auto res = buddd::cmd::parse_logging_args(args.argc(), args.argv(), 1);
    REQUIRE(res.has_value());
    REQUIRE(res->global_min_level == buddd::log::LogLevel::Warn);
}

TEST_CASE("parse_logging_args start parameter offsets correctly", "[cli][logging]") {
    // Simulate: program <command> --log-level=error
    // With start=2, argv[0]=program, argv[1]=run, argv[2]=--log-level=error
    Argv args{"program", "run", "--log-level=error"};
    auto res = buddd::cmd::parse_logging_args(args.argc(), args.argv(), 2);
    REQUIRE(res.has_value());
    REQUIRE(res->global_min_level == buddd::log::LogLevel::Error);
}

TEST_CASE("parse_logging_args no flags returns defaults", "[cli][logging]") {
    Argv args{"program"};
    auto res = buddd::cmd::parse_logging_args(args.argc(), args.argv(), 1);
    REQUIRE(res.has_value());
    // Default level depends on build type
#ifdef NDEBUG
    REQUIRE(res->global_min_level == buddd::log::LogLevel::Warn);
#else
    REQUIRE(res->global_min_level == buddd::log::LogLevel::Debug);
#endif
    REQUIRE(res->sinks.empty());
    REQUIRE(res->tag_overrides.empty());
}
