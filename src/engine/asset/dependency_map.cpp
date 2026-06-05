#include "asset/dependency_map.h"

#include <algorithm>

namespace buddd::engine {

auto DependencyMap::add_dependency(std::string_view asset_id, std::string_view source_path) -> void {
    auto asset_key = std::string(asset_id);
    auto path_key = std::string(source_path);

    auto& paths = forward_[asset_key];
    if (std::find(paths.begin(), paths.end(), path_key) == paths.end()) {
        paths.push_back(path_key);
    }

    auto& assets = reverse_[path_key];
    if (std::find(assets.begin(), assets.end(), asset_key) == assets.end()) {
        assets.push_back(asset_key);
    }
}

auto DependencyMap::get_dependencies(std::string_view asset_id) const -> std::span<const std::string> {
    auto it = forward_.find(std::string(asset_id));
    if (it == forward_.end()) return {};
    return it->second;
}

auto DependencyMap::get_dependents(std::string_view source_path) const -> std::span<const std::string> {
    auto it = reverse_.find(std::string(source_path));
    if (it == reverse_.end()) return {};
    return it->second;
}

auto DependencyMap::remove_asset(std::string_view asset_id) -> void {
    auto asset_key = std::string(asset_id);
    auto it = forward_.find(asset_key);
    if (it == forward_.end()) return;

    // Remove from reverse map
    for (const auto& path : it->second) {
        auto rev_it = reverse_.find(path);
        if (rev_it != reverse_.end()) {
            auto& assets = rev_it->second;
            assets.erase(std::remove(assets.begin(), assets.end(), asset_key), assets.end());
            if (assets.empty()) {
                reverse_.erase(rev_it);
            }
        }
    }

    forward_.erase(it);
}

auto DependencyMap::clear() -> void {
    forward_.clear();
    reverse_.clear();
}

} // namespace buddd::engine
