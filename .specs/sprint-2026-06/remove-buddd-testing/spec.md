# SPEC-024 — Remove BUDDD_TESTING

## Problem

The `BUDDD_TESTING` preprocessor define is unconditionally set for the engine library in `src/engine/CMakeLists.txt:64`, meaning test-only code behind `#ifdef BUDDD_TESTING` is compiled into **all** build configurations (debug, release, CI, etc.). This creates three concrete problems:

1. **Test backdoor risk**: Methods like `testing_inject_file_event()`, which simulate file system events to trigger hot-reload, are compiled into production builds. While no production code calls them, they represent an unnecessary attack surface — any code within the library can trigger internal state mutations without going through the normal API.

2. **Bloat in public headers**: `#ifdef` guards in public headers (`asset_manager.h`, potentially `log/log.h` via `memory_sink.h`) create two distinct APIs depending on whether `BUDDD_TESTING` is defined. This is a maintenance burden and makes the public API shape non-deterministic.

3. **Hidden value**: The guarded code is genuinely useful — a memory-backed log sink, dependency map introspection, shader program cache inspection, and a file reload trigger are all features that should be first-class public API, not second-class test-only backdoors.

4. **Deceitful CMake**: The define is named `BUDDD_TESTING` but is never scoped to test builds. It is always-on, making the name misleading.

## Goals

- **Remove** the `BUDDD_TESTING` preprocessor define entirely from both `src/engine/CMakeLists.txt` and `tests/CMakeLists.txt`.
- **Promote** all code currently guarded by `#ifdef BUDDD_TESTING` to always-compiled, regular public API.
- **Rename** methods to match the project's public API naming conventions (no `testing_` or `get_` prefixes).
- **Replace** `testing_inject_file_event(const FileEvent&)` with a cleaner, self-contained `reload(std::string_view path)` method that does not expose the internal `FileEvent` struct.
- **Update** all tests to use the new API.
- **Update** documentation (wiki, ADRs) to reflect the removed define and the new method names.
- **Remove** `ShaderProgram::testing_handle()` entirely — tests use `handle()` directly, which returns the same value in all configurations.

## Non-goals

- **`FileEvent` struct**: Remains an internal type in `src/engine/asset/file_watcher.h`. It is not exposed through any new public method.
- **`DependencyMap` class**: Remains as-is. Only the accessor method is promoted; the class itself is unchanged.
- **Read-only historical specs**: No `.specs/` files from previous sprints are modified.
- **Adding new features**: This spec does not add new test coverage or new capabilities beyond promoting existing code to public API. Tests are only updated to use the renamed API.
- **Changing the logging system architecture**: `MemorySink` stays header-only; it only loses its `#ifdef BUDDD_TESTING` guard.

## Actors

| Actor | Description |
|---|---|
| **Engine developer** | Writes code that uses `AssetManager`. Gains stable public API methods for dependency map introspection, shader program cache inspection, and explicit asset reload. |
| **Test developer** | Writes tests for the engine. No longer needs `BUDDD_TESTING` to access formerly-guarded methods. Tests use the new public API (`reload(path)`, `dependency_map()`, `shader_programs()`). |
| **Maintainer** | Maintains CMake build files. Removes the misleading `BUDDD_TESTING` define. |
| **Build system** | CMake — removes two `target_compile_definitions` lines. No new dependencies. |

## API changes

### 1. AssetManager — `get_dependency_map()` → `dependency_map()`

**Before** (asset_manager.h, lines 61-66, inside `#ifdef BUDDD_TESTING`):
```cpp
#ifdef BUDDD_TESTING
    auto get_dependency_map() const -> const DependencyMap&;
#endif
```

**After** (always compiled, renamed):
```cpp
    /// Returns a const reference to the internal dependency map.
    /// Useful for diagnostics, tooling, crash reporting, and editor features.
    [[nodiscard]] auto dependency_map() const noexcept -> const DependencyMap&;
```

