# SPEC-F-06 — Inspector — Component Properties

## Problem

The Properties Panel currently shows only the Transform section (F-05 + F-06) and entity name. When a user selects an entity that has additional components (Camera, PointLight, MeshRenderer, etc.), there is no way to inspect or edit those components' properties. The `InspectorTypeEditorRegistry` provides reusable editors for built-in types (float, int, bool, string, Vec3, Quat, Color, etc.), but no code iterates the entity's components, no per-property editing loop exists, and there is no `SetComponentPropertyCommand` for undo/redo of component property edits.

Additionally, the `InspectorTypeEditorRegistry` only supports typed `draw<T>()` calls where the caller knows the type at compile time. For runtime type dispatch (component properties are type-erased via `type_index`), a `draw_any()` mechanism is needed that accepts `std::any` + `type_index` and delegates to the correct registered editor.

## Goals

| ID | Goal |
|---|---|
| G-01 | **Component section rendering**: Each component attached to the selected entity renders as a collapsible section under the Transform section, in the order returned by `Entity::component_at()`. |
| G-02 | **Per-property editing**: Each property within a component section renders using the appropriate editor from `InspectorTypeEditorRegistry`, matching the 2-column table layout (property name \| value) used by the Transform section. |
| G-03 | **Runtime type dispatch**: `InspectorTypeEditorRegistry` gains a `draw_any(label, std::any&, type_index, flags, ctx)` method that looks up the registered editor by `type_index` and delegates, enabling the Properties Panel to edit properties without knowing their C++ type at compile time. |
| G-04 | **SetComponentPropertyCommand**: A new Command that stores entity_id, component type name, property name, old YAML value, and new YAML value. Supports undo/redo by writing the old/new YAML value back via `ComponentInfoBase` property serialization methods. |
| G-05 | **Engine: ComponentInfoBase single-property access**: Add `property_serialize(comp, idx, ctx) -> YAML::Node` and `property_deserialize(comp, idx, node, ctx) -> Result<void>` virtual methods to `ComponentInfoBase`, implemented by `ComponentInfo<T>` delegating to the Property's `serialize()`/`deserialize()` methods. |
| G-06 | **Engine: TypeRegistry type-erased YAML encode/decode**: Add `yaml_encode(type_index, std::any, ctx) -> Result<YAML::Node>` and `yaml_decode(type_index, YAML::Node, ctx) -> Result<std::any>` type-erased convenience methods to `TypeRegistry`. |
| G-07 | **color tag → ColorEdit3**: Properties with `PropertyFlags::tag("rgb")` render with `ImGui::ColorEdit3` (3-channel color picker). Properties with no `"rgb"` tag but `math::Color` type render with `ImGui::ColorEdit4` (4-channel with alpha). |
| G-08 | **Non-regression**: All existing tests pass. Zero new warnings from `src/editor/`, `src/engine/scene/component_registry/`, and `tests/`. |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | **No Add Component button** — removed from scope. Users cannot add new components to an entity from the Properties Panel. |
| NG-02 | **No Remove Component button** — removed from scope. Users cannot remove components from an entity from the Properties Panel. |
| NG-03 | **No Play-mode read-only enforcement** — deferred to F-15. Component sections and their properties are interactive regardless of play state. |
| NG-04 | **No multi-select editing** — component sections shown only for `primary()` entity. Simultaneous editing of multiple entities is deferred. |
| NG-05 | **No reordering of component sections** — sections appear in the fixed order of `Entity::component_at()`. |
| NG-06 | **No component search/filter** — no search bar for component sections in this feature. |
| NG-07 | **No per-component undo grouping** — each property edit creates a separate `SetComponentPropertyCommand`. Undo/redo granularity is per-property, not per-component-section. |
| NG-08 | **No custom editors for non-built-in types** — only types with a registered `InspectorTypeEditor` or a `TypeRegistry` entry can be edited. Unregistered types fall back to read-only display. |
| NG-09 | **No changes to existing Transform section** — the Transform section (Position/Rotation/Scale in 2-column table) remains unchanged from F-06. |
| NG-10 | **No changes to entity name field** — the editable entity name field at the top remains unchanged. |
| NG-11 | **No drag-and-drop asset references** — deferred to a future feature. |

### Differences from north-star UX spec

| # | Deviation | Rationale |
|---|---|---|
| D-01 | **Component sections default to collapsed** (not expanded). The north-star UX spec specifies "Expanded by default for 1–2 components; collapsed for 3+." | This feature defaults ALL component sections to collapsed for simplicity and consistency with the initial implementation. The expansion behavior may be refined in a follow-up. |
| D-02 | **No Add/Remove Component buttons** — the north-star UX spec includes both. | Removed from scope as instructed by the initial feature brief. These will be added in a future sprint. |

## Actors

| Actor | Description |
|---|---|
| **Editor user** | Opens the editor, selects an entity in the Scene Panel, sees the Properties Panel populate with Transform + collapsible component sections. Edits component properties (float, int, bool, color, etc.) and observes changes in the viewport (e.g., changing light color or camera FOV). |
| **Engine developer** | Adds new component types with properties. No editor changes are needed for basic property editing — the `InspectorTypeEditorRegistry` and `SetComponentPropertyCommand` generically support any type registered in `TypeRegistry` and `ComponentRegistry`. |
| **Serialization author** | Implements `property_serialize`/`property_deserialize` on `ComponentInfoBase` for single-property YAML round-trip. This is the transport layer for Command undo/redo. |

## User-visible behavior

### A. Component section rendering

When an entity is selected and has at least one component (beyond the implicit Transform), the Properties Panel renders a collapsible section for each component below the always-expanded Transform section.

