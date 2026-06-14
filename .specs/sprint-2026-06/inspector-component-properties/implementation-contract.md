# IMPL-F-06 — Inspector — Component Properties

## Source spec

`.specs/sprint-2026-06/inspector-component-properties/spec.md`

## Goal

Extend the Properties Panel to render all components attached to the selected entity as collapsible sections below the always-expanded Transform section. Each component section exposes its properties in a 2-column table (property name | value editor) using `InspectorTypeEditorRegistry` for typed editing. Enable runtime type dispatch via `draw_any()` on `InspectorTypeEditor`. Add `SetComponentPropertyCommand` for undo/redo of property edits. Extend the engine's `ComponentInfoBase` with single-property serialize/deserialize, and `TypeRegistry` with type-erased `yaml_encode`/`yaml_decode`.

## Non-goals

- No Add Component button or workflow (explicitly excluded from scope).
- No Remove Component button or workflow.
- No Play-mode read-only enforcement (deferred to F-15).
- No multi-select editing — component sections shown only for `primary()` entity.
- No reordering of component sections.
- No component search/filter bar.
- No per-component undo grouping — each property edit creates its own `SetComponentPropertyCommand`.
- No custom editors for non-built-in types — only types registered in `InspectorTypeEditorRegistry` or `TypeRegistry` can be edited; unregistered types fall back to read-only display.
- No changes to the existing Transform section layout or entity name field.
- No drag-and-drop asset references.
- No changes to existing component registration code (register_all_components.cpp, etc.).
- No changes to `CMakeLists.txt` files (editor uses GLOB_RECURSE).
- No new dependencies beyond those already in the project.

## Relevant ADRs

- **ADR-028**: Component Type Registry with TypeRegistry and SerializationContext — defines the property system, TypeRegistry built-in types, and ComponentInfoBase architecture that this feature extends.
- **ADR-027**: Editor Architecture — defines the editor library, namespace conventions, and ImGui integration that this feature builds upon.
- **ADR-001**: Result/Error pattern used by `property_deserialize`, `yaml_encode`, `yaml_decode`.
- **ADR-026**: ImGui integration (v1.91.8-docking) — CollapsingHeader, BeginTable, ColorEdit3/ColorEdit4 etc. are available.

## Files to inspect

| File | Purpose |
|---|---|
| `src/engine/scene/component_registry/component_info.h` | Read existing `ComponentInfoBase` and `ComponentInfo<T>` — insertion point for `property_serialize`/`property_deserialize` virtual methods |
| `src/engine/scene/component_registry/type_registry.h` | Read existing `TypeRegistry` structure, `TypeEntry` struct, template implementations — add `yaml_encode`/`yaml_decode` type-erased declarations |
| `src/engine/scene/component_registry/type_registry.cpp` | Read existing `entry_map()` — add type-erased dispatch implementation |
| `src/engine/scene/component_registry/property.h` | Read `PropertyFlags` struct — understand fields for `EditorFlags` mapping |
| `src/engine/scene/component_registry/property.cpp` | Read `Property::serialize()`/`deserialize()` — delegates for `ComponentInfo<T>` property methods |
| `src/engine/scene/component_registry/serialization_context.h` | Read `SerializationContext` struct |
| `src/editor/inspector_editors.h` | Read existing `InspectorTypeEditor`, `TypedInspectorEditor<T>`, `InspectorTypeEditorRegistry` — add `draw_any()` declarations |
| `src/editor/inspector_editors.cpp` | Read existing built-in editors (float, int, bool, string, Vec2, Vec3, Vec4, Quat, Color) and `register_builtin_inspector_editors()` |
| `src/editor/panels/properties_panel.h` | Read existing header — add `draw_component_sections()` helper declaration |
| `src/editor/panels/properties_panel.cpp` | Read existing `draw_ui()`, `draw_transform_section()` — insertion point for `draw_component_sections()` call |
| `src/editor/command.h` | Read `Command` base class signature |
| `src/editor/commands/rename_entity_command.h` | Reference pattern for command implementation (constructor, execute, undo, name) |
| `src/editor/command_stack.h` | Read `CommandStack::execute()` API |
| `src/editor/editor_context.h` | Read `EditorContext` struct (editor + engine) |
| `src/editor/editor_selection.h` | Read `EditorSelection::primary()` API |
| `src/engine/scene/entity.h` | Read `Entity::component_count()`, `Entity::component_at()` signatures |
| `src/engine/scene/component.h` | Read `Component` base class |
| `src/engine/scene/component_registry/component_registry.h` | Read `ComponentRegistry::describe()` and `ComponentRegistry::all_types()` |
| `src/engine/engine_context.h` | Read `EngineContext` struct |
| `src/engine/engine_service.h` | Read `EngineService::registry()` accessor (returns `ComponentRegistry&`) |
| `src/engine/scene/component_registry/register_all_components.cpp` | Read existing component registrations to understand property types and flags (CameraComponent with fov_y min/max, PointLightComponent with color tag("rgb"), etc.) |
| `tests/editor/inspector_editors_tests.cpp` | Read existing test patterns (TestContext struct, Catch2 style) |
| `tests/editor/properties_panel_tests.cpp` | Read existing panel test patterns |

## Files allowed to change

- `src/engine/scene/component_registry/component_info.h` — add `property_serialize()` and `property_deserialize()` virtual methods to `ComponentInfoBase`; implement in `ComponentInfo<T>`
- `src/engine/scene/component_registry/type_registry.h` — add type-erased `yaml_encode`/`yaml_decode` static methods declarations; extend `TypeEntry` struct with dispatch functions
- `src/engine/scene/component_registry/type_registry.cpp` — implement type-erased `yaml_encode`/`yaml_decode` methods
- `src/editor/inspector_editors.h` — add `#include <any>`; add `draw_any()` virtual method to `InspectorTypeEditor`; add `draw_any()` override to `TypedInspectorEditor<T>`; add static `draw_any()` to `InspectorTypeEditorRegistry`
- `src/editor/panels/properties_panel.h` — add `draw_component_sections()` helper declaration
- `src/editor/panels/properties_panel.cpp` — implement `draw_component_sections()`; call it from `draw_ui()` after `draw_transform_section()`

## Files allowed to create

