#pragma once

#include <catch2/catch_test_macros.hpp>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers for CLI integration tests
// ---------------------------------------------------------------------------

/// Determine the path to the buddd binary relative to the running test.
/// Uses /proc/self/exe on Linux to find the test binary location, then derives
/// the sibling binary path under src/cmd/.
inline auto buddd_binary_path() -> std::string {
    char buf[4096];
    const ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len == -1) {
        return "./src/cmd/buddd";
    }
    buf[len] = '\0';
    std::string path(buf);

    auto slash = path.rfind('/');
    if (slash != std::string::npos) {
        path = path.substr(0, slash);
    }
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
inline auto temp_filename(const char* prefix) -> std::string {
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

/// Run buddd with the given arguments, capturing stdout/stderr/exit code.
/// Uses the offscreen SDL driver to prevent windows from popping up.
inline auto run_buddd(const std::string& args) -> CommandResult {
    const auto binary = buddd_binary_path();

    const auto out_file = temp_filename("buddd_out");
    const auto err_file = temp_filename("buddd_err");

    const std::string shell_cmd = "SDL_VIDEO_DRIVER=offscreen \""
                                  + binary + "\" " + args
                                  + " > \"" + out_file + "\" 2> \"" + err_file + "\"";

    const int ret = std::system(shell_cmd.c_str());
    const int exit_code = (ret != -1 && WIFEXITED(ret)) ? WEXITSTATUS(ret) : -1;

    auto read_file = [](const std::string& path) -> std::string {
        std::ifstream f(path, std::ios::binary);
        if (!f) return {};
        return std::string((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
    };

    CommandResult result;
    result.stdout_str = read_file(out_file);
    result.stderr_str = read_file(err_file);
    result.exit_code = exit_code;

    std::remove(out_file.c_str());
    std::remove(err_file.c_str());

    return result;
}