```
▼ Transform (always expanded, first — unchanged from F-06)
  Position  [■ X][  0.00][■ Y][  0.00][■ Z][  0.00]
  Rotation  [■ X][  0.00][■ Y][  0.00][■ Z][  0.00]
  Scale     [■ X][  1.00][■ Y][  1.00][■ Z][  1.00]
─────────────── separator ───────────────────
▶ camera (collapsible, default closed)
▶ point_light (collapsible, default closed)
▶ mesh_renderer (collapsible, default closed)
```

**Section header**:
- Uses `ImGui::CollapsingHeader` with **default closed** state (`ImGuiTreeNodeFlags_None`).
- Header text = component type name (e.g., "camera", "point_light", "mesh_renderer") as returned by `ComponentInfoBase::type_name()`.
- No icon or remove button on the header.
- A thin `ImGui::Separator()` is drawn between the Transform section and the first component section, and between consecutive component sections.

**Section order**: Components iterate in the order returned by `Entity::component_at()` (which matches the order they were added). The Transform section always remains first and is not part of this iteration.

**Empty component**: If a component has zero properties (`property_count() == 0`), the section header still appears (collapsible) but the body shows centered text "No editable properties" in disabled text style.

### B. Property table (inside each component section)

Each component section contains a 2-column `ImGui::Table` matching the Transform section layout from F-06:

- **Column 0**: Property name (text rendered by the panel, fixed width based on longest property name or a reasonable minimum).
- **Column 1**: Value area — calls the appropriate editor from `InspectorTypeEditorRegistry` via the new `draw_any()` method.

**No column headers**. The table spans the full content region width.

Example (camera section expanded):

```
▼ camera
  fov_y      [  1.05]
  aspect     [  1.78]
  near       [  0.10]
  far       [100.00]
```

### C. Property value transport via YAML

Properties cannot be edited by direct memory access — the ComponentInfo system only provides getter/setter lambdas that produce/consume `YAML::Node` values. Therefore:

1. **Read path**: The panel calls `ComponentInfoBase::property_serialize(comp, idx, ctx)` to get the current property value as a `YAML::Node`.
2. **YAML → typed value**: The panel calls `TypeRegistry::yaml_decode(type_index, node, ctx)` to decode the YAML into a `std::any` containing the typed value.
3. **Edit**: The panel passes the `std::any` to `InspectorTypeEditorRegistry::draw_any(label, any, type_index, flags, ctx)` which dispatches to the typed editor.
4. **Typed value → YAML**: If the value changed, the panel calls `TypeRegistry::yaml_encode(type_index, any, ctx)` to encode back to a `YAML::Node`.
5. **Write path**: A `SetComponentPropertyCommand` is created with the old YAML value, new YAML value, entity_id, component type name, and property name. The command is executed via `CommandStack::execute()`.

**Performance note**: The YAML encode/decode round-trip happens once per property per frame (not once per edit). The value is read from the component, decoded to `std::any`, displayed in ImGui, and on edit, re-encoded to YAML. This is acceptable for MVP1 since the number of component properties per frame is small (typically <20).

### D. Engine changes: ComponentInfoBase single-property access

Two new virtual methods on `ComponentInfoBase`:

```cpp
// In src/engine/scene/component_registry/component_info.h
class ComponentInfoBase {
public:
    // ... existing methods ...

    /// Serialize a single property by index from the component to a YAML node.
    [[nodiscard]] virtual auto property_serialize(
        const Component& comp, size_t index,
        const SerializationContext& ctx) const -> YAML::Node = 0;

    /// Deserialize a single property by index from a YAML node into the component.
    [[nodiscard]] virtual auto property_deserialize(
        Component& comp, size_t index,
        const YAML::Node& node,
        const SerializationContext& ctx) const -> Result<void> = 0;
};
```

Implemented in `ComponentInfo<T>`:

```cpp
template<typename T>
class ComponentInfo : public ComponentInfoBase {
    // ... existing methods ...

    auto property_serialize(const Component& comp, size_t index,
                            const SerializationContext& ctx) const -> YAML::Node override {
        const auto& typed = static_cast<const T&>(comp);
        return properties_[index].serialize(typed, ctx);
    }

    auto property_deserialize(Component& comp, size_t index,
                              const YAML::Node& node,
                              const SerializationContext& ctx) const -> Result<void> override {
        auto& typed = static_cast<T&>(comp);
        return properties_[index].deserialize(typed, node, ctx);
    }
};
```

**Backward compatibility**: All existing `ComponentInfo<T>` instances automatically gain these methods since the template implementation delegates to the existing `Property::serialize()`/`deserialize()` methods. No changes to existing component registration code are needed.

### E. Engine changes: TypeRegistry type-erased YAML encode/decode

Two new static methods on `TypeRegistry`:

```cpp
// In src/engine/scene/component_registry/type_registry.h
class TypeRegistry {
public:
    // ... existing methods ...

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
};
```

These are implemented in `type_registry.cpp` (not inline in the header) because they need to switch on `type_index` and call the templated `TypeInfo<T>` functions stored in the `TypeEntry` map. The implementation pattern:

1. Look up the `TypeEntry` for the given `type_index`.
2. Use a type-erased dispatch mechanism (e.g., a `std::function` stored alongside the `TypeInfo<T>` in `TypeEntry`, or by storing a `std::type_index` → dispatch function map in the `.cpp`).

**For the contract**: The exact dispatch mechanism is an implementation detail, but the spec requires that:
- `yaml_encode(type_index, any, ctx)` returns a `YAML::Node` or an error.
- `yaml_decode(type_index, node, ctx)` returns a `std::any` containing the decoded value, or an error.
- If the `type_index` is not registered, both return an error.
- If the `std::any` does not hold the expected type for `yaml_encode`, an error is returned.

### F. Editor changes: InspectorTypeEditor draw_any()

The `InspectorTypeEditor` base class gains a new virtual method:

