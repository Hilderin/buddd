# Implementation Review — Asset Manager (Final Review)

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **BLOCKING-1: `BUDDD_HAS_DISPLAY` not defined for `buddd_engine` library — demo segfaults**

  **RESOLVED**: `src/engine/CMakeLists.txt` now propagates `BUDDD_HAS_DISPLAY` to the `buddd_engine` library target.

- [x] **BLOCKING-2: Demo binary crashes — visual verification impossible**

  **RESOLVED**: Both `asset-demo` and `hot-reload` demos run successfully without crashing.

- [x] **BLOCKING-3: Hot-reload handlers are stubs (no actual reload)**

  **RESOLVED**: `handle_yaml_change()` and `handle_source_change()` in `asset_manager.cpp` are now fully implemented:
  - Texture YAML change: reloads image, creates new GPU texture, swaps GL handles in-place.
  - Material YAML change: re-parses YAML, re-resolves texture references, re-applies constants. Shader path changes cannot be applied in-place (V1 limitation, documented).
  - Texture source change: reloads image, creates new GPU texture, swaps GL handle in-place via `replace_gl_handle`/`release_gl_handle`.
  - Shader source change: re-reads shader sources, recompiles shader program, replaces GL handle on existing `ShaderProgram` in-place via `replace_handle`/`release_handle`.

- [x] **BLOCKING-4: AssetDemoApp creates AssetManager as local in setup() (no hot-reload possible)**

  **RESOLVED**: `AssetDemoApp` now stores `asset_manager_` as a class member and overrides `on_frame_begin()` to call `asset_manager_->poll_file_events()`.

- [x] **BLOCKING-5: InotifyFileWatcher only monitors top-level directory (no recursive subdirectory monitoring)**

  **RESOLVED**: `InotifyFileWatcher` now recursively watches all subdirectories via `add_watch_recursive()`. Watch descriptors tracked in `watch_dirs_` map. Event paths constructed relative to the watch base, matching the dependency map format.

- [x] **BLOCKING-6: Path format mismatch between FileWatcher and dependency map**

  **RESOLVED**: `resolve_path()` now uses `std::filesystem::lexically_relative` to convert YAML source paths to paths relative to `base_path_`, matching FileWatcher event format. `make_full_path()` helper reconstructs absolute paths for file I/O. Dependency map stores relative paths throughout.

## Warnings

Non-blocking concerns for awareness:

- **`src/cmd/app.h` and `src/cmd/app.cpp` were modified despite being forbidden by the implementation contract**: The contract's `Files forbidden to change` section listed `src/cmd/app.h` and `src/cmd/app.cpp` as forbidden. These were modified to add `virtual on_frame_begin() -> void {}` and to call it in `run_app()`. This was a necessary architectural change to enable hot-reload polling — the human explicitly requested this fix. The contract divergence is intentional and acceptable.

- **`demo_command.h/.cpp` deleted per human request**: The contract specified DemoCommand files (DC-21), but the human requested removal in favor of registering apps directly as scenes under `buddd run`. Already noted in previous review.

- **`BUDDD_TESTING` always defined for engine library**: The engine library has `target_compile_definitions(buddd_engine PRIVATE BUDDD_TESTING)`, which enables test-only accessors in release builds. Pre-existing condition.

- **`testing_inject_file_event()` duplicates `poll_file_events()` logic**: Both methods implement an almost identical loop that iterates dependents and dispatches to handler functions. The test-only accessor should ideally reuse `poll_file_events()` by injecting directly into the FileWatcher's queue rather than duplicating the dispatch logic.

- **Hot-reload of shader source files on material YAML change is a V1 limitation**: When a material's YAML changes and specifies different shader paths, the new shader cannot be applied to an existing Material object. The handler logs a warning and skips shader recompilation. Texture bindings and constants are still updated.

- **Duplicate inotify events may trigger redundant hot-reload processing**: File copy operations (e.g., the hot-reload demo's texture swap) may generate multiple inotify events. The system processes each independently, which is harmless but does redundant work.

- **Untracked prototype files**: `assets/textures/texture_prototype_gray.yaml`, `assets/textures/texture_prototype_red.yaml`, `assets/materials/demo_hot_reload.yaml` are untracked files not used by any test or demo. These are leftover artifacts with no functional impact.

## Required changes

- [x] **RC-1**: Add `BUDDD_HAS_DISPLAY` compile definition to `buddd_engine` in `src/engine/CMakeLists.txt`.
- [x] **RC-2**: Verify the demo runs without crashing.
- [x] **RC-3**: Capture and visually verify hot-reload (before: red, after: blue).
- [x] **RC-4**: Implement hot-reload handlers (was stub, now full implementation).
- [x] **RC-5**: Persist AssetManager as class member in AssetDemoApp.
- [x] **RC-6**: Recursive InotifyFileWatcher with proper relative paths.
- [x] **RC-7**: Fix path format mismatch between FileWatcher and dependency map.

## Suggested improvements

Optional ideas (not required):

- Refactor `testing_inject_file_event()` to push directly into the FileWatcher's event queue and reuse `poll_file_events()`, eliminating code duplication.
- Clean up untracked prototype asset files (`texture_prototype_*.yaml`, `demo_hot_reload.yaml`).
- Add `--help` output for the `hot-reload` scene in `main.cpp` usage text.
