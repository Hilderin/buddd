#pragma once

#include "error.h"
#include "scene/component_registry/serialization_context.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <utility>  // for pair

namespace YAML { class Node; }

namespace buddd::engine {

/// RAII handle that unregisters an observer when destroyed.
class Connection {
public:
    Connection() = default;
    ~Connection();
    Connection(Connection&&) noexcept;
    Connection& operator=(Connection&&) noexcept;
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
private:
    friend class SettingsStore;
    explicit Connection(std::function<void()> cleanup);
    std::function<void()> cleanup_;
};

/// A YAML-backed settings store with dot-path access, TypeRegistry-based
/// type conversion, dirty tracking, and RAII observer registration.
class SettingsStore {
public:
    explicit SettingsStore(std::filesystem::path file_path, SerializationContext ctx);
    ~SettingsStore();

    SettingsStore(const SettingsStore&) = delete;
    SettingsStore& operator=(const SettingsStore&) = delete;
    SettingsStore(SettingsStore&&) noexcept = default;
    SettingsStore& operator=(SettingsStore&&) noexcept = default;

    /// Load settings from the file. If file does not exist, uses default (empty) state.
    [[nodiscard]] auto load() -> Result<void>;

    /// Save settings to file. No-op if not dirty.
    [[nodiscard]] auto save() -> Result<void>;

    /// Get a typed value by dot-separated key path.
    /// Uses TypeRegistry for type conversion.
    /// Returns default_value if key missing, type not registered, or decode fails.
    template<typename T>
    [[nodiscard]] auto get(const std::string& key, const T& default_value = T{}) const -> T {
        return get_impl<T>(key, default_value);
    }

    /// Set a typed value by dot-separated key path.
    /// Uses TypeRegistry for type conversion.
    /// No-op + warning if type not registered.
    template<typename T>
    auto set(const std::string& key, const T& value) -> void {
        set_impl<T>(key, value);
    }

    [[nodiscard]] auto is_dirty() const noexcept -> bool { return dirty_; }

    using ChangeCallback = std::function<void(const std::string& key)>;

    /// Register an observer. Returns a RAII Connection that auto-unregisters on destruction.
    [[nodiscard]] auto observe(const std::string& key, ChangeCallback callback) -> std::unique_ptr<Connection>;

private:
    // Template helpers defined only in .cpp with explicit instantiations
    template<typename T>
    auto get_impl(const std::string& key, const T& default_value) const -> T;

    template<typename T>
    auto set_impl(const std::string& key, const T& value) -> void;

    // YAML node navigation (yaml-cpp uses value semantics for Node)
    auto find_node(const std::string& key) -> YAML::Node;
    auto find_node(const std::string& key) const -> YAML::Node;
    auto ensure_node_path(const std::string& key) -> YAML::Node;

    // Observer notification
    auto notify_observers(const std::string& key) -> void;

    std::filesystem::path file_path_;
    SerializationContext ctx_;
    std::unique_ptr<YAML::Node> root_;
    bool dirty_ = false;

    // Observer storage: key -> vector of (id, callback)
    std::unordered_map<std::string, std::vector<std::pair<size_t, std::function<void(const std::string&)>>>> observers_;
    size_t next_id_ = 0;
};

} // namespace buddd::engine
