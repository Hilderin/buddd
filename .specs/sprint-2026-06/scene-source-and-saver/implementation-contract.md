# IMPL-2026-06-SCENE-SOURCE-SAVER — Scene Source Tracking and Saver

## Source spec

- `.specs/sprint-2026-06/scene-source-and-saver/spec.md`

## Goal

Add `EntitySourceType`/`EntitySource` tracking to every entity so the engine knows whether an entity was created from a prefab, a model directive, or directly (`None`). Then create a `SceneSaver` class symmetric to `SceneLoader` that serializes the `World` back to YAML, emitting `prefab:` and `model:` references where applicable instead of expanded entity trees. This enables compact, editable saved scene files and round-trip fidelity.

## Non-goals

- No editor integration (NG-01).
- No saving entity `type:` or `version:` metadata from World — SceneSaver always writes `type: Scene` and `version: 1` (NG-02).
- No partial component overrides on prefab or model entities — they are immutable outside of transform (NG-03).
- No YAML schema validation beyond what yaml-cpp provides (NG-04).
- No recursive scene file saving (e.g., writing back to prefab files) (NG-05).
- No changes to `add_model_to_world()` — model child entities get source `None` (NG-06).
- No runtime scene hot-reload or file watching (NG-07).
- No changes to `SceneLoader`'s existing error handling or transform composition logic (NG-08).

## Relevant ADRs

| ADR | Relevance |
|---|---|
| ADR-028 (Component Type Registry) | SceneSaver uses `ComponentRegistry::all_types()`, `ComponentInfoBase::serialize()`, and `serialize_component()` to serialize component data. The reverse-lookup mechanism (Component* → ComponentInfoBase*) is addressed by building a `type_index`→info map using `registry_.all_types()` and `info->type_name()`. |
| ADR-016 (yaml-cpp Dependency) | SceneSaver writes YAML using yaml-cpp's `YAML::Node` construction and stream output. YAML exceptions are caught and wrapped in `Result` errors. |
| ADR-001 (Result/Error Pattern) | `SceneSaver::save_to_file()` returns `Result<void>`. Errors use `Error::Category::IoFailed`. |
| ADR-019 (Architecture Boundaries) | SceneSaver lives in `src/engine/scene/` and uses engine abstractions only. No SDL3/OpenGL/GLM headers outside `src/engine/`. |
| ADR-011 (Ownership, Nullability, Lifetime) | `SceneSaver` stores `World&`, `ComponentRegistry&`, `AssetManager&` (non-owning references). No raw pointer members beyond the `type_to_info_` map which stores `const ComponentInfoBase*` obtained from `ComponentRegistry::all_types()`. |

## Files to inspect

| File | Reason |
|---|---|
| `src/engine/scene/world.h` | EntityNode struct — `source_` field goes here. Understand existing private helpers (`get_name`/`set_name` pattern). |
| `src/engine/scene/world.cpp` | Understand existing helper implementations. |
| `src/engine/scene/entity.h` | Entity class — `source()` / `set_source()` methods go here. Pattern: `transform()` delegates to `world_->get_transform(id_)`. |
| `src/engine/scene/entity.cpp` | Entity implementation — existing delegation pattern for reference. |
| `src/engine/scene/scene_loader.h` | SceneLoader API — SceneSaver is symmetric. Note `compose_transform()` as public static. |
| `src/engine/scene/scene_loader.cpp` | `load_entity()` — must add source-setting calls after prefab/model handling. Understand the existing source code structure. |
| `src/engine/scene/component_registry/component_registry.h` | ComponentRegistry API — `all_types()` returns `span<const ComponentInfoBase*>`. `create(string_view)` returns `Result<unique_ptr<Component>>`. `describe(string_view)` returns `const ComponentInfoBase*`. |
| `src/engine/scene/component_registry/component_info.h` | `ComponentInfoBase` — has `type_name()`, `create()`, `serialize(comp, ctx)`. `ComponentInfo<T>` templates. The `serialize()` returns properties YAML node. |
| `src/engine/scene/component_registry/serialization.h` | `serialize_component(info, comp, ctx)` free function — SceneSaver calls this. |
| `src/engine/scene/component_registry/serialization_context.h` | `SerializationContext{AssetManager&}` — SceneSaver constructs one for component serialization. |
| `src/engine/error.h` | `Result<T>`, `Error::Category` (`IoFailed`, `InvalidArgument`), `make_error()` helpers. |
| `src/engine/scene/transform.h` | Transform struct — position, rotation, scale. |
| `src/engine/asset/asset_manager.h` | AssetManager API — `base_path()` may be relevant. |
| `tests/scene_loader_tests.cpp` | Test pattern reference (Catch2, `TestEnv` with `EngineService::create()`, YAML node building helpers). |

## Files allowed to change

- **`src/engine/scene/entity_source.h`** — **CREATE**: `EntitySourceType` enum and `EntitySource` struct.
- **`src/engine/scene/world.h`** — **MODIFY**: Add `#include "scene/entity_source.h"`. Add `EntitySource source_` field to `EntityNode`. Add source accessor declarations, root entity iteration declarations, and component iteration declarations in the private helper section.
- **`src/engine/scene/world.cpp`** — **MODIFY**: Implement `get_source()`, `set_source()`, `root_entity_count()`, `get_root_entity()`, `component_count()`, `get_component_at()`.
- **`src/engine/scene/entity.h`** — **MODIFY**: Add `source()`, `set_source()`, `component_count()`, `component_at()` declarations.
- **`src/engine/scene/entity.cpp`** — **MODIFY**: Implement `Entity::source()`, `Entity::set_source()`, `Entity::component_count()`, `Entity::component_at()` delegating to `world_`.
- **`src/engine/scene/scene_loader.cpp`** — **MODIFY**: Add `entity.set_source(...)` calls after prefab entity is returned and after model directive is processed. Add `#include "scene/entity_source.h"`.
- **`src/engine/scene/scene_saver.h`** — **CREATE**: SceneSaver class declaration.
- **`src/engine/scene/scene_saver.cpp`** — **CREATE**: SceneSaver implementation.
- **`src/engine/scene/component_registry/property.h`** — **MODIFY**: Add `DefaultChecker` type alias, constructor parameter, and member. Modify `serialize()` to return null node when checker reports default value. (See Step 4b.)
- **`src/engine/scene/component_registry/component_info.h`** — **MODIFY**: In `add_property()` overloads, compute default value by instantiating default `T{}`, calling getter, and storing as a `DefaultChecker` lambda on the Property. In `ComponentInfo::serialize()`, skip null-node properties. (See Step 4b.)
- **`tests/scene_saver_tests.cpp`** — **CREATE**: At least 8 unit tests.

## Files forbidden to change

- `src/engine/scene/component_registry/component_registry.h` — Public API invariant (all_types(), create(), describe() are sufficient).
- `src/engine/scene/component_registry/serialization.h` / `.cpp` — No changes needed (serialize_component() just wraps info.serialize(), no logic change).
- `src/engine/scene/component_registry/register_all_components.h` / `.cpp` — No changes needed (add_property API unchanged from caller perspective).
- `src/engine/scene/component_registry/type_registry.h` — No changes needed (used by ComponentInfo<T>::serialize() indirectly via Property::serialize()).
- `src/engine/scene/component.h` — No changes needed.
- `src/engine/engine_service.h` / `.cpp` — No changes needed.
- `src/engine/error.h` — No changes needed.
- `src/engine/scene/transform.h` — No changes needed.
- `src/engine/render/model_utils.h` — No changes needed.
- `src/engine/asset/asset_manager.h` / `.cpp` — No changes needed.
- `src/cmd/main.cpp` — No changes needed.
- `src/cmd/app.h` / `.cpp` — No changes needed.
- Any existing `App` subclass file.
- `tests/CMakeLists.txt` — Already globs `*_tests.cpp`, no changes needed.
- `tests/scene_loader_tests.cpp` — Must not be modified (must continue to pass unchanged per SC-004).
- Any existing demo/prefab/scene YAML files.

