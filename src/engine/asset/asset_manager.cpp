#include "asset/asset_manager.h"
#include "asset/model_loader.h"
#include "render/render_device.h"
#include "render/pbr/pbr_material.h"
#include "image/image.h"
#include "log/log.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <functional>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

BUDDD_LOG_TAG("Asset");

namespace buddd::engine {

// ============================================================================
// Construction / Destruction
// ============================================================================

AssetManager::AssetManager(RenderDevice& device, std::string base_path)
    : device_(device)
    , base_path_(std::move(base_path))
{
}

AssetManager::~AssetManager() = default;

auto AssetManager::create(RenderDevice& device, std::string_view base_path)
    -> Result<std::unique_ptr<AssetManager>>
{
    if (base_path.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "AssetManager base path must not be empty");
    }

    auto mgr = std::unique_ptr<AssetManager>(
        new AssetManager(device, std::string(base_path)));

    // Create and start FileWatcher
#ifdef __linux__
    if (auto watcher = FileWatcher::create(mgr->base_path_)) {
        mgr->file_watcher_ = std::move(*watcher);
        mgr->file_watcher_->start();
    } else {
        BUDDD_LOG_WARN("FileWatcher failed to create: {} \u2014 falling back to NullFileWatcher", watcher.error().message);
        mgr->file_watcher_ = std::make_unique<NullFileWatcher>();
    }
#else
    mgr->file_watcher_ = std::make_unique<NullFileWatcher>();
#endif

    return mgr;
}

// ============================================================================
// Public API
// ============================================================================

auto AssetManager::create_texture(std::string_view id) -> Result<std::shared_ptr<TextureAsset>> {
    return create<TextureAsset>(id);
}

auto AssetManager::create_material(std::string_view id) -> Result<std::shared_ptr<MaterialAsset>> {
    return create<MaterialAsset>(id);
}

auto AssetManager::create_model(std::string_view id) -> Result<std::shared_ptr<ModelAsset>> {
    return create<ModelAsset>(id);
}

auto AssetManager::clear() -> void {
    size_t count = cache_.size();
    cache_.clear();
    shader_programs_.clear();
    dependency_map_.clear();
    BUDDD_LOG_DEBUG("Cache cleared ({} assets)", count);
}

auto AssetManager::base_path() const noexcept -> std::string_view {
    return base_path_;
}

auto AssetManager::poll_file_events() -> void {
    if (!file_watcher_ || !file_watcher_enabled_) return;

    auto events = file_watcher_->poll_events();
    for (const auto& event : events) {
        auto dependents = dependency_map_.get_dependents(event.path);
        if (dependents.empty()) continue;

        // Collect asset IDs into a vector (avoid iterator invalidation)
        std::vector<std::string> asset_ids(dependents.begin(), dependents.end());

        for (const auto& asset_id : asset_ids) {
            if (event.path.size() >= 5 &&
                event.path.substr(event.path.size() - 5) == ".yaml") {
                handle_yaml_change(event.path, asset_id);
            } else {
                handle_source_change(event.path, asset_id);
            }
        }
    }
}

auto AssetManager::set_file_watcher_enabled(bool enabled) -> void {
    file_watcher_enabled_ = enabled;
}

// ============================================================================
// Test-only accessors
// ============================================================================

#ifdef BUDDD_TESTING
auto AssetManager::get_dependency_map() const -> const DependencyMap& {
    return dependency_map_;
}

auto AssetManager::testing_shader_programs() const noexcept
    -> const std::unordered_map<ShaderProgramKey, std::shared_ptr<ShaderProgram>>&
{
    return shader_programs_;
}

void AssetManager::testing_inject_file_event(const FileEvent& event) {
    auto dependents = dependency_map_.get_dependents(event.path);
    if (dependents.empty()) return;

    std::vector<std::string> asset_ids(dependents.begin(), dependents.end());

    for (const auto& asset_id : asset_ids) {
        if (event.path.size() >= 5 &&
            event.path.substr(event.path.size() - 5) == ".yaml") {
            handle_yaml_change(event.path, asset_id);
        } else {
            handle_source_change(event.path, asset_id);
        }
    }
}
#endif

