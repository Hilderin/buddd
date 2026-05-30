#include "test_helpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cstdlib>
#include <fstream>
#include <string>

// ---------------------------------------------------------------------------
// Demo execution tests (tagged [cli][demo])
// Each test runs the full demo binary with a timeout and verifies completion.
// ---------------------------------------------------------------------------

TEST_CASE("buddd demo triangle runs and completes", "[cli][demo]") {
    const auto binary = buddd_binary_path();
    const auto out_file = temp_filename("buddd_demo_tri_out");
    const auto err_file = temp_filename("buddd_demo_tri_err");

    const std::string shell_cmd = "timeout 5 \"" + binary + "\" demo triangle > \""
                                  + out_file + "\" 2> \"" + err_file + "\" || true";

    const int sys_ret = std::system(shell_cmd.c_str());
    (void)sys_ret;

    auto read_file = [](const std::string& path) -> std::string {
        std::ifstream f(path, std::ios::binary);
        if (!f) return {};
        return std::string((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
    };

    const auto stderr_str = read_file(err_file);
    std::remove(out_file.c_str());
    std::remove(err_file.c_str());

    REQUIRE(stderr_str.find("Demo complete: triangle (120 frames rendered)") != std::string::npos);
}

TEST_CASE("buddd demo cube runs and completes", "[cli][demo]") {
    const auto binary = buddd_binary_path();
    const auto out_file = temp_filename("buddd_demo_cube_out");
    const auto err_file = temp_filename("buddd_demo_cube_err");

    const std::string shell_cmd = "timeout 5 \"" + binary + "\" demo cube > \""
                                  + out_file + "\" 2> \"" + err_file + "\" || true";

    const int sys_ret = std::system(shell_cmd.c_str());
    (void)sys_ret;

    auto read_file = [](const std::string& path) -> std::string {
        std::ifstream f(path, std::ios::binary);
        if (!f) return {};
        return std::string((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
    };

    const auto stderr_str = read_file(err_file);

    // Verify it ran (no crash), logged the demo start message, and completed
    CHECK_THAT(stderr_str, Catch::Matchers::ContainsSubstring("Demo started: cube ("));
    CHECK_THAT(stderr_str, Catch::Matchers::ContainsSubstring("Demo complete: cube ("));

    std::remove(out_file.c_str());
    std::remove(err_file.c_str());
}

TEST_CASE("buddd demo cube-scene runs and completes", "[cli][demo]") {
    const auto binary = buddd_binary_path();
    const auto out_file = temp_filename("buddd_demo_cube_scene_out");
    const auto err_file = temp_filename("buddd_demo_cube_scene_err");

    const std::string shell_cmd = "timeout 5 \"" + binary + "\" demo cube-scene > \""
                                  + out_file + "\" 2> \"" + err_file + "\" || true";

    const int sys_ret = std::system(shell_cmd.c_str());
    (void)sys_ret;

    auto read_file = [](const std::string& path) -> std::string {
        std::ifstream f(path, std::ios::binary);
        if (!f) return {};
        return std::string((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
    };

    const auto stderr_str = read_file(err_file);
    const auto stdout_str = read_file(out_file);

    // Verify it ran (no crash), logged the demo start message, and completed
    CHECK_THAT(stderr_str, Catch::Matchers::ContainsSubstring("Demo started: cube (scene-based"));
    CHECK_THAT(stderr_str, Catch::Matchers::ContainsSubstring("Demo complete: cube (scene-based"));

    std::remove(out_file.c_str());
    std::remove(err_file.c_str());
}