## Existing conventions to follow

1. **Include style**: `#include "..."` for project headers, `<...>` for external/system headers. Paths relative to `src/engine/` for engine headers.
2. **Namespace nesting**: `buddd::engine`. Use `namespace` blocks without indentation of content.
3. **`#pragma once`**: All new headers must use `#pragma once`.
4. **`[[nodiscard]]`**: All `Result<T>`-returning functions must be marked `[[nodiscard]]`.
5. **Entity accessor pattern**: `Entity::source()` delegates to `world_->get_source(id_)`. `Entity::set_source()` delegates to `world_->set_source(id_, source)`. Same pattern as `name()`/`set_name()`.
6. **Error construction**: Use `make_error(Error::Category::..., "message")`.
7. **Log macros**: `BUDDD_LOG_INFO`, `BUDDD_LOG_ERROR`, `BUDDD_LOG_WARN`, `BUDDD_LOG_TAGGED_WARN`. Log tag set via `BUDDD_LOG_TAG("SceneSaver")` in `.cpp` files.
8. **Test pattern**: Catch2 `TEST_CASE("name", "[tag]")` with `#include <catch2/catch_test_macros.hpp>` and `#include <catch2/catch_approx.hpp>`.
9. **Entity construction in tests**: `TestEnv` struct with `EngineService::create(Backend::Headless, ...)` and `World world`.
10. **YAML building in tests**: Use `YAML::Node` programmatic construction (not file I/O) where possible.
11. **`Result<void>` return**: Use `return {};` for success, `return make_error(...)` for errors.
12. **`yaml-cpp` in headers**: Do NOT include `<yaml-cpp/yaml.h>` in public headers. Forward-declare `YAML::Node` as `namespace YAML { class Node; }` where needed in headers. The full include goes in `.cpp` files.

## Required implementation behavior

### Step 1: EntitySource types — `entity_source.h` (NEW)

Create `src/engine/scene/entity_source.h`:

```cpp
#pragma once

#include <cstdint>
#include <string>

namespace buddd::engine {

enum class EntitySourceType : uint8_t {
    None,    // Created directly (no prefab/model) — default
    Prefab,  // Created from a prefab file via `prefab:`
    Model,   // Created from a model directive via `model:`
};

struct EntitySource {
    EntitySourceType type{EntitySourceType::None};
    std::string path;  // Path to the prefab or model file (empty for None type)
};

} // namespace buddd::engine
```

- `#pragma once` for guard.
- `#include <cstdint>` for `uint8_t`.
- `#include <string>` for `std::string`.
- `EntitySource` is default-constructible (default `None` type, empty path).

### Step 2: World modifications — `world.h` + `world.cpp`

**`src/engine/scene/world.h`**:

1. Add `#include "scene/entity_source.h"` in the include block (after `"scene/component.h"`).

2. In the `EntityNode` struct (line 112-121), add `EntitySource source_;` after `std::string name_;` and before `EntityNode* parent_ = nullptr;`:

```cpp
struct EntityNode {
    EntityId id_;
    Transform transform_;
    std::string name_;
    EntitySource source_;                // NEW
    EntityNode* parent_ = nullptr;
    std::vector<std::unique_ptr<EntityNode>> children_;
    std::vector<std::unique_ptr<Component>> components_;
    World* world_ = nullptr;
    bool pending_destroy_ = false;
};
```

3. In the private helper section (after the `// -- Entity name --` block), add three new private method groups:

```cpp
    // -- Entity source --
    auto get_source(EntityId id) const noexcept -> const EntitySource&;
    auto set_source(EntityId id, const EntitySource& source) -> void;

    // -- Root entity iteration (for SceneSaver) --
    auto root_entity_count() const noexcept -> size_t;
    auto get_root_entity(size_t index) const noexcept -> Entity;

    // -- Component raw iteration (for SceneSaver) --
    auto component_count(EntityId id) const noexcept -> size_t;
    auto get_component_at(EntityId id, size_t index) noexcept -> Component&;
    auto get_component_at(EntityId id, size_t index) const noexcept -> const Component&;
```

**`src/engine/scene/world.cpp`**:

Add the following implementations:

```cpp
// ---------------------------------------------------------------------------
// Entity source
// ---------------------------------------------------------------------------
auto World::get_source(EntityId id) const noexcept -> const EntitySource& {
    auto* node = lookup_node(id);
    BUDDD_ASSERT(node != nullptr);
    return node->source_;
}

auto World::set_source(EntityId id, const EntitySource& source) -> void {
    auto* node = lookup_node(id);
    BUDDD_ASSERT(node != nullptr);
    node->source_ = source;
}

// ---------------------------------------------------------------------------
// Root entity iteration
// ---------------------------------------------------------------------------
auto World::root_entity_count() const noexcept -> size_t {
    return roots_.size();
}

auto World::get_root_entity(size_t index) const noexcept -> Entity {
    if (index >= roots_.size()) return Entity{};
    return Entity(*const_cast<World*>(this), roots_[index]->id_);
}

// ---------------------------------------------------------------------------
// Component raw iteration
// ---------------------------------------------------------------------------
auto World::component_count(EntityId id) const noexcept -> size_t {
    auto* node = lookup_node(id);
    if (!node) return 0;
    return node->components_.size();
}

auto World::get_component_at(EntityId id, size_t index) noexcept -> Component& {
    auto* node = lookup_node(id);
    BUDDD_ASSERT(node != nullptr && index < node->components_.size());
    return *node->components_[index];
}

auto World::get_component_at(EntityId id, size_t index) const noexcept -> const Component& {
    auto* node = lookup_node(id);
    BUDDD_ASSERT(node != nullptr && index < node->components_.size());
    return *node->components_[index];
}
```

Key details:
- `get_source()` returns `const EntitySource&` — reference valid as long as `EntityNode` is alive (same contract as `get_transform()` / `get_name()`).
- `set_source()` copies the `EntitySource` struct (small: `uint8_t` + `std::string`).
- `root_entity_count()` returns the raw count of root entities (including those pending_destroy — caller should filter).
- `get_root_entity(index)` returns `Entity{}` if index out of bounds (safe fallthrough).
- `component_count()` returns 0 for invalid entity IDs.
- `get_component_at()` asserts index in bounds — UB if out of range.
- `#include "scene/entity_source.h"` must be added to `world.h`.

### Step 3: Entity modifications — `entity.h` + `entity.cpp`

**`src/engine/scene/entity.h`**:

After the `// -- Name --` section (after `set_name` declaration) and before `// -- Transform --`, add:

```cpp
    // -- Source --
    auto source() const noexcept -> const EntitySource&;
    void set_source(const EntitySource& source);
```

After `// -- Components --` (after `remove_component` declaration) and before `// -- Hierarchy --`, add:

```cpp
    /// Number of components on this entity (for SceneSaver iteration).
    auto component_count() const noexcept -> size_t;
    /// Access component by index (for SceneSaver iteration).
    auto component_at(size_t index) noexcept -> Component&;
```

Also add `#include "scene/entity_source.h"` to the include block.

**`src/engine/scene/entity.cpp`**:

Add the following implementations after the existing set_name implementation:

```cpp
auto Entity::source() const noexcept -> const EntitySource& {
    return world_->get_source(id_);
}

void Entity::set_source(const EntitySource& source) {
    world_->set_source(id_, source);
}

auto Entity::component_count() const noexcept -> size_t {
    if (!world_) return 0;
    return world_->component_count(id_);
}

auto Entity::component_at(size_t index) noexcept -> Component& {
    return world_->get_component_at(id_, index);
}
```

### Step 4: SceneLoader modifications — `scene_loader.cpp`

In `load_entity()` (in `src/engine/scene/scene_loader.cpp`):

