#include "settings/settings_store.h"
#include "scene/component_registry/type_registry.h"
#include "scene/component_registry/serialization_context.h"
#include "log/log.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

BUDDD_LOG_TAG("Settings");

namespace buddd::engine {

// ── Connection ──

Connection::Connection(std::function<void()> cleanup)
    : cleanup_(std::move(cleanup))
{
}

Connection::~Connection() {
    if (cleanup_) {
        cleanup_();
    }
}

Connection::Connection(Connection&& other) noexcept
    : cleanup_(std::move(other.cleanup_))
{
    other.cleanup_ = nullptr;
}

auto Connection::operator=(Connection&& other) noexcept -> Connection& {
    if (this != &other) {
        if (cleanup_) {
            cleanup_();
        }
        cleanup_ = std::move(other.cleanup_);
        other.cleanup_ = nullptr;
    }
    return *this;
}

// ── SettingsStore ──

SettingsStore::SettingsStore(std::filesystem::path file_path, SerializationContext ctx)
    : file_path_(std::move(file_path))
    , ctx_(ctx)
    , root_(std::make_unique<YAML::Node>(YAML::NodeType::Map))
{
}

SettingsStore::~SettingsStore() = default;

auto SettingsStore::load() -> Result<void> {
    // If file does not exist, use empty map node
    if (!std::filesystem::exists(file_path_)) {
        root_ = std::make_unique<YAML::Node>(YAML::NodeType::Map);
        dirty_ = false;
        return {};
    }

    // If path is a directory, return IoFailed
    if (std::filesystem::is_directory(file_path_)) {
        return make_error(Error::Category::IoFailed,
            "Settings: path is a directory: " + file_path_.string());
    }

    // If file is empty/zero-size, use empty map node
    try {
        if (std::filesystem::file_size(file_path_) == 0) {
            root_ = std::make_unique<YAML::Node>(YAML::NodeType::Map);
            dirty_ = false;
            return {};
        }
    } catch (const std::exception& e) {
        // file_size can throw (e.g., permission denied)
        return make_error(Error::Category::IoFailed,
            "Settings: cannot read " + file_path_.string() + ": " + e.what());
    }

    // Try to load the YAML file
    try {
        *root_ = YAML::LoadFile(file_path_.string());
    } catch (const YAML::BadFile& e) {
        return make_error(Error::Category::IoFailed,
            "Settings: cannot read " + file_path_.string() + ": " + e.what());
    } catch (const YAML::ParserException& e) {
        return make_error(Error::Category::InvalidFormat,
            "Settings: malformed YAML in " + file_path_.string() + ": " + e.what());
    } catch (const YAML::Exception& e) {
        return make_error(Error::Category::InvalidFormat,
            "Settings: malformed YAML in " + file_path_.string() + ": " + e.what());
    } catch (const std::exception& e) {
        return make_error(Error::Category::IoFailed,
            "Settings: unexpected error reading " + file_path_.string() + ": " + e.what());
    }

    dirty_ = false;
    BUDDD_LOG_INFO("Settings: loading from {}", file_path_.string());
    return {};
}

auto SettingsStore::save() -> Result<void> {
    if (!dirty_) {
        return {};
    }

    // Create parent directories if needed
    try {
        std::filesystem::create_directories(file_path_.parent_path());
    } catch (const std::exception& e) {
        return make_error(Error::Category::IoFailed,
            "Settings: cannot create directory for " + file_path_.string() + ": " + e.what());
    }

    try {
        YAML::Emitter emitter;
        emitter << *root_;
        std::ofstream file(file_path_);
        if (!file) {
            return make_error(Error::Category::IoFailed,
                "Settings: cannot open " + file_path_.string() + " for writing");
        }
        file << emitter.c_str();
        if (!file) {
            return make_error(Error::Category::IoFailed,
                "Settings: write error for " + file_path_.string());
        }
    } catch (const std::exception& e) {
        return make_error(Error::Category::IoFailed,
            "Settings: failed to save " + file_path_.string() + ": " + e.what());
    }

    dirty_ = false;
    BUDDD_LOG_INFO("Settings: saved to {}", file_path_.string());
    return {};
}

template<typename T>
auto SettingsStore::get_impl(const std::string& key, const T& default_value) const -> T {
    YAML::Node node = find_node(key);
    if (!node.IsDefined() || node.IsNull()) {
        return default_value;
    }

    auto result = TypeRegistry::yaml_decode<T>(node, ctx_);
    if (!result) {
        BUDDD_LOG_WARN("Settings: failed to decode key '{}' as type '{}': {}",
            key, typeid(T).name(), result.error().message);
        return default_value;
    }
    return std::move(*result);
}

template<typename T>
auto SettingsStore::set_impl(const std::string& key, const T& value) -> void {
    auto encoded = TypeRegistry::yaml_encode<T>(value, ctx_);
    if (!encoded) {
        BUDDD_LOG_WARN("Settings: failed to encode key '{}' as type '{}': {}",
            key, typeid(T).name(), encoded.error().message);
        return;
    }

    // Build the YAML tree using direct path-based approach.
    // Navigate from root to target, assigning at the last segment.
    // All modifications go through operator[] on a handle that shares data with *root_,
    // so the tree is correctly modified in-place.
    std::vector<std::string> segs;
    {
        std::istringstream stream(key);
        std::string segment;
        while (std::getline(stream, segment, '.')) {
            if (segment.empty()) continue;
            segs.push_back(segment);
        }
    }

    if (segs.empty()) {
        // Empty key: assign directly to root
        *root_ = *encoded;
        dirty_ = true;
        notify_observers(key);
        return;
    }

    // Navigate using YAML::Node handles.
    // IMPORTANT: Use Node::reset() (not operator=) to update the current handle
    // when descending. operator= calls AssignNode which does set_ref on the
    // underlying detail::node, corrupting the tree. reset() just replaces the
    // handle's internal pointers without modifying the tree.
    YAML::Node current = *root_;
    for (size_t i = 0; i < segs.size(); ++i) {
        const auto& seg = segs[i];
        YAML::Node child = current[seg];
        if (i == segs.size() - 1) {
            // Last segment: compare with existing value
            if (child.IsDefined() && !child.IsNull()) {
                std::string old = YAML::Dump(child);
                std::string neu = YAML::Dump(*encoded);
                if (old == neu) {
                    return;  // No change
                }
            }
            // Assign through operator[] on the current handle — modifies the tree.
            // The temporary returned by operator[] is fine — AssignNode is called
            // on the temporary, not on current.
            current[seg] = *encoded;
        } else {
            // Intermediate segment: ensure it's a map
            if (!child.IsDefined() || child.IsNull()) {
                current[seg] = YAML::Node(YAML::NodeType::Map);
                child = current[seg];
            }
            // Descend: use reset() to update current without calling set_ref.
            // Node::reset() replaces the handle's internal pointers only.
            current.reset(child);
        }
    }

    dirty_ = true;
    notify_observers(key);
}

auto SettingsStore::find_node(const std::string& key) -> YAML::Node {
    return static_cast<const SettingsStore*>(this)->find_node(key);
}

auto SettingsStore::ensure_node_path(const std::string& key) -> YAML::Node {
    if (!root_) {
        root_ = std::make_unique<YAML::Node>(YAML::NodeType::Map);
    }

    YAML::Node current = *root_;
    std::istringstream stream(key);
    std::string segment;
    std::vector<std::string> segments;

    while (std::getline(stream, segment, '.')) {
        if (segment.empty()) continue;
        segments.push_back(segment);
    }

    for (size_t i = 0; i < segments.size(); ++i) {
        const auto& seg = segments[i];
        if (i == segments.size() - 1) {
            return current[seg];
        }
        YAML::Node child = current[seg];
        if (!child.IsDefined() || child.IsNull()) {
            current[seg] = YAML::Node(YAML::NodeType::Map);
            child = current[seg];
        }
        current = child;
    }

    return current;
}

auto SettingsStore::find_node(const std::string& key) const -> YAML::Node {
    if (!root_ || root_->IsNull() || root_->Type() != YAML::NodeType::Map) {
        return YAML::Node();
    }

    YAML::Node current = *root_;
    std::istringstream stream(key);
    std::string segment;

    while (std::getline(stream, segment, '.')) {
        if (segment.empty()) continue;

        if (!current.IsMap()) {
            return YAML::Node();
        }

        // Navigate using operator[] and reset() — NEVER use assignment (operator=)
        // because it calls AssignNode which corrupts the tree via set_ref.
        YAML::Node child = current[segment];
        if (!child.IsDefined() || child.IsNull()) {
            return YAML::Node();
        }
        current.reset(child);
    }

    return current;
}

auto SettingsStore::notify_observers(const std::string& key) -> void {
    auto it = observers_.find(key);
    if (it == observers_.end()) return;

    // Copy the vector to handle self-unregistration during iteration
    auto callbacks = it->second;
    for (auto& [id, cb] : callbacks) {
        try {
            cb(key);
        } catch (const std::exception& e) {
            BUDDD_LOG_WARN("Settings: observer callback threw exception: {}", e.what());
        } catch (...) {
            BUDDD_LOG_WARN("Settings: observer callback threw unknown exception");
        }
    }
}

auto SettingsStore::observe(const std::string& key, ChangeCallback callback) -> std::unique_ptr<Connection> {
    size_t id = next_id_++;
    observers_[key].push_back({id, std::move(callback)});

    // Store copies of key and id for the cleanup lambda
    std::string key_copy = key;
    return std::unique_ptr<Connection>(new Connection([this, key_copy, id]() {
        auto it = observers_.find(key_copy);
        if (it == observers_.end()) return;
        auto& vec = it->second;
        vec.erase(std::remove_if(vec.begin(), vec.end(),
            [id](const auto& pair) { return pair.first == id; }), vec.end());
        if (vec.empty()) {
            observers_.erase(it);
        }
    }));
}

// ── Explicit template instantiations ──
// These are the types supported by SettingsStore get/set.
// uint64_t is included for test coverage of TypeRegistry's unregistered-type
// fallback (TypeRegistry::yaml_decode<uint64_t> returns InvalidArgument).

template auto SettingsStore::get_impl<bool>(const std::string&, const bool&) const -> bool;
template auto SettingsStore::get_impl<int32_t>(const std::string&, const int32_t&) const -> int32_t;
template auto SettingsStore::get_impl<float>(const std::string&, const float&) const -> float;
template auto SettingsStore::get_impl<double>(const std::string&, const double&) const -> double;
template auto SettingsStore::get_impl<std::string>(const std::string&, const std::string&) const -> std::string;
template auto SettingsStore::get_impl<uint64_t>(const std::string&, const uint64_t&) const -> uint64_t;

template auto SettingsStore::set_impl<bool>(const std::string&, const bool&) -> void;
template auto SettingsStore::set_impl<int32_t>(const std::string&, const int32_t&) -> void;
template auto SettingsStore::set_impl<float>(const std::string&, const float&) -> void;
template auto SettingsStore::set_impl<double>(const std::string&, const double&) -> void;
template auto SettingsStore::set_impl<std::string>(const std::string&, const std::string&) -> void;
template auto SettingsStore::set_impl<uint64_t>(const std::string&, const uint64_t&) -> void;

} // namespace buddd::engine
