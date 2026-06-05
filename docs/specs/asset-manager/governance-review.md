# Governance Review — Asset Manager (SPEC-019/IMPL-019)

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [ ] **ShaderProgram architecture pattern mismatch (spec vs code)**: The spec (Section 5, lines 502–527) and implementation contract (Section 1, lines 172–260) describe `ShaderProgram` as a monolithic concrete class with `#ifdef BUDDD_HAS_DISPLAY` dispatch. The actual code implements it as an abstract base class (`src/engine/render/shader_program.h`) with separate backend subclasses (`ShaderProgramOpenGL`, `ShaderProgramHeadless`). The `create()` static factory was moved to `RenderDevice::create_shader_program()`. The wiki (`module-map.md`, `overview.md`, `glossary.md`) correctly reflects the abstract base pattern. This means spec and contract are out of sync with the code. The change was documented in coordination.md (`## code-implementer`) and accepted by code review, but the upstream spec and contract documents were never updated to reflect the refactored architecture.
- [ ] **ADR-016 references wrong spec number**: ADR-016 (line 240) says "SPEC-018 / IMPL-018: Asset Manager feature — the consumer of yaml-cpp." The correct spec is **SPEC-019** and the correct contract is **IMPL-019**. SPEC-018 is the Phong Lighting specification. This is a stale reference error in ADR-016.
- [ ] **Spec mentions ADR-015 as placeholder for yaml-cpp decision**: Spec line 846 says "An ADR (e.g., ADR-015) should be created." ADR-015 already existed (CI Docker image pre-publishing). The actual ADR created was ADR-016. Minor out-of-date reference — does not block.
- [ ] **AssetDemoApp bypasses EngineService for AssetManager**: The spec (lines 44, 84) and contract (Section 13, lines 790–822) say `AssetManager` is owned by `EngineService` and accessible via `engine_service.assets()`. The actual `AssetDemoApp` creates its own `AssetManager` as a stack-local variable in `setup()`, bypassing `EngineService`. This was a human-directed change (DemoCommand removed, AssetDemoApp registered as scene under `buddd run`). The spec and contract were not updated to reflect this divergence.
- [ ] **Contract lists `demo_command.h/.cpp` but human deleted them**: The implementation contract (lines 100–101) specifies `src/cmd/commands/demo_command.h` and `demo_command.cpp` as new files. The human requested their removal; the implementation matches the human directive. The contract is out of sync with the implementation.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Constitution violations

Checks against `docs/constitution/**`:

