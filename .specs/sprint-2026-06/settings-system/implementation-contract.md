# IMPL-036 — Settings System (MVP1 Skeleton)

## Source spec

`.specs/sprint-2026-06/settings-system/spec.md`

## Goal

Build the infrastructure for a YAML-backed three-tier settings system using TypeRegistry for type conversion: `SettingsStore` (generic YAML key-value store with dot-path access, SerializationContext, dirty tracking, and RAII observer registration), `SettingsManager` (orchestrator owning editor/project/user-project stores with path resolution derived from centralized `editor_data_root()`/`editor_user_data_root()` utilities), `os_user_config_dir()` (platform-specific config directory, moved to `src/engine/util/`), and Editor lifecycle integration (load in `setup()`, save in `shutdown()`). SettingsStore uses TypeRegistry for `get<T>()`/`set<T>()` and SerializationContext for context-dependent encode/decode. No INI migration — ImGui layout file path is simply set to `.buddd/user/layout.ini`. All components live in `src/engine/settings/` and `src/engine/util/`. No actual settings keys are defined — MVP1 is skeleton-only.

## Non-goals

- Do NOT define or use any concrete editor/project/user-project settings keys (theme, shortcuts, project name, renderer config, last paths, open tabs).
- Do NOT add any ImGui includes (`<imgui.h>`, `<imgui_internal.h>`) in `src/engine/settings/` or `src/engine/util/` headers or `.cpp` files.
- Do NOT modify `src/engine/CMakeLists.txt`, `src/editor/CMakeLists.txt`, or `tests/CMakeLists.txt` — the existing `GLOB_RECURSE` auto-discovers new files.
- Do NOT auto-create `buddd.project.yaml` — it is optional and only created by the user.
- Do NOT change `build_title_string()` or any existing Editor constructor/destructor logic beyond the specific setup/shutdown hooks.
- Do NOT add hot-reload, settings UI panels, file format versioning, multi-project support, or network-synced settings.
- Do NOT implement INI migration from `buddd_editor.ini` to `.buddd/user/layout.ini`. The old INI file is left untouched. The new layout path is set directly.
- Do NOT provide a `migrate_ini()` method on `SettingsManager`.

## Relevant ADRs

| ADR | Constraint |
|-----|-----------|
| ADR-001 | Error propagation via `Result<T>` / `Error` struct. yaml-cpp exceptions caught and converted to `Result` errors. |
| ADR-016 | yaml-cpp is a PRIVATE dependency of `buddd_engine`. Settings headers must NOT include `<yaml-cpp/yaml.h>` — only `.cpp` files may use it. All `YAML::LoadFile()` calls wrapped in `try-catch`. TypeRegistry headers (`type_registry.h`), which include yaml-cpp, must NOT be included in `settings_store.h` — only in `settings_store.cpp`. |
| ADR-019 | `src/engine/settings/` and `src/engine/util/` headers must NOT include SDL3, OpenGL, GLM, or any `render/` headers. Only `std::filesystem`, `<cstdlib>`, and standard library headers are allowed in public headers. |
| ADR-026 | ImGui lifecycle is managed by RenderDevice. SettingsManager only sets `ImGui::GetIO().IniFilename` — it does NOT init/shutdown ImGui. |
| ADR-007 | All compiled FetchContent dependencies built in Release mode (already configured for yaml-cpp). |
| ADR-011 | `[[nodiscard]]` on all `Result<T>`-returning functions. Non-copyable-by-default, movable where needed. |

## Files to inspect

| File | Reason |
|------|--------|
| `src/engine/error.h` | Confirm existing `InvalidFormat` and `IoFailed` error categories are sufficient — no new categories needed |
| `src/engine/scene/component_registry/type_registry.h` | Understand `TypeRegistry::yaml_encode<T>()` / `yaml_decode<T>()` static API for SettingsStore get/set integration |
| `src/engine/scene/component_registry/serialization_context.h` | Understand `SerializationContext` struct (has `AssetManager& assets` member) — needed by SettingsStore constructor |
| `src/engine/CMakeLists.txt` | Verify `GLOB_RECURSE` picks up all new files under `src/engine/settings/` and `src/engine/util/`; confirm yaml-cpp include/link setup |
| `src/editor/editor.h` | Understand current member layout, public accessor pattern, and `#include` structure |
| `src/editor/editor.cpp` | Understand `Editor::setup()` ImGui init section, `IniFilename` setting, and `Editor::shutdown()` no-op body |
| `src/editor/CMakeLists.txt` | Verify `GLOB_RECURSE` picks up new editor test files |
| `tests/CMakeLists.txt` | Verify `GLOB_RECURSE` picks up `*_tests.cpp` and understand `BUDDD_HAS_DISPLAY` guard pattern |
| `tests/editor/editor_tests.cpp` | Understand `HeadlessTestContext` pattern and display-backed test pattern (`#ifdef BUDDD_HAS_DISPLAY`, `SDL_SetHint`) |
| `docs/adr/ADR-016-yaml-cpp-dependency.md` | yaml-cpp exception-safe wrapper pattern |
| `docs/adr/ADR-019-architecture-boundaries.md` | Confirms no platform/graphics headers outside `src/engine/` |

## Files allowed to change

