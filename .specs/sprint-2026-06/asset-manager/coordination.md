# Workflow Coordination: asset-manager

## Orchestrator

**Feature**: `asset-manager`
**Status**: completed
**Current step**: completed
**Initial instructions**: Implémenter un système d'Asset Manager. Chaque asset est un fichier YAML avec type et version. L'ID est le chemin relatif depuis assets/. Types V1 : Texture, Material. Utiliser yaml-cpp. FileWatcher inotify en debug pour hot-reload automatique des YAML et fichiers sources.
**Notes**:
- Design decisions prises pendant la phase "grill me":
  - V1 : Material + Texture uniquement
  - yaml-cpp via FetchContent (ADR nécessaire)
  - ID = chemin relatif depuis assets/ (pas de champ id dans le YAML)
  - Lazy loading (pas de scan au démarrage)
  - Fichiers sources découplés des YAML, chemins dans le YAML
  - Material référence des fichiers shader .vert/.frag
  - Déduplication des programmes shaders par paire (vert+frag) — partage uniquement le programme GL, pas l'objet Material
  - MaterialInstance reporté en V2
  - FileWatcher Linux inotify, thread dédié, file thread-safe, toujours disponible
  - Hot-reload automatique : YAML + fichiers sources (images, shaders)
  - Version format : entier (version: 1)
  - Le champ id est retiré du YAML (l'ID est le chemin)
  - AssetManager crée les GPU resources (pas data-only) via un RenderDevice& passé à la création
  - EngineService expose render_device() — le DefaultRenderDevice
  - API : AssetManager::create<T>(id) pour loader + créer GPU
  - yaml-cpp lié en PRIVATE
  - poll_file_events() appel explicite par l'utilisateur
  - Settings texture parsés seulement en V1 (pas appliqués)
  - Assets de test versionnés dans tests/assets/
  - Texture hot-reload: update le GLuint handle dans l'objet Texture existant (mutations)
  - create_material: nouvel overload avec shared_ptr<ShaderProgram>, ancienne signature conservée
  - EngineService garde device() (pas render_device())

## spec-author

**Status**: completed
**Summary**:
- Spec rework to fix all 8 blocking issues from spec-critic rejection.
- BLOCKING-1: Added new `RenderDevice::create_material(shared_ptr<ShaderProgram>)` overload; kept existing signature unchanged; documented both signatures in ShaderProgram deduplication section.
- BLOCKING-2: `ShaderProgram` now explicitly defined in `src/engine/render/shader_program.h` (render layer, CONST-001 compliant); added to file list; noted cross-module inclusion from asset layer.
- BLOCKING-3: Replaced contradictory Strategy A/B with "mutable Texture handle swap" approach — Texture object is mutated in-place when source image changes; no material iteration needed.
- BLOCKING-4: AC-023 split into headless AC-023 (pipeline verification) and OpenGL AC-025 (GPU handle change, guarded by `BUDDD_HAS_DISPLAY`); AC-024 covers failure recovery (headless).
- BLOCKING-5: AC-026/AC-027 now explicitly specify `#ifdef BUDDD_TESTING` test-only accessor for dependency map inspection.
- BLOCKING-6: Added `try-catch` blocks around all yaml-cpp `YAML::LoadFile()` calls in both `create<TextureAsset>` and `create<MaterialAsset>` pseudo-code.
- BLOCKING-7: Added `virtual` keyword and `= 0` to `FileWatcher::poll_events()`, `start()`, `stop()`. Added `virtual ~FileWatcher()`.
- BLOCKING-8: Replaced all `render_device()` with existing `device()` accessor throughout; EngineService section now uses `engine_service.device()`.
- Additional: Removed duplicate AC-022; added Thread Safety section (section 10); updated Open Questions; added `shader_program.h/.cpp` and modified `render_device*` files to file lists.
**Artifacts**:
- `.specs/sprint-2026-06/asset-manager/spec.md`
**Questions for human**:
none
**Warnings**:
- Texture settings (wrap_s, wrap_t, min_filter, mag_filter, generate_mipmaps) are parsed and validated but NOT applied in V1. GPU texture creation uses current defaults (linear filtering, clamp-to-edge wrapping).
- The mutable Texture handle swap approach requires `Texture` concrete classes to expose a `swap_handle()` method (private, accessible to AssetManager via friendship or internal detail).
- FileWatcher coverage depends on `inotify_add_watch` succeeding for all subdirectories — if the inotify watch limit is exceeded, some files may not trigger hot-reload events.
**Blocking issues**:
- None

## spec-critic

**Status**: completed
**Summary**:
- Re-review completed. All 8 previously reported blocking issues have been resolved.
- All additional fixes (duplicate AC removal, thread safety documentation, open questions table) verified.
- No new blocking issues found. Spec is accepted for the next workflow stage.
**Artifacts**:
- `.specs/sprint-2026-06/asset-manager/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- Materials holding `shared_ptr<ShaderProgram>` must dereference it at bind time (not cache the raw GLuint) for hot-reload to work "automatically".
- Texture settings are parsed but not applied in V1 — implementers should avoid wiring up settings application prematurely.
- FileWatcher inotify_add_watch may fail if system watch limit is exceeded — hot-reload coverage may be reduced.
- SC-005 (microsecond timing) is difficult to verify reliably across CI hardware.
**Blocking issues**:
- None

## implementation-contract-author

**Status**: completed
**Summary**:
- Fixed BLOCKING-4: AssetDemoApp now uses `setup(RenderDevice&)` and `render(RenderDevice&, int)` matching the `App` base class signatures, stores `unique_ptr<AssetManager>` instead of depending on `EngineService`.
- Added `src/cmd/commands/demo_command.h` to the new files table and contract description.
- Fixed `handle_yaml_change()` sequencing to try-load first, then update cache only on success.
- Removed vestigial `Material::testing_shader_program()` from API compatibility section.
- Updated DC-21 to include `demo_command.h`.
**Artifacts**:
- `.specs/sprint-2026-06/asset-manager/implementation-contract.md`
**Questions for human**:
- none
**Warnings**:
- Option A (create AssetManager directly in setup()) was chosen per critic recommendation, bypassing EngineService for the demo app. This means AssetDemoApp does not exercise EngineService integration — AC-019/AC-020 are covered by unit tests instead.
**Blocking issues**:
- none

## implementation-contract-critic

**Status**: completed
**Summary**:
- Re-review completed. BLOCKING-4 (AssetDemoApp API mismatch) is now fixed.
- AssetDemoApp uses `setup(RenderDevice&)` / `render(RenderDevice&, int)` matching the `App` base class, stores `unique_ptr<AssetManager>`, and creates it in `setup()`.
- All 4 prior required changes verified: (1) App base class signatures fixed, (2) `handle_yaml_change()` sequencing corrected, (3) `demo_command.h` added to new files table, (4) vestigial `Material::testing_shader_program()` removed.
- No new blocking issues found. Contract accepted.
**Artifacts**:
- `.specs/sprint-2026-06/asset-manager/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- `ShaderProgram::create()` returns `Result<ShaderProgram>` by value; the hot-reload description's "new_program" phrasing is ambiguous about ownership semantics (vs a `unique_ptr<ShaderProgram>`).
- Test 26 (AC-025, P3) has no `TEST_CASE` implementation pattern — acceptable for P3, but OpenGL-targeting implementers may want guidance.
**Blocking issues**:
- none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: Thu Jun  4 11:41:55 PM EDT 2026
**Warnings**:
<none>
**Notes**: Approval donnée après validation du spec, du contrat, et ajout du demo asset-demo.

## code-implementer

**Status**: completed
**Summary**:
- Fix 1: `InotifyFileWatcher` now recursively watches all subdirectories using `add_watch_recursive()` instead of a single `inotify_add_watch()` on the top-level directory. Watch descriptors are tracked in `watch_dirs_` map; event paths are constructed relative to the watch base (matching the dependency map format).
- Fix 2: Path format mismatch resolved. `resolve_path()` now uses `std::filesystem::lexically_relative` to convert YAML source paths to paths relative to `base_path_`, matching the format produced by the recursive FileWatcher. Added `make_full_path()` helper for file I/O (prepends `base_path_`). Dependency map stores relative paths (`id + ".yaml"` for YAML, relative source/shader paths).
- Updated `load_texture`, `load_material`, `handle_yaml_change`, and `handle_source_change` to use relative paths for dependency tracking and `make_full_path()` for file I/O.
- Updated all hot-reload tests (21, 26, 27, 28) to inject paths in the new relative format.
- All 307 tests pass.
**Artifacts**:
- Modified:
  - `src/engine/asset/file_watcher_inotify.h` (replaced `watch_fd_` with `watch_dirs_` map, added `add_watch_recursive`)
  - `src/engine/asset/file_watcher_inotify.cpp` (recursive watch implementation, event path construction via `watch_dirs_`)
  - `src/engine/asset/asset_manager.h` (remove `static` from `resolve_path`, added `make_full_path`)
  - `src/engine/asset/asset_manager.cpp` (relative path normalization, dependency registration, file I/O helpers)
  - `tests/asset_manager_tests.cpp` (updated test 21/26/27/28 injected paths and comments)
**Questions for human**:
- None
**Warnings**:
- The `resolve_path` implementation uses `std::filesystem::lexically_relative` which is a purely lexical operation — it works correctly when both the base path and source path resolve to absolute paths under the same root. Paths outside the base path tree are returned unchanged (not watched by FileWatcher).
- All file I/O operations in `load_texture`, `load_material`, and hot-reload handlers now use `make_full_path()` to reconstruct the absolute path from the base path and the relative path. This ensures files are read from the correct location regardless of CWD.
- Material YAML change: shader program changes cannot be applied in-place with current Material API — V1 limitation documented in handler.
- In headless mode, `replace_gl_handle` and `release_gl_handle` are no-ops — texture hot-reload cannot be functionally verified in headless tests.
**Blocking issues**:
- None

## code-reviewer

**Status**: completed
**Summary**:
- Final review completed. All prior blocking issues resolved: hot-reload handlers implemented (no longer stubs), InotifyFileWatcher is recursive, AssetDemoApp persists AssetManager as member, path format between FileWatcher and dependency map is consistent.
- `on_frame_begin()` virtual hook added to `App` base class, called in `run_app()` between `begin_frame()` and `render()`.
- `HotReloadApp` test app created — swaps texture at frame 30, dual captures + vision analysis confirm before (red checker) and after (blue checker) differ correctly.
- 28 asset manager tests pass (150 assertions), including tests 21/22/26/27/28 for hot-reload pipeline (texture source change, shader source change, YAML change for texture, YAML change for material).
- `demo_command.h/.cpp` deleted per human request (apps registered directly as scenes under `buddd run`).
- Two contract divergences noted: `src/cmd/app.h/.cpp` were modified despite being in the forbidden list (necessary for `on_frame_begin`), and `demo_command` files were deleted per human request. Both are intentional and acceptable.
**Artifacts**:
- `.specs/sprint-2026-06/asset-manager/code-review.md`
**Questions for human**:
none
**Warnings**:
- `src/cmd/app.h` and `src/cmd/app.cpp` were modified despite being in the implementation contract's forbidden files list — necessary for `on_frame_begin()` hook. Intentional deviation.
- `demo_command.h/.cpp` deleted per human request (contract specified them as DC-21).
- `BUDDD_TESTING` always defined for the engine library (test-only accessors compiled into release builds). Pre-existing condition.
- `testing_inject_file_event()` duplicates `poll_file_events()` lookup/dispatch logic.
- Material shader hot-reload on YAML change is V1-limited (shader path changes not applied).
- Duplicate inotify events from file copy may cause redundant hot-reload processing (harmless).
**Blocking issues**:
- None

## adr-agent

**Status**: completed
**Summary**:
- Created ADR-016 documenting the yaml-cpp dependency decision for the Asset Manager.
- Covers: YAML format choice over JSON/TOML, yaml-cpp library choice over rapidyaml/libyaml, FetchContent integration, CONST-001 implications (PRIVATE linkage), and exception safety (yaml-cpp exceptions caught and converted to Result<T> per ADR-001).
- Matches existing ADR pattern (Context, Decision, Alternatives, Consequences, References).
**Artifacts**:
- `docs/adr/016-yaml-cpp-dependency.md` (new)
**Decisions needed**:
- The ADR is self-contained and documents decisions already made during the feature's design phase.
- ADR-015 already exists (CI Docker image pre-publishing); the next available number was 016.
**Questions for human**:
- None
**Warnings**:
- yaml-cpp requires C++ exceptions enabled — this is a constraint if the project later adopts `-fno-exceptions` configurations. The exception wrapper pattern isolates yaml-cpp exception usage, but yaml-cpp itself cannot be used without exception support.
**Blocking issues**:
- None

## constitution-agent

**Status**: completed
**Summary**:
- Constitution review completed. The Asset Manager implementation respects all existing rules.
- CONST-001 (Architecture Boundaries): Asset Manager lives in `src/engine/asset/` within the engine layer, uses `RenderDevice&` abstraction, yaml-cpp linked PRIVATE with no types in public headers. `Texture::replace_gl_handle(uint32_t)` avoids `GLuint` leak into base class. All `GLuint` types remain in `src/engine/render/` (ShaderProgramOpenGL, TextureOpenGL, etc.). No violations.
- CONST-002 (Testing Policy): 555-line test file at `tests/asset_manager_tests.cpp` with 25+ asset manager tests. All pass.
- No new dependency policy rule needed — the charter mentions "Dependency policy" as a category but no CONST-005 exists; this is a pre-existing gap, not triggered by this implementation.
- No constitutional changes proposed.
**Artifacts**:
none
**Changes needed**:
none
**Questions for human**:
none
**Warnings**:
- The charter lists "Dependency policy" as a rule category but no corresponding rule file (CONST-005) exists. This is a pre-existing gap, not triggered by the Asset Manager implementation. If the project wants to codify dependency addition guidelines (PRIVATE linkage, FetchContent pattern, no public header leaks), a future CONST-005 could formalise what current convention already enforces.
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**:
- Updated module-map.md: added `on_frame_begin()` to `app.h`/`app.cpp` descriptions, added `HotReloadApp` to App subclasses table, added `hot-reload` to scene dispatch, updated `asset_manager.cpp` to reflect fully implemented hot-reload, updated `file_watcher_inotify.cpp` for recursive watching.
- Updated data-flow.md: updated frame loop to include `app.on_frame_begin()` step, updated hot-reload flow to indicate functional (not stub) implementation with GPU handle swap, documented recursive inotify.
- Updated business-rules.md: added `HotReloadApp` to CLI output table, added `hot-reload` scene to available scenes, updated App lifecycle to include `on_frame_begin()`, rewrote Hot-reload section from "stub handlers" to "fully implemented" with details of handle swapping and recursive inotify.
**Artifacts**:
- `docs/wiki/architecture/module-map.md`
- `docs/wiki/architecture/data-flow.md`
- `docs/wiki/domain/business-rules.md`
**Changes made**:
- Documented `App::on_frame_begin()` virtual method (default no-op, called by `run_app()` between `begin_frame()` and `render()`)
- Added `HotReloadApp` (hot-reload verification test, swaps texture at frame 30)
- Updated hot-reload documentation from "stubs in V1" to "fully implemented" with `handle_yaml_change()`/`handle_source_change()` GPU handle swap
- Documented `InotifyFileWatcher` recursive directory watching via `add_watch_recursive()`
- Updated render loop lifecycle to include `app.on_frame_begin()` step
- Updated `AssetDemoApp` to note it overrides `on_frame_begin()` for polling
**Questions for human**:
none
**Warnings**:
- yaml-cpp exception-safety wrappers are a manual enforcement point
- HotReloadApp material shader program cannot be changed after creation (V1 limitation)
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
- Re-review completed. All constitutional rules remain respected (CONST-001 through CONST-004, all principles).
- 6 non-blocking cross-document contradictions identified: (1) ShaderProgram refactored to abstract base + backends pattern but spec/contract still describe monolithic class, (2) ADR-016 references SPEC-018 instead of SPEC-019, (3) Spec mentions ADR-015 placeholder but actual ADR is 016, (4) AssetDemoApp bypasses EngineService per human direction, (5) demo_command.h/.cpp listed in contract but deleted per human request, (6) src/cmd/app.h/.cpp modified despite contract forbidding it (necessary for on_frame_begin() hook).
- Since previous review: hot-reload handlers are now fully implemented (were stubs), InotifyFileWatcher now watches recursively (was top-level only), on_frame_begin() hook added to App base class, HotReloadApp test app created with vision analysis verification (before=red, after=blue), 307 tests pass (100%).
- ADR-016 correctly documents yaml-cpp dependency decision (FetchContent, PRIVATE link, exception safety).
- Wiki accurately reflects fully implemented hot-reload state, recursive inotify, and on_frame_begin() lifecycle.
- No blocking issues found. Recommended governance updates same as previous review (backport ShaderProgram architecture, fix ADR-016 reference) plus new notes on contract divergences.
**Artifacts**:
- `.specs/sprint-2026-06/asset-manager/governance-review.md`
**Questions for human**:
- Should the spec and contract be updated to reflect the ShaderProgram abstract base + backends architecture (ShaderProgramOpenGL, ShaderProgramHeadless, RenderDevice::create_shader_program), or is coordination.md documentation sufficient?
**Warnings**:
- ShaderProgram refactoring is not backported to spec/contract (spec Section 5 and contract Section 1 still describe monolithic concrete class).
- ADR-016 line 240 references SPEC-018 instead of SPEC-019.
- `BUDDD_TESTING` always-on in engine library (test-only accessors compiled into release).
- `src/cmd/app.h` and `src/cmd/app.cpp` modified despite being in contract's forbidden list (necessary for on_frame_begin() hook).
- `demo_command.h/.cpp` deleted per human request (contract still lists them as DC-21).
- `testing_inject_file_event()` duplicates `poll_file_events()` lookup/dispatch logic.
- Hot-reload of shader source files on material YAML change is a V1 limitation (shader path changes not applied).
- Duplicate inotify events may trigger redundant hot-reload processing (harmless).
- ShaderProgramKey hash vulnerable to collisions for swapped vertex/fragment paths.
- Untracked prototype asset files (texture_prototype_*.yaml, demo_hot_reload.yaml) have no functional impact.
**Blocking issues**:
- None

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## adr-agent` and `## constitution-agent` sections must include their extra fields (`**Decisions needed**` and `**Changes needed**` respectively).
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
