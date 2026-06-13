#include "settings/settings_manager.h"
#include "util/os_config_dir.h"
#include "util/editor_data_root.h"
#include "log/log.h"

#include <filesystem>

BUDDD_LOG_TAG("Settings");

namespace buddd::engine {

SettingsManager::SettingsManager(std::filesystem::path project_root, SerializationContext ctx)
    : project_root_(std::move(project_root))
    , ctx_(ctx)
    , editor_settings_(std::make_unique<SettingsStore>(
          os_user_config_dir() / "editor.yaml", ctx_))
    , project_settings_(std::make_unique<SettingsStore>(
          project_root_ / "buddd.project.yaml", ctx_))
    , user_project_settings_(std::make_unique<SettingsStore>(
          editor_user_data_root(project_root_) / "settings.yaml", ctx_))
    , ini_path_((editor_user_data_root(project_root_) / "layout.ini").string())
{
}

auto SettingsManager::load_all() -> Result<void> {
    // Create .buddd/user/ directory
    auto user_data_root = editor_user_data_root(project_root_);
    try {
        std::filesystem::create_directories(user_data_root);
        BUDDD_LOG_DEBUG("Settings: created directory {}", user_data_root.string());
    } catch (const std::exception& e) {
        BUDDD_LOG_WARN("Settings: failed to create directory {}: {}", user_data_root.string(), e.what());
        return make_error(Error::Category::InitFailed,
            "Settings: cannot create " + user_data_root.string() + ": " + e.what());
    }

    // Load all stores (non-fatal if individual store fails)
    auto try_load = [](SettingsStore& store, const std::string& name) {
        auto result = store.load();
        if (!result) {
            BUDDD_LOG_WARN("Settings: failed to load {} settings: {} (using defaults)",
                name, result.error().message);
        }
    };

    try_load(*editor_settings_, "editor");
    try_load(*project_settings_, "project");
    try_load(*user_project_settings_, "user project");

    return {};
}

auto SettingsManager::save_all() -> Result<void> {
    Error first_err;
    bool has_error = false;

    auto try_save = [&first_err, &has_error](SettingsStore& store, const std::string& name) {
        if (!store.is_dirty()) return;
        auto result = store.save();
        if (!result) {
            BUDDD_LOG_WARN("Settings: failed to save {} settings: {}", name, result.error().message);
            if (!has_error) {
                first_err = result.error();
                has_error = true;
            }
        }
    };

    try_save(*editor_settings_, "editor");
    try_save(*project_settings_, "project");
    try_save(*user_project_settings_, "user project");

    if (has_error) {
        return make_error(first_err);
    }
    return {};
}

auto SettingsManager::editor_settings() -> SettingsStore& {
    return *editor_settings_;
}

auto SettingsManager::project_settings() -> SettingsStore& {
    return *project_settings_;
}

auto SettingsManager::user_project_settings() -> SettingsStore& {
    return *user_project_settings_;
}

auto SettingsManager::layout_ini_path() const noexcept -> const std::string& {
    return ini_path_;
}

} // namespace buddd::engine