### New files (create)
- `src/engine/util/os_config_dir.h`
- `src/engine/util/os_config_dir.cpp`
- `src/engine/util/editor_data_root.h`
- `src/engine/util/editor_data_root.cpp`
- `src/engine/settings/settings_store.h`
- `src/engine/settings/settings_store.cpp`
- `src/engine/settings/settings_manager.h`
- `src/engine/settings/settings_manager.cpp`
- `tests/engine/settings_store_tests.cpp`
- `tests/editor/settings_integration_tests.cpp`

### Existing files (modify)
- `src/editor/editor.h` — add `#include "settings/settings_manager.h"`, `std::unique_ptr<SettingsManager> settings_manager_` member, `auto settings_manager() -> SettingsManager&;` accessor
- `src/editor/editor.cpp` — integrate SettingsManager lifecycle in `setup()` and `shutdown()`, replace hardcoded INI logic with `SettingsManager::layout_ini_path()`; NO `migrate_ini()` call

## Files forbidden to change

- `src/engine/CMakeLists.txt` — GLOB_RECURSE auto-discovers new files under `src/engine/settings/` and `src/engine/util/`
- `src/editor/CMakeLists.txt` — GLOB_RECURSE auto-discovers new test files
- `tests/CMakeLists.txt` — GLOB_RECURSE + list filter handles new test files
- `src/engine/error.h` — no new error categories needed (reuse `InvalidFormat`, `IoFailed`, `InitFailed`)
- `src/engine/scene/component_registry/type_registry.h` — NOT modified (used as-is via include in `.cpp` only)
- `src/engine/scene/component_registry/serialization_context.h` — NOT modified
- Any file outside the explicit allowed list above

## Existing conventions to follow

### Code style
- `#pragma once` header guards (not `#ifndef`/`#define`).
- Namespace `buddd::engine` for engine code, `buddd::editor` for editor code.
- `[[nodiscard]]` on all `Result<T>`-returning functions. `[[nodiscard]]` on accessors returning references (`auto editor_settings() -> SettingsStore&;`).
- Non-copyable, movable classes: `SettingsStore(const SettingsStore&) = delete; SettingsStore& operator=(const SettingsStore&) = delete; SettingsStore(SettingsStore&&) noexcept = default;`.
- `auto` return type trailing return type syntax for all functions.
- `explicit` on single-argument constructors.
- Use `make_error()` to construct `std::unexpected<Error>` return values.

### Logging
- Tag at file scope: `BUDDD_LOG_TAG("Settings");`
- Log level usage: `Info` for load/save events, `Warn` for failures and TypeRegistry decode failures, `Debug` for directory creation.
- All settings-related messages use the `"Settings"` log tag.

### Error handling
- yaml-cpp exceptions wrapped in `try-catch` per ADR-016 pattern:
  ```cpp
  try {
      *root_ = YAML::LoadFile(file_path_.string());
  } catch (const YAML::Exception& e) {
      return make_error(Error::Category::InvalidFormat,
          "Settings: malformed YAML in " + file_path_.string() + ": " + e.what());
  } catch (const std::exception& e) {
      return make_error(Error::Category::IoFailed,
          "Settings: unexpected error reading " + file_path_.string() + ": " + e.what());
  }
  ```
- Return `std::unexpected<Error>` via `make_error()` on failure.

### TypeRegistry integration
- `TypeRegistry::yaml_encode<T>()` and `TypeRegistry::yaml_decode<T>()` are static template functions that return `Result<YAML::Node>` and `Result<T>` respectively.
- TypeRegistry is a static-only class with pre-registered built-in types (bool, int32_t, float, double, std::string, Vec3, Vec4, Quat).
- For unregistered types, TypeRegistry returns an `InvalidArgument` error. The store must log a warning and return default / no-op.
- `TypeRegistry` headers include yaml-cpp — DO NOT include `type_registry.h` in `settings_store.h`. Include only in `settings_store.cpp`.

### SerializationContext
- `SerializationContext` is a simple struct with one member: `AssetManager& assets`.
- It must be passed to `TypeRegistry::yaml_encode<T>()` / `yaml_decode<T>()` for context-dependent encode/decode.

### Testing
- Test file naming: `*_tests.cpp`, auto-discovered by CMake GLOB_RECURSE.
- Tags: `[settings]`, `[settings_store]`, `[settings_manager]`.
- Display-backed tests guarded with `#ifdef BUDDD_HAS_DISPLAY`.
- Display-backed tests use `SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")` before engine creation.
- Headless tests use `Backend::Headless`.
- Use `std::filesystem::temp_directory_path()` for temporary test files.
- Use `Catch2` (`#include <catch2/catch_test_macros.hpp>`).

### Movement semantics
- Only move semantics on `Connection` and `SettingsStore` (no copy).
- `Connection` is stored by `std::unique_ptr` — returned from `observe()`, not by value.

## Required implementation behavior

### 1. `os_user_config_dir()` (`src/engine/util/os_config_dir.h/.cpp`)

```cpp
namespace buddd::engine {
    [[nodiscard]] auto os_user_config_dir() -> std::filesystem::path;
}
```

