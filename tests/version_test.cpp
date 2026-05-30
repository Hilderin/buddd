#include "version.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers for the CLI integration tests
// ---------------------------------------------------------------------------

/// Determine the path to the buddd binary relative to the running test.
/// Uses /proc/self/exe on Linux to find the test binary location, then derives
/// the sibling binary path under src/cmd/.
static auto buddd_binary_path() -> std::string {
    char buf[4096];
    const ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len == -1) {
        // Fallback: assume we are running from the build root
        return "./src/cmd/buddd";
    }
    buf[len] = '\0';
    std::string path(buf);

    // path looks like: .../build/debug/tests/buddd_tests
    // Chop the filename
    auto slash = path.rfind('/');
    if (slash != std::string::npos) {
        path = path.substr(0, slash);
    }
    // Now path = .../build/debug/tests
    // Go up one level and into src/cmd/buddd
    path += "/../src/cmd/buddd";
    return path;
}

/// Run the buddd binary with the given arguments and capture stdout, stderr,
/// and the exit code.
struct CommandResult {
    std::string stdout_str;
    std::string stderr_str;
    int exit_code{0};
};

/// Create a unique temporary filename in /tmp.
static auto temp_filename(const char* prefix) -> std::string {
    std::string tmpl = std::string("/tmp/") + prefix + "XXXXXX";
    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');
    const int fd = mkstemp(buf.data());
    if (fd == -1) {
        FAIL("Failed to create temp file: " << std::strerror(errno));
    }
    close(fd);
    return std::string(buf.data());
}

static auto run_buddd(const std::string& args) -> CommandResult {
    const auto binary = buddd_binary_path();

    // Create temp files for stdout and stderr
    const auto out_file = temp_filename("buddd_out");
    const auto err_file = temp_filename("buddd_err");

    // Build shell command: run binary, capture stdout and stderr separately
    const std::string shell_cmd = "\"" + binary + "\" " + args
                                  + " > \"" + out_file + "\" 2> \"" + err_file + "\"";

    const int ret = std::system(shell_cmd.c_str());
    const int exit_code = (ret != -1 && WIFEXITED(ret)) ? WEXITSTATUS(ret) : -1;

    // Read captured output
    auto read_file = [](const std::string& path) -> std::string {
        std::ifstream f(path, std::ios::binary);
        if (!f) return {};
        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
        return content;
    };

    CommandResult result;
    result.stdout_str = read_file(out_file);
    result.stderr_str = read_file(err_file);
    result.exit_code = exit_code;

    // Cleanup temp files
    std::remove(out_file.c_str());
    std::remove(err_file.c_str());

    return result;
}

// ---------------------------------------------------------------------------
// API-level unit test
// ---------------------------------------------------------------------------

TEST_CASE("engine version is non-empty", "[sanity]") {
    REQUIRE_FALSE(buddd::engine::version().empty());
}

// ---------------------------------------------------------------------------
// CLI command process-level tests (tagged [cli])
// ---------------------------------------------------------------------------

TEST_CASE("buddd version outputs correct version string", "[cli]") {
    const auto res = run_buddd("version");

    REQUIRE(res.exit_code == 0);
    // Exact output: "buddd 0.1.0\n"
    REQUIRE(res.stdout_str == "buddd 0.1.0\n");
}

TEST_CASE("buddd help outputs usage text", "[cli]") {
    const auto res = run_buddd("help");

    REQUIRE(res.exit_code == 0);
    // Must contain the usage header and all four command names
    REQUIRE(res.stdout_str.find("Usage: buddd <command> [<args>]") != std::string::npos);
    REQUIRE(res.stdout_str.find("run") != std::string::npos);
    REQUIRE(res.stdout_str.find("demo") != std::string::npos);
    REQUIRE(res.stdout_str.find("version") != std::string::npos);
    REQUIRE(res.stdout_str.find("help") != std::string::npos);
}

