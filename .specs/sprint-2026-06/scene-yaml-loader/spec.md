# SPEC-NNNN — Scene YAML Loader

## Problem

The engine has no way to define scenes as data files. Every scene is hard-coded in C++ (`AssetDemoApp`, `CubeSceneApp`, etc.) — entities, transforms, components, and their properties are all set up programmatically. This makes iteration slow (edit → rebuild → run), prevents designers and artists from contributing scene content, and makes the engine harder to demo with a simple `buddd run scene.yaml`. Without a scene YAML format, there is no shared interchange between tools, no prefab system, and no path to an editor-driven workflow.

## Goals

- **G-01**: Define a YAML format for scene files that describes a hierarchical entity tree with components and transforms.
- **G-02**: Define a YAML format for scene files with `prefab:` and `model:` directives for reusable entity templates and model hierarchy expansion.
- **G-03**: Create a `SceneLoader` C++ class in `src/engine/scene/` that parses scene/prefab YAML files and populates a `World`.
- **G-04**: Create a `SceneApp` subclass of `App` that loads a scene from a CLI-provided YAML path.
- **G-05**: Add CLI auto-detection: `buddd run <path.yaml>` falls through to `SceneApp` when the argument ends with `.yaml`/`.yml`.
- **G-06**: Register `FreeCameraMovement` as a typed component in `ComponentRegistry` with 5 properties.
- **G-07**: Store `ComponentRegistry` as a persistent member of `EngineService` with a `registry()` accessor.
- **G-08**: Add an entity name field (`std::string`) on `EntityNode`, exposed via `Entity::name()`/`set_name()`.
- **G-09**: Provide demo scene files (`assets/scenes/demo.yaml`, `assets/prefabs/free_camera.yaml`).
- **G-10**: Provide at least 7 unit tests for the `SceneLoader` and 1 visual demo verification.

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | No editor changes — engine-only V1. |
| NG-02 | No `--scene` CLI flag. Only auto-detection by file extension. |
| NG-03 | No component overrides on prefab instances (V1 only supports transform overrides on root entity). |
| NG-04 | No recursive prefab nesting (a prefab using another prefab). |
| NG-05 | No runtime scene reloading or hot-reload of YAML files. |
| NG-06 | No YAML schema validation beyond what yaml-cpp provides. |
| NG-07 | No changes to existing `App` subclasses. |
| NG-08 | No changes to `run_app()` or the `App` base class. |
| NG-09 | No changes to existing `add_model_to_world()` function (already exists and correct). |

## Key entities

### `SceneLoader` class (`src/engine/scene/scene_loader.h/.cpp`)

```cpp
namespace buddd::engine {

class SceneLoader {
public:
    SceneLoader(World& world, ComponentRegistry& registry, AssetManager& assets);

    /// Load a scene YAML file and populate the World.
    [[nodiscard]] auto load_from_file(const std::string& path) -> Result<void>;

    /// Load from an already-parsed YAML node.
    [[nodiscard]] auto load_from_yaml(const YAML::Node& node) -> Result<void>;

private:
    auto load_entity(const YAML::Node& node, Entity parent = Entity::none()) -> Result<Entity>;
    
    /// Load a prefab YAML and return the root entity (for transform composition).
    [[nodiscard]] auto load_prefab(const std::string& path) -> Result<Entity>;

    World& world_;
    ComponentRegistry& registry_;
    AssetManager& assets_;
};

} // namespace buddd::engine
```

- `load_from_file(path)` opens and parses a YAML file, validates the `type` and `version` fields, then iterates the `entities:` array calling `load_entity()` for each top-level entity.
- `load_from_yaml(node)` is the same but operates on an already-parsed YAML node (useful for testing and in-memory workflows).
- `load_entity(node, parent)` parses a single entity: reads `name`, `prefab`, `model`, `components:`, `transform:`, and `children:`. Recursively processes children.
- `load_prefab(path)` loads a prefab YAML file, creates entities from it, and returns the root entity for transform composition.
- All errors are propagated via `Result<void>` or `Result<Entity>`.

### `SceneApp` class (`src/cmd/apps/scene_app.h/.cpp`)

