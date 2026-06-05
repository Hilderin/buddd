#pragma once

// Template implementation for AssetManager::create<T>()
// Included at the bottom of asset_manager.h

#include <type_traits>

namespace buddd::engine {

template<typename T>
auto AssetManager::create(std::string_view id) -> Result<std::shared_ptr<T>> {
    // Compile-time guard: T must be TextureAsset or MaterialAsset
    static_assert(std::is_same_v<T, TextureAsset> || std::is_same_v<T, MaterialAsset>,
        "AssetManager::create<T>() is only supported for TextureAsset and MaterialAsset");

    // 1. Validate id is not empty
    if (id.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Asset ID must not be empty");
    }

    // 2. Check cache
    auto it = cache_.find(std::string(id));
    if (it != cache_.end()) {
        auto* ptr = dynamic_cast<T*>(it->second.get());
        if (!ptr) {
            return make_error(Error::Category::InvalidArgument,
                "Cached asset type mismatch for '" + std::string(id) + "'");
        }
#ifndef NDEBUG
        std::cerr << "[Asset] Cache hit: " << id << "\n";
#endif
        return std::shared_ptr<T>(it->second, ptr);
    }

    // 3. Compute YAML path
    auto yaml_path = base_path_ + "/" + std::string(id) + ".yaml";

    // 4. Delegate to type-specific loader
    if constexpr (std::is_same_v<T, TextureAsset>) {
        return load_texture(std::string(id), yaml_path);
    } else if constexpr (std::is_same_v<T, MaterialAsset>) {
        return load_material(std::string(id), yaml_path);
    }
}

} // namespace buddd::engine
