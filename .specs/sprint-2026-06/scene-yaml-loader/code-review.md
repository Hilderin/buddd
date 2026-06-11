# Spec Review — Scene YAML Loader

Review date: 2026-06-10
Reviewer: code-reviewer agent

## Summary

The Loop 4 implementation adds `model:` directive support in `SceneLoader::load_entity()`, fixes `resolve_model()` in `AssetManager` with depth-first ModelNode tree traversal, and updates `demo.yaml` to a two-box format. The code changes themselves are correct — the `model:` directive properly loads a `ModelAsset` and expands it via `add_model_to_world()`, and the new `resolve_model()` correctly traverses the `ModelNode` tree depth-first returning the first found Model or an error. Build produces zero warnings from our code; all 485 tests pass.

**However, 3 blocking issues remain:**
1. **No unit test for AC-024** (model directive creates child entities with MeshRenderer)
2. **No unit test for AC-025** (resolve_model depth-first traversal with multi-node tree)
3. **E2E verification fails** — `demo.yaml` uses `colour: [1.0, 1.0, 1.0]` (sequence) but the Vec3 YAML converter expects mapping format `{x: 1.0, y: 1.0, z: 1.0}`, causing directional_light deserialization to error

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **TEST-001 gap — Inadequate entity existence test (AC-003)**: **RESOLVED** — Test 1 now checks `world.entity_count() > 0` and finds the entity via `each<CameraComponent>()`, verifying existence, name, and default transform.

- [x] **TEST-002 gap — Entity name not verified after YAML load (AC-004)**: **RESOLVED** — Test 2b loads entity with `name: foo` from YAML and asserts `entity.name() == "foo"` via `each<CameraComponent>()` lookup.

- [x] **TEST-003 gap — Default transform values not verified (AC-006)**: **RESOLVED** — Test 1 now verifies default position=[0,0,0], rotation=identity, scale=[1,1,1] on entity loaded without `transform:` block.

- [x] **TEST-004 gap — Component deserialization not verified (AC-007)**: **RESOLVED** — Test 4 finds the camera entity and checks `camera->fov_y() == Approx(1.0f)`.

- [x] **TEST-005 gap — Known component existence not verified (AC-008)**: **RESOLVED** — Test 5 verifies camera exists with correct `fov_y` and confirms only 1 CameraComponent exists (unknown was skipped).

- [x] **TEST-006 gap — Children hierarchy not verified (AC-023)**: **RESOLVED** — Test 8 verifies `child_count() == 2`, child names, and `child.parent() == parent_entity`.

- [x] **TEST-007 missing — No test for missing prefab file error (AC-010)**: **RESOLVED** — Test 9 loads `prefab: nonexistent_xyzzy_path` and asserts `REQUIRE_FALSE(result.has_value())`.

- [x] **TEST-008 missing — No test for prefab with >1 root entity error (AC-011)**: **RESOLVED** — Test 10 creates temp prefab with 2 root entities, loads it, and asserts `REQUIRE_FALSE(result.has_value())`.

- [x] **AC-009 implementation gap — Entity-level unknown keys produce no warning**: **RESOLVED** — `load_entity()` now has a static set of known entity keys and warns on any unknown key. Test 6 verifies unknown keys produce warning without blocking entity creation.

---

### Loop 4 — model: directive + resolve_model fix

- [ ] **AC-024 missing — No unit test for model: directive**: The spec requires "Unit test: load entity with `model: models/box/Box`, verify children exist and have `MeshRenderer` components." The `scene_loader_tests.cpp` has 10 tests but none test the `model:` directive. The implementation in `scene_loader.cpp` lines 244-254 is correct, but no test covers AC-024.

- [ ] **AC-025 missing — No unit test for resolve_model depth-first traversal**: The spec requires: "Unit test: create a `ModelAsset` with a multi-node tree, call `resolve_model()`, verify it returns the first node's Model. Unit test: create a `ModelAsset` with no mesh nodes, verify `resolve_model()` returns an error." The `resolve_model()` implementation in `asset_manager.cpp` (lines 120-148) correctly traverses depth-first, but no unit test exercises this code path with a multi-node tree or a no-model tree.

- [ ] **E2E verification fails — demo.yaml colour format incompatible with Vec3 YAML converter**: Running `buddd run assets/scenes/demo.yaml --frame 120 --capture 120:/tmp/scene_demo.png` fails with:
  ```
  InvalidArgument: Failed to deserialize component 'directional_light': Failed to deserialize property 'colour' of component 'directional_light': Failed to decode property 'colour': Vec3: failed to decode YAML node (expected mapping with x, y, z)
  ```
  The `colour` property is `[1.0, 1.0, 1.0]` (sequence format) but `vec3_yaml.h` defines `YAML::convert<math::Vec3>` to expect a mapping: `{x: 1.0, y: 1.0, z: 1.0}`. Fix: change `colour: [1.0, 1.0, 1.0]` to `colour: {x: 1.0, y: 1.0, z: 1.0}` in `assets/scenes/demo.yaml`.