1. **After prefab is loaded and before reparent check** (after line ~157 `entity = *prefab_result;`): Add:
```cpp
        // Set source: Prefab type with resolved path
        entity.set_source(EntitySource{EntitySourceType::Prefab, *resolved});
```

Note: `resolved` is the `Result<std::string>` from `resolve_prefab_path()`. Use `*resolved` (the string value, not the error). Place this AFTER `entity = *prefab_result;` but BEFORE any reparent, rename, or transform composition.

2. **In the direct entity branch (else block)**, after the entity is created and name/transform are set but before the shared model/component/children processing (after line ~191 `}` closing the `if (node["transform"])` block and before line ~194 `// ── Model directive (shared) ──`): Add no source-setting here — direct entities remain default `None`.

3. **After the model directive processing** (after line ~206 `add_model_to_world(world_, root, entity);` and before `// ── Component processing (shared) ──`): Add:
```cpp
        // Set source: Model type with the model path from the YAML
        entity.set_source(EntitySource{EntitySourceType::Model, model_path});
```

Note: `model_path` is the raw string from `node["model"].as<std::string>()` (e.g., `"models/box/Box"`). Place this AFTER the `add_model_to_world` call.

4. Add `#include "scene/entity_source.h"` to the include block at the top of `scene_loader.cpp`.

**Important**: The source-setting must happen AFTER prefab loading or model loading but BEFORE component/children processing. The source indicates the root entity's origin; components and children added beyond the prefab/model are additional (the source type controls how the entity is saved, not whether components can be added at load time).

### Step 4b: Default-value property omission — `property.h` + `component_info.h` (MODIFY)

This step adds efficient default-value comparison for component properties. The default value is computed ONCE during `add_property()` (by creating a default `T{}` and calling the getter), then stored as a type-erased check function on `Property`. During serialization, the checker compares the raw PropType value against the stored default — no component creation at serialization time.

**`src/engine/scene/component_registry/property.h`**:

Add a `DefaultChecker` type alias and store it on `Property`:

```cpp
/// Functor that checks whether a property's current value equals its registered default.
/// Returns true if the property is at its default value (should be omitted during save).
using DefaultChecker = std::function<bool(const Component&, const SerializationContext&)>;

class Property {
public:
    Property(std::string name,
             std::type_index type_index,
             GetterFn getter,
             SetterFn setter,
             PropertyFlags flags = {},
             DefaultChecker default_checker = nullptr);
    // ... existing methods ...
    
    [[nodiscard]] auto has_default() const noexcept -> bool;
    
private:
    // ... existing members ...
    DefaultChecker default_checker_;
};
```

Modify `serialize()` to return null node when value matches default:

```cpp
auto Property::serialize(const Component& comp, const SerializationContext& ctx) const -> YAML::Node {
    if (default_checker_ && default_checker_(comp, ctx)) {
        return YAML::Node{};  // null — value matches default, caller should skip
    }
    return getter_(comp, ctx);
}
```

**`src/engine/scene/component_registry/component_info.h`**:

In `ComponentInfo<T>::add_property()` (overload B: simple getter, no context), after computing the Property but before inserting it, compute the default and set the checker:

```cpp
// In the overload B body (simple getter without SerializationContext):
// ... existing Property construction ...
T default_instance{};
PropType default_raw = getter(default_instance);

Property prop(name, typeid(PropType),
    /* serializer: */ [getter](const Component& comp, const SerializationContext& ctx) -> YAML::Node {
        const T& typed = dynamic_cast<const T&>(comp);
        return TypeRegistry::template yaml_encode<PropType>(getter(typed), ctx);
    },
    /* setter: */ [setter](Component& comp, const YAML::Node& node, const SerializationContext& ctx) -> Result<void> {
        T& typed = dynamic_cast<T&>(comp);
        auto decoded = TypeRegistry::template yaml_decode<PropType>(node, ctx);
        if (!decoded) return make_error(decoded.error());
        return setter(typed, *decoded);
    },
    flags,
    /* default_checker: */ [getter, default_raw](const Component& comp, const SerializationContext&) -> bool {
        const T& typed = dynamic_cast<const T&>(comp);
        return getter(typed) == default_raw;
    }
);
properties_.push_back(std::move(prop));
```

For overload C (getter with `SerializationContext`), the checker similarly captures the context-aware getter:

```cpp
T default_instance{};
PropType default_raw = getter(default_instance, ctx);  // No ctx available at registration!
```

Wait — overload C's getter requires a `SerializationContext`, which is NOT available at property registration time. **For overload C, the default checker cannot be computed during `add_property()`**. Instead, for overload C, the default checker is computed lazily:

```cpp
T default_instance{};
// Compute default lazily: store the default instance and the getter
Property prop(name, typeid(PropType),
    /* serializer: */ [getter](const Component& comp, const SerializationContext& ctx) -> YAML::Node {
        const T& typed = dynamic_cast<const T&>(comp);
        return TypeRegistry::template yaml_encode<PropType>(getter(typed, ctx), ctx);
    },
    /* setter: */ [...] /* (same as overload B) */,
    flags,
    /* default_checker: */ [getter](const Component& comp, const SerializationContext& ctx) -> bool {
        // Compute default lazily on first comparison
        static thread_local std::optional<PropType> cached_default;
        if (!cached_default.has_value()) {
            T default_instance{};
            cached_default = getter(default_instance, ctx);
        }
        const T& typed = dynamic_cast<const T&>(comp);
        return getter(typed, ctx) == *cached_default;
    }
);
```

⚠ **Important**: The cached default uses `static thread_local` to compute the default only once per type. This is safe because:
- The default is computed from a default-constructed `T{}`, which is deterministic.
- `static thread_local` ensures thread safety for parallel serialization.
- The `SerializationContext` is only used for asset resolution during the getter call — the default value itself doesn't depend on the context (it reads from a default-constructed component).

Also modify `ComponentInfo<T>::serialize()` to skip null-node properties:

```cpp
auto serialize(const Component& comp, const SerializationContext& ctx) const -> YAML::Node override {
    YAML::Node node;
    for (const auto& prop : properties_) {
        auto value = prop.serialize(comp, ctx);
        if (value.IsNull()) continue;  // Default-valued property — skip
        node[std::string(prop.name())] = value;
    }
    return node;
}
```

**Performance guarantee**: No component is created at serialization time. The default checker compares raw PropType values via `operator==` (no YAML encoding/decoding). The only runtime cost is one raw comparison per property. For overload C, the default is computed once (first call) and cached.

**Edge case**: If ALL properties of a component are default-valued, `serialize()` returns an empty YAML node (`{}`). The SceneSaver's `save_entity()` checks if the properties node is non-empty before including it (see Step 6).

**No changes to `Property`'s getter/setter lambdas** — only the new `default_checker_` member and constructor parameter are added. The serialization lambdas in `add_property()` remain unchanged.

### Step 5: SceneSaver — `scene_saver.h` (NEW)

Create `src/engine/scene/scene_saver.h`:

```cpp
#pragma once

#include "error.h"
#include "scene/entity.h"
#include "scene/entity_source.h"

#include <string>
#include <typeindex>
#include <unordered_map>

// Forward declare YAML types — no yaml-cpp include in public header per ADR-016 / ADR-019.
namespace YAML {
class Node;
}

namespace buddd::engine {

class World;
class ComponentRegistry;
class AssetManager;
class ComponentInfoBase;
struct SerializationContext;

class SceneSaver {
public:
    SceneSaver(World& world, ComponentRegistry& registry, AssetManager& assets);

    /// Save the World to a YAML file.
    [[nodiscard]] auto save_to_file(const std::string& path) -> Result<void>;

    /// Save the World to a YAML::Node (useful for testing and in-memory workflows).
    [[nodiscard]] auto save_to_yaml() -> YAML::Node;

private:
    /// Serialize a single entity to a YAML node.
    auto save_entity(Entity entity) -> YAML::Node;

    /// Serialize a Transform to a YAML node.
    auto save_transform(const Transform& t) -> YAML::Node;

    /// Build the reverse-lookup map from std::type_index to ComponentInfoBase*.
    void build_type_to_info_map();

    World& world_;
    ComponentRegistry& registry_;
    AssetManager& assets_;

    /// Reverse map: std::type_index(Component subclass) → ComponentInfoBase*
    std::unordered_map<std::type_index, const ComponentInfoBase*> type_to_info_;
};

} // namespace buddd::engine
```