- [x] **CONST-001 (Architecture Boundaries)**: Fully respected. No GL/SDL types outside `src/engine/render/`. `ShaderProgram` base class uses `uint32_t` not `GLuint` in public API. yaml-cpp is PRIVATE linked, no types leak into public headers. `Texture::replace_gl_handle(uint32_t)` avoids `GLuint` leak. All concrete backend types (OpenGL, Headless) remain inside `src/engine/`.
- [x] **CONST-002 (Testing Policy)**: Fully respected. 304 tests pass (including 25 asset-manager-specific tests in `asset_manager_tests.cpp`). Tests follow Catch2 v3 patterns, all headless-compatible.
- [x] **CONST-003 (Documentation Policy)**: Rule body is `TODO` — no violation possible. All applicable documentation artifacts exist (spec, implementation contract, ADR-016, wiki updates, critic reviews, code review).
- [x] **CONST-004 (Security Policy)**: Rule body is `TODO` — no violation possible. No security concerns identified (local filesystem-only I/O, yaml-cpp exceptions caught, no network access, no elevated privileges).
- [x] **Engineering Principles (principles.md)**: All respected. Explicit contracts exist, changes were scoped, existing conventions followed, requirements are testable, governance documents are largely coherent (minor non-blocking contradictions noted above).

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-016 (yaml-cpp dependency)**: Correctly documents the yaml-cpp decision. Covers YAML format choice, library selection, FetchContent integration, CONST-001 implications (PRIVATE linkage), exception safety wrappers, and alternatives. Matches existing ADR pattern.
- [x] **ADR-001 (Result/Error pattern)**: Contract explicitly references ADR-001 for exception safety (yaml-cpp exceptions caught and converted to `Result<T>`). All new APIs use `Result<T>`. **Compliant.**
- [x] **ADR-009 (test naming)**: Test file named `asset_manager_tests.cpp` (plural `_tests.cpp` suffix). **Compliant.**
- [x] **ADR-010 (no raw pointers)**: All public APIs use `T&` or `shared_ptr` — no raw pointers. **Compliant.**
- [x] **ADR-012 (EngineService navigable graph)**: `asset_manager_` declared AFTER `device_` in EngineService for correct destruction order. `assets()` accessor added. **Compliant.**

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **module-map.md**: Correctly documents `asset/` submodule with all file entries. `ShaderProgram` correctly shown as abstract base class with OpenGL/Headless backends. EngineService correctly shows `AssetManager` member and `assets()` accessor.
- [x] **overview.md**: Shows `asset/` directory tree with correct file listing. `shader_program*` files correctly reflect the abstract base + backends pattern. `asset-demo` scene listed in key behaviors.
- [x] **data-flow.md**: Asset loading data flow correctly documented with YAML parsing, shader deduplication, hot-reload flow, and FileWatcher notes. Error propagation documented. Asset-demo scene dispatch included.
- [x] **glossary.md**: All 12 asset-system terms defined correctly (Asset, AssetManager, TextureAsset, MaterialAsset, ShaderProgram, ShaderProgramKey, FileWatcher, NullFileWatcher, FileEvent, DependencyMap, Asset ID, YAML asset metadata, yaml-cpp). `ShaderProgram` correctly shown as abstract base class.
- [x] **business-rules.md**: Asset Manager rules section complete with ID naming convention, YAML schemas, loading rules, shader deduplication, hot-reload (V1 stubs noted). Asset-demo scene registered in command and scene tables.
- [x] **adr-index.md**: ADR-016 correctly listed with title and status `Accepted`.
- [x] **Wiki correctly notes V1 stub status**: Hot-reload handlers documented as stubs. This matches the actual code state.

## Warnings

Non-blocking concerns for awareness:

- **ShaderProgram refactoring not backported to spec/contract**: The abstract base + backends refactoring is a significant architectural change that deviates from what SPEC-019 and IMPL-019 describe. The code is correct, the wiki is correct, but the upstream spec and contract documents are stale. Future readers relying on the spec or contract alone would get a misleading picture of the ShaderProgram design. A targeted update to the spec (Section 5) and contract (Section 1) is recommended.
- **ADR-016 reference error**: Line 240 cites SPEC-018 instead of SPEC-019. Minor but should be fixed to prevent confusion.
- **Hot-reload handlers are stubs in V1**: The pipeline structure (FileWatcher, dependency map, event injection) is complete and tested, but `handle_yaml_change()` and `handle_source_change()` are empty. This is a known V1 limitation documented in both wiki and code review.
- **`BUDDD_TESTING` always-on in engine library**: Test-only accessors (`testing_shader_programs()`, `testing_inject_file_event()`) are compiled into release builds. Noted in code review — non-blocking but worth addressing.
- **`AssetDemoApp` does not persist `AssetManager`**: The AssetManager is stack-local in `setup()` — hot-reload cannot function in the demo even if handlers were implemented. Contract showed `asset_manager_` as a member but code diverged.
- **InotifyFileWatcher only monitors top-level directory**: No recursive subdirectory watching. This may miss changes to assets in subdirectories.
- **`ShaderProgramKey` hash quality**: XOR-based hash `h(vp) ^ (h(fp) << 1)` can produce collisions for swapped paths. Acceptable for expected map size in V1.

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

- Update **SPEC-019** Section 5 (ShaderProgram) to reflect the abstract base + backend subclasses pattern (ShaderProgramOpenGL, ShaderProgramHeadless) and `RenderDevice::create_shader_program()` factory.
- Update **IMPL-019** Section 1 (ShaderProgram) to match the same abstract base + backend subclass pattern.
- Update **ADR-016** line 240 to reference SPEC-019/IMPL-019 instead of SPEC-018/IMPL-018.
- Consider updating the spec to note that `AssetDemoApp` creates its own `AssetManager` independent of `EngineService` (since the DemoCommand was removed per human direction).
- Consider removing `demo_command.h` and `demo_command.cpp` from the contract's new files table since they were deleted per human request.
