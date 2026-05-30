#include "commands/demo_command.h"
#include "commands/help_command.h"
#include "commands/run_command.h"
#include "commands/version_command.h"

#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace bc = buddd::cmd;

auto main(int argc, char* argv[]) -> int {
    // No positional argument -> default to run
    if (argc < 2 || argv[1] == nullptr) {
        return bc::RunCommand{}.run(argc, argv);
    }

    const std::string_view cmd{argv[1]};

    if (cmd == "run") {
        return bc::RunCommand{}.run(argc, argv);
    }

    if (cmd == "demo") {
        return bc::DemoCommand{}.run(argc, argv);
    }

    if (cmd == "version") {
        return bc::VersionCommand{}.run(argc, argv);
    }

    if (cmd == "help") {
        return bc::HelpCommand{}.run(argc, argv);
    }

    // Unknown command (includes "test", "--test", "--version", etc.)
    std::fprintf(stderr, "Unknown command: '%s'\n\n",
                 argv[1]);
    std::fwrite(bc::k_usage_text.data(), 1, bc::k_usage_text.size(), stderr);
    return EXIT_FAILURE;
}
