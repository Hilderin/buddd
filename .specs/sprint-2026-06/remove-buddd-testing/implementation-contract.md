# IMPL-024 — Remove BUDDD_TESTING

## Source spec

`.specs/sprint-2026-06/remove-buddd-testing/spec.md`

## Goal

Remove the `BUDDD_TESTING` preprocessor define entirely from the engine build system. Promote all code currently guarded by `#ifdef BUDDD_TESTING` (`MemorySink`, test-only accessors on `AssetManager`, the old `testing_inject_file_event`) to always-compiled, regular public API. Rename methods to match project conventions (`get_dependency_map()` → `dependency_map()`, `testing_shader_programs()` → `shader_programs()`, `testing_inject_file_event()` → `reload(std::string_view)`). Extract the duplicated dispatch logic into a private `dispatch_file_event()` method. Remove `ShaderProgram::testing_handle()` entirely (tests switch to `handle()`). Update all tests to use the renamed API.

## Non-goals

- Do NOT modify the `FileEvent` struct or expose it through any public method.
- Do NOT modify the `DependencyMap` class (only the accessor method is promoted).
- Do NOT modify historical `.specs/` files from previous sprints.
- Do NOT add new test coverage or new capabilities beyond promoting existing code to public API.
- Do NOT change the logging system architecture (`MemorySink` stays header-only).
- Do NOT modify wiki files or ADR files — those are handled by the wiki-agent and governance-reviewer respectively.
- Do NOT create any new ADR — that is the governance-reviewer's responsibility.
- Do NOT modify `src/engine/asset/file_watcher.h` — `FileEventType` and `FileEvent` remain internal types.

## Relevant ADRs

| ADR | Constraint |
|-----|-----------|
| ADR-020 (custom logging system) | Line 56 documents MemorySink as `#ifdef BUDDD_TESTING`. This ADR will be updated by governance-reviewer; the implementation must not modify it. |
| ADR-021 (developer assertions) | Lines 56, 161-163, 218 reference `BUDDD_TESTING` as a define used for test-only infrastructure. The implementation must not change assertion behaviour — only the define is removed. Line 218 explicitly says `assert.h` MUST NOT use `BUDDD_TESTING`, which remains satisfied. |

## Files to inspect

Before editing, the Code Agent must read each file listed below to understand the existing code structure, exact line positions, and surrounding context.

| File | Purpose |
|------|---------|
| `src/engine/log/memory_sink.h` | Read the `#ifdef` guards (lines 7, 25) — they are identical to the spec's before block. |
| `src/engine/asset/asset_manager.h` | Read the `#ifdef BUDDD_TESTING` guard (lines 60-66) and the private section (lines 73+). Understand the `ShaderProgramKey` hash specialization, `DependencyMap`, and existing private methods to place `dispatch_file_event()` correctly. |
| `src/engine/asset/asset_manager.cpp` | Read the guarded section (lines 121-147) and `poll_file_events()` (lines 91-111). Understand the dispatch logic that will be extracted. |
| `src/engine/render/shader_program.h` | Read line 33 — `testing_handle()` declaration. |
| `src/engine/render/shader_program.cpp` | Read lines 7-9 — `testing_handle()` implementation. |
| `src/engine/render/shader_program_headless.h` | Read line 30 — `testing_handle()` override. |
| `src/engine/CMakeLists.txt` | Read line 64 — the `target_compile_definitions` line to remove. |
| `tests/CMakeLists.txt` | Read line 24-25 — the `target_compile_definitions` line to remove. |
| `tests/asset_manager_tests.cpp` | Read all `testing_shader_programs()`, `testing_inject_file_event()`, `get_dependency_map()`, and `testing_handle()` calls to understand the exact transformation. |
| `tests/model_asset_tests.cpp` | Read lines 737-740 — `FileEvent` construction + `testing_inject_file_event()` call. |
| `src/engine/asset/file_watcher.h` | Read to confirm `FileEventType` enum and `FileEvent` struct definitions (for `dispatch_file_event` signature). |