- `src/editor/commands/set_component_property_command.h` — new file: `SetComponentPropertyCommand` class
- `tests/editor/component_property_commands_tests.cpp` — new test file: `SetComponentPropertyCommand` tests
- `tests/engine/type_registry_type_erased_tests.cpp` — new test file: type-erased `yaml_encode`/`yaml_decode` tests
- `tests/engine/component_info_property_serialize_tests.cpp` — new test file: `property_serialize`/`property_deserialize` tests

## Files forbidden to change

- `src/engine/scene/entity.h` — no changes
- `src/engine/scene/entity.cpp` — no changes
- `src/engine/scene/component.h` — no changes
- `src/engine/scene/component_registry/register_all_components.cpp` — no changes (existing component registrations must remain as-is)
- `src/engine/scene/component_registry/component_registry.h` — no changes
- `src/engine/scene/component_registry/property.h` — no changes
- `src/engine/scene/component_registry/property.cpp` — no changes
- `src/engine/engine_context.h` — no changes
- `src/engine/engine_service.h` — no changes
- `src/editor/editor.h` — no changes
- `src/editor/editor_context.h` — no changes
- `src/editor/command.h` — no changes
- `src/editor/command_stack.h` — no changes
- `src/editor/editor_selection.h` — no changes
- `src/editor/panels/scene_panel.h` — no changes
- `src/editor/panels/scene_panel.cpp` — no changes
- Any `CMakeLists.txt` (editor uses GLOB_RECURSE, engine is unchanged)
- Any `.yaml`, `.json`, or configuration files
- Existing test files (`tests/editor/inspector_editors_tests.cpp`, `tests/editor/properties_panel_tests.cpp`, `tests/editor/entity_selection_tests.cpp`)

## Existing conventions to follow

- **Namespaces**: `buddd::engine` for engine code, `buddd::editor` for editor code, `buddd::engine::math` for math types.
- **C++ style**: Trailing return types (`auto foo() -> Bar`), `[[nodiscard]]` on query methods, `noexcept` where appropriate, `explicit` on single-arg constructors.
- **Command pattern**: Commands are `final` classes inheriting from `Command`; header-only (define execute/undo/name inline in the `.h`). See `rename_entity_command.h` pattern.
- **EditorContext access**: `ctx.editor.world()`, `ctx.editor.selection()`, `ctx.editor.command_stack()`, `ctx.editor.mark_dirty()`.
- **Engine access through EditorContext**: `ctx.engine.services.registry()` for `ComponentRegistry`, `ctx.engine.services.assets()` for `AssetManager`.
- **Logging**: `BUDDD_LOG_TAGGED_DEBUG("Editor:ComponentProperties", "...")` and `BUDDD_LOG_TAGGED_WARN("Editor:ComponentProperties", "...")` for failures.
- **ImGui patterns**: `ImGui::CollapsingHeader`, `ImGui::BeginTable`/`EndTable`, `ImGui::TableSetupColumn`, `ImGui::TableNextRow`, `ImGui::TableSetColumnIndex`.
- **Test patterns**: Catch2 `TEST_CASE` with tags like `[editor][component-properties]`, `[engine][component-registry]`.
- **EditorFlags tags**: `tags_` member uses `std::vector<std::string>` with `has_tag()` method (matching `PropertyFlags::has_tag()` convention).
- **Result type**: `Result<T> = std::expected<T, Error>` with `make_error()` factory functions.
- **Log tag convention**: Use `"Editor:ComponentProperties"` for panel-level logs, `"Editor:Command"` for command logs, `"TypeRegistry"` for type-registry logs.

## Required implementation behavior

### A. Engine: ComponentInfoBase — add property_serialize / property_deserialize

In `src/engine/scene/component_registry/component_info.h`:

**Add to `ComponentInfoBase`** (abstract base class, after `property_flags()` declaration):

```cpp
/// Serialize a single property by index from the component to a YAML node.
[[nodiscard]] virtual auto property_serialize(
    const Component& comp, size_t index,
    const SerializationContext& ctx) const -> YAML::Node = 0;

/// Deserialize a single property by index from a YAML node into the component.
/// Returns error if index is out of bounds or deserialization fails.
[[nodiscard]] virtual auto property_deserialize(
    Component& comp, size_t index,
    const YAML::Node& node,
    const SerializationContext& ctx) const -> Result<void> = 0;
```

**Implement in `ComponentInfo<T>`** (template class, after `property_flags()` override):

```cpp
auto property_serialize(const Component& comp, size_t index,
                        const SerializationContext& ctx) const -> YAML::Node override {
    const auto& typed = static_cast<const T&>(comp);
    if (index >= properties_.size()) {
        return YAML::Node();  // null node — caller must check
    }
    return properties_[index].serialize(typed, ctx);
}

auto property_deserialize(Component& comp, size_t index,
                          const YAML::Node& node,
                          const SerializationContext& ctx) const -> Result<void> override {
    if (index >= properties_.size()) {
        return make_error(Error::Category::InvalidArgument,
            "Property index " + std::to_string(index) + " out of bounds for component '" + type_name_ + "'");
    }
    auto& typed = static_cast<T&>(comp);
    return properties_[index].deserialize(typed, node, ctx);
}
```

**No includes needed** — `property.h` and `error.h` are already included in `component_info.h`.

### B. Engine: TypeRegistry — add type-erased yaml_encode / yaml_decode

In `src/engine/scene/component_registry/type_registry.h`:

**Extend `TypeEntry` struct** (add dispatch function members):

```cpp
struct TypeEntry {
    std::any info;  // holds TypeInfo<T>

    // Type-erased dispatch: encode a std::any to a YAML node.
    // The function is populated during register_type<T>().
    std::function<Result<YAML::Node>(const std::any&, const SerializationContext&)> yaml_encode_any;

    // Type-erased dispatch: decode a YAML node to a std::any.
    std::function<Result<std::any>(const YAML::Node&, const SerializationContext&)> yaml_decode_any;
};
```

**Add public static methods** to `TypeRegistry` (after the templated `yaml_encode`/`yaml_decode`):

```cpp
/// Type-erased YAML encode: given a type_index and a std::any containing
/// a value of that type, encode to YAML using the registered callbacks.
/// Returns error if the type is not registered or if the std::any doesn't match.
[[nodiscard]] static auto yaml_encode(
    std::type_index type, const std::any& value,
    const SerializationContext& ctx) -> Result<YAML::Node>;

/// Type-erased YAML decode: given a type_index and a YAML node,
/// decode to a std::any containing a value of that type.
/// Returns error if the type is not registered or decode fails.
[[nodiscard]] static auto yaml_decode(
    std::type_index type, const YAML::Node& node,
    const SerializationContext& ctx) -> Result<std::any>;
```

