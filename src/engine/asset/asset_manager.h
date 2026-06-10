#pragma once

#include "error.h"
#include "asset/asset.h"
#include "asset/dependency_map.h"
#include "asset/texture_asset.h"
#include "asset/material_asset.h"
#include "asset/model_asset.h"
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
    [[nodiscard]] auto create_model(std::string_view id) -> Result<std::shared_ptr<ModelAsset>>;

    auto clear() -> void;
    auto base_path() const noexcept -> std::string_view;
    auto poll_file_events() -> void;
    auto set_file_watcher_enabled(bool enabled) -> void;

    /// Reverse lookup: find the asset ID string for a given Model.
    /// Returns empty string if the model is not owned by any registered ModelAsset.
    [[nodiscard]] virtual auto find_asset_id(const Model& model) const -> std::string;

    /// Resolve an asset ID string to a shared_ptr<Model>.
    /// Returns error if the ID is not registered or the asset is not a ModelAsset.
    [[nodiscard]] virtual auto resolve_model(const std::string& id) -> Result<std::shared_ptr<Model>>;

    /// Returns a const reference to the internal dependency map.
    /// Useful for diagnostics, tooling, crash reporting, and editor features.
    [[nodiscard]] auto dependency_map() const noexcept -> const DependencyMap&;

    /// Returns a const reference to the shader program deduplication map.
    /// Useful for cache introspection, diagnostics, and tooling.
    [[nodiscard]] auto shader_programs() const noexcept
        -> const std::unordered_map<ShaderProgramKey, std::shared_ptr<ShaderProgram>>&;

    /// Trigger a hot-reload for the asset(s) that depend on the given file path.
    /// The path is relative to the asset base path and should match the format
    /// used by the FileWatcher (e.g. "textures/brick.png" or "materials/wall.yaml").
    ///
    /// The method determines from the file extension whether it is a YAML change
    /// or a source file change (same logic as poll_file_events()), then dispatches
    /// to the appropriate internal handler.
    ///
    /// This is the programmatic equivalent of a file system change notification.
    /// Safe to call even when the FileWatcher is disabled or a NullFileWatcher is in use.
    auto reload(std::string_view path) -> void;

    AssetManager(const AssetManager&) = delete;
    auto operator=(const AssetManager&) -> AssetManager& = delete;
    AssetManager(AssetManager&&) = delete;
    auto operator=(AssetManager&&) -> AssetManager& = delete;

protected:
    AssetManager(RenderDevice& device, std::string base_path);

private:

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

    // Internal: load a ModelAsset (non-template version).
    auto load_model(const std::string& id, const std::string& yaml_path) -> Result<std::shared_ptr<ModelAsset>>;

    // Hot-reload handlers.
    auto handle_yaml_change(const std::string& changed_path, const std::string& asset_id) -> void;
    auto handle_source_change(const std::string& changed_path, const std::string& asset_id) -> void;

    /// Dispatches a file change event to the appropriate internal handler
    /// based on the file extension. Routes `.yaml` files to `handle_yaml_change`
    /// and all other files to `handle_source_change`.
    ///
    /// Called internally by both:
    ///   - `poll_file_events()`   (with the real FileEventType from FileWatcher)
    ///   - `reload(path)`         (with FileEventType::Modified)
    auto dispatch_file_event(const std::string& path, FileEventType type) -> void;

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