```cpp
namespace buddd::cmd::app {

class SceneApp final : public App {
public:
    explicit SceneApp(std::string scene_path);

    auto config() const -> AppConfig override;

    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void> override;

private:
    std::string scene_path_;
};

} // namespace buddd::cmd::app
```

- `config()` returns `AppConfig` with title derived from the scene file name (e.g. `"demo"`), width 1024, height 768.
- `setup()` creates a `SceneLoader` with `ctx.world`, `ctx.services.registry()`, `ctx.services.assets()`, then calls `load_from_file(scene_path_)`.

### YAML file formats

**Scene file** (`assets/scenes/demo.yaml`):
```yaml
type: Scene
version: 1
entities:
  - prefab: prefabs/free_camera
    name: main_camera
    transform:
      position: [0, 0, 0]

  - name: box_via_directive
    model: models/box/Box
    transform:
      position: [-2, 0, 0]

  - name: box_via_component
    components:
      - type: mesh_renderer
        properties:
          model: models/box/Box
    transform:
      position: [2, 0, 0]

  - name: light
    components:
      - type: directional_light
        properties:
          color: [1.0, 1.0, 0.9]
          intensity: 1.0
    transform:
      rotation: [0.707, -0.707, 0.0, 0.0]
```

### Model directive

`model:` at the entity level is a **directive** (not a component). It loads a glTF model through the asset system and expands it into a hierarchy of child entities:

1. `SceneLoader` calls `AssetManager::create<ModelAsset>(path)` to load the `ModelAsset` from the given asset ID.
2. It then calls `add_model_to_world(world_, root_node, entity)` — an existing inline function in `src/engine/render/model_utils.h` — which traverses the `ModelNode` tree depth-first and creates child entities with `Transform` + `MeshRenderer` components for each node that has a Model.

The entity with the `model:` directive acts as the container/parent. The model's meshes are created as children via `add_model_to_world()`.

### `resolve_model()` fix

`resolve_model()` (used by the `mesh_renderer.model` property via the TypeRegistry, e.g. `properties: { model: models/box/Box }`) currently only checks the root `ModelNode` for a Model. A glTF file may have multiple meshes across multiple nodes, so `resolve_model()` now:

- Loads the `ModelAsset` via `create<ModelAsset>(id)`.
- Traverses the `ModelNode` tree depth-first.
- Returns the first node that has a Model (`std::optional<Model>` is engaged).
- Returns an error if no node with a Model is found in the entire tree.

This ensures both the `model:` directive and the `mesh_renderer.model` property work correctly with any glTF file.

**Prefab file** (`assets/prefabs/free_camera.yaml`):
```yaml
type: Prefab
version: 1
entities:
  - name: free_camera
    components:
      - type: camera
        properties:
          fov_y: 1.047
          aspect: 1.333
          near: 0.1
          far: 100.0
      - type: free_camera_movement
        properties:
          move_speed: 5.0
          mouse_sensitivity: 0.002
    transform:
      position: [3.0, 2.0, 3.0]
```

### Transform composition (prefab + instance)

When an entity has both `prefab:` and a `transform:` block, the prefab's root entity transform is composed with the instance transform:

- `position` = prefab_position + instance_position (additive)
- `scale` = prefab_scale * instance_scale (multiplicative, element-wise)
- `rotation` = prefab_rotation * instance_rotation (quaternion multiplication)

All three are optional. Defaults: position `[0,0,0]`, rotation `[1,0,0,0]` (w,x,y,z identity quaternion), scale `[1,1,1]`.

### Entity name

A `name` field is added to `World::EntityNode`:
```cpp
struct EntityNode {
    EntityId id_;
    Transform transform_;
    std::string name_;                    // NEW
    EntityNode* parent_ = nullptr;
    std::vector<std::unique_ptr<EntityNode>> children_;
    std::vector<std::unique_ptr<Component>> components_;
    World* world_ = nullptr;
    bool pending_destroy_ = false;
};
```

And corresponding accessors on `Entity`:
```cpp
auto name() const noexcept -> const std::string&;
void set_name(const std::string& name);
```

Default name is empty string.

### FreeCameraMovement registration

Registered in `register_all_components()` with 5 properties:

| Property | Type | Default | Constraints |
|---|---|---|---|
| `move_speed` | float | 5.0 | min 0 |
| `mouse_sensitivity` | float | 0.002 | min 0 |
| `pitch_clamp_degrees` | float | 89.0 | — |
| `invert_yaw` | bool | false | — |
| `invert_pitch` | bool | false | — |

These map to the public fields on `FreeCameraMovement`:
- `move_speed` → `FreeCameraMovement::move_speed`
- `mouse_sensitivity` → `FreeCameraMovement::mouse_sensitivity`
- `pitch_clamp_degrees` → `FreeCameraMovement::pitch_clamp_degrees`
- `invert_yaw` → `FreeCameraMovement::invert_yaw`
- `invert_pitch` → `FreeCameraMovement::invert_pitch`

### ComponentRegistry in EngineService

`EngineService` gains a new member and accessor:

```cpp
class EngineService {
public:
    auto registry() noexcept -> ComponentRegistry&;
    // ... existing members ...
private:
    std::unique_ptr<ComponentRegistry> registry_;  // NEW
    // ... existing members ...
};
```

The `ComponentRegistry` is created during `EngineService::create()` after `register_builtin_types()` and before `register_all_components(registry)` — but now stored persistently instead of as a local variable.

## Actors

| Actor | Description |
|---|---|
| **Developer** | Runs `buddd run <path.yaml>` to view a scene defined in YAML without writing C++ code. |
| **Content creator** | Creates or edits YAML scene and prefab files in `assets/scenes/` and `assets/prefabs/`. |
| **Test runner** | Runs unit tests that verify `SceneLoader` behaviour programmatically. |

## User-visible behavior

### `buddd run demo.yaml` (or any `.yaml`/`.yml`)

When the argument after `run` ends with `.yaml` or `.yml` and the file exists, a `SceneApp` is created instead of the usual named-app dispatch. The scene file is parsed, entities and components are created in the `World`, and the window opens with the scene rendered.

**Behavior details:**
- File extension is checked case-insensitively (`.yaml`, `.yml`).
- If the file does not exist, an error is printed and the app exits with code 1.
- If the YAML is malformed, an error is printed and the app exits with code 1.
- `--frame N` and `--capture N:path` work via `run_app()` infrastructure.
- If no camera component exists in the scene, a `BUDDD_LOG_WARN` is emitted (existing `RenderSystem` behavior).

### SceneApp window

- Title: the scene file name without extension (e.g. `"demo"` for `demo.yaml`).
- Size: 1024×768 (default, configurable in future).
- Runs until window close or Escape (via `FreeCameraMovement` or existing input handlers).

### Error handling during scene load

| Condition | Behavior |
|---|---|
| Unknown component type (`type:` not in registry) | Log a warning, skip that component, continue loading other components on the entity. |
| Asset reference not found (e.g. model ID `models/box/Box` cannot be resolved) | Return an error; scene load fails, app exits with code 1. |
| Malformed YAML (parse error) | Return an error; app exits with code 1. |
| Unknown YAML keys at entity or property level | Log a warning, continue (forward-compatible). |
| Entity with both `prefab:` and `components:` | Components from the entity's own `components:` list are added after prefab components. No overrides — duplicate component types may cause UB (same as adding two cameras to one entity in code). |
| Entity with `prefab:` but no prefab file | Return an error; scene load fails. |
| Prefab file has more than one top-level entity | Return an error; scene load fails (V1 limitation). |

## User stories

### Story 1 — Load a scene from YAML with two boxes (Priority: P1)

As a developer, I want to run `buddd run assets/scenes/demo.yaml` and see both boxes rendered, so that I can verify the `model:` directive and the `mesh_renderer.model` property work correctly.

**Given** the `buddd` binary is compiled with display support
**And** `assets/scenes/demo.yaml` exists
**When** I run `buddd run assets/scenes/demo.yaml --frame 1 --capture 1:out.png`
**Then** a window opens showing the scene (two Box models at positions [-2, 0, 0] and [2, 0, 0] + directional light)
**And** the process exits with code 0
**And** `out.png` shows the rendered scene with two boxes

### Story 2 — CLI auto-detection (Priority: P1)

As a developer, I want `buddd run <file.yaml>` to be automatically routed to `SceneApp`, so that I don't need a separate `--scene` flag.