## Files allowed to change

Only the following files may be modified:

1. `src/engine/log/memory_sink.h`
2. `src/engine/asset/asset_manager.h`
3. `src/engine/asset/asset_manager.cpp`
4. `src/engine/render/shader_program.h`
5. `src/engine/render/shader_program.cpp`
6. `src/engine/render/shader_program_headless.h`
7. `src/engine/CMakeLists.txt`
8. `tests/CMakeLists.txt`
9. `tests/asset_manager_tests.cpp`
10. `tests/model_asset_tests.cpp`

## Files forbidden to change

The following files must NOT be modified (they are out of scope, handled by other agents, or must remain unchanged):

| File | Reason |
|------|--------|
| `src/engine/asset/file_watcher.h` | `FileEvent` and `FileEventType` remain internal. |
| `src/engine/log/log.h` | Only has a comment referencing `memory_sink.h` — no code changes needed. |
| Any `.specs/` file from previous sprints | Read-only per workflow rules. |
| Any `docs/adr/` file | Updated by governance-reviewer. |
| Any `docs/wiki/` file | Updated by wiki-agent. |
| `tests/logging_tests.cpp` | No changes needed (MemorySink API unchanged). |
| `tests/assertion_tests.cpp` | No changes needed. |
| `tests/cmd_tests.cpp` | No changes needed. |
| `tests/scene_rendering_tests.cpp` | No changes needed. |
| `tests/log_helpers.h` | No changes needed (uses `MemorySink` by name, which remains the same). |

## Existing conventions to follow

1. **`auto` return type syntax**: The codebase uses trailing return type: `auto method(args) -> ReturnType`.
2. **`noexcept` on accessors**: Simple const accessors returning references to members are `noexcept`.
3. **`[[nodiscard]]` on accessors**: Public methods returning values that should not be ignored use `[[nodiscard]]`.
4. **Public member ordering**: Public methods first, private methods and members follow. The methods being promoted (`dependency_map`, `shader_programs`, `reload`) should be placed in the public section right after `set_file_watcher_enabled`.
5. **Private method placement**: `dispatch_file_event` should be placed in the private section, near `handle_yaml_change` and `handle_source_change` (both private hot-reload handlers).
6. **Comment style**: Section headers use `// ============================================================================` lines. Method groups are documented with `// ============================================================================` heading blocks.
7. **Test naming**: Test cases use `TEST_CASE("Description", "[tag][headless]")` format.
8. **`void` return type**: `poll_file_events()` returns `void`. The new `reload()` also returns `void` to match.
9. **Include style**: Headers use `#include "asset/asset_manager.h"` style (relative to `src/engine/`).
10. **No `std::` qualifier for `buddd::engine` types**: Within `namespace buddd::engine`, types like `FileEventType` and `ShaderProgram` are unqualified.

## Required implementation behavior

### A. memory_sink.h — Remove `#ifdef` guard

Remove lines 7 and 25 (the `#ifdef BUDDD_TESTING` and `#endif // BUDDD_TESTING` lines). Remove line 12 (the comment `/// Only compiled when BUDDD_TESTING is defined.`) and its surrounding blank lines. Update the comments to reflect always-compiled status.

The resulting class must be always compiled, declared unconditionally in `namespace buddd::log`.

### B. asset_manager.h — Remove `#ifdef`, rename, add new methods

1. **Remove the `#ifdef` guard**: Delete lines 60-66 inclusive (the comment block starting at line 60, the `#ifdef`, the three method declarations, and the `#endif`).
2. **Add promoted public methods** in the public section, after `set_file_watcher_enabled` (after line 58):