```cpp
// In src/editor/inspector_editors.h
class InspectorTypeEditor {
public:
    // ... existing draw() method ...

    /// Draw the editor for a type-erased value. The `type_index` identifies
    /// the C++ type of the value stored in `value` (a std::any).
    /// Default implementation: extracts the typed value from std::any and
    /// calls draw(). Subclasses may override for more efficient dispatch.
    [[nodiscard]] virtual auto draw_any(const std::string& label,
                                        std::any& value,
                                        std::type_index type_index,
                                        const EditorFlags& flags,
                                        const EditorContext& ctx) -> bool;
};
```

The default implementation in `InspectorTypeEditor` casts the `void*` obtained from `std::any_cast` and delegates to `draw()`. However, since `InspectorTypeEditorRegistry` needs to route by `type_index`, the `draw_any()` method on `InspectorTypeEditor` allows a future advanced editor to handle `std::any` directly.

`TypedInspectorEditor<T>` overrides `draw_any()`:

```cpp
template<typename T>
class TypedInspectorEditor : public InspectorTypeEditor {
public:
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
};
```

`InspectorTypeEditorRegistry` gains a new static method:

```cpp
class InspectorTypeEditorRegistry {
public:
    /// Draw the editor for a type-erased value, dispatching by type_index.
    /// @param label   ImGui label/PushID string (used for scoping).
    /// @param value   Type-erased value to edit (modified in-place).
    /// @param type_index The C++ type of the value.
    /// @param flags   Editor constraints.
    /// @param ctx     Editor context.
    /// @return true if the value was modified.
    [[nodiscard]] static auto draw_any(const std::string& label,
                                       std::any& value,
                                       std::type_index type_index,
                                       const EditorFlags& flags,
                                       const EditorContext& ctx) -> bool;
};
```

Implementation:
1. Looks up the editor for `type_index` via `get(type_index)`.
2. If found: calls `editor->draw_any(label, value, type_index, flags, ctx)`.
3. If NOT found: calls `draw_fallback_readonly(label, type_index, ctx)` and returns `false`.

**Note on `draw_any()` vs `draw()` lifetime**: The `std::any` reference passed to `draw_any()` must remain alive for the duration of the ImGui frame. The panel retains the `std::any` as a per-property transient value during the draw loop.

### G. SetComponentPropertyCommand

A new Command class for undo/redo of component property edits:

```cpp
// In src/editor/commands/set_component_property_command.h
namespace buddd::editor {

class SetComponentPropertyCommand final : public Command {
public:
    SetComponentPropertyCommand(
        buddd::engine::EntityId entity_id,
        std::string component_type_name,   // e.g., "camera"
        std::string property_name,          // e.g., "fov_y"
        YAML::Node old_value,              // YAML value before edit
        YAML::Node new_value               // YAML value after edit
    );

    auto execute(EditorContext const& ctx) -> void override;
    auto undo(EditorContext const& ctx) -> void override;
    [[nodiscard]] auto name() const -> std::string_view override;

private:
    buddd::engine::EntityId entity_id_;
    std::string component_type_name_;
    std::string property_name_;
    YAML::Node old_value_;
    YAML::Node new_value_;
};

} // namespace buddd::editor
```

**Execute behavior**:
1. Look up the entity via `ctx.editor.world().entity(entity_id_)`.
2. If entity is invalid/stale: log warning and return (no crash).
3. Look up the component by `component_type_name_` using internal iteration (iterate `component_at()` until type `ComponentInfoBase::type_name()` matches, or use a `ComponentRegistry` descriptor lookup).
4. If component not found: log error and return.
5. Find the property index by iterating properties until `property_name(size_t)` matches.
6. Read the **current** property value via `property_serialize()` — this becomes the effective old value for the next redo.
7. If the current value already equals `new_value_`: no-op (edit is already applied).
8. Otherwise: write `new_value_` via `property_deserialize()`.
9. Call `ctx.editor.mark_dirty()`.

**Undo behavior**:
1. Same entity/component/property lookup as execute.
2. Write `old_value_` via `property_deserialize()`.
3. Call `ctx.editor.mark_dirty()`.

**Why is old_value_ re-read on execute?** Because the value may have changed between the command's creation and execution (e.g., if another command or external process modified the same property). Re-reading ensures the Command stores the true pre-mutation value. However, the `old_value_` stored in the constructor is used for undo, not the re-read value.

**Important nuance**: On execute(), the command reads the current value via `property_serialize()` and compares it to the stored `new_value_`. If they already match, the command is a no-op (avoids redundant undo entries and infinite loops). The `old_value_` passed to the constructor is used for undo — it represents the value before the user's edit. The re-read value is used only for the redundancy check.

### H. PropertiesPanel: draw_component_sections()

The `PropertiesPanel` gains a new private helper `draw_component_sections()` that iterates all components on the selected entity and renders each as a collapsible section.

**Data flow per component section**:

```
For each component index i in [0, entity.component_count()):
  comp = entity.component_at(i)
  type_name = ComponentRegistry::describe(comp.type_name())->type_name()
  
  // Build a map: type_name → ComponentInfoBase* for quick descriptor lookup
  // This map is built once per draw_ui() frame and reused for all components.
  
  info = component_info_map[comp.type_name()]
  if !info: continue (component type not in registry — skip)

  ImGui::CollapsingHeader(type_name)
  if !open: continue

  ImGui::BeginTable("##prop_table", 2, ...)
  
  for j in [0, info->property_count()):
    prop_name = info->property_name(j)
    flags = info->property_flags(j)
    type_idx = info->property_type_index(j)
    
    // Read current value as YAML
    yaml_node = info->property_serialize(comp, j, ctx)
    
    // Decode YAML to std::any
    any_value = TypeRegistry::yaml_decode(type_idx, yaml_node, ctx)
    if error: continue
    
    // Map PropertyFlags to EditorFlags
    editor_flags = map_flags(flags)
    
    // Draw the editor
    changed = InspectorTypeEditorRegistry::draw_any(prop_name, any_value, type_idx, editor_flags, ctx)
    
    if changed:
      // Encode back to YAML
      new_yaml = TypeRegistry::yaml_encode(type_idx, any_value, ctx)
      if error: continue
      
      // Create and execute command
      cmd = SetComponentPropertyCommand(entity_id, comp_type_name, prop_name, yaml_node, new_yaml)
      ctx.editor.command_stack().execute(cmd, ctx)
```