// ============================================================================
// Path resolution and file I/O
// ============================================================================

auto AssetManager::resolve_path(std::string_view path) -> std::string {
    if (path.empty()) return std::string(path);
    if (path.front() == '/') return std::string(path);
    // Return the path relative to base_path_ so it matches FileWatcher event format.
    // Use std::filesystem::relative to handle both absolute and relative base_path_.
    auto path_str = std::string(path);
    try {
        auto base_abs = std::filesystem::absolute(std::filesystem::path(base_path_));
        auto src_abs = std::filesystem::absolute(std::filesystem::path(path_str));
        auto rel = src_abs.lexically_relative(base_abs);
        if (!rel.empty() && rel.generic_string().front() != '.') {
            return rel.generic_string();
        }
    } catch (...) {
        // Fall through on error
    }
    return path_str;
}

auto AssetManager::make_full_path(const std::string& path) const -> std::string {
    if (path.empty() || path.front() == '/') return path;
    return base_path_ + "/" + path;
}

auto AssetManager::read_file(const std::string& path) -> Result<std::string> {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        return make_error(Error::Category::IoFailed,
            "Failed to open: " + path + " (" + std::strerror(errno) + ")");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    if (file.bad()) {
        return make_error(Error::Category::IoFailed,
            "Failed to read: " + path);
    }

    return buffer.str();
}

// ============================================================================
// YAML parsing helper
// ============================================================================

namespace {

auto parse_yaml_file(const std::string& yaml_path) -> Result<YAML::Node> {
    try {
        return YAML::LoadFile(yaml_path);
    } catch (const YAML::Exception& e) {
        return make_error(Error::Category::IoFailed,
            "YAML parse error in " + yaml_path + ": " + e.what());
    } catch (const std::exception& e) {
        return make_error(Error::Category::IoFailed,
            "Unexpected error parsing " + yaml_path + ": " + e.what());
    }
}

auto get_yaml_type(const YAML::Node& yaml) -> std::string {
    try {
        return yaml["type"].as<std::string>("");
    } catch (...) {
        return "";
    }
}

auto get_yaml_version(const YAML::Node& yaml) -> int {
    try {
        return yaml["version"].as<int>(1);
    } catch (...) {
        return 1;
    }
}

} // anonymous namespace

// ============================================================================
// load_texture
// ============================================================================

auto AssetManager::load_texture(const std::string& id, const std::string& yaml_path)
    -> Result<std::shared_ptr<TextureAsset>>
{
    // 1. Parse YAML
    auto yaml_result = parse_yaml_file(yaml_path);
    if (!yaml_result) {
        BUDDD_LOG_DEBUG("YAML error: {} - {}", yaml_path, yaml_result.error().message);
        return std::unexpected(yaml_result.error());
    }
    auto yaml = std::move(*yaml_result);

    // 2. Validate type
    auto type = get_yaml_type(yaml);
    if (type != "Texture") {
        auto err = make_error(Error::Category::InvalidArgument,
            "Expected type 'Texture', got '" + type + "'");
        BUDDD_LOG_DEBUG("Type mismatch: {} (expected Texture, got {})", id, type);
        return std::unexpected(err);
    }

    // 3. Validate version
    auto version = get_yaml_version(yaml);
    if (version != 1) {
        return make_error(Error::Category::Unsupported,
            "Unsupported Texture version: " + std::to_string(version));
    }

    // 4. Parse source image path
    std::string source;
    try {
        source = yaml["source"].as<std::string>("");
    } catch (...) {}
    if (source.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Texture 'source' field is required");
    }
    auto source_path = resolve_path(source);

    // 5. Load image and create texture
    auto image = Image::load(make_full_path(source_path));
    if (!image) {
        return std::unexpected(image.error());
    }

    auto texture = device_.create_texture(*image);
    if (!texture) {
        return std::unexpected(texture.error());
    }

    std::shared_ptr<Texture> shared_tex(std::move(*texture));

    // 6. Texture settings from YAML — parsed but NOT applied (V1)
    //    Fields: wrap_s, wrap_t, min_filter, mag_filter, generate_mipmaps
    try {
        auto settings = yaml["settings"];
        if (settings) {
            auto parse_filter = [](const std::string&) {
                // Validated but not applied
            };
            parse_filter(settings["wrap_s"].as<std::string>(""));
            parse_filter(settings["wrap_t"].as<std::string>(""));
            parse_filter(settings["min_filter"].as<std::string>(""));
            parse_filter(settings["mag_filter"].as<std::string>(""));
            // generate_mipmaps: parsed but not applied
            settings["generate_mipmaps"].as<bool>(false);
        }
    } catch (...) {
        // Parsing settings is best-effort; ignore parse errors
    }

    // 7. Create asset wrapper
    auto asset = std::make_shared<TextureAsset>(std::move(shared_tex));

    // 8. Cache and track dependencies
    //    Store paths relative to base_path_ so they match FileWatcher events.
    cache_[id] = asset;
    dependency_map_.add_dependency(id, std::string(id) + ".yaml");
    dependency_map_.add_dependency(id, source_path);

    BUDDD_LOG_DEBUG("Texture created: {} ({}x{}, {}ch)", id, image->width(), image->height(), image->channels());

    return asset;
}