**Given** the `buddd` binary is compiled
**When** I run `buddd run assets/scenes/demo.yaml`
**Then** a `SceneApp` is created and the scene is loaded
**And** the process does NOT fail with "Unknown scene"

**Given** I run `buddd run nonexistent.yaml`
**Then** an error is printed (file does not exist)
**And** the process exits with code 1

**Given** I run `buddd run demo.txt`
**Then** the argument is NOT treated as YAML scene
**And** the existing "Unknown scene: 'demo.txt'" error is shown

### Story 3 — Prefab loading with transform composition (Priority: P1)

As a content creator, I want to use `prefab:` to include a reusable entity template with transformed position.

**Given** a scene YAML references `assets/prefabs/free_camera.yaml` via `prefab:`
**And** the scene entity specifies `transform: { position: [0, 0, 0] }`
**When** the scene is loaded
**Then** the prefab's entities are created in the `World`
**And** the root entity's position is the sum of prefab position `[3, 2, 3]` and instance position `[0, 0, 0]` = `[3, 2, 3]`
**And** the camera and free_camera_movement components from the prefab are present on the entity

### Story 4 — Entity names (Priority: P2)

As a developer, I want entities loaded from YAML to carry their `name:` field so I can identify them at runtime.

**Given** a scene YAML with `entities: [{name: my_box, ...}]`
**When** the scene is loaded
**Then** the created entity has `entity.name() == "my_box"`

### Story 5 — FreeCameraMovement in registry (Priority: P1)

As a developer, I want `FreeCameraMovement` to be registered in `ComponentRegistry` so it can be deserialized from YAML.

**Given** `register_all_components(registry)` is called
**When** I call `registry.describe("free_camera_movement")`
**Then** it returns non-null
**And** `property_count() == 5`
**And** property `move_speed` is of type `float` with min 0
**And** property `mouse_sensitivity` is of type `float` with min 0
**And** property `pitch_clamp_degrees` is of type `float`
**And** property `invert_yaw` is of type `bool`
**And** property `invert_pitch` is of type `bool`

### Story 6 — Unknown component is skipped (Priority: P2)

As a developer, I want unknown component types to be skipped with a warning rather than causing a hard failure, so that scene files with future component types still load partially.

**Given** a scene YAML with an entity that has a component with `type: "future_component"`
**When** the scene is loaded
**Then** the unknown component is skipped
**And** a warning is logged
**And** other components on the same entity are loaded correctly

### Story 8 — Model directive loading (Priority: P1)

As a content creator, I want to use `model:` at the entity level to load a glTF model and have it automatically expand into child entities with MeshRenderer, so I don't need to manually define mesh_renderer components for each mesh node.