**Type → Editor mapping**:

| TypeRegistry type | Editor widget | Notes |
|---|---|---|
| `float` | `ImGui::DragFloat` | Uses `flags.min_value`, `flags.max_value`, `flags.step_value` |
| `int32_t` | `ImGui::DragInt` | Uses `flags.min_value`, `flags.max_value` |
| `bool` | `ImGui::Checkbox` | |
| `std::string` | `ImGui::InputText` | |
| `math::Vec3` | 3× axis-colored composite widgets | Same as Transform Position |
| `math::Vec4` | 4× axis-colored composite widgets | Grey W handle |
| `math::Quat` | 3× axis-colored composite widgets (Euler degrees) | Same as Transform Rotation |
| `math::Color` | `ImGui::ColorEdit3` if `flags.has_tag("rgb")`, else `ImGui::ColorEdit4` | Uses the registered Color editor |
| `std::shared_ptr<Model>` | `ImGui::InputText` (fallback via TypeRegistry string round-trip) | Shows asset ID string |

**PropertyFlags → EditorFlags mapping**:

| PropertyFlags field | EditorFlags field |
|---|---|
| `min_value` | `min_value` |
| `max_value` | `max_value` |
| `step_value` | `step_value` |
| `tags_` | `tags_` (for "rgb" tag detection) |

### I. Layout visual reference

```
┌─────────────────────────────────────┐
│ Properties                          │
├─────────────────────────────────────┤
│ [Entity Name]                [input]│
├─────────────────────────────────────┤
│ ▼ Transform                         │
│   Position  [■ X][0.00][■ Y][0.00]…│
│   Rotation  [■ X][0.00][■ Y][0.00]…│
│   Scale     [■ X][1.00][■ Y][1.00]…│
├─────────────────────────────────────┤
│ ▶ camera                            │
├─────────────────────────────────────┤
│ ▶ point_light                       │
├─────────────────────────────────────┤
│ ▶ mesh_renderer                     │
└─────────────────────────────────────┘

When "camera" is expanded:

┌─────────────────────────────────────┐
│ ▼ camera                            │
│   fov_y      [  1.05]               │
│   aspect     [  1.78]               │
│   near       [  0.10]               │
│   far       [100.00]                │
└─────────────────────────────────────┘

When "point_light" is expanded:

┌─────────────────────────────────────┐
│ ▼ point_light                       │
│   color      [■ ColorPicker]        │
│   intensity  [  1.00]               │
│   range      [ 10.00]               │
└─────────────────────────────────────┘
```

### J. Property editing triggers dirty marking

The `SetComponentPropertyCommand` calls `ctx.editor.mark_dirty()` on both execute and undo, ensuring the scene dirty state is correctly tracked for all component property edits.

### K. Selection change behavior

When the user switches selection to another entity (or deselects), the component sections redraw from the new primary entity automatically — the panel re-reads `editor.selection().primary()` and re-builds the UI each frame. Any in-progress ImGui interaction (dragging a DragFloat, editing a ColorEdit) is terminated at the frame boundary (standard ImGui behavior).

## Key entities

### ComponentInfoBase (engine — extended)

```
ComponentInfoBase
├── type_name() -> string_view        (existing)
├── property_count() -> size_t        (existing)
├── property_name(idx) -> string_view (existing)
├── property_type_index(idx) -> type_index  (existing)
├── property_flags(idx) -> PropertyFlags    (existing)
├── serialize(comp, ctx) -> Node     (existing — all properties at once)
├── deserialize(comp, node, ctx)     (existing)
├── property_serialize(comp, idx, ctx) -> Node          ← NEW
└── property_deserialize(comp, idx, node, ctx) -> Result ← NEW
```

### TypeRegistry (engine — extended)

```
TypeRegistry (static)
├── yaml_encode<T>(value, ctx) -> Result<Node>           (existing)
├── yaml_decode<T>(node, ctx) -> Result<T>               (existing)
├── yaml_encode(type_index, any, ctx) -> Result<Node>    ← NEW
└── yaml_decode(type_index, node, ctx) -> Result<any>    ← NEW
```

### InspectorTypeEditor (editor — extended)

```
InspectorTypeEditor (base)
├── draw(label, void*, flags, ctx) -> bool               (existing)
└── draw_any(label, any&, type_index, flags, ctx) -> bool ← NEW
```

### InspectorTypeEditorRegistry (editor — extended)

```
InspectorTypeEditorRegistry (static)
├── register_editor<T>(...)                              (existing)
├── draw<T>(label, value, flags, ctx) -> bool            (existing)
├── has_editor<T>() -> bool                               (existing)
├── get(type_index) -> InspectorTypeEditor*               (existing)
└── draw_any(label, any&, type_index, flags, ctx) -> bool ← NEW
```

### SetComponentPropertyCommand (editor — new)

```
SetComponentPropertyCommand
├── entity_id
├── component_type_name
├── property_name
├── old_value (YAML)
├── new_value (YAML)
├── execute(ctx)                                          ← NEW
├── undo(ctx)                                             ← NEW
└── name() -> string_view                                 ← NEW
```

## User stories

