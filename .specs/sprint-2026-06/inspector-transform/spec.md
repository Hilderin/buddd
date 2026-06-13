# SPEC-F-05 — Inspector — Transform

## Problem

The Properties Panel has been an empty placeholder since F-00. The Scene Panel now supports entity selection (F-03) and entity operations (F-04), but selecting an entity does nothing — there is no feedback in the Properties Panel, no transform editing, and no way to modify entity properties. Users cannot see or edit the selected entity's name, position, rotation, or scale from within the editor.

Additionally, there is no reusable system for rendering type-appropriate editor widgets (float, int, bool, Vec3, Quat, etc.) in ImGui. Each editor panel that needs to edit entity properties would have to reimplement the same DragFloat/InputText/Checkbox logic. A centralized `InspectorTypeEditor` registry — analogous to the engine's `TypeRegistry` — is needed so that any future panel (Inspector component sections, asset editors, settings) can reuse the same type-editor widgets.

Finally, `Quat` lacks a `to_euler()` conversion method, making it impossible to display quaternion rotation as editable Euler angles (degrees) in the Properties Panel. `EditorSelection` lacks a `primary()` accessor to identify the "last-selected" entity for multi-select scenarios.

## Goals

| ID | Goal |
|---|---|
| G-01 | **InspectorTypeEditor registry**: Static registry mapping C++ types to reusable ImGui editor widgets. Base class + typed template subclass + fallback to `TypeRegistry::to_string()/from_string()` text input. |
| G-02 | **Built-in editors**: Pre-registered editors for `float` (DragFloat), `int` (DragInt), `bool` (Checkbox), `std::string` (InputText), `Vec2` (2× DragFloat), `Vec3` (3× DragFloat), `Vec4` (4× DragFloat), `Quat` (3× DragFloat Euler angles in degrees, round-trip via `to_euler`/`from_euler`). |
| G-03 | **Quat::to_euler()**: Add `Quat::to_euler() -> Vec3` returning pitch/yaw/roll in radians, matching the convention of `from_euler()`. |
| G-04 | **EditorSelection::primary()**: Add `primary()` accessor returning the last-selected entity (last entity passed to `select()`). Reset on `clear()`. Used by Properties Panel to determine which entity to show under multi-select. |
| G-05 | **PropertiesPanel — entity name**: Editable entity name field at top of panel, reusing `RenameEntityCommand`. |
| G-06 | **PropertiesPanel — Transform section**: Always-expanded section showing Position (editable Vec3), Rotation (editable Euler angles in degrees), and Scale (editable Vec3). All edits go through direct mutation and mark the scene dirty. |
| G-07 | **No-selection state**: When selection is empty, show centered text "No entity selected" in the Properties Panel. |
| G-08 | **Non-regression**: All existing tests pass. Zero new warnings from `src/editor/` and `tests/`. |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | **No component property editing** — this feature covers only Transform and entity name. Component property editors (e.g., MeshRenderer, Light components) are deferred to a future feature. |
| NG-02 | **No multi-select editing** — multi-select shows only the `primary()` entity. Simultaneous editing of multiple entities is deferred. |
| NG-03 | **No Rotation gizmo integration** — rotation is edited via DragFloat fields only. Viewport rotate gizmo is deferred (F-07). |
| NG-04 | **No SetTransformCommand** — Scale edits use direct mutation (same as Position/Rotation). A Command-based undo for transform edits is deferred. |
| NG-05 | **No undo for inline rename** — inline renames use `RenameEntityCommand` (F-04) which already supports undo/redo. |
| NG-06 | **No custom editors for non-built-in types** — only the 8 built-in types have dedicated editors. All other types fall back to text input. |
| NG-07 | **No keyboard shortcut for rename in Properties Panel** — rename is initiated from the Scene Panel (F4/F2) only. Properties Panel name field does not have its own shortcut. |
| NG-08 | **No Play-mode read-only enforcement** — the Properties Panel does not yet enforce read-only during Play mode (deferred to the Play-mode feature). |
| NG-09 | **No changes to `src/engine/scene/transform.h`** — the Transform struct is consumed as-is. No new fields or methods are added. |
| NG-10 | **No `SetTransformCommand` yet** — individual position/rotation edits are assumed to use a new Command (implied by the feature but the Command class itself is an implementation detail). |

### Differences from north-star UX spec

| # | Deviation | Rationale |
|---|---|---|
| D-01 | **Rotation fields are editable** in the Transform section (Pitch/Yaw/Roll DragFloat). The north-star UX spec (`docs/.specs/sprint-2026-06/editor-ux-design/spec.md`, Inspector section) and north-star wiki (`docs/wiki/editor/editor-panels.md`) specify rotation as read-only in MVP1. | This feature makes rotation editable to provide a more complete editing experience. The decision was made during the grill-me step (see coordination.md notes). The north-star wiki and north-star UX spec should be updated to reflect that rotation is editable in this implementation. |

## Actors

| Actor | Description |
|---|---|
| **Editor user** | Opens the editor, selects an entity in the Scene Panel, sees the Properties Panel populate with the entity's name and transform. Edits position, rotation, and name. Sees scale as read-only fields. Observes scene dirty state (*) on window title after edits. |
| **InspectorTypeEditor consumer** | Editor developer or future panel implementation that calls `InspectorTypeEditorRegistry::draw<T>()` to render a type-appropriate editor widget. Provides `label`, `void* value`, and `PropertyFlags`; receives `bool` indicating whether the value changed. |
| **Command author** | Implements the `SetTransformCommand` (or equivalent) that is created when position/rotation DragFloat values change. The Command snapshots the old transform, applies the new transform, marks scene dirty, and supports undo. |
| **Renderer developer** | Uses `Quat::to_euler()` to convert quaternions to Euler angles for display in UI or debug overlays. |

## User-visible behavior

### InspectorTypeEditor Architecture

The `InspectorTypeEditor` system provides a reusable, registry-based widget system for editing C++ types via ImGui. It follows the same static-registry pattern as the engine's `TypeRegistry`.

#### Class hierarchy

