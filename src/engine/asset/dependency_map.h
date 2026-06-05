#pragma once

#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace buddd::engine {

class DependencyMap {
public:
    auto add_dependency(std::string_view asset_id, std::string_view source_path) -> void;
    auto get_dependencies(std::string_view asset_id) const -> std::span<const std::string>;
    auto get_dependents(std::string_view source_path) const -> std::span<const std::string>;
    auto remove_asset(std::string_view asset_id) -> void;
    auto clear() -> void;

    DependencyMap() = default;
    DependencyMap(const DependencyMap&) = delete;
    auto operator=(const DependencyMap&) -> DependencyMap& = delete;
    DependencyMap(DependencyMap&&) = default;
    auto operator=(DependencyMap&&) -> DependencyMap& = default;

private:
    std::unordered_map<std::string, std::vector<std::string>> forward_;  // asset_id -> [source_paths]
    std::unordered_map<std::string, std::vector<std::string>> reverse_;  // source_path -> [asset_ids]
};

} // namespace buddd::engine
