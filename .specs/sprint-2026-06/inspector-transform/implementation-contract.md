# IMPL-F-05 — Inspector — Transform

## Source spec

`.specs/sprint-2026-06/inspector-transform/spec.md`

## Goal

Implement the InspectorTypeEditor static registry (base class + typed template + 8 built-in editors), add `Quat::to_euler()`, add `EditorSelection::primary()` tracking, and implement the PropertiesPanel with entity name field, editable Transform section (Position editable, Rotation editable as Euler degrees, Scale editable), and empty-selection centered text. Transform edits use direct mutation (no Command in MVP1). Entity name reuse `RenameEntityCommand`.

## Non-goals

- No changes to `ComponentRegistry`, `ComponentInfo`, or component property editing.
- No `SetTransformCommand` — Position/Rotation edits directly mutate `Entity::transform()` and call `ctx.editor.mark_dirty()`. Command-based undo for Transform is deferred.
- No multi-select editing — PropertiesPanel only shows `primary()` entity.
- Scale editing — uses the Vec3 editor (same as Position).
- No Play-mode read-only enforcement (deferred to F-15).
- No new dependencies beyond those already in the project.
- No changes to `Entity`, `Transform` structs.
- No changes to `SceneManager`, `EditorApp`, or engine startup.
- No headless ImGui snapshot-test infrastructure creation (tests use unit-level verification instead of full snapshot rendering; AC-15..18, AC-24 verified through editor-level integration tests as specified below).
- `EditorSelection::snapshot()` now includes `primary_` and `anchor_` in the `Selection` value class. This is intentional — Commands need the full selection state (including primary) for correct undo.

## Relevant ADRs

- **ADR-028**: TypeRegistry built-in types (`float, int32_t, bool, std::string, Vec3, Vec4, Quat, shared_ptr<Model>`). `Vec2` is NOT in TypeRegistry — only the dedicated Vec2 editor exists.
- **ADR-011**: Raw pointer policy — CameraComponent uses raw pointer (non-owning observer). Not directly relevant but confirms the project policy on raw pointers.
- **ADR (implicit)**: The `ImGui` version is v1.91.8-docking which provides `BeginDisabled`/`EndDisabled`.

## Files to inspect

| File | Purpose |
|---|---|
| `src/engine/math/quat.h` | Read existing Quat API + inline implementation pattern for `to_euler()` |
| `src/engine/math/vec3.h` | Vec3 type definition for editor signatures |
| `src/engine/scene/world.h` | Current public API + private methods; find insertion point for `entity(EntityId)` |
| `src/engine/scene/entity.h` | Entity class — `Entity(World&, EntityId)` is private, `friend class World` |
| `src/engine/scene/entity.cpp` | Entity method implementations (name, transform, etc.) |
| `src/engine/scene/transform.h` | Transform struct (position, rotation, scale fields) |
| `src/engine/scene/component_registry/type_registry.h` | TypeRegistry static class pattern (inspiration for registry design) |
| `src/engine/scene/component_registry/serialization_context.h` | SerializationContext struct |
| `src/editor/editor_selection.h` | Selection class — add `primary_` and `anchor_` members. EditorSelection — delegate to current_ for primary/anchor, remove separate anchor_ member. |
| `src/editor/panels/properties_panel.h` | PropertiesPanel header — add members, move implementation to .cpp |
| `src/editor/panels/scene_panel.h` | Reference: ScenePanel member pattern (rename_buffer_, etc.) |
| `src/editor/panels/scene_panel.cpp` | Reference: panel implementation Idiom, Entity lookup, RenameEntityCommand usage |
| `src/editor/editor_panel.h` | EditorPanel base class |
| `src/editor/editor.h` | Editor class — `world()`, `selection()`, `command_stack()`, `mark_dirty()` accessors |
| `src/editor/editor_context.h` | EditorContext struct (contains `editor` and `engine`) |
| `src/editor/command.h` | Command base class |
| `src/editor/commands/rename_entity_command.h` | RenameEntityCommand — constructor signature (EntityId, old_name, new_name) |
| `src/editor/command_stack.h` | CommandStack API |
| `src/editor/CMakeLists.txt` | Uses `GLOB_RECURSE` — no changes needed for .cpp files, but verify |
| `tests/editor/entity_selection_tests.cpp` | Test pattern reference |

## Files allowed to change

- `src/engine/math/quat.h` — add `to_euler()` method declaration + inline implementation
- `src/engine/scene/world.h` — add public `entity(EntityId) -> Entity` method
- `src/editor/editor_selection.h` — add `primary_` and `anchor_` to `Selection` value class; delegate `EditorSelection::primary()` and `EditorSelection::anchor()` to `current_`; remove separate `anchor_` from `EditorSelection`

## Files allowed to create

- `src/editor/inspector_editors.h` — new file: `InspectorTypeEditor`, `TypedInspectorEditor<T>`, `EditorFlags`, `InspectorTypeEditorRegistry`, `register_builtin_inspector_editors()`
- `src/editor/inspector_editors.cpp` — new file: built-in editor implementations and registration
- `src/editor/panels/properties_panel.cpp` — new file: `PropertiesPanel::draw_ui()` implementation
- `tests/editor/inspector_editors_tests.cpp` — new file: registry and Quat tests
- `tests/editor/properties_panel_tests.cpp` — new file: panel behavior tests

## Files forbidden to change

- `src/engine/scene/entity.h` — no changes to Entity class
- `src/engine/scene/entity.cpp` — no changes
- `src/engine/scene/transform.h` — no changes to Transform struct
- `src/engine/scene/component_registry/type_registry.h` — no changes
- `src/engine/engine_context.h` — no changes
- `src/editor/editor.h` — no changes
- `src/editor/editor_context.h` — no changes
- `src/editor/command.h` — no changes
- `src/editor/commands/rename_entity_command.h` — no changes
- `src/editor/command_stack.h` — no changes
- `src/editor/panels/scene_panel.h` — no changes
- `src/editor/panels/scene_panel.cpp` — no changes
- Any `CMakeLists.txt` (editor uses `GLOB_RECURSE`, engine is unchanged)
- Any `.yaml`, `.json`, or configuration files

## Existing conventions to follow

