# Governance Review — Asset Manager (SPEC-019/IMPL-019)

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [ ] **ShaderProgram architecture pattern mismatch (spec vs code)**: The spec (Section 5, lines 502–527) and implementation contract (Section 1, lines 172–260) describe `ShaderProgram` as a monolithic concrete class with `#ifdef BUDDD_HAS_DISPLAY` dispatch. The actual code implements it as an abstract base class (`src/engine/render/shader_program.h`) with separate backend subclasses (`ShaderProgramOpenGL`, `ShaderProgramHeadless`). The `create()` static factory was moved to `RenderDevice::create_shader_program()`. The wiki (`module-map.md`, `overview.md`, `glossary.md`) correctly reflects the abstract base pattern. This means spec and contract are out of sync with the code. The change was documented in coordination.md (`## code-implementer`) and accepted by code review, but the upstream spec and contract documents were never updated to reflect the refactored architecture.
- [ ] **ADR-016 references wrong spec number**: ADR-016 (line 240) says "SPEC-018 / IMPL-018: Asset Manager feature — the consumer of yaml-cpp." The correct spec is **SPEC-019** and the correct contract is **IMPL-019**. SPEC-018 is the Phong Lighting specification. This is a stale reference error in ADR-016.
- [ ] **Spec mentions ADR-015 as placeholder for yaml-cpp decision**: Spec line 846 says "An ADR (e.g., ADR-015) should be created." ADR-015 already existed (CI Docker image pre-publishing). The actual ADR created was ADR-016. Minor out-of-date reference — does not block.
- [ ] **AssetDemoApp bypasses EngineService for AssetManager**: The spec (lines 44, 84) and contract (Section 13, lines 790–822) say `AssetManager` is owned by `EngineService` and accessible via `engine_service.assets()`. The actual `AssetDemoApp` creates its own `AssetManager` as a member in `setup()`, bypassing `EngineService`. This was a human-directed change (DemoCommand removed, AssetDemoApp registered as scene under `buddd run`). The spec and contract were not updated to reflect this divergence.
- [ ] **Contract lists `demo_command.h/.cpp` but human deleted them**: The implementation contract (lines 100–101, DC-21) specifies `src/cmd/commands/demo_command.h` and `demo_command.cpp` as new files. The human requested their removal (apps registered directly as scenes under `buddd run`). The contract is out of sync with the implementation.
- [ ] **`src/cmd/app.h` and `src/cmd/app.cpp` modified despite contract forbidding it**: The implementation contract (line 146) lists `src/cmd/app.h` and `src/cmd/app.cpp` as forbidden to modify. These were modified to add `virtual on_frame_begin() -> void {}` to the `App` base class and call it in `run_app()`. This was a necessary architectural change to enable hot-reload polling, explicitly requested by the human. The contract divergence is intentional and acceptable, but the contract was never updated to reflect this.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Constitution violations

Checks against `docs/constitution/**`:

- [x] **CONST-001 (Architecture Boundaries)**: Fully respected. No GL/SDL types outside `src/engine/render/`. `ShaderProgram` abstract base class uses `uint32_t` not `GLuint` in public API. yaml-cpp is PRIVATE linked, no types leak into public headers. `Texture::replace_gl_handle(uint32_t)` avoids `GLuint` leak. All concrete backend types (OpenGL, Headless) remain inside `src/engine/`.
- [x] **CONST-002 (Testing Policy)**: Fully respected. 307 tests pass (100%, including 28 asset-manager-specific tests in `asset_manager_tests.cpp`). Tests follow Catch2 v3 patterns, all headless-compatible. Hot-reload pipeline verified end-to-end via `HotReloadApp` with vision analysis (before=red, after=blue).
- [x] **CONST-003 (Documentation Policy)**: Rule body is `TODO` — no violation possible. All applicable documentation artifacts exist (spec, implementation contract, ADR-016, wiki updates, critic reviews, code review, governance review).
- [x] **CONST-004 (Security Policy)**: Rule body is `TODO` — no violation possible. No security concerns identified (local filesystem-only I/O, yaml-cpp exceptions caught, no network access, no elevated privileges).
- [x] **Engineering Principles (principles.md)**: All respected. Explicit contracts exist, changes were scoped, existing conventions followed, requirements are testable, governance documents are largely coherent (minor non-blocking contradictions noted above).

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-016 (yaml-cpp dependency)**: Correctly documents the yaml-cpp decision. Covers YAML format choice, library selection, FetchContent integration, CONST-001 implications (PRIVATE linkage), exception safety wrappers, and alternatives. Matches existing ADR pattern. (Note: minor stale spec reference in line 240 — tracked in Cross-document coherence.)
- [x] **ADR-001 (Result/Error pattern)**: Contract explicitly references ADR-001 for exception safety (yaml-cpp exceptions caught and converted to `Result<T>`). All new APIs use `Result<T>`. **Compliant.**
- [x] **ADR-009 (test naming)**: Test file named `asset_manager_tests.cpp` (plural `_tests.cpp` suffix). **Compliant.**
- [x] **ADR-010 (no raw pointers)**: All public APIs use `T&` or `shared_ptr` — no raw pointers. **Compliant.**
- [x] **ADR-012 (EngineService navigable graph)**: `asset_manager_` declared AFTER `device_` in EngineService for correct destruction order. `assets()` accessor added. **Compliant.**

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **module-map.md**: Correctly documents `asset/` submodule with all file entries. `ShaderProgram` correctly shown as abstract base class with OpenGL/Headless backends. EngineService correctly shows `AssetManager` member and `assets()` accessor. `App::on_frame_begin()` documented as virtual default no-op. `HotReloadApp` listed in App subclasses table.
- [x] **overview.md**: Shows `asset/` directory tree with correct file listing. `shader_program*` files correctly reflect the abstract base + backends pattern. `asset-demo` and `hot-reload` scenes listed in key behaviors.
- [x] **data-flow.md**: Asset loading data flow correctly documented with YAML parsing, shader deduplication, hot-reload flow with GPU handle swap (replace_gl_handle / replace_handle), and recursive FileWatcher notes. Frame loop correctly includes `app.on_frame_begin()` step. Error propagation documented.
- [x] **glossary.md**: All 12 asset-system terms defined correctly (Asset, AssetManager, TextureAsset, MaterialAsset, ShaderProgram, ShaderProgramKey, FileWatcher, NullFileWatcher, FileEvent, DependencyMap, Asset ID, YAML asset metadata, yaml-cpp). `ShaderProgram` correctly shown as abstract base class.
- [x] **business-rules.md**: Asset Manager rules section complete with ID naming convention, YAML schemas, loading rules, shader deduplication, hot-reload (fully implemented with GPU handle swap), recursive inotify documentation. HotReloadApp and asset-demo scene registered in command and scene tables. App lifecycle updated to include `on_frame_begin()`.
- [x] **adr-index.md**: ADR-016 correctly listed with title and status `Accepted`.
- [x] **Wiki correctly reflects fully implemented hot-reload**: Hot-reload section updated from "stubs in V1" to "fully implemented" — handles YAML changes, texture source changes, and shader source changes with in-place GPU handle swaps. This matches the actual code state.