```
InspectorTypeEditor              (abstract base, src/editor/inspector_editors.h)
  └── TypedInspectorEditor<T>    (CRTP/template subclass, provides typed wrapper)

InspectorTypeEditorRegistry      (static registry, src/editor/inspector_editors.h/.cpp)
```

#### InspectorTypeEditor base class

```cpp
namespace buddd::editor {

/// Flags for editor behaviour (mirrors engine PropertyFlags conventions).
/// Corresponds to the engine's PropertyFlags struct (min_value, max_value, step_value, enum_choices).
struct EditorFlags {
    float min_value = -std::numeric_limits<float>::max();
    float max_value = std::numeric_limits<float>::max();
    float step_value = 0.0f;
};

/// Abstract base for a single type editor widget.
class InspectorTypeEditor {
public:
    virtual ~InspectorTypeEditor() = default;

    /// Draw the editor widget for a value of the registered type.
    /// @param label   ImGui label (includes "##" ID suffix as needed).
    /// @param value   Pointer to the value to edit (type-erased).
    /// @param flags   Optional numeric constraints (min, max, step).
    /// @param ctx      EditorContext providing Editor& (for mark_dirty, world access) and
    ///                 EngineContext const& (for SerializationContext from services.assets()).
    /// @return true if the value was modified by the user.
    [[nodiscard]] virtual auto draw(std::string_view label, void* value, EditorFlags flags = {},
                                    const EditorContext& ctx) -> bool = 0;
};

} // namespace buddd::editor
```

#### TypedInspectorEditor<T> (convenience template)

```cpp
namespace buddd::editor {

template<typename T>
class TypedInspectorEditor : public InspectorTypeEditor {
public:
    /// Draw the editor using type-safe access to the value pointer.
    /// Implementations cast `value` to `T*` and render the appropriate ImGui widget.
    /// The EditorContext provides access to editor.mark_dirty() and engine services.
    [[nodiscard]] auto draw(std::string_view label, void* value, EditorFlags flags = {},
                            const EditorContext& ctx) -> bool override = 0;
};

}
```

#### InspectorTypeEditorRegistry (static)

```cpp
namespace buddd::editor {

class InspectorTypeEditorRegistry {
public:
    InspectorTypeEditorRegistry() = delete; // static only

    /// Register an editor for type T. Replaces any existing editor for that type.
    /// Typically called once at editor startup.
    /// @tparam T The C++ type to register (e.g., float, Vec3, Quat).
    /// @param editor A unique_ptr to the editor instance.
    template<typename T>
    static auto register_editor(std::unique_ptr<InspectorTypeEditor> editor) -> void;

    /// Draw the editor for type T using the EditorContext for registry fallback (SerializationContext)
    /// and dirty marking (editor.mark_dirty()).
    /// @tparam T The C++ type whose editor should be used.
    /// @return true if the value was modified.
    template<typename T>
    [[nodiscard]] static auto draw(std::string_view label, T& value, EditorFlags flags = {},
                                   const EditorContext& ctx) -> bool;

    /// Check if an editor is registered for type T.
    template<typename T>
    [[nodiscard]] static auto has_editor() -> bool;

private:
    static auto editor_map() -> std::unordered_map<std::type_index, std::unique_ptr<InspectorTypeEditor>>&;
};

} // namespace buddd::editor
```

**`draw<T>()` behaviour:**
1. Looks up the editor for `std::type_index(typeid(T))`.
2. If found: calls `editor->draw(label, &value, flags, ctx)` and returns the result.
3. If NOT found (fallback):
   - Attempts to construct a `SerializationContext` from `ctx.engine.services.assets()`.
   - If the engine context provides a valid `AssetManager`: renders an editable `ImGui::InputText` text field.
     - Uses `TypeRegistry::to_string(value, serialization_ctx)` to populate the initial text.
     - On edit: attempts `TypeRegistry::from_string<T>(text, serialization_ctx)`. If parsing succeeds, updates value, calls `ctx.editor.mark_dirty()`, and returns `true`. If parsing fails, displays the invalid text in red (ImGui `PushStyleColor`) and returns `false`.
   - If no valid engine context: renders the value as read-only text (`ImGui::Text`). Returns `false` (no modification possible).

#### Built-in Editor Registrations

The following editors are registered at editor startup (in `inspector_editors.cpp` via a `register_builtin_inspector_editors()` free function):

| Type | Editor widget | Notes |
|---|---|---|
| `float` | `ImGui::DragFloat` | Uses `flags.step_value` if > 0, else default speed 0.1. Clamps to `[flags.min_value, flags.max_value]`. Default `min_value=-FLT_MAX, max_value=FLT_MAX`. |
| `int` | `ImGui::DragInt` | Default speed 1. Clamps to flags. |
| `bool` | `ImGui::Checkbox` | Flags ignored (no numeric constraints). |
| `std::string` | `ImGui::InputText` | Flags ignored. |
| `math::Vec2` | 2× `ImGui::DragFloat` labelled "X" and "Y" on same line. | Uses `ImGui::BeginDisabled`/`EndDisabled` if min==max. |
| `math::Vec3` | 3× `ImGui::DragFloat` labelled "X", "Y", "Z" on same line. | Same flags for all three components (from `EditorFlags`). |
| `math::Vec4` | 4× `ImGui::DragFloat` labelled "X", "Y", "Z", "W" on same line. | Same flags for all four components. |
| `math::Quat` | 3× `ImGui::DragFloat` labelled "Pitch", "Yaw", "Roll" in degrees. | Converts quaternion to Euler radians via `to_euler()`, converts to degrees for display, wraps to [-180, 180]. On edit: converts back to radians, applies `Quat::from_euler()`, stores result. Uses default DragFloat speed 0.5. Flags are NOT propagated to individual axes (Quat editing is special-cased). |

### Quat::to_euler() API

```cpp
// Add to src/engine/math/quat.h, inside struct Quat:

/// Convert quaternion to Euler angles (pitch, yaw, roll) in radians.
/// Convention: pitch around X, yaw around Y, roll around Z, in XYZ order.
/// Matches the convention of from_euler().
[[nodiscard]] auto to_euler() const noexcept -> Vec3;
```