Implementation:
```cpp
auto os_user_config_dir() -> std::filesystem::path {
#if defined(_WIN32)
    if (auto* appdata = std::getenv("APPDATA"))
        return std::filesystem::path(appdata) / "buddd";
    return std::filesystem::path("C:/") / "Users" / "Default" / "AppData" / "Roaming" / "buddd";
#elif defined(__APPLE__)
    if (auto* home = std::getenv("HOME"))
        return std::filesystem::path(home) / "Library" / "Application Support" / "buddd";
    return std::filesystem::path("/tmp/buddd-config");
#else  // Linux
    if (auto* xdg = std::getenv("XDG_CONFIG_HOME"))
        return std::filesystem::path(xdg) / "buddd";
    if (auto* home = std::getenv("HOME"))
        return std::filesystem::path(home) / ".config" / "buddd";
    return std::filesystem::path("/tmp/buddd-config");
#endif
}
```

Only includes `<cstdlib>` and `<filesystem>` — no yaml-cpp, no SDL3, no platform/graphics headers.

### 2. `editor_data_root()` and `editor_user_data_root()` (`src/engine/util/editor_data_root.h/.cpp`)

```cpp
namespace buddd::engine {
    /// Returns <project_root>/.buddd/
    [[nodiscard]] auto editor_data_root(const std::filesystem::path& project_root) -> std::filesystem::path;

    /// Returns <project_root>/.buddd/user/
    [[nodiscard]] auto editor_user_data_root(const std::filesystem::path& project_root) -> std::filesystem::path;
}
```

Implementation:
```cpp
auto editor_data_root(const std::filesystem::path& project_root) -> std::filesystem::path {
    return project_root / ".buddd";
}

auto editor_user_data_root(const std::filesystem::path& project_root) -> std::filesystem::path {
    return editor_data_root(project_root) / "user";
}
```

### 3. `SettingsStore` (`src/engine/settings/settings_store.h/.cpp`)

#### Public API (exact signatures)

```cpp
#pragma once
#include "error.h"
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include "scene/component_registry/serialization_context.h"

namespace YAML { class Node; }

namespace buddd::engine {

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

class SettingsStore {
public:
    explicit SettingsStore(std::filesystem::path file_path, SerializationContext ctx);
    ~SettingsStore();

    SettingsStore(const SettingsStore&) = delete;
    SettingsStore& operator=(const SettingsStore&) = delete;
    SettingsStore(SettingsStore&&) noexcept = default;
    SettingsStore& operator=(SettingsStore&&) noexcept = default;

    [[nodiscard]] auto load() -> Result<void>;
    [[nodiscard]] auto save() -> Result<void>;

    template<typename T>
    [[nodiscard]] auto get(const std::string& key, const T& default_value = T{}) const -> T;

    template<typename T>
    auto set(const std::string& key, const T& value) -> void;

    [[nodiscard]] auto is_dirty() const noexcept -> bool;

    using ChangeCallback = std::function<void(const std::string& key)>;
    [[nodiscard]] auto observe(const std::string& key, ChangeCallback callback) -> std::unique_ptr<Connection>;

private:
    auto find_node(const std::string& key) -> YAML::Node*;
    auto find_node(const std::string& key) const -> const YAML::Node*;
    auto ensure_node_path(const std::string& key) -> YAML::Node;
    auto notify_observers(const std::string& key) -> void;

    std::filesystem::path file_path_;
    SerializationContext ctx_;
    std::unique_ptr<YAML::Node> root_;  // pimpl — yaml-cpp only in .cpp
    bool dirty_ = false;

    std::unordered_map<std::string,
        std::vector<std::pair<size_t, std::function<void(const std::string&)>>>> observers_;
    size_t next_id_ = 0;
};

} // namespace buddd::engine
```

#### Implementation details

**Store**: `std::unique_ptr<YAML::Node> root_` as opaque pointer (pimpl-like per ADR-016 — yaml-cpp headers never included in `.h` files). The destructor `~SettingsStore()` is defined in `settings_store.cpp` where `YAML::Node` is complete. Template `get<T>()` and `set<T>()` are **declared** in the header but **defined** in `settings_store.cpp` with explicit instantiations for `bool`, `int32_t`, `float`, `double`, `std::string`. This keeps yaml-cpp and TypeRegistry includes entirely out of the public header while preserving the template call syntax for callers.

**`settings_store.cpp` includes**:
```cpp
#include "settings/settings_store.h"
#include "scene/component_registry/type_registry.h"
#include "scene/component_registry/serialization_context.h"
#include "log/log.h"
#include <yaml-cpp/yaml.h>
```

**Constructor**: Store `file_path_` and `ctx_`. Initialize `root_` with `std::make_unique<YAML::Node>()` (empty node). Do NOT load the file.

**`load()`**:
1. If file does not exist (`!std::filesystem::exists(file_path_)`): reset `root_` to empty node, `dirty_ = false`, return success.
2. If file exists but is empty/zero-size: reset `root_` to empty node, `dirty_ = false`, return success.
3. Otherwise: `*root_ = YAML::LoadFile(file_path_.string())` wrapped in try-catch per ADR-016 pattern.
   - `YAML::BadFile` → `IoFailed` error.
   - `YAML::ParserException` → `InvalidFormat` error.
   - `YAML::Exception` → `InvalidFormat` error.
   - `std::exception` → `IoFailed` error.