**Given** a scene YAML with an entity `{name: my_model, model: models/box/Box, transform: {position: [1, 0, 0]}}`
**When** the scene is loaded
**Then** the entity `my_model` is created in the World
**And** child entities are created under `my_model` for each mesh node in the glTF
**And** the child entities have `MeshRenderer` components with the correct Model
**And** the child entities' transforms are composed with the parent's transform

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `src/engine/scene/scene_loader.h` exists and declares `class SceneLoader` in `namespace buddd::engine` with `load_from_file(path) -> Result<void>` and `load_from_yaml(node) -> Result<void>`. | File exists; inspect declaration. |
| AC-002 | `SceneLoader` stores references to `World&`, `ComponentRegistry&`, `AssetManager&`. | Inspect header; verify member references. |
| AC-003 | `SceneLoader::load_from_file()` parses a YAML file, validates `type: Scene` or `type: Prefab` and `version: 1`, and creates entities in the `World`. | Unit test: load a minimal scene with one entity and verify entity exists. |
| AC-004 | Entities created by `SceneLoader` have their `name` property set via `Entity::set_name()`. | Unit test: load entity with `name: foo`, verify `entity.name() == "foo"`. |
| AC-005 | `prefab:` loads a prefab YAML file and creates the prefab's entities as children/sub-entities of the current context. The prefab's root entity transform is composed with the instance's transform. Position is additive, scale is multiplicative, rotation is quaternion-multiplied. | Unit test: load an entity with `prefab:` and instance `transform: {position: [1,2,3]}`, verify final position = prefab_position + [1,2,3]. |
| AC-006 | Transform fields (`position`, `rotation`, `scale`) are optional and default to `[0,0,0]`, `[1,0,0,0]`, `[1,1,1]` respectively. | Unit test: entity with no transform block has default transform values. |
| AC-007 | Components defined in `components:` are created via `ComponentRegistry::create()` and populated via `deserialize_component()`. | Unit test: load entity with `components: [{type: camera, properties: {fov_y: 1.0}}]` and verify camera's fov_y is 1.0. |
| AC-008 | Unknown component types are skipped with a warning, and other components on the same entity are still loaded. | Unit test: load entity with one known and one unknown component; verify known component exists and unknown is skipped. |
| AC-009 | Unknown YAML keys at entity or component level produce a warning (not an error). | Unit test: load entity with extra unknown key; verify load succeeds (no error). |
| AC-010 | Missing prefab file causes a hard error (scene load fails). | Unit test: load scene with `prefab: nonexistent` and verify error. |
| AC-011 | Prefab file with more than one top-level entity returns an error. | Unit test: load prefab with 2 top-level entities and verify error. |
| AC-012 | `src/cmd/apps/scene_app.h` exists and declares `class SceneApp` inheriting `App`. | File exists; inspect class declaration. |
| AC-013 | `SceneApp::config()` returns `AppConfig` with title derived from scene file base name, width 1024, height 768. | Inspect implementation; verify returned `AppConfig` fields. |
| AC-014 | `SceneApp::setup()` creates a `SceneLoader` and calls `load_from_file(scene_path_)`. | Inspect implementation; verify `SceneLoader` construction and load call. |
| AC-015 | `src/cmd/main.cpp` routes positional args ending in `.yaml`/`.yml` (case-insensitive) to `SceneApp`. | Inspect `main.cpp`; verify YAML extension check before existing app dispatch. |
| AC-016 | `EngineService` stores a `std::unique_ptr<ComponentRegistry>` and exposes `registry() -> ComponentRegistry&`. | Inspect `engine_service.h`; verify member and accessor. |
| AC-017 | `ComponentRegistry` is created during `EngineService::create()` and persists for the lifetime of `EngineService`. | Inspect `engine_service.cpp`; verify `registry_` is created and `register_all_components()` is called on it. |
| AC-018 | `FreeCameraMovement` is registered in `register_all_components()` as `free_camera_movement` with 5 properties: `move_speed` (float, min 0), `mouse_sensitivity` (float, min 0), `pitch_clamp_degrees` (float), `invert_yaw` (bool), `invert_pitch` (bool). | Inspect `register_all_components.cpp`; verify registration call with all 5 properties. |
| AC-019 | `World::EntityNode` has a `std::string name_` member and `Entity::name()/set_name()` accessors. | Inspect `world.h` and `entity.h`; verify field and accessors. |
| AC-020 | `assets/scenes/demo.yaml` exists with 4 entities: camera (using free_camera prefab), box_via_directive (model: directive), box_via_component (mesh_renderer with `models/box/Box`), and light (directional_light). | File exists; inspect content matches format. |
| AC-021 | `assets/prefabs/free_camera.yaml` exists with camera + free_camera_movement components. | File exists; inspect content matches format. |
| AC-022 | `Entity::name()` defaults to empty string for entities created without `set_name()`. | Unit test: create entity via `World::add_entity()`, verify `name() == ""`. |
| AC-023 | Entities with `children:` in YAML have their children created as child entities in the `World` hierarchy, with the parent's entity set as the parent. | Unit test: load entity with `children: [{name: child1}]`, verify child entity exists and `entity.parent() == parent_entity`. |
| AC-024 | Entity with `model:` directive loads a `ModelAsset`, calls `add_model_to_world()`, and creates child entities with `MeshRenderer` components for each mesh node in the glTF tree. | Unit test: load entity with `model: models/box/Box`, verify children exist and have `MeshRenderer` components. |
| AC-025 | `resolve_model()` traverses the `ModelNode` tree depth-first and returns the first node with a Model, or returns an error if no node has a Model. | Unit test: create a `ModelAsset` with a multi-node tree, call `resolve_model()`, verify it returns the first node's Model. Unit test: create a `ModelAsset` with no mesh nodes, verify `resolve_model()` returns an error. |