## Warnings

Non-blocking concerns for awareness:

- **`parse_transform()` silently swallows errors**: When `parse_vec3()` or `parse_quat()` return errors (e.g., malformed array with wrong element count), the `if (pos)` / `if (rot)` / `if (sc)` guards silently skip the error and use defaults instead of propagating an error. This means a malformed `position: [1,2]` (only 2 values) silently uses default [0,0,0] rather than failing with an error. The contract designed `parse_transform` to return `Transform` (not `Result<Transform>`), making error propagation impossible here, but this still deviates from the spec's error case "Malformed YAML → error; scene load fails".

- **`add_model_to_world() moves Model out of ModelNode (destructive)**: The `resolve_model()` function uses `std::move(*const_cast<ModelNode&>(node).model)` to extract the model from the node. Similarly, `add_model_to_world_impl()` does `std::move(*node.model)`. This means after calling `resolve_model()` or `add_model_to_world()`, the ModelNode tree's model values are consumed and no longer available. This is a pre-existing pattern (the old code also moved), but it means `add_model_to_world()` can only be called once per ModelAsset tree. This is acceptable since scenes are loaded once at startup.

- **`engine_service.h` forward-declares `ComponentRegistry`** for `std::unique_ptr<ComponentRegistry> registry_` member, relying on the destructor being defined in the .cpp file where the full type is visible. This compiles correctly but is a fragile pattern.

## Required changes

Concrete, actionable changes requested:

### Blocking fixes

1. **Add unit test for AC-024**: In `tests/scene_loader_tests.cpp`, add a test that creates a scene YAML with an entity using `model: models/box/Box`, loads it, and verifies:
   - The entity exists in the World.
   - Child entities were created under the container entity via `add_model_to_world()`.
   - Child entities have `MeshRenderer` components.

2. **Add unit test for AC-025**: Add a test that:
   - Programmatically creates a `ModelAsset` with a multi-node tree (root has no model, child has a model).
   - Calls `AssetManager::resolve_model()` and verifies it returns the child node's Model.
   - Creates a `ModelAsset` with no mesh nodes in the entire tree.
   - Calls `resolve_model()` and verifies it returns an error.

3. **Fix demo.yaml colour format**: Change `colour: [1.0, 1.0, 1.0]` to `colour: {x: 1.0, y: 1.0, z: 1.0}` in `assets/scenes/demo.yaml` to match the Vec3 YAML converter format. Then re-run the E2E capture to verify visual output.

## Suggested improvements

Optional ideas (not required):

- Consider making `parse_transform()` return `Result<Transform>` so that malformed transform data can produce a proper error rather than silently using defaults.
- Consider adding the `#include "scene/component_registry/component_registry.h"` in `engine_service.h` instead of the forward declaration, to avoid the fragile pattern with `std::unique_ptr`.
- The spec and contract both document `colour: [1.0, 1.0, 0.9]`/`[1.0, 1.0, 1.0]` in sequence format, but the Vec3 YAML converter expects mapping format. The spec/contract should be updated to use the correct format.

## Re-review verdict (2026-06-10)

**All 9 pre-Loop-4 blocking issues are resolved. 3 new blocking issues found in Loop 4 (model directive + resolve_model fix).**

| Check | Status |
|---|---|
| Build warnings (src/, tests/) | ✅ Zero warnings |
| Tests pass | ✅ 485/485 (21,779 assertions) |
| AC-003: Entity existence | ✅ Test 1 verified |
| AC-004: Entity name from YAML | ✅ Test 2b verified |
| AC-006: Default transform | ✅ Test 1 verified |
| AC-007: Component deserialization | ✅ Test 4 verified |
| AC-008: Unknown component skipped | ✅ Test 5 verified |
| AC-009: Entity-level unknown key warning | ✅ Tests 6 verified |
| AC-010: Missing prefab error | ✅ Test 9 verified |
| AC-011: Multi-root prefab error | ✅ Test 10 verified |
| AC-023: Children hierarchy | ✅ Test 8 verified |
| **AC-024: model: directive creates child entities** | **❌ No test** |
| **AC-025: resolve_model depth-first traversal** | **❌ No test** |
| **E2E verification (demo.yaml visual output)** | **❌ Fails — colour format mismatch** |

**Verdict: REJECTED** — 3 blocking issues remain. The code changes are correct, but the acceptance criteria AC-024 and AC-025 lack required unit tests, and the E2E demo cannot run due to the colour format incompatibility in `demo.yaml`.