4. After successful load: `dirty_ = false`.
5. Log at Info: `"Settings: loading from {path}"`.

**`save()`**:
1. If `!dirty_`: return success (no-op).
2. Create parent directory if it doesn't exist: `std::filesystem::create_directories(file_path_.parent_path())`.
3. Write using `YAML::Emitter`:
   ```cpp
   YAML::Emitter emitter;
   emitter << *root_;
   std::ofstream file(file_path_);
   if (!file) { return make_error(Error::Category::IoFailed, "..."); }
   file << emitter.c_str();
   ```
4. Wrapped in try-catch for I/O errors → `IoFailed`.
5. After successful write: `dirty_ = false`.
6. Log at Info: `"Settings: saved to {path}"`.

**`get<T>(key, default_value)`**:
1. Call `find_node(key)`. If null or null node, return `default_value`.
2. Call `TypeRegistry::yaml_decode<T>(*node, ctx_)`.
3. If decode fails (unregistered type or decode error): log warning `"Settings: failed to decode key '{}' as type '{}': {}"`, return `default_value`.
4. Otherwise return decoded value.

**`set<T>(key, value)`**:
1. Call `TypeRegistry::yaml_encode<T>(value, ctx_)`.
2. If encode fails: log warning `"Settings: failed to encode key '{}' as type '{}': {}"`, return (no-op).
3. Call `ensure_node_path(key)` to get or create the target node.
4. Compare encoded node with existing value via `YAML::are_equal()` (or string-compare using `YAML::Dump`). If equal, return (no dirty, no observer fire).
5. Otherwise: assign node, set `dirty_ = true`, call `notify_observers(key)`.

**`find_node(key)`** (const):
- Split `key` by `'.'`. Skip empty segments.
- Traverse from `*root_` following segments. If any intermediate segment is missing or not a map, return nullptr.
- Return pointer to the final node, or nullptr if path doesn't resolve.

**`find_node(key)`** (non-const): Same as const version but returns non-const pointer.

**`ensure_node_path(key)`**:
- Ensure `root_` is allocated.
- Split key by `'.'`, skip empty segments.
- For each segment except the last, ensure `(*root_)[segment]` is a map (create YAML::Node(YAML::NodeType::Map) if missing/null).
- Return reference to the node for the last segment (may be null/undefined).

**`is_dirty()`**: Returns `dirty_`.

**`observe(key, callback)`**:
1. Assign an incrementing ID (`next_id_++`) to the observer.
2. Push `{id, callback}` into `observers_[key]`.
3. Return `std::unique_ptr<Connection>` whose cleanup lambda uses the ID to find and erase the entry from the vector. If the vector becomes empty, erase the key from the map.

**Connection**:
- Stores a `std::function<void()> cleanup_` lambda.
- Destructor invokes `cleanup_` if non-null.
- Move transfers cleanup_ and nullifies source.
- `unique_ptr<Connection>` ownership model: caller owns the handle; destroying it unregisters the observer.

**`notify_observers(key)`**:
- Find the observer vector for the key. If not found, return.
- Copy the vector (pair<id, callback> copy) and iterate over the copy.
- Wrap each invocation in try-catch: catch `std::exception` and `...`, log at Warn and continue.
- Use copy-before-call pattern to handle observer self-unregistration during invocation.

**Explicit instantiations** (in `.cpp`):
```cpp
template auto SettingsStore::get<bool>(const std::string&, const bool&) const -> bool;
template auto SettingsStore::get<int32_t>(const std::string&, const int32_t&) const -> int32_t;
template auto SettingsStore::get<float>(const std::string&, const float&) const -> float;
template auto SettingsStore::get<double>(const std::string&, const double&) const -> double;
template auto SettingsStore::get<std::string>(const std::string&, const std::string&) const -> std::string;

template auto SettingsStore::set<bool>(const std::string&, const bool&) -> void;
template auto SettingsStore::set<int32_t>(const std::string&, const int32_t&) -> void;
template auto SettingsStore::set<float>(const std::string&, const float&) -> void;
template auto SettingsStore::set<double>(const std::string&, const double&) -> void;
template auto SettingsStore::set<std::string>(const std::string&, const std::string&) -> void;
```

### 4. `SettingsManager` (`src/engine/settings/settings_manager.h/.cpp`)

#### Public API (exact signatures)

```cpp
#include "error.h"
#include "settings/settings_store.h"
#include "scene/component_registry/serialization_context.h"

namespace buddd::engine {

class SettingsManager {
public:
    explicit SettingsManager(std::filesystem::path project_root, SerializationContext ctx);
    ~SettingsManager() = default;

    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;
    SettingsManager(SettingsManager&&) = delete;  // not movable — stores persistent string for IniFilename ptr

    [[nodiscard]] auto load_all() -> Result<void>;
    [[nodiscard]] auto save_all() -> Result<void>;

    [[nodiscard]] auto editor_settings() -> SettingsStore&;
    [[nodiscard]] auto project_settings() -> SettingsStore&;
    [[nodiscard]] auto user_project_settings() -> SettingsStore&;

    /// Returns the resolved path for the ImGui layout INI file.
    /// Returns a const reference to a persistent std::string member, ensuring
    /// the const char* obtained via .c_str() remains valid for SettingsManager lifetime.
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
```

