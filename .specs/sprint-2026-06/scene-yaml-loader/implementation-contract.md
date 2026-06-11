# IMPL-2026-06-SCENE-YAML — Scene YAML Loader

## Source spec

- `.specs/sprint-2026-06/scene-yaml-loader/spec.md`

## Goal

Create a `SceneLoader` C++ class in `src/engine/scene/` that parses YAML scene and prefab files and populates a `World` with hierarchical entities, transforms, and serialized components. Add a `SceneApp` `App` subclass that loads a scene from a CLI-provided YAML path. Add CLI auto-detection so `buddd run <path.yaml>` routes to `SceneApp`. Add `Entity::name()`/`set_name()` support. Persist `ComponentRegistry` in `EngineService` with a `registry()` accessor. Register `FreeCameraMovement` as a typed component in `ComponentRegistry`. Provide at least 7 unit tests for `SceneLoader` and demo YAML files. This is engine-only V1, no editor changes.

## Non-goals

- No editor changes or panels.
- No `--scene` CLI flag — only auto-detection by file extension.
- No component overrides on prefab instances (V1 only supports transform overrides on root entity).
- No recursive prefab nesting (prefab using another prefab).
- No runtime scene reloading or hot-reload of YAML files.
- No YAML schema validation beyond what yaml-cpp provides.
- No changes to existing `App` subclasses, `App` base class, or `run_app()`.
- No changes to `AssetDemoApp` (already migrated).
- No changes to `src/cmd/app.h`, `src/cmd/app.cpp`, or `src/engine/engine_context.h`.
- No changes to the root `CMakeLists.txt`.
- No new CMake targets — `scene_loader.cpp` is globbed by the existing `buddd_engine` library.

## Relevant ADRs

| ADR | Relevance |
|---|---|
| ADR-014 (CLI App System) | SceneApp follows the same App lifecycle pattern. YAML auto-detection is a new dispatch in main.cpp before the existing scene dispatch. `run_app()` is unchanged. |
| ADR-028 (Component Type Registry) | SceneLoader uses `ComponentRegistry::create()`, `describe()`, and `deserialize_component()` to instantiate and populate components by string name from YAML. FreeCameraMovement registration uses overload (B) (simple lambdas, no SerializationContext). |
| ADR-019 (Architecture Boundaries) | SceneLoader lives in `src/engine/scene/` and uses engine abstractions only. No SDL3/OpenGL/GLM headers outside `src/engine/`. |
| ADR-001 (Result/Error Pattern) | All fallible APIs return `Result<T>` (`std::expected<T, Error>`). Errors use existing `Error::Category` values: `InvalidFormat`, `IoFailed`, `InvalidArgument`. |
| ADR-016 (yaml-cpp Dependency) | yaml-cpp is already a dependency. SceneLoader uses `YAML::LoadFile()` and `YAML::Node` directly. |
| ADR-012 (Navigable Object Graph) | SceneLoader accesses `EngineService::registry()` through `EngineContext`. SceneApp accesses `ctx.services.registry()` and `ctx.services.assets()`. |

## Files to inspect

| File | Reason |
|---|---|
| `src/engine/scene/world.h` | EntityNode struct definition — `name_` field goes here. Understand World API. |
| `src/engine/scene/entity.h` | Entity class — `name()` / `set_name()` methods go here. Pattern: `transform()` delegates to `world_->get_transform(id_)`. |
| `src/engine/scene/entity.cpp` | Entity implementation — existing getter pattern for add reference. |
| `src/engine/scene/world.cpp` | World implementation — understand `add_entity()`, `create_child()`, `get_transform()`, `get_parent()` patterns. |
| `src/engine/engine_service.h` | Add `registry_` member and `registry()` accessor after `assets()`. |
| `src/engine/engine_service.cpp` | Currently creates `ComponentRegistry` as local variable — must be changed to persistent member. |
| `src/engine/scene/component_registry/register_all_components.cpp` | FreeCameraMovement registration goes here alongside other light/camera registrations. |
| `src/engine/scene/component_registry/register_all_components.h` | Header — no change needed (free function signature unchanged). |
| `src/engine/scene/component_registry/component_registry.h` | ComponentRegistry API (`create()`, `describe()`, `register_component()`). |
| `src/engine/scene/component_registry/component_info.h` | ComponentInfo `add_property()` overloads (B) and (C). FreeCameraMovement uses overload (B). |
| `src/engine/scene/component_registry/serialization.h` | `deserialize_component()` free function — SceneLoader calls it. |
| `src/engine/scene/component_registry/serialization.cpp` | Implementation — wraps `info.deserialize()` with YAML exception catch. |
| `src/engine/scene/component_registry/serialization_context.h` | `SerializationContext{AssetManager&}` — SceneLoader constructs this when calling `deserialize_component()`. |
| `src/engine/scene/component_registry/type_registry.h` | TypeRegistry — SceneLoader may use for type queries. |
| `src/engine/scene/component_registry/property.h` | PropertyFlags — `PropertyFlags{}.min(0.0f)` pattern for min constraints. |
| `src/engine/scene/free_camera_movement.h` | Public fields: `move_speed`, `mouse_sensitivity`, `pitch_clamp_degrees`, `invert_yaw`, `invert_pitch`. Constructor takes `(initial_yaw, initial_pitch)`. |
| `src/engine/scene/camera_component.h` | Registration pattern reference — uses overload (B). |
| `src/engine/scene/transform.h` | Transform struct — `position`, `rotation`, `scale`. Default rotation is `Quat::identity()` (w,x,y,z). |
| `src/engine/asset/asset_manager.h` | AssetManager API — SceneLoader stores `AssetManager&`, constructs `SerializationContext{assets_}`. |
| `src/engine/error.h` | `Result<T>`, `Error::Category` enum (`IoFailed`, `InvalidFormat`, `InvalidArgument`), `make_error()` helpers. |
| `src/engine/engine_context.h` | `EngineContext{services, window, device, world, render_system, delta_time, frame}` — SceneApp uses. |
| `src/cmd/app.h` | App base class — SceneApp overrides `config()` and `setup()`. |
| `src/cmd/app.cpp` | `run_app()` creates `EngineService` which now has `registry()`. No change needed. |
| `src/cmd/app_config.h` | `RunningArgs`, `CaptureSpec`, `parse_running_args()`. |
| `src/cmd/main.cpp` | Add YAML detection branch before the "Unknown scene" error. |
| `src/cmd/apps/editor_app.h` | Reference for App subclass pattern. |
| `src/cmd/apps/run_app.h` | Reference for minimal App inline pattern. |
| `src/cmd/apps/asset_demo_app.h` | Reference for App with private Entity member. |
| `tests/CMakeLists.txt` | Already globs `*_tests.cpp` and links `buddd_engine`. No changes needed. |
| `tests/scene_graph_tests.cpp` | Test pattern reference (Catch2 `TEST_CASE`), World/Entity usage patterns. |
| `tests/component_registry_tests.cpp` | Test pattern for ComponentRegistry usage, `SerializationContext` construction. |