Implementation (in `quat.h` inline section, matching the existing style):
```cpp
inline auto Quat::to_euler() const noexcept -> Vec3 {
    auto const euler = glm::eulerAngles(glm());
    return Vec3{euler.x, euler.y, euler.z};
}
```

### Selection value class — primary & anchor

The `Selection` value class gains `primary_` and `anchor_` fields so that `snapshot()`/`restore()` atomically capture and restore the full selection state (required for correct undo in Commands).

```cpp
// In src/editor/editor_selection.h, class Selection:

class Selection {
public:
    // ... existing members (contains(), size(), empty(), first(), add(), remove(), clear(), iteration) ...

    // -- Primary (last-selected) --
    [[nodiscard]] auto primary() const noexcept -> std::optional<EntityId>;
    void set_primary(EntityId id);
    void reset_primary();

    // -- Anchor (for Shift+click range) --
    [[nodiscard]] auto anchor() const noexcept -> std::optional<EntityId>;
    void set_anchor(EntityId id);
    void reset_anchor();

    // -- Comparison includes primary and anchor --
    auto operator==(const Selection&) const noexcept -> bool = default;

private:
    friend class EditorSelection;
    std::unordered_set<EntityId> selected_;
    std::optional<EntityId> primary_;   // NEW
    std::optional<EntityId> anchor_;    // NEW (moved from EditorSelection)
};
```

### EditorSelection — updated

The `EditorSelection` manager tracks the active selection state. `primary_` and `anchor_` are now part of `Selection` (not separate members), so `snapshot()`/`restore()` capture the full state.

```cpp
// In src/editor/editor_selection.h, class EditorSelection:

/// Returns the last-selected entity (most recent `select()` call).
[[nodiscard]] auto primary() const noexcept -> std::optional<EntityId> {
    return current_.primary();
}

/// Snapshot captures the full state: selected set + primary + anchor.
[[nodiscard]] auto snapshot() const noexcept -> Selection;

/// Restore restores the full state including primary and anchor.
void restore(const Selection& saved);

// select(), clear(), set_selection() all update primary and anchor
// on the current_ Selection object.
```

**Semantics update:**
- `select(id, Replace)`: Sets `current_.set_primary(id)` and `current_.set_anchor(id)`.
- `select(id, Toggle)`: Sets `current_.set_primary(id)` (anchor unchanged).
- `clear()`: Calls `current_.clear()`, `current_.reset_primary()`, `current_.reset_anchor()`.
- `set_selection(ids)`: If non-empty, sets `current_.set_primary(ids[0])`. Anchor unchanged.
- `snapshot()`: Returns a **copy** of `current_` (preserves full state including primary and anchor).
- `restore(saved)`: Copies `saved` into `current_` — restores set, primary, and anchor atomically.

### PropertiesPanel Layout

The Properties Panel (`src/editor/panels/properties_panel.h` + new `properties_panel.cpp`) replaces the current empty placeholder with structured content.

#### States

| Selection state | Properties Panel display |
|---|---|
| **Empty selection** (`editor.selection().empty() == true`) | Centered text "No entity selected" in the panel body. No sections shown. |
| **Single selection** (`editor.selection().size() == 1`) | Entity name field + Transform section populated from the sole selected entity. |
| **Multi-select** (`editor.selection().size() > 1`) | Entity name field + Transform section populated from `editor.selection().primary()` entity. Panel does not indicate multi-select state visually (deferred). |

#### Layout structure

```
┌─────────────────────────────────────┐
│ Properties                          │  ← Panel title bar (unchanged)
├─────────────────────────────────────┤
│ [Entity Name]                [input]│  ← Editable text field, reuses RenameEntityCommand
├─────────────────────────────────────┤
│ ▼ Transform                         │  ← Always-expanded collapsible header (default open, cannot close)
│   Position    X: [0.00] Y: [0.00] Z: [0.00]   ← 3× DragFloat, editable
│   Rotation    X: [0.00] Y: [0.00] Z: [0.00]   ← 3× DragFloat, degrees, editable
│   Scale       X: [1.00] Y: [1.00] Z: [1.00]   ← 3× DragFloat, editable
└─────────────────────────────────────┘
```

#### Entity name field
- Rendered as `ImGui::InputText` with the entity's current name.
- On Enter or focus loss: if the name has changed (and is non-empty), executes `RenameEntityCommand` via `ctx.editor.command_stack().execute()`.
- If the new name is empty: the input reverts to the previous name (no command pushed), matching F-04 Scene Panel inline rename behaviour.
- The field label is "Name" (or no label — implementation choice, documented as a decision for the contract).
- This is the same `RenameEntityCommand` used by the Scene Panel (F-04); no new command is needed.

#### Transform section
- **Collapsible header**: Rendered using `ImGui::CollapsingHeader` with `ImGuiTreeNodeFlags_DefaultOpen`. The header is NOT closable (user cannot collapse it) — it always shows the transform.
- **Always expanded**: The section is rendered first, above any future component sections. It cannot be collapsed or reordered in MVP1.
- **Position row**: 
  - Label "Position", then 3× `ImGui::DragFloat` for X, Y, Z.
  - Uses `InspectorTypeEditorRegistry::draw<Vec3>()` with the entity's `transform.position`.
  - Default DragFloat speed: 0.1. Range: no clamp (unbounded).
  - On value change (`draw` returns `true`): the panel creates and executes a `SetTransformCommand` (or equivalent command) that sets the new position, marks the scene dirty.
- **Rotation row**:
  - Label "Rotation", then 3× `ImGui::DragFloat` for Pitch/X, Yaw/Y, Roll/Z, displayed in **degrees**.
  - The `Quat` editor registered in `InspectorTypeEditorRegistry` handles the Quat→degrees→Euler→Quat round-trip:
    1. Read `transform.rotation` (Quat).
    2. Call `quat.to_euler()` → radians.
    3. Convert radians to degrees: `degrees = radians * (180.0f / π)`.
    4. Wrap each angle to [-180, 180].
    5. Display in 3× DragFloat labelled "Pitch", "Yaw", "Roll".
    6. On drag: new degrees → wrap to [-180, 180] → convert to radians → `Quat::from_euler(pitch, yaw, roll)` → store back.
  - Default DragFloat speed: 0.5. Range: no clamp (wrapping handles boundary).
  - On value change: executes command, marks scene dirty.
