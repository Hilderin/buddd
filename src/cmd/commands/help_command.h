#pragma once

#include <string_view>

namespace buddd::cmd {

/// Shared usage text constant, used by both HelpCommand and main.cpp's unknown-
/// command handler.
inline constexpr std::string_view k_usage_text =
    "Usage: buddd <command> [<args>]\n"
    "\n"
    "Commands:\n"
    "  run       Run a scene or the interactive window (default)\n"
    "  edit      Open the editor (optionally with a scene file)\n"
    "  version   Print version information\n"
    "  help      Show this help message\n"
    "\n"
    "Flags for run/edit: --frame N, --capture N:path\n"
    "For scene usage: buddd run --help\n";

class HelpCommand {
public:
    /// Prints the usage message to stdout and exits with code 0.
    /// Extra arguments are silently ignored.
    [[nodiscard]] auto run(int argc, const char* const* argv) -> int;
};

} // namespace buddd::cmd