### Story 1 — View and edit camera properties (Priority: P1)

As an editor user, I want to see the camera component's properties (fov_y, aspect, near, far) when I select a camera entity, and edit them to tune the view.

**Given** an entity with a CameraComponent is selected
**When** I view the Properties Panel
**Then** a collapsible "camera" section appears below the Transform section

**Given** the "camera" section is collapsed
**When** I click the "camera" header
**Then** the section expands to show fov_y, aspect, near, and far property rows
**And** each property shows a DragFloat editor with its current value

**Given** the camera section is expanded showing fov_y = 1.05
**When** I drag the fov_y value to 1.57 (approx 90°)
**Then** the camera's FOV changes in the viewport
**And** the scene is marked dirty (title shows `*`)
**And** pressing Ctrl+Z undoes the change, reverting fov_y to 1.05

### Story 2 — Edit point light color with ColorPicker (Priority: P1)

As an editor user, I want to change the color of a point light using a color picker.

**Given** an entity with a PointLightComponent is selected
**When** I expand the "point_light" section
**Then** the "color" property shows a ColorEdit3 widget (no alpha)

**Given** the color widget shows white (1.0, 1.0, 1.0)
**When** I click the color widget and pick red
**Then** the light's color changes to red in the viewport
**And** the scene is marked dirty
**And** pressing Ctrl+Z reverts the color to white

### Story 3 — Edit mesh renderer model reference (Priority: P2)

As an editor user, I want to see the model asset reference on a MeshRenderer and edit it via text input.

**Given** an entity with a MeshRenderer component is selected
**When** I expand the "mesh_renderer" section
**Then** the "model" property shows an InputText field with the current model ID (or empty if no model)

**Given** the model field shows "player.gltf"
**When** I change it to "enemy.gltf" and press Enter
**Then** a SetComponentPropertyCommand is pushed
**And** the scene is marked dirty

### Story 4 — Component section with zero properties (Priority: P2)

As an editor user, I want to see a component section even if it has no editable properties, so I know the component exists.

**Given** an entity with a component that has zero registered properties
**When** I expand that component's section
**Then** the body shows centered text "No editable properties" in disabled text style

### Story 5 — Multiple components rendered in order (Priority: P2)

As an editor user, I want all components to appear in a predictable order matching their attachment order.

**Given** an entity has CameraComponent and PointLightComponent (camera was added first)
**When** I view the Properties Panel
**Then** the "camera" section appears first, followed by "point_light"
**And** both sections are collapsed by default

**Given** I expand both sections
**When** I edit a property in the point_light section
**Then** the edit succeeds and marks the scene dirty

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-01 | `ComponentInfoBase` gains `property_serialize(comp, idx, ctx) -> YAML::Node` and `property_deserialize(comp, idx, node, ctx) -> Result<void>` virtual methods. | Unit test: compile check, then test round-trip on a known component (e.g., CameraComponent, property 0). |
| AC-02 | `ComponentInfo<T>::property_serialize()` delegates to `Property::serialize()` for the given index. | Unit test: register a component with properties, serialize property 0, verify YAML matches expected. |
| AC-03 | `ComponentInfo<T>::property_deserialize()` delegates to `Property::deserialize()` for the given index. | Unit test: serialize, modify YAML, deserialize, verify component changed. |
| AC-04 | `TypeRegistry` gains `yaml_encode(type_index, any, ctx) -> Result<YAML::Node>` method. | Unit test: encode a known float value, verify YAML node matches. |
| AC-05 | `TypeRegistry::yaml_encode()` returns error for unregistered type. | Unit test: encode with unregistered type_index → error result. |
| AC-06 | `TypeRegistry::yaml_encode()` returns error if std::any doesn't match the type_index. | Unit test: type_index(float), any(string) → error result. |
| AC-07 | `TypeRegistry` gains `yaml_decode(type_index, node, ctx) -> Result<std::any>` method. | Unit test: decode a float YAML node, verify result holds float with correct value. |
| AC-08 | `TypeRegistry::yaml_decode()` returns error for unregistered type. | Unit test: unregistered type_index → error result. |
| AC-09 | `InspectorTypeEditor` gains virtual `draw_any(label, any&, type_index, flags, ctx) -> bool`. | Unit test: compile check. |
| AC-10 | `TypedInspectorEditor<T>::draw_any()` extracts T from std::any and delegates to draw_fn_. | Unit test: register a mock editor for int, call draw_any with any(int), verify mock called. |
| AC-11 | `TypedInspectorEditor<T>::draw_any()` returns false if std::any doesn't hold T. | Unit test: register int editor, call draw_any with any(float), verify returns false. |
| AC-12 | `InspectorTypeEditorRegistry::draw_any()` looks up editor by type_index and delegates. | Unit test: register mock for int, call draw_any with type_index(int), verify mock returns true. |
| AC-13 | `InspectorTypeEditorRegistry::draw_any()` falls back to read-only display if no editor registered. | Unit test: unregistered type → draw_fallback_readonly called (verify log output). |
| AC-14 | `SetComponentPropertyCommand` exists with constructor taking (entity_id, component_type_name, property_name, old_value, new_value). | Unit test: compile check. |
| AC-15 | `SetComponentPropertyCommand::execute()` writes new_value via property_deserialize and marks dirty. | Integration test: create entity with CameraComponent, execute command to change fov_y to 2.0, verify component value changed and dirty flag set. |
| AC-16 | `SetComponentPropertyCommand::undo()` writes old_value and marks dirty. | Integration test: execute then undo, verify component value reverts to original. |
| AC-17 | `SetComponentPropertyCommand::execute()` is safe (no crash) if entity is invalid/stale. | Integration test: destroy entity, execute command on it → no crash, warning logged. |
| AC-18 | `SetComponentPropertyCommand::execute()` is safe if component is missing from entity. | Integration test: create entity without CameraComponent, execute command for "camera" → no crash, error logged. |
| AC-19 | `PropertiesPanel` renders a collapsible section for each component below the Transform section. | Snapshot test (headless): create entity with CameraComponent + PointLightComponent, select it, verify both section headers appear in draw output. |
| AC-20 | Component sections default to collapsed (Transform remains always expanded). | Snapshot test: verify CollapsingHeader for camera is not expanded (default). |
| AC-21 | Each component section's properties render in a 2-column table matching Transform layout. | Snapshot test: expand camera section, verify property rows appear with names (fov_y, aspect, near, far) and editor widgets. |
| AC-22 | Color properties with `PropertyFlags::tag("rgb")` use ColorEdit3 (3-channel color picker). | Snapshot test: point_light color property renders with `ImGui::ColorEdit3` widget. |
| AC-23 | Editing a component property pushes a `SetComponentPropertyCommand` to the CommandStack. | Integration test: set up entity with camera, execute panel draw_ui, simulate value change, verify command pushed. |
| AC-24 | Rapid consecutive edits to the same property push one Command per user interaction (not per frame). | Integration test: verify that changing a DragFloat value produces exactly one command per end-drag. |
| AC-25 | Component sections appear in the order returned by `Entity::component_at()`. | Unit test: add CameraComponent, then PointLightComponent, verify component_at(0) is camera, component_at(1) is point_light. |
| AC-26 | Component with zero properties shows "No editable properties" text when expanded. | Snapshot test: component with no properties → section body shows disabled text. |
| AC-27 | The component_info map (type_name → ComponentInfoBase*) is built from `ComponentRegistry` each frame. | Code review: verify `ctx.editor.world().component_registry().describe()` or equivalent is called in draw_ui. |
| AC-28 | PropertyFlags min/max/step are correctly mapped to EditorFlags when calling draw_any. | Unit test: CameraComponent fov_y has min=0.001f, max=3.14159f. Verify EditorFlags has these values. |
| AC-29 | All existing tests still pass. | Run `buddd_tests` |
| AC-30 | Zero new warnings from `src/editor/`, `src/engine/scene/component_registry/`, and `tests/`. | Build with `cmake --build --preset debug` |

