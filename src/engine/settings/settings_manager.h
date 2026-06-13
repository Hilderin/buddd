#pragma once

#include "error.h"
#include "settings/settings_store.h"
#include "scene/component_registry/serialization_context.h"

#include <filesystem>
#include <memory>
#include <string>

namespace buddd::engine {

class SettingsManager {
public:
    explicit SettingsManager(std::filesystem::path project_root, SerializationContext ctx);
    ~SettingsManager() = default;

    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;
    SettingsManager(SettingsManager&&) = delete;  // persistent string for IniFilename ptr

    [[nodiscard]] auto load_all() -> Result<void>;
    [[nodiscard]] auto save_all() -> Result<void>;

    [[nodiscard]] auto editor_settings() -> SettingsStore&;
    [[nodiscard]] auto project_settings() -> SettingsStore&;
    [[nodiscard]] auto user_project_settings() -> SettingsStore&;

    /// Path to ImGui layout INI file (.buddd/user/layout.ini).
    /// Returns const ref to persistent string — safe for ImGui::GetIO().IniFilename = .c_str()
    [[nodiscard]] auto layout_ini_path() const noexcept -> const std::string&;

private:
    std::filesystem::path project_root_;
    SerializationContext ctx_;
    std::unique_ptr<SettingsStore> editor_settings_;
    std::unique_ptr<SettingsStore> project_settings_;
    std::unique_ptr<SettingsStore> user_project_settings_;
    std::string ini_path_;  // persistent backing for layout_ini_path()
};

} // namespace buddd::engine