## File changes

### Created

| File | Purpose |
|---|---|---|
| `src/engine/scene/scene_loader.h` | `SceneLoader` class declaration. |
| `src/engine/scene/scene_loader.cpp` | `SceneLoader` implementation. |
| `src/cmd/apps/scene_app.h` | `SceneApp` class declaration. |
| `src/cmd/apps/scene_app.cpp` | `SceneApp` implementation. |
| `assets/prefabs/free_camera.yaml` | Free camera prefab with camera + free_camera_movement components. |

### Modified

| File | Change |
|---|---|
| `src/engine/scene/world.h` | Add `std::string name_` to `EntityNode`. |
| `src/engine/scene/entity.h` | Add `name()` and `set_name()` methods. (Template implementations in `world.h` may also need update.) |
| `src/engine/engine_service.h` | Add `std::unique_ptr<ComponentRegistry> registry_` member and `registry()` accessor. |
| `src/engine/engine_service.cpp` | Create `registry_` in `create()`, store it persistently, call `register_all_components()` on it. |
| `src/engine/scene/component_registry/register_all_components.cpp` | Register `FreeCameraMovement` as `"free_camera_movement"` with 5 properties. |
| `src/cmd/main.cpp` | Add YAML extension detection before named-app dispatch; route to `SceneApp`. |
| `assets/scenes/demo.yaml` | Updated to two-box scene: box_via_directive (model: directive) and box_via_component (mesh_renderer with model property). |
| `src/engine/asset/asset_manager.h` | Add `resolve_model()` traversing ModelNode tree to find first mesh node. |
| `src/engine/asset/asset_manager.cpp` | Implement recursive `resolve_model()` with depth-first ModelNode traversal. |
| `tests/CMakeLists.txt` | No change needed (auto-discovers `*_tests.cpp`). |

### Unchanged

| File | Reason |
|---|---|
| `src/cmd/app.h` | `App` interface unchanged. |
| `src/cmd/app.cpp` | `run_app()` unchanged. SceneApp uses existing lifecycle. |
| `src/engine/engine_context.h` | `EngineContext` unchanged. |
| All existing `App` subclasses | No changes to existing scenes. |
| `assets/models/box/Box.yaml` | Existing model definition, used by demo scene. |
| `assets/materials/demo_cube.yaml` | Existing material, unchanged. |

## Documentation to update

The following non-code files must be updated to reflect the new scene YAML loader feature:

| File | Changes needed |
|---|---|
| `docs/wiki/architecture/module-map.md` | Add `SceneLoader` class in the `scene/` submodule. Add `SceneApp` in the CLI apps section. Document `Entity::name()`/`set_name()` additions. Document `EngineService::registry()` accessor. |
| `docs/wiki/architecture/overview.md` | Update directory layout to include `src/engine/scene/` for `SceneLoader`. Update CMake targets if a new library target is created. |
| `docs/wiki/domain/business-rules.md` | Document CLI command behavior for YAML auto-detection (`buddd run <path.yaml>`). Document entity naming conventions. |
| `docs/wiki/architecture/dependency-map.md` | Add `scene_loader` → `AssetManager` and `scene_loader` → `ComponentRegistry` dependencies. |

**ADRs**: No ADR updates are required. ADR-014 (CLI system) already describes the CLI extension mechanism and is compatible with the `.yaml`/`.yml` auto-detection added here. ADR-027 (editor architecture) is not affected since this is an engine-only V1 change.

## E2E Verification

| Method | Description |
|---|---|
| **Visual demo (display)** | Run `buddd run assets/scenes/demo.yaml --frame 120 --capture 120:out.png`. Verify `out.png` shows a scene with two rendered Box models (one via `model:` directive, one via `mesh_renderer.model` property) with directional light. |
| **Unit tests (headless)** | At least 7 unit tests in `tests/scene_loader_tests.cpp` (or similar). See User Stories for test coverage. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A new scene can be created by writing a YAML file in `assets/scenes/` and running `buddd run <path>`, without any C++ changes. | Developer creates `assets/scenes/test_scene.yaml`, runs `buddd run assets/scenes/test_scene.yaml`, sees the scene rendered. |
| SC-002 | A new prefab can be created by writing a YAML file in `assets/prefabs/` and referenced with `prefab:` from any scene file. | Developer creates a prefab, updates a scene to use it, runs the scene, and sees the prefab entities created. |
| SC-003 | All scene_loader unit tests pass in headless CI. | Run `ctest --preset debug` or equivalent; all scene_loader tests pass. |
| SC-004 | All existing `App` subclasses continue to build and run unchanged. | Build succeeds; `buddd run triangle --frame 1` renders correctly. |