#### Path resolution

| Tier | Path |
|------|------|
| Editor settings | `os_user_config_dir() / "editor.yaml"` |
| Project settings | `project_root_ / "buddd.project.yaml"` |
| User project settings | `editor_user_data_root(project_root_) / "settings.yaml"` |
| Layout INI | `editor_user_data_root(project_root_) / "layout.ini"` |

#### Constructor
- Store `project_root_` and `ctx_`.
- Construct but do NOT load the three `SettingsStore` instances:
  - `editor_settings_` = `SettingsStore(os_user_config_dir() / "editor.yaml", ctx_)`
  - `project_settings_` = `SettingsStore(project_root_ / "buddd.project.yaml", ctx_)`
  - `user_project_settings_` = `SettingsStore(editor_user_data_root(project_root_) / "settings.yaml", ctx_)`
- Initialize `ini_path_` to `(editor_user_data_root(project_root_) / "layout.ini").string()`.

#### `load_all()`
1. Create `.buddd/user/` directory: `std::filesystem::create_directories(editor_user_data_root(project_root_))`. Log at Debug. On failure, log at Warn and return `InitFailed` error.
2. Call `load()` on all three stores in order (editor, project, user-project).
3. If any store fails to load, log at Warn but continue (non-fatal — use defaults).
4. Return success even if individual store loads failed (logged).

#### `save_all()`
1. Call `save()` on each dirty store.
2. Log failures at Warn level: `"Settings: failed to save {path}: {error}"`.
3. Return first error encountered (continue trying remaining stores).

#### `layout_ini_path()`
- Returns `ini_path_` as `const std::string&`. This is the persistent backing string whose `.c_str()` is passed to `ImGui::GetIO().IniFilename`.

#### Accessors
- `editor_settings()`, `project_settings()`, `user_project_settings()` each return a reference to the corresponding `SettingsStore`.

### 5. Editor integration (`src/editor/editor.h`, `src/editor/editor.cpp`)

#### `editor.h` changes

1. Add `#include "settings/settings_manager.h"` at the top with other includes.
2. Add member after `selection_`:
   ```cpp
   // ── Settings system (MVP1) ──
   std::unique_ptr<buddd::engine::SettingsManager> settings_manager_;
   ```
3. Add public accessor before `private:`:
   ```cpp
   [[nodiscard]] auto settings_manager() -> buddd::engine::SettingsManager&;
   ```

#### `editor.cpp` changes — `Editor::setup()`

Replace the INI-block (lines 74-95 in current file) with the following:

```cpp
// ── Settings system initialisation (MVP1) ──
{
    auto sctx = be::SerializationContext{engine_->assets()};
    settings_manager_ = std::make_unique<be::SettingsManager>(
        std::filesystem::current_path(), sctx);
    ImGui::GetIO().IniFilename = settings_manager_->layout_ini_path().c_str();
    BUDDD_LOG_INFO("Editor: layout file: {}", settings_manager_->layout_ini_path());

    auto load_result = settings_manager_->load_all();
    if (!load_result) {
        BUDDD_LOG_WARN("Editor: settings load warning: {} (using defaults)",
            load_result.error().message);
    }
}
```

**Important**: The entire old INI block (lines 74-95: `ifstream` validation, `has_docking`/`has_windows` check, `std::remove`, `ImGui::GetIO().IniFilename = "buddd_editor.ini"` and its log line) is removed entirely. There is NO `migrate_ini()` call. The new path is set directly.

**Order in `setup()`**: The settings block goes AFTER the ImGui initialization check (line 66-69), and AFTER `ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;` (line 72), replacing the old INI handling block (lines 74-95).

#### `editor.cpp` changes — `Editor::shutdown()`

Add before `initialized_ = false;`:

```cpp
if (settings_manager_) {
    auto save_result = settings_manager_->save_all();
    if (!save_result) {
        BUDDD_LOG_WARN("Editor: settings save warning: {}",
            save_result.error().message);
    }
}
```

#### `editor.cpp` — `settings_manager()` accessor

```cpp
auto Editor::settings_manager() -> be::SettingsManager& {
    return *settings_manager_;
}
```

### 6. Observer fire safety

When `set()` triggers observers, use the copy-before-call pattern with try-catch wrapping:

```cpp
auto it = observers_.find(key);
if (it == observers_.end()) return;
auto callbacks = it->second;  // copy the vector of pairs
for (auto& [id, cb] : callbacks) {
    try {
        cb(key);
    } catch (const std::exception& e) {
        BUDDD_LOG_WARN("Settings: observer callback threw exception: {}", e.what());
    } catch (...) {
        BUDDD_LOG_WARN("Settings: observer callback threw unknown exception");
    }
}
```

This ensures that if a callback destroys its own `Connection` (which modifies the original vector), the iteration is safe. It also guarantees that exceptions from callbacks never propagate out of `set()`.

### 7. YAML::Node comparison for dirty tracking

