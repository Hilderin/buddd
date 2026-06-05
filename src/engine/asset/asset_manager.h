#pragma once

#include "error.h"
#include "asset/asset.h"
#include "asset/dependency_map.h"
#include "asset/texture_asset.h"
#include "asset/material_asset.h"
#include "asset/file_watcher.h"
#include "render/shader_program.h"

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace buddd::engine {

class RenderDevice;

struct ShaderProgramKey {
    std::string vertex_path;
    std::string fragment_path;

    auto operator==(const ShaderProgramKey&) const -> bool = default;
};

} // namespace buddd::engine

template<>
struct std::hash<buddd::engine::ShaderProgramKey> {
    auto operator()(const buddd::engine::ShaderProgramKey& k) const noexcept -> size_t {
        return std::hash<std::string>{}(k.vertex_path)
             ^ (std::hash<std::string>{}(k.fragment_path) << 1);
    }
};

namespace buddd::engine {

class AssetManager {
public:
    [[nodiscard]] static auto create(RenderDevice& device, std::string_view base_path)
        -> Result<std::unique_ptr<AssetManager>>;

    ~AssetManager();

    template<typename T>
    [[nodiscard]] auto create(std::string_view id) -> Result<std::shared_ptr<T>>;

    [[nodiscard]] auto create_texture(std::string_view id) -> Result<std::shared_ptr<TextureAsset>>;
    [[nodiscard]] auto create_material(std::string_view id) -> Result<std::shared_ptr<MaterialAsset>>;

    auto clear() -> void;
    auto base_path() const noexcept -> std::string_view;
    auto poll_file_events() -> void;
    auto set_file_watcher_enabled(bool enabled) -> void;

    // Test-only accessors (under BUDDD_TESTING only, never in release builds).
#ifdef BUDDD_TESTING
    auto get_dependency_map() const -> const DependencyMap&;
    auto testing_shader_programs() const noexcept
        -> const std::unordered_map<ShaderProgramKey, std::shared_ptr<ShaderProgram>>&;
    void testing_inject_file_event(const FileEvent& event);
#endif

    AssetManager(const AssetManager&) = delete;
    auto operator=(const AssetManager&) -> AssetManager& = delete;
    AssetManager(AssetManager&&) = delete;
    auto operator=(AssetManager&&) -> AssetManager& = delete;

private:
    AssetManager(RenderDevice& device, std::string base_path);

    // Resolve a path from YAML. If absolute, return as-is.
    // If relative, strip the base_path_/ prefix so the returned path
    // is relative to base_path_ (matching FileWatcher event format).
    auto resolve_path(std::string_view path) -> std::string;

    // Convert a relative path (as returned by resolve_path) back to
    // a full filesystem path by prepending base_path_.
    auto make_full_path(const std::string& path) const -> std::string;

    // Read entire file into string. Returns IoFailed on error.
    static auto read_file(const std::string& path) -> Result<std::string>;

    // Internal: load a TextureAsset (non-template version).
    auto load_texture(const std::string& id, const std::string& yaml_path) -> Result<std::shared_ptr<TextureAsset>>;

    // Internal: load a MaterialAsset (non-template version).
    auto load_material(const std::string& id, const std::string& yaml_path) -> Result<std::shared_ptr<MaterialAsset>>;

    // Hot-reload handlers.
    auto handle_yaml_change(const std::string& changed_path, const std::string& asset_id) -> void;
    auto handle_source_change(const std::string& changed_path, const std::string& asset_id) -> void;

    RenderDevice& device_;
    std::string base_path_;

    // Cache: asset_id -> shared_ptr<Asset>
    std::unordered_map<std::string, std::shared_ptr<Asset>> cache_;

    // Shader program deduplication map
    std::unordered_map<ShaderProgramKey, std::shared_ptr<ShaderProgram>> shader_programs_;

    // Dependency tracking (bidirectional)
    DependencyMap dependency_map_;

    // File watcher (inotify on Linux, NullFileWatcher otherwise)
    std::unique_ptr<FileWatcher> file_watcher_;
    bool file_watcher_enabled_ = true;
};

} // namespace buddd::engine

#include "asset_manager.tpp"  // Template implementations