- **Scale row**:
  - Label "Scale", then 3× `ImGui::DragFloat` for X, Y, Z (editable).
  - Uses `InspectorTypeEditorRegistry::draw<Vec3>()` with the entity's `transform.scale`.
  - Default DragFloat speed: 0.1. Minimum value: 0.001 (prevent negative/zero scale edge cases).
  - On value change: direct mutation + `ctx.editor.mark_dirty()`.

#### Change notification
- Any transform edit that results in a value difference pushes a Command to `ctx.editor.command_stack()`.
- The Command (e.g., `SetTransformCommand`) stores the previous transform and the new transform.
- The Command calls `ctx.editor.mark_dirty()` to set the dirty flag.
- Undo/redo restores the previous transform value and updates dirty state appropriately.

### ImGui Layout Details

The Properties Panel uses `ImGui::Begin()`/`End()` with the panel's existing `id()` and `title()`. The internal draw order is:

1. If `editor.selection().empty()`: center "No entity selected" text → `return`.
2. Retrieve `primary_id` from `editor.selection().primary()`.
3. Retrieve `Entity` object via `ctx.editor.world().entity(primary_id)` (the new public method on World).
4. Draw entity name field (top of panel, labelled "Name").
5. Draw "Transform" `CollapsingHeader` with `ImGuiTreeNodeFlags_DefaultOpen`.
   1. Push 3-column layout: label column + value columns (or use `ImGui::Columns` / `ImGui::Table` as per convention).
   2. Position row: label "Position", then Vec3 editor.
   3. Rotation row: label "Rotation", then Quat editor (as degrees Euler).
    4. Scale row: label "Scale", then Vec3 editor (editable, same as Position).
6. (Future: additional component sections go here.)

## Key entities

### `InspectorTypeEditor` (`src/editor/inspector_editors.h` — new file)

```cpp
namespace buddd::editor {

struct EditorFlags {
    float min_value = -std::numeric_limits<float>::max();
    float max_value = std::numeric_limits<float>::max();
    float step_value = 0.0f;
};

class InspectorTypeEditor {
public:
    virtual ~InspectorTypeEditor() = default;
    [[nodiscard]] virtual auto draw(std::string_view label, void* value, EditorFlags flags = {},
                                    const EditorContext& ctx) -> bool = 0;
};

template<typename T>
class TypedInspectorEditor : public InspectorTypeEditor {
public:
    [[nodiscard]] auto draw(std::string_view label, void* value, EditorFlags flags = {},
                            const EditorContext& ctx) -> bool override {
        return draw_typed(label, *static_cast<T*>(value), flags, ctx);
    }
    [[nodiscard]] virtual auto draw_typed(std::string_view label, T& value, EditorFlags flags,
                                          const EditorContext& ctx) -> bool = 0;
};

} // namespace buddd::editor
```

### InspectorTypeEditorRegistry (new file `inspector_editors.h`/`.cpp`)

```cpp
namespace buddd::editor {

class InspectorTypeEditorRegistry {
public:
    template<typename T>
    static auto register_editor(std::unique_ptr<InspectorTypeEditor> editor) -> void;

    template<typename T>
    [[nodiscard]] static auto draw(std::string_view label, T& value, EditorFlags flags = {},
                                   const EditorContext& ctx) -> bool;

    template<typename T>
    [[nodiscard]] static auto has_editor() -> bool;

private:
    using EditorMap = std::unordered_map<std::type_index, std::unique_ptr<InspectorTypeEditor>>;
    static auto editor_map() -> EditorMap&;
};

} // namespace buddd::editor
```

### Interface Changes

**New files:**
- `src/engine/math/quat.h` — Add `to_euler()` method (modify existing file).
- `src/editor/inspector_editors.h` — New file: `InspectorTypeEditor`, `TypedInspectorEditor`, `EditorFlags`, `InspectorTypeEditorRegistry`, plus declaration of `register_builtin_inspector_editors()`.
- `src/editor/inspector_editors.cpp` — New file: Registration of 8 built-in editors, fallback text input logic, `register_builtin_inspector_editors()` implementation.
- `src/editor/panels/properties_panel.cpp` — New file: `PropertiesPanel::draw_ui()` implementation (replacing the inline empty body in the header).
- Test files (see below).

**Modified files:**
- `src/engine/scene/world.h` — Add `World::entity(EntityId) -> Entity` as a new public factory method. This is a small engine change: constructs and returns an `Entity` handle from a valid `EntityId`. The private `Entity(World&, EntityId)` constructor remains `friend class World`.
- `src/engine/math/quat.h` — Add `to_euler() -> Vec3` declaration and inline implementation.
- `src/editor/editor_selection.h` — Add `primary()` accessor, `primary_id_` member, update `select()`, `clear()`, `set_selection()`, `restore()` to track `primary_id_`.
- `src/editor/panels/properties_panel.h` — Remove inline empty `draw_ui()`; replace with declaration (or keep inline but delegate to `.cpp`). Add any helper members needed.
- `src/editor/CMakeLists.txt` — No explicit change needed (already uses `GLOB_RECURSE` which picks up new `.cpp` files automatically).

**New test files:**
- `tests/f05_inspector_transform_tests.cpp` — Unit tests for:
  - `InspectorTypeEditorRegistry` (register, draw, fallback)
  - `Quat::to_euler()` round-trip precision
  - `EditorSelection::primary()` tracking
  - `SetTransformCommand` serialization (if a dedicated command is introduced)
  - Properties Panel snapshot tests (headless mode)

## User stories

### Story 1 — Select entity shows properties (Priority: P1)

As an editor user, I want to click an entity in the Scene Panel and see its name and transform values appear in the Properties Panel, so that I can inspect the entity's properties.

**Given** an empty selection (Properties Panel shows "No entity selected")
**When** I left-click an entity named "Player" in the Scene Panel
**Then** the Properties Panel displays "Player" in the name field
**And** the Transform section shows Player's Position, Rotation (as Euler degrees), and Scale values

**Given** no entities are selected
**When** I click empty space in the Scene Panel
**Then** the Properties Panel shows "No entity selected"
**And** no name field or transform section is visible