## Edge cases

| Case | Expected behavior |
|---|---|
| Empty `entities:` array | Scene loads successfully with zero entities. |
| Entity with neither `name:` nor `prefab:` | Entity is created with empty name. |
| Entity with `prefab:` pointing to prefab that has no `entities:` | Error: prefab file invalid. |
| Prefab with zero entities | Error: prefab must have at least one entity. |
| Transform with partial fields (e.g. only `position:`) | Missing fields use defaults. |
| Rotation as `[1,0,0,0]` (identity) | No rotation applied. |
| `name:` containing special characters | Stored as-is (no validation). |
| Scene file with unknown top-level keys (e.g. `metadata:`) | Warning logged, keys ignored. |
| `prefab:` with absolute path (`/home/user/file.yaml`) | Path resolution relative to `assets/` or cwd — determined by AssetManager base path. |
| Circular prefab reference (A uses B, B uses A) | Error (circular reference detected by visited-set in SceneLoader; scene load fails with error message). |
| Case-insensitive `.YAML`, `.Yml` extensions | Matched case-insensitively. |
| Scene file in current working directory vs `assets/` | Path resolution for the initial CLI scene file argument: try as-is first; if not found, resolve relative to assets base path. Prefab references in `prefab:` are always resolved relative to the assets base path (consistent with AssetManager). |
| Entity with both `model:` and `components:` | Components from the entity's own `components:` list are added to the container entity (the one with `model:`). The model's mesh nodes are expanded as child entities via `add_model_to_world()`. |
| Entity with both `model:` and `children:` | Model expansion entities AND explicit children are both created under the container entity. The model's meshes come first (from `add_model_to_world`), then explicit children from the `children:` block. |
| `model:` pointing to a non-existent asset | Asset lookup fails, error returned, scene load fails (consistent with other asset reference failures). |

## Error cases