```cpp
    /// Returns a const reference to the internal dependency map.
    /// Useful for diagnostics, tooling, crash reporting, and editor features.
    [[nodiscard]] auto dependency_map() const noexcept -> const DependencyMap&;

    /// Returns a const reference to the shader program deduplication map.
    /// Useful for cache introspection, diagnostics, and tooling.
    [[nodiscard]] auto shader_programs() const noexcept
        -> const std::unordered_map<ShaderProgramKey, std::shared_ptr<ShaderProgram>>&;

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

3. **Add `dispatch_file_event`** in the private section, after `handle_source_change` declaration (after the existing line 99). Place it in the "Hot-reload handlers" area:

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

The resulting public section must have methods in this order:
- `create` (static)
- `~AssetManager`
- `create<T>` (template)
- `create_texture`, `create_material`, `create_model`
- `clear`
- `base_path`
- `poll_file_events`
- `set_file_watcher_enabled`
- `dependency_map`
- `shader_programs`
- `reload`

### C. asset_manager.cpp — Remove `#ifdef`, rename, extract dispatch logic

1. **Remove the entire section from line 117 to line 147** (the `// ============================================================================` comment, `#ifdef BUDDD_TESTING`, the three old method implementations, and `#endif`).

2. **Rename `get_dependency_map` → `dependency_map`**: Add `noexcept` to the signature. Body unchanged.

```cpp
auto AssetManager::dependency_map() const noexcept -> const DependencyMap& {
    return dependency_map_;
}
```

3. **Rename `testing_shader_programs` → `shader_programs`**: Signature unchanged (already noexcept). Body unchanged.

```cpp
auto AssetManager::shader_programs() const noexcept
    -> const std::unordered_map<ShaderProgramKey, std::shared_ptr<ShaderProgram>>&
{
    return shader_programs_;
}
```

4. **Implement `reload`**: Takes `std::string_view path`, calls `dispatch_file_event(std::string(path), FileEventType::Modified)`.

5. **Implement `dispatch_file_event`**: Private method with the exact dispatch logic extracted from what was previously duplicated in both `poll_file_events` and `testing_inject_file_event`.

```cpp
auto AssetManager::dispatch_file_event(const std::string& path, FileEventType type) -> void {
    auto dependents = dependency_map_.get_dependents(path);
    if (dependents.empty()) return;

    std::vector<std::string> asset_ids(dependents.begin(), dependents.end());

    for (const auto& asset_id : asset_ids) {
        if (path.size() >= 5 && path.substr(path.size() - 5) == ".yaml") {
            handle_yaml_change(path, asset_id);
        } else {
            handle_source_change(path, asset_id);
        }
    }
}
```

6. **Refactor `poll_file_events()`** (lines 91-111): Replace the inline dispatch logic with a call to `dispatch_file_event(event.path, event.type)` for each event.

```cpp
auto AssetManager::poll_file_events() -> void {
    if (!file_watcher_ || !file_watcher_enabled_) return;

    auto events = file_watcher_->poll_events();
    for (const auto& event : events) {
        dispatch_file_event(event.path, event.type);
    }
}
```

7. **Update the section comment**: The section comment at line 117 (`// Test-only accessors`) should be removed or updated to reflect the promoted methods. Replace it with a comment indicating these are public API methods:

```cpp
// ============================================================================
// Public introspection and reload API
// ============================================================================
```

The overall order in asset_manager.cpp should be:
- (existing) Construction/Destruction
- (existing) Public API — up to `set_file_watcher_enabled`
- (existing) `poll_file_events` — refactored as above
- **NEW**: Public introspection and reload API section
  - `dependency_map()`
  - `shader_programs()`
  - `reload(std::string_view)`
- (existing) Path resolution and file I/O
- (existing) YAML parsing helper
- (existing) load_texture, load_material, load_model
- (existing) Hot-reload handlers
  - `dispatch_file_event` (new)
  - `handle_yaml_change` (existing, unchanged)
  - `handle_source_change` (existing, unchanged)
- (existing) Explicit instantiations

### D. shader_program.h — Remove `testing_handle()` declaration

Remove line 33 entirely:
```cpp
    /// Returns an opaque ID that changes on each successful recompilation.
    /// Default implementation returns handle().
    virtual auto testing_handle() const -> uint32_t;
```
This is lines 31-33 (comment + declaration). Remove all three.

### E. shader_program.cpp — Remove `testing_handle()` implementation

