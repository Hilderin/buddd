# SPEC-036 — Settings System (MVP1 Skeleton)

## Problem

The Buddd Editor currently has no persistent settings infrastructure. Editor preferences (theme, shortcuts) cannot survive restarts. Per-project per-user state (last open tabs, panel layout overrides) is lost between sessions. Shared project-level settings (project name, renderer configuration) have no place to live in the file system.

While ImGui persists its docking layout via `buddd_editor.ini`, this is a single flat file for a single purpose — it does not address the three distinct tiers of settings that the editor needs:

1. **Editor Settings** — global per-machine preferences (theme, keyboard shortcuts, general editor behaviour).
2. **User Project Settings** — per-project per-user state that must NOT be version-controlled (last paths, open tabs, panel layout overrides).
3. **Project Settings** — per-project settings shared with the team via VCS (project name, renderer settings).

Without this infrastructure, every editor feature that needs persistence must invent its own file format, path, and load/save lifecycle. This leads to inconsistent behaviour, scattered files, and technical debt.

## Goals

- Define a `SettingsStore` class in `src/engine/settings/` that can load, save, access, and observe YAML settings files.
- Define a `SettingsManager` orchestrator that owns one `SettingsStore` per tier and provides a unified API.
- Resolve correct OS-specific file paths for each tier (editor global, project-shared, project-per-user).
- Create the `.buddd/` directory tree in the project root on editor startup.
- Create centralized `editor_data_root()` / `editor_user_data_root()` path utilities in `src/engine/util/`.
- Integrate settings loading into `Editor::setup()` and saving into `Editor::shutdown()`.
- Provide a public API that future editor features can use to read/write settings values.
- Provide unit test coverage for all classes, file path resolution, YAML round-trips, and error cases.
- **MVP1 scope**: Skeleton only — no actual editor features consume settings yet. The infrastructure must exist, be tested, and integrate into the editor lifecycle, but no real settings keys are defined or used.

## Non-goals

- Defining or using any concrete editor settings (theme, shortcuts, etc.) — MVP1 builds only the infrastructure.
- Defining or using any concrete project settings (project name, renderer, etc.) — MVP1 builds only the infrastructure.
- Defining or using any concrete user project settings (last paths, open tabs, etc.) — MVP1 builds only the infrastructure.
- Settings hot-reload (watching files for changes at runtime) — deferred.
- Settings UI panels — deferred.
- Settings migration/versioning between file format versions — deferred.
- Multi-project or project-switching support — deferred.
- Network-synced settings — deferred.

## Actors

| Actor | Role |
|---|---|
| **Editor startup sequence** (`Editor::setup`) | Loads all three settings stores from disk. Creates `.buddd/` directory if missing. |
| **Editor shutdown sequence** (`Editor::shutdown`) | Saves all three settings stores to disk. |
| **Editor features** (future) | Read/write settings values through `SettingsManager`. Register change observers if needed. |
| **Tests** | Create `SettingsStore` and `SettingsManager` instances with temporary YAML files, verify load/save round-trips, path resolution, error handling. |

## User-visible behavior

- On first editor launch after MVP1, a `.buddd/` directory is created at the project root with the following structure:
  ```
  .buddd/
  └── user/
      └── layout.ini
  ```
- On first launch, `editor.yaml` is created at the OS config directory if absent (with empty/comment-only content).
- `buddd.project.yaml` is never created automatically — it is optional and only created when a user/project adds project settings.
- On editor shutdown, all dirty settings stores are saved to disk.
- No editor settings panel exists yet — the settings infrastructure is invisible to the user in MVP1.

## User stories

### Story 1 — Settings Infrastructure Loads at Startup (Priority: P1)

*As a developer, I want the settings system to initialise when the editor starts so that future features can access settings immediately.*

**Given** the editor is starting up
**When** `Editor::setup()` completes
**Then** all three `SettingsStore` instances are loaded (editor, project, user-project)
**And** `.buddd/user/` directory exists in the CWD
**And** `ImGui::GetIO().IniFilename` points to `.buddd/user/layout.ini`
**And** no error is raised if any settings file does not exist (defaults are used)