### Story 2 — Edit Position via DragFloat (Priority: P1)

As an editor user, I want to drag the X/Y/Z position fields to move an entity, so that I can position it in the scene.

**Given** entity "Player" is selected and its Position is (0, 0, 0)
**When** I drag the X field of Position from 0 to 5
**Then** the entity's Position X changes to 5
**And** the scene is marked dirty (title shows `*`)

**Given** the entity's Position has been changed to (5, 0, 0)
**When** I press Ctrl+Z
**Then** the entity's Position returns to (0, 0, 0)
**And** the Properties Panel reflects the restored value

### Story 3 — Edit Rotation as Euler angles in degrees (Priority: P1)

As an editor user, I want to rotate an entity by editing pitch/yaw/roll in degrees, so that I can orient it intuitively.

**Given** entity "Player" is selected and its rotation quaternion represents an identity rotation (all Euler angles 0°)
**When** I drag the Yaw field from 0 to 90
**Then** the entity rotates 90 degrees around the Y axis
**And** the Properties Panel displays Yaw as 90 (Yaw field shows 90)
**And** the scene is marked dirty

**Given** entity "Player" is rotated 90° Yaw
**When** I press Ctrl+Z
**Then** the entity's rotation returns to identity (Yaw 0)
**And** the Rotation fields in the Properties Panel update to (0, 0, 0)

### Story 4 — Editable Scale (Priority: P1)

As an editor user, I want to edit the Scale of the selected entity, so that I can resize entities in the Inspector.

**Given** entity "Player" is selected and its Scale is (2, 2, 2)
**When** I change the X Scale field from 2.00 to 4.00 and press Enter
**Then** the entity's scale X becomes 4.00
**And** the scene is marked dirty

### Story 5 — Rename entity from Properties Panel (Priority: P2)

As an editor user, I want to rename an entity by editing the name field in the Properties Panel, so that I have an alternative to the Scene Panel inline rename.

**Given** entity "Player" is selected
**When** I change the name field from "Player" to "Hero" and press Enter
**Then** the entity's name is changed to "Hero"
**And** `RenameEntityCommand` is pushed to the undo stack
**And** the scene is marked dirty

**Given** entity "Hero" is selected
**When** I change the name field to an empty string and press Enter
**Then** the name reverts to "Hero"
**And** no command is pushed

### Story 6 — Multi-select shows primary entity (Priority: P2)

As an editor user, I want the Properties Panel to show properties of the last-selected entity when I have multiple entities selected, so that I can still inspect and edit while multi-selecting.

**Given** "Player" and "Light" are both selected ("Light" was selected last)
**When** I view the Properties Panel
**Then** the name field shows "Light"
**And** the Transform section shows Light's position, rotation, and scale

**Given** "Light" is shown in Properties Panel (primary entity)
**When** I Ctrl+click "Player" (adding it, so Player becomes primary)
**Then** the Properties Panel now shows "Player"'s name and transform

### Story 7 — Angle wrapping to [-180, 180] (Priority: P2)

As an editor user, I want rotation angles to always display within [-180, 180] degrees, so that the values are consistent and easy to read.

**Given** entity "Player" has rotation Quat corresponding to Euler (270°, 0°, 0°)
**When** I view the Rotation fields in the Properties Panel
**Then** Pitch displays as -90° (wrapped from 270° to -180..180 range)

**Given** Pitch is displayed as -90°
**When** I drag the Pitch field to 200°
**Then** after release, the Pitch field displays -160° (wrapped to [-180, 180])
**And** the entity's rotation Quat corresponds to the original 200° value

### Story 8 — Quat-to-Euler round-trip precision (Priority: P2)

As an editor user, I want rotation to round-trip cleanly between Quat and Euler angles, so that editing rotation does not drift over successive edits.

**Given** entity "Player" has rotation Quat representing 45° Yaw
**When** I view the Rotation Yaw field (shows 45°)
**And** I do not modify it
**Then** the underlying Quat is unchanged (no drift from the conversion round-trip)

**Given** I change Pitch from 0 to 30 and back to 0 several times
**When** I check the final Quat value
**Then** it equals the original identity Quat (within single-precision epsilon)

### Story 9 — Fallback text input for unregistered types (Priority: P3)

As a developer, I want unregistered types in the Inspector editor system to fall back to text-based editing, so that any type with TypeRegistry support is editable without writing a custom editor.