Remove lines 7-9 entirely:
```cpp
auto ShaderProgram::testing_handle() const -> uint32_t {
    return handle();
}
```

### F. shader_program_headless.h — Remove `testing_handle()` override

Remove line 30 entirely:
```cpp
    auto testing_handle() const -> uint32_t override { return static_cast<uint32_t>(generation_); }
```

### G. src/engine/CMakeLists.txt — Remove `BUDDD_TESTING` define

Remove lines 63-64:
```cmake
# Enable test-only accessors (BUDDD_TESTING guards)
target_compile_definitions(buddd_engine PRIVATE BUDDD_TESTING)
```

### H. tests/CMakeLists.txt — Remove `BUDDD_TESTING` define

Remove lines 24-25:
```cmake
# Enable BUDDD_TESTING for test-only accessors
target_compile_definitions(buddd_tests PRIVATE BUDDD_TESTING)
```

### I. tests/asset_manager_tests.cpp — Rename all API calls

Make the following exact replacements:

| Line(s) | Before | After | Notes |
|---------|--------|-------|-------|
| 311 (comment) | `// Verify deduplication via testing_shader_programs() accessor` | `// Verify deduplication via shader_programs() accessor` | Comment update |
| 312 | `assets->testing_shader_programs()` | `assets->shader_programs()` | |
| 450 | `assets->testing_inject_file_event({source_path, FileEventType::Modified});` | `assets->reload(source_path);` | Remove `FileEvent` aggregate init |
| 474 | `assets->testing_shader_programs()` | `assets->shader_programs()` | |
| 476 | `programs_before.begin()->second->testing_handle()` | `programs_before.begin()->second->handle()` | |
| 483 | `assets->testing_inject_file_event({shader_path, FileEventType::Modified});` | `assets->reload(shader_path);` | **Spec miss**: not in spec table but must be updated |
| 487 | `assets->testing_shader_programs()` | `assets->shader_programs()` | |
| 491 | `programs_after.begin()->second->testing_handle()` | `programs_after.begin()->second->handle()` | |
| 506 | `assets->get_dependency_map()` | `assets->dependency_map()` | |
| 532 | `assets->get_dependency_map()` | `assets->dependency_map()` | |
| 574 | `assets->testing_shader_programs()` | `assets->shader_programs()` | |
| 576 | `programs_before.begin()->second->testing_handle()` | `programs_before.begin()->second->handle()` | |
| 583 | `assets->testing_inject_file_event({vert_path, FileEventType::Modified});` | `assets->reload(vert_path);` | |
| 587 | `assets->testing_shader_programs()` | `assets->shader_programs()` | |
| 589 | `programs_after.begin()->second->testing_handle()` | `programs_after.begin()->second->handle()` | |
| 611 | `assets->testing_inject_file_event({yaml_path, FileEventType::Modified});` | `assets->reload(yaml_path);` | |
| 643 | `assets->testing_inject_file_event({yaml_path, FileEventType::Modified});` | `assets->reload(yaml_path);` | |

### J. tests/model_asset_tests.cpp — Replace `testing_inject_file_event`

Replace lines 737-740:

**Before**:
```cpp
    // Inject a synthetic FileEvent for the glTF source file
    // The dependency map tracks the source path (relative to base_path_)
    FileEvent event;
    event.path = "models/box/BoxTextured.gltf";
    event.type = FileEventType::Modified;
    assets->testing_inject_file_event(event);
```

**After**:
```cpp
    // Trigger a reload for the glTF source file
    assets->reload("models/box/BoxTextured.gltf");
```

### K. Changes to include directives (optional but allowed)

The implementer MAY remove `#include "asset/file_watcher.h"` from `tests/asset_manager_tests.cpp` and `tests/model_asset_tests.cpp` if it produces no other needed symbols after the changes. However, removing includes is not required — the build must succeed either way.

## Required tests

No new tests are required. The spec explicitly states test coverage is out of scope. Tests are only updated to match the renamed/removed API.

The implementer must verify that all existing tests pass after the changes:

### Unit tests
- All existing tests in `tests/asset_manager_tests.cpp` must compile and pass after the API renames.
- All existing tests in `tests/model_asset_tests.cpp` must compile and pass after the `reload()` replacement.
- All existing tests in `tests/logging_tests.cpp`, `tests/assertion_tests.cpp`, `tests/cmd_tests.cpp`, `tests/scene_rendering_tests.cpp` must continue to compile and pass (they were not modified but must still compile without `BUDDD_TESTING` defined).

### E2E / Integration verification
- Full test suite: `ctest` must report all tests passing (including the updated `asset_manager_tests` and `model_asset_tests`).
- Both debug and release builds must compile without errors after removing `BUDDD_TESTING`.
- Grep check: `grep -r 'BUDDD_TESTING' src/engine/ tests/ --include='*.h' --include='*.cpp' --include='*.txt' --include='*.cmake'` must return zero matches.

## Edge cases

The implementation must preserve the following behaviours (from spec Edge cases §1-6 and Error cases §1-3):

1. **`reload(path)` called before any assets are loaded**: The dependency map is empty, so `get_dependents()` returns an empty span. `dispatch_file_event` returns immediately with no side effects.
2. **`reload(path)` called with a path that is a dependency of multiple assets**: All dependent assets are reloaded, matching the same behaviour as `poll_file_events()`. The method processes each dependent in sequence.
3. **`reload(path)` called during hot-reload via `poll_file_events()`**: Both methods call the same internal handlers (`handle_yaml_change`, `handle_source_change`) which are not re-entrant-safe. Calling `reload(path)` from within `poll_file_events()` is undefined behaviour (same as the existing restriction).
4. **`MemorySink` used without initialising the Logger**: Same as before — `Sink::write()` is only called by the Logger after init. Constructing a `MemorySink` without init is safe (empty vector).
5. **`dependency_map()` called on a freshly constructed AssetManager**: Returns a const ref to an empty `DependencyMap`. Safe.
6. **`shader_programs()` called before any material is loaded**: Returns a const ref to an empty map. Safe.
7. **`reload(path)` with an empty path string**: `get_dependents("")` returns empty (no asset depends on empty path). No-op, no error.
8. **`reload(path)` with a path whose dependent asset has been removed from cache**: `handle_yaml_change` / `handle_source_change` check `cache_.find(asset_id)` and return early if not found. Safe no-op.
9. **`reload(path)` with a binary/unknown extension (not `.yaml`, not a tracked shader/image/glTF)**: Dispatched to `handle_source_change`, which checks `shader_programs_` for matching paths. If nothing matches, returns early with a log message. Safe no-op.
10. **`reload(path)` with an absolute path (non-dependency)**: `get_dependents()` returns empty (dependency map stores relative paths). Safe no-op. The test at line 483 (Test 22) exercises this case with `project_root() + "/tests/assets/shaders/compile_error.vert"`.
11. **`reload(path)` with a relative path that IS a dependency**: Must correctly dispatch to `handle_yaml_change` (for `.yaml` extension) or `handle_source_change` (for any other extension).

## Security impact

- Removing `BUDDD_TESTING` eliminates the test-backdoor risk: `reload(path)` is now an explicit public API that can be audited and access-controlled just like any other method.
- `MemorySink` captures all log messages in process memory. No change from current behaviour for tests. In production use, the caller is responsible for not leaking sensitive information through log messages — this is a general logging concern.
- No new file system access, network access, or elevated permissions are introduced.

## Data and migration impact

None. No schema changes, no data migrations, no seed data changes.

## API compatibility impact

The following API changes break backward compatibility:

| Old API | New API | Impact |
|---------|---------|--------|
| `asset_manager.get_dependency_map()` | `asset_manager.dependency_map()` | Callers must rename. Previously guarded — only test code calls this. |
| `asset_manager.testing_shader_programs()` | `asset_manager.shader_programs()` | Callers must rename. Previously guarded — only test code calls this. |
| `asset_manager.testing_inject_file_event(const FileEvent&)` | `asset_manager.reload(std::string_view)` | Callers must update both name and signature. The old method's `FileEvent` parameter is replaced by a plain path string. |
| `program->testing_handle()` | `program->handle()` | Callers must rename. Both return the same value in all configurations. |