### Story 2 — Settings Save on Shutdown (Priority: P1)

*As a developer, I want settings to be persisted when the editor exits so that changes survive restarts.*

**Given** the editor has made changes to a `SettingsStore` (keys set or modified)
**When** `Editor::shutdown()` is called
**Then** each `SettingsStore` that has been modified (dirty) writes its YAML file to disk
**And** clean (unmodified) stores are not written

### Story 3 — On-Demand Save (Priority: P2)

*As a developer, I want to trigger a settings save at any point so that critical settings are not lost on crash.*

**Given** a `SettingsManager` instance
**When** `save_all()` is called explicitly
**Then** each dirty `SettingsStore` writes its YAML file to disk immediately
**And** clean stores are not written

### Story 4 — Observing Settings Changes (Priority: P2)

*As a developer, I want to register a callback that fires when a specific key changes so that I can react to settings updates in real time.*

**Given** a `SettingsStore` with a registered observer for key `"editor.theme"`
**When** that key's value is changed via `set()`
**Then** the observer callback is invoked with the key name and new value
**And** observers for other keys are not invoked

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|---|
| AC-001 | `SettingsStore` can be constructed with a file path and an empty YAML node (default content). | Unit test: construct and verify `load()` succeeds, `get()` returns default for unknown key. |
| AC-002 | `SettingsStore::load()` reads a valid YAML file from disk and makes its keys accessible. | Unit test: write a YAML file with known keys, `load()`, then `get()` returns correct values. |
| AC-003 | `SettingsStore::load()` returns `IoFailed` if the file path is not readable (permission denied, directory). | Unit test: load from a directory path or non-existent file returns error. |
| AC-004 | `SettingsStore::load()` succeeds with an empty/default node if the file does not exist (store stays in default state). | Unit test: load from non-existent path returns success, all `get()` calls return defaults. |
| AC-005 | `SettingsStore::load()` returns `InvalidFormat` if the file contains malformed YAML. | Unit test: write garbage content, load returns error with `InvalidFormat` category. |
| AC-006 | `SettingsStore::save()` writes the current YAML node to disk. | Unit test: set keys, save, read back file content, parse YAML, verify values match. |
| AC-007 | `SettingsStore::save()` returns `IoFailed` if the file cannot be written (readonly directory, invalid path). | Unit test: save to a non-writable path returns error. |
| AC-008 | `SettingsStore::save()` is a no-op if the store is not dirty (no changes since last load/save). | Unit test: load, save, verify file modification time does not change (or file not touched). |
| AC-009 | `SettingsStore` tracks dirty state: `is_dirty()` returns `true` after `set()`, `false` after `save()`. | Unit test: verify state transitions. |
| AC-010 | `SettingsStore::set(key, value)` stores a value via TypeRegistry and marks the store dirty. | Unit test: set string, int, bool, float values via TypeRegistry, get them back, verify dirty flag. |
| AC-011 | `SettingsStore::get(key, default)` returns the stored value or the provided default if the key does not exist. | Unit test: get existing keys returns stored value; get missing keys returns default. |
| AC-012 | `SettingsStore::observe(key, callback)` fires the callback when `set()` changes that specific key. | Unit test: register observer, set key, verify callback invoked; set different key, verify not invoked. |
| AC-013 | `SettingsStore::observe()` supports multiple observers for the same key. | Unit test: register two callbacks, set key, both fire. |
| AC-014 | `SettingsManager` is constructed with a `SerializationContext` and CWD, and owns three `SettingsStore` instances. | Unit test: construct with temp dir and context, verify three stores exist with correct paths. |
| AC-015 | `SettingsManager` resolves editor settings path to OS config dir + `buddd/editor.yaml`. | Unit test: on Linux, path contains `.config/buddd/editor.yaml`; on macOS, `Library/Application Support/buddd/editor.yaml`; on Windows, `APPDATA/buddd/editor.yaml`. |
| AC-016 | `SettingsManager` resolves project settings path to `<cwd>/buddd.project.yaml`. | Unit test: given CWD `/tmp/testproj`, path is `/tmp/testproj/buddd.project.yaml`. |
| AC-017 | `SettingsManager` resolves user project settings path derived from `editor_user_data_root(cwd)` to `<cwd>/.buddd/user/settings.yaml`. | Unit test: given CWD `/tmp/testproj`, path is `/tmp/testproj/.buddd/user/settings.yaml`. |
| AC-018 | `SettingsManager::load_all()` loads all three stores, creating `.buddd/user/` if absent. | Unit test: call `load_all()` in a temp dir, verify `.buddd/user/` directory now exists. |
| AC-019 | `SettingsManager::save_all()` saves all dirty stores; clean stores are not written. | Unit test: set a key on each store, `save_all()`, read each file, verify dirty stores are written. AC-008 covers the per-store no-op behavior for clean stores — combined with AC-019 this implicitly covers `save_all()` for clean stores. |
| AC-020 | `Editor::setup()` calls `SettingsManager::load_all()`. | Integration test (requires `BUDDD_HAS_DISPLAY=ON`): instantiate Editor with a display, verify files are created. |
| AC-021 | `Editor::shutdown()` calls `SettingsManager::save_all()`. | Integration test (requires `BUDDD_HAS_DISPLAY=ON`): modify settings via `SettingsManager`, call `Editor::shutdown()`, verify files on disk. |
| AC-022 | `SettingsStore` does NOT include any platform/graphics headers (ADR-019 compliance). | Code review: `src/engine/settings/` headers must not include SDL3, OpenGL, GLM, or any `render/` headers. |
| AC-023 | Setting a key to its current value does not mark the store as dirty. | Unit test: set key, save, set same key again, `is_dirty()` returns `false`. |
| AC-024 | `SettingsStore` supports nested YAML keys via dot-separated paths (e.g. `"renderer.resolution.width"`). | Unit test: set nested key, save, read YAML file, verify nested structure; load back, get nested key returns correct value. |
| AC-025 | Destroying a `Connection` object unregisters its observer — subsequent `set()` calls on the observed key no longer trigger the callback. | Unit test: register observer, set key (callback fires), destroy `Connection`, set key again (callback does not fire). |
| AC-026 | `editor_data_root()` returns `<cwd>/.buddd/` for any given project root path. | Unit test: pass temp path, verify returned path ends with `/.buddd/`. |
| AC-027 | `editor_user_data_root()` returns `<cwd>/.buddd/user/` for any given project root path. | Unit test: pass temp path, verify returned path ends with `/.buddd/user/`. |
| AC-028 | `SettingsStore::get<bool>()` reads a YAML boolean value correctly via TypeRegistry. | Unit test: set bool `true`, save, reload, `get<bool>()` returns `true`. |
| AC-029 | `SettingsStore::get<int32_t>()` reads a YAML integer value correctly via TypeRegistry. | Unit test: set int `42`, save, reload, `get<int32_t>()` returns `42`. |
| AC-030 | `SettingsStore::get<float>()` reads a YAML float value correctly via TypeRegistry. | Unit test: set float `3.14f`, save, reload, `get<float>()` returns `3.14f`. |
| AC-031 | `SettingsStore::get<std::string>()` reads a YAML string value correctly via TypeRegistry. | Unit test: set string `"hello"`, save, reload, `get<std::string>()` returns `"hello"`. |
| AC-032 | `SettingsStore::set/get` with an unregistered type (e.g. `uint64_t`) logs a warning and returns default / is no-op. | Unit test: call `get<uint64_t>()` on existing key, verify default is returned and a warning is logged; call `set<uint64_t>()`, verify no-op. |