Key declarations:
- `save_to_file(path)` returns `Result<void>`.
- `save_to_yaml()` returns `YAML::Node` — no `Result` wrapper because YAML node construction can throw; exceptions are caught in `save_to_file()` or the caller is expected to wrap.
- `save_entity()` is private, called recursively for children.
- `save_transform()` is a private helper.
- `build_type_to_info_map()` is a private helper called once during construction.
- `type_to_info_` maps runtime type_index to the ComponentInfoBase for serialization.

### Step 6: SceneSaver implementation — `scene_saver.cpp` (NEW)

Create `src/engine/scene/scene_saver.cpp`:

```cpp
#include "scene/scene_saver.h"

#include "debug/assert.h"
#include "log/log.h"
#include "error.h"
#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/component_info.h"
#include "scene/component_registry/serialization.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/world.h"
#include "scene/transform.h"

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <typeindex>
#include <unordered_map>

BUDDD_LOG_TAG("SceneSaver");

namespace buddd::engine {

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
SceneSaver::SceneSaver(World& world, ComponentRegistry& registry, AssetManager& assets)
    : world_(world)
    , registry_(registry)
    , assets_(assets)
{
    build_type_to_info_map();
}

// ---------------------------------------------------------------------------
// build_type_to_info_map
// ---------------------------------------------------------------------------
void SceneSaver::build_type_to_info_map() {
    for (const auto* info : registry_.all_types()) {
        // registry_.all_types() returns const pointers, but create() is non-const.
        // The underlying objects in ComponentRegistry::infos_ are stored as
        // non-const unique_ptr<ComponentInfoBase> — the const in all_types() is
        // an access-level constraint, not an ownership qualifier. create() does
        // not modify the ComponentInfoBase state (it constructs a new Component
        // subclass instance). So const_cast is safe here.
        auto* mutable_info = const_cast<ComponentInfoBase*>(info);
        auto tmp = mutable_info->create();
        BUDDD_ASSERT(tmp != nullptr);
        type_to_info_[typeid(*tmp)] = info;
    }
}

// ---------------------------------------------------------------------------
// save_to_file
// ---------------------------------------------------------------------------
auto SceneSaver::save_to_file(const std::string& path) -> Result<void> {
    BUDDD_LOG_INFO("Saving scene: {}", path);

    YAML::Node node;
    try {
        node = save_to_yaml();
    } catch (const YAML::Exception& e) {
        return make_error(Error::Category::IoFailed,
            "Failed to build YAML node: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return make_error(Error::Category::IoFailed,
            "Unexpected error saving scene: " + std::string(e.what()));
    }

    try {
        std::ofstream fout(path);
        if (!fout.is_open()) {
            return make_error(Error::Category::IoFailed,
                "Failed to open file for writing: " + path);
        }
        fout << node;
        if (!fout.good()) {
            return make_error(Error::Category::IoFailed,
                "Failed to write YAML to file: " + path);
        }
    } catch (const std::exception& e) {
        return make_error(Error::Category::IoFailed,
            "I/O error writing scene file: " + std::string(e.what()));
    }

    // Count non-pending-destroy entities for the log message
    size_t entity_count = 0;
    for (size_t i = 0; i < world_.root_entity_count(); ++i) {
        auto entity = world_.get_root_entity(i);
        if (!entity.is_pending_destroy()) ++entity_count;
    }

    BUDDD_LOG_INFO("Scene saved: {} entities", entity_count);
    return {};
}

// ---------------------------------------------------------------------------
// save_to_yaml
// ---------------------------------------------------------------------------
auto SceneSaver::save_to_yaml() -> YAML::Node {
    YAML::Node root;
    root["type"] = "Scene";
    root["version"] = 1;

    YAML::Node entities;
    for (size_t i = 0; i < world_.root_entity_count(); ++i) {
        auto entity = world_.get_root_entity(i);
        if (entity.is_pending_destroy()) continue;
        entities.push_back(save_entity(entity));
    }
    root["entities"] = entities;

    return root;
}

// ---------------------------------------------------------------------------
// save_entity
// ---------------------------------------------------------------------------
auto SceneSaver::save_entity(Entity entity) -> YAML::Node {
    YAML::Node node;

    const auto& src = entity.source();

    auto maybe_transform = [&]() -> YAML::Node {
        auto t = save_transform(entity.transform());
        // Only emit transform if at least one field is non-default
        return t.IsMap() && t.size() > 0 ? t : YAML::Node{};
    };

    // ── Prefab source: emit only prefab ref + name + transform ──
    if (src.type == EntitySourceType::Prefab) {
        node["prefab"] = src.path;
        node["name"] = entity.name();
        if (auto t = maybe_transform(); !t.IsNull()) node["transform"] = t;
        return node;
    }

    // ── Model source: emit only name + model ref + transform ──
    if (src.type == EntitySourceType::Model) {
        node["name"] = entity.name();
        node["model"] = src.path;
        if (auto t = maybe_transform(); !t.IsNull()) node["transform"] = t;
        return node;
    }

    // ── None source: full entity serialization ──
    node["name"] = entity.name();
    if (auto t = maybe_transform(); !t.IsNull()) node["transform"] = t;

    // Serialize components
    if (entity.component_count() > 0) {
        YAML::Node components;
        SerializationContext ctx{assets_};
        for (size_t i = 0; i < entity.component_count(); ++i) {
            Component& comp = entity.component_at(i);
            auto it = type_to_info_.find(typeid(comp));
            if (it == type_to_info_.end()) {
                // Unregistered component type — this is an error in the World state.
                // Propagate by throwing an exception that will be caught by save_to_file().
                throw std::runtime_error(
                    "Unregistered component type encountered during scene save: " +
                    std::string(typeid(comp).name()));
            }
            const ComponentInfoBase* info = it->second;
            YAML::Node comp_node;
            comp_node["type"] = std::string(info->type_name());
            auto props = serialize_component(*info, comp, ctx);
            // Only include properties if there are non-default ones
            if (props.IsMap() && props.size() > 0) {
                comp_node["properties"] = std::move(props);
            }
            components.push_back(comp_node);
        }
        // Only include components if there's at least one
        if (components.size() > 0) {
            node["components"] = components;
        }
    }

    // Recursively save children
    if (entity.child_count() > 0) {
        YAML::Node children;
        for (size_t i = 0; i < entity.child_count(); ++i) {
            auto child = entity.get_child(i);
            children.push_back(save_entity(child));
        }
        node["children"] = children;
    }

    return node;
}

// ---------------------------------------------------------------------------
// save_transform
// ---------------------------------------------------------------------------
auto SceneSaver::save_transform(const Transform& t) -> YAML::Node {
    YAML::Node node;

    // Only emit fields that differ from defaults (skip default-valued fields
    // for cleaner YAML output).

    // Default position: [0, 0, 0]
    if (t.position.x != 0.0f || t.position.y != 0.0f || t.position.z != 0.0f) {
        YAML::Node pos;
        pos.push_back(t.position.x);
        pos.push_back(t.position.y);
        pos.push_back(t.position.z);
        node["position"] = pos;
    }

    // Default rotation: identity quaternion [1, 0, 0, 0]
    if (t.rotation.w != 1.0f || t.rotation.x != 0.0f || t.rotation.y != 0.0f || t.rotation.z != 0.0f) {
        YAML::Node rot;
        rot.push_back(t.rotation.w);
        rot.push_back(t.rotation.x);
        rot.push_back(t.rotation.y);
        rot.push_back(t.rotation.z);
        node["rotation"] = rot;
    }

    // Default scale: [1, 1, 1]
    if (t.scale.x != 1.0f || t.scale.y != 1.0f || t.scale.z != 1.0f) {
        YAML::Node scl;
        scl.push_back(t.scale.x);
        scl.push_back(t.scale.y);
        scl.push_back(t.scale.z);
        node["scale"] = scl;
    }

    return node;
}

} // namespace buddd::engine
```