**Update `register_type<T>()` implementation** (in the header inline body, after `it->second.info = std::move(info);`):

```cpp
template<typename T>
auto TypeRegistry::register_type(TypeInfo<T> info) -> void {
    auto& map = entry_map();
    auto key = std::type_index(typeid(T));
    auto [it, inserted] = map.insert({key, TypeEntry{}});
    if (!inserted) {
        BUDDD_LOG_TAGGED_WARN("TypeRegistry",
            "Overwriting existing registration for type '{}'", typeid(T).name());
    }
    it->second.info = std::move(info);

    // Populate type-erased dispatch functions
    it->second.yaml_encode_any = [](const std::any& value, const SerializationContext& ctx) -> Result<YAML::Node> {
        auto* typed = std::any_cast<T>(&value);
        if (!typed) {
            return make_error(Error::Category::InvalidArgument,
                "Type mismatch in yaml_encode: expected " + std::string(typeid(T).name()));
        }
        auto* info = get<T>();
        if (!info) {
            return make_error(Error::Category::InvalidArgument,
                "Type not registered: " + std::string(typeid(T).name()));
        }
        return info->yaml_encode(*typed, ctx);
    };

    it->second.yaml_decode_any = [](const YAML::Node& node, const SerializationContext& ctx) -> Result<std::any> {
        auto decoded = yaml_decode<T>(node, ctx);
        if (!decoded) {
            return make_error(decoded.error());
        }
        return std::any(std::move(*decoded));
    };
}
```

In `src/engine/scene/component_registry/type_registry.cpp` — add implementations:

```cpp
auto TypeRegistry::yaml_encode(std::type_index type, const std::any& value,
                                const SerializationContext& ctx) -> Result<YAML::Node> {
    auto& map = entry_map();
    auto it = map.find(type);
    if (it == map.end()) {
        return make_error(Error::Category::InvalidArgument,
            "yaml_encode: type '" + std::string(type.name()) + "' not registered");
    }
    if (!it->second.yaml_encode_any) {
        return make_error(Error::Category::InvalidArgument,
            "yaml_encode: type '" + std::string(type.name()) + "' has no yaml_encode_any dispatch (was it registered via the old API?)");
    }
    return it->second.yaml_encode_any(value, ctx);
}

auto TypeRegistry::yaml_decode(std::type_index type, const YAML::Node& node,
                                const SerializationContext& ctx) -> Result<std::any> {
    auto& map = entry_map();
    auto it = map.find(type);
    if (it == map.end()) {
        return make_error(Error::Category::InvalidArgument,
            "yaml_decode: type '" + std::string(type.name()) + "' not registered");
    }
    if (!it->second.yaml_decode_any) {
        return make_error(Error::Category::InvalidArgument,
            "yaml_decode: type '" + std::string(type.name()) + "' has no yaml_decode_any dispatch (was it registered via the old API?)");
    }
    return it->second.yaml_decode_any(node, ctx);
}
```

**Header includes**: `type_registry.h` already includes `<any>`, `<functional>`, `<typeindex>`, `<unordered_map>`. No new includes needed.

**Backward compatibility**: Existing code using `TypeRegistry::register_type<T>()` without the new dispatch functions is handled — the functions are populated inside `register_type<T>()`, so all existing registrations automatically get the dispatch functions.

### C. Editor: InspectorTypeEditor — add draw_any()

In `src/editor/inspector_editors.h`:

**Add to `InspectorTypeEditor`** (abstract base, after `draw()` declaration):

```cpp
/// Draw the editor for a type-erased value. The `type_index` identifies
/// the C++ type of the value stored in `value` (a std::any).
/// Default implementation returns false (untyped editors must override).
/// @return true if the value was modified.
[[nodiscard]] virtual auto draw_any(const std::string& label,
                                    std::any& value,
                                    std::type_index type_index,
                                    const EditorFlags& flags,
                                    const EditorContext& ctx) -> bool;
```

**Add to `TypedInspectorEditor<T>`** (override, after `draw()` override):

```cpp
[[nodiscard]] auto draw_any(const std::string& label,
                            std::any& value,
                            std::type_index type_index,
                            const EditorFlags& flags,
                            const EditorContext& ctx) -> bool override {
    (void)type_index;
    auto* typed = std::any_cast<T>(&value);
    if (!typed) return false;  // type mismatch
    return draw_fn_(label, *typed, flags, ctx);
}
```

**Add to `InspectorTypeEditorRegistry`** (static method, after `get()` declaration):

```cpp
/// Draw the editor for a type-erased value, dispatching by type_index.
/// If no editor is registered for the type, renders read-only fallback.
/// @return true if the value was modified.
[[nodiscard]] static auto draw_any(const std::string& label,
                                   std::any& value,
                                   std::type_index type_index,
                                   const EditorFlags& flags,
                                   const EditorContext& ctx) -> bool;
```

**Add `#include <any>`** to the includes section (alongside existing includes).

**Add implementation in `inspector_editors.h`** (in the template section, after the existing `draw()` template):

```cpp
inline auto InspectorTypeEditorRegistry::draw_any(
    const std::string& label,
    std::any& value,
    std::type_index type_index,
    const EditorFlags& flags,
    const EditorContext& ctx) -> bool
{
    auto* editor = get(type_index);
    if (editor) {
        return editor->draw_any(label, value, type_index, flags, ctx);
    }
    // No editor registered — fallback to read-only display
    draw_fallback_readonly(label, type_index, ctx);
    return false;
}
```

Alternatively, this can be in the .cpp to reduce header size. **Preference: keep in .h** (inline) for symmetry with the existing `draw<T>()` template which is in the header.

No changes to `inspector_editors.cpp` needed for draw_any itself, but the Color editor's `draw_any` works automatically because the `TypedInspectorEditor<T>::draw_any()` delegates to `draw_fn_()` which handles the "rgb" tag dispatch.

### D. Editor: SetComponentPropertyCommand

Create `src/editor/commands/set_component_property_command.h`:

```cpp
#pragma once

#include "command.h"
#include "editor_context.h"
#include "scene/component_registry/component_info.h"
#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/world.h"

#include "log/log.h"

#include <yaml-cpp/yaml.h>

#include <optional>
#include <string>
#include <string_view>
#include <typeindex>

namespace buddd::editor {

/// Command that modifies a single component property on an entity.
/// Uses YAML-based value transport: the old and new values are stored as YAML nodes,
/// and applied via ComponentInfoBase::property_deserialize().
class SetComponentPropertyCommand final : public Command {
public:
    SetComponentPropertyCommand(
        buddd::engine::EntityId entity_id,
        std::string component_type_name,
        std::string property_name,
        YAML::Node old_value,
        YAML::Node new_value)
        : entity_id_(entity_id)
        , component_type_name_(std::move(component_type_name))
        , property_name_(std::move(property_name))
        , old_value_(std::move(old_value))
        , new_value_(std::move(new_value))
    {}

    auto execute(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();
        auto entity = world.entity(entity_id_);

        // Check if entity is valid
        if (entity.id() == buddd::engine::EntityId::none()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand: entity {} not found",
                entity_id_.index);
            return;
        }

        // Resolve component_type_name_ to ComponentInfoBase
        auto& registry = ctx.engine.services.registry();
        const auto* info = registry.describe(component_type_name_);
        if (!info) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand: component type '{}' not registered",
                component_type_name_);
            return;
        }

        // Build type_index from info using SceneSaver pattern:
        // create() a temporary instance and extract typeid(*tmp).
        auto tmp = const_cast<buddd::engine::ComponentInfoBase*>(info)->create();
        auto target_type = std::type_index(typeid(*tmp));

        // Find the component on the entity by matching type_index
        std::optional<size_t> component_index;
        for (size_t i = 0; i < entity.component_count(); ++i) {
            auto& comp = entity.component_at(i);
            if (std::type_index(typeid(comp)) == target_type) {
                component_index = i;
                break;
            }
        }

        if (!component_index.has_value()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand: component '{}' not found on entity {}",
                component_type_name_, entity_id_.index);
            return;
        }

        // Find the property index by name
        std::optional<size_t> prop_index;
        for (size_t j = 0; j < info->property_count(); ++j) {
            if (info->property_name(j) == property_name_) {
                prop_index = j;
                break;
            }
        }

        if (!prop_index.has_value()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand: property '{}' not found on component '{}'",
                property_name_, component_type_name_);
            return;
        }

        // Read current value — for redundancy check
        auto ser_ctx = buddd::engine::SerializationContext{ctx.engine.services.assets()};
        auto current_yaml = info->property_serialize(entity.component_at(*component_index), *prop_index, ser_ctx);

        // If current value already matches new_value, no-op
        if (current_yaml == new_value_) {
            return;
        }

        // Apply new value
        auto result = info->property_deserialize(entity.component_at(*component_index), *prop_index, new_value_, ser_ctx);
        if (!result) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand: property_deserialize failed for '{}' on '{}': {}",
                property_name_, component_type_name_, result.error().message);
            // Still mark dirty — the write was attempted
        }

        ctx.editor.mark_dirty();

        BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
            "SetComponentProperty: entity={} comp={} prop={}", entity_id_.index, component_type_name_, property_name_);
    }

    auto undo(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();
        auto entity = world.entity(entity_id_);

        if (entity.id() == buddd::engine::EntityId::none()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand UNDO: entity {} not found",
                entity_id_.index);
            return;
        }

        // Resolve component_type_name_ to ComponentInfoBase
        auto& registry = ctx.engine.services.registry();
        const auto* info = registry.describe(component_type_name_);
        if (!info) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand UNDO: component type '{}' not registered",
                component_type_name_);
            return;
        }

        // Build type_index from info using SceneSaver pattern
        auto tmp = const_cast<buddd::engine::ComponentInfoBase*>(info)->create();
        auto target_type = std::type_index(typeid(*tmp));

        // Find the component on the entity by matching type_index
        std::optional<size_t> component_index;
        for (size_t i = 0; i < entity.component_count(); ++i) {
            auto& comp = entity.component_at(i);
            if (std::type_index(typeid(comp)) == target_type) {
                component_index = i;
                break;
            }
        }

        if (!component_index.has_value()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand UNDO: component '{}' not found on entity {}",
                component_type_name_, entity_id_.index);
            return;
        }

        // Find the property index by name
        std::optional<size_t> prop_index;
        for (size_t j = 0; j < info->property_count(); ++j) {
            if (info->property_name(j) == property_name_) {
                prop_index = j;
                break;
            }
        }

        if (!prop_index.has_value()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand UNDO: property '{}' not found on component '{}'",
                property_name_, component_type_name_);
            return;
        }

        // Write old value
        auto ser_ctx = buddd::engine::SerializationContext{ctx.engine.services.assets()};
        auto result = info->property_deserialize(entity.component_at(*component_index), *prop_index, old_value_, ser_ctx);
        if (!result) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand UNDO: property_deserialize failed for '{}' on '{}': {}",
                property_name_, component_type_name_, result.error().message);
        }

        ctx.editor.mark_dirty();

        BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
            "SetComponentProperty UNDO: entity={} comp={} prop={}", entity_id_.index, component_type_name_, property_name_);
    }

    [[nodiscard]] auto name() const -> std::string_view override {
        return "Set Component Property";
    }

private:
    buddd::engine::EntityId entity_id_;
    std::string component_type_name_;
    std::string property_name_;
    YAML::Node old_value_;
    YAML::Node new_value_;
};

} // namespace buddd::editor
```