## E2E Verification

- **Method**: Editor integration test (similar to `tests/editor/editor_tests.cpp`) that:
  1. Creates a temporary project directory.
  2. Constructs an `EngineService` with a display and an `Editor`.
  3. Calls `Editor::setup()` (which loads settings).
  4. Verifies `.buddd/user/` exists, `layout.ini` path is set, settings files are loadable.
  5. Modifies a setting via the `SettingsManager` (accessed through a public accessor).
  6. Calls `Editor::shutdown()`.
  7. Verifies modified settings file was written to disk.
  8. Verifies contents of written file match expected YAML.

This test requires a display and runs only when `BUDDD_HAS_DISPLAY=ON`, following the same pattern as existing SDL3 backend tests. AC-020 and AC-021 are gated accordingly. The lower-level `SettingsStore` and `SettingsManager` unit tests (AC-001 through AC-019) plus AC-026 through AC-032 remain headless-compatible.

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | All six `SettingsStore` operation types (construct, load, save, get, set, observe) have at least one unit test. |
| SC-002 | Path resolution is tested on all three platforms via `#ifdef`-guarded unit tests. |
| SC-003 | `SettingsManager` load and save round-trip correctly: no data loss when loading a file, modifying values, and saving. |
| SC-004 | Editor lifecycle integration (setup loads, shutdown saves) passes the E2E test. |
| SC-005 | All error categories (`IoFailed`, `InvalidFormat`) produce testable, non-crashing results. |