// ============================================================================
// load_material
// ============================================================================

auto AssetManager::load_material(const std::string& id, const std::string& yaml_path)
    -> Result<std::shared_ptr<MaterialAsset>>
{
    // 1. Parse YAML
    auto yaml_result = parse_yaml_file(yaml_path);
    if (!yaml_result) {
        return std::unexpected(yaml_result.error());
    }
    auto yaml = std::move(*yaml_result);

    // 2. Validate type
    auto type = get_yaml_type(yaml);
    if (type != "Material") {
        auto err = make_error(Error::Category::InvalidArgument,
            "Expected type 'Material', got '" + type + "'");
        BUDDD_LOG_DEBUG("Type mismatch: {} (expected Material, got {})", id, type);
        return std::unexpected(err);
    }

    // 3. Validate version
    auto version = get_yaml_version(yaml);
    if (version != 1) {
        return make_error(Error::Category::Unsupported,
            "Unsupported Material version: " + std::to_string(version));
    }

    // 4. Read shader paths
    std::string vert_path_str, frag_path_str;
    try {
        auto shaders_node = yaml["shaders"];
        if (!shaders_node) {
            return make_error(Error::Category::InvalidArgument,
                "Material 'shaders' field is required");
        }
        vert_path_str = shaders_node["vertex"].as<std::string>("");
        frag_path_str = shaders_node["fragment"].as<std::string>("");
    } catch (const YAML::Exception&) {
        return make_error(Error::Category::InvalidArgument,
            "Material 'shaders' field is required");
    }

    if (vert_path_str.empty() || frag_path_str.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Material shaders 'vertex' and 'fragment' are required");
    }

    auto vert_path = resolve_path(vert_path_str);
    auto frag_path = resolve_path(frag_path_str);

    // 5. Load shader source files
    auto vert_source = read_file(make_full_path(vert_path));
    if (!vert_source) return std::unexpected(vert_source.error());

    auto frag_source = read_file(make_full_path(frag_path));
    if (!frag_source) return std::unexpected(frag_source.error());

    // 6. Deduplicate shader program by (vert_path, frag_path)
    ShaderProgramKey program_key{vert_path, frag_path};
    std::shared_ptr<ShaderProgram> shader_program;

    auto program_iter = shader_programs_.find(program_key);
    if (program_iter != shader_programs_.end()) {
        shader_program = program_iter->second;
        BUDDD_LOG_DEBUG("Shader program cache hit: ({}, {})", vert_path, frag_path);
    } else {
        // Compile new shader program
        auto vs = device_.create_shader(ShaderType::Vertex, *vert_source);
        if (!vs) return std::unexpected(vs.error());

        auto fs = device_.create_shader(ShaderType::Fragment, *frag_source);
        if (!fs) return std::unexpected(fs.error());

        auto program = device_.create_shader_program(std::move(*vs), std::move(*fs));
        if (!program) return std::unexpected(program.error());

        shader_program = std::move(*program);
        shader_programs_[program_key] = shader_program;

        BUDDD_LOG_DEBUG("Shader program compiled: ({}, {})", vert_path, frag_path);
    }

    // 7. Create a fresh Material for THIS asset
    auto material = device_.create_material(shader_program);
    if (!material) return std::unexpected(material.error());
    auto shared_material = std::shared_ptr<Material>(std::move(*material));

    // 8. Resolve texture references
    try {
        auto textures_node = yaml["textures"];
        if (textures_node) {
            for (auto it = textures_node.begin(); it != textures_node.end(); ++it) {
                auto tex_name = it->first.as<std::string>();
                auto tex_id = it->second.as<std::string>();

                auto tex_asset = create<TextureAsset>(tex_id);
                if (!tex_asset) {
                    return std::unexpected(tex_asset.error());
                }

                auto set_tex_result = shared_material->set_texture(tex_name, (*tex_asset)->texture());
                if (!set_tex_result) {
                    BUDDD_LOG_DEBUG("Warning: could not set texture '{}' on material {}", tex_name, id);
                }
            }
        }
    } catch (const YAML::Exception&) {
        // If texture parsing fails, skip textures (material still valid)
    }

    // 9. Apply constant overrides
    try {
        auto constants_node = yaml["constants"];
        if (constants_node) {
            for (auto it = constants_node.begin(); it != constants_node.end(); ++it) {
                auto name = it->first.as<std::string>();
                auto value = it->second;
                if (value.IsScalar()) {
                    try {
                        float float_val = value.as<float>();
                        auto set_result = shared_material->set_uniform(name, float_val);
                        if (!set_result) {
                            BUDDD_LOG_DEBUG("Constant '{}' not found in material {}", name, id);
                        }
                    } catch (const YAML::TypedBadConversion<float>&) {
                        BUDDD_LOG_DEBUG("Warning: constant '{}' is not a valid float, skipping", name);
                    }
                }
            }
        }
    } catch (const YAML::Exception&) {
        // If constants parsing fails, skip constants
    }

    // 10. Create asset wrapper
    auto asset = std::make_shared<MaterialAsset>(std::move(shared_material));

    // 11. Cache and track dependencies
    //     Store paths relative to base_path_ so they match FileWatcher events.
    cache_[id] = asset;
    dependency_map_.add_dependency(id, std::string(id) + ".yaml");
    dependency_map_.add_dependency(id, vert_path);
    dependency_map_.add_dependency(id, frag_path);

    BUDDD_LOG_DEBUG("Material created: {} ({}, {})", id, vert_path, frag_path);

    return asset;
}