Implementation details and contracts:

1. **Constructor**: Calls `build_type_to_info_map()` immediately. Stores references.

2. **`build_type_to_info_map()`**: Iterates `registry_.all_types()`. For each `ComponentInfoBase*`, calls `info->create()` (which returns `unique_ptr<Component>`), takes `typeid(*tmp)` to get the `std::type_index`, and stores the mapping `type_index → info`. Makes `BUDDD_ASSERT(tmp != nullptr)` for each type. This map is used during `save_entity()` for O(1) reverse lookup.

   ⚠ **Note**: `ComponentInfoBase::create()` is declared as non-const in the base class but does not actually modify the info object. The call happens during construction, which is safe.

3. **`save_to_file(path)`**:
   - Calls `save_to_yaml()` inside a try-catch for `YAML::Exception` and `std::exception`. If caught, returns `Error::Category::IoFailed`.
   - Opens `std::ofstream fout(path)`. If open fails, returns `IoFailed`.
   - Writes the YAML node via `fout << node;` (yaml-cpp emitter integration).
   - If `fout.good()` fails after write, returns `IoFailed`.
   - Catches `std::exception` around file writing to handle any I/O or yaml-cpp errors.
   - Logs scene save start (`BUDDD_LOG_INFO`) and completion with entity count.
   - **Does NOT create parent directories** — returns `IoFailed` if the parent directory does not exist (per spec Q02).

4. **`save_to_yaml()`**:
   - Creates root YAML node with `type: Scene` and `version: 1`.
   - Iterates all root entities via `world_.root_entity_count()` and `world_.get_root_entity(i)`.
   - Skips entities with `is_pending_destroy()`.
   - Calls `save_entity(entity)` for each root entity and pushes into `entities` array.
   - Always produces an `entities:` key (even if empty — `entities: []`).

5. **`save_entity(entity)`** — Entity serialization by source type:

   **Prefab source**:
   - Output: `prefab: <path>`, `name: <name>`, `transform: {...}`.
   - No `components:`, no `children:`.
   - The `prefab:` value is `src.path` (the resolved prefab path as loaded by SceneLoader).

   **Model source**:
   - Output: `name: <name>`, `model: <path>`, `transform: {...}`.
   - No `components:`, no `children:`.
   - The `model:` value is `src.path` (the model asset ID string from YAML).

   **None source**:
   - Output: `name:`, `transform:`, optionally `components:`, optionally `children:`.
   - Components are serialized using the reverse-lookup map (`type_to_info_`).
   - For each component: look up `typeid(comp)` in the map. If not found → throw `std::runtime_error` (caught by `save_to_file()` and converted to `IoFailed` error).
   - Call `serialize_component(*info, comp, ctx)` to get the properties YAML node.
   - Structure: `{type: "<type_name>", properties: {...}}`.
   - If `component_count() == 0`, `components:` key is omitted entirely (per spec Q01).
   - Children are recursively saved via `save_entity(child)`.
   - If `child_count() == 0`, `children:` key is omitted entirely.

6. **`save_transform(t)`**: Creates a YAML mapping node with optional `position`, `rotation`, and `scale` fields. Fields that match their default values are omitted:
   - `position` default: `[0, 0, 0]` → omitted if x==0 && y==0 && z==0
   - `rotation` default: `[1, 0, 0, 0]` (identity quaternion) → omitted if w==1 && x==0 && y==0 && z==0
   - `scale` default: `[1, 1, 1]` → omitted if x==1 && y==1 && z==1
   
   If all three fields are at defaults, an empty (or null) node is returned. The caller (`save_entity()`) checks for this and omits the `transform:` key entirely.

   ⚠ **yaml-cpp float formatting**: yaml-cpp may emit `1` for integer-valued floats like `1.0f`. This is acceptable — the output is semantically equivalent. The test tolerance `1e-5f` handles any precision differences on round-trip.

### Step 7: Add `#include "scene/entity_source.h"` to files that need it

- `src/engine/scene/world.h` — already covered in Step 2.
- `src/engine/scene/entity.h` — already covered in Step 3.
- `src/engine/scene/scene_loader.cpp` — already covered in Step 4.

No new includes needed in `scene_loader.h` (source is internal to `.cpp` implementation).

## Required tests

### Unit tests — `tests/scene_saver_tests.cpp`

All tests must be tagged `[scene_saver]`. Tests use `load_from_yaml()` to set up test state and `save_to_yaml()` to verify output, avoiding filesystem dependency where possible. Use the `TestEnv` pattern from `tests/scene_loader_tests.cpp`.

Follow the same test infrastructure:

```cpp
#include "scene/scene_saver.h"
#include "scene/scene_loader.h"
#include "scene/world.h"
#include "scene/entity.h"
#include "scene/entity_source.h"
#include "scene/transform.h"
#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/register_all_components.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/component_registry/serialization.h"
#include "scene/camera_component.h"
#include "asset/asset_manager.h"
#include "engine_service.h"
#include "platform/platform.h"
#include "window/window.h"
#include "error.h"

#include <yaml-cpp/yaml.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

using namespace buddd::engine;
using Catch::Approx;

namespace {
constexpr float TOL = 1e-5f;

struct TestEnv {
    std::unique_ptr<EngineService> engine;
    World world;

    TestEnv()
        : engine(EngineService::create(
              Backend::Headless,
              WindowConfig{"Test", 800, 600}).value())
    {}
};
} // anonymous namespace
```

**Test 1 — Default source for directly-created entity** (AC-005):
- Create entity via `World::add_entity()`.
- Verify `source().type == EntitySourceType::None`.
- Verify `source().path == ""`.

**Test 2 — Entity source can be set and retrieved** (AC-017):
- Create entity via `World::add_entity()`.
- Call `entity.set_source(EntitySource{EntitySourceType::Model, "models/box/Box"})`.
- Verify `source().type == EntitySourceType::Model`.
- Verify `source().path == "models/box/Box"`.
- Verify `source()` returns the same data via `const&` (no dangling reference).

**Test 3 — Prefab source tracking via SceneLoader** (AC-006, AC-012):
This test requires a prefab file on disk. Create a temp prefab YAML file in the assets directory, then load a scene that references it. Verify the loaded entity has `source().type == EntitySourceType::Prefab` and `source().path` is non-empty.

```cpp
// Create temporary prefab file
TestEnv env;
std::string base_path(env.engine->assets().base_path());
std::string prefab_path = base_path + "/buddd_test_prefab.yaml";

// Write prefab file
std::ofstream f(prefab_path);
REQUIRE(f.is_open());
f << "type: Prefab\nversion: 1\nentities:\n  - name: test_prefab_root\n";
f.close();

// Load a scene that references the prefab
SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());
YAML::Node scene_node;
scene_node["type"] = "Scene";
scene_node["version"] = 1;
scene_node["entities"][0]["prefab"] = "buddd_test_prefab";
scene_node["entities"][0]["name"] = "instance";

auto result = loader.load_from_yaml(scene_node);
REQUIRE(result.has_value());

// Find the entity
Entity entity = Entity::none();
env.world.each<Component>([&](Entity e, Component&) {
    entity = e;
    return false;
});

// If no component-based entity, find by name
// (Prefab has no components, so we need another way to find it)
// Use world root entity iteration or lookup by name
// Alternative: iterate roots directly
for (size_t i = 0; i < env.world.root_entity_count(); ++i) {
    auto e = env.world.get_root_entity(i);
    if (e.name() == "instance") {
        entity = e;
        break;
    }
}

REQUIRE(entity.id() != EntityId::none());
CHECK(entity.source().type == EntitySourceType::Prefab);
CHECK_FALSE(entity.source().path.empty());

// Clean up
std::filesystem::remove(prefab_path);
```