**Given** a custom type `MyType` is registered in `TypeRegistry` with `to_string`/`from_string` callbacks
**But** no `InspectorTypeEditor` is registered for `MyType`
**When** `InspectorTypeEditorRegistry::draw<MyType>(...)` is called
**Then** an `ImGui::InputText` is shown with the string representation
**And** editing the text and confirming attempts `TypeRegistry::from_string<MyType>()`
**And** if parsing succeeds, the value is updated
**And** if parsing fails, the text is shown in red and the value is not updated

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-01 | `InspectorTypeEditor` base class exists with virtual `draw(label, void*, EditorFlags) -> bool`. | Unit test: compile check |
| AC-02 | `TypedInspectorEditor<T>` template casts `void*` to `T*` and calls `draw_typed()`. | Unit test: register a test editor, call `draw` with mocked value, verify typed dispatch |
| AC-03 | `InspectorTypeEditorRegistry` is a static class (deleted constructor). `register_editor<T>()` stores an editor; `draw<T>()` looks it up and delegates. | Unit test: register a mock editor for `int`, draw returns true, verify mock called |
| AC-04 | `InspectorTypeEditorRegistry::draw<T>()` returns `false` when the editor does not modify the value. | Unit test: register mock that always returns false, verify `draw` returns false |
| AC-05 | `InspectorTypeEditorRegistry::draw<T>()` calls the fallback text input when no editor is registered for type T. | Unit test: unregistered type with `TypeRegistry` mock, draw shows InputText, verify `TypeRegistry::to_string`/`from_string` are called |
| AC-06 | `InspectorTypeEditorRegistry::draw<T>()` fallback shows error state (red text) when `from_string` fails. | Unit test: unregistered type, `from_string` returns error, verify `ImGui::PushStyleColor(ImGuiCol_Text, ...)` is called |
| AC-07 | `InspectorTypeEditorRegistry::has_editor<T>()` returns `true` for registered types, `false` for unregistered types. | Unit test |
| AC-08 | `Quat::to_euler()` returns a `Vec3` (pitch, yaw, roll) in radians. | Unit test: identity → all zeros; known Quat → expected Euler values within epsilon |
| AC-09 | `Quat::to_euler()` round-trips with `Quat::from_euler()` within single-precision epsilon for non-gimbal-lock angles. | Unit test: random pitches/yaws/rolls, round-trip, verify original Quat ≈ result |
| AC-10 | `EditorSelection::primary()` returns `std::nullopt` when selection is empty (freshly constructed or after `clear()`). | Unit test |
| AC-11 | `EditorSelection::primary()` returns the last entity passed to `select(id, Replace)`. | Unit test: select(A), select(B), verify primary == B |
| AC-12 | `EditorSelection::primary()` returns the last entity passed to `select(id, Toggle)`. | Unit test: select(A), select(B, Toggle), verify primary == B |
| AC-13 | `EditorSelection::clear()` resets `primary()` to `std::nullopt`. | Unit test |
| AC-14 | `EditorSelection::set_selection(ids)` sets primary to the first element of the span (non-empty), or `std::nullopt` (empty span). | Unit test |
| AC-15 | `PropertiesPanel::draw_ui()` shows "No entity selected" centered text when `editor.selection().empty()`. | Snapshot test (headless): verify ImGui draw output contains "No entity selected" |
| AC-16 | `PropertiesPanel::draw_ui()` shows the entity name field when an entity is selected. | Snapshot test: select entity, verify ImGui draw output contains entity name |
| AC-17 | `PropertiesPanel::draw_ui()` shows the Transform section (Position, Rotation, Scale rows) when an entity is selected. | Snapshot test: select entity with known transform, verify text output contains "Position", "Rotation", "Scale", and the numeric values |
| AC-18 | `PropertiesPanel::draw_ui()` shows Scale fields as editable (same as Position). | Snapshot test: verify Scale DragFloat fields are interactive |
| AC-19 | Editing Position via the Vec3 editor in Properties Panel pushes a Command that changes the entity's transform position. | Integration test: set up entity, select it, call inspector_editors draw for Vec3, verify Command execution changes position |
| AC-20 | Editing Rotation via the Quat editor (Euler degrees) changes the entity's rotation Quat. The round-trip Quat→Euler→Quat preserves the intended rotation. | Integration test: set rotation Quat, edit pitch via the editor, verify new Quat corresponds to edited Euler |
| AC-21 | Rotation Euler display wraps values to [-180, 180] degrees both on initial display and after edit. | Unit test: set Quat corresponding to Euler (270, 0, 0), verify display returns -90, 0, 0. Edit Pitch to 200, verify display wraps to -160 (or the equivalent angle in [-180,180]) |
| AC-22 | Renaming entity via Properties Panel name field executes `RenameEntityCommand` and marks scene dirty. | Integration test: select entity, set InputText value, call draw_ui, verify command pushed and dirty flag set |
| AC-23 | Renaming entity to empty string in Properties Panel does nothing (no command pushed, name unchanged). | Integration test: select entity, set empty InputText, verify no command executed |
| AC-24 | Multi-select: Properties Panel shows primary entity's name and transform. | Snapshot test: select 2 entities (B last), verify output shows B's name and values |
| AC-25 | Rapid consecutive edits to Position only push one Command per DragFloat end-drag (not per-frame). | Test: simulate DragFloat value change, verify exactly one Command is pushed |
| AC-26 | Editing a destroyed entity's properties is a no-op (no crash, no command). | Integration test: select entity, destroy it, attempt edit, verify no exception and no command pushed |
| AC-27 | `EditorSelection::primary()` with `EntityId::none()` is not possible (guarded by `select()`'s existing `EntityId::none()` guard). | Unit test: `select(none)` is no-op, primary unchanged |
| AC-28 | `InspectorTypeEditorRegistry::draw<float>()` with `EditorFlags{min=0, max=100}` clamps DragFloat to [0, 100]. | Unit test: mock drag to -10 → value is 0; mock drag to 200 → value is 100 |
| AC-29 | Built-in editors are all registered after `register_builtin_inspector_editors()` is called. `has_editor<T>()` returns true for float, int, bool, string, Vec2, Vec3, Vec4, Quat. | Unit test: call register, verify all 8 types are registered |
| AC-30 | `PropertiesPanel` Entity name field uses `RenameEntityCommand` (same class as Scene Panel F-04). | Code review: verify include and usage of `commands/rename_entity_command.h` |
| AC-31 | All existing tests still pass. | Run `buddd_tests` |
| AC-32 | Zero new warnings from `src/editor/` and `tests/`. | Build with `cmake --build --preset debug` — verify zero warnings |

## E2E Verification

| Method | Description |
|---|---|
| **Headless unit test (CI)** | Build with `BUDDD_HAS_DISPLAY=OFF`. Run `buddd_tests`. Verify `[editor][inspector]` tagged tests pass — `InspectorTypeEditorRegistry` lifecycle, `Quat::to_euler()`, `EditorSelection::primary()`, snapshot-based Properties Panel tests. |
| **Manual smoke test (display)** | Run `buddd edit` with a scene loaded. Select an entity in Scene Panel. Verify Properties Panel shows entity name and transform. Edit Position: drag X, verify entity moves and scene becomes dirty (title `*`). Edit Rotation: drag Yaw, verify entity rotates and dirty state updates. Edit Scale: drag X, verify entity scales and dirty state updates. Rename entity via Properties Panel name field. |
| **Clean build verification (CI)** | Run `cmake --build --preset debug` and verify zero warnings from `src/editor/` and `tests/`. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A user can select an entity in the Scene Panel and immediately see its name, position, rotation (as degrees), and scale in the Properties Panel. | Manual: click entity, observe Properties Panel populated |
| SC-002 | A user can edit an entity's position by dragging any X/Y/Z field and see the change persist and be undoable. | Manual: edit position, verify Ctrl+Z reverts it |
| SC-003 | A user can edit an entity's rotation by dragging pitch/yaw/roll and the rotation updates correctly, with values wrapped to [-180, 180]. | Manual: edit rotation, observe entity rotates, verify displayed values stay in range |
| SC-004 | A user can rename an entity from the Properties Panel name field and the name updates in both the Properties Panel and Scene Panel. | Manual: rename in Properties Panel, verify Scene Panel tree updates |
| SC-005 | The Quat↔Euler round-trip has no observable drift over 10 consecutive edit-save-read cycles for non-gimbal-lock rotations. | Automated test: 10 round-trips, verify Quat difference < 1e-6 |
| SC-006 | InspectorTypeEditor system is fully reusable: calling `InspectorTypeEditorRegistry::draw<Vec3>()` works anywhere without additional setup. | Code review: verify no singleton dependency or editor-panel coupling |

## Edge cases

| Case | Expected behaviour |
|---|---|
| **Empty selection (no entities selected)** | Properties Panel shows "No entity selected" centered text. No sections rendered. |
| **Multi-select** | Properties Panel shows `primary()` entity's name and transform. No multi-edit support. |
| **Rapid edits to position** | Each DragFloat drag session produces exactly one Command on end-drag (not per-frame). Debouncing is handled by ImGui's DragFloat behaviour (value only changes on release or after continuous drag). |
| **Quat→Euler→Quat round-trip for identity** | `Quat::from_euler(0,0,0)` = identity. `identity.to_euler()` = (0,0,0). Round-trip within epsilon. |
| **Quat→Euler at gimbal lock (pitch ≈ ±90°)** | `glm::eulerAngles` produces a valid result. Yaw and roll may sum to a single angle. Display values are deterministic. Editing one of the ambiguous fields may produce unexpected results, but no crash or NaN occurs. |
| **Angle wrapping: display vs storage** | Displayed Euler values are always wrapped to [-180, 180] degrees. The underlying Quat stores rotation without wrapping (Quat normalization handles equivalent rotations). |
| **Very large position values (1e10)** | DragFloat displays and edits large values without overflow. `ImGui::DragFloat` handles large floats natively. |
| **Entity with empty name ("")** | Name field shows empty field. Editing to a non-empty string renames. Attempting to save empty name reverts to empty (no command). |
| **Entity destroyed while selected** | Properties Panel continues to reference the EntityId. On the next frame's `draw_ui()`, the Entity lookup may fail. Expected: graceful skip (no crash, panel shows "No entity selected" or clears). |
| **Entity ID becomes invalid during drag** | Not possible in practice — entity destruction is deferred to `flush_destroyed()` which runs in `Editor::update()` between frames, not during ImGui interaction. |
| **Scene switch while editing** | `Editor::new_scene()` clears selection → Properties Panel shows "No entity selected". Any in-progress drag is terminated by ImGui's frame boundary. |
| **Selection callback during Properties Panel rendering** | `EditorSelection::fire_callbacks()` may trigger during Properties Panel draw if selection is modified externally. The panel re-reads the selection each frame, so the next frame will reflect the change. No crash or stale data. |
| **Very long entity name (>256 chars)** | `ImGui::InputText` has a default buffer size of 256. Longer names are truncated. This is consistent with the existing Scene Panel rename behaviour. |
| **Quat with NaN/inf components** | `Quat::to_euler()` returns NaN/inf Euler values. The editor should display NaN as "nan" in DragFloat fields. No crash. Editing resets to valid values. |

## Error cases

| Case | Expected behaviour |
|---|---|
| **Editor not registered for type** | `InspectorTypeEditorRegistry::draw<T>()` falls back to `TypeRegistry::to_string()` / `from_string()` with text input (using `SerializationContext` from `ctx.engine.services.assets()`). If `ctx.engine` has no valid `AssetManager`, renders read-only text. If `TypeRegistry` also has no entry, the fallback text input shows conversion errors in red. |
| **TypeRegistry fallback: invalid text input** | When the user enters text that `TypeRegistry::from_string()` cannot parse, the text field displays the invalid text in red (`ImGui::PushStyleColor` with `ImGuiCol_Text` = red). The value is not updated. |
| **Entity ID is `EntityId::none()`** | `EditorSelection::primary()` may return `EntityId::none()` if the primary ID could somehow become `none()`. The `select()` method already guards against this (no-op for `none()`). As a safety net, Properties Panel checks for `none()` before entity lookup. |
| **Entity lookup failure (stale ID)** | `World::entity(id)` returns an invalid/null Entity or throws. Properties Panel checks validity and skips rendering (shows "No entity selected" on next frame). |
| **RenameEntityCommand fails** | `CommandStack::execute()` catches exceptions. If `RenameEntityCommand::execute()` throws, the error is logged, the command is NOT pushed, and the selection state is preserved. The Properties Panel name field reverts to the original name on the next frame. |
| **SetTransformCommand fails** | Same pattern: exception is caught, command not pushed, values revert. |
| **Out-of-memory during editor registration** | `std::bad_alloc` may be thrown when registering editors. This is consistent with existing editor behaviour. |
| **ImGui frame not active** | PropertiesPanel `draw_ui()` is only called when the panel is visible. ImGui `Begin()` returns `false` if the panel is collapsed/hidden. Guard ensures no extra work is done. |

## Permissions and security

- No changes to permissions or security posture.
- The Properties Panel reads and writes entity data from the in-memory World. No file I/O is performed during editing.
- Commands pushed to `CommandStack` are bounded (128 entries). No unbounded memory growth from editing.
- No authentication or authorization boundaries are crossed.
- Transform edits are not persisted until the user explicitly saves the scene (File > Save).

## Observability

| Signal | Source |
|---|---|
| **InspectorTypeEditorRegistry registration** | Debug-level log: `BUDDD_LOG_DEBUG("InspectorEditor: registered editor for type '{}'", typeid(T).name())` at `register_editor<T>()` time. |
| **InspectorTypeEditorRegistry fallback usage** | Debug-level log: `BUDDD_LOG_DEBUG("InspectorEditor: no editor registered for type '{}', using TypeRegistry fallback", typeid(T).name())` at `draw<T>()` fallback path. |
| **Properties Panel selection change** | Debug-level log in Properties Panel: `BUDDD_LOG_DEBUG("PropertiesPanel: showing entity {} (name='{}')", primary_id.index, entity_name)` on frame when displayed entity changes. |
| **Transform edit via Command** | Debug-level log in Command: `BUDDD_LOG_DEBUG("SetTransformCommand: entity {} position ({}, {}, {})", id.index, pos.x, pos.y, pos.z)` on execute. |
| **Rename via Properties Panel** | Debug-level log: `BUDDD_LOG_DEBUG("PropertiesPanel: renaming entity {} from '{}' to '{}'", id.index, old_name, new_name)` — logged by `RenameEntityCommand` (existing F-04 behaviour). |

## Documentation impact

The following existing wiki pages must be updated when this spec is implemented:

| Document | Reason for update |
|---|---|
| `docs/wiki/editor/editor-panels.md` | Update the Properties Panel section to document the implemented behaviour (entity name field, Transform section, no-selection state, multi-select behaviour). Update the Inspector Property Editors table to reflect that `InspectorTypeEditorRegistry` now provides the editor widgets. |
| `docs/wiki/editor/cross-panel-communication.md` | Update to reflect that Properties Panel now consumes `EditorSelection` (via `primary()`) and that transform edits use the Command system. |
| `docs/wiki/domain/glossary.md` | Add `InspectorTypeEditor` and `InspectorTypeEditorRegistry` to the glossary if not already present. |
| `docs/wiki/architecture/module-map.md` | Update the Editor section to include `inspector_editors.h/.cpp` as new files in `src/editor/`. |

## Out of scope

- Component property editing (collapsible sections for MeshRenderer, Light components, etc.).
- `SetTransformCommand` implementation details (the spec refers to it conceptually; the contract defines its exact API).
- Multi-select simultaneous editing (deferred).
- SetTransformCommand: Transform edits use direct mutation (no Command undo in MVP1).
- Viewport gizmo integration (F-07).
- Play-mode read-only enforcement (deferred to Play-mode feature).
- Drag-and-drop asset references into Inspector fields.
- Search/filter in the Properties Panel.
- Reordering of transform/component sections in the Panel.
- Color picker widgets (deferred).
- Keyboard shortcut for rename in Properties Panel.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `glm::eulerAngles(glm::quat)` returns Euler angles in radians with XYZ order, matching the `glm::quat(glm::vec3{pitch, yaw, roll})` constructor used in `Quat::from_euler()`. Confirmed by GLM documentation. |
| A-02 | `TypeRegistry::to_string<T>()` and `TypeRegistry::from_string<T>()` exist for all 8 built-in types registered at startup (float, int, bool, string, Vec3, Vec4, Quat). Both require `const SerializationContext&`. The fallback path constructs a `SerializationContext` from `ctx.engine.services.assets()` where `ctx` is the `EditorContext` parameter of `draw()`. |
| A-03 | `Entity::name()` and `Entity::set_name()` exist on the `Entity` class (added in F-02/F-04). |
| A-04 | `Entity::transform()` returns a mutable `Transform&` (or equivalent accessor) allowing direct field modification via Command. |
| A-05 | `PropertiesPanel::draw_ui()` receives `EditorContext const& ctx` which provides `ctx.editor.world()`, `ctx.editor.selection()`, `ctx.editor.command_stack()`, and `ctx.editor.mark_dirty()`. |
| A-06 | `World::entity(EntityId) -> Entity` is added as a new public method on `World` (part of this feature). It constructs and returns an `Entity` handle from a valid `EntityId`. For invalid/stale IDs, the returned `Entity` is invalid (check via `Entity::is_valid()` or similar). The `Entity(World&, EntityId)` constructor remains private — `World::entity()` is the only public factory. |
| A-07 | `RenameEntityCommand` constructor accepts (EntityId, new_name) and is already registered in the F-04 codebase. It can be reused without modification by creating it with `std::make_unique<RenameEntityCommand>(id, new_name)`. |
| A-08 | `ImGui::DragFloat` with a speed of 0.0 uses a default step (1.0) in ImGui. We use explicit non-zero speeds (0.1 for position, 0.5 for rotation) to ensure user-friendly dragging. |
| A-09 | The `EditorFlags` struct is a simplified version of the engine's `PropertyFlags`. It is part of the editor, not the engine, to avoid coupling the editor to the engine's component registry types. |
| A-10 | All 8 built-in types are registered at editor startup via `register_builtin_inspector_editors()` called from `Editor::setup()`. |
| A-11 | The `SetTransformCommand` (or equivalent) stores the previous transform and the new transform. It is pushed via `ctx.editor.command_stack().execute(std::make_unique<SetTransformCommand>(...))`. |
| A-12 | Rotation in the transform is stored as a `Quat`. The `to_euler()` / `from_euler()` round-trip is lossy only at gimbal lock (single-precision epsilon otherwise). |
| A-13 | The entity name field uses `ImGui::InputText` with `ImGuiInputTextFlags_EnterReturnsTrue` to detect Enter press. Focus loss also confirms the edit. |
| A-14 | `std::numeric_limits<float>::max()` is used as the default max value in `EditorFlags`, matching the engine's `PropertyFlags` convention. |

## Open questions

| ID | Question | Resolution |
|---|---|---|
| Q-01 | **Should the Properties Panel name field use `ImGui::InputText` with a character limit?** The Scene Panel F-4 uses `ImGui::InputText` without explicit character limit (ImGui default buffer is 256). The Properties Panel should match this behaviour for consistency. | **No clarification needed.** Match Scene Panel: no explicit limit, accept ImGui default (256 chars). |
| Q-02 | **What should the Properties Panel do if the primary entity is stale (destroyed)?** The entity lookup from World may fail. The safest behaviour is to clear the panel state and show "No entity selected" for that frame. The next frame will re-evaluate selection state. | **No clarification needed.** Defensive handling: skip rendering on failed lookup. |
| Q-03 | **Should `EditorFlags.min_value` / `max_value` be nullable to indicate "no constraint"?** The engine's `PropertyFlags` uses `std::numeric_limits<float>::max()` as default. We follow the same convention for consistency. | **No clarification needed.** Use `std::numeric_limits<float>::max()` sentinel. |
