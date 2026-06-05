#include "test_helpers.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <string>

// ---------------------------------------------------------------------------
// CLI App System integration tests (tagged [cli][app])
// These tests verify the new CLI dispatch: run, version, help, and flags.
// ---------------------------------------------------------------------------

TEST_CASE("buddd run triangle --frame 2 runs and exits", "[cli][app]") {
    const auto res = run_buddd("run triangle --frame 2");

    REQUIRE(res.exit_code == 0);
    // Should complete without errors
    REQUIRE(res.stderr_str.find("Scene complete:") != std::string::npos);
    REQUIRE(res.stderr_str.find("Buddd Engine \u2014 triangle") != std::string::npos);
}

TEST_CASE("buddd run triangle extra_arg warns and proceeds", "[cli][app]") {
    const auto res = run_buddd("run triangle --frame 2 extra_arg");

    // Limit with --frame so the process exits quickly
    REQUIRE(res.exit_code == 0);
    REQUIRE(res.stderr_str.find("Scene complete:") != std::string::npos);
    REQUIRE(res.stderr_str.find("Warning: unexpected arguments") != std::string::npos);
}

TEST_CASE("buddd run unknownscene prints error and exits 1", "[cli][app]") {
    const auto res = run_buddd("run unknownscene");

    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("Unknown scene: 'unknownscene'") != std::string::npos);
}

TEST_CASE("buddd demo is now unknown command", "[cli][app]") {
    // The 'demo' command was removed; 'buddd demo' should show unknown command.
    const auto res = run_buddd("demo triangle");

    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("Unknown command: 'demo'") != std::string::npos);
}

TEST_CASE("buddd capture cube is unknown command", "[cli][app]") {
    const auto res = run_buddd("capture cube");

    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("Unknown command: 'capture'") != std::string::npos);
}

TEST_CASE("buddd version prints version", "[cli][app]") {
    const auto res = run_buddd("version");

    REQUIRE(res.exit_code == 0);
    REQUIRE(res.stdout_str == "buddd 0.1.0\n");
}

TEST_CASE("buddd help shows updated usage", "[cli][app]") {
    const auto res = run_buddd("help");

    REQUIRE(res.exit_code == 0);
    // New help text should have 'run' with updated description
    REQUIRE(res.stdout_str.find("run") != std::string::npos);
    // 'demo' was removed, should NOT appear in help text
    REQUIRE(res.stdout_str.find("demo") == std::string::npos);
    REQUIRE(res.stdout_str.find("capture") == std::string::npos);
}

TEST_CASE("buddd run with no args runs empty window", "[cli][app]") {
    // This spawns a window; we can verify it opens and starts
    const auto binary = buddd_binary_path();
    const auto out_file = temp_filename("buddd_run_out");
    const auto err_file = temp_filename("buddd_run_err");

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
    std::remove(out_file.c_str());
    std::remove(err_file.c_str());

    REQUIRE(stdout_str.find("Window opened: 1024x768") != std::string::npos);
}