## Edge cases

| Case | Expected behaviour |
|---|---|
| Settings file is empty (zero bytes). | `load()` succeeds, store contains empty YAML node, `get()` returns defaults. |
| Settings file contains only comments. | Same as empty. |
| Settings file has a key with a YAML type that the reader does not understand (e.g., a binary blob). | The YAML is parsed by yaml-cpp but the specific key returns an error or null when accessed as an unsupported type. Store remains usable for other keys. |
| Project CWD contains no `.buddd/` directory. | `load_all()` creates `.buddd/user/` recursively (mkdir -p style). |
| `.buddd/user/` exists but `settings.yaml` is missing. | `load()` returns default node, no error. |
| Multiple consecutive `set()` calls on the same key. | Only the last value is stored. `is_dirty()` remains `true` after first change (no flip-flopping). |
| `set()` with a key identical to current value. | No-op: `is_dirty()` does not change (store is not marked dirty — see AC-023). |
| Observer is unregistered (lifecycle). | When the `Connection` object is destroyed, the observer is automatically unregistered and no longer fires. |
| TypeRegistry decode failure for a registered type (e.g., YAML node is wrong shape/type). | `get<T>()` logs a warning and returns the provided default (does not crash). |
| Editor settings path's parent directory does not exist. | `save()` creates parent directories. |
| Project settings file (`buddd.project.yaml`) is read-only when `save()` is called. | `save()` returns `IoFailed`, editor continues. |
| `Editor::setup()` is called multiple times. | Settings stores are not reloaded on subsequent calls (idempotent after first setup). |
| `Editor::shutdown()` is called without preceding `setup()`. | No-op: stores are not initialised, no save attempted. |

## Error cases

| Error | Scenario | Behaviour |
|---|---|---|
| `IoFailed` | Cannot read settings file (permission, corrupt filesystem). | `load()` returns `Result<void>` with `IoFailed` error. Store stays in default state. |
| `IoFailed` | Cannot write settings file (permission, disk full). | `save()` returns `Result<void>` with `IoFailed` error. Editor logs a warning and continues. |
| `InvalidFormat` | Settings file contains malformed YAML. | `load()` returns `Result<void>` with `InvalidFormat` error. Store stays in default state. |
| Missing settings file | File does not exist. | `load()` succeeds: store uses default (empty YAML node). Not an error. |
| `InitFailed` | Settings directory cannot be created (permission on `.buddd/`). | `SettingsManager::load_all()` returns error. Editor logs an error. This is non-fatal — editor can continue without persisting settings. |
| Resource exhaustion | Out of memory during YAML parse. | yaml-cpp throws or returns null. Caught and wrapped as `IoFailed` with descriptive message. |

