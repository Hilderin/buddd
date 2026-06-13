#pragma once
#include <filesystem>

namespace buddd::engine {
/// Returns the OS-standard user config directory.
/// Linux: $XDG_CONFIG_HOME/buddd or ~/.config/buddd
/// macOS: ~/Library/Application Support/buddd
/// Windows: %APPDATA%/buddd
[[nodiscard]] auto os_user_config_dir() -> std::filesystem::path;
}
