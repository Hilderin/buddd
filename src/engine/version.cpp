#include "version.h"

#include <string_view>

namespace buddd::engine {

auto version() -> std::string_view {
    return "0.1.0";
}

} // namespace buddd::engine