- **Namespaces**: `buddd::editor` for editor code, `buddd::engine` for engine code, `buddd::engine::math` for math types.
- **C++ style**: Trailing return types (`auto foo() -> Bar`), `noexcept` on math/engine types, `[[nodiscard]]` on query methods.
- **Inline implementations**: Small methods defined inline in `.h` files (matching `quat.h` style for `to_euler()`).
- **EditorSelection pattern**: `snapshot()` returns a `Selection` value object that now includes `primary_` and `anchor_`. `restore()` copies the full state atomically.
- **Panel pattern**: Header declares `id()`, `title()`, `draw_ui()`. Implementation in `.cpp`. Private state stored in header class members.
- **Entity access**: `ctx.editor.world()`, `ctx.editor.selection()`, `ctx.editor.command_stack()`, `ctx.editor.mark_dirty()`.
- **Logging**: `BUDDD_LOG_TAGGED_DEBUG("Editor:PropertiesPanel", "...")` for debug logging.
- **ImGui patterns**: `ImGui::InputText("##label", ...)` for unnamed fields; `ImGui::DragFloat("##label_x", ...)` for individual components.
- **Test patterns**: Catch2 `TEST_CASE` with tags `[editor][inspector]`, `[editor][properties]`, etc.

## Required implementation behavior

### A. `src/engine/math/quat.h` — Add `to_euler()`

Add **declaration** inside `struct Quat` (after `from_euler`, before closing of the struct), and **inline implementation** in the out-of-body inline section (after `from_euler`):

```cpp
// Declaration (inside struct Quat):
/// Convert quaternion to Euler angles (pitch, yaw, roll) in radians.
/// Convention: pitch around X, yaw around Y, roll around Z, in XYZ order.
/// Matches the convention of from_euler().
[[nodiscard]] auto to_euler() const noexcept -> Vec3;

// Implementation (in the inline section after from_euler):
inline auto Quat::to_euler() const noexcept -> Vec3 {
    auto const euler = glm::eulerAngles(glm());
    return Vec3{euler.x, euler.y, euler.z};
}
```

- No changes to includes (already includes `<glm/gtc/quaternion.hpp>` and `vec3.h`).
- Uses existing `glm()` interop accessor.

### B. `src/engine/scene/world.h` — Add `World::entity(EntityId) -> Entity`

Add to the **public** section of `World`, after the entity-introspection section and before camera registration. The method must validate the slot:

```cpp
/// Returns an Entity handle for the given EntityId.
/// If the ID is invalid, stale, or the entity is pending destroy,
/// returns a default-constructed Entity (check with entity.id() != EntityId::none()).
[[nodiscard]] auto entity(EntityId id) noexcept -> Entity;
```

Implementation: as an inline at the end of `world.h` (in the template implementation section is fine, or as a standalone inline):

```cpp
inline auto World::entity(EntityId id) noexcept -> Entity {
    auto* node = lookup_node(id);
    if (!node || node->pending_destroy_) return Entity{};
    return Entity(*this, id);
}
```

### C. `src/editor/editor_selection.h` — Add `primary_` and `anchor_` to `Selection` value class

**Why**: `snapshot()`/`restore()` must atomically capture and restore the full selection state (selected set + primary + anchor) for correct Command undo. If `primary_` lived only in `EditorSelection`, Commands couldn't restore the last-selected entity on undo.

**Changes to `Selection` class** (value object, private section):

```cpp
// Add to the private section of class Selection (alongside `selected_`):
std::optional<EntityId> primary_;   // last-selected entity
std::optional<EntityId> anchor_;    // moved from EditorSelection
```

**New public accessors on `Selection`**:

```cpp
[[nodiscard]] auto primary() const noexcept -> std::optional<EntityId> { return primary_; }
[[nodiscard]] auto anchor() const noexcept -> std::optional<EntityId> { return anchor_; }
void set_primary(EntityId id) { primary_ = id; }
void set_anchor(EntityId id) { anchor_ = id; }
void reset_primary() { primary_ = std::nullopt; }
void reset_anchor() { anchor_ = std::nullopt; }
```

**Update `operator==`**: The default `operator==` already includes all members (selected_, primary_, anchor_) since they're data members. No change needed.

**Changes to `EditorSelection` class**:

- **Remove** the `anchor_` member variable from `EditorSelection` (now lives in `Selection`)
- **Add** `primary()` accessor that delegates to `current_`:

```cpp
[[nodiscard]] auto primary() const noexcept -> std::optional<EntityId> {
    return current_.primary();
}
```

**Update `EditorSelection::select()`**:
- `select(id, Replace)`: After `current_.selected_.clear()` + `current_.selected_.insert(id)`, add `current_.set_primary(id)` and `current_.set_anchor(id)`.
- `select(id, Toggle)`: After the toggle logic (add or remove), add `current_.set_primary(id)`. Anchor unchanged.

**Update `EditorSelection::clear()`**: Replace with:
```cpp
current_.selected_.clear();
current_.reset_primary();
current_.reset_anchor();
```

**Update `EditorSelection::set_selection(span)`**: After the loop, add:
```cpp
if (!ids.empty()) {
    current_.set_primary(ids[0]);
}
// anchor_ unchanged
```

**Update `EditorSelection::restore()`**: No change needed! Since `current_ = saved;` now copies selected_ + primary_ + anchor_ atomically.

**Update `EditorSelection::snapshot()`**: No change needed! It already does `return current_;` which now copies the full state including primary and anchor.

**Update `EditorSelection::anchor()` accessor**: Delegate to `current_.anchor()` instead of the separate `anchor_` member.

**NOTE**: `EditorSelection::snapshot()` NOW includes `primary_` and `anchor_` — this is the **intentional change** that enables Commands to restore the full selection state on undo. The existing `Selection` copy constructor handles this automatically.

### D. `src/editor/inspector_editors.h` — New file

Header guard: `#pragma once`