## Files allowed to change

- `src/engine/scene/world.h` — **modify**: add `std::string name_` to `EntityNode`, add `get_name(id)` / `set_name(id, name)` private helper declarations, add `add_component_raw()` public method declaration. If `#include <string>` not already present, add it for `std::string name_` field.
- `src/engine/scene/entity.h` — **modify**: add `name() -> const std::string&` and `set_name(const std::string&)` to `Entity` class.
- `src/engine/scene/entity.cpp` — **modify**: implement `Entity::name()` and `Entity::set_name()` delegating to `world_`.
- `src/engine/scene/world.cpp` — **modify**: implement `World::get_name()`, `World::set_name()`, and `World::add_component_raw()`.
- `src/engine/engine_service.h` — **modify**: add `std::unique_ptr<ComponentRegistry> registry_` member, `registry() -> ComponentRegistry&` accessor.
- `src/engine/engine_service.cpp` — **modify**: create `registry_` in `create()`, store persistently, call `register_all_components(*registry_)` on it.
- `src/engine/scene/component_registry/register_all_components.cpp` — **modify**: add `FreeCameraMovement` registration with 5 properties.
- `src/engine/scene/scene_loader.h` — **create**
- `src/engine/scene/scene_loader.cpp` — **create**
- `src/cmd/apps/scene_app.h` — **create**
- `src/cmd/apps/scene_app.cpp` — **create**
- `src/cmd/main.cpp` — **modify**: add YAML extension detection branch before "Unknown scene" error.
- `assets/scenes/demo.yaml` — **create**
- `assets/prefabs/free_camera.yaml` — **create**
- `src/engine/asset/asset_manager.h` — **modify**: add `resolve_model()` depth-first ModelNode traversal.
- `src/engine/asset/asset_manager.cpp` — **modify**: implement recursive `resolve_model()` with depth-first ModelNode tree traversal.
- `tests/scene_loader_tests.cpp` — **create**

## Files forbidden to change

- `src/cmd/app.h` — App interface invariant.
- `src/cmd/app.cpp` — `run_app()` invariant.
- `src/engine/engine_context.h` — EngineContext struct invariant (no `registry()` field — accessed via `ctx.services.registry()`).
- `src/engine/scene/component_registry/component_registry.h` — Public API invariant.
- `src/engine/scene/component_registry/component_info.h` — Public API invariant.
- `src/engine/scene/component_registry/serialization.h` / `.cpp` — No changes needed.
- `src/engine/scene/component_registry/property.h` — No changes needed.
- `src/engine/scene/component_registry/type_registry.h` — No changes needed.
- ~~`src/engine/asset/asset_manager.h` — No changes needed.~~ *(Now allowed to change — see Files allowed to change)*
- `src/engine/error.h` — No changes needed.
- `src/engine/scene/component.h` — No changes needed.
- `src/engine/scene/free_camera_movement.h` / `.cpp` — No changes (fields are public, constructors unchanged).
- `src/engine/scene/transform.h` — No changes.
- `src/cmd/app_config.h` / `.cpp` — No changes.
- `src/cmd/apps/asset_demo_app.*` — Already migrated, no changes.
- Any existing `App` subclass file (triangle, cube, phong, etc.).
- `tests/CMakeLists.txt` — Already globs `*_tests.cpp`, no changes needed.

## Existing conventions to follow

1. **Include style**: Use `#include "..."` for project headers, `<...>` for external/system headers. Paths are relative to `src/engine/` for engine headers, `src/cmd/` for cmd headers.
2. **Namespace nesting**: `buddd::engine` for engine code, `buddd::cmd::app` for SceneApp. Use `namespace` blocks without indentation of content (project style).
3. **`#pragma once`**: All new headers must use `#pragma once` as the include guard.
4. **`[[nodiscard]]`**: All `Result<T>`-returning functions must be marked `[[nodiscard]]`.
5. **App subclass pattern**: Header declares `class SceneApp final : public buddd::cmd::App`, implements `config()` and `setup()` virtual methods. See `run_app.h` for minimal pattern. Forward-declare engine types.
6. **Entity accessor pattern**: `Entity::name()` delegates to `world_->get_name(id_)` (same pattern as `transform()`). `Entity::set_name()` delegates to `world_->set_name(id_, name)`.
7. **Error construction**: Use `make_error(Error::Category::..., "message")` for returning `Result<void>` errors. See `src/engine/error.h`.
8. **Log macros**: `BUDDD_LOG_INFO`, `BUDDD_LOG_WARN`, `BUDDD_LOG_ERROR`, `BUDDD_LOG_TAGGED_WARN`. Log tag set via `BUDDD_LOG_TAG("SceneLoader")` in .cpp files.
9. **Test pattern**: Catch2 `TEST_CASE("name", "[tag]")` with `#include <catch2/catch_test_macros.hpp>` and `#include <catch2/catch_approx.hpp>`. See `tests/scene_graph_tests.cpp`.
10. **Entity construction in tests**: `World world; auto entity = world.add_entity();` pattern.
11. **Component registration pattern**: `info.add_property<float>(name, getter, setter, flags)` — overload (B) with simple lambdas. See `component_info.h` lines 117-134.
12. **EngineService member order**: Member destruction order is `platform_`, `window_`, `device_`, `asset_manager_` then `registry_` (registry_ has no dependency on asset_manager_ or device_, so it can be declared after asset_manager_).

## Required implementation behavior

### Step 1: Entity name — `world.h` + `entity.h` + `entity.cpp` + `world.cpp`

**`src/engine/scene/world.h`** — In the private `EntityNode` struct (line 103-111), add `std::string name_;` after `Transform transform_;`. Default is empty string (already handled by `std::string` default constructor). If `<string>` is not already included (check existing includes), add `#include <string>` at the top for `std::string name_`. Add two new private methods:
```cpp
auto get_name(EntityId id) const noexcept -> const std::string&;
auto set_name(EntityId id, const std::string& name) -> void;
```

**`src/engine/scene/entity.h`** — After the `// -- Transform --` section and before `// -- Components --`, add:
```cpp
// -- Name --
auto name() const noexcept -> const std::string&;
auto set_name(const std::string& name) -> void;
```

**`src/engine/scene/entity.cpp`** — Add implementations:
```cpp
auto Entity::name() const noexcept -> const std::string& {
    return world_->get_name(id_);
}
auto Entity::set_name(const std::string& name) -> void {
    world_->set_name(id_, name);
}
```