## E2E Verification

| Method | Description |
|---|---|
| **Headless unit test (CI)** | Run `buddd_tests` with `[editor][inspector]`, `[editor][component-properties]`, and `[engine][component-registry]` tags. Verify all AC tests pass. |
| **Manual smoke test (display)** | Run `buddd edit` with a scene that has entities with components (Camera, PointLight, MeshRenderer, FreeCameraMovement). Select an entity. Verify: (1) Component sections appear collapsed below Transform; (2) Expanding a section shows properties in a 2-column table; (3) Editing a float property (e.g., fov_y) changes the viewport and marks scene dirty; (4) Editing a color property shows ColorEdit3 picker; (5) Ctrl+Z undoes the property change; (6) Switching selection switches component sections correctly. |
| **Build verification** | `cmake --build --preset debug` with zero new warnings from affected directories. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A user can select any entity with components and see all component properties organized in collapsible sections. | Manual: select entities with different component sets, verify all properties visible. |
| SC-002 | A user can edit any component property and the edit is undoable via Ctrl+Z. | Manual: edit property, verify Ctrl+Z reverts it, Ctrl+Shift+Z redoes it. |
| SC-003 | Color properties use the correct picker variant (ColorEdit3 for "rgb" tagged, ColorEdit4 otherwise). | Manual: verify point_light color uses 3-channel picker; camera has no color property (N/A). |
| SC-004 | The component section iteration plus draw_any dispatch loop processes all properties in under 1ms for typical entities (<10 components, <20 properties total). | Manual: profile with Tracy or frame-time overlay during entity selection. |
| SC-005 | All engine changes (ComponentInfoBase, TypeRegistry) are backward-compatible — existing component registrations compile without changes. | Build verification: no compile errors from existing component files (register_all_components.cpp, etc.). |

## Edge cases

| Case | Expected behaviour |
|---|---|
| **Entity with no components** | Only the Transform section is shown. No component sections rendered. |
| **Entity with 10+ components** | All sections render in order. Each is collapsed by default. No performance issues from iteration. |
| **Component with many properties** (e.g., 20) | All properties render in the 2-column table. Scroll within the panel if content exceeds panel height. |
| **Property type not registered in TypeRegistry** | `TypeRegistry::yaml_decode()` returns error. The property is skipped (no editor widget shown). A debug-level warning is logged. |
| **Property type has no InspectorTypeEditor** | `InspectorTypeEditorRegistry::draw_any()` falls back to `draw_fallback_readonly()` — shows "(no editor for type <name>)" in disabled text. |
| **Property value is at default** | The property renders normally with its default value shown. No special default-value indicator in MVP1. |
| **std::any type mismatch during encode** | If the editor modified the std::any to hold a different type (should not happen with well-behaved editors), `yaml_encode()` returns an error. The property change is not applied. |
| **Entity destroyed while editing a property** | On the next frame's draw_ui(), the entity lookup fails → panel shows "No entity selected". The in-progress edit is terminated by ImGui frame boundary. |
| **Scene switch while editing** | Selection clears → panel shows "No entity selected". Any in-progress property edit is terminated. |
| **Rapid undo/redo of component edits** | Each SetComponentPropertyCommand stores YAML snapshots. Undo/redo is O(1) per command (single property YAML write). |
| **CommandStack overflow (128 entries)** | Oldest commands are discarded (existing behavior). No special handling needed for SetComponentPropertyCommand. |
| **Component type name changes between game and editor** | The component type name is determined by the `ComponentRegistry` registration at startup. It is stable for the editor session. |
| **Property flags have extreme values** (e.g., min = -FLT_MAX, max = FLT_MAX) | EditorFlags maps these directly. DragFloat handles extreme ranges natively. |

