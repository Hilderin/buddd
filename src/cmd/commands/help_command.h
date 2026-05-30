#pragma once

#include <string_view>

namespace buddd::cmd {

/// Shared usage text constant, used by both HelpCommand and main.cpp's unknown-
/// command handler.
inline constexpr std::string_view k_usage_text =
    "Usage: buddd <command> [<args>]\n"
    "\n"
    "Commands:\n"
    "  run       Run the engine in interactive mode (empty window)\n"
    "  demo      Run a demo by name (try 'buddd demo triangle')\n"
    "  version   Print version information\n"
    "  help      Show this help message\n";

class HelpCommand {
public:
    /// Prints the usage message to stdout and exits with code 0.
    /// Extra arguments are silently ignored.
    [[nodiscard]] auto run(int argc, const char* const* argv) -> int;
};

} // namespace buddd::cmd