**Test 4 — Model source tracking via SceneLoader** (AC-007):
This test loads a scene entity with `model: models/box/Box` and verifies the entity gets `EntitySourceType::Model` with the correct model path. Requires the box model asset to be available.

```cpp
TestEnv env;
SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

YAML::Node node;
node["type"] = "Scene";
node["version"] = 1;
node["entities"][0]["name"] = "box_entity";
node["entities"][0]["model"] = "models/box/Box";

auto result = loader.load_from_yaml(node);
if (!result) {
    // Model asset may not be available in headless test env — skip gracefully
    WARN("Model asset 'models/box/Box' not available, skipping model source test");
} else {
    // Find the entity by root iteration
    Entity entity = Entity::none();
    for (size_t i = 0; i < env.world.root_entity_count(); ++i) {
        auto e = env.world.get_root_entity(i);
        if (e.name() == "box_entity") {
            entity = e;
            break;
        }
    }
    REQUIRE(entity.id() != EntityId::none());
    CHECK(entity.source().type == EntitySourceType::Model);
    CHECK(entity.source().path == "models/box/Box");
}
```

If the model asset is not available in the test environment, a programmatic verification is also acceptable: create an entity, set its source to `{Model, "models/box/Box"}`, and verify via `entity.source()`. This tests the source API directly (already covered by Test 2), while the SceneLoader's model source-setting is verified as correct by code review (it follows the same pattern as the prefab source-setting in Test 3).

**Test 4b — Model child entities have None source** (AC-008):
Loading an entity with a `model:` directive creates child entities via `add_model_to_world()` for each mesh node. These children should have `source().type == EntitySourceType::None` (they are internal expansion nodes, not saved separately).

```cpp
TestEnv env;
SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

YAML::Node node;
node["type"] = "Scene";
node["version"] = 1;
node["entities"][0]["name"] = "box_container";
node["entities"][0]["model"] = "models/box/Box";

auto result = loader.load_from_yaml(node);
if (!result) {
    WARN("Model asset 'models/box/Box' not available, skipping model child source test");
} else {
    // Find the container entity
    Entity container = Entity::none();
    for (size_t i = 0; i < env.world.root_entity_count(); ++i) {
        auto e = env.world.get_root_entity(i);
        if (e.name() == "box_container") {
            container = e;
            break;
        }
    }
    REQUIRE(container.id() != EntityId::none());

    // Check each child has None source
    for (size_t i = 0; i < container.child_count(); ++i) {
        auto child = container.get_child(i);
        CHECK(child.source().type == EntitySourceType::None);
        CHECK(child.source().path == "");
    }
}
```

If model assets are not available, the test can alternatively create child entities programmatically and verify their default source is None (which is already covered by Test 1 parent→child tests).

**Test 5 — Save empty World** (AC-010, AC-019):
- Create empty World.
- Create SceneSaver.
- Call `save_to_yaml()`.
- Verify root has `type: Scene` and `version: 1`.
- Verify `entities:` is present and is an empty sequence (`entities.IsSequence() && entities.size() == 0`).

**Test 5 — Save None-source entity with components** (AC-011, AC-015):
- Load a scene with a directly-created entity that has a CameraComponent.
- Save via SceneSaver.
- Verify the saved YAML contains `name:`, `transform:`, `components:` with a `camera` entry that has `fov_y` property matching the original.

```cpp
TestEnv env;
SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

YAML::Node node;
node["type"] = "Scene";
node["version"] = 1;
node["entities"][0]["name"] = "camera_entity";
node["entities"][0]["components"][0]["type"] = "camera";
node["entities"][0]["components"][0]["properties"]["fov_y"] = 1.0;
node["entities"][0]["components"][0]["properties"]["aspect"] = 1.5;

auto load_result = loader.load_from_yaml(node);
REQUIRE(load_result.has_value());

SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
YAML::Node saved = saver.save_to_yaml();

REQUIRE(saved["type"].as<std::string>() == "Scene");
REQUIRE(saved["version"].as<int>() == 1);
REQUIRE(saved["entities"].IsSequence());
REQUIRE(saved["entities"].size() == 1);

auto& ent = saved["entities"][0];
CHECK(ent["name"].as<std::string>() == "camera_entity");
CHECK(ent["transform"].IsDefined());
CHECK(ent["components"].IsSequence());
CHECK(ent["components"].size() == 1);
CHECK(ent["components"][0]["type"].as<std::string>() == "camera");
CHECK(ent["components"][0]["properties"]["fov_y"].as<double>() == Approx(1.0));
```

**Test 6 — Save Prefab source entity as reference** (AC-012):
- Create an entity, set its source to `{Prefab, "prefabs/free_camera"}` and name to `"main_camera"`.
- Call `save_to_yaml()`.
- Verify the entity YAML has `prefab: prefabs/free_camera` and `name: main_camera` and `transform:`.
- Verify there is NO `components:` key and NO `children:` key.

```cpp
TestEnv env;
auto entity = env.world.add_entity();
entity.set_name("main_camera");
entity.set_source(EntitySource{EntitySourceType::Prefab, "prefabs/free_camera"});

SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
YAML::Node saved = saver.save_to_yaml();

REQUIRE(saved["entities"].IsSequence());
REQUIRE(saved["entities"].size() == 1);

auto& ent = saved["entities"][0];
CHECK(ent["prefab"].as<std::string>() == "prefabs/free_camera");
CHECK(ent["name"].as<std::string>() == "main_camera");
CHECK(ent["transform"].IsDefined());
CHECK_FALSE(ent["components"].IsDefined());
CHECK_FALSE(ent["children"].IsDefined());
```

**Test 7 — Save Model source entity as reference** (AC-013):
- Create an entity, set its source to `{Model, "models/box/Box"}` and name to `"my_box"`.
- Call `save_to_yaml()`.
- Verify the entity YAML has `name: my_box` and `model: models/box/Box` and `transform:`.
- Verify there is NO `children:` key.

```cpp
TestEnv env;
auto entity = env.world.add_entity();
entity.set_name("my_box");
entity.set_source(EntitySource{EntitySourceType::Model, "models/box/Box"});

SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
YAML::Node saved = saver.save_to_yaml();

REQUIRE(saved["entities"].IsSequence());
REQUIRE(saved["entities"].size() == 1);

auto& ent = saved["entities"][0];
CHECK(ent["name"].as<std::string>() == "my_box");
CHECK(ent["model"].as<std::string>() == "models/box/Box");
CHECK(ent["transform"].IsDefined());
CHECK_FALSE(ent["children"].IsDefined());
```

**Test 9 — Round-trip save then load** (AC-016):
- Load a scene with a direct entity that has a name, custom transform, and a CameraComponent.
- Save via SceneSaver.
- Load the saved YAML into a fresh World via SceneLoader.
- Verify the new World has the same entity count, names, transform values (within tolerance), and source types.

