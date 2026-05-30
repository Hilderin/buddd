#include "help_command.h"

#include <cstdio>
#include <cstdlib>

namespace bc = buddd::cmd;

auto bc::HelpCommand::run([[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv) -> int {
    std::fwrite(k_usage_text.data(), 1, k_usage_text.size(), stdout);
    return EXIT_SUCCESS;
}
