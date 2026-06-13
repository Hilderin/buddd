# Settings System

The settings system provides a three-tier YAML-backed persistent settings infrastructure for the Buddd Editor. It is defined in [SPEC-036](/.specs/sprint-2026-06/settings-system/spec.md) and implemented in `src/engine/settings/`.

## Three tiers

| Tier | File path | Scope | VCS-tracked |
|---|---|---|---|
| **Editor settings** | `~/.config/buddd/editor.yaml` (Linux) / `~/Library/Application Support/buddd/editor.yaml` (macOS) / `%APPDATA%/buddd/editor.yaml` (Windows) | Global per-machine preferences (theme, keyboard shortcuts) | No |
| **Project settings** | `<cwd>/buddd.project.yaml` | Per-project settings shared with the team (project name, renderer config) | Yes |
| **User project settings** | `<cwd>/.buddd/user/settings.yaml` | Per-project per-user state (last paths, open tabs, panel layout overrides, **window geometry**) | No (gitignored) |

## YAML storage format

All settings files use YAML via yaml-cpp (PRIVATE dependency, ADR-016 compliant — yaml-cpp is never exposed in public headers thanks to the pimpl pattern in `SettingsStore`). Keys are accessed via dot-separated paths (e.g. `"renderer.resolution.width"`) and are stored as nested YAML mappings.

## File locations

- **Editor settings** path is resolved by `os_user_config_dir()` in `src/engine/util/os_config_dir.h` — a platform-dependent but stdlib-only utility that queries environment variables (`$XDG_CONFIG_HOME`, `$HOME`, `%APPDATA%`).
- **Project settings** path is `<cwd>/buddd.project.yaml` — never auto-created; only written when a project explicitly sets values.
- **User project settings** path is derived from `editor_user_data_root(cwd)` → `<cwd>/.buddd/user/settings.yaml`.
- **ImGui layout INI** is at `<cwd>/.buddd/user/layout.ini` (managed separately by Dear ImGui, path configured via `SettingsManager::layout_ini_path()`).

## TypeRegistry integration

`SettingsStore::get<T>()` and `SettingsStore::set<T>()` delegate to `TypeRegistry::yaml_decode<T>()` and `TypeRegistry::yaml_encode<T>()` for type conversion.

Explicit template instantiations exist for: `bool`, `int32_t`, `float`, `std::string`. Types already registered in TypeRegistry (`Vec3`, `Vec4`, `Quat`) are also supported implicitly. Unregistered types log a warning and return the default value.

## Observer pattern

`SettingsStore::observe(key, callback)` registers a change observer for a specific dot-separated key path:

```cpp
auto conn = store.observe("editor.theme", [](const std::string& key) {
    // react to change
});
```

- Callback fires when `set()` changes the key's value.
- Multiple observers on the same key are supported.
- Destroying the returned `std::unique_ptr<Connection>` unregisters the observer (RAII).
- `Connection` is a top-level class in namespace `buddd::engine` (move-only, non-copyable).
- Setting a key to its current value is a no-op — the store is not marked dirty and observers do not fire.

## Editor lifecycle

| Phase | Action |
|---|---|
| `Editor::setup()` | Constructs `SettingsManager` with CWD and `SerializationContext`, calls `load_all()`, sets `ImGui::GetIO().IniFilename` to `layout_ini_path()`. **After load**: reads window geometry from `user_project_settings`, validates, and applies to window (see First consumer below). |
| Runtime | Editor features read/write settings via `SettingsManager` accessors. `Editor::update()` caches the window's last known Normal position/size for later use during shutdown. |
| `Editor::shutdown()` | **Before save**: writes current window geometry (position, size, state) to `user_project_settings`. Then calls `save_all()` — dirty stores are flushed to disk, clean stores are skipped. |

`SettingsManager::save_all()` can also be called on-demand at any point to persist critical settings (crash safety).

## Key classes

- **`SettingsStore`** — `src/engine/settings/settings_store.h` — YAML-backed store with dot-path get/set, dirty tracking, observer registration.
- **`SettingsManager`** — `src/engine/settings/settings_manager.h` — orchestrator owning three `SettingsStore` instances; provides `load_all()`/`save_all()` lifecycle and path resolution.
- **`Connection`** — RAII handle returned by `observe()`; auto-unregisters on destruction.

## Path utilities

- **`os_user_config_dir()`** — `src/engine/util/os_config_dir.h` — returns OS-standard config directory.
- **`editor_data_root(project_root)`** — `src/engine/util/editor_data_root.h` — returns `<root>/.buddd/`.
- **`editor_user_data_root(project_root)`** — `src/engine/util/editor_data_root.h` — returns `<root>/.buddd/user/`.

## First consumer: Editor Window Geometry

[SPEC-037](/.specs/sprint-2026-06/editor-window-settings/spec.md) is the first concrete consumer of the settings infrastructure. It uses the `user_project_settings` tier to persist the editor window's position, size, and state (normal/maximized/minimized) across sessions.

Settings keys (all under `user_project_settings`):

| Key | Type | Default | Description |
|---|---|---|---|
| `editor.window.x` | `int32_t` | (unset) | Window left edge position in screen coordinates |
| `editor.window.y` | `int32_t` | (unset) | Window top edge position in screen coordinates |
| `editor.window.width` | `int32_t` | `1280` | Window inner width in pixels |
| `editor.window.height` | `int32_t` | `800` | Window inner height in pixels |
| `editor.window.state` | `std::string` | `"normal"` | Window state: `"normal"`, `"maximized"`, `"minimized"` |

**On startup** (`Editor::setup()` after `load_all()`): raw values are read from settings, validated (minimum 400×300 size, at-least-partially-visible position via `Platform::display_count()`/`display_bounds()`, minimized→normal force), and applied to the window via `Window::resize()`, `Window::set_position()`, and `Window::set_state()`.

**On shutdown** (`Editor::shutdown()` before `save_all()`): the current window position, size, and state are written to `user_project_settings` via the `editor.window.*` keys, then `save_all()` persists all dirty stores.

**Geometry tracking**: During `Editor::update()`, the Editor caches the window's last known Normal position and size. When the window is maximized or minimized, the cached Normal geometry is saved instead of the current maximized/minimized values, ensuring that restoring from a maximized session yields the user's intended un-maximised position and size.

### Associated API additions

- **`WindowState`** enum (`window.h`): `Normal`, `Maximized`, `Minimized`
- **`WindowPosition`** struct (`window.h`): `int x`, `int y`
- **`Window`** abstract class gains: `position()`, `set_position()`, `state()`, `set_state()`, `resize()`
- **`DisplayBounds`** struct (`platform.h`): `int x`, `int y`, `int width`, `int height`
- **`Platform`** abstract class gains: `display_count()`, `display_bounds()`
- **`window_utils.h/.cpp`**: `window_state_to_string()` / `parse_window_state()` conversion helpers

No SDL3 or platform-specific types are exposed to editor code — all platform interaction goes through the abstract `Window` and `Platform` interfaces (ADR-019).

## References

- [SPEC-036](/.specs/sprint-2026-06/settings-system/spec.md) — Full specification
- [Implementation Contract](/.specs/sprint-2026-06/settings-system/implementation-contract.md)
- [Code Review](/.specs/sprint-2026-06/settings-system/code-review.md)
- [SPEC-037](/.specs/sprint-2026-06/editor-window-settings/spec.md) — Editor Window Geometry Persistence