// ============================================================================
// load_model
// ============================================================================

auto AssetManager::load_model(const std::string& id, const std::string& yaml_path)
    -> Result<std::shared_ptr<ModelAsset>>
{
    // 1. Parse YAML
    auto yaml_result = parse_yaml_file(yaml_path);
    if (!yaml_result) {
        return std::unexpected(yaml_result.error());
    }
    auto yaml = std::move(*yaml_result);

    // 2. Validate type
    auto type = get_yaml_type(yaml);
    if (type != "Model") {
        auto err = make_error(Error::Category::InvalidArgument,
            "Expected type 'Model', got '" + type + "'");
        BUDDD_LOG_DEBUG("Type mismatch: {} (expected Model, got {})", id, type);
        return std::unexpected(err);
    }

    // 3. Validate version
    auto version = get_yaml_version(yaml);
    if (version != 1) {
        return make_error(Error::Category::Unsupported,
            "Unsupported Model version: " + std::to_string(version));
    }

    // 4. Read source field
    std::string source;
    try {
        source = yaml["source"].as<std::string>("");
    } catch (...) {}
    if (source.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Model 'source' field is required");
    }
    auto source_path = resolve_path(source);

    // 5. Read settings.scale
    float scale = 1.0f;
    try {
        scale = yaml["settings"]["scale"].as<float>(1.0f);
    } catch (...) {}

    if (scale == 0.0f) {
        BUDDD_LOG_WARN("Model '{}' scale is 0.0", id);
    }

    // 6. Load glTF model
    auto load_result = detail::load_gltf_model(device_, make_full_path(source_path), scale);
    if (!load_result) {
        BUDDD_LOG_DEBUG("Model load failed: {} \u2014 {}", id, load_result.error().message);
        return std::unexpected(load_result.error());
    }

    // Count vertices and nodes for logging (BEFORE moving root)
    size_t vertex_count = 0;
    size_t root_children_count = load_result->root.children.size();
    // Simple recursive count
    std::function<void(const ModelNode&)> count_verts = [&](const ModelNode& n) {
        if (n.model.has_value()) {
            vertex_count += n.model->vertex_count();
        }
        for (const auto& c : n.children) {
            count_verts(c);
        }
    };
    count_verts(load_result->root);

    // 7. Create ModelAsset
    auto asset = std::make_shared<ModelAsset>(std::move(load_result->root));

    // 8. Cache and track dependencies
    cache_[id] = asset;
    dependency_map_.add_dependency(id, std::string(id) + ".yaml");
    dependency_map_.add_dependency(id, source_path);

    BUDDD_LOG_DEBUG("Model loaded: {} ({} verts, {} root nodes)", id, vertex_count, root_children_count);

    return asset;
}