For the set-same-value-no-dirty optimization, compare nodes by serializing to string via `YAML::Emitter` or `YAML::Dump` and comparing strings. Alternatively, for scalar types, compare directly via `.as<T>()`. For MVP1, string comparison via `YAML::Dump(old) != YAML::Dump(new)` is sufficient.

## Required tests

### Unit tests (`tests/engine/settings_store_tests.cpp`)

All tests in this file are headless-compatible (no display needed). Use `std::filesystem::temp_directory_path()` for temporary file creation.

| AC | Test description |
|----|-----------------|
| AC-001 | Construct `SettingsStore` with temp path and a `SerializationContext`, call `load()` (file doesn't exist), verify `get()` returns default for unknown key. |
| AC-002 | Write a valid YAML file with known keys (string, int, bool, float), `load()`, then `get()` each key returns correct value via TypeRegistry. |
| AC-003 | Test `load()` on a directory path (not a file) returns `IoFailed`. |
| AC-004 | Test `load()` on non-existent file path returns success, all `get()` calls return defaults. |
| AC-005 | Write garbage content to file, `load()` returns `InvalidFormat` error. |
| AC-006 | Set keys via TypeRegistry, `save()`, read back file content via `YAML::LoadFile`, verify values match. |
| AC-007 | Attempt `save()` to a non-writable path (e.g., read-only directory) — returns `IoFailed`. |
| AC-008 | Load, save (no changes), verify file not touched (compare content or use timestamp heuristic). |
| AC-009 | Verify `is_dirty()` is `false` after construction/load, `true` after `set()`, `false` after `save()`. |
| AC-010 | Set string, int32_t, bool, float, double values via TypeRegistry, `get()` them back, verify dirty flag transitions correctly for each. |
| AC-011 | `get(existing_key)` returns stored value; `get(missing_key)` returns provided default. |
| AC-012 | Register observer for key `"editor.theme"`, set that key, verify callback invoked; set `"other.key"`, verify callback NOT invoked. |
| AC-013 | Register two observers for same key, set key, BOTH callbacks are invoked. |
| AC-014 | Construct `SettingsManager` with a temp dir as project root and a `SerializationContext`, verify three stores exist with correct paths. Use `PathsMatch(matcher)` helper via `.string()` comparison. |
| AC-015 | Platform-dependent test (guarded by `#ifdef _WIN32` / `__APPLE__` / `__linux__`): verify `SettingsManager` editor settings path resolves to OS-appropriate config dir. |
| AC-016 | Verify project settings path: CWD `/tmp/testproj` → `/tmp/testproj/buddd.project.yaml`. |
| AC-017 | Verify user project settings path via `editor_user_data_root()`: CWD `/tmp/testproj` → `/tmp/testproj/.buddd/user/settings.yaml`. |
| AC-018 | Call `load_all()` with temp dir, verify `.buddd/user/` directory now exists. Verify the directory is created even if no settings files exist. |
| AC-019 | Set key on each store, `save_all()`, read each file, verify content matches. Also verify that clean stores are NOT written (check modification time or content). |
| AC-020 | `Editor::setup()` calls `SettingsManager::load_all()`. (Integration test, requires `BUDDD_HAS_DISPLAY=ON`.) |
| AC-021 | `Editor::shutdown()` calls `SettingsManager::save_all()`. (Integration test, requires `BUDDD_HAS_DISPLAY=ON`.) |
| AC-022 | `SettingsStore` does NOT include any platform/graphics headers (ADR-019 compliance). Code review: `src/engine/settings/` headers must not include SDL3, OpenGL, GLM, or any `render/` headers. |
| AC-023 | Setting a key to its current value does not mark the store as dirty. |
| AC-024 | `SettingsStore` supports nested YAML keys via dot-separated paths (e.g. `"renderer.resolution.width"`). |
| AC-025 | Destroying a `Connection` object unregisters its observer — subsequent `set()` calls on the observed key no longer trigger the callback. |
| AC-026 | `editor_data_root()` returns `<cwd>/.buddd/` for any given project root path. |
| AC-027 | `editor_user_data_root()` returns `<cwd>/.buddd/user/` for any given project root path. |
| AC-028 | `SettingsStore::get<bool>()` reads a YAML boolean value correctly via TypeRegistry. |
| AC-029 | `SettingsStore::get<int32_t>()` reads a YAML integer value correctly via TypeRegistry. |
| AC-030 | `SettingsStore::get<float>()` reads a YAML float value correctly via TypeRegistry. |
| AC-031 | `SettingsStore::get<std::string>()` reads a YAML string value correctly via TypeRegistry. |
| AC-032 | `SettingsStore::set/get` with an unregistered type (e.g. `uint64_t`) logs a warning and returns default / is no-op. |

Additional tests not tied to a specific AC but required for coverage:
- Test `os_user_config_dir()` returns non-empty path on all platforms.
- Test empty settings file (zero bytes) `load()` succeeds.
- Test settings file with only comments loads successfully.
- Test `observe()` with multiple keys — only the observed key's callback fires.
- Test `Connection` move semantics: move-construct Connection, verify original is null and moved-to works.
- Test `Connection` destruction without having been moved-from (cleanup lambda executes).
- Test observer callback throws exception: register a callback that throws `std::runtime_error`, call `set()` on the observed key, verify no exception propagates out of `set()` (Catch2 `REQUIRE_NOTHROW`), and verify the value was still set correctly.
- Test `SerializationContext` is passed through correctly to TypeRegistry by verifying that encode/decode functions called by the store receive the same context instance.
- Test `Editor::setup()` multiple times: reuse the same Editor instance, call `setup()` twice, verify second call does not crash, verify `settings_manager()` still returns a valid reference. (Integration test, guarded with `#ifdef BUDDD_HAS_DISPLAY`.)
- Test `Editor::shutdown()` without `setup()`: construct Editor, call `shutdown()` directly (no preceding `setup()`), verify no crash or undefined behavior. (Integration test, guarded with `#ifdef BUDDD_HAS_DISPLAY`.)

### Integration tests (`tests/editor/settings_integration_tests.cpp`)

Guarded with `#ifdef BUDDD_HAS_DISPLAY`. Follow the existing display-backed test pattern from `editor_tests.cpp` (use `SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")`).

| AC | Test description |
|----|-----------------|
| AC-020 | Create EngineService with SDL3 backend, construct Editor, call `Editor::setup()`, verify `.buddd/user/` directory exists and `ImGui::GetIO().IniFilename` points to `.buddd/user/layout.ini`. |
| AC-021 | Create EngineService with SDL3 backend, construct Editor, call `Editor::setup()`, access `SettingsManager` via `Editor::settings_manager()`, set a key on `editor_settings()`, call `Editor::shutdown()`, verify the edited settings file was written to disk with correct content. |

## Edge cases

| Case | Expected behavior |
|------|------------------|
| Settings file is empty (zero bytes) | `load()` succeeds, store contains empty YAML node, `get()` returns defaults. |
| Settings file contains only comments | Same as empty. |
| Settings file has binary blob value | yaml-cpp parses it but TypeRegistry decode fails → log warning, return default. Store remains usable for other keys. |
| Project CWD contains no `.buddd/` directory | `load_all()` creates `.buddd/user/` recursively via `editor_user_data_root()`. |
| `.buddd/user/` exists but `settings.yaml` missing | `load()` on user-project store returns success with empty default node. |
| Multiple consecutive `set()` calls on same key | Last value stored. `is_dirty()` stays `true` after first change. |
| `set()` with key identical to current value | No-op: `is_dirty()` unchanged (see AC-023). |
| Connection destroyed during observer callback | Copy-before-call pattern prevents iterator invalidation. |
| `std::unique_ptr<Connection>` dropped without explicit reset | Destructor calls cleanup lambda, unregisters observer. |
| Editor settings path's parent directory does not exist | `save()` creates parent directories. |
| Project settings file is read-only when `save()` called | `save()` returns `IoFailed`, editor continues (logged warn). |
| `Editor::setup()` called multiple times | Second call constructs a new `SettingsManager` (replacing old one). Stores are reloaded. |
| `Editor::shutdown()` without preceding `setup()` | `settings_manager_` is nullptr — no-op. |
| Dot-path with empty segments `"foo..bar"` | Skip empty segments, treat as `"foo.bar"`. |
| YAML file with trailing content after valid YAML | `YAML::LoadFile` parses first document only (single-document mode). Trailing content ignored. |
| Observer callback throws exception | Exception must NOT propagate out of `set()`. Wrap observer invocations in try-catch, log warning. |
| TypeRegistry decode failure for a registered type (e.g., YAML node has wrong shape) | `get<T>()` logs a warning and returns the provided default (does not crash). |
| TypeRegistry not yet initialized (no built-in types registered) | `TypeRegistry::yaml_encode<T>()` / `yaml_decode<T>()` returns `InvalidArgument` error. Store logs warning and returns default / no-op. |

## Security impact

- Settings files are read/written only within the OS user config directory and the CWD.
- No network access, no credentials stored.
- No explicit `chmod` — OS default file permissions apply.
- yaml-cpp and TypeRegistry handle malformed input without crashing (wrapped in try-catch).

## Data and migration impact

- **No INI migration**: There is no migration of `buddd_editor.ini` to `.buddd/user/layout.ini`. The old INI file is left in place. The new layout path is set directly to the new location.
- **New directory**: `.buddd/user/` is created in the CWD on first `load_all()` call.
- **New files**: `editor.yaml` created at OS config dir on first save (auto-created when any editor setting is set and saved).
- **No data loss**: No existing data is moved or deleted.
- **No schema migration**: MVP1 does not handle versioning between different settings file format versions.

## API compatibility impact

- **New public API**: `SettingsStore`, `SettingsManager`, `Connection`, `os_user_config_dir()`, `editor_data_root()`, `editor_user_data_root()` in namespace `buddd::engine`.
- **New Editor API**: `Editor::settings_manager()` accessor.
- **No deprecations**: No existing API is deprecated or removed.
- **No breaking changes**: All existing public APIs (`Editor`, `EngineService`, `EngineContext`, etc.) remain unchanged.

## Documentation impact

- README: No changes needed. The settings system is an internal infrastructure component.
- Wiki pages: The wiki-agent should create/update `docs/wiki/architecture/settings-system.md` documenting the architecture, path resolution, and API reference after implementation.
- Other specs: No changes to existing specs.

## ADR impact

No new ADR needed. The implementation is fully constrained by existing ADRs:
- ADR-016 (yaml-cpp PRIVATE dependency) — follows the same pattern via pimpl in `settings_store.h` + TypeRegistry include in `.cpp` only.
- ADR-019 (architecture boundaries) — settings module sits in `src/engine/`, no platform/graphics headers.
- ADR-026 (ImGui integration) — only touch point is setting `IniFilename`.
- ADR-001 (Result/Error) — all error handling uses existing `Error` categories.

## Done criteria

- [ ] `src/engine/util/os_config_dir.h` declares `os_user_config_dir()` in `buddd::engine` namespace. Only includes `<cstdlib>` and `<filesystem>`.
- [ ] `src/engine/util/os_config_dir.cpp` implements platform-specific path resolution per spec (Linux/XDG, macOS, Windows) per the implementation in `Required implementation behavior`.
- [ ] `src/engine/util/editor_data_root.h` declares `editor_data_root()` and `editor_user_data_root()` in `buddd::engine` namespace.
- [ ] `src/engine/util/editor_data_root.cpp` implements both functions per the implementation in `Required implementation behavior`.
- [ ] `src/engine/settings/settings_store.h` created with exact public API signatures as specified: forward-declared `YAML::Node` (no yaml-cpp include), includes `serialization_context.h` (no yaml-cpp, no platform headers), `std::unique_ptr<YAML::Node> root_` opaque pointer member, `~SettingsStore()` declared (not defaulted, defined in `.cpp`), template `get<T>()` and `set<T>()` declared in header but defined in `.cpp` with explicit instantiations for `bool`, `int32_t`, `float`, `double`, `std::string`. `observe()` returns `std::unique_ptr<Connection>`. No include of `type_registry.h` in header.
- [ ] `src/engine/settings/settings_store.cpp` includes `type_registry.h` and `serialization_context.h` (yaml-cpp and TypeRegistry only in `.cpp`). Implements `~SettingsStore()`, `load()`, `save()`, `get<T>()`, `set<T>()`, observer storage, Connection cleanup, with yaml-cpp exception-safe wrappers per ADR-016. Uses `TypeRegistry::yaml_decode<T>()` / `TypeRegistry::yaml_encode<T>()` for all type conversions.
- [ ] `src/engine/settings/settings_manager.h` created with exact public API signatures: no `migrate_ini()`, no `ini_path_storage()`, `layout_ini_path()` returns `const std::string&`. Constructor takes `SerializationContext` (includes `serialization_context.h`). Three `std::unique_ptr<SettingsStore>` members. Persistent `std::string ini_path_` member.
- [ ] `src/engine/settings/settings_manager.cpp` implements `load_all()`, `save_all()`, path resolution using `os_user_config_dir()`, `editor_data_root()`, and `editor_user_data_root()` per the table in `Required implementation behavior`. `layout_ini_path()` returns `ini_path_` as `const std::string&`.
- [ ] `src/engine/util/` and `src/engine/settings/` headers include NO `<SDL3/`, `<GL/`, `<glad/>`, `<imgui.h>`, `<yaml-cpp/yaml.h>`, or any `render/` headers. `settings_store.h` does NOT include `type_registry.h`. Only `.cpp` files may include yaml-cpp or TypeRegistry headers.
- [ ] `src/editor/editor.h` updated: `#include "settings/settings_manager.h"`, `std::unique_ptr<buddd::engine::SettingsManager> settings_manager_` member, `auto settings_manager() -> buddd::engine::SettingsManager&;` accessor.
- [ ] `src/editor/editor.cpp` updated:
  - Old INI block (lines 74-95: `ifstream` validation, `has_docking`/`has_windows` check, `std::remove`, hardcoded `"buddd_editor.ini"`) **removed entirely** and replaced with SettingsManager construction with `SerializationContext`, `layout_ini_path()`, and `load_all()`. NO `migrate_ini()` call.
  - `Editor::shutdown()` calls `settings_manager_->save_all()` with warn logging, before `initialized_ = false;`.
  - `settings_manager()` accessor implemented.
- [ ] `tests/engine/settings_store_tests.cpp` covers all AC-001 through AC-032 with headless-compatible tests, including TypeRegistry type tests (bool, int32_t, float, double, std::string), unregistered type warning test, `editor_data_root()`/`editor_user_data_root()` tests, SerializationContext pass-through test.
- [ ] `tests/editor/settings_integration_tests.cpp` covers AC-020 and AC-021 guarded with `#ifdef BUDDD_HAS_DISPLAY`, using offscreen SDL3 driver. No INI migration tests.
- [ ] All tests pass: `cmake --build build && ctest --test-dir build --output-on-failure`.
- [ ] `src/engine/error.h` is NOT modified (no new error categories).
- [ ] `src/engine/CMakeLists.txt`, `src/editor/CMakeLists.txt`, `tests/CMakeLists.txt` are NOT modified.
- [ ] No concrete settings keys (theme, shortcuts, project name, etc.) are defined or used anywhere.
- [ ] `set()` uses copy-before-call pattern for observer invocation to handle self-unregistration during observer callback.
- [ ] `Connection` uses `std::unique_ptr` ownership so that destroying the handle cleanly unregisters the observer.