**`src/engine/scene/world.cpp`** — Add implementations:
```cpp
auto World::get_name(EntityId id) const noexcept -> const std::string& {
    auto* node = lookup_node(id);
    BUDDD_ASSERT(node != nullptr);
    return node->name_;
}
auto World::set_name(EntityId id, const std::string& name) -> void {
    auto* node = lookup_node(id);
    BUDDD_ASSERT(node != nullptr);
    node->name_ = name;
}
```

Note: `Entity::name()` returns `const std::string&` — must never return a reference to a temporary. The reference is valid as long as the EntityNode is alive (same lifetime contract as `get_transform()`).

### Step 2: EngineService + ComponentRegistry — `engine_service.h` + `engine_service.cpp`

**`src/engine/engine_service.h`**:
- Add `#include "scene/component_registry/component_registry.h"` (ComponentRegistry is a value-like class, no forward declaration needed, but its full definition is needed for `unique_ptr`).
- In the private section after `std::unique_ptr<AssetManager> asset_manager_;`, add `std::unique_ptr<ComponentRegistry> registry_;`.
- In the public section after `auto assets() noexcept -> AssetManager&;`, add `auto registry() noexcept -> ComponentRegistry&;`.

**`src/engine/engine_service.cpp`** — In `create()`:
Replace:
```cpp
    // Register all engine components
    auto registry = ComponentRegistry();
    register_all_components(registry);
```
With:
```cpp
    // Register all engine components
    engine->registry_ = std::make_unique<ComponentRegistry>();
    register_all_components(*engine->registry_);
```

Add accessor:
```cpp
auto EngineService::registry() noexcept -> ComponentRegistry& {
    BUDDD_ASSERT(registry_ != nullptr);
    return *registry_;
}
```

The member declaration order must be: `platform_`, `window_`, `device_`, `asset_manager_`, `registry_` (registry_ has no dependency on any of the above, so position at the end is safe).

### Step 3: FreeCameraMovement registration — `register_all_components.cpp`

Add a new block in `register_all_components()` after the MeshRenderer block and before the closing `}`:

```cpp
    // ── FreeCameraMovement: uses overload (B) — no SerializationContext needed ──
    {
        auto& info = registry.register_component<FreeCameraMovement>("free_camera_movement");

        info.add_property<float>("move_speed",
            [](const FreeCameraMovement& c) { return c.move_speed; },
            [](FreeCameraMovement& c, float v) -> Result<void> { c.move_speed = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );

        info.add_property<float>("mouse_sensitivity",
            [](const FreeCameraMovement& c) { return c.mouse_sensitivity; },
            [](FreeCameraMovement& c, float v) -> Result<void> { c.mouse_sensitivity = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );

        info.add_property<float>("pitch_clamp_degrees",
            [](const FreeCameraMovement& c) { return c.pitch_clamp_degrees; },
            [](FreeCameraMovement& c, float v) -> Result<void> { c.pitch_clamp_degrees = v; return {}; }
        );

        info.add_property<bool>("invert_yaw",
            [](const FreeCameraMovement& c) { return c.invert_yaw; },
            [](FreeCameraMovement& c, bool v) -> Result<void> { c.invert_yaw = v; return {}; }
        );

        info.add_property<bool>("invert_pitch",
            [](const FreeCameraMovement& c) { return c.invert_pitch; },
            [](FreeCameraMovement& c, bool v) -> Result<void> { c.invert_pitch = v; return {}; }
        );
    }
```

Add the include at the top of `register_all_components.cpp`:
```cpp
#include "scene/free_camera_movement.h"
```

### Step 4: SceneLoader — `scene_loader.h` + `scene_loader.cpp`

**`src/engine/scene/scene_loader.h`**:

```cpp
#pragma once

#include "error.h"
#include "scene/entity.h"

#include <string>
#include <unordered_set>
#include <yaml-cpp/yaml.h>

namespace buddd::engine {

class World;
class ComponentRegistry;
class AssetManager;

class SceneLoader {
public:
    SceneLoader(World& world, ComponentRegistry& registry, AssetManager& assets);

    /// Load a scene YAML file and populate the World.
    [[nodiscard]] auto load_from_file(const std::string& path) -> Result<void>;

    /// Load from an already-parsed YAML node.
    [[nodiscard]] auto load_from_yaml(const YAML::Node& node) -> Result<void>;

private:
    /// Load and return a single entity from a YAML node.
    /// If parent is not Entity::none(), the entity is created as a child of parent.
    [[nodiscard]] auto load_entity(const YAML::Node& node, Entity parent = Entity::none()) -> Result<Entity>;

    /// Load a prefab YAML file and return the root entity (for transform composition).
    [[nodiscard]] auto load_prefab(const std::string& path) -> Result<Entity>;

    /// Parse a transform block from a YAML node, applying defaults for missing fields.
    auto parse_transform(const YAML::Node& node) -> Transform;

    /// Compose prefab root transform with instance transform.
    auto compose_transform(const Transform& prefab, const Transform& instance) -> Transform;

    World& world_;
    ComponentRegistry& registry_;
    AssetManager& assets_;

    /// Set of prefab paths currently being loaded (cycle detection).
    std::unordered_set<std::string> loading_prefabs_;
};

} // namespace buddd::engine
```

**`src/engine/scene/scene_loader.cpp`** — Detailed implementation requirements:

**Constructor**: Store references. Initialize `loading_prefabs_` as empty.

**`load_from_file(path)`**:
1. Log `BUDDD_LOG_INFO("Loading scene: {}", path)`.
2. Attempt to open and parse the YAML file using `YAML::LoadFile(path)`. If `YAML::Exception` is thrown, catch it and return `make_error(Error::Category::InvalidFormat, "YAML parse error: " + std::string(e.what()))`.
3. Call `load_from_yaml(node)` with the parsed node.

**`load_from_yaml(node)`**:
1. Validate `node["type"]` exists and is a string. If missing, return error `"missing type field"`.
2. Validate `type` is `"Scene"` or `"Prefab"`. If neither, return error `"unsupported type: <type>"`.
3. Validate `node["version"]` exists and equals `1`. If missing, return error `"missing version field"`. If not 1, return error `"unsupported version"`.
4. If there is a `"metadata"` or other unknown top-level key, log a warning (for forward compatibility).
5. If `node["entities"]` does not exist or is not a sequence, no entities are loaded (success).
6. Iterate each element of `node["entities"]` and call `load_entity(entity_node, Entity::none())`.
7. After processing all entities, log `BUDDD_LOG_INFO("Scene loaded: {} entities", entity_count)`.
8. Return success.

**`load_entity(node, parent)`**: Returns a `Result<Entity>`.