// ============================================================================
// Hot-reload handlers
// ============================================================================

auto AssetManager::handle_yaml_change(const std::string& changed_path, const std::string& asset_id) -> void {
    // Find the cached asset
    auto cache_it = cache_.find(asset_id);
    if (cache_it == cache_.end()) return;

    // changed_path is relative to base_path_ — reconstruct full path for I/O
    auto full_changed_path = make_full_path(changed_path);

    // Determine asset type and reload
    if (auto tex_asset = std::dynamic_pointer_cast<TextureAsset>(cache_it->second)) {
        // Texture YAML change: reload image and swap GPU handles
        auto yaml_result = parse_yaml_file(full_changed_path);
        if (!yaml_result) {
            BUDDD_LOG_ERROR("Hot-reload YAML parse error: {}", full_changed_path);
            return;
        }
        auto yaml = std::move(*yaml_result);

        // Read source path
        std::string source;
        try { source = yaml["source"].as<std::string>(""); } catch (...) {}
        if (source.empty()) {
            BUDDD_LOG_ERROR("Hot-reload: missing source in {}", full_changed_path);
            return;
        }
        auto source_path = resolve_path(source);

        // Load new image (source_path is relative to base_path_, prepend for I/O)
        auto image = Image::load(make_full_path(source_path));
        if (!image) {
            BUDDD_LOG_ERROR("Hot-reload: image load failed: {}", source_path);
            return;
        }

        // Create new GPU texture
        auto new_tex = device_.create_texture(*image);
        if (!new_tex) {
            BUDDD_LOG_ERROR("Hot-reload: texture creation failed");
            return;
        }

        // Swap handles: extract handle from new texture, inject into existing
        auto new_handle = (*new_tex)->release_gl_handle();
        tex_asset->texture()->replace_gl_handle(new_handle);

        // Update dependency map (source path may have changed)
        // Use relative paths matching FileWatcher format
        dependency_map_.remove_asset(asset_id);
        dependency_map_.add_dependency(asset_id, std::string(asset_id) + ".yaml");
        dependency_map_.add_dependency(asset_id, source_path);

        BUDDD_LOG_INFO("Hot-reloaded: {} (YAML change)", asset_id);

    } else if (auto mat_asset = std::dynamic_pointer_cast<MaterialAsset>(cache_it->second)) {
        // Material YAML change: re-parse and update bindings
        auto yaml_result = parse_yaml_file(full_changed_path);
        if (!yaml_result) {
            BUDDD_LOG_ERROR("Hot-reload YAML parse error: {}", full_changed_path);
            return;
        }
        auto yaml = std::move(*yaml_result);

        // Read shader paths
        std::string vert_path_str, frag_path_str;
        try {
            vert_path_str = yaml["shaders"]["vertex"].as<std::string>("");
            frag_path_str = yaml["shaders"]["fragment"].as<std::string>("");
        } catch (...) {}
        if (vert_path_str.empty() || frag_path_str.empty()) return;

        auto vert_path = resolve_path(vert_path_str);
        auto frag_path = resolve_path(frag_path_str);

        // Recompile shader if needed (will use cache if same key exists)
        ShaderProgramKey program_key{vert_path, frag_path};
        auto program_it = shader_programs_.find(program_key);

        auto& material = mat_asset->material();

        std::shared_ptr<ShaderProgram> shader_program;
        if (program_it != shader_programs_.end()) {
            shader_program = program_it->second;
        } else {
            // New shader pair — compile fresh
            auto vert_source = read_file(make_full_path(vert_path));
            auto frag_source = read_file(make_full_path(frag_path));
            if (!vert_source || !frag_source) {
                BUDDD_LOG_ERROR("Hot-reload: failed to read shader sources");
                return;
            }
            auto vs = device_.create_shader(ShaderType::Vertex, *vert_source);
            auto fs = device_.create_shader(ShaderType::Fragment, *frag_source);
            if (!vs || !fs) return;
            auto program = device_.create_shader_program(std::move(*vs), std::move(*fs));
            if (!program) return;
            shader_program = std::move(*program);
            shader_programs_[program_key] = shader_program;
        }

        // V1 limitation: cannot change Material's shader program after creation
        BUDDD_LOG_INFO("Hot-reload: material {} YAML changed (textures/constants will update, shader changes require re-creation)", asset_id);

        // Update texture bindings
        try {
            auto textures_node = yaml["textures"];
            if (textures_node) {
                for (auto it = textures_node.begin(); it != textures_node.end(); ++it) {
                    auto tex_name = it->first.as<std::string>();
                    auto tex_id = it->second.as<std::string>();
                    auto tex_asset = create<TextureAsset>(tex_id);
                    if (tex_asset) {
                        (void)material->set_texture(tex_name, (*tex_asset)->texture());
                    }
                }
            }
        } catch (...) {}

        // Update constant overrides
        try {
            auto constants_node = yaml["constants"];
            if (constants_node) {
                for (auto it = constants_node.begin(); it != constants_node.end(); ++it) {
                    auto name = it->first.as<std::string>();
                    auto value = it->second;
                    if (value.IsScalar()) {
                        try { (void)material->set_uniform(name, value.as<float>()); } catch (...) {}
                    }
                }
            }
        } catch (...) {}

        // Update dependency map — relative paths matching FileWatcher format
        dependency_map_.remove_asset(asset_id);
        dependency_map_.add_dependency(asset_id, std::string(asset_id) + ".yaml");
        dependency_map_.add_dependency(asset_id, vert_path);
        dependency_map_.add_dependency(asset_id, frag_path);

        BUDDD_LOG_INFO("Hot-reloaded: {} (YAML change)", asset_id);

    } else if (auto model_asset = std::dynamic_pointer_cast<ModelAsset>(cache_it->second)) {
        // Model YAML changed — reload entirely
        BUDDD_LOG_INFO("Hot-reload: {} (Model YAML changed)", asset_id);
        auto result = load_model(asset_id, full_changed_path);
        if (!result) {
            BUDDD_LOG_ERROR("Hot-reload: model reload failed: {} \u2014 retaining old model ({})", asset_id, result.error().message);
            return;
        }
        // The new asset is already cached by load_model. Remove the old one.
        // Note: load_model calls cache_[id] = asset, overwriting the old entry.
        // The old ModelNode tree is destroyed when the old shared_ptr goes out of scope.
        // Old shared_ptr<Material> references held by external code remain valid.
        BUDDD_LOG_INFO("Hot-reload: model reloaded: {}", asset_id);
    }
}