**Rationale**: The `get_` prefix is inconsistent with the rest of the codebase (cf. `base_path()`). Since this is becoming a first-class public method, the name is modernised. Also gains `noexcept` (it's a simple accessor returning a const ref to a member).

**Definition change** (asset_manager.cpp): The `#ifdef BUDDD_TESTING` guard is removed. The method body is unchanged. The `noexcept` specifier is added to the signature.

### 2. AssetManager — `testing_shader_programs()` → `shader_programs()`

**Before** (inside `#ifdef BUDDD_TESTING`):
```cpp
    auto testing_shader_programs() const noexcept
        -> const std::unordered_map<ShaderProgramKey, std::shared_ptr<ShaderProgram>>&;
```

**After** (always compiled, renamed):
```cpp
    /// Returns a const reference to the shader program deduplication map.
    /// Useful for cache introspection, diagnostics, and tooling.
    [[nodiscard]] auto shader_programs() const noexcept
        -> const std::unordered_map<ShaderProgramKey, std::shared_ptr<ShaderProgram>>&;
```

**Rationale**: Removes the `testing_` prefix, which falsely implied the method is only for tests. The method is a simple cached-state accessor, identical in nature to the dependency map accessor above.

### 3. AssetManager — `testing_inject_file_event(const FileEvent&)` → `reload(std::string_view)`

**Before** (inside `#ifdef BUDDD_TESTING`):
```cpp
    void testing_inject_file_event(const FileEvent& event);
```

**After** (always compiled, redesigned):
```cpp
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
```

**Rationale**:
- Eliminates exposure of the internal `FileEvent` struct from the public API.
- The method name `reload` is self-documenting — it triggers a reload based on a file path.
- The method delegates to a new private method `dispatch_file_event(path, type)` that encapsulates the extension-based routing logic. This eliminates the code duplication with `poll_file_events()`, which uses the same dispatch pattern (`.yaml` → `handle_yaml_change`, else → `handle_source_change`).
- The internal implementation replaces the old method's body (lines 132-146 of asset_manager.cpp). Where the old method contained the inline dispatch logic, the new method delegates to `dispatch_file_event()` — a single point of truth for the routing decision.

**Implementation sketch** (dispatch logic extracted to `dispatch_file_event`, see below):
```cpp
auto AssetManager::reload(std::string_view path) -> void {
    dispatch_file_event(std::string(path), FileEventType::Modified);
}
```

The new private method `dispatch_file_event()` (documented below) encapsulates the dependent lookup and extension-based routing — the same logic previously duplicated in both `poll_file_events()` and `testing_inject_file_event()`.

#### 3.1 AssetManager — new private `dispatch_file_event()` method

A new private method extracts the common dispatch logic that was previously duplicated between `poll_file_events()` and `testing_inject_file_event()` / `reload()`:

```cpp
    /// Dispatches a file change event to the appropriate internal handler
    /// based on the file extension. Routes `.yaml` files to `handle_yaml_change`
    /// and all other files to `handle_source_change`.
    ///
    /// Called internally by both:
    ///   - `poll_file_events()`   (with the real FileEventType from FileWatcher)
    ///   - `reload(path)`         (with FileEventType::Modified)
    auto dispatch_file_event(const std::string& path, FileEventType type) -> void;
```

**Properties**:

| Property | Value |
|---|---|
| Visibility | Private (not part of public API) |
| Signature | `auto dispatch_file_event(const std::string& path, FileEventType type) -> void` |
| Called by | `poll_file_events()` and `reload(path)` |
| Logic | Gets dependents from `dependency_map_`, then routes each dependent to `handle_yaml_change` (for `.yaml` paths) or `handle_source_change` (for all other extensions) |
| Rationale | Eliminates the code duplication that existed between `poll_file_events()` and the old `testing_inject_file_event()` — both had an identical `.yaml` extension check and dispatch to the same two handlers |

**Effect on existing code**:

- `poll_file_events()`: Where it previously had inline dispatch logic, it now calls `dispatch_file_event(event.path, event.type)` per event.
- `reload(path)`: Where the old `testing_inject_file_event()` had inline dispatch logic, it now calls `dispatch_file_event(path, FileEventType::Modified)`.
- **No functional change** — this is a pure internal refactoring. All acceptance criteria and edge cases covering `reload()` behaviour (AC-007 through AC-010, §Edge cases 1–3) remain valid and cover the same observable behaviour.

### 4. MemorySink — always compiled

**Before** (memory_sink.h, inside `#ifdef BUDDD_TESTING`):
```cpp
#ifdef BUDDD_TESTING

namespace buddd::log {

class MemorySink : public Sink {
public:
    void write(const LogMessage& message) override { messages_.push_back(message); }
    [[nodiscard]] auto messages() const -> const std::vector<LogMessage>& { return messages_; }
    void clear() { messages_.clear(); }
private:
    std::vector<LogMessage> messages_;
};

} // namespace buddd::log

#endif // BUDDD_TESTING
```

**After** (always compiled, no `#ifdef` guard):
```cpp
namespace buddd::log {

/// Memory sink that accumulates log messages in a vector for test assertions
/// or any use case requiring in-process log capture (diagnostics, tooling).
/// Not thread-safe.
class MemorySink : public Sink {
public:
    void write(const LogMessage& message) override { messages_.push_back(message); }
    [[nodiscard]] auto messages() const -> const std::vector<LogMessage>& { return messages_; }
    void clear() { messages_.clear(); }
private:
    std::vector<LogMessage> messages_;
};

} // namespace buddd::log
```

**Rationale**: MemorySink is a valid, generally-useful log sink. Any consumer might want to capture log messages in memory for in-process diagnostics, tooling, or editor crash log capture. Keeping it only compiled in test builds is unnecessarily restrictive.

### 5. ShaderProgram — remove `testing_handle()`

**Before**: `ShaderProgram` has three files with `testing_handle()`:
- `src/engine/render/shader_program.h:33` — `virtual auto testing_handle() const -> uint32_t;`
- `src/engine/render/shader_program.cpp:7` — default implementation returning `handle()`
- `src/engine/render/shader_program_headless.h:30` — headless override returning `static_cast<uint32_t>(generation_)`

In display mode, `testing_handle()` returns `handle()` (identical value). In headless mode, `handle_` is derived from `generation_` via `handle_ = static_cast<uint32_t>(generation_)`, so both methods return the same value. Both values change on recompilation, making `handle()` equally suitable for before/after comparisons in tests.

**After**: The `testing_handle()` method is completely removed:
- Declaration removed from `src/engine/render/shader_program.h`
- Default implementation removed from `src/engine/render/shader_program.cpp`
- Override removed from `src/engine/render/shader_program_headless.h`
- All test callers switch to `handle()` with identical behaviour.

**Rationale**: `testing_handle()` is redundant — it always returns the same value as `handle()` in every configuration (display, headless, before/after recompilation). Its `testing_` prefix is inconsistent with the public API direction established by this spec. Removing it is a natural cleanup while we are eliminating `testing_`-prefixed methods.

**Implementation sketch** (removals only, no replacements):
```diff
- // shader_program.h
- virtual auto testing_handle() const -> uint32_t;

- // shader_program.cpp
- auto ShaderProgram::testing_handle() const -> uint32_t { return handle(); }

- // shader_program_headless.h
- auto testing_handle() const -> uint32_t override { return static_cast<uint32_t>(generation_); }
```

## Build changes

### `src/engine/CMakeLists.txt`

Remove line 64:
```cmake
# Enable test-only accessors (BUDDD_TESTING guards)
target_compile_definitions(buddd_engine PRIVATE BUDDD_TESTING)
```

### `tests/CMakeLists.txt`

Remove line 25:
```cmake
# Enable BUDDD_TESTING for test-only accessors
target_compile_definitions(buddd_tests PRIVATE BUDDD_TESTING)
```

No other CMake changes. The `buddd_engine` library and `buddd_tests` executable continue to compile with no functional change (all previously-guarded code is now always compiled).

## Test changes

### Files with changes

| File | What changes | Reason |
|---|---|---|
| `tests/asset_manager_tests.cpp` | `testing_shader_programs()` → `shader_programs()`, `testing_inject_file_event({...})` → `reload(...)`, `get_dependency_map()` → `dependency_map()`, `testing_handle()` → `handle()` | Updated to match renamed/removed public API |
| `tests/model_asset_tests.cpp` | `testing_inject_file_event(event)` → `reload(path_string)` | Updated to match `reload()` API |
| `tests/logging_tests.cpp` | None (MemorySink API unchanged) | No rename, only the `#ifdef` guard is removed |
| `tests/assertion_tests.cpp` | None | Same as above |
| `tests/cmd_tests.cpp` | None | Same as above |
| `tests/scene_rendering_tests.cpp` | None | Same as above |
| `tests/log_helpers.h` | None | Uses `MemorySink` by name, which remains unchanged |

### Specific test changes

**`tests/asset_manager_tests.cpp`**:

| Line(s) | Before | After |
|---|---|---|
| ~312 | `assets->testing_shader_programs()` | `assets->shader_programs()` |
| ~450 | `assets->testing_inject_file_event({source_path, FileEventType::Modified})` | `assets->reload(source_path)` |
| ~474, 487 | `assets->testing_shader_programs()` | `assets->shader_programs()` |
| ~506 | `assets->get_dependency_map()` | `assets->dependency_map()` |
| ~532 | `assets->get_dependency_map()` | `assets->dependency_map()` |
| ~574, 587 | `assets->testing_shader_programs()` | `assets->shader_programs()` |
| ~583 | `assets->testing_inject_file_event({vert_path, FileEventType::Modified})` | `assets->reload(vert_path)` |
| ~611 | `assets->testing_inject_file_event({yaml_path, FileEventType::Modified})` | `assets->reload(yaml_path)` |
| ~643 | `assets->testing_inject_file_event({yaml_path, FileEventType::Modified})` | `assets->reload(yaml_path)` |

**`tests/model_asset_tests.cpp`**:

| Line(s) | Before | After |
|---|---|---|
| ~737-740 | `FileEvent event; event.path = "..."; event.type = ...; assets->testing_inject_file_event(event);` | `assets->reload("models/box/BoxTextured.gltf");` |

### `testing_handle()` → `handle()` changes (ShaderProgram)

**`tests/asset_manager_tests.cpp`**:

| Line(s) | Before | After |
|---|---|---|
| ~476 | `programs_before.begin()->second->testing_handle()` | `programs_before.begin()->second->handle()` |
| ~491 | `programs_after.begin()->second->testing_handle()` | `programs_after.begin()->second->handle()` |
| ~576 | `programs_before.begin()->second->testing_handle()` | `programs_before.begin()->second->handle()` |
| ~589 | `programs_after.begin()->second->testing_handle()` | `programs_after.begin()->second->handle()` |

## Documentation changes

The following documents must be updated:

| Document | Change |
|---|---|
| `docs/wiki/architecture/module-map.md` | Line 43: Remove `(test-only, \`#ifdef BUDDD_TESTING\`)` from `MemorySink` description. Line 52: Change "Guarded by `#ifdef BUDDD_TESTING`" to "Always compiled. ..." |
| `docs/wiki/domain/logging.md` | Line 233-235: Change "Compiled only in test builds (`#ifdef BUDDD_TESTING`)" to "Always compiled. Accumulates messages in a `std::vector<LogMessage>` ..." |
| `docs/wiki/domain/asset-manager.md` (or equivalent) | Document `dependency_map()`, `shader_programs()`, and `reload(path)` as public API methods. |
| `docs/adr/ADR-020-custom-logging-system.md` | Line 56: Update `MemorySink` description to remove "compiled only in test builds (`#ifdef BUDDD_TESTING`)". |
| `docs/adr/ADR-021-developer-assertions.md` | Lines 56, 161-163, 218: Update references to `BUDDD_TESTING` usage for `MemorySink` since it's no longer guarded by it. |
| **New ADR** | Create a new ADR (e.g., `ADR-022-remove-buddd-testing.md`) documenting the decision to remove `BUDDD_TESTING` and promote the guarded code to public API. |

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `BUDDD_TESTING` is not defined anywhere in `src/engine/CMakeLists.txt`. | Grep confirms no `BUDDD_TESTING` line in the file. |
| AC-002 | `BUDDD_TESTING` is not defined anywhere in `tests/CMakeLists.txt`. | Grep confirms no `BUDDD_TESTING` line in the file. |
| AC-003 | No source file in `src/engine/` contains `#ifdef BUDDD_TESTING` or `#ifndef BUDDD_TESTING`. | Grep on `src/engine/` returns zero matches for `BUDDD_TESTING`. |
| AC-004 | `memory_sink.h` has no `#ifdef BUDDD_TESTING` guard — the `MemorySink` class is always compiled. | Code review confirms the `#ifdef` / `#endif` lines are removed from `memory_sink.h`. |
| AC-005 | `AssetManager` has a public `dependency_map() const noexcept -> const DependencyMap&` method (not guarded by any `#ifdef`). | File compiles; method is declared outside any preprocessor conditional. |
| AC-006 | `AssetManager` has a public `shader_programs() const noexcept -> const std::unordered_map<ShaderProgramKey, std::shared_ptr<ShaderProgram>>&` method (not guarded). | File compiles; method is declared outside any preprocessor conditional. |
| AC-007 | `AssetManager` has a public `reload(std::string_view path) -> void` method (not guarded), replacing `testing_inject_file_event(const FileEvent&)`. | File compiles; old `testing_inject_file_event` declaration is removed; `reload` is present. |
| AC-008 | `AssetManager::reload(path)` with a `.yaml` path triggers `handle_yaml_change` for dependent assets. | Unit test: load asset, call `reload("textures/test_texture.yaml")`, verify no crash and asset is still valid (same test pattern as current Test 27). |
| AC-009 | `AssetManager::reload(path)` with a non-`.yaml` path (e.g., shader source) triggers `handle_source_change` for dependent assets. | Unit test: load material, call `reload("shaders/test.vert")`, verify shader program generation changes (same test pattern as current Test 26). |
| AC-010 | `AssetManager::reload(path)` with a path that has no dependents is a safe no-op. | Unit test: call `reload("nonexistent/file.txt")` — no crash, no exception. |
| AC-011 | Tests compile without `BUDDD_TESTING` defined and pass. | Full test suite passes (`ctest` / Catch2 test run). |
| AC-012 | `AssetManager::dependency_map()` returns a const reference to the internal `DependencyMap`. | Unit test: load a texture, call `dependency_map().get_dependencies(id)`, verify expected paths are present. |
| AC-013 | `AssetManager::shader_programs()` returns a const reference to the internal shader program cache. | Unit test: load two materials with same shaders, call `shader_programs()`, verify map size is 1 and the entry is non-null. |
| AC-014 | `ShaderProgram::testing_handle()` is removed from all three source files (`.h`, `.cpp`, `headless.h`). | Grep on `src/engine/render/` confirms no `testing_handle` declarations or definitions remain. |
| AC-015 | No test file calls `testing_handle()`. | Grep on `tests/` confirms zero matches for `testing_handle` in `.cpp` files. |
| AC-016 | Tests compile and pass after switching from `testing_handle()` to `handle()`. | Full test suite passes. |

## E2E Verification

This is a refactoring with no user-facing feature. Verification is through:

- **Full test suite**: `ctest` runs all existing tests (including updated asset_manager_tests and model_asset_tests).
- **Compile check**: Both debug and release builds compile without errors after removing `BUDDD_TESTING`.
- **Grep check**: Confirm no `BUDDD_TESTING` references remain in `src/engine/` or CMakeLists.txt files.

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | All acceptance criteria pass in CI (both debug and release builds). |
| SC-002 | Zero binary size change (the promoted code was already compiled into all builds via the always-on define). |
| SC-003 | Zero behaviour change for callers of unchanged API (poll_file_events, create<>, etc.). |
| SC-004 | `BUDDD_TESTING` appears nowhere in `src/engine/`, `tests/`, or CMakeLists.txt. |

## Edge cases

1. **`reload(path)` called before any assets are loaded**: The dependency map is empty, so `get_dependents()` returns an empty span. The method returns immediately with no side effects. Verified by AC-010.

2. **`reload(path)` called with a path that is a dependency of multiple assets**: All dependent assets are reloaded, matching the same behaviour as `poll_file_events()`. The method processes each dependent in sequence.

3. **`reload(path)` called during hot-reload via `poll_file_events()`**: Both methods call the same internal handlers (`handle_yaml_change`, `handle_source_change`) which are not re-entrant-safe. Calling `reload(path)` from within `poll_file_events()` is undefined behaviour (same as the existing restriction on calling `create<T>()` from within handlers).

4. **`MemorySink` used without initialising the Logger**: Same as before — `Sink::write()` is only called by the Logger after init. Constructing a `MemorySink` without init is safe (empty vector).

5. **`dependency_map()` called on a freshly constructed AssetManager**: Returns a const ref to an empty `DependencyMap`. Safe.

6. **`shader_programs()` called before any material is loaded**: Returns a const ref to an empty map. Safe.

## Error cases

| Scenario | Behaviour |
|---|---|
| `reload(path)` with an empty path string | `get_dependents("")` returns empty (no asset depends on empty path). No-op, no error. |
| `reload(path)` with a path whose dependent asset has been removed from cache | `handle_yaml_change` / `handle_source_change` check `cache_.find(asset_id)` and return early if not found. Safe no-op. |
| `reload(path)` with a binary/unknown extension (not `.yaml`, not a tracked shader/image/glTF) | Dispatched to `handle_source_change`, which checks `shader_programs_` for matching paths. If nothing matches, returns early with a log message. Safe no-op. |

## Permissions and security

- Removing `BUDDD_TESTING` eliminates the test-backdoor risk: `reload(path)` is now an explicit public API that can be audited and access-controlled just like any other method.
- `MemorySink` captures all log messages in process memory. No change from current behaviour for tests. In production use, the caller is responsible for not leaking sensitive information through log messages — this is a general logging concern, not specific to `MemorySink`.
- No new file system access, network access, or elevated permissions are introduced.

## Observability

- No new logging or metrics are introduced by this change.
- Promoted methods (`dependency_map()`, `shader_programs()`, `reload()`) make it easier to write diagnostics tools that introspect AssetManager state without test-only accessors.
- The `reload()` method is useful for automated tooling (e.g., an editor that triggers asset reload on save) and will produce the same log messages as a normal hot-reload event.

## Out of scope

(all items from Non-goals above, expanded for clarity)

| Item | Reason |
|---|---|
| Removing or renaming the `FileEvent` struct | Internal type; not exposed by public API. |
| Modifying `DependencyMap` class | Not part of the change; only the accessor is promoted. |
| Adding new test coverage | Out of scope; tests are only updated to match renamed API. |
| Modifying historical `.specs/` files | Read-only per workflow rules. |
| Changing `MemorySink` thread-safety behaviour | Stays documented as not thread-safe, same as before. |

## Assumptions

1. The `reload(std::string_view)` method uses exactly the same extension-based dispatch logic as the old `testing_inject_file_event(const FileEvent&)` — no new logic, no new edge cases.
2. The full test suite passing is sufficient verification of correctness. No separate manual QA is needed.
3. `noexcept` is appropriate for `dependency_map()` and `shader_programs()` because they are simple accessors returning const references to member variables. No allocation, no I/O, no throw.
4. The `MemorySink` class is cheap enough to compile into all builds (it is header-only, ~25 lines, no external dependencies beyond `<vector>` and `log/log.h`). Zero cost when not instantiated.
5. The old `testing_inject_file_event(const FileEvent&)` declaration and definition are completely removed (not merely deprecated), because no production code calls them and the new `reload(path)` is a clean replacement.

## Resolved clarifications

1. **`ShaderProgram::testing_handle()` naming**: Human decided to **remove it entirely**, not rename. Tests use `handle()` directly. (Resolved: 2026-06-06)

2. **`reload(path)` return type**: Human chose **`void`** (consistent with `poll_file_events()`), not `Result<void>`. (Resolved: 2026-06-06)