1. **Prefab check**: If `node["prefab"]` exists:
   a. Resolve prefab path relative to assets base path. Get assets base path: `assets_.base_path()`. Prepend it to the prefab path (e.g. `prefabs/free_camera` → `"assets/prefabs/free_camera"`). If the prefab path doesn't have `.yaml`/`.yml` extension, try `.yaml` first, then `.yml`.
   b. Call `load_prefab(resolved_path)` which creates the prefab entities in the World and returns the root `Entity` handle.
   c. **The instance entity IS the prefab root entity** — do NOT call `world_.add_entity()`. Set `entity = prefab_root_entity`.
    d. If `parent` is not `Entity::none()`, reparent: `entity.reparent(parent)`.
    e. If `node["name"]` exists (as a scalar string), rename the prefab root entity: `entity.set_name(name)`.
    f. If `node["transform"]` exists, parse the instance transform via `parse_transform(node["transform"])` and compose with the prefab root's current transform: `entity.transform() = compose_transform(entity.transform(), instance_transform)`.
    g. If `node["model"]` exists:
       - Get the model asset path string from `node["model"].as<std::string>()`.
       - Call `assets_.create<ModelAsset>(model_path)` to load the ModelAsset.
       - If creation fails, propagate the error.
       - Get the root node: `auto& root = (*model_asset)->root_node();`.
       - Call `add_model_to_world(world_, root, entity)` to expand model hierarchy into child entities.
       - `add_model_to_world` is already inline in `src/engine/render/model_utils.h`.
       - Children created by `add_model_to_world` each have Transform + MeshRenderer components.
    h. Apply components (step 3) if `node["components"]` exists — these are added ON TOP of the prefab's existing components.
    i. Apply children (step 4) if `node["children"]` exists — these become children of the prefab root entity.
    j. Return `entity`.

2. **Prefab check**: If `node["prefab"]` does NOT exist (direct entity):
    a. Create entity via `world_.add_entity()`. If `parent` is not `Entity::none()`, reparent: `entity.reparent(parent)`.
    b. If `node["name"]` exists (as a scalar string), call `entity.set_name(name)`.
    c. If `node["transform"]` exists, call `parse_transform(node["transform"])` and set the entity's transform fields.
    d. If `node["model"]` exists:
       - Get the model asset path string from `node["model"].as<std::string>()`.
       - Call `assets_.create<ModelAsset>(model_path)` to load the ModelAsset.
       - If creation fails, propagate the error.
       - Get the root node: `auto& root = (*model_asset)->root_node();`.
       - Call `add_model_to_world(world_, root, entity)` to expand model hierarchy into child entities.
       - `add_model_to_world` is already inline in `src/engine/render/model_utils.h`.
       - Children created by `add_model_to_world` each have Transform + MeshRenderer components.
    e. Apply components (step 3) if `node["components"]` exists.
    f. Apply children (step 4) if `node["children"]` exists.
    g. Return `entity`.

3. **Component processing** (shared logic for both branches):
   a. For each component entry in the sequence, read `type` field (string).
   b. Call `registry_.describe(type)`. If null (unknown type), log `BUDDD_LOG_WARN("Unknown component type '{}' skipped", type)` and continue.
   c. Call `registry_.create(type)` to create a default component instance.
   d. If the component entry has a `properties:` map:
      - Construct `SerializationContext{assets_}`.
      - Get the `ComponentInfoBase*` via `registry_.describe(type)`.
      - Call `deserialize_component(*info, properties_node, *component_instance, ctx)`. If deserialization fails, propagate the error.
   e. Move the component into the entity via `world_.add_component_raw(entity.id(), std::move(component_instance))`. This calls `on_attach()` and registers `Updatable` subclasses.
   f. ⚠ **The spec says "component deserialization failure → error; scene load fails."** Do NOT silently skip components that fail deserialization. Only unknown component types are skipped (with a warning). A known component whose deserialization fails returns a hard error.

4. **Children processing** (shared logic for both branches):
   a. If `node["children"]` exists and is a sequence:
      - For each child node, call `load_entity(child_node, entity)` recursively, creating the child under the current entity.

**`load_prefab(path)`**:
1. Check `loading_prefabs_` for `path`. If already present, return `make_error(Error::Category::InvalidFormat, "circular prefab reference detected: " + path)`.
2. Insert `path` into `loading_prefabs_`.
3. Open and parse the prefab YAML file (same error handling as `load_from_file` for YAML parse errors).
4. Validate `type` is `"Prefab"`. If not, remove from `loading_prefabs_` and return error "prefab file has invalid type".
5. Validate `version` is `1`. If not, remove from `loading_prefabs_` and return error.
6. Validate `entities:` exists and is a sequence with exactly one element. If zero elements: return error "prefab must have at least one entity". If >1 element: return error "prefab must have exactly one root entity".
7. Call `load_entity(entities[0], Entity::none())` to create the prefab's entities.
8. Remove `path` from `loading_prefabs_`.
9. Return the root entity.

**`parse_transform(node)`**:
1. Start with `Transform{}` (default: position=0, rotation=identity, scale=1).
2. If `node["position"]` exists, parse as `YAML::Node` sequence of 3 floats `[x, y, z]` and set `transform.position`.
3. If `node["rotation"]` exists, parse as sequence of 4 floats `[w, x, y, z]` and set `transform.rotation`. The spec says rotation default is `[1,0,0,0]` (w,x,y,z identity quaternion).
4. If `node["scale"]` exists, parse as sequence of 3 floats `[x, y, z]` and set `transform.scale`.
5. Return the transform.

Parse helper pattern (for parsing float arrays from YAML node):
```cpp
static auto parse_vec3(const YAML::Node& node) -> Result<math::Vec3> {
    if (!node.IsSequence() || node.size() != 3) {
        return make_error(Error::Category::InvalidFormat, "expected sequence of 3 floats");
    }
    return math::Vec3{node[0].as<float>(), node[1].as<float>(), node[2].as<float>()};
}
```

**`compose_transform(prefab, instance)`**:
1. `result.position = prefab.position + instance.position;`
2. `result.scale = math::Vec3{prefab.scale.x * instance.scale.x, prefab.scale.y * instance.scale.y, prefab.scale.z * instance.scale.z};`
3. `result.rotation = prefab.rotation * instance.rotation;` (quaternion multiplication — order: prefab then instance)

