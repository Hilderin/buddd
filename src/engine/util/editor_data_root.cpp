#include "util/editor_data_root.h"

#include <filesystem>

namespace buddd::engine {

auto editor_data_root(const std::filesystem::path& project_root) -> std::filesystem::path {
    return project_root / ".buddd";
}

auto editor_user_data_root(const std::filesystem::path& project_root) -> std::filesystem::path {
    return editor_data_root(project_root) / "user";
}

} // namespace buddd::engine