## Error cases

| Case | Expected behaviour |
|---|---|
| **TypeRegistry::yaml_decode fails** | The property is skipped (not rendered). A warning is logged: `BUDDD_LOG_TAGGED_WARN("Editor:ComponentProperties", "Failed to decode property '{}': {}")`. |
| **TypeRegistry::yaml_encode fails** | The edit is not applied. The command is not pushed. A warning is logged. |
| **ComponentInfoBase::property_deserialize fails** | The SetComponentPropertyCommand::execute() still marks dirty (the value was attempted). A warning is logged. On undo, the old value is re-applied. |
| **Entity lookup fails on command execute** | Warning logged: `BUDDD_LOG_TAGGED_WARN("Editor:Command", "SetComponentPropertyCommand: entity {} not found")`. Command is a no-op. |
| **Component not found on entity during command execute** | Warning logged. Command is a no-op. |
| **Property index out of bounds** | `property_serialize`/`property_deserialize` throws or returns error. Caught by the property editing loop. Property is skipped with a warning. |
| **ImGui frame not active** | `draw_ui()` is only called when panel is visible. Guarded by panel system. |
| **Out of memory during YAML operations** | `std::bad_alloc` may be thrown. Consistent with existing editor behaviour. |

## Permissions and security

- No changes to permissions or security posture.
- Component property edits go through the Command system, which is bounded (128 entries).
- All edits are to in-memory data only. No file I/O during editing.
- YAML encode/decode round-trip is used for value transport only — no persisted YAML is written per-edit.
- No authentication or authorization boundaries are crossed.

## Observability

| Signal | Source |
|---|---|
| **Component section rendering** | Debug-level log per frame per component: `BUDDD_LOG_TAGGED_DEBUG("Editor:Properties", "Drawing component section '{}' ({} properties)", type_name, property_count)` — only on first draw per selection or on section expand. |
| **SetComponentPropertyCommand execute** | Debug-level log: `BUDDD_LOG_TAGGED_DEBUG("Editor:Command", "SetComponentProperty: entity={} comp={} prop={} old={} new={}", ...)` |
| **SetComponentPropertyCommand undo** | Debug-level log: `BUDDD_LOG_TAGGED_DEBUG("Editor:Command", "SetComponentProperty UNDO: entity={} comp={} prop={}")` |
| **TypeRegistry yaml_decode failure** | Warning: `BUDDD_LOG_TAGGED_WARN("TypeRegistry", "yaml_decode: type '{}' not registered")` |
| **Property type has no editor** | Debug log: `BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector", "No editor registered for type '{}'")` |
| **Component iteration** | Debug-level log when selection changes: `BUDDD_LOG_TAGGED_DEBUG("Editor:Properties", "Showing entity {} with {} components", primary_id.index, component_count)` |

## Documentation impact

The following existing wiki pages must be updated when this spec is implemented:

| Document | Reason for update |
|---|---|
| `docs/wiki/editor/editor-panels.md` | Update the Inspector Panel section to document component section rendering, draw_any() flow, and SetComponentPropertyCommand. Document the new Property Editors mapping table (PropertyFlags → EditorFlags, type → editor widget). |
| `docs/wiki/editor/cross-panel-communication.md` | Update to reflect that component property edits now use Commands (SetComponentPropertyCommand) with YAML-based value transport. |
| `docs/wiki/domain/glossary.md` | Add `SetComponentPropertyCommand`, `draw_any()`, `property_serialize`/`property_deserialize`. |
| `docs/wiki/architecture/module-map.md` | Update Editor section to include `set_component_property_command.h/.cpp` as new files. Update Engine section to note ComponentInfoBase extension (property_serialize/property_deserialize) and TypeRegistry extension (type-erased yaml_encode/yaml_decode). |

The north-star UX spec (`.specs/sprint-2026-06/editor-ux-design/spec.md`) should be updated to reflect:
- Component sections default to collapsed (deviation D-01).
- No Add/Remove Component buttons in this sprint (deviation D-02).

## Out of scope

- Add Component button and workflow.
- Remove Component button and workflow.
- Play-mode read-only enforcement (deferred to F-15).
- Multi-select editing.
- Reordering of component sections.
- Search/filter in the Properties Panel.
- Per-component undo grouping (each property edit is its own command).
- Custom editors for non-built-in types (e.g., enum dropdowns, asset drag-drop).
- Changes to the existing Transform section layout.
- Changes to the entity name field.
- Drag-and-drop asset references into Inspector fields.
- Color picker HSV/LAB modes beyond what ImGui provides.
- Component section header customization (icons, colors).
- Changes to `ImGui::ColorEdit` flags beyond `ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR`.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `Entity::component_count()` and `Entity::component_at(size_t)` are available and iterate in component-add order (the order they were attached to the entity). |
| A-02 | `ComponentRegistry::describe(type_name)` returns a `ComponentInfoBase*` for any registered component type. The pointer is valid for the lifetime of the editor. |
| A-03 | Each property type used in component registrations has a corresponding `TypeRegistry` entry (enforced by `ComponentInfo<T>::add_property()` runtime check with `std::abort()` otherwise — see `component_info.h` lines 158-165). So `yaml_decode`/`yaml_encode` will succeed for all registered component properties. |
| A-04 | The `YAML::Node` values produced by `Property::serialize()` and consumed by `Property::deserialize()` are valid and type-matched. |
| A-05 | `std::any_cast<T>(&value)` succeeds for the `any<T>` produced by `TypeRegistry::yaml_decode<T>()` and consumed by `TypedInspectorEditor<T>::draw_any()`. |
| A-06 | The `SerializationContext` obtained from `ctx.engine.services.assets()` is valid for the current frame and provides `AssetManager` access for `shared_ptr<Model>` property types. |
| A-07 | The mapping from `PropertyFlags` to `EditorFlags` is a simple field-by-field copy (`min_value`, `max_value`, `step_value`, `tags_`). No structural differences exist between the two structs. |
| A-08 | `ImGui::ColorEdit3` and `ImGui::ColorEdit4` are available in the used ImGui version (docking branch v1.91.8-docking). Both support `ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR`. |
| A-09 | The `SetComponentPropertyCommand` stores YAML nodes cheaply (yaml-cpp uses shared semantics for node copying). No performance concern for storing old/new YAML per command. |
| A-10 | The component-registry-to-editor dispatch does not need to handle `Quat` properties on non-Transform components — no existing component has a Quat property. If one is added later, the existing Quat editor works via draw_any dispatch. |
| A-11 | `Entity::component_at()` returns a `Component&` reference. The actual component type can be identified via its `type_name()` matched against `ComponentRegistry::describe()` entries. |
| A-12 | The component_info map (type_name → ComponentInfoBase*) is rebuilt each frame from the ComponentRegistry. This is cheap (entries are stable pointers) and avoids stale-pointer issues. |

