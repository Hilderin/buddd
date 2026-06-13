#include "util/os_config_dir.h"

#include <cstdlib>
#include <filesystem>

namespace buddd::engine {

auto os_user_config_dir() -> std::filesystem::path {
#if defined(_WIN32)
    if (auto* appdata = std::getenv("APPDATA"))
        return std::filesystem::path(appdata) / "buddd";
    return std::filesystem::path("C:/") / "Users" / "Default" / "AppData" / "Roaming" / "buddd";
#elif defined(__APPLE__)
    if (auto* home = std::getenv("HOME"))
        return std::filesystem::path(home) / "Library" / "Application Support" / "buddd";
    return std::filesystem::path("/tmp/buddd-config");
#else  // Linux
    if (auto* xdg = std::getenv("XDG_CONFIG_HOME"))
        return std::filesystem::path(xdg) / "buddd";
    if (auto* home = std::getenv("HOME"))
        return std::filesystem::path(home) / ".config" / "buddd";
    return std::filesystem::path("/tmp/buddd-config");
#endif
}

} // namespace buddd::engine
