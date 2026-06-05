# Implementation Contract Review — Asset Manager (IMPL-019)

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **BLOCKING-1: Texture hot-reload destroys the newly-swapped GPU handle**

  **Status**: RESOLVED ✓

  **Original issue**: `handle_source_change()` created a temporary `unique_ptr<Texture>`, swapped its GPU handle into the existing texture, then let the temporary go out of scope — which called `glDeleteTextures` on the handle that was just swapped in, producing a dangling handle.

  **Fix verified**: Added `virtual auto release_gl_handle() noexcept -> uint32_t` to the `Texture` base class (default returns 0, no-op for headless). `TextureOpenGL` override stores `texture_` in local, sets `texture_ = 0`, returns stored value. `TextureHeadless` inherits no-op default. The `handle_source_change()` flow for textures now correctly calls `replace_gl_handle()` first (swaps handle), then `release_gl_handle()` on the temporary (clears its internal handle), so the temporary's destructor does nothing.

- [x] **BLOCKING-2: Shader hot-reload does NOT propagate to existing Materials**

  **Status**: RESOLVED ✓

  **Original issue**: The contract used a "replace map entry" approach where a new `shared_ptr<ShaderProgram>` was inserted into `shader_programs_` — but existing Materials held a `shared_ptr` copy to the *old* `ShaderProgram` and never saw the new handle.

  **Fix verified**: The contract now uses **in-place mutation** via move assignment (`*existing_program = std::move(*new_program)`). `ShaderProgram` has `replace_handle(GLuint)` (deletes old handle via `glDeleteProgram`, assigns new) and `release_handle() -> GLuint` (extracts handle, sets to 0) for individual handle control. The hot-reload flow compiles a new `ShaderProgram`, then move-assigns its state into the existing object. The move assignment deletes the old GL handle and transfers the new handle (or generation counter in headless mode). The source is left with `handle_ = 0` so its destructor is no-op. All Materials holding `shared_ptr<ShaderProgram>` to the same object see the updated handle at `bind()` time automatically. The `shader_programs_` map is not touched.

- [x] **BLOCKING-3: Missing `file_watcher.cpp` — `FileWatcher::create()` and `~FileWatcher()` have no implementation file**

  **Status**: RESOLVED ✓

  **Original issue**: `FileWatcher::create()` and `~FileWatcher()` were declared non-inline in the header but no `file_watcher.cpp` was listed in the new files table.

  **Fix verified**: `src/engine/asset/file_watcher.cpp` is now listed in the "New files" table with the purpose "FileWatcher::create() factory and ~FileWatcher() destructor implementations." Section 17 provides full implementation details for both functions.

- [x] **BLOCKING-4: AssetDemoApp `setup()`/`render()` signatures do not match the `App` base class**

  **Status**: RESOLVED ✓

  **Fix verified**: The contract now implements Option A:
  1. `AssetDemoApp::setup()` takes `buddd::engine::RenderDevice& device` — matching the `App` base class signature.
  2. `AssetDemoApp::render()` takes `buddd::engine::RenderDevice& device, int frame` — matching the `App` base class signature.
  3. `AssetManager` is created directly in `setup()` via `AssetManager::create(device, "assets")` and stored as `std::unique_ptr<buddd::engine::AssetManager> asset_manager_` member.
  4. The class has no custom constructor (default constructor used), compatible with `std::make_unique<AssetDemoApp>()`.
  5. `demo_command.h` is now listed in the new files table (line 100).
  6. `handle_yaml_change()` sequencing is fixed: try-load first (step 3), update cache only on success (step 4), retain old on failure (step 5).
  7. The vestigial `Material::testing_shader_program()` reference has been removed from the API compatibility section.

## Warnings

Non-blocking concerns for awareness:

- [x] ~~**`handle_yaml_change` cache removal ordering**~~ — **RESOLVED**: Sequencing is now correct (try-load first, update cache only on success, retain old on failure). See lines 752–757.

- [x] ~~**`demo_command.h` missing from new files table**~~ — **RESOLVED**: Both `demo_command.h` and `demo_command.cpp` are now listed in the new files table (lines 100–101).

- [x] ~~**API compatibility section mentions `Material::testing_shader_program()`**~~ — **RESOLVED**: The vestigial reference has been removed from the API compatibility section (lines 1239–1245).

- **Test 26 (AC-025, P3) has no implementation pattern**: Unlike tests 1–25 which have detailed `TEST_CASE` patterns or explicit references, test 26 (OpenGL GPU handle change verification) is listed only as a table entry with no `TEST_CASE` skeleton. For P3 this is acceptable, but an implementer targeting OpenGL may want guidance.

- **`ShaderProgram::create()` returns `Result<ShaderProgram>` (by value) but hot-reload flow treats it as an owning pointer**: The `handle_source_change()` description says "Create new `ShaderProgram` via `ShaderProgram::create()`" and then `*existing_program = std::move(*new_program)`. The `new_program` in context would come from unwrapping the `Result<ShaderProgram>` (not a `unique_ptr`). This is workable with move semantics but the description's "new_program" phrasing is ambiguous about ownership — it could be read as requiring a `unique_ptr<ShaderProgram>` wrapper. A brief note clarifying the value semantics would help.

## Required changes

All four previously-requested changes have been verified as fixed:

1. **Fix AssetDemoApp `App` base class compatibility (BLOCKING-4)** — **VERIFIED**: `setup(RenderDevice&)` / `render(RenderDevice&, int)` now match the `App` base class. `unique_ptr<AssetManager>` stored as member, created in `setup()` via `AssetManager::create(device, "assets")`.

2. **Fix `handle_yaml_change` sequencing** — **VERIFIED**: Steps are now in correct order: log, determine type, try-load, on-success-update-cache, on-failure-retain-old.

3. **Add `commands/demo_command.h` to new files table** — **VERIFIED**: Both `demo_command.h` and `demo_command.cpp` are listed in the new files table (lines 100–101).

4. **Remove `Material::testing_shader_program()` from API compatibility section** — **VERIFIED**: No mention of this method remains in the API compatibility section (lines 1239–1245) or anywhere else in the contract.

No new blocking issues found.

## Suggested improvements

Optional ideas (not required):

- **Explicit destruction order assertion**: The contract already declares `asset_manager_` after `device_` in EngineService and notes destruction order. Consider adding an explicit `assert(device_ != nullptr)` or comment in `AssetManager` methods that use `device_`, as a defensive measure.

- **`ShaderProgramKey` hash quality**: The XOR-based hash `h(vp) ^ (h(fp) << 1)` may produce collisions if vertex and fragment paths share similar string hashes. Consider using a more robust combiner (e.g., `h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2))` or boost::hash_combine). Not blocking for the small expected map size in V1.