**Component attachment** (since we cannot use template `Entity::add_component<T>()` for runtime types):
- Get the `EntityNode*` via internal lookup (or use a low-level helper method on World).
- Calling `on_attach()` and setting `world_`/`entity_id_` on the component is required for the component to function correctly (e.g., `CameraComponent::on_attach()` registers the camera with the World).
- The World class doesn't expose a public `add_component_raw()` — but since SceneLoader is in the same namespace and has access to `world_`, and World has `friend class Entity;`, we need an alternative. The cleanest approach is to add a public method to `World` or make SceneLoader own the component attachment logic via the existing `add_component` template. Since we know the type at compile time for typed components... but SceneLoader doesn't. We need a runtime dispatch.
- **Recommended approach**: Add a public method to `World` for raw component injection:
  ```cpp
  // In world.h public section:
  auto add_component_raw(EntityId id, std::unique_ptr<Component> component) -> Component&;
  ```
  This method does: set `component->world_ = this`, `component->entity_id_ = id`, store in `node->components_`, call `component->on_attach()`, and return reference.
  This is necessary for runtime-typed component creation from SceneLoader.

### Step 5: World helper for raw component injection — `world.h` + `world.cpp`

**`src/engine/scene/world.h`** — In public section, add:
```cpp
/// Attach a runtime-typed component to an entity (for SceneLoader / deserialization).
/// The component is moved into the World's storage.
auto add_component_raw(EntityId id, std::unique_ptr<Component> component) -> Component&;
```

**`src/engine/scene/world.cpp`** — Add implementation:
```cpp
auto World::add_component_raw(EntityId id, std::unique_ptr<Component> component) -> Component& {
    auto* node = lookup_node(id);
    BUDDD_ASSERT(node != nullptr && !node->pending_destroy_);
    Component* ptr = component.get();
    ptr->world_ = this;
    ptr->entity_id_ = id;
    node->components_.push_back(std::move(component));
    ptr->on_attach();

    // Auto-register Updatable components
    if (auto* upd = dynamic_cast<Updatable*>(ptr)) {
        updatables_.push_back(upd);
    }

    return *ptr;
}
```

This method is NOT in the original spec but is required for the implementation to work. The spec's existing `ComponentRegistry::create()` returns `unique_ptr<Component>`, and SceneLoader needs a way to inject it into the World's internal storage.

### Step 6: SceneApp — `scene_app.h` + `scene_app.cpp`

**`src/cmd/apps/scene_app.h`**:
```cpp
#pragma once

#include "app.h"

#include <string>

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

**`src/cmd/apps/scene_app.cpp`**:
```cpp
#include "apps/scene_app.h"

#include "engine_context.h"
#include "engine_service.h"
#include "error.h"
#include "scene/scene_loader.h"
#include "scene/component_registry/component_registry.h"
#include "scene/world.h"

#include <filesystem>

namespace buddd::cmd::app {

SceneApp::SceneApp(std::string scene_path)
    : scene_path_(std::move(scene_path)) {}

auto SceneApp::config() const -> AppConfig {
    auto stem = std::filesystem::path(scene_path_).stem().string();
    return {stem, 1024, 768};
}

auto SceneApp::setup(buddd::engine::EngineContext const& ctx)
    -> buddd::engine::Result<void>
{
    buddd::engine::SceneLoader loader(
        ctx.world, ctx.services.registry(), ctx.services.assets());
    return loader.load_from_file(scene_path_);
}

} // namespace buddd::cmd::app
```

### Step 7: CLI dispatch — `main.cpp`

In `src/cmd/main.cpp`, inside the `"run"` command branch, after `std::string_view scene{argv[2]};` is assigned and before the named scene if/else-if chain begins, restructure the dispatch so the YAML file check is the **first** `else if` branch. The chain becomes:

```cpp
        // Priority 1: YAML scene file auto-detection (check BEFORE named scenes)
        auto is_yaml_file = [](std::string_view path) -> bool {
            auto pos = path.rfind('.');
            if (pos == std::string_view::npos || pos == path.size() - 1)
                return false;
            std::string_view ext = path.substr(pos + 1);
            // Normalise to lowercase
            std::string lower_ext;
            for (auto c : ext)
                lower_ext.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            return (lower_ext == "yaml" || lower_ext == "yml");
        };

        if (is_yaml_file(scene)) {
            if (std::filesystem::exists(scene)) {
                app = std::make_unique<bc::app::SceneApp>(std::string(scene));
            } else {
                BUDDD_LOG_ERROR("Scene file not found: '{}'", argv[2]);
                return EXIT_FAILURE;
            }
        }
        else if (scene == "triangle") {
            app = std::make_unique<app::TriangleApp>();
        }
        // ... (existing named scene else-if chain continues unchanged) ...
        else {
            BUDDD_LOG_ERROR("Unknown scene: '{}'", argv[2]);
            ...
        }
```

Key requirements:
- The YAML check is the **first** `else if` — takes priority over named scenes per spec.
- If `scene` ends with `.yaml`/`.yml` (case-insensitive) AND the file exists → create `SceneApp`.
- If `scene` ends with `.yaml`/`.yml` but the file does NOT exist → print error + `return EXIT_FAILURE` immediately (do NOT fall through to "Unknown scene").
- If `scene` does NOT end with `.yaml`/`.yml` → fall through to the existing named scene dispatch chain unchanged.
- The lambda `is_yaml_file` can be defined inline or as a free helper. Case-insensitive comparison is required (handles `.YAML`, `.Yml`, etc.).
- The `scene` variable (`std::string_view scene{argv[2]}`) is already assigned before this chain and can be used directly for both the YAML check and the named scene checks.

Add `#include "apps/scene_app.h"` and `#include <filesystem>` to the includes block in main.cpp.

### Step 8: Demo YAML files

**`assets/scenes/demo.yaml`**:
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
          color: [1.0, 1.0, 1.0]
          intensity: 1.0
    transform:
      rotation: [0.707, -0.707, 0.0, 0.0]
