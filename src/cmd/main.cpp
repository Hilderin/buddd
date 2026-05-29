#include "version.h"

#include <cstdio>
#include <string_view>

auto main(int argc, char* argv[]) -> int {
    if (argc == 2 && std::string_view{argv[1]} == "--version") {
        std::printf("buddd %s\n", buddd::engine::version().data());
    } else {
        std::printf("Buddd Engine v%s\n", buddd::engine::version().data());
    }
    return 0;
}