## Open questions

| ID | Question | Priority | Impact |
|---|---|---|---|
| Q-01 | **Should the `ComponentInfoBase` → entity iteration use a cached map or per-frame lookup?** The Properties Panel needs to find the `ComponentInfoBase*` for each component on the entity. The current approach is to build a `type_name → ComponentInfoBase*` map from `ComponentRegistry` once per draw_ui() frame. Alternative: cache the map in the PropertiesPanel member and invalidate on registry change. | **Low** — neither approach is wrong. Per-frame lookup is simpler and cheap (a few pointers). Per-frame recommended for MVP1. | Implementation detail; no clarification needed. |
| Q-02 | **Should the `std::any` per-property value be cached across frames or re-read each frame?** Re-reading each frame is simpler but means the YAML→any decode happens every frame. Caching could improve performance but complicates state management when properties change from outside (Command undo, direct mutation). | **Low** — re-read each frame is recommended for MVP1 simplicity. The number of properties is small. | Implementation detail; no clarification needed. |
| Q-03 | **Should `math::Color` with the `"rgb"` tag use `ColorEdit3` (no alpha) or `ColorEdit4` (with alpha)?** The `"rgb"` tag conventionally indicates no alpha channel. All existing light components (PointLight, DirectionalLight, SpotLight) use `"rgb"` tag. The Color type always stores 4 channels (rgba). The editor should display `ColorEdit3` for `"rgb"` tagged properties (hides alpha) and `ColorEdit4` otherwise. | **Low** — clear from existing convention. | No clarification needed. |

These questions are all implementation-level and do not block the spec from being accepted.

---

## Self-validation checklist

| Check | Pass/Fail |
|---|---|
| Is every acceptance criterion testable? | ✅ Yes — all ACs have clear verification methods. |
| Are all edge cases and error cases covered? | ✅ Yes — 13 edge cases and 9 error cases listed. |
| Are there any hidden implementation decisions? | ✅ No — the spec specifies behavior, not implementation (draw_any mechanism is specified, not the internal dispatch table). |
| Are success criteria measurable and technology-agnostic? | ✅ Yes — SC-001 through SC-005 are about user outcomes, not implementation details. |
| Are user stories prioritized and independently testable? | ✅ Yes — P1/P2 stories with Given/When/Then. |
| Are there no more than 10 `[NEEDS CLARIFICATION]` markers? | ✅ Yes — zero markers. All open questions are tagged with priority assessment and no `[NEEDS CLARIFICATION]`. |
| Does the spec contradict any accepted spec? | ✅ No — aligns with F-05, F-06 UX Polish, and north-star UX. Explicit deviations (D-01, D-02) are documented. |
| Are assumptions documented for every reasonable default made? | ✅ Yes — all 12 assumptions documented. |
| Does the spec satisfy the Definition of Ready? | ✅ Yes — see DoR check below. |

### Definition of Ready check

| Criterion | Status |
|---|---|
| Scope is clearly defined (what is included and what is explicitly excluded) | ✅ Yes — Goals vs Non-goals clearly separate in-scope from out-of-scope. |
| Dependencies on other features, modules, or external systems are identified | ✅ Yes — depends on ComponentRegistry, TypeRegistry, InspectorTypeEditorRegistry (F-05), ComponentInfoBase (engine). |
| Edge cases and error conditions are described | ✅ Yes — 13 edge cases, 9 error cases. |
| The expected behavior is unambiguous and testable | ✅ Yes — ACs are specific, Gherkin stories, behavioral descriptions. |
| The spec defines how the feature will be verified end-to-end | ✅ Yes — manual smoke test + CI unit tests + build verification. |
| Acceptance criteria are specific, measurable, and verifiable | ✅ Yes — 30 ACs with specific verification methods. |
| Success and failure states are described | ✅ Yes — success criteria + error cases. |
| Interface changes (CLI flags, API signatures, config keys) are documented | ✅ Yes — ComponentInfoBase virtual methods, TypeRegistry static methods, InspectorTypeEditor virtual draw_any, SetComponentPropertyCommand constructor. |
| Existing documentation that must be updated is listed | ✅ Yes — 4 wiki pages + 1 north-star UX spec. |
| Technical constraints are identified (system APIs, libraries, build changes) | ✅ Yes — yaml-cpp, ImGui ColorEdit3/4, std::any. |
| Risks or unknowns are surfaced | ✅ Yes — open questions section with priority assessment. |
| Performance or resource implications, if any, are noted | ✅ Yes — YAML encode/decode per frame section, Command bounding (128 entries). |