## Warnings

Non-blocking concerns for awareness:

- **ShaderProgram refactoring not backported to spec/contract**: The abstract base + backends refactoring is a significant architectural change that deviates from what SPEC-019 and IMPL-019 describe. The code is correct, the wiki is correct, but the upstream spec and contract documents are stale. Future readers relying on the spec or contract alone would get a misleading picture of the ShaderProgram design. A targeted update to the spec (Section 5) and contract (Section 1) is recommended.
- **ADR-016 reference error**: Line 240 cites SPEC-018 instead of SPEC-019. Minor but should be fixed to prevent confusion.
- **`BUDDD_TESTING` always-on in engine library**: Test-only accessors (`testing_shader_programs()`, `testing_inject_file_event()`) are compiled into release builds. Pre-existing condition noted in code review.
- **`ShaderProgramKey` hash quality**: XOR-based hash `h(vp) ^ (h(fp) << 1)` can produce collisions for swapped paths. Acceptable for expected map size in V1.
- **`src/cmd/app.h` and `src/cmd/app.cpp` modified despite contract forbidding it**: Necessary architectural change to enable hot-reload polling (`on_frame_begin()` virtual hook). Intentional and acceptable divergence.
- **`demo_command.h/.cpp` deleted per human request**: Contract specified these files (DC-21), but the human requested removal in favor of registering apps directly as scenes under `buddd run`. Contract is stale.
- **`testing_inject_file_event()` duplicates `poll_file_events()` lookup/dispatch logic**: Both methods implement an almost identical loop that iterates dependents and dispatches to handler functions. The test-only accessor should ideally reuse `poll_file_events()` by injecting directly into the FileWatcher's queue rather than duplicating the dispatch logic.
- **Hot-reload of shader source files on material YAML change is a V1 limitation**: When a material's YAML changes and specifies different shader paths, the new shader cannot be applied to an existing Material object. The handler logs a warning and skips shader recompilation. Texture bindings and constants are still updated.
- **Duplicate inotify events may trigger redundant hot-reload processing**: File copy operations (e.g., the hot-reload demo's texture swap) may generate multiple inotify events. The system processes each independently, which is harmless but does redundant work.
- **Untracked prototype files**: `assets/textures/texture_prototype_gray.yaml`, `assets/textures/texture_prototype_red.yaml`, `assets/materials/demo_hot_reload.yaml` are untracked files not used by any test or demo. Leftover artifacts with no functional impact.

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

- Update **SPEC-019** Section 5 (ShaderProgram) to reflect the abstract base + backend subclasses pattern (ShaderProgramOpenGL, ShaderProgramHeadless) and `RenderDevice::create_shader_program()` factory.
- Update **IMPL-019** Section 1 (ShaderProgram) to match the same abstract base + backend subclass pattern.
- Update **ADR-016** line 240 to reference SPEC-019/IMPL-019 instead of SPEC-018/IMPL-018.
- Consider updating the spec to note that `AssetDemoApp` creates its own `AssetManager` independent of `EngineService` (since the DemoCommand was removed per human direction).
- Consider removing `demo_command.h` and `demo_command.cpp` from the contract's new files table and DC-21 since they were deleted per human request.
- Consider updating the contract's "Files forbidden to change" section to acknowledge that `src/cmd/app.h` and `src/cmd/app.cpp` may need modification for lifecycle hooks (or document the accepted divergence).