These are all test-only API changes — no production callers are affected (no production code called these guarded methods).

## Documentation impact

- README: None
- Wiki pages: The wiki-agent handles updates to `docs/wiki/architecture/module-map.md`, `docs/wiki/domain/logging.md`, `docs/wiki/domain/assertions.md`, and `docs/wiki/domain/business-rules.md`. The implementation must NOT modify these files.
- ADRs: The governance-reviewer handles updates to `docs/adr/ADR-020-custom-logging-system.md`, `docs/adr/ADR-021-developer-assertions.md`, and creation of a new ADR documenting the removal. The implementation must NOT modify these files.
- Other specs: None

## ADR impact

This implementation does not create, modify, or deprecate any ADR. A new ADR documenting the removal decision will be created by the governance-reviewer.

## Done criteria

The implementation is complete when ALL of the following are verifiable:

- [ ] **File: `src/engine/log/memory_sink.h`**: No `#ifdef BUDDD_TESTING` or `#endif` lines remain. The `MemorySink` class is declared unconditionally (verified by reading the file).
- [ ] **File: `src/engine/asset/asset_manager.h`**: No `#ifdef BUDDD_TESTING` line exists. No `testing_shader_programs`, `testing_inject_file_event`, or `get_dependency_map` declarations exist. The following new declarations exist in the public section: `dependency_map()`, `shader_programs()`, `reload(std::string_view)`. The following private declaration exists: `dispatch_file_event(const std::string&, FileEventType)`. (Verified by reading the file.)
- [ ] **File: `src/engine/asset/asset_manager.cpp`**: No `#ifdef BUDDD_TESTING` or `#endif` lines remain. No `testing_shader_programs`, `testing_inject_file_event`, or `get_dependency_map` definitions exist. `poll_file_events()` calls `dispatch_file_event(event.path, event.type)` (no inline dispatch logic). `dependency_map()` has `noexcept` qualifier. (Verified by reading the file.)
- [ ] **File: `src/engine/render/shader_program.h`**: No `testing_handle` declaration exists (verified by grep on the file).
- [ ] **File: `src/engine/render/shader_program.cpp`**: No `testing_handle` definition exists (verified by grep on the file).
- [ ] **File: `src/engine/render/shader_program_headless.h`**: No `testing_handle` override exists (verified by grep on the file).
- [ ] **File: `src/engine/CMakeLists.txt`**: No `BUDDD_TESTING` string exists (verified by grep on the file).
- [ ] **File: `tests/CMakeLists.txt`**: No `BUDDD_TESTING` string exists (verified by grep on the file).
- [ ] **File: `tests/asset_manager_tests.cpp`**: No `testing_shader_programs()`, `testing_inject_file_event()`, `get_dependency_map()`, or `testing_handle()` calls remain. All replaced with `shader_programs()`, `reload()`, `dependency_map()`, or `handle()` respectively (verified by grep).
- [ ] **File: `tests/model_asset_tests.cpp`**: No `testing_inject_file_event()` call remains. Replaced with `reload("models/box/BoxTextured.gltf")` (verified by reading the file). No `FileEvent` struct usage remains in this file (the `FileEvent event; event.path = ...; event.type = ...` pattern is removed).
- [ ] **No `BUDDD_TESTING` references in engine source**: `grep -r 'BUDDD_TESTING' src/engine/ tests/ --include='*.h' --include='*.cpp' --include='*.cmake' --include='*.txt'` returns zero matches.
- [ ] **Full test suite passes**: `ctest` (or direct test runner) reports all tests passing. The full test suite must compile and pass without `BUDDD_TESTING` defined anywhere.
- [ ] **No `testing_handle` in tests**: `grep -n 'testing_handle' tests/ --include='*.cpp'` returns zero matches.