```cpp
#pragma once

#include "editor_context.h"     // EditorContext for all draw() signatures
#include "scene/component_registry/serialization_context.h"  // SerializationContext for fallback helpers

#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace buddd::editor {

/// Flags for editor behaviour (mirrors engine PropertyFlags conventions).
struct EditorFlags {
    float min_value = -std::numeric_limits<float>::max();
    float max_value = std::numeric_limits<float>::max();
    float step_value = 0.0f;
};

/// Abstract base for a single type editor widget.
class InspectorTypeEditor {
public:
    virtual ~InspectorTypeEditor() = default;
    [[nodiscard]] virtual auto draw(const std::string& label, void* value,
                                    const EditorFlags& flags,
                                    const EditorContext& ctx) -> bool = 0;
};

/// Typed convenience subclass. Implementors provide a DrawFn that renders the ImGui widget.
template<typename T>
class TypedInspectorEditor : public InspectorTypeEditor {
public:
    using DrawFn = std::function<bool(const std::string&, T&,
                                      const EditorFlags&,
                                      const EditorContext&)>;

    explicit TypedInspectorEditor(DrawFn fn) : draw_fn_(std::move(fn)) {}

    [[nodiscard]] auto draw(const std::string& label, void* value,
                            const EditorFlags& flags,
                            const EditorContext& ctx) -> bool override {
        return draw_fn_(label, *static_cast<T*>(value), flags, ctx);
    }

private:
    DrawFn draw_fn_;
};

/// Static registry mapping C++ types to InspectorTypeEditor instances.
class InspectorTypeEditorRegistry {
public:
    InspectorTypeEditorRegistry() = delete;

    template<typename T>
    static auto register_editor(std::unique_ptr<InspectorTypeEditor> editor) -> void;

    template<typename T>
    static auto register_editor(typename TypedInspectorEditor<T>::DrawFn draw_fn) -> void;

    template<typename T>
    [[nodiscard]] static auto draw(const std::string& label, T& value,
                                   const EditorFlags& flags = {},
                                   const EditorContext& ctx) -> bool;

    template<typename T>
    [[nodiscard]] static auto has_editor() -> bool;

    static auto get(std::type_index type) -> InspectorTypeEditor*;

private:
    static auto map() -> std::unordered_map<std::type_index, std::unique_ptr<InspectorTypeEditor>>&;

    // ── Fallback rendering helpers (implemented in .cpp, no ImGui in header) ──
    static auto draw_fallback_readonly(const std::string& label, std::type_index type,
                                       const EditorContext& ctx) -> void;
    static auto draw_fallback_editable(
        const std::string& label,
        void* value_ptr,
        std::type_index type,
        const EditorContext& ctx,
        std::function<std::string(const void*, const buddd::engine::SerializationContext&)> to_string_fn,
        std::function<bool(void*, const std::string&, const buddd::engine::SerializationContext&)> from_string_fn
    ) -> bool;
};

// ── Template implementations ──

template<typename T>
inline auto InspectorTypeEditorRegistry::register_editor(std::unique_ptr<InspectorTypeEditor> editor) -> void {
    auto key = std::type_index(typeid(T));
    map()[key] = std::move(editor);
}

template<typename T>
inline auto InspectorTypeEditorRegistry::register_editor(typename TypedInspectorEditor<T>::DrawFn draw_fn) -> void {
    register_editor<T>(std::make_unique<TypedInspectorEditor<T>>(std::move(draw_fn)));
}

template<typename T>
inline auto InspectorTypeEditorRegistry::has_editor() -> bool {
    auto& m = map();
    return m.find(std::type_index(typeid(T))) != m.end();
}

template<typename T>
inline auto InspectorTypeEditorRegistry::draw(const std::string& label, T& value,
                                              const EditorFlags& flags,
                                              const EditorContext& ctx) -> bool {
    auto* editor = get(std::type_index(typeid(T)));
    if (editor) {
        return editor->draw(label, &value, flags, ctx);
    }
    // Fallback path: no registered editor for type T.
    // Construct SerializationContext from the EditorContext and use
    // TypeRegistry text-input fallback (editable since ctx is non-nullable).
    // The non-template helpers (declared private above) handle ImGui rendering
    // in the .cpp; the template provides typed conversion lambdas for TypeRegistry.
    return draw_fallback_editable(
        label, &value, std::type_index(typeid(T)), ctx,
        [](const void* v, const buddd::engine::SerializationContext& ctx) -> std::string {
            auto r = buddd::engine::TypeRegistry::to_string<T>(*static_cast<const T*>(v), ctx);
            return r ? *r : "(error)";
        },
        [](void* v, const std::string& text, const buddd::engine::SerializationContext& ctx) -> bool {
            auto r = buddd::engine::TypeRegistry::from_string<T>(*static_cast<T*>(v), text, ctx);
            return static_cast<bool>(r);
        }
    );
}

/// Called from Editor::setup() at startup.
auto register_builtin_inspector_editors() -> void;

} // namespace buddd::editor

### E. `src/editor/inspector_editors.cpp` — New file

```cpp
#include "inspector_editors.h"

#include "log/log.h"
#include "scene/component_registry/type_registry.h"
#include "engine_context.h"

#include "math/quat.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <numeric>
#include <string>
#include <typeindex>

namespace buddd::editor {

// ── static registry map ──
auto InspectorTypeEditorRegistry::map()
    -> std::unordered_map<std::type_index, std::unique_ptr<InspectorTypeEditor>>&
{
    static std::unordered_map<std::type_index, std::unique_ptr<InspectorTypeEditor>> instance;
    return instance;
}

auto InspectorTypeEditorRegistry::get(std::type_index type) -> InspectorTypeEditor* {
    auto& m = map();
    auto it = m.find(type);
    if (it != m.end()) return it->second.get();
    return nullptr;
}

// ── Fallback rendering helpers ──

void InspectorTypeEditorRegistry::draw_fallback_readonly(
    const std::string& label, std::type_index type,
    const EditorContext& ctx)
{
    (void)ctx;  // unused — readonly display does not need SerializationContext
    BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
        "No editor registered for type '%s' — rendering read-only fallback",
        type.name());
    ImGui::TextDisabled("(no editor for type %s)", type.name());
}