```cpp
TestEnv env;

// Load initial scene
SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());
YAML::Node node;
node["type"] = "Scene";
node["version"] = 1;
node["entities"][0]["name"] = "test_entity";
node["entities"][0]["transform"]["position"] = YAML::Node(std::vector<float>{1.0f, 2.0f, 3.0f});
node["entities"][0]["components"][0]["type"] = "camera";
node["entities"][0]["components"][0]["properties"]["fov_y"] = 1.0;
node["entities"][0]["components"][0]["properties"]["aspect"] = 1.5;

auto load_result = loader.load_from_yaml(node);
REQUIRE(load_result.has_value());

// Save
SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
YAML::Node saved = saver.save_to_yaml();

// Load into fresh World
World world2;
SceneLoader loader2(world2, env.engine->registry(), env.engine->assets());
auto reload_result = loader2.load_from_yaml(saved);
REQUIRE(reload_result.has_value());

// Verify properties
CHECK(world2.entity_count() == env.world.entity_count());
// Find entity in world2 by name
Entity found = Entity::none();
for (size_t i = 0; i < world2.root_entity_count(); ++i) {
    auto e = world2.get_root_entity(i);
    if (e.name() == "test_entity") {
        found = e;
        break;
    }
}
REQUIRE(found.id() != EntityId::none());

// Verify source type is None (direct entity)
CHECK(found.source().type == EntitySourceType::None);

// Verify transform
auto& t = found.transform();
CHECK(t.position.x == Approx(1.0f).margin(TOL));
CHECK(t.position.y == Approx(2.0f).margin(TOL));
CHECK(t.position.z == Approx(3.0f).margin(TOL));

// Verify component
auto cam = found.get_component<CameraComponent>();
REQUIRE(cam.has_value());
CHECK(cam->fov_y() == Approx(1.0f).margin(TOL));
```

**Test 10 — Save entity with no components omits components key** (AC-011 edge case):
- Create an entity with source `None`, set name and transform, but add NO components.
- Save via SceneSaver.
- Verify the YAML has `name:`, `transform:` but NO `components:` key.

**Test 11 — `save_to_file()` writes valid YAML to disk** (AC-014):
- Create a World with one entity.
- Save to a temp file via `save_to_file()`.
- Read the file back via `YAML::LoadFile()`.
- Verify the content matches `save_to_yaml()` output.
- Clean up temp file.

**Test 12 — Default transform fields omitted from output** (AC-020):
- Create an entity with source `None` and a default transform (position=[0,0,0], rotation=identity, scale=[1,1,1]).
- Give it a CameraComponent with default properties (fov_y=1.047, aspect=1.333).
- Save via SceneSaver.
- Verify the YAML has NO `transform:` key (all fields at defaults).
- Now change the entity's position to [5, 0, 0].
- Save again.
- Verify the YAML has `transform: {position: [5, 0, 0]}` but NO `rotation:` or `scale:` keys.

```cpp
TestEnv env;

// Add entity with all-default transform
auto entity = env.world.add_entity();
entity.set_name("default_entity");
// Add a component so we can find the entity
entity.add_component<CameraComponent>();

SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
YAML::Node saved = saver.save_to_yaml();
REQUIRE(saved["entities"].IsSequence());
REQUIRE(saved["entities"].size() == 1);

auto& ent = saved["entities"][0];
// Transform should be entirely omitted when all fields are default
CHECK_FALSE(ent["transform"].IsDefined());
```

**Test 13 — Default component properties omitted from output** (AC-021):
- Load a scene with a camera entity where only `fov_y` is explicitly set to a non-default value, and other camera properties (aspect, near, far) are at defaults.
- Save via SceneSaver.
- Verify the `camera` component's properties only contain the non-default `fov_y`.

```cpp
TestEnv env;
SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

YAML::Node node;
node["type"] = "Scene";
node["version"] = 1;
node["entities"][0]["name"] = "cam";
// Only set fov_y to a non-default value (default is ~1.047)
node["entities"][0]["components"][0]["type"] = "camera";
node["entities"][0]["components"][0]["properties"]["fov_y"] = 0.8;

auto load_result = loader.load_from_yaml(node);
REQUIRE(load_result.has_value());

SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
YAML::Node saved = saver.save_to_yaml();

REQUIRE(saved["entities"].IsSequence());
REQUIRE(saved["entities"].size() == 1);

auto& comps = saved["entities"][0]["components"];
REQUIRE(comps.IsSequence());
REQUIRE(comps.size() == 1);
REQUIRE(comps[0]["type"].as<std::string>() == "camera");

// fov_y (non-default) should be present
CHECK(comps[0]["properties"]["fov_y"].as<double>() == Approx(0.8));
// aspect should be absent (at default 1.333)
CHECK_FALSE(comps[0]["properties"]["aspect"].IsDefined());
```

**Test 14 — All-default component has no properties key** (AC-022):
- Load a scene with a camera entity that has NO explicit properties (all at defaults).
- Save via SceneSaver.
- Verify the component entry has `{type: camera}` with NO `properties:` key.

```cpp
TestEnv env;
SceneLoader loader(env.world, env.engine->registry(), env.engine->assets());

YAML::Node node;
node["type"] = "Scene";
node["version"] = 1;
node["entities"][0]["name"] = "default_cam";
// Camera component with default properties (no explicit properties block)
node["entities"][0]["components"][0]["type"] = "camera";

auto load_result = loader.load_from_yaml(node);
REQUIRE(load_result.has_value());

SceneSaver saver(env.world, env.engine->registry(), env.engine->assets());
YAML::Node saved = saver.save_to_yaml();

REQUIRE(saved["entities"].IsSequence());
REQUIRE(saved["entities"].size() == 1);

auto& comps = saved["entities"][0]["components"];
REQUIRE(comps.IsSequence());
REQUIRE(comps.size() == 1);
REQUIRE(comps[0]["type"].as<std::string>() == "camera");

// properties key should be ABSENT when all values are at defaults
CHECK_FALSE(comps[0]["properties"].IsDefined());
```

At least 10 of the above tests must be implemented to cover all Acceptance Criteria. Required minimum set: Tests 1, 2, 3, 4 (or 4b), 5, 6, 7, 12, 13, 14 (covering AC-005, AC-017, AC-006, AC-007+AC-008, AC-010+AC-019, AC-011+AC-015, AC-012+AC-013, AC-020, AC-021, AC-022). Round-trip test (Test 9) is strongly recommended for full AC-016 coverage. Test 11 (AC-014) is recommended for filesystem save verification. Required minimum set: Tests 1, 2, 3, 4 (or 4b), 5, 6, 7, 8 (covering AC-005, AC-017, AC-006, AC-007+AC-008, AC-010+AC-019, AC-011+AC-015, AC-012, AC-013). Round-trip test (Test 9) is strongly recommended for full AC-016 coverage. Test 11 (AC-014) is also recommended for filesystem save verification.

### E2E / Integration verification

| Method | What it verifies |
|---|---|
| **Existing scene_loader tests** | All existing `[scene_loader]` tests pass unchanged (SC-004). |
| **Round-trip with demo.yaml** | Load `assets/scenes/demo.yaml`, save via SceneSaver, diff the saved YAML against original to verify prefab/model references are preserved (manual). |
| **All unit tests** | `ctest --preset debug` passes all tests (SC-001, SC-002, SC-003). |
| **Build verification** | `cmake --build --preset debug` succeeds (no compile/link errors). |
| **Existing scenes** | `buddd run triangle --frame 1` continues to work without regression. |

## Edge cases

