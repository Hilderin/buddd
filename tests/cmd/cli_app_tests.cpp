#include "test_helpers.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <string>

// ---------------------------------------------------------------------------
// CLI App System integration tests (tagged [cli][app])
// These tests verify the new CLI dispatch: run, version, help, and flags.
// ---------------------------------------------------------------------------

TEST_CASE("buddd run triangle --frame 2 runs and exits", "[cli][app]") {
    // --log-level=info ensures INFO-level messages appear in release builds
    const auto res = run_buddd("run triangle --frame 2 --log-level=info");

    REQUIRE(res.exit_code == 0);
    // Should complete without errors
    REQUIRE(res.stderr_str.find("Scene complete:") != std::string::npos);
    REQUIRE(res.stderr_str.find("Buddd Engine \u2014 triangle") != std::string::npos);
}

TEST_CASE("buddd run triangle extra_arg warns and proceeds", "[cli][app]") {
    // --log-level=info ensures INFO-level messages appear in release builds
    const auto res = run_buddd("run triangle --frame 2 extra_arg --log-level=info");

    // Limit with --frame so the process exits quickly
    REQUIRE(res.exit_code == 0);
    REQUIRE(res.stderr_str.find("Scene complete:") != std::string::npos);
    REQUIRE(res.stderr_str.find("Warning: unexpected arguments") != std::string::npos);
}

TEST_CASE("buddd run unknownscene prints error and exits 1", "[cli][app]") {
    const auto res = run_buddd("run unknownscene");

    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("[ERROR] [App] Unknown scene: 'unknownscene'") != std::string::npos);
}

TEST_CASE("buddd demo is now unknown command", "[cli][app]") {
    // The 'demo' command was removed; 'buddd demo' should show unknown command.
    const auto res = run_buddd("demo triangle");

    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("[ERROR] [App] Unknown command: 'demo'") != std::string::npos);
}

TEST_CASE("buddd capture cube is unknown command", "[cli][app]") {
    const auto res = run_buddd("capture cube");

    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("[ERROR] [App] Unknown command: 'capture'") != std::string::npos);
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
    // 'capture' now appears in edit command description
    REQUIRE(res.stdout_str.find("capture") != std::string::npos);
}

TEST_CASE("buddd run with no args runs empty window", "[cli][app]") {
    // This spawns a window; we can verify it opens and starts.
    // Use --log-level=info so INFO-level messages appear in release builds.
    const auto binary = buddd_binary_path();
    const auto out_file = temp_filename("buddd_run_out");
    const auto err_file = temp_filename("buddd_run_err");

    static_cast<void>(setenv("SDL_VIDEO_DRIVER", "offscreen", 1));

    const std::string shell_cmd = "timeout 2 \"" + binary + "\" run --log-level=info > \""
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

// ── buddd edit [<scene>] tests ──

TEST_CASE("buddd edit --frame 2 opens editor and exits", "[cli][app]") {
    const auto res = run_buddd("edit --frame 2 --log-level=info");

#ifdef BUDDD_HAS_DISPLAY
    // In display build: editor runs and outputs layout file log
    REQUIRE(res.exit_code == 0);
    REQUIRE(res.stderr_str.find("layout.ini") != std::string::npos);
#else
    // In headless build: editor fails with "editor requires a display"
    // Either way, the CLI dispatch itself should not crash or produce unknown-command errors.
    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("editor requires a display") != std::string::npos);
#endif
}

TEST_CASE("buddd edit nonexistent.yaml prints error and exits 1", "[cli][app]") {
    const auto res = run_buddd("edit nonexistent.yaml");

    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("[ERROR] [App] Scene file not found: 'nonexistent.yaml'") != std::string::npos);
}

TEST_CASE("buddd edit somearg unknown argument exits 1", "[cli][app]") {
    const auto res = run_buddd("edit somearg");

    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("[ERROR] [App] Unknown argument for edit: 'somearg'") != std::string::npos);
}

TEST_CASE("buddd edit --capture flag not mistaken for unknown arg", "[cli][app]") {
    // --capture 2:path starts with '-', should be treated as flags, not unknown arg
    // In display build: exit 0 (editor runs)
    // In headless build: exit 1 (editor requires display)
    // Key assertion: NOT "Unknown argument for edit"
    const auto res = run_buddd("edit --capture 2:/tmp/test_cap_edit.png --frame 2");
    REQUIRE(res.stderr_str.find("Unknown argument for edit") == std::string::npos);
}

TEST_CASE("buddd edit .yaml extension-only exits 1", "[cli][app]") {
    const auto res = run_buddd("edit .yaml");

    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("[ERROR] [App] Scene file not found: '.yaml'") != std::string::npos);
}

TEST_CASE("buddd edit .yml extension-only exits 1", "[cli][app]") {
    const auto res = run_buddd("edit .yml");

    REQUIRE(res.exit_code == 1);
    REQUIRE(res.stderr_str.find("[ERROR] [App] Scene file not found: '.yml'") != std::string::npos);
}

TEST_CASE("buddd edit with valid yaml file --frame 2 accepts scene path", "[cli][app]") {
    // Create a temp valid YAML scene file
    const auto scene_file = temp_filename("buddd_test_scene");
    {
        std::ofstream f(scene_file);
        f << "# empty test scene\nentities: []\n";
    }

    const auto res = run_buddd("edit \"" + scene_file + "\" --frame 2 --log-level=info");

    std::remove(scene_file.c_str());

    // Key assertion: no "Scene file not found" error (file was accepted)
    REQUIRE(res.stderr_str.find("Scene file not found") == std::string::npos);
    // Editor may or may not run (depends on display build), but dispatch was correct
}

TEST_CASE("buddd edit with .YML file is case-insensitive", "[cli][app]") {
    // Create temp file with .YML extension
    const auto scene_file = temp_filename("buddd_test_scene");
    const std::string yml_file = scene_file + ".YML";
    {
        std::ofstream f(yml_file);
        f << "# empty test scene\nentities: []\n";
    }

    const auto res = run_buddd("edit \"" + yml_file + "\" --frame 2 --log-level=info");

    std::remove(yml_file.c_str());

    // Key assertion: no "Scene file not found" (case-insensitive matching works)
    REQUIRE(res.stderr_str.find("Scene file not found") == std::string::npos);
}