TEST_CASE("buddd unknowncommand exits with code 1", "[cli]") {
    const auto res = run_buddd("unknowncommand");

    REQUIRE(res.exit_code == 1);
    // stderr must contain the error and usage
    REQUIRE(res.stderr_str.find("Unknown command: 'unknowncommand'") != std::string::npos);
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
    REQUIRE(res.stdout_str.find("demo") != std::string::npos);
    REQUIRE(res.stdout_str.find("version") != std::string::npos);
    REQUIRE(res.stdout_str.find("help") != std::string::npos);
}

TEST_CASE("buddd demo with no name prints usage and exits 1", "[cli]") {
    const auto res = run_buddd("demo");

    REQUIRE(res.exit_code == 1);
    // stderr must contain the demo usage text
    REQUIRE(res.stderr_str.find("Usage: buddd demo <demo>") != std::string::npos);
    REQUIRE(res.stderr_str.find("triangle") != std::string::npos);
    REQUIRE(res.stderr_str.find("Demo names are case-sensitive.") != std::string::npos);
}

TEST_CASE("buddd demo unknownname prints error and exits 1", "[cli]") {
    const auto res = run_buddd("demo unknownname");

    REQUIRE(res.exit_code == 1);
    // stderr must contain "Unknown demo: 'unknownname'"
    REQUIRE(res.stderr_str.find("Unknown demo: 'unknownname'") != std::string::npos);
    // Must also contain the demo usage
    REQUIRE(res.stderr_str.find("Usage: buddd demo <demo>") != std::string::npos);
}

TEST_CASE("buddd test is unknown command", "[cli]") {
    const auto res = run_buddd("test");

    REQUIRE(res.exit_code == 1);
    // stderr must contain "Unknown command: 'test'"
    REQUIRE(res.stderr_str.find("Unknown command: 'test'") != std::string::npos);
    // Must also contain the updated usage block (which has "demo" not "test")
    REQUIRE(res.stderr_str.find("Usage: buddd <command> [<args>]") != std::string::npos);
}

TEST_CASE("buddd with no arguments defaults to run command", "[cli]") {
    // Run the binary with no args and use 'timeout 2' to kill it after 2 seconds.
    // The binary uses SDL3 backend (with display) or headless backend (without).
    // Headless poll_events() always returns true, so the loop runs until timeout.
    const auto binary = buddd_binary_path();
    const auto out_file = temp_filename("buddd_run_out");
    const auto err_file = temp_filename("buddd_run_err");

    const std::string shell_cmd = "timeout 2 \"" + binary + "\" > \""
                                  + out_file + "\" 2> \"" + err_file + "\" || true";

    const int sys_ret = std::system(shell_cmd.c_str());
    (void)sys_ret;

    auto read_file = [](const std::string& path) -> std::string {
        std::ifstream f(path, std::ios::binary);
        if (!f) return {};
        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
        return content;
    };

    const auto stdout_str = read_file(out_file);
    std::remove(out_file.c_str());
    std::remove(err_file.c_str());

    // The window opened message must have been printed before timeout killed it
    REQUIRE(stdout_str.find("Window opened: 1024x768") != std::string::npos);
}

TEST_CASE("buddd demo triangle runs and completes", "[cli]") {
    // Run the demo with a 5-second timeout and verify the completion message.
    // The binary uses headless backend when BUDDD_HAS_DISPLAY=OFF.
    // The triangle demo runs 120 frames (~2 seconds with frame limiting) then exits.
    const auto binary = buddd_binary_path();
    const auto out_file = temp_filename("buddd_demo_out");
    const auto err_file = temp_filename("buddd_demo_err");

    const std::string shell_cmd = "timeout 5 \"" + binary + "\" demo triangle > \""
                                  + out_file + "\" 2> \"" + err_file + "\" || true";

    const int sys_ret = std::system(shell_cmd.c_str());
    (void)sys_ret;

    auto read_file = [](const std::string& path) -> std::string {
        std::ifstream f(path, std::ios::binary);
        if (!f) return {};
        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
        return content;
    };

    const auto stderr_str = read_file(err_file);
    std::remove(out_file.c_str());
    std::remove(err_file.c_str());

    // The demo should complete (120 frames ~2 seconds, well within 5s timeout)
    REQUIRE(stderr_str.find("Demo complete: triangle (120 frames rendered)") != std::string::npos);
}