## Permissions and security

- The settings system reads and writes files only within:
  - The OS-standard user config directory (editor settings).
  - The current working directory (project settings, user project settings, layout INI).
- No settings are transmitted over a network.
- No secrets or credentials are expected to be stored in settings (this is out of scope).
- File permissions are inherited from the OS default (no explicit `chmod` calls).
- YAML parsing uses yaml-cpp, which already handles malformed input without crashing.

## Observability

- All load/save operations are logged at `Info` level with file path, e.g.:
  - `"Settings: loading editor settings from /home/user/.config/buddd/editor.yaml"`
  - `"Settings: saved user project settings (3 keys)"`
- Write failures are logged at `Warn` level: `"Settings: failed to save project settings: <error>"`
- Malformed YAML is logged at `Warn` level: `"Settings: malformed YAML in <path>, using defaults"`
- TypeRegistry decode/skip warnings are logged at `Warn` level: `"Settings: type not registered for key <key>"`
- Directory creation is logged at `Debug` level.
- A log tag `"Settings"` is used for all settings-related messages.

## Key entities

### `SettingsStore` (in `src/engine/settings/`)

A generic YAML-backed settings store. Uses TypeRegistry for type conversion. yaml-cpp is confined to the `.cpp` file via a pimpl pattern (`std::unique_ptr<YAML::Node>`).

**Public API** (namespace `buddd::engine`, header `settings/settings_store.h`):

```cpp
namespace YAML { class Node; }

class SettingsStore {
public:
    /// Construct with a file path and a SerializationContext for TypeRegistry usage.
    /// Does NOT load the file.
    explicit SettingsStore(std::filesystem::path file_path, SerializationContext ctx);

    /// Destructor defined in .cpp for pimpl.
    ~SettingsStore();

    /// Load settings from the file. If file does not exist, uses default (empty) state.
    /// Returns error on malformed YAML or I/O errors.
    [[nodiscard]] auto load() -> Result<void>;

    /// Save current settings to file. No-op if not dirty.
    [[nodiscard]] auto save() -> Result<void>;

    /// Get a value by dot-separated key path. Uses TypeRegistry for type conversion.
    /// Returns default_value if key missing or TypeRegistry decode fails (logs warning).
    template<typename T>
    [[nodiscard]] auto get(const std::string& key, const T& default_value = T{}) const -> T;

    /// Set a value by dot-separated key path. Uses TypeRegistry for type conversion.
    /// Marks store dirty if value changed. No-op + warning if type not registered.
    template<typename T>
    auto set(const std::string& key, const T& value) -> void;

    /// Returns true if any key has been modified since last load/save.
    [[nodiscard]] auto is_dirty() const noexcept -> bool;

    /// Observer callback type.
    using ChangeCallback = std::function<void(const std::string& key)>;

    /// RAII handle that auto-unregisters an observer on destruction.
    class Connection {
    public:
        ~Connection();
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        Connection(Connection&&) noexcept;
        Connection& operator=(Connection&&) noexcept;
    private:
        friend class SettingsStore;
        Connection(SettingsStore* store, std::string key, std::uint64_t id);
        SettingsStore* store_;
        std::string key_;
        std::uint64_t id_;
    };

    /// Register an observer for a specific key. Fires when the key's value changes via set().
    /// Returns a Connection that auto-unregisters the observer on destruction.
    [[nodiscard]] auto observe(const std::string& key, ChangeCallback callback) -> std::unique_ptr<Connection>;

    // Non-copyable, movable.
    SettingsStore(const SettingsStore&) = delete;
    SettingsStore& operator=(const SettingsStore&) = delete;
    SettingsStore(SettingsStore&&) noexcept = default;
    SettingsStore& operator=(SettingsStore&&) noexcept = default;

private:
    std::filesystem::path file_path_;
    SerializationContext ctx_;
    std::unique_ptr<YAML::Node> root_;   // pimpl — yaml-cpp not exposed in header
    bool dirty_ = false;
    // Observer storage (map of key -> list of callbacks with connection IDs)
};
```