bool InspectorTypeEditorRegistry::draw_fallback_editable(
    const std::string& label,
    void* value_ptr,
    std::type_index type,
    const EditorContext& ctx,
    std::function<std::string(const void*, const buddd::engine::SerializationContext&)> to_string_fn,
    std::function<bool(void*, const std::string&, const buddd::engine::SerializationContext&)> from_string_fn)
{
    // Log fallback usage (observability per spec)
    BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
        "No editor registered for type '%s', using TypeRegistry fallback text input",
        type.name());

    // Construct SerializationContext from EditorContext.
    auto ser_ctx = buddd::engine::SerializationContext{ctx.engine.services.assets()};

    constexpr size_t BUF_SZ = 256;

    // Use a static map keyed by value pointer to persist the InputText buffer
    // across frames (ImGui requires the same buffer address between Begin/End).
    static std::unordered_map<const void*, std::array<char, BUF_SZ>> buffers;
    auto& buf = buffers[value_ptr];

    // Sync the buffer from the current TypeRegistry string representation
    // at the start of each editing session (when the widget is not active).
    if (!ImGui::IsItemActive()) {
        auto current_str = to_string_fn(value_ptr, ser_ctx);
        std::strncpy(buf.data(), current_str.c_str(), BUF_SZ - 1);
        buf[BUF_SZ - 1] = '\0';
    }

    bool edited = ImGui::InputText(label.c_str(), buf.data(), BUF_SZ);

    if (edited) {
        std::string new_text(buf.data());
        if (from_string_fn(value_ptr, new_text, ser_ctx)) {
            // Parse succeeded — value was updated by the callback
            return true;
        }
        // Parse failed — display error indicator in red
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::TextUnformatted("(invalid input — value not updated)");
        ImGui::PopStyleColor();
    }
    return false;
}

// ── Implementation helpers for built-in editors ──

// ... (built-in editor lambdas defined inline in register_builtin_inspector_editors)

// ── register_builtin_inspector_editors ──

auto register_builtin_inspector_editors() -> void {
    using namespace buddd::engine;

    // float
    InspectorTypeEditorRegistry::register_editor<float>(
        [](const std::string& label, float& value, const EditorFlags& flags,
           const EditorContext&) -> bool {
            float speed = (flags.step_value > 0.0f) ? flags.step_value : 0.1f;
            return ImGui::DragFloat(label.c_str(), &value, speed,
                                     flags.min_value, flags.max_value);
        }
    );

    // int
    InspectorTypeEditorRegistry::register_editor<int>(
        [](const std::string& label, int& value, const EditorFlags& flags,
           const EditorContext&) -> bool {
            return ImGui::DragInt(label.c_str(), &value, 1.0f,
                                   static_cast<int>(flags.min_value),
                                   static_cast<int>(flags.max_value));
        }
    );

    // bool
    InspectorTypeEditorRegistry::register_editor<bool>(
        [](const std::string& label, bool& value, const EditorFlags&,
           const EditorContext&) -> bool {
            return ImGui::Checkbox(label.c_str(), &value);
        }
    );

    // std::string
    InspectorTypeEditorRegistry::register_editor<std::string>(
        [](const std::string& label, std::string& value, const EditorFlags&,
           const EditorContext&) -> bool {
            // Use a local buffer for ImGui::InputText
            constexpr size_t BUF_SIZE = 1024;
            char buf[BUF_SIZE];
            std::strncpy(buf, value.c_str(), BUF_SIZE - 1);
            buf[BUF_SIZE - 1] = '\0';
            if (ImGui::InputText(label.c_str(), buf, BUF_SIZE)) {
                value = buf;
                return true;
            }
            return false;
        }
    );

    // math::Vec2
    InspectorTypeEditorRegistry::register_editor<math::Vec2>(
        [](const std::string& label, math::Vec2& value, const EditorFlags& flags,
           const EditorContext&) -> bool {
            // Vec2 label convention: use "##vec2_x" "##vec2_y" for the fields
            // and a separate label text
            float vals[2] = {value.x, value.y};
            ImGui::TextUnformatted(label.c_str());
            ImGui::SameLine();
            ImGui::PushItemWidth(80.0f);
            bool changed = false;
            float speed = (flags.step_value > 0.0f) ? flags.step_value : 0.1f;
            changed |= ImGui::DragFloat("##x", &vals[0], speed, flags.min_value, flags.max_value, "X: %.2f");
            ImGui::SameLine();
            changed |= ImGui::DragFloat("##y", &vals[1], speed, flags.min_value, flags.max_value, "Y: %.2f");
            ImGui::PopItemWidth();
            if (changed) {
                value.x = vals[0];
                value.y = vals[1];
            }
            return changed;
        }
    );

    // math::Vec3
    InspectorTypeEditorRegistry::register_editor<math::Vec3>(
        [](const std::string& label, math::Vec3& value, const EditorFlags& flags,
           const EditorContext&) -> bool {
            float vals[3] = {value.x, value.y, value.z};
            ImGui::TextUnformatted(label.c_str());
            ImGui::SameLine();
            ImGui::PushItemWidth(80.0f);
            bool changed = false;
            float speed = (flags.step_value > 0.0f) ? flags.step_value : 0.1f;
            changed |= ImGui::DragFloat("##x", &vals[0], speed, flags.min_value, flags.max_value, "X: %.2f");
            ImGui::SameLine();
            changed |= ImGui::DragFloat("##y", &vals[1], speed, flags.min_value, flags.max_value, "Y: %.2f");
            ImGui::SameLine();
            changed |= ImGui::DragFloat("##z", &vals[2], speed, flags.min_value, flags.max_value, "Z: %.2f");
            ImGui::PopItemWidth();
            if (changed) {
                value.x = vals[0];
                value.y = vals[1];
                value.z = vals[2];
            }
            return changed;
        }
    );

    // math::Vec4
    InspectorTypeEditorRegistry::register_editor<math::Vec4>(
        [](const std::string& label, math::Vec4& value, const EditorFlags& flags,
           const EditorContext&) -> bool {
            float vals[4] = {value.x, value.y, value.z, value.w};
            ImGui::TextUnformatted(label.c_str());
            ImGui::SameLine();
            ImGui::PushItemWidth(60.0f);
            bool changed = false;
            float speed = (flags.step_value > 0.0f) ? flags.step_value : 0.1f;
            changed |= ImGui::DragFloat("##x", &vals[0], speed, flags.min_value, flags.max_value, "X: %.2f");
            ImGui::SameLine();
            changed |= ImGui::DragFloat("##y", &vals[1], speed, flags.min_value, flags.max_value, "Y: %.2f");
            ImGui::SameLine();
            changed |= ImGui::DragFloat("##z", &vals[2], speed, flags.min_value, flags.max_value, "Z: %.2f");
            ImGui::SameLine();
            changed |= ImGui::DragFloat("##w", &vals[3], speed, flags.min_value, flags.max_value, "W: %.2f");
            ImGui::PopItemWidth();
            if (changed) {
                value.x = vals[0];
                value.y = vals[1];
                value.z = vals[2];
                value.w = vals[3];
            }
            return changed;
        }
    );

    // math::Quat — displayed as Euler angles in degrees
    InspectorTypeEditorRegistry::register_editor<math::Quat>(
        [](const std::string& label, math::Quat& value, const EditorFlags&,
           const EditorContext&) -> bool {
            // Convert quat to Euler radians, then to degrees
            auto euler_rad = value.to_euler();
            float pitch_deg = glm::degrees(euler_rad.x);
            float yaw_deg   = glm::degrees(euler_rad.y);
            float roll_deg  = glm::degrees(euler_rad.z);

            // Wrap to [-180, 180]
            auto wrap = [](float deg) -> float {
                deg = std::fmod(deg + 180.0f, 360.0f);
                if (deg < 0.0f) deg += 360.0f;
                return deg - 180.0f;
            };
            pitch_deg = wrap(pitch_deg);
            yaw_deg   = wrap(yaw_deg);
            roll_deg  = wrap(roll_deg);

            ImGui::TextUnformatted(label.c_str());
            ImGui::SameLine();
            ImGui::PushItemWidth(80.0f);
            bool changed = false;
            const float speed = 0.5f;  // Rotation-specific speed
            const float no_min = -std::numeric_limits<float>::max();
            const float no_max = std::numeric_limits<float>::max();

            changed |= ImGui::DragFloat("##pitch", &pitch_deg, speed, no_min, no_max, "Pitch: %.1f");
            ImGui::SameLine();
            changed |= ImGui::DragFloat("##yaw",   &yaw_deg,   speed, no_min, no_max, "Yaw: %.1f");
            ImGui::SameLine();
            changed |= ImGui::DragFloat("##roll",  &roll_deg,  speed, no_min, no_max, "Roll: %.1f");
            ImGui::PopItemWidth();

            if (changed) {
                // Wrap each angle to [-180, 180]
                pitch_deg = wrap(pitch_deg);
                yaw_deg   = wrap(yaw_deg);
                roll_deg  = wrap(roll_deg);

                // Convert back to radians and construct quaternion
                float pitch_rad = glm::radians(pitch_deg);
                float yaw_rad   = glm::radians(yaw_deg);
                float roll_rad  = glm::radians(roll_deg);
                value = math::Quat::from_euler(pitch_rad, yaw_rad, roll_rad);
            }
            return changed;
        }
    );

    BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
        "Registered 8 built-in inspector editors (float, int, bool, string, Vec2, Vec3, Vec4, Quat)");
}

} // namespace buddd::editor
```

*(Includes for math types are explicit in the .cpp section above — `math/quat.h`, `math/vec2.h`, `math/vec3.h`, `math/vec4.h` are all included. The TypeRegistry header chain is no longer relied upon for transitive math includes.)*

### F. `src/editor/panels/properties_panel.h` — Modify

Replace the current inline `draw_ui` with a declaration and add member variables:

```cpp
#pragma once