```

**`assets/prefabs/free_camera.yaml`**:
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

### Step 9: AssetManager — fix `resolve_model()` to traverse ModelNode tree

**`src/engine/asset/asset_manager.h`** and **`src/engine/asset/asset_manager.cpp`**:

The current `resolve_model()` (in `asset_manager.cpp` lines 120-135) only checks the root `ModelNode` for a `model` field. A glTF file may have multiple meshes across multiple nodes in the tree. Fix it to traverse depth-first and return the first `Model` found:

```cpp
// Replacement for the existing resolve_model() in asset_manager.cpp
auto AssetManager::resolve_model(const std::string& id) -> Result<std::shared_ptr<Model>> {
    auto asset_result = create<ModelAsset>(id);
    if (!asset_result) {
        return make_error(Error::Category::InvalidArgument,
            "Failed to resolve model asset '" + id + "': " + asset_result.error().message);
    }
    auto& root = (*asset_result)->root_node();

    // Depth-first traversal to find first node with a model
    // Lambda must be std::function because it captures itself recursively
    std::function<auto(const ModelNode&)->std::optional<std::shared_ptr<Model>>> find_first_model;
    find_first_model = [&](const ModelNode& node) -> std::optional<std::shared_ptr<Model>> {
        if (node.model.has_value()) {
            return std::make_shared<Model>(std::move(*const_cast<ModelNode&>(node).model));
        }
        for (const auto& child : node.children) {
            auto result = find_first_model(child);
            if (result.has_value()) return result;
        }
        return std::nullopt;
    };

    auto found = find_first_model(root);
    if (!found.has_value()) {
        return make_error(Error::Category::InvalidArgument,
            "Model asset '" + id + "' has no model in any node");
    }
    return std::move(*found);
}
```

Key requirements:
- Traverse the `ModelNode` tree depth-first using a recursive lambda.
- If a node has `model.has_value()`, move the Model out of the node into a `shared_ptr<Model>` and return it.
- If no node in the entire tree has a model, return an error: `"Model asset '<id>' has no model in any node"`.
- The `#include <functional>` header is already included transitively via existing includes in `asset_manager.cpp` — no new include needed.
- The `ModelNode` struct is already available via `#include "asset/model_asset.h"` in `asset_manager.h`.

### Step 10: Path resolution logic in SceneLoader

**Scene file path** (from CLI): The path is passed as-is to `load_from_file()`. `YAML::LoadFile()` will use it directly. If the file doesn't exist, `YAML::LoadFile` throws `YAML::BadFile` which is caught and returned as `Error::Category::IoFailed`.

**Prefab path** (from `prefab:`): The path in `prefab:` is relative to the assets base path. The resolution logic:
1. Get assets base path: `assets_.base_path()` (e.g., `"assets"`).
2. Get the prefab path string.
3. Construct `std::string full_path = std::string(assets_.base_path()) + "/" + prefab_path;`
4. If `full_path` doesn't have a `.yaml`/`.yml` extension, try appending `.yaml` first, then `.yml`.
5. The ambiguity between "prefabs/free_camera" and "prefabs/free_camera.yaml" is resolved by checking existence: try `full_path + ".yaml"`, if exists use it; else try `full_path + ".yml"`, if exists use it; else return error "prefab file not found".

## Required tests

### Unit tests — `tests/scene_loader_tests.cpp`

All tests must be tagged `[scene_loader]`. Separate from visual demo tests. Tests use `load_from_yaml()` to avoid filesystem dependency where possible. Use `World`, a `ComponentRegistry` with `register_builtin_types()` and `register_all_components()` called, and a `SerializationContext` with a stub `AssetManager` reference.

Follow the `TestEngine` pattern from `tests/component_registry_tests.cpp` (create a headless `EngineService` which already registers all types and components):

```cpp
#include "scene/scene_loader.h"
#include "scene/world.h"
#include "scene/entity.h"
#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/register_all_components.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/component_registry/serialization.h"
#include "scene/camera_component.h"
#include "engine_service.h"
#include "error.h"

#include <yaml-cpp/yaml.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

using namespace buddd::engine;
using Catch::Approx;

// Test engine with ComponentRegistry set up
struct TestEnv {
    std::unique_ptr<EngineService> engine;
    World world;

    TestEnv()
        : engine(EngineService::create(
              Backend::Headless,
              WindowConfig{"Test", 800, 600}).value())
    {}
};
```

**Test 1 — Minimal scene loads one entity** (AC-003, AC-006):
- Create a minimal YAML node with `type: Scene, version: 1, entities: [{name: test}]` (no transform, no components).
- Create `SceneLoader(world, engine->registry(), engine->assets());`
- Call `loader.load_from_yaml(node);`
- Verify the entity exists in the world (entity count > 0).
- Verify `entity.name() == "test"`.
- Verify entity's transform has default values: position=[0,0,0], rotation=identity, scale=[1,1,1].

**Test 2 — Entity name set via YAML** (AC-004, AC-022):
- Create entity without `name:` in YAML, verify `Entity::name()` returns `""` (empty string).
- Create entity with `name: foo`, verify `entity.name() == "foo"`.

**Test 3 — Transform parsing** (AC-006):
- Load entity with `transform: {position: [1,2,3], rotation: [0.707, 0.0, 0.707, 0.0], scale: [2,2,2]}`.
- Verify position, rotation, scale match expected values.

**Test 4 — Component deserialization** (AC-007):
- Load entity with `components: [{type: camera, properties: {fov_y: 1.0, aspect: 1.5, near: 0.01, far: 200.0}}]`.
- Verify the entity has a CameraComponent via `entity.get_component<CameraComponent>()`.
- Verify `camera_component.fov_y() == Approx(1.0)`.

**Test 5 — Unknown component skipped with warning** (AC-008):
- Load entity with one known component (`camera`) and one unknown component (`future_component`).
- Verify the camera component exists.
- Verify the unknown component does not exist (only camera).

**Test 6 — Unknown YAML keys produce warning, not error** (AC-009):
- Load entity with extra unknown key like `unknown_field: hello`.
- Verify load succeeds (no error returned).

**Test 7 — Prefab transform composition via `compose_transform()`** (AC-005):
- The `compose_transform()` method (position add, scale mul, rotation quat-mul) is the core of AC-005.
- Since full prefab loading requires filesystem I/O, test the composition logic directly:
- Make `compose_transform()` either a `public static` method on `SceneLoader` or a free function in a `detail` namespace (exposed to tests via `scene_loader_private.h` or inline in header).
- Test: call `compose_transform(prefab, instance)` where:
  - prefab = `{position: [3,2,3], rotation: identity, scale: [1,1,1]}`
  - instance = `{position: [1,2,3], rotation: identity, scale: [2,2,2]}`
- Verify: `result.position == [4,4,6]`, `result.scale == [2,2,2]`, `result.rotation == identity`.

**Test 8 — Children hierarchy** (AC-023):
- Load scene YAML with root entity that has `children: [{name: child1}]`.
- Verify child entity exists and `child.parent() == root_entity`.
- Verify child's name is `"child1"`.

**Test 9 — Entity with `model:` directive creates child entities** (AC-024):
- Create a scene YAML with an entity that has `model: models/box/Box`.
- Call `loader.load_from_yaml(node);`.
- Verify the entity exists (entity count > 0).
- Verify child entities exist under the container entity (children created by `add_model_to_world`).
- Verify child entities have `MeshRenderer` components (via `entity.get_component<MeshRenderer>()` or `each<MeshRenderer>`).
- Note: This test requires that a `MeshRenderer` component registration exists and that `ModelAsset` creation succeeds with a valid model asset. If `models/box/Box` is not loadable in a headless test environment, use a YAML string that references an alternative model asset available in tests, or programmatically create a `ModelAsset` with a known `ModelNode` tree.