**TypeRegistry usage in `.cpp`:**
- `get<T>()`: finds YAML node by dot-path, calls `TypeRegistry::yaml_decode<T>(node, ctx_)`. On failure (type not registered or decode error): log warning, return `default_value`.
- `set<T>()`: calls `TypeRegistry::yaml_encode<T>(value, ctx_)`. On failure: log warning, no-op.
- Explicit template instantiations for: `bool`, `int32_t`, `float`, `double`, `std::string`. Vec3, Vec4, Quat available implicitly since they are already registered.

### `SettingsManager` (in `src/engine/settings/`)

Orchestrator that owns the three settings stores and handles path resolution.

**Public API** (namespace `buddd::engine`, header `settings/settings_manager.h`):

```cpp
class SettingsManager {
public:
    /// Construct with the project root directory (CWD at launch) and a SerializationContext.
    explicit SettingsManager(std::filesystem::path project_root, SerializationContext ctx);

    /// Load all three settings stores. Creates `.buddd/user/` if absent.
    [[nodiscard]] auto load_all() -> Result<void>;

    /// Save all dirty settings stores.
    [[nodiscard]] auto save_all() -> Result<void>;

    /// Access individual stores.
    [[nodiscard]] auto editor_settings() -> SettingsStore&;
    [[nodiscard]] auto project_settings() -> SettingsStore&;
    [[nodiscard]] auto user_project_settings() -> SettingsStore&;

    /// Returns the resolved path for the ImGui layout INI file.
    /// Returns a const reference to a persistent std::string member, ensuring
    /// the const char* obtained via .c_str() remains valid for SettingsManager lifetime.
    [[nodiscard]] auto layout_ini_path() const -> const std::string&;

private:
    std::filesystem::path project_root_;
    std::string ini_path_;    // persistent string backing layout_ini_path()
    std::unique_ptr<SettingsStore> editor_settings_;
    std::unique_ptr<SettingsStore> project_settings_;
    std::unique_ptr<SettingsStore> user_project_settings_;
};
```

### Path resolution rules

| Tier | OS | Path |
|---|---|---|
| Editor settings | Linux | `$XDG_CONFIG_HOME/buddd/editor.yaml` (falls back to `~/.config/buddd/editor.yaml`) |
| Editor settings | macOS | `~/Library/Application Support/buddd/editor.yaml` |
| Editor settings | Windows | `%APPDATA%/buddd/editor.yaml` |
| Project settings | All | `<cwd>/buddd.project.yaml` |
| User project settings | All | `<editor_user_data_root(cwd)>/settings.yaml` |
| Layout INI | All | `<editor_user_data_root(cwd)>/layout.ini` |

### `SettingsManager` integration with `Editor`

The `Editor` class gains a `std::unique_ptr<SettingsManager> settings_manager_` member.

Changes in `Editor::setup()` (after existing ImGui init):
1. Construct a `SerializationContext` from `engine_->assets()`.
2. Construct `SettingsManager` with CWD and the serialization context: `std::make_unique<SettingsManager>(std::filesystem::current_path(), ctx)`.
3. Call `settings_manager_->load_all()` (returns error if `.buddd/` cannot be created — logged, non-fatal).
4. Set `ImGui::GetIO().IniFilename = settings_manager_->layout_ini_path().c_str()`. The pointer is safe because `layout_ini_path()` returns a `const std::string&` to a persistent member in `SettingsManager`.

Changes in `Editor::shutdown()`:
1. Call `settings_manager_->save_all()` (returns error if write fails — logged non-fatally).

The `SettingsManager` is accessible via a public accessor for future features:
```cpp
[[nodiscard]] auto settings_manager() -> SettingsManager&;
```

For MVP1, this accessor exists but no editor features use it yet.