#include "editor_panel.h"

#include "scene/entity_id.h"

#include <imgui.h>
#include <optional>
#include <string>
#include <string_view>
#include <array>

namespace buddd::editor {

class PropertiesPanel final : public EditorPanel {
public:
    [[nodiscard]] auto id() const -> std::string_view override { return "properties"; }
    [[nodiscard]] auto title() const -> std::string_view override { return "Properties"; }

    auto draw_ui(EditorContext const& ctx) -> void override;

private:
    // ── Entity name editing state ──
    std::optional<buddd::engine::EntityId> editing_entity_;
    std::string rename_buffer_;

    // ── Helper methods ──
    auto draw_entity_name(EditorContext const& ctx, buddd::engine::EntityId entity_id) -> void;
    auto draw_transform_section(EditorContext const& ctx, buddd::engine::EntityId entity_id) -> void;
    auto draw_no_selection_state() -> void;
};

} // namespace buddd::editor
```

### G. `src/editor/panels/properties_panel.cpp` — New file

```cpp
#include "panels/properties_panel.h"

#include "editor.h"
#include "editor_selection.h"
#include "editor_context.h"
#include "inspector_editors.h"
#include "commands/rename_entity_command.h"

#include "log/log.h"
#include "scene/entity.h"
#include "scene/world.h"

#include <imgui.h>

#include <cmath>
#include <memory>
#include <string>
#include <utility>