| Case | Expected behavior |
|---|---|
| YAML parse error (malformed file) | Load fails with error `Error::Category::InvalidFormat`. App exits with code 1. |
| File not found | Load fails with error `Error::Category::IoFailed`. App exits with code 1. |
| Missing `type:` field | Load fails with error: "missing type field". |
| Unsupported `version:` | Load fails with error: "unsupported version". |
| Invalid `type:` (not "Scene" or "Prefab") | Load fails with error: "unknown type". |
| Unknown component type | Warning logged, component skipped. |
| Asset reference fails (e.g. model not found) | Deserialization error propagated; scene load fails. |
| Prefab file with >1 top-level entity | Load fails with error: "prefab must have exactly one root entity". |
| Missing `prefab:` target file | Load fails with error: prefab file not found. |
| Component deserialization fails (e.g. type mismatch) | Error propagated; scene load fails. |
| Component property validation fails (e.g. out of range) | Error propagated; scene load fails. |
| `buddd run` with a YAML path pointing to a directory | YAML load fails (yaml-cpp returns parse error or file open fails). |
| CLI: `buddd run unknown.yaml` (file doesn't exist) | Error: file not found, exit code 1. |

## Permissions and security

- The scene loader reads files only from the filesystem (no network access).
- All YAML files are assumed to be trusted (developer/artist-generated content).
- No elevated privileges are required.
- No secrets or credentials are consumed.
- Scene file paths are provided via CLI arguments — no external input injection.

## Observability

| Signal | Source |
|---|---|
| Scene loading started | `BUDDD_LOG_INFO("Loading scene: {}", path)` in `SceneLoader::load_from_file()`. |
| Scene loading complete | `BUDDD_LOG_INFO("Scene loaded: {} entities, {} components", ...)` after load. |
| Unknown component skipped | `BUDDD_LOG_WARN("Unknown component type '{}' skipped", type)` in `load_entity()`. |
| Unknown YAML key | `BUDDD_LOG_WARN("Unknown key '{}' skipped")` (via existing `ComponentInfo::deserialize`). |
| Asset resolution failure | Error propagated via `Result<>`, logged by caller. |
| Missing camera | `BUDDD_LOG_WARN("No camera in scene")` (existing `RenderSystem` behavior). |
| Scene app started | `BUDDD_LOG_INFO("Window opened: {}x{}")` (existing `run_app()`). |
| Scene app complete | `BUDDD_LOG_INFO("Scene complete: {} ({} frames rendered)")` (existing `run_app()`). |

## Out of scope

- Runtime scene hot-reload or file watching.
- Editor integration (panels, scene hierarchy view).
- YAML schema validation beyond yaml-cpp parsing.
- Recursive prefab nesting (prefab using another prefab).
- Component overrides on prefab instances.
- Multiple prefab root entities.
- `--scene` CLI flag (auto-detection only).
- Scene save/export (write YAML from World).
- Material or texture definitions in scene files.
- Physics, animation, or audio components.


## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `yaml-cpp` is already fetched and available as a dependency (used by existing component registry code). |
| A-02 | Path resolution for both prefab references (`prefab:`) and scene file fallback is relative to the assets base path (`assets/`), consistent with `AssetManager`. The initial CLI scene file argument is tried as-is first, then falls back to assets-relative resolution. |
| A-03 | The assets base path is `"assets"` (set in `EngineService::create()`). |
| A-04 | `FreeCameraMovement` fields (`move_speed`, `mouse_sensitivity`, etc.) are public and directly settable. |
| A-05 | The existing `ComponentInfo::deserialize()` already handles unknown-key warnings — shared with `SceneLoader`. |
| A-06 | `Entity::name()` returns a `const std::string&` — no validation on set. |
| A-07 | The `register_builtin_types()` call in `EngineService::create()` remains before `register_all_components()`. |
| A-08 | The existing `SceneApp`-less dispatch in `main.cpp` for unknown scenes prints "Unknown scene: 'foo'" and exits — this is unchanged when `.yaml` extension does not match. |
| A-09 | The `tests/CMakeLists.txt` glob pattern `tests/*_tests.cpp` will auto-discover `scene_loader_tests.cpp` — no CMake change needed. |

## Risks

| ID | Risk | Mitigation |
|---|---|---|
| R-01 | **Circular prefab references**: If prefab A references prefab B and B references A, `load_prefab()` would recurse infinitely without detection. | `SceneLoader` maintains a `std::unordered_set<std::string>` of paths currently being loaded. Before loading a prefab, the path is inserted; if already present, load fails with `Error::Category::InvalidFormat` and message: "circular prefab reference detected". |
| R-02 | **Path resolution dual-strategy**: Scene files are resolved via "try as-is, then assets/". Prefab references are resolved relative to the assets base path. If a scene file in a custom directory references `prefab:` with a relative path, the resolution may behave inconsistently. | In V1, both scene file paths and prefab reference paths use the assets base path as the default resolution root. The "try as-is" fallback applies only to the initial scene file argument from the CLI. This is documented in Assumptions A-02/A-03 and Edge cases. Future versions may introduce an explicit `base_path` parameter on `SceneLoader`. |
| R-03 | **FreeCameraMovement yaw/pitch not YAML-configurable**: The constructor takes `initial_yaw`/`initial_pitch` default parameters, but these are not exposed as ComponentRegistry properties. Camera always starts with yaw=0, pitch=0 regardless of prefab transform rotation. | Documented as a V1 limitation. The camera is user-controllable via mouse, so the initial orientation is a minor UX concern. Future iterations can add `initial_yaw`/`initial_pitch` as registry properties. |
| R-04 | **Entity name memory**: `std::string` per entity is heap-allocated (typically 24–32 bytes + allocation overhead on the heap). For average scenes (< 500 entities) this is negligible. | Acceptable for V1. For scenes with 10k+ entities, memory profiling should be performed and alternatives (small-string optimisation, interned names) may be considered. |

## Open questions

All questions are resolved. No `[NEEDS CLARIFICATION]` markers remain.