### Utility: OS config directory

A free function in `src/engine/util/` (header `util/os_config_dir.h`):
```cpp
namespace buddd::engine {
    /// Returns the OS-standard user config directory.
    /// Linux: $XDG_CONFIG_HOME or ~/.config
    /// macOS: ~/Library/Application Support
    /// Windows: %APPDATA%
    [[nodiscard]] auto os_user_config_dir() -> std::filesystem::path;
}
```

This is platform-dependent code but does NOT include any SDL3/OpenGL headers (solely uses `std::filesystem` and environment variable queries via `<cstdlib>` / `std::getenv`). It was moved from `src/engine/settings/` to `src/engine/util/` so it can be reused outside the settings module.

### Utility: Centralized editor_data_root

Two free functions in `src/engine/util/` (header `util/editor_data_root.h`) that are the single source of truth for the `.buddd/` directory structure:

```cpp
namespace buddd::engine {
    /// Returns <project_root>/.buddd/
    [[nodiscard]] auto editor_data_root(const std::filesystem::path& project_root) -> std::filesystem::path;

    /// Returns <project_root>/.buddd/user/
    [[nodiscard]] auto editor_user_data_root(const std::filesystem::path& project_root) -> std::filesystem::path;
}
```

`SettingsManager` uses `editor_user_data_root(project_root_)` to derive paths for `settings.yaml` and `layout.ini`. This eliminates path derivation duplication across settings and any future subsystems that need `.buddd/` paths.

### Error category

Settings parse errors reuse the existing `InvalidFormat` error category (no new category added).

## Out of scope

- Actual settings keys (themes, shortcuts, project name, renderer, last paths, open tabs) — MVP1 is infrastructure only.
- Settings panel in the editor UI.
- Settings hot-reload / file watcher.
- Settings migration between versions.
- Project-switching UI or multi-project support.
- `SettingsStore` for any tier beyond the three defined (e.g., system-wide, network-shared).
- Encryption or secure storage of any settings values.
- CLI flags for overriding settings paths.
- Per-platform settings for the ImGui INI path (it goes in `.buddd/user/` on all platforms).

## Existing documentation that must be updated

- `docs/adr/ADR-019-architecture-boundaries.md` — No update needed. The settings module sits in `src/engine/` and is already within the architecture boundary defined by ADR-019.
- `docs/adr/ADR-016` — Confirm yaml-cpp PRIVATE dependency compliance via pimpl pattern in `SettingsStore` (yaml-cpp not exposed in any header).
- `.specs/sprint-2026-06/editor-foundation/spec.md` — No direct changes needed, but future specs that add editor features consuming settings will reference this SPEC-036.
- No wiki pages currently document the settings system — after implementation, the wiki-agent should create/update relevant wiki pages.

## Assumptions

| Assumption | Rationale |
|---|---|
| yaml-cpp is available as a PRIVATE dependency of `buddd_engine`. | Already confirmed in engine CMakeLists.txt. |
| `std::filesystem` is available (C++17, which the project already uses). | Project already uses `<filesystem>` in editor.cpp and elsewhere. |
| The CWD at editor launch is the project root. | Documented in the human design decisions. |
| `buddd.project.yaml` is optional and never auto-created by the settings system. | Consistent with the human's statement that project without this file is still a project with defaults. |
| ImGui's INI file is treated as a separate concern from settings — it remains managed by ImGui, only its path is configured by `SettingsManager`. | Keeps the settings system clean and independent of ImGui. |
| The editor runs on a machine with a filesystem (not embedded or ROM environments). | The editor is a desktop application. |
| No concurrent access to settings files from multiple processes. | The editor is single-instance. |
| The `Editor` class owns the `SettingsManager` and its lifetime matches the `Editor` lifecycle. | Consistent with existing pattern (`EngineService`, `World`, etc. are owned by Editor or EditorApp). |

## Open questions

All open questions resolved in human validation (13-Jun-2026).