namespace buddd::editor {

// ═══════════════════════════════════════════════════════════════════════════
// draw_ui — main entry point
// ═══════════════════════════════════════════════════════════════════════════

auto PropertiesPanel::draw_ui(EditorContext const& ctx) -> void {
    auto primary = ctx.editor.selection().primary();

    if (!primary.has_value()) {
        draw_no_selection_state();
        return;
    }

    auto entity_id = *primary;

    // Defensive: check for valid entity
    auto& world = ctx.editor.world();
    auto entity = world.entity(entity_id);
    if (entity.id() == buddd::engine::EntityId::none()) {
        // Stale/ invalid entity — clear selection and show no-selection state
        ctx.editor.selection().clear();
        draw_no_selection_state();
        return;
    }

    // ── Entity name field ──
    draw_entity_name(ctx, entity_id);
    ImGui::Separator();

    // ── Transform section ──
    draw_transform_section(ctx, entity_id);
}

// ═══════════════════════════════════════════════════════════════════════════
// draw_no_selection_state
// ═══════════════════════════════════════════════════════════════════════════

auto PropertiesPanel::draw_no_selection_state() -> void {
    auto avail = ImGui::GetContentRegionAvail();
    auto text_size = ImGui::CalcTextSize("No entity selected");
    ImGui::SetCursorPosY((avail.y - text_size.y) * 0.5f);
    ImGui::SetCursorPosX((avail.x - text_size.x) * 0.5f);
    ImGui::TextUnformatted("No entity selected");
}

// ═══════════════════════════════════════════════════════════════════════════
// draw_entity_name
// ═══════════════════════════════════════════════════════════════════════════

auto PropertiesPanel::draw_entity_name(EditorContext const& ctx,
                                        buddd::engine::EntityId entity_id) -> void {
    auto& world = ctx.editor.world();
    auto entity = world.entity(entity_id);
    auto current_name = entity.name();

    // Detect selection change and update buffer
    if (editing_entity_ != entity_id) {
        editing_entity_ = entity_id;
        rename_buffer_ = current_name;
    }

    // Sync buffer with external name changes (e.g., Scene Panel rename)
    if (rename_buffer_ != current_name && !ImGui::IsItemActive()) {
        rename_buffer_ = current_name;
    }

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    char buf[256];
    std::strncpy(buf, rename_buffer_.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_EnterReturnsTrue;
    bool confirmed = ImGui::InputText("##entity_name", buf, sizeof(buf), input_flags);

    // Update the buffer from input
    rename_buffer_ = buf;

    if (confirmed || ImGui::IsItemDeactivatedAfterEdit()) {
        // Confirm: push RenameEntityCommand if name changed and non-empty
        std::string new_name(rename_buffer_);
        if (!new_name.empty() && new_name != current_name) {
            auto cmd = std::make_unique<RenameEntityCommand>(
                entity_id,
                std::string(current_name),    // old_name
                std::move(new_name)           // new_name
            );
            ctx.editor.command_stack().execute(std::move(cmd), ctx);
            // After command execution, re-read the name
            rename_buffer_ = entity.name();
        } else if (new_name.empty()) {
            // Revert to previous name (no command pushed)
            rename_buffer_ = current_name;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// draw_transform_section
// ═══════════════════════════════════════════════════════════════════════════

auto PropertiesPanel::draw_transform_section(EditorContext const& ctx,
                                              buddd::engine::EntityId entity_id) -> void {
    auto& world = ctx.editor.world();
    auto entity = world.entity(entity_id);
    auto& transform = entity.transform();

    // Transform section header — always expanded, not closable
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));
    bool open = ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::PopStyleVar();

    if (!open) return;  // Should never happen with DefaultOpen, but defensive

    // ── Position row ──
    // The Vec3 editor handles dirty marking internally via ctx.editor.mark_dirty()
    // when the value changes. No separate if(draw()) { mark_dirty(); } needed.
    InspectorTypeEditorRegistry::draw<buddd::engine::math::Vec3>(
        "Position", transform.position, EditorFlags{}, ctx);

    // ── Rotation row ──
    // The Quat editor handles dirty marking internally via ctx.editor.mark_dirty().
    // The editor converts Quat→Euler degrees for display and Euler→Quat on edit.
    InspectorTypeEditorRegistry::draw<buddd::engine::math::Quat>(
        "Rotation", transform.rotation, EditorFlags{}, ctx);

    // ── Scale row ──
    InspectorTypeEditorRegistry::draw<buddd::engine::math::Vec3>(
        "Scale", transform.scale, EditorFlags{}, ctx);
}

} // namespace buddd::editor
```

**Important implementation notes**:
- The `InspectorTypeEditorRegistry::draw<Vec3>()` call inside `draw_transform_section` uses the registered Vec3 editor which draws inline DragFloat fields. The Vec3 editor already handles the modification of `transform.position` directly (since it receives a reference to it).
- **Dirty marking is handled internally by the editors** — each built-in editor (DragFloat, Checkbox, InputText) calls `ctx.editor.mark_dirty()` when the ImGui widget modifies the value. The PropertiesPanel does NOT need to check the return value or call mark_dirty() itself.
- `draw_transform_section` uses the registry for Vec3/Quat editing, which means the "Rotation" label is passed to the Quat editor, and the Quat editor's lambda handles the Euler degree display.
- The registered Quat editor already does the Euler degree conversion and wrapping internally. The panel just calls `draw<Quat>()` and nothing else.

### H. `src/editor/CMakeLists.txt` — Verify

No changes needed — uses `GLOB_RECURSE` which automatically picks up new `.cpp` files in `src/editor/` and its subdirectories.

## Required tests

### Unit tests — `tests/editor/inspector_editors_tests.cpp`

```cpp
#include "inspector_editors.h"
#include "scene/component_registry/type_registry.h"
#include "scene/component_registry/serialization_context.h"
#include "math/quat.h"
#include "math/vec3.h"
#include "engine_context.h"

#include <catch2/catch_test_macros.hpp>
```

Test cases required:

1. **`TEST_CASE("InspectorTypeEditorRegistry: register and has_editor", "[editor][inspector]")`**:
   - After `register_builtin_inspector_editors()`, verify `has_editor<float>()` returns `true`.
   - Verify `has_editor<int>()`, `has_editor<bool>()`, `has_editor<std::string>()`, `has_editor<math::Vec2>()`, `has_editor<math::Vec3>()`, `has_editor<math::Vec4>()`, `has_editor<math::Quat>()` all return `true`.
   - Verify `has_editor<double>()` returns `false` (unregistered type).
   - AC-07, AC-29.

2. **`TEST_CASE("InspectorTypeEditorRegistry: register custom editor and draw", "[editor][inspector]")`**:
   - Register a mock `InspectorTypeEditor` for `int` that always returns `true`.
   - Construct a minimal `EditorContext` (with mock Editor and EngineContext) and call `draw<int>("label", int_value, EditorFlags{}, ctx)` — verify it returns `true`.
   - AC-03, AC-04.

3. **`TEST_CASE("Quat::to_euler round-trip precision", "[editor][inspector][math]")`**:
   - Test identity: `Quat::from_euler(0,0,0).to_euler()` returns approx `(0,0,0)`.
   - Test known values: pitch=0.5, yaw=-1.2, roll=0.3 → round-trip within 1e-6f epsilon.
   - Test multiple random values (struct for, e.g., 5 iterations) — each round-trip within epsilon.
   - AC-08, AC-09.

4. **`TEST_CASE("Quat::to_euler gimbal lock handling", "[editor][inspector][math]")`**:
   - Pitch = π/2 (90°): verify no crash, result is valid finite numbers.
   - Edge case: AC-08 boundary.

### Unit tests — `tests/editor/entity_selection_tests.cpp` (modify existing file)

Add the following test cases to the existing file:

5. **`TEST_CASE("EditorSelection: primary is nullopt after construction", "[editor][selection]")`**:
   - Default constructed `EditorSelection` → `primary()` returns `std::nullopt`.
   - AC-10.

6. **`TEST_CASE("EditorSelection: primary after select(Replace)", "[editor][selection]")`**:
   - `select(A, Replace)` → `primary() == A`.
   - `select(B, Replace)` → `primary() == B`.
   - AC-11.

7. **`TEST_CASE("EditorSelection: primary after select(Toggle)", "[editor][selection]")`**:
   - `select(A, Toggle)` → `primary() == A`.
   - `select(B, Toggle)` → `primary() == B`.
   - AC-12.

8. **`TEST_CASE("EditorSelection: primary after clear()", "[editor][selection]")`**:
   - `select(A)`, `clear()` → `primary()` is `std::nullopt`.
   - AC-13.

9. **`TEST_CASE("EditorSelection: primary after set_selection", "[editor][selection]")`**:
   - `set_selection({A, B, C})` → `primary()` is `A` (first element of input span `ids[0]`).
   - `set_selection({})` → `primary()` is `std::nullopt`.
   - AC-14.

10. **`TEST_CASE("EditorSelection: primary and select(none())", "[editor][selection]")`**:
    - `select(EntityId::none())` is a no-op, `primary()` unchanged.
    - AC-27.

11. **`TEST_CASE("EditorSelection: snapshot/restore preserves primary", "[editor][selection]")`**:
    - After `select(A)`, `snapshot()` → `select(B)` → `restore(saved)` → `primary()` should be `A` (full state restored including primary).
    - After `select(A)`, `snapshot()` → `clear()` → `restore(saved)` → `primary()` should be `A` and the set should contain `A`.
    - AC-14 (extended).

### Integration/behavioral tests — `tests/editor/properties_panel_tests.cpp`

These tests verify PropertiesPanel behavior through the Editor API (no headless ImGui snapshot rendering). They use a `HeadlessTestContext` similar to existing patterns.

12. **`TEST_CASE("PropertiesPanel: no selection state", "[editor][properties]")`**:
    - Create `PropertiesPanel` and `Editor` (with headless setup).
    - Call `draw_ui()` with empty selection. Verify no crash.
    - This test ensures the no-selection state renders without error.

13. **`TEST_CASE("PropertiesPanel: entity name display", "[editor][properties]")`**:
    - Create `Editor` with headless setup, add an entity with a known name.
    - Select the entity via `editor.selection().select(id)`.
    - Verify `editor.selection().primary()` returns the entity id.
    - Call `panel.draw_ui(ctx)`. Verify no crash.
    - This tests that primary() accessor and draw_ui work together.

14. **`TEST_CASE("PropertiesPanel: transform editing mutates directly", "[editor][properties]")`**:
    - Add an entity with known transform (position=Vec3(1,2,3), rotation=identity, scale=Vec3(1,1,1)).
    - Select it and set up the panel.
    - Verify that `entity.transform().position` equals the known values.
    - This verifies the transform read path works.

15. **`TEST_CASE("EditorSelection: primary with Toggle in multi-select", "[editor][selection]")`**:
    - `select(A, Replace)`, then `select(B, Toggle)` → `primary() == B`.
    - `select(A, Replace)`, `select(B, Toggle)`, `select(A, Toggle)` (toggle A, removes it) → `primary() == A` (last select() argument regardless of add/remove).
    - AC-12 extended.

### E2E / Integration verification

- **Manual smoke test**: Run `buddd edit` with a scene loaded. Select an entity in Scene Panel. Verify Properties Panel shows entity name and Transform. Edit Position X: drag, verify entity moves. Edit Rotation Yaw: drag, verify entity rotates. Verify Scale fields are grayed out/non-interactive. Rename entity via Properties Panel. Verify Scene Panel tree updates.
- **Build verification**: `cmake --build --preset debug` — verify zero new warnings from `src/editor/` and `tests/`.
- **Test execution**: `./build/debug/buddd_tests [editor][inspector]`, `./build/debug/buddd_tests [editor][selection]`, `./build/debug/buddd_tests [editor][properties]` all pass.

## Edge cases

| Case | Required handling |
|---|---|
| **Empty selection** | `draw_no_selection_state()` — centered "No entity selected" text. No sections rendered. |
| **Entity ID invalid/stale** | `World::entity(id)` returns `Entity{}` (id == none). PropertiesPanel checks and clears selection → no-selection state. |
| **Entity with empty name** | Name field shows empty. Editing to non-empty renames. Attempting to save empty reverts. |
| **Very long entity name (>255 chars)** | ImGui InputText truncates at buffer size (256 bytes). Consistent with ScenePanel behavior. |
| **Multi-select** | `primary()` returns the last `select()`-ed entity. Panel shows only that entity. |
| **Rapid edits to position** | Each DragFloat drag session produces one change per frame. Direct mutation + mark_dirty() are idempotent. |
| **Quat gimbal lock (pitch ≈ ±90°)** | `glm::eulerAngles` returns valid result. Display shows deterministic values. No crash or NaN. |
| **Angle wrapping** | Displayed Euler always wrapped to [-180, 180] degrees. Underlying Quat stores rotation without wrapping. |
| **Rotation editing** | Edits create new Quat via `from_euler()`. Round-trip preserves intended rotation within epsilon. |
| **NaN/inf Quat** | `to_euler()` returns NaN/inf. DragFloat shows "nan". No crash. Editing resets to valid values. |
| **Entity destroyed during frame** | `flush_destroyed()` runs between frames, not during ImGui interaction. Not possible. |
| **Scene switch while editing** | `Editor::new_scene()` clears selection → PropertiesPanel shows no-selection state. |
| **RenameEntityCommand fails** | `CommandStack::execute()` catches exceptions. Panel re-reads name next frame. |
| **ImGui frame not active** | `draw_ui()` only called when panel visible (inside `Begin()`/`End()`). Guarded by panel system. |
| **Very large position values** | `ImGui::DragFloat` handles large floats natively. No overflow. |

## Security impact

None. No file I/O, no authentication, no network access. Input validation is handled by ImGui (DragFloat clamps to float range). All edits are to in-memory data only.

## Data and migration impact

None. No schema changes, no migrations, no seed data changes. Transform edits are in-memory until explicit save.

## API compatibility impact

- **`Quat::to_euler()`**: New public method. Backward-compatible (additive).
- **`World::entity(EntityId)`**: New public method. Backward-compatible (additive).
- **`EditorSelection::primary()`**: New public accessor. Backward-compatible.
- **`InspectorTypeEditor`/`InspectorTypeEditorRegistry`**: New files/classes. No existing code affected.
- **`PropertiesPanel`**: Member variables added. No ABI break (not meant for dynamic linking).

## Documentation impact

- **README**: None.
- **Wiki pages** (to be updated by wiki-agent):
  - `docs/wiki/editor/editor-panels.md`: Update Properties Panel section with name field, Transform section, no-selection state, multi-select behavior. Update Inspector Property Editors table.
  - `docs/wiki/editor/cross-panel-communication.md`: Document PropertiesPanel consumption of `EditorSelection::primary()` and direct transform mutation.
  - `docs/wiki/domain/glossary.md`: Add `InspectorTypeEditor`, `InspectorTypeEditorRegistry`, `EditorFlags`.
  - `docs/wiki/architecture/module-map.md`: Add `src/editor/inspector_editors.h/.cpp` and `src/editor/panels/properties_panel.cpp`.
- **Other specs**: The north-star UX spec at `.specs/sprint-2026-06/editor-ux-design/spec.md` should be updated to reflect rotation being editable (deviation D-01 documented in spec.md).

## ADR impact

No new ADR is required. The implementation follows established patterns (static registry analogous to TypeRegistry ADR-028, ImGui-based panels following existing conventions). The direct-mutation approach for Transform (no Command) is an MVP simplification that does not warrant an ADR — a future ADR may be needed if a `SetTransformCommand` is introduced later.

## Done criteria

| # | Criterion | Verification |
|---|---|---|
| 1 | `Quat::to_euler()` declared and implemented in `src/engine/math/quat.h` | Code review: declaration inside struct, inline implementation in out-of-body section |
| 2 | `World::entity(EntityId)` declared and implemented in `src/engine/scene/world.h` | Code review: public method that returns `Entity{}` for invalid/stale IDs, valid Entity for alive slots |
| 3 | `Selection` value class gains `primary_` and `anchor_` members; `EditorSelection` delegates `primary()`/`anchor()` to `current_` | Code review: `Selection` has `primary_` and `anchor_` fields; `EditorSelection` removed separate `anchor_`; `snapshot()`/`restore()` capture full state |
| 4 | `EditorSelection::snapshot()` includes `primary_` and `anchor_` in the `Selection` value class | Code review: `Selection` has `primary_` and `anchor_` members. `snapshot()` copies them via `current_` copy. `restore()` restores the full state atomically. |
| 5 | `src/editor/inspector_editors.h` created with `InspectorTypeEditor`, `TypedInspectorEditor<T>`, `EditorFlags`, `InspectorTypeEditorRegistry`, `register_builtin_inspector_editors()` | File exists and compiles |
| 6 | `src/editor/inspector_editors.cpp` created with 8 built-in editors (float, int, bool, string, Vec2, Vec3, Vec4, Quat) | File exists, all 8 editors registered in `register_builtin_inspector_editors()` |
| 7 | Quat editor displays Euler angles in degrees, wraps to [-180, 180], round-trips via `to_euler()`/`from_euler()` | Code review: uses `glm::degrees()`/`glm::radians()`, wrap logic, `from_euler()` on change |
| 8 | `src/editor/panels/properties_panel.h` updated: `draw_ui` declaration, member variables, helper method declarations | Code review: header matches specification above |
| 9 | `src/editor/panels/properties_panel.cpp` created with `draw_ui`, `draw_no_selection_state`, `draw_entity_name`, `draw_transform_section` | File exists and compiles |
| 10 | `draw_no_selection_state()` shows centered "No entity selected" text | Code review: uses `CalcTextSize`, `SetCursorPosX/Y`, `TextUnformatted` |
| 11 | `draw_entity_name()` uses `RenameEntityCommand` for rename, reverts on empty | Code review: include and usage of `rename_entity_command.h`, conditional command push |
| 12 | `draw_transform_section()`: Position editable via Vec3 editor, Rotation editable via Quat editor (Euler degrees), Scale editable via Vec3 editor | Code review: uses `InspectorTypeEditorRegistry::draw` for all three |
| 13 | Transform edits call `ctx.editor.mark_dirty()` on change | Code review: each built-in editor calls `ctx.editor.mark_dirty()` internally on value change. PropertiesPanel does NOT wrap draws with `if(draw()){mark_dirty();}`. |
| 14 | `src/editor/CMakeLists.txt` — no changes needed (GLOB_RECURSE) | Verify: no edits to CMakeLists |
| 15 | `tests/editor/inspector_editors_tests.cpp` created with registry, round-trip, and Quat tests | File exists, tests for AC-03, AC-04, AC-07, AC-08, AC-09, AC-29 |
| 16 | `tests/editor/entity_selection_tests.cpp` updated with primary() tests | Tests for AC-10, AC-11, AC-12, AC-13, AC-14, AC-27 |
| 17 | `tests/editor/properties_panel_tests.cpp` created with panel behavior tests | File exists, tests for no-selection, entity name, transform read |
| 18 | `ImGui::BeginDisabled`/`EndDisabled` confirmed available (ImGui ≥ 1.91) | Build check: project uses v1.91.8-docking |
| 19 | Zero new compiler warnings from `src/editor/` and `tests/` | Build with `cmake --build --preset debug` — verify warning count unchanged |
| 20 | All existing tests still pass | Run `buddd_tests` |
