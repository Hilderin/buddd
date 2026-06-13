#pragma once
#include <filesystem>

namespace buddd::engine {
/// Returns <project_root>/.buddd/
[[nodiscard]] auto editor_data_root(const std::filesystem::path& project_root) -> std::filesystem::path;

/// Returns <project_root>/.buddd/user/
[[nodiscard]] auto editor_user_data_root(const std::filesystem::path& project_root) -> std::filesystem::path;
}