auto AssetManager::handle_source_change(const std::string& changed_path, const std::string& asset_id) -> void {
    auto cache_it = cache_.find(asset_id);
    if (cache_it == cache_.end()) return;

    // Check if this is a texture asset
    if (auto tex_asset = std::dynamic_pointer_cast<TextureAsset>(cache_it->second)) {
        // Source image changed — reload and swap handle
        // changed_path is relative to base_path_, reconstruct for I/O
        auto image = Image::load(make_full_path(changed_path));
        if (!image) {
            BUDDD_LOG_ERROR("Hot-reload: image load failed: {}", changed_path);
            return;
        }
        auto new_tex = device_.create_texture(*image);
        if (!new_tex) {
            BUDDD_LOG_ERROR("Hot-reload: texture creation failed");
            return;
        }
        auto new_handle = (*new_tex)->release_gl_handle();
        tex_asset->texture()->replace_gl_handle(new_handle);
        BUDDD_LOG_INFO("Hot-reloaded texture: {} (source: {})", asset_id, changed_path);

    } else if (auto mat_asset = std::dynamic_pointer_cast<MaterialAsset>(cache_it->second)) {
        // Shader source file changed — find and update ShaderProgram
        for (auto& [key, program] : shader_programs_) {
            if (key.vertex_path == changed_path || key.fragment_path == changed_path) {
                // Re-read both shader sources
                // key.vertex_path/key.fragment_path are relative to base_path_
                auto vert_source = read_file(make_full_path(key.vertex_path));
                auto frag_source = read_file(make_full_path(key.fragment_path));
                if (!vert_source || !frag_source) {
                    BUDDD_LOG_ERROR("Hot-reload: failed to read shader sources");
                    return;
                }

                // Recompile
                auto vs = device_.create_shader(ShaderType::Vertex, *vert_source);
                if (!vs) { BUDDD_LOG_ERROR("Hot-reload: vertex shader compile failed"); return; }
                auto fs = device_.create_shader(ShaderType::Fragment, *frag_source);
                if (!fs) { BUDDD_LOG_ERROR("Hot-reload: fragment shader compile failed"); return; }

                auto new_program = device_.create_shader_program(std::move(*vs), std::move(*fs));
                if (!new_program) {
                    BUDDD_LOG_ERROR("Hot-reload: shader program link failed \u2014 keeping old program");
                    return;
                }

                // In-place mutation: replace the handle on the existing ShaderProgram
                auto new_handle = (*new_program)->release_handle();
                program->replace_handle(new_handle);

                BUDDD_LOG_INFO("Hot-reloaded shaders: ({}, {})", key.vertex_path, key.fragment_path);
                return; // Found and updated
            }
        }

        BUDDD_LOG_INFO("Hot-reload: no shader program uses {}", changed_path);

    } else if (auto model_asset = std::dynamic_pointer_cast<ModelAsset>(cache_it->second)) {
        // glTF source file changed — reload and replace in-place
        BUDDD_LOG_INFO("Hot-reload: {} (glTF source changed)", asset_id);

        // Re-read YAML to get scale setting
        auto yaml_path = base_path_ + "/" + asset_id + ".yaml";
        auto yaml_result = parse_yaml_file(yaml_path);
        if (!yaml_result) {
            BUDDD_LOG_ERROR("Hot-reload: YAML parse error for {}", asset_id);
            return;
        }
        auto yaml = std::move(*yaml_result);
        float scale = 1.0f;
        try { scale = yaml["settings"]["scale"].as<float>(1.0f); } catch (...) {}

        // Reload the model
        auto result = detail::load_gltf_model(device_, make_full_path(changed_path), scale);
        if (!result) {
            BUDDD_LOG_ERROR("Hot-reload: model reload failed: {} \u2014 retaining old model", asset_id);
            return;
        }

        // Replace in-place
        model_asset->replace_root(std::move(result->root));
        BUDDD_LOG_INFO("Hot-reload: model reloaded: {}", asset_id);
    }
}

// ============================================================================
// Explicit instantiations
// ============================================================================

template auto AssetManager::create<TextureAsset>(std::string_view id) -> Result<std::shared_ptr<TextureAsset>>;
template auto AssetManager::create<MaterialAsset>(std::string_view id) -> Result<std::shared_ptr<MaterialAsset>>;
template auto AssetManager::create<ModelAsset>(std::string_view id) -> Result<std::shared_ptr<ModelAsset>>;

} // namespace buddd::engine
