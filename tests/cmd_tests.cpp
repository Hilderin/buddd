#include "test_helpers.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

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
    REQUIRE(res.stdout_str.find("demo") != std::string::npos);
    REQUIRE(res.stdout_str.find("version") != std::string::npos);
    REQUIRE(res.stdout_str.find("help") != std::string::npos);
}

TEST_CASE("buddd unknowncommand exits with code 1", "[cli]") {
    const auto res = run_buddd("unknowncommand");

    REQUIRE(res.exit_code == 1);
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
    REQUIRE(res.stderr_str.find("Usage: buddd demo <demo>") != std::string::npos);
    REQUIRE(res.stderr_str.find("triangle") != std::string::npos);
    REQUIRE(res.stderr_str.find("Demo names are case-sensitive.") != std::string::npos);
}

TEST_CASE("buddd demo unknownname prints error and exits 1", "[cli]") {
    const auto res = run_buddd("demo unknownname");

    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("Unknown demo: 'unknownname'") != std::string::npos);
    REQUIRE(res.stderr_str.find("Usage: buddd demo <demo>") != std::string::npos);
}

TEST_CASE("buddd test is unknown command", "[cli]") {
    const auto res = run_buddd("test");

    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("Unknown command: 'test'") != std::string::npos);
    REQUIRE(res.stderr_str.find("Usage: buddd <command> [<args>]") != std::string::npos);
}

TEST_CASE("buddd with no arguments defaults to run command", "[cli]") {
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
        return std::string((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
    };

    const auto stdout_str = read_file(out_file);
    std::remove(out_file.c_str());
    std::remove(err_file.c_str());

    REQUIRE(stdout_str.find("Window opened: 1024x768") != std::string::npos);
}