**Test 10 — `resolve_model()` traverses ModelNode tree depth-first** (AC-025):
- Programmatically create a `ModelAsset` with a multi-node tree where only a leaf node has a Model, and the root node does not.
- Call `asset_manager.resolve_model(asset_id)` directly.
- Verify it returns the leaf node's Model (not an error).
- Create a `ModelAsset` with no mesh nodes in the entire tree.
- Call `resolve_model()` and verify it returns an error.

At least 7 of the above tests must be implemented. Recommended set: Tests 1, 2, 3, 4, 5, 6, 8 (covering AC-003, AC-004, AC-006, AC-007, AC-008, AC-009, AC-022, AC-023). If feasible, include Tests 9 and 10 for full AC-024 and AC-025 coverage.

**AC-010 and AC-011 coverage**: These acceptance criteria (missing prefab file → hard error; prefab with >1 root entity → error) are covered by `load_prefab()` validation logic. While they require filesystem I/O or programmatic prefab YAML construction, the implementation must test them at minimum via:
- **AC-010**: Programmatic test that calls `load_prefab()` with a non-existent path and verifies error is returned. If `load_prefab` is private, test through a public helper or via `load_from_yaml()` that references a nonexistent prefab (requires a temporary file or error injection).
- **AC-011**: Programmatic test that creates a prefab YAML node with 2 root entities and verifies `load_prefab()` returns an error. If tested through `load_from_yaml()`, create a scene with `prefab:` pointing to an inline prefab node (string-based YAML) — but this requires refactoring. **Simplest approach**: Test `load_prefab()`'s validation by directly invoking it if made accessible, or add a dedicated test file that creates a multi-entity prefab on disk in a temp directory.

### E2E / Integration verification

| Method | What it verifies |
|---|---|
| **Visual demo (display)** | `buddd run assets/scenes/demo.yaml --frame 120 --capture 120:out.png` → verify `out.png` shows a rendered Box model with directional light. |
| **Manual CLI test** | `buddd run nonexistent.yaml` → prints error and exits with code 1. |
| **Manual CLI test** | `buddd run demo.txt` → shows "Unknown scene: 'demo.txt'" error (unchanged behavior). |
| **Build verification** | `cmake --build --preset debug` succeeds (no compile/link errors). |
| **Existing scenes** | `buddd run triangle --frame 1` and other scenes continue to work without regression. |
| **All unit tests** | `ctest --preset debug` passes. |

## Edge cases

| Case | Expected behavior | Handled by |
|---|---|---|
| Empty `entities:` array | Scene loads successfully with zero entities. | `load_from_yaml()`: check for missing/empty entities sequence. |
| Entity with neither `name:` nor `prefab:` | Entity created with empty name. | Default `EntityNode::name_` is `""`. |
| Entity with `prefab:` but no prefab file | Error returned; scene load fails. | `load_prefab()`: file not found → `IoFailed` error. |
| Prefab with zero entities | Error: "prefab must have at least one entity". | `load_prefab()` validation. |
| Prefab with >1 top-level entity | Error: "prefab must have exactly one root entity". | `load_prefab()` validation. |
| Transform with partial fields (e.g. only `position:`) | Missing fields use defaults. | `parse_transform()` uses `Transform{}` defaults. |
| Unknown top-level keys in scene file | Warning logged, keys ignored. | `load_from_yaml()` forward-compat check. |
| Circular prefab reference (A uses B, B uses A) | Error detected by visited-set; scene load fails. | `loading_prefabs_` check in `load_prefab()`. |
| Case-insensitive `.YAML`, `.Yml` extensions | Matched case-insensitively. | CLI detection uses `std::tolower()`. |
| Scene file in cwd vs assets/ | Path as-is to YAML::LoadFile. If file doesn't exist, error. | `YAML::LoadFile` throws `YAML::BadFile` → caught → `IoFailed`. |
| Unknown component type | Warning logged, component skipped, other components loaded. | `load_entity()`: `registry_.describe()` returns null → skip. |
| Asset reference fails (e.g. model not found) | Deserialization error propagated; scene load fails. | `deserialize_component()` returns error → propagated. |
| YAML parse error (malformed file) | Error returned; scene load fails. | `YAML::LoadFile()` throws `YAML::Exception` → `InvalidFormat` error. |
| Component deserialization fails | Error returned; scene load fails. | `deserialize_component()` returns error → propagated. |
| Component property validation fails | Error returned; scene load fails. | `Property::deserialize()` validates via PropertyFlags → error. |
| Entity with both `prefab:` and `components:` | Prefab entities + entity's own components are both present. | `load_entity()` processes prefab first, then components. |
| No camera component in scene | `BUDDD_LOG_WARN` emitted by existing `RenderSystem`. | Existing behavior, no change needed. |
| `name:` containing special characters | Stored as-is (no validation). | `Entity::set_name()` stores the string. |
| Entity with `model:` directive | ModelAsset loaded, add_model_to_world creates child entities with Transform + MeshRenderer. | `load_entity()` model: handling step. |
| Entity with both `model:` and `components:` | Components from `components:` go on the container entity; model meshes are child entities. | `load_entity()` processes model first, then components. |
| Entity with both `model:` and `children:` | Model expansion entities come first, then explicit children. | `load_entity()` processes model before children. |
| `model:` pointing to non-existent asset | Asset lookup fails, error returned, scene load fails. | `assets_.create<ModelAsset>()` failure → propagated error. |
| `resolve_model()` with nil ModelNode tree | Error: "Model asset '<id>' has no model in any node". | Depth-first traversal returns nullopt → error. |

## Security impact

None. Scene files are loaded from the local filesystem only. No network access. All YAML files are trusted developer/artist-created content. No elevated privileges required.

## Data and migration impact

None. No schema changes to existing data. No migrations required. The new `name_` field on `EntityNode` has a default empty string, so existing code that creates entities without `set_name()` is unaffected. The `ComponentRegistry` was previously a local variable in `EngineService::create()` and is now a persistent member — behavior is identical.

## API compatibility impact

