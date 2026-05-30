#include "version_command.h"
#include "version.h"

#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace be = buddd::engine;
namespace bc = buddd::cmd;

auto bc::VersionCommand::run([[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv) -> int {
    const auto ver = be::version();
    std::fwrite("buddd ", 1, 6, stdout);
    std::fwrite(ver.data(), 1, ver.size(), stdout);
    std::fwrite("\n", 1, 1, stdout);
    return EXIT_SUCCESS;
}