**Key behavior notes**:
- `execute()` re-reads the current value and compares to `new_value_` — skips if they match (redundancy guard).
- `undo()` writes `old_value_` (the value stored at command construction time).
- **Component resolution uses the SceneSaver `typeid` pattern**: `registry.describe(component_type_name_)` returns the `ComponentInfoBase*`; a temporary is created via `info->create()` and `std::type_index(typeid(*tmp))` is used to find the matching `Component&` on the entity by comparing against `std::type_index(typeid(comp))` for each component.
- `std::optional<size_t>` is used for `component_index` and `prop_index` instead of a sentinel value.
- `SerializationContext` constructed from `ctx.engine.services.assets()` — same pattern as existing serialization code.
- No `#include <yaml-cpp/yaml.h>` needed in header beyond the declaration; the header uses `YAML::Node` by value (it's fully defined in yaml-cpp). Include `<yaml-cpp/yaml.h>` in the header for the `YAML::Node` member types.

### E. Editor: PropertiesPanel — add draw_component_sections()

In `src/editor/panels/properties_panel.h`:

**Add helper method declarations** (in the private section, after `draw_transform_section`):

```cpp
auto draw_component_sections(EditorContext const& ctx, buddd::engine::EntityId entity_id) -> void;
```

**No new members needed** — the type_index → `ComponentInfoBase*` map is built as a local `std::unordered_map` each frame in `draw_component_sections()` (typically <20 registered component types, so fresh allocation is negligible).

In `src/editor/panels/properties_panel.cpp`:

**Update `draw_ui()`**: After `draw_transform_section(ctx, entity_id);`, add:

```cpp
// ── Component sections ──
draw_component_sections(ctx, entity_id);
```

**Implement `draw_component_sections()`**:

```cpp
auto PropertiesPanel::draw_component_sections(EditorContext const& ctx,
                                               buddd::engine::EntityId entity_id) -> void {
    auto& world = ctx.editor.world();
    auto entity = world.entity(entity_id);
    if (entity.id() == buddd::engine::EntityId::none()) return;

    auto& registry = ctx.engine.services.registry();
    auto& assets = ctx.engine.services.assets();

    // Build type_index → ComponentInfoBase* map (SceneSaver pattern).
    // Rebuilt each frame — cheap (<20 registered component types).
    std::unordered_map<std::type_index, const buddd::engine::ComponentInfoBase*> type_to_info;
    for (const auto* info : registry.all_types()) {
        auto* mutable_info = const_cast<buddd::engine::ComponentInfoBase*>(info);
        auto tmp = mutable_info->create();
        type_to_info[std::type_index(typeid(*tmp))] = info;
    }

    size_t component_count = entity.component_count();

    // Log when selection changes (first draw or new entity)
    static buddd::engine::EntityId last_logged_entity = buddd::engine::EntityId::none();
    if (entity_id != last_logged_entity) {
        BUDDD_LOG_TAGGED_DEBUG("Editor:Properties",
            "Showing entity {} with {} components", entity_id.index, component_count);
        last_logged_entity = entity_id;
    }

    // Separator before first component section
    if (component_count > 0) {
        ImGui::Separator();
    }

    for (size_t i = 0; i < component_count; ++i) {
        auto& comp = entity.component_at(i);

        // Look up ComponentInfoBase* by type_index (keyed on the actual Component subclass)
        auto it = type_to_info.find(std::type_index(typeid(comp)));
        if (it == type_to_info.end()) {
            BUDDD_LOG_TAGGED_DEBUG("Editor:Properties",
                "Skipping component at index {} — no ComponentInfoBase found for type",
                i);
            continue;
        }
        const auto* info = it->second;

        auto type_name = info->type_name();
        size_t prop_count = info->property_count();

        BUDDD_LOG_TAGGED_DEBUG("Editor:Properties",
            "Drawing component section '{}' ({} properties)", type_name, prop_count);

        // Collapsible header — default closed
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));
        bool open = ImGui::CollapsingHeader(type_name.data(), ImGuiTreeNodeFlags_None);
        ImGui::PopStyleVar();

        if (!open) continue;

        if (prop_count == 0) {
            // Centered "No editable properties" text in disabled style
            auto avail = ImGui::GetContentRegionAvail();
            auto text_size = ImGui::CalcTextSize("No editable properties");
            ImGui::SetCursorPosX((avail.x - text_size.x) * 0.5f);
            ImGui::TextDisabled("No editable properties");
            continue;
        }

        // 2-column table matching Transform section layout
        constexpr int COLUMNS = 2;
        ImGui::Indent(4.0f);  // slight indent for visual hierarchy
        if (ImGui::BeginTable("##prop_table", COLUMNS, ImGuiTableFlags_None)) {
            // Column 0: fixed width based on longest property name
            float max_label_width = 60.0f;  // minimum width
            for (size_t j = 0; j < prop_count; ++j) {
                float w = ImGui::CalcTextSize(info->property_name(j).data()).x;
                if (w > max_label_width) max_label_width = w;
            }
            ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed,
                                    max_label_width + 12.0f);
            ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthStretch);

            auto ser_ctx = buddd::engine::SerializationContext{assets};

            for (size_t j = 0; j < prop_count; ++j) {
                auto prop_name = info->property_name(j);
                auto prop_type = info->property_type_index(j);
                auto prop_flags = info->property_flags(j);

                // Read current value as YAML
                auto yaml_node = info->property_serialize(comp, j, ser_ctx);

                // Decode YAML to std::any
                auto any_result = buddd::engine::TypeRegistry::yaml_decode(prop_type, yaml_node, ser_ctx);
                if (!any_result) {
                    BUDDD_LOG_TAGGED_WARN("Editor:ComponentProperties",
                        "Failed to decode property '{}': {}",
                        prop_name, any_result.error().message);
                    continue;
                }

                // Map PropertyFlags to EditorFlags
                EditorFlags editor_flags;
                editor_flags.min_value = prop_flags.min_value;
                editor_flags.max_value = prop_flags.max_value;
                editor_flags.step_value = prop_flags.step_value;
                editor_flags.tags_ = prop_flags.tags_;

                // Draw the editor
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(prop_name.data());
                ImGui::TableSetColumnIndex(1);

                bool changed = InspectorTypeEditorRegistry::draw_any(
                    std::string(prop_name), *any_result, prop_type, editor_flags, ctx);

                if (changed) {
                    // Encode back to YAML
                    auto new_yaml = buddd::engine::TypeRegistry::yaml_encode(prop_type, *any_result, ser_ctx);
                    if (!new_yaml) {
                        BUDDD_LOG_TAGGED_WARN("Editor:ComponentProperties",
                            "Failed to encode property '{}' after edit: {}",
                            prop_name, new_yaml.error().message);
                        continue;
                    }

                    // Create and execute command
                    auto cmd = std::make_unique<SetComponentPropertyCommand>(
                        entity_id,
                        std::string(type_name),
                        std::string(prop_name),
                        yaml_node,
                        std::move(*new_yaml)
                    );
                    ctx.editor.command_stack().execute(std::move(cmd), ctx);
                }
            }

            ImGui::EndTable();
        }
        ImGui::Unindent(4.0f);
    }
}
```

**Includes for `properties_panel.cpp`**: Add `#include "commands/set_component_property_command.h"`, `#include <typeindex>`, and `#include <unordered_map>` (needed for the `type_index` lookup map in `draw_component_sections()`).

**Performance note**: The YAML encode/decode round-trip happens once per property per frame. This is acceptable for MVP1 (typically <20 properties per frame).

### F. Type_index map building (SceneSaver pattern)

As described in `draw_component_sections()`, the `type_index → ComponentInfoBase*` map is rebuilt each frame using the established SceneSaver pattern (see `scene_saver.cpp` lines 73–86): iterate `registry.all_types()`, `const_cast` each pointer, call `info->create()` to produce a temporary `Component` instance, then store `typeid(*tmp) → info` in a `std::unordered_map<std::type_index, const ComponentInfoBase*>`. The per-component lookup is a direct O(1) `unordered_map::find(typeid(comp))` — no linear scan needed. This is safe and efficient for the small number of registered component types (<20).

### G. Registration

The `register_builtin_inspector_editors()` function is already called from `Editor::setup()` (confirmed by F-05 implementation). The Color editor is already registered among the 9 built-in editors. No changes are needed to registration.

## Required tests

### Unit tests — `tests/engine/component_info_property_serialize_tests.cpp` (new file)

1. **`TEST_CASE("ComponentInfoBase: property_serialize round-trip", "[engine][component-registry]")`**:
   - Register a CameraComponent via ComponentRegistry.
   - Create a CameraComponent instance with known fov_y=1.05f.
   - Call `info->property_serialize(comp, 0, ctx)` for fov_y.
   - Verify returned YAML::Node is a float scalar ≈ 1.05.
   - AC-01, AC-02.

2. **`TEST_CASE("ComponentInfoBase: property_deserialize modifies component", "[engine][component-registry]")`**:
   - Register CameraComponent, create instance with default fov_y.
   - Call `info->property_deserialize(comp, 0, YAML::Node(2.0), ctx)`.
   - Verify component's fov_y is now ≈ 2.0.
   - AC-03.

3. **`TEST_CASE("ComponentInfoBase: property_serialize with out-of-bounds index", "[engine][component-registry]")`**:
   - Register CameraComponent, serialize with index = 99.
   - Verify returned YAML::Node is null (default-constructed).
   - AC-02 edge case.

4. **`TEST_CASE("ComponentInfoBase: property_deserialize with out-of-bounds index returns error", "[engine][component-registry]")`**:
   - Register CameraComponent, deserialize with index = 99.
   - Verify Result<void> is an error.
   - AC-03 edge case.

### Unit tests — `tests/engine/type_registry_type_erased_tests.cpp` (new file)

5. **`TEST_CASE("TypeRegistry: yaml_encode(type_index, any) for registered type", "[engine][component-registry]")`**:
   - After `register_builtin_types()`, call `yaml_encode(type_index(float), any(3.14f), ctx)`.
   - Verify Result contains YAML::Node with float value ≈ 3.14.
   - AC-04.

6. **`TEST_CASE("TypeRegistry: yaml_encode returns error for unregistered type", "[engine][component-registry]")`**:
   - Call `yaml_encode(type_index(double), any(1.0), ctx)` (double is not registered).
   - Verify Result is an error.
   - AC-05.

7. **`TEST_CASE("TypeRegistry: yaml_encode returns error on type mismatch", "[engine][component-registry]")`**:
   - Call `yaml_encode(type_index(float), any(std::string("hello")), ctx)`.
   - Verify Result is an error.
   - AC-06.

8. **`TEST_CASE("TypeRegistry: yaml_decode(type_index, node) for registered type", "[engine][component-registry]")`**:
   - Call `yaml_decode(type_index(float), YAML::Node(2.71f), ctx)`.
   - Verify Result contains std::any holding float ≈ 2.71.
   - AC-07.

9. **`TEST_CASE("TypeRegistry: yaml_decode returns error for unregistered type", "[engine][component-registry]")`**:
   - Call `yaml_decode(type_index(double), YAML::Node(1.0), ctx)`.
   - Verify Result is an error.
   - AC-08.

### Unit tests — `tests/editor/component_property_commands_tests.cpp` (new file)

10. **`TEST_CASE("SetComponentPropertyCommand: compile check", "[editor][component-properties]")`**:
    - Verify `SetComponentPropertyCommand` exists with the expected constructor signature.
    - Create a command instance with mock values.
    - Verify `name()` returns non-empty string_view.
    - AC-14.

11. **`TEST_CASE("SetComponentPropertyCommand: execute writes new value and marks dirty", "[editor][component-properties]")`**:
    - Use `TestContext` from inspector_editors_tests.cpp (or similar).
    - Get the editor's World, add an entity with a CameraComponent.
    - Verify initial fov_y.
    - Create and execute `SetComponentPropertyCommand` with old_value=current_fov_y, new_value=2.0f, property="fov_y".
    - Verify entity's component now has fov_y ≈ 2.0.
    - Verify `editor.is_dirty()` returns true.
    - AC-15.

12. **`TEST_CASE("SetComponentPropertyCommand: undo reverts to old value", "[editor][component-properties]")`**:
    - Same setup as AC-15 test.
    - Execute command, then undo.
    - Verify fov_y reverts to original value.
    - AC-16.

13. **`TEST_CASE("SetComponentPropertyCommand: execute is safe with invalid entity", "[editor][component-properties]")`**:
    - Create a command with EntityId(999, 0) (invalid).
    - Call execute — verify no crash (log warning is acceptable).
    - AC-17.

14. **`TEST_CASE("SetComponentPropertyCommand: execute is safe with missing component", "[editor][component-properties]")`**:
    - Create entity without CameraComponent.
    - Create command for "camera" component type.
    - Call execute — verify no crash (log warning is acceptable).
    - AC-18.

15. **`TEST_CASE("SetComponentPropertyCommand: execute no-op when value already matches", "[editor][component-properties]")`**:
    - Set up entity with CameraComponent, fov_y = 1.05.
    - Create command with new_value = 1.05 (same as current).
    - Execute — verify fov_y unchanged (still 1.05).
    - AC-24 (no redundant commands).

16. **`TEST_CASE("InspectorTypeEditorRegistry: draw_any dispatches to registered editor", "[editor][inspector]")`**:
    - Register mock editor for int.
    - Call `draw_any("label", any(int_val), type_index(int), flags, ctx)`.
    - Verify returns true (mock always returns true).
    - AC-12.

17. **`TEST_CASE("InspectorTypeEditorRegistry: draw_any falls back to read-only", "[editor][inspector]")`**:
    - Call `draw_any("label", any(1.0), type_index(double), flags, ctx)` with no editor for double.
    - Verify returns false (no edit possible).
    - AC-13.

18. **`TEST_CASE("TypedInspectorEditor: draw_any extracts typed value", "[editor][inspector]")`**:
    - Create `TypedInspectorEditor<int>` with a mock that records the value.
    - Call `draw_any("label", any(42), type_index(int), flags, ctx)`.
    - Verify the mock received value 42.
    - AC-10.

19. **`TEST_CASE("TypedInspectorEditor: draw_any returns false on type mismatch", "[editor][inspector]")`**:
    - Create `TypedInspectorEditor<int>`.
    - Call `draw_any("label", any(3.14f), type_index(int), flags, ctx)` (any holds float, not int).
    - Verify returns false.
    - AC-11.

### Integration / behavioral tests

20. **`TEST_CASE("PropertiesPanel: component sections render for entities with components", "[editor][component-properties]")`**:
    - Create TestContext with Editor and World.
    - Add entity with CameraComponent and PointLightComponent.
    - Verify `entity.component_count() >= 2`.
    - Call `panel.draw_ui(ctx)` — verify no crash.
    - AC-19 (basic rendering — full ImGui snapshot deferred to manual test).

21. **`TEST_CASE("Component ordering matches component_at order", "[editor][component-properties]")`**:
    - Add CameraComponent then PointLightComponent to entity.
    - Verify using `typeid`: `std::type_index(typeid(entity.component_at(0))) == std::type_index(typeid(CameraComponent))` and `std::type_index(typeid(entity.component_at(1))) == std::type_index(typeid(PointLightComponent))`.
    - AC-25.

### E2E / Integration verification

- **Manual smoke test**: Run `buddd edit` with a scene that has entities with components (Camera, PointLight, MeshRenderer). Select an entity and verify:
  1. Component sections appear collapsed below Transform.
  2. Expanding a section shows properties in a 2-column table.
  3. Editing a float property (e.g., fov_y) changes the viewport and marks scene dirty.
  4. Editing a color property shows ColorEdit3 picker (for "rgb" tagged properties).
  5. Ctrl+Z undoes the property change.
  6. Switching selection switches component sections correctly.
- **Build verification**: `cmake --build --preset debug` with zero new warnings from `src/editor/`, `src/engine/scene/component_registry/`, and `tests/`.
- **Test execution**: `./build/debug/buddd_tests [editor][component-properties]`, `./build/debug/buddd_tests [engine][component-registry]`, `./build/debug/buddd_tests [editor][inspector]` all pass.
- **All existing tests still pass**: `./build/debug/buddd_tests` — verify AC-29, AC-30.

## Edge cases

| Case | Required handling |
|---|---|
| **Entity with no components** (`component_count() == 0`) | `draw_component_sections()` finds no components, renders no sections. Only Transform section shown. |
| **Entity with 10+ components** | All sections render in order, each collapsed by default. `component_info_cache_` rebuilt each frame (cheap). |
| **Component with zero properties** | Section header renders. Body shows "No editable properties" centered and disabled. |
| **Property type not registered in TypeRegistry** | `yaml_decode()` returns error. Property is skipped (no editor widget shown). Warning logged. |
| **Property type has no InspectorTypeEditor** | `InspectorTypeEditorRegistry::draw_any()` falls back to `draw_fallback_readonly()` — shows "(no editor for type \<name\>)" in disabled text. |
| **std::any type mismatch during encode** | `yaml_encode()` returns error. Property change not applied. Warning logged. |
| **Entity destroyed while editing a property** | Next frame: entity lookup fails → panel shows "No entity selected" (existing behavior). In-progress ImGui edit terminated at frame boundary. |
| **Scene switch while editing** | Selection clears → panel shows "No entity selected". In-progress edit terminated. |
| **Rapid undo/redo of component edits** | Each `SetComponentPropertyCommand` stores YAML snapshots. Undo/redo is O(1) per command. |
| **CommandStack overflow (128 entries)** | Oldest commands discarded (existing CommandStack behavior). No special handling needed. |
| **PropertyFlags have extreme values** (min = -FLT_MAX, max = FLT_MAX) | EditorFlags maps these directly. DragFloat handles extreme ranges natively. |
| **Color property without "rgb" tag** | Color editor's draw_any delegates to `ColorEdit4` (with alpha). Handled by existing Color editor logic. |
| **Entity with only Transform (no components)** | `component_count() == 0`. No component sections rendered. No separator drawn. |
| **Rapid consecutive edits to same property** | Each end-of-drag produces one `SetComponentPropertyCommand`. The YAML comparison in execute prevents redundant commands when values haven't changed between frames. |

## Security impact

None. No file I/O, no authentication, no network access. Input validation is handled by ImGui (DragFloat clamps to float range) and by Property::deserialize (min/max checks in ComponentInfo<T>). All edits are to in-memory data only.

## Data and migration impact

None. No schema changes, no migrations, no seed data changes. The new `property_serialize`/`property_deserialize` methods are additive (virtual methods on `ComponentInfoBase`) — existing component registrations compile without changes because `ComponentInfo<T>` automatically provides the implementations via the template override.

## API compatibility impact

- **`ComponentInfoBase`**: Two new pure virtual methods (`property_serialize`, `property_deserialize`). All existing `ComponentInfo<T>` subclasses must implement these. Since `ComponentInfo<T>` is a template class defined in the same header, the implementation is provided in the template and all existing registrations automatically gain it. No existing code changes needed.
- **`ComponentInfo<T>`**: Two new overrides. No existing code affected.
- **`TypeRegistry`**: Two new static methods (`yaml_encode(type_index, any, ctx)` and `yaml_decode(type_index, node, ctx)`). Additive — no existing code affected.
- **`TypeEntry`**: Two new optional members (`yaml_encode_any`, `yaml_decode_any`). Backward-compatible — existing `TypeEntry` objects have these as nullopt until `register_type<T>()` is called. Since `register_type<T>()` now populates them automatically, all existing registrations are covered.
- **`InspectorTypeEditor`**: One new virtual method (`draw_any`) with default implementation returning false. Additive — all existing subclasses compile without changes.
- **`TypedInspectorEditor<T>`**: One new override (`draw_any`). Additive.
- **`InspectorTypeEditorRegistry`**: One new static method (`draw_any`). Additive.
- **`PropertiesPanel`**: One new private method (`draw_component_sections`). Additive — no public API changes.
- **`SetComponentPropertyCommand`**: New class. Additive.

## Documentation impact

- **README**: None.
- **Wiki pages** (to be updated by wiki-agent):
  - `docs/wiki/editor/editor-panels.md`: Update Properties Panel section to document component section rendering, `draw_any()` flow, and `SetComponentPropertyCommand`. Document the Property Editors mapping table (PropertyFlags → EditorFlags, type → editor widget).
  - `docs/wiki/editor/cross-panel-communication.md`: Update to reflect that component property edits now use Commands (`SetComponentPropertyCommand`) with YAML-based value transport.
  - `docs/wiki/domain/glossary.md`: Add `SetComponentPropertyCommand`, `draw_any()`, `property_serialize`/`property_deserialize`.
  - `docs/wiki/architecture/module-map.md`: Update Editor section to include `set_component_property_command.h` as new file. Update Engine section to note `ComponentInfoBase` extension and `TypeRegistry` extension.
- **Other specs**: The north-star UX spec at `.specs/sprint-2026-06/editor-ux-design/spec.md` should be updated to reflect component sections default to collapsed (deviation D-01) and no Add/Remove Component buttons in this sprint (deviation D-02).

## ADR impact

No new ADR is required. The implementation follows established patterns (extending `ComponentInfoBase` per ADR-028, extending `TypeRegistry` per ADR-028, adding a new Command per the Command pattern from ADR-027, using ImGui per ADR-026). No architectural decisions are changed.

## Done criteria

| # | Criterion | Verification |
|---|---|---|
| 1 | `ComponentInfoBase` gains `property_serialize()` and `property_deserialize()` pure virtual methods | Code review: declarations present in `component_info.h` in `ComponentInfoBase` class |
| 2 | `ComponentInfo<T>::property_serialize()` delegates to `Property::serialize()` for the given index | Code review: implementation in `component_info.h` calls `properties_[index].serialize()` |
| 3 | `ComponentInfo<T>::property_deserialize()` delegates to `Property::deserialize()` for the given index, returns error on out-of-bounds | Code review: implementation in `component_info.h` calls `properties_[index].deserialize()` |
| 4 | `TypeRegistry::yaml_encode(type_index, any, ctx)` declared in `type_registry.h` and implemented in `type_registry.cpp` | Code review: declaration as static method, implementation via dispatch function lookup |
| 5 | `TypeRegistry::yaml_decode(type_index, node, ctx)` declared and implemented | Code review: same pattern |
| 6 | `TypeEntry` struct extended with `yaml_encode_any` and `yaml_decode_any` dispatch functions | Code review: members present in `TypeEntry` |
| 7 | `register_type<T>()` populates `yaml_encode_any` and `yaml_decode_any` dispatch functions | Code review: lambdas assigned in template body |
| 8 | `InspectorTypeEditor` gains virtual `draw_any()` method | Code review: declaration in `inspector_editors.h` |
| 9 | `TypedInspectorEditor<T>::draw_any()` extracts T from std::any and delegates to draw_fn_ | Code review: `std::any_cast<T>(&value)`, calls `draw_fn_()` |
| 10 | `InspectorTypeEditorRegistry::draw_any()` dispatches by type_index, falls back to read-only on miss | Code review: calls `get(type_index)`, then `editor->draw_any()` or `draw_fallback_readonly()` |
| 11 | `src/editor/commands/set_component_property_command.h` created with correct class | File exists, compiles; constructor takes (entity_id, component_type_name, property_name, old_value, new_value) |
| 12 | `SetComponentPropertyCommand::execute()` writes new_value via `property_deserialize()`, re-reads current for dedup | Code review: reads current via `property_serialize()`, compares to `new_value_`, writes via `property_deserialize()` |
| 13 | `SetComponentPropertyCommand::undo()` writes old_value via `property_deserialize()` | Code review: undo body uses `old_value_` |
| 14 | `PropertiesPanel` gains `draw_component_sections()` helper | Code review: declaration in header, implementation in .cpp |
| 15 | `draw_ui()` calls `draw_component_sections()` after `draw_transform_section()` | Code review: call present in `properties_panel.cpp` |
| 16 | Component sections use `ImGui::CollapsingHeader` with `ImGuiTreeNodeFlags_None` (default collapsed) | Code review: no `DefaultOpen` flag on component headers |
| 17 | Properties in each section render in a 2-column `ImGui::Table` | Code review: `BeginTable("##prop_table", 2, ...)` with label column + value column |
| 18 | PropertyFlags min/max/step/tags are mapped to EditorFlags | Code review: field-by-field copy in `draw_component_sections()` |
| 19 | Zero-property components show "No editable properties" in disabled text | Code review: check `prop_count == 0` before table rendering |
| 20 | `tests/engine/component_info_property_serialize_tests.cpp` created with 4 test cases | File exists, tests for AC-01, AC-02, AC-03 |
| 21 | `tests/engine/type_registry_type_erased_tests.cpp` created with 5 test cases | File exists, tests for AC-04, AC-05, AC-06, AC-07, AC-08 |
| 22 | `tests/editor/component_property_commands_tests.cpp` created with 10+ test cases | File exists, tests for AC-10 to AC-18, AC-24, AC-25 |
| 23 | All tests pass: `./build/debug/buddd_tests [editor][component-properties]`, `[editor][inspector]`, `[engine][component-registry]` | Test runner exits with 0 |
| 24 | Build succeeds with zero new warnings from `src/editor/`, `src/engine/scene/component_registry/`, and `tests/` | `cmake --build --preset debug` produces no warnings in affected directories |
| 25 | All existing tests still pass | `./build/debug/buddd_tests` exits with 0 (AC-29) |