- **`Entity` class**: Adds `name()` and `set_name()`. No existing API changes. Backward compatible.
- **`EngineService` class**: Adds `registry()` accessor. No existing API changes. Backward compatible.
- **`World` class**: Adds `add_component_raw()`. Existing template-based `add_component<T>()` unchanged. Backward compatible.
- **`ComponentRegistry` usage**: Previously created as local variable in `EngineService::create()`, now accessed via `engine->registry()`. All existing `register_all_components()` callers (there's only one) work unchanged.
- **`register_all_components()` signature**: Unchanged — still takes `ComponentRegistry&`.
- **`App` base class**: Unchanged. `SceneApp` is a new subclass.
- **CLI**: No existing commands changed. YAML file paths are silently routed to SceneApp before the error path. If a scene were named `"scene.yaml"` and was intended to be an existing named scene, it would now be treated as a file — this is acceptable per the spec (auto-detection by extension takes priority).

## Documentation impact

- **README**: None. No README changes needed.
- **Wiki pages to update**: The wiki-agent handles these, but the changes expected are:
  - `docs/wiki/architecture/module-map.md`: Add SceneLoader class in scene/ submodule. Add SceneApp in CLI apps section. Document `Entity::name()`/`set_name()`. Document `EngineService::registry()` accessor.
  - `docs/wiki/architecture/overview.md`: Update directory layout to include `src/engine/scene/` for SceneLoader. Update CMake targets if needed (SceneLoader is auto-globbed by engine CMakeLists.txt).
  - `docs/wiki/domain/business-rules.md`: Document CLI command behavior for YAML auto-detection (`buddd run <path.yaml>`). Document entity naming conventions.
  - `docs/wiki/architecture/dependency-map.md`: Add `scene_loader` → `AssetManager` and `scene_loader` → `ComponentRegistry` dependencies.
- **Other specs**: None.

## ADR impact

- No ADR updates are required. ADR-014 already describes the CLI extension mechanism and is compatible with `.yaml`/`.yml` auto-detection. ADR-028 was written with SceneLoader in mind. ADR-016 already established yaml-cpp as a dependency.
- No new ADR is needed for this implementation.

## Done criteria

The implementation is complete when ALL of the following are verifiable:

- [ ] **AC-001**: `src/engine/scene/scene_loader.h` exists, declares `class SceneLoader` in `namespace buddd::engine` with `load_from_file(path) -> Result<void>` and `load_from_yaml(node) -> Result<void>`.
- [ ] **AC-002**: `SceneLoader` stores references to `World&`, `ComponentRegistry&`, `AssetManager&` (inspect header member declarations).
- [ ] **AC-003**: `SceneLoader::load_from_yaml()` parses a YAML node, validates `type` and `version` fields, creates entities in the `World`. Unit test: load minimal scene, verify entity exists.
- [ ] **AC-004**: Entities created by SceneLoader have their `name` property set via `Entity::set_name()`. Unit test: load entity with `name: foo`, verify `entity.name() == "foo"`.
- [ ] **AC-005**: `prefab:` loads prefab entities and composes transforms (position additive, scale multiplicative, rotation quaternion multiplication). Unit test: compose with known values.
- [ ] **AC-006**: Transform fields are optional with defaults: position `[0,0,0]`, rotation `[1,0,0,0]`, scale `[1,1,1]`. Unit test: entity without transform has default values.
- [ ] **AC-007**: Components from `components:` are created via `ComponentRegistry::create()` and deserialized. Unit test: load entity with `camera` component, verify `fov_y` property.
- [ ] **AC-008**: Unknown component types are skipped with a warning. Unit test: load entity with one known and one unknown component; verify known exists and unknown is skipped.
- [ ] **AC-009**: Unknown YAML keys produce a warning (not an error). Unit test: load entity with extra unknown key; verify load succeeds.
- [ ] **AC-010**: Missing prefab file causes a hard error. Unit test (if filesystem test is feasible): `load_from_file` with `prefab: nonexistent` returns error.
- [ ] **AC-011**: Prefab with >1 top-level entity returns error. Unit test: create prefab YAML with 2 root entities, verify error.
- [ ] **AC-012**: `src/cmd/apps/scene_app.h` exists and declares `class SceneApp` inheriting `App`.
- [ ] **AC-013**: `SceneApp::config()` returns `AppConfig` with title derived from scene file base name, width 1024, height 768.
- [ ] **AC-014**: `SceneApp::setup()` creates a `SceneLoader` and calls `load_from_file(scene_path_)`.
- [ ] **AC-015**: `src/cmd/main.cpp` routes positional args ending in `.yaml`/`.yml` (case-insensitive) to `SceneApp`.
- [ ] **AC-016**: `EngineService` stores `std::unique_ptr<ComponentRegistry>` and exposes `registry() -> ComponentRegistry&`.
- [ ] **AC-017**: `ComponentRegistry` is created during `EngineService::create()` and persists for the lifetime of `EngineService`.
- [ ] **AC-018**: `FreeCameraMovement` is registered as `free_camera_movement` with 5 properties: `move_speed` (float, min 0), `mouse_sensitivity` (float, min 0), `pitch_clamp_degrees` (float), `invert_yaw` (bool), `invert_pitch` (bool).
- [ ] **AC-019**: `World::EntityNode` has `std::string name_` member and `Entity::name()`/`set_name()` accessors.
- [ ] **AC-020**: `assets/scenes/demo.yaml` exists with 4 entities: camera (free_camera prefab), box_via_directive (`model:` directive), box_via_component (mesh_renderer with `models/box/Box`), light (directional_light).
- [ ] **AC-021**: `assets/prefabs/free_camera.yaml` exists with camera + free_camera_movement components.
- [ ] **AC-022**: `Entity::name()` defaults to empty string for entities created without `set_name()`. Unit test: create entity via `World::add_entity()`, verify `name() == ""`.
- [ ] **AC-023**: Entities with `children:` in YAML have children created as child entities in the World hierarchy. Unit test: load entity with `children: [{name: child1}]`, verify child entity exists and `entity.parent() == parent_entity`.
- [ ] **AC-024**: Entity with `model:` directive loads a `ModelAsset`, calls `add_model_to_world()`, and creates child entities with `MeshRenderer` components for each mesh node. Unit test: load entity with `model: models/box/Box`, verify children exist and have `MeshRenderer`.
- [ ] **AC-025**: `resolve_model()` traverses the `ModelNode` tree depth-first and returns the first node with a Model, or returns an error if no node has a Model. Unit test: multi-node tree finds first model; tree with no mesh nodes returns error.
- [ ] **CUSTOM-001**: `World::add_component_raw(EntityId, unique_ptr<Component>)` exists for runtime-type component injection.
- [ ] **CUSTOM-002**: Circular prefab references are detected via `loading_prefabs_` visited-set and return an error.
- [ ] **TEST-001**: `tests/scene_loader_tests.cpp` exists with at least 7 unit tests tagged `[scene_loader]`.
- [ ] **TEST-002**: All scene_loader tests pass: `ctest --preset debug`.
- [ ] **TEST-003**: All existing tests continue to pass: `ctest --preset debug`.
- [ ] **TEST-004**: Build succeeds: `cmake --build --preset debug`.
- [ ] **TEST-005**: `buddd run triangle --frame 1` continues to work (no regression on existing scenes).