| Case | Expected behavior | Handled by |
|---|---|---|
| Entity with source `None` but empty name | Saved with `name: ""` (empty string). | `save_entity()` writes `entity.name()` which returns `""` for unset names. |
| Entity with source `None` and no components | `components:` key omitted entirely (per Q01). | `save_entity()` checks `component_count() > 0` before emitting components. |
| AC-007 model source test via SceneLoader | Requires model asset (`models/box/Box`) to be loadable in test environment. If not available, test is skipped with WARN. | Test 4 uses `if (!result) WARN(...)` to gracefully skip. Code review verifies SceneLoader follows same pattern as prefab source-setting. |
| Entity with source `Prefab` and transform override | Saved with `prefab:`, `name:`, `transform:` containing current values. | `save_entity()` writes `entity.transform()` directly (includes any overrides). |
| Entity with source `Model` and default transform | Saved with `name:` and `model:` only. `transform:` key omitted entirely when all fields are at defaults. | `save_entity()` checks `save_transform()` return and omits `transform:` if all fields are default. |
| World with zero entities | YAML root has `type: Scene`, `version: 1`, `entities: []`. | `save_to_yaml()` builds empty entities sequence, never omitted. |
| None-source entity with Prefab-source child | Parent saved with full expansion, child saved as prefab reference. | `save_entity()` is called recursively; each entity is saved per its own source type. |
| Entities marked pending_destroy | Skipped — not included in saved output. | `save_to_yaml()` checks `entity.is_pending_destroy()` and skips. |
| Round-trip floating-point precision | Values match within `1e-5f` tolerance (yaml-cpp serializes floats to YAML, re-parses them). | Test tolerance `1e-5f` in all assertions. |
| Component type has no entry in `type_to_info_` map | `save_entity()` throws `std::runtime_error`; `save_to_file()` catches it and returns `IoFailed` error. | Error propagated via exception → catch in `save_to_file()`. |
| File write failure (permissions, disk full) | `save_to_file()` returns `Error::Category::IoFailed` with descriptive message. | `std::ofstream::is_open()` check + `fout.good()` check + exception catch. |
| Parent directory does not exist | `save_to_file()` returns `IoFailed` (std::ofstream open fails). | `std::ofstream` constructor fails → `is_open()` returns false. |
| `ComponentInfoBase::create()` returns null | `build_type_to_info_map()` triggers `BUDDD_ASSERT`. | Non-null assertion. |

## Security impact

None. Scene files are written to the local filesystem only. All file paths are provided by the caller. No network access. No elevated privileges required. Existing files at the target path will be overwritten (caller responsibility).

## Data and migration impact

None. No schema changes to existing data. No migrations required. The new `source_` field on `EntityNode` has a default `{None, ""}` value. Existing code that creates entities without setting a source is unaffected — all directly-created entities default to `EntitySourceType::None`.

## API compatibility impact

- **`Entity` class**: Adds `source()`, `set_source()`, `component_count()`, `component_at()`. No existing API changes. Backward compatible.
- **`World` class**: Adds private `get_source()`, `set_source()`, `root_entity_count()`, `get_root_entity()`, `component_count()`, `get_component_at()`. Existing template-based `add_component<T>()`, `get_component<T>()`, `remove_component<T>()` unchanged. Backward compatible.
- **`SceneLoader`**: No API changes. Internal behavior modified to set source on prefab and model entities. Existing `load_from_file()` and `load_from_yaml()` signatures unchanged. All existing callers unaffected.
- **`ComponentRegistry`**: No changes. `all_types()`, `create()`, `describe()` are sufficient for the reverse-lookup approach.
- **No new CMake targets**: `scene_saver.cpp` and `entity_source.h` are auto-globbed by the existing `buddd_engine` library build (CMake uses `glob` for `src/engine/**/*.cpp`).

## Documentation impact

- **README**: None.
- **Wiki pages to update** (handled by wiki-agent):
  - `docs/wiki/architecture/module-map.md`: Add `EntitySource`, `EntitySourceType` to the `scene/` submodule. Add `SceneSaver` class. Document `Entity::source()`/`set_source()` additions.
  - `docs/wiki/architecture/overview.md`: Update directory layout to include `src/engine/scene/scene_saver.h/.cpp`.
  - `docs/wiki/domain/business-rules.md`: Document entity source types and serialization rules (prefab/model entities emit references). Document that `SceneLoader` now sets source on entities.
- **Other specs**: None.

## ADR impact

No ADR updates are required. ADR-028 already describes the component registry API and reverse-lookup is handled entirely within SceneSaver without modifying the registry. No new ADR is needed for this implementation.

## Done criteria

The implementation is complete when ALL of the following are verifiable:

- [ ] **AC-001**: `src/engine/scene/entity_source.h` exists, defines `EntitySourceType` enum with `None`, `Prefab`, `Model`, and `EntitySource` struct with `type` and `path` fields.
- [ ] **AC-002**: `EntitySource` struct has `type: EntitySourceType` and `path: std::string` members.
- [ ] **AC-003**: `World::EntityNode` contains `EntitySource source_` member. Inspected in `world.h`.
- [ ] **AC-004**: `Entity` has `source() -> const EntitySource&` and `set_source(const EntitySource&)` accessors. Inspect `entity.h` declarations and `entity.cpp` implementations.
- [ ] **AC-005**: Entity created via `World::add_entity()` has `source().type == None` and `source().path == ""`. Unit test.
- [ ] **AC-006**: `SceneLoader::load_entity()` sets `EntitySourceType::Prefab` and the resolved prefab path when processing a `prefab:` directive. Unit test with temporary prefab file on disk.
- [ ] **AC-007**: `SceneLoader::load_entity()` sets `EntitySourceType::Model` and the model path when processing a `model:` directive. Unit test: load entity with `model:`, verify source type and path via `entity.source()`.
- [ ] **AC-008**: Child entities created by `add_model_to_world()` have `source().type == EntitySourceType::None`. Unit test: load entity with `model:`, check child entities have None source.
- [ ] **AC-009**: `src/engine/scene/scene_saver.h` exists, declares `class SceneSaver` in `namespace buddd::engine` with `save_to_file(path) -> Result<void>` and `save_to_yaml() -> YAML::Node`.
- [ ] **AC-010**: `SceneSaver::save_to_yaml()` produces YAML with `type: Scene` and `version: 1` as top-level keys. Unit test: save empty World, verify root keys.
- [ ] **AC-011**: Entity with source type `None` is saved with `name:`, `transform:`, and optionally `components:` and `children:`. Unit test: entity with camera component, save, verify structure.
- [ ] **AC-012**: Entity with source type `Prefab` is saved with only `prefab:`, `name:`, `transform:`. No `components:` or `children:`. Unit test.
- [ ] **AC-013**: Entity with source type `Model` is saved with only `name:`, `model:`, `transform:`. No expanded children. Unit test.
- [ ] **AC-014**: `SceneSaver::save_to_file(path)` writes YAML to the specified file. Unit test: save to temp file, read back via `YAML::LoadFile`, verify structure.
- [ ] **AC-015**: Components are serialized via `serialize_component()` and appear under `components: [{type: ..., properties: {...}}]`. Unit test: entity with camera component, verify `fov_y` appears in YAML.
- [ ] **AC-016**: Round-trip: A World saved to YAML and then loaded into a fresh World produces the same entity count, names, transforms (within 1e-5f), and source types. Unit test.
- [ ] **AC-017**: `Entity::set_source(EntitySource)` sets both type and path. Unit test: set source, verify via `source()` that both fields match.
- [ ] **AC-018**: Transform fields not explicitly set are saved with their current values (default or composed). Default-valued fields are OMITTED from saved YAML (per AC-020). Verified by tests: non-default transform fields are preserved; all-default transform omits `transform:` key.
- [ ] **AC-019**: SceneSaver saves a scene with zero entities (`entities: []`). Unit test: empty World → verify `entities` is an empty sequence.
- [ ] **AC-020**: Default transform fields are omitted from saved YAML (position `[0,0,0]`, rotation `[1,0,0,0]`, scale `[1,1,1]`). Unit test: entity with default transform → no transform fields saved; entity with custom position → only position saved.
- [ ] **AC-021**: Component properties at their registered defaults are omitted from saved YAML. Unit test: load camera with default `fov_y` → save → `fov_y` not in YAML.
- [ ] **AC-022**: Component with ALL default properties has no `properties:` key. Unit test: entity with default-valued camera → `components: [{type: camera}]` with no `properties:` key.
- [ ] **TEST-001**: `tests/scene_saver_tests.cpp` exists with at least 6 unit tests tagged `[scene_saver]`.
- [ ] **TEST-002**: All scene_saver tests pass: `ctest --preset debug`.
- [ ] **TEST-003**: All existing scene_loader tests pass unchanged: `ctest --preset debug`.
- [ ] **TEST-004**: Build succeeds: `cmake --build --preset debug`.
- [ ] **TEST-005**: `buddd run triangle --frame 1` continues to work (no regression on existing scenes).
