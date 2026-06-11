# Spec Review — Scene YAML Loader

**Re-review verdict (2026-06-09): All 3 blocking issues resolved. Spec accepted.**

**Re-review (2026-06-10, Loop 4 — fixes applied): All 3 issues resolved. Spec ACCEPTED.**

The spec-author applied all three fixes: (1) `asset_manager.h` and `asset_manager.cpp` added to the Modified files table, (2) `model:` added to the `load_entity()` parsed-field list, (3) edge case for entity with both `model:` and `children:` documented. No new issues found. All 25 acceptance criteria (AC-001 through AC-025) are documented and testable.

The spec-author addressed all three blocking issues from the previous review:
1. ✅ Added `Documentation to update` section listing 4 wiki files.
2. ✅ Added `Risks` section (R-01 to R-04) covering circular prefab detection, path resolution, FreeCameraMovement yaw/pitch, and entity name memory.
3. ✅ Resolved children: contradiction — children hierarchy included in V1 scope, AC-023 added, out-of-scope entry removed.

All 4 warnings from initial review also addressed: G-10 rephrased to "at least 7 unit tests", AssetDemoApp migration removed, path resolution clarified in Edge cases/A-02, circular prefab detection added via visited-set (R-01). No new issues found.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **DoR violation: Existing documentation updates not listed.** The Definition of Ready requires that "existing documentation that must be updated is listed (README, wiki, ADRs, other specs)." The spec's File Changes section lists only code files. At minimum, `docs/wiki/architecture/module-map.md` needs updating to document the new `SceneLoader` class in the `scene/` submodule, the `SceneApp` in the CLI file structure, the `Entity::name()`/`set_name()` additions, and the `EngineService::registry()` accessor. The spec should also identify whether any ADRs (e.g., ADR-014 CLI system) or the README need updating. **Action**: Add a `Documentation to update` section listing all non-code files that must be modified.
  **Resolution**: Added `Documentation to update` section (lines 391-402) listing 4 wiki files needing updates. Confirms no ADR updates required. ✅

- [x] **DoR violation: Risks and unknowns not surfaced.** The spec has an Assumptions section but no explicit Risks section. Several risks should be documented: (a) **Circular prefab reference** — The Edge cases table says "file not found on second load" but circular references would cause infinite recursion / stack overflow in `load_prefab()`, not a clean file-not-found error. The spec acknowledges this is not detected explicitly, which is a risk that should be surfaced. (b) **`children:` parsing scope ambiguity** (see next item). (c) **Path resolution dual-strategy** — scene files use "try as-is, then assets/" while prefab paths are resolved via AssetManager's base path. The two strategies may interact in unexpected ways. **Action**: Add a Risks section documenting these and any other operational risks.
  **Resolution**: Added `Risks` section (lines 504-511) with 4 risks: R-01 circular prefab detection (visited-set), R-02 path resolution dual-strategy, R-03 FreeCameraMovement yaw/pitch limitation, R-04 entity name memory. ✅

- [x] **Contradiction: Entity `children:` parsing vs. Out of scope.** The `load_entity()` API description (line 67) states it parses `name`, `use_prefab`, `components:`, `transform:`, and **`children:`**, and recursively processes children. However, the Out of scope section (NG — line 484) lists "Entity parenting in YAML (nested `children:` hierarchy — V1 may include it but it's secondary to prefab support)." Listings under "Out of scope" are conventionally excluded, yet the API mandates `children:` parsing. This is contradictory and must be resolved: either `children:` is explicitly in scope (update API/ACs) or `children:` is out of scope (remove from API description and note that the `parent` parameter in `load_entity()` is only for prefab-driven parenting).
  **Resolution**: Children hierarchy is now explicitly in scope (D-17 in Decision Log). Removed "Entity parenting in YAML" from Out of scope. Added AC-023 for children hierarchy with parent verification. ✅

### New issues (Loop 4 — model directive update)

- [x] **Missing File Changes entry for `resolve_model()` fix.** The spec describes a behavioral change to `AssetManager::resolve_model()` (lines 139-148): it must now traverse the `ModelNode` tree depth-first instead of only checking the root node. The current implementation is in `src/engine/asset/asset_manager.cpp` (lines 120-135), and this file must be modified to implement the fix. However, the File Changes table (lines 408-416) does NOT list `asset_manager.cpp` as a modified file. The header (`asset_manager.h`) declares the virtual method — it may not need a signature change, but the implementation file must be listed. **Action**: Add `src/engine/asset/asset_manager.cpp` (and `asset_manager.h` if the traverse helper signature changes) to the Modified files table. This is a DoR violation (interface changes must be documented).
  **Resolution**: Both `asset_manager.h` and `asset_manager.cpp` added to the Modified files table (lines 417-418). ✅

## Warnings (historical — all addressed)

Non-blocking concerns for awareness:

- ~~**G-11 / AC count mismatch**: G-11 promises exactly "7 unit tests" for the SceneLoader, but the acceptance criteria describe at least 9 distinct testable scenarios (AC-003 through AC-011). While some ACs can share a single test method, the hard constraint of "7" may force under-testing or contradict the scope. Recommend rephrasing G-11 to "at least 7 unit tests" or auditing the exact count.~~
  → **Addressed**: G-10 rephrased to "at least 7 unit tests".

- ~~**AssetDemoApp migration already done**: G-09 says "Migrate AssetDemoApp to use ctx.services.assets() instead of a local AssetManager (if not already done)." Inspection of `src/cmd/apps/asset_demo_app.cpp` (line 43) and `asset_demo_app.h` confirms the migration is already complete — there is no local `AssetManager`. The File Changes table lists `asset_demo_app.cpp` as modified, but the current code already satisfies AC-023. This creates potential confusion for the implementer (the "change" is a no-op verification).~~
  → **Addressed**: AssetDemoApp migration removed from goals, modified files, and ACs.

- ~~**Path resolution dual-strategy ambiguity**: The Edge cases table (line 432) says scene file paths are resolved by "try as-is first; if not found, resolve relative to assets base path." But prefab paths (Assumption A-02) are resolved via `AssetManager` which has a fixed base path of `"assets"` (A-03). If a scene file is at `/absolute/path/scene.yaml` and it references `use_prefab: prefabs/foo`, should the prefab be resolved relative to the scene file's directory or relative to `assets/`? This is not specified and may cause inconsistent behavior.~~
  → **Addressed**: Path resolution clarified in Edge cases (line 433-435) and Assumption A-02 (lines 493-495). Prefab references always resolve relative to assets base path.

- ~~**Circular prefab reference: underspecified failure mode**: The Edge cases table says circular references cause "file not found on second load — not detected explicitly, YAML load fails." In practice, if both files exist and reference each other, the runtime would hit infinite recursion / stack overflow, not a clean "file not found" error. The spec should either add explicit cycle detection (e.g., a visited-set in SceneLoader) or document the stack-overflow risk.~~
  → **Addressed**: Added R-01 in Risks section with visited-set mitigation.

- ~~**`FreeCameraMovement` constructor yaw/pitch not exposed**: The spec registers 5 properties for `FreeCameraMovement`, but its constructor takes `initial_yaw`/`initial_pitch` parameters (defaulting to 0). These are not exposed as registry properties, meaning YAML scene files cannot set initial camera orientation. This is acceptable for V1 (the user controls the camera anyway), but it should be noted as a limitation.~~
  → **Addressed**: Documented as R-03 in Risks section.

### New warnings (Loop 4 — model directive update)

- [x] **`load_entity()` description missing `model:` field**: Line 67 lists the entity fields parsed by `load_entity()` as `name`, `prefab`, `components:`, `transform:`, `children:` but does not include `model:`. Since `model:` is now a supported entity-level directive, it should be listed here for API documentation completeness. The Model directive section (lines 130-137) correctly describes the behavior, so this is non-blocking.
  **Resolution**: `model:` added to the parsed-field list at line 67. ✅
- [x] **Entity with both `model:` and `children:` not covered in Edge cases**: The edge cases table covers `model:` + `components:` (line 475), but the interaction between `model:` and explicit `children:` is unspecified (e.g., creation order of model-generated children vs explicit YAML children). Non-blocking because all children share the same parent entity, but should be documented for completeness.
  **Resolution**: Edge case added at lines 478-479 documenting creation order (model meshes first, then explicit children). ✅

## Required changes (all completed)

All 6 required changes from the initial review have been addressed in the spec:

1. ✅ Add a `Documentation to update` section listing `docs/wiki/architecture/module-map.md` and any ADRs or README files that need changes.
2. ✅ Add a `Risks` section documenting circular prefab recursion, path resolution edge cases, and the `children:` scope ambiguity.
3. ✅ Resolve the contradiction in the `children:` field: explicitly included in scope (removed from Out of scope, AC-023 added).
4. ✅ Rephrase G-10 to "at least 7 unit tests".
5. ✅ Clarify the path resolution algorithm in Edge cases and Assumption A-02.
6. ✅ Add cycle detection for prefab references (visited-set in R-01).

## Suggested improvements

Optional ideas (not required):

- Consider adding an explicit `base_path` parameter to `SceneLoader` to avoid reliance on `AssetManager`'s base path for file resolution.
- Consider noting in the spec that the `FreeCameraMovement` yaw/pitch initial values are fixed to 0 and not YAML-configurable in V1.
- The Entity name is stored as `std::string` on `EntityNode` (heap-allocated per entity). This is fine for V1 but could be noted as a memory consideration for scenes with thousands of entities.
- The spec could reference the existing `InvalidFormat` error category usage precedent (from glTF model loader) to strengthen consistency claims.
