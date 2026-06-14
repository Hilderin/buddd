# IMPL-F-06b — Inspector — Add/Remove Components (Index-Based)

## Source spec

`.specs/sprint-2026-06/inspector-add-remove-components/spec.md`

## Goal

Allow the editor user to add new components to (and remove existing components from) the selected entity via the Properties Panel, using an **index-based** approach for component identification. Two new Commands (`AddComponentCommand`, `RemoveComponentCommand`) provide undo/redo. The World gets two new index-based methods (`remove_component_at`, `insert_component_raw_at`) to enable runtime (non-template) component manipulation. The Properties Panel gains an "Add Component" button with a searchable popup listing all registered component types, and a small "ⓧ" remove button on each non-Transform component header that uses the loop index to reference the component.

## Non-goals

- No confirmation dialog for remove (removal is immediate, undoable via Ctrl+Z).
- No "last component" guard — removing the last non-Transform component is allowed.
- No component reordering.
- No multi-select component add/remove.
- No play-mode read-only enforcement.
- No changes to the Transform section (always present, no remove button).
- No presets, favorites, or template-based creation.
- No drag-and-drop support.
- No new dependencies beyond what yaml-cpp and ImGui already provide.

## Relevant ADRs

| ADR | Relevance |
|-----|-----------|
| ADR-027 (Editor Architecture) | Commands live in `src/editor/commands/`; panels in `src/editor/panels/`. No SDL3/OpenGL/GLM headers in `src/editor/`. |
| ADR-028 (Component Type Registry) | `ComponentRegistry` and `ComponentInfoBase` provide the create/describe/serialize/deserialize APIs the commands depend on. |
| ADR-029 (Editor UX Decisions) | Adds to the Properties Panel's component sections with Add/Remove UI. Decision 5 (Play mode read-only) means Add/Remove buttons may need hiding in a future feature, but not in this contract. |

## Files to inspect

The Code Agent must read these files before editing any code:

1. `src/editor/commands/create_entity_command.h` — Pattern for command with execute/undo/name; selection snapshot save/restore.
2. `src/editor/commands/set_component_property_command.h` — Pattern for command using `ComponentRegistry::describe()`, `ComponentInfoBase::create()` for type_index lookup, `SerializationContext`, YAML serialize/deserialize.
3. `src/editor/command.h` — `Command` base class (execute, undo, name, try_update_new_value).
4. `src/editor/panels/properties_panel.h` — Current public/private interface (to add new methods and state).
5. `src/editor/panels/properties_panel.cpp` — Current full implementation of `draw_ui()`, `draw_component_sections()`, `draw_transform_section()`, `draw_entity_name()`, `draw_no_selection_state()`.
6. `src/engine/scene/world.h` — `World` class: `lookup_node()`, `add_component_raw()`, `remove_component<T>()` template, `EntityNode` struct with `components_` vector, existing `component_count()`, `get_component_at()`.
7. `src/engine/scene/world.cpp` — Implementation of `add_component_raw()`, `remove_component<T>` equivalent logic, existing `component_count()`, `get_component_at()`.
8. `src/engine/scene/entity.h` — `Entity::component_count()`, `Entity::component_at()`.
9. `src/engine/scene/component_registry/component_registry.h` — `ComponentRegistry` with `describe()`, `create()`, `all_types()`.
10. `src/engine/scene/component_registry/component_info.h` — `ComponentInfoBase` with `create()`, `serialize()`, `deserialize()`, `type_name()`.
11. `src/engine/scene/component_registry/serialization_context.h` — `SerializationContext` struct (takes `AssetManager&`).
12. `src/editor/inspector_editors.h` — `InspectorTypeEditorRegistry::draw_any()` (used in properties_panel.cpp).
13. `tests/editor/component_property_commands_tests.cpp` — Test pattern (TestContext setup, compile-time checks, execute/undo validation, edge cases).
14. `tests/editor/properties_panel_tests.cpp` — Test pattern for panel-level tests.

## Files allowed to change

- `src/engine/scene/world.h` — Add `remove_component_at()` and `insert_component_raw_at()` declarations in the public section after `add_component_raw()`.
- `src/engine/scene/world.cpp` — Implement `remove_component_at()` and `insert_component_raw_at()`.
- `src/editor/panels/properties_panel.h` — Add new private methods (`draw_add_component_button`, `draw_add_component_popup`) and state (`add_component_filter_`, `pending_auto_expand_type_`).
- `src/editor/panels/properties_panel.cpp` — Modify `draw_ui()` to call `draw_add_component_button()`, modify `draw_component_sections()` with remove ⓧ buttons (using loop index `i`) and auto-expand logic, add `draw_add_component_button()` and `draw_add_component_popup()` implementations, add new includes.
- `docs/wiki/editor/editor-panels.md` — Update Inspector Panel section.
- `docs/wiki/domain/glossary.md` — Add `AddComponentCommand`, `RemoveComponentCommand`.
- `docs/wiki/architecture/module-map.md` — Update Editor section with new command files.

### New files

- `src/editor/commands/add_component_command.h` — New Command class (index-based storage for undo).
- `src/editor/commands/remove_component_command.h` — New Command class (index-based — constructor takes component_index).
- `tests/editor/add_remove_component_commands_tests.cpp` — Test suite.

## Files forbidden to change

- Any file under `src/engine/scene/component_registry/` except the includes added by `world.h`/`world.cpp`.
- Any file under `src/engine/` other than `world.h` and `world.cpp`.
- Any existing test file (only create new test file, do NOT modify existing test files).
- Any file in `src/cmd/`.
- Any `CMakeLists.txt` file.
- `src/editor/editor.h`, `src/editor/editor_context.h`, `src/editor/editor_selection.h`, `src/editor/command.h`, `src/editor/command_stack.h` — these provide infrastructure the commands use but must NOT be modified.

## Existing conventions to follow

1. **Command class pattern**: Header-only, `final : public Command`, constructor takes value parameters by value (move for strings), all members are private. Implement `execute(ctx)`, `undo(ctx)`, `name()`, `try_update_new_value()` (return false for add/remove commands). Logging with `BUDDD_LOG_TAGGED_*("Editor:Command", ...)`.

2. **Selection snapshot**: For structural commands (create/delete entity), save `Selection pre_execution_selection_ = ctx.editor.selection().snapshot()` on `execute()`, restore `ctx.editor.selection().restore(pre_execution_selection_)` on `undo()`. Follow this pattern for both AddComponentCommand and RemoveComponentCommand.

3. **EditorContext access**: `ctx.editor.world()` → `World&`, `ctx.editor.command_stack()` → `CommandStack&`, `ctx.editor.mark_dirty()`, `ctx.engine.services.registry()` → `ComponentRegistry&`, `ctx.engine.services.assets()` → `AssetManager&`.

4. **Component type_index matching**: Use the "SceneSaver pattern" as done in `set_component_property_command.h`: `auto tmp = const_cast<ComponentInfoBase*>(info)->create(); auto target_type = std::type_index(typeid(*tmp));`. Then iterate entity components with `for (size_t i = 0; i < entity.component_count(); ++i) { auto& comp = entity.component_at(i); if (std::type_index(typeid(comp)) == target_type) ... }`.

5. **PropertiesPanel member style**: `char buffer_[64]` for ImGui filter buffers, `std::string` for names, `std::optional<EntityId>` for tracking editing targets.

6. **Include style**: Commands use paths relative to `src/editor/` (e.g., `"commands/add_component_command.h"`). Engine headers use paths relative to `src/engine/` (e.g., `"scene/world.h"`, `"scene/component_registry/component_registry.h"`).

7. **Namespace**: All editor code in `namespace buddd::editor`. All engine code in `namespace buddd::engine`.

8. **Unique PushID for ImGui items**: Each component section header uses `ImGui::PushID(static_cast<int>(i))` to scope the remove button's ID, preventing collisions between components of the same type at different indices.

9. **Testing pattern**: Use `TestContext` struct (as in `component_property_commands_tests.cpp`) that creates `EngineService` (headless), registers components, creates `Editor` and `EditorContext`. Each test creates a fresh `TestContext`.

## Required implementation behavior

### 1. `World::remove_component_at()` and `World::insert_component_raw_at()` — Engine additions

**Declaration** (`src/engine/scene/world.h`):
Add TWO new public methods after `add_component_raw()` (around line 116):
```cpp
/// Remove a component from an entity by index into the entity's components_ vector.
/// Handles Updatable cleanup (dynamic_cast, std::erase from updatables_).
/// Returns false if id is invalid, node is pending_destroy, or index is out of bounds.
auto remove_component_at(EntityId id, size_t index) -> bool;

/// Insert a runtime-typed component at a specific index in the entity's
/// components_ vector. The component is moved into the World's storage.
/// If index >= component_count, pushes at the back.
/// Calls on_attach() and auto-registers Updatable components.
auto insert_component_raw_at(EntityId id, size_t index, std::unique_ptr<Component> component) -> Component&;
```

**`remove_component_at` implementation** (`world.cpp`):
1. `auto* node = lookup_node(id); if (!node || node->pending_destroy_) return false;`
2. `if (index >= node->components_.size()) return false;`
3. Handle Updatable: `if (auto* upd = dynamic_cast<Updatable*>(node->components_[index].get())) { std::erase(updatables_, upd); }`
4. `node->components_.erase(node->components_.begin() + index); return true;`

**`insert_component_raw_at` implementation** (`world.cpp`):
1. `auto* node = lookup_node(id); BUDDD_ASSERT(node != nullptr && !node->pending_destroy_);`
2. `Component* ptr = component.get(); ptr->world_ = this; ptr->entity_id_ = id;`
3. Clamp index: `if (index > node->components_.size()) index = node->components_.size();`
4. `node->components_.insert(node->components_.begin() + index, std::move(component));`
5. `ptr->on_attach();`
6. Auto-register Updatable: `if (auto* upd = dynamic_cast<Updatable*>(ptr)) { updatables_.push_back(upd); }`
7. Return `*ptr`

No new includes needed in `world.h` (no forward declaration required — the new methods take only `EntityId`, `size_t`, and `std::unique_ptr<Component>` which are already included via existing headers).

---

### 2. `AddComponentCommand` — New command

**File**: `src/editor/commands/add_component_command.h`

**Class**: `AddComponentCommand final : public Command` in `namespace buddd::editor`.

**Constructor**: Takes `buddd::engine::EntityId entity_id, std::string component_type_name`. Stores members.

**Private members**:
- `buddd::engine::EntityId entity_id_`
- `std::string component_type_name_`
- `std::optional<size_t> component_index_` — set on execute (index of newly added component, always at the back)
- `Selection pre_execution_selection_` — saved on execute, restored on undo

**`execute(ctx)`**:
1. Save selection: `pre_execution_selection_ = ctx.editor.selection().snapshot();`
2. Validate entity: `auto& world = ctx.editor.world(); auto entity = world.entity(entity_id_);` — if `entity.id() == EntityId::none()`, log `WARN` "AddComponentCommand: entity {} not found", return.
3. Get registry: `auto& registry = ctx.engine.services.registry();`
4. Check type exists: `const auto* info = registry.describe(component_type_name_);` — if null, log `ERROR` "AddComponentCommand: unregistered type '{}'", return.
5. Create component: `auto comp_result = registry.create(component_type_name_);` — if error, log `ERROR` "...", return.
6. Record component count BEFORE adding: `size_t count_before = entity.component_count();`
7. Attach: `world.add_component_raw(entity_id_, std::move(*comp_result));`
8. Store index for undo: `component_index_ = count_before;` (the newly added component is at the back, which was `count_before` before addition)
9. Mark dirty: `ctx.editor.mark_dirty();`
10. Log: `BUDDD_LOG_TAGGED_DEBUG("Editor:Command", "AddComponent: entity={} type={} index={}", entity_id_.index, component_type_name_, *component_index_);`

**`undo(ctx)`**:
1. Validate entity exists — log WARN and return if not.
2. Validate `component_index_` has value — if not, return (should never happen in normal flow).
3. Call `world.remove_component_at(entity_id_, *component_index_);`
4. Restore selection: `ctx.editor.selection().restore(pre_execution_selection_);`
5. Mark dirty: `ctx.editor.mark_dirty();`
6. Log: `BUDDD_LOG_TAGGED_DEBUG("Editor:Command", "AddComponent UNDO: entity={} type={}", entity_id_.index, component_type_name_);`

**NOTE**: This command does NOT check for duplicate components. Since the engine allows multiple components of the same type on an entity (the rationale for the index-based approach), the AddComponentCommand permits adding any type regardless of whether the entity already has a component of that type.

**`name()`**: Returns `"Add Component"`.

**`try_update_new_value()`**: Returns `false` (default, no merge).

**Includes**: `command.h`, `editor.h`, `editor_context.h`, `editor_selection.h`, `scene/world.h`, `scene/component_registry/component_registry.h`, `log/log.h`, `<string>`, `<string_view>`, `<optional>`, `<memory>`.

---

### 3. `RemoveComponentCommand` — New command

**File**: `src/editor/commands/remove_component_command.h`

**Class**: `RemoveComponentCommand final : public Command` in `namespace buddd::editor`.

**Constructor**: Takes `buddd::engine::EntityId entity_id, std::string component_type_name, size_t component_index`. The `component_index` is the position of the component in the entity's component vector at the time the command is CREATED (which is the same as at execute time in single-frame UI flow). Stores members.

**Private members**:
- `buddd::engine::EntityId entity_id_`
- `std::string component_type_name_`
- `size_t component_index_` — position of the component in the entity's `components_` vector (set at construction time, remains constant for this command instance)
- `YAML::Node serialized_state_` — full serialized state captured on execute
- `Selection pre_execution_selection_` — saved on execute, restored on undo

**`execute(ctx)`**:
1. Save selection: `pre_execution_selection_ = ctx.editor.selection().snapshot();`
2. Validate entity: `auto& world = ctx.editor.world(); auto entity = world.entity(entity_id_);` — if `entity.id() == EntityId::none()`, log `WARN` "RemoveComponentCommand: entity {} not found", return.
3. Get registry: `auto& registry = ctx.engine.services.registry();`
4. Check type exists: `const auto* info = registry.describe(component_type_name_);` — if null, log `ERROR` "RemoveComponentCommand: unregistered type '{}'", return.
5. **Safety check — verify component at stored index matches expected type**: Build target type_index from info (the SceneSaver pattern). Check `component_index_ < entity.component_count()` and `std::type_index(typeid(entity.component_at(component_index_))) == target_type`. If out of bounds or type mismatch, log `WARN` "RemoveComponentCommand: expected component '{}' at index {} but found different type — indices may have shifted since command creation", return (no-op).
6. Serialize full state: `auto& comp = entity.component_at(component_index_); auto ser_ctx = SerializationContext{ctx.engine.services.assets()}; auto state = info->serialize(comp, ser_ctx); serialized_state_ = YAML::Clone(state);`
7. Remove: `world.remove_component_at(entity_id_, component_index_);`
8. Mark dirty: `ctx.editor.mark_dirty();`
9. Log debug: "RemoveComponent: entity={} type={} index={} properties={}"

**`undo(ctx)`**:
1. Validate entity exists — log WARN and return if not.
2. Get registry.
3. Get info: `const auto* info = registry.describe(component_type_name_);` — if null, log ERROR, return.
4. Create component: `auto comp_result = registry.create(component_type_name_);` — if error, log ERROR, return.
5. Deserialize state: `info->deserialize(*comp_result, serialized_state_, SerializationContext{ctx.engine.services.assets()});` — if error, log `WARN` but continue (component still attached with default properties).
6. Attach at stored index: `world.insert_component_raw_at(entity_id_, component_index_, std::move(*comp_result));`
7. Restore selection: `ctx.editor.selection().restore(pre_execution_selection_);`
8. Mark dirty: `ctx.editor.mark_dirty();`
9. Log debug: "RemoveComponent UNDO: entity={} type={} index={}"

**`name()`**: Returns `"Remove Component"`.

**`try_update_new_value()`**: Returns `false` (default, no merge).

**Includes**: Same as AddComponentCommand plus `#include <yaml-cpp/yaml.h>` and `#include "scene/component_registry/serialization_context.h"` and `#include "scene/component_registry/component_info.h"`.

---

### 4. PropertiesPanel UI changes

**`properties_panel.h`** — Add:
```cpp
// New private methods (after existing helpers, around line 33)
auto draw_add_component_button(EditorContext const& ctx, buddd::engine::EntityId entity_id) -> void;
auto draw_add_component_popup(EditorContext const& ctx, buddd::engine::EntityId entity_id) -> void;

// New private state (after last_selection_gen_ around line 29)
char add_component_filter_[64] = "";
std::string pending_auto_expand_type_;  // cleared after one auto-expand
```

**`properties_panel.cpp`** changes:

- Add includes:
  ```cpp
  #include "commands/add_component_command.h"
  #include "commands/remove_component_command.h"
  #include <algorithm>
  #include <cctype>   // std::tolower
  ```

- **`draw_ui()`** — after `draw_component_sections(ctx, entity_id);`, add:
  ```cpp
  draw_add_component_button(ctx, entity_id);
  ```

- **`draw_component_sections()`** — For each non-Transform component section (all components except the Transform section, which is handled separately in `draw_transform_section()`):

  The loop variable `i` IS the component index in the entity's component vector. Use it when constructing the RemoveComponentCommand.

  Replace the existing collapsing header section (from `ImGui::PushStyleVar` around line 321 through `if (!open) continue;` around line 325) with:

  ```cpp
  ImGui::PushID(static_cast<int>(i));
  
  // Determine flags: auto-expand for newly added component
  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
  if (!pending_auto_expand_type_.empty() && pending_auto_expand_type_ == type_name) {
      flags |= ImGuiTreeNodeFlags_DefaultOpen;
      pending_auto_expand_type_.clear();
  }
  
  // Draw collapsing header
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));
  bool open = ImGui::CollapsingHeader(type_name.data(), flags);
  ImGui::PopStyleVar();
  
  // Remove ⓧ button on the same line, right-aligned
  // Use `i` (NOT type_name) as the component identifier for the command
  float remove_button_width = ImGui::GetFrameHeight();
  ImGui::SameLine(ImGui::GetContentRegionAvail().x - remove_button_width);
  if (ImGui::SmallButton("X")) {
      auto cmd = std::make_unique<RemoveComponentCommand>(entity_id, std::string(type_name), i);
      ctx.editor.command_stack().execute(std::move(cmd), ctx);
  }
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
      ImGui::SetTooltip("Remove %s component", type_name.data());
  }
  
  ImGui::PopID();
  
  if (!open) continue;
  ```
  
  NOTE: The Transform section is NOT modified — no remove button.

- **New `draw_add_component_button()`**:
  ```cpp
  auto PropertiesPanel::draw_add_component_button(EditorContext const& ctx,
                                                   buddd::engine::EntityId entity_id) -> void {
      ImGui::Separator();
      ImGui::Dummy(ImVec2(0.0f, 2.0f));
      ImGui::Indent(8.0f);
      float button_width = ImGui::GetContentRegionAvail().x;
      if (ImGui::Button("+ Add Component", ImVec2(button_width, 0.0f))) {
          ImGui::OpenPopup("Add Component");
          add_component_filter_[0] = '\0';
          // Log popup opened
          auto& registry = ctx.engine.services.registry();
          BUDDD_LOG_TAGGED_DEBUG("Editor:Properties",
              "AddComponent popup opened, {} types available", registry.all_types().size());
      }
      ImGui::Unindent(8.0f);
      draw_add_component_popup(ctx, entity_id);
  }
  ```

- **New `draw_add_component_popup()`** — Shows ALL registered types (does NOT filter out already-present types, since duplicates are allowed):
  ```cpp
  auto PropertiesPanel::draw_add_component_popup(EditorContext const& ctx,
                                                  buddd::engine::EntityId entity_id) -> void {
      if (!ImGui::IsPopupOpen("Add Component")) return;
      
      auto& registry = ctx.engine.services.registry();
      auto& world = ctx.editor.world();
      auto entity = world.entity(entity_id);
      if (entity.id() == EntityId::none()) return;
      
      ImGui::SetNextWindowSize(ImVec2(280, 320), ImGuiCond_FirstUseEver);
      if (ImGui::BeginPopupModal("Add Component", nullptr, ImGuiWindowFlags_None)) {
          // Filter field
          ImGui::InputTextWithHint("##filter", "Filter types...", add_component_filter_, sizeof(add_component_filter_));
          ImGui::Separator();
          
          // Get filter string (lowercase)
          std::string filter_str(add_component_filter_);
          std::transform(filter_str.begin(), filter_str.end(), filter_str.begin(),
              [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
          
          // Collect ALL registered types (no duplicate filtering — duplicates allowed),
          // sort alphabetically
          std::vector<const buddd::engine::ComponentInfoBase*> types;
          for (const auto* info : registry.all_types()) {
              types.push_back(info);
          }
          std::sort(types.begin(), types.end(), [](const auto* a, const auto* b) {
              return a->type_name() < b->type_name();
          });
          
          // Scrollable list
          ImGui::BeginChild("##comp_list", ImVec2(0, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()), true);
          bool any_visible = false;
          for (const auto* info : types) {
              auto tn = info->type_name();
              
              // Apply filter (case-insensitive substring match)
              if (!filter_str.empty()) {
                  std::string tn_lower(tn);
                  std::transform(tn_lower.begin(), tn_lower.end(), tn_lower.begin(),
                      [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
                  if (tn_lower.find(filter_str) == std::string::npos) continue;
              }
              
              any_visible = true;
              if (ImGui::Selectable(tn.data())) {
                  auto cmd = std::make_unique<AddComponentCommand>(entity_id, std::string(tn));
                  ctx.editor.command_stack().execute(std::move(cmd), ctx);
                  pending_auto_expand_type_ = std::string(tn);
                  ImGui::CloseCurrentPopup();
              }
          }
          if (!any_visible) {
              ImGui::TextDisabled("No matching components");
          }
          ImGui::EndChild();
          
          // Close button
          if (ImGui::Button("Close", ImVec2(120, 0))) {
              ImGui::CloseCurrentPopup();
          }
          
          ImGui::EndPopup();
      }
  }
  ```

**NOTE on spec deviations (intentional)**:
- **AC-005** (prevent duplicates): Not implemented. The index-based approach intentionally allows duplicate components of the same type. AddComponentCommand does not check for duplicates.
- **AC-022** (filter out already-present types): Not implemented. The popup shows ALL registered types, allowing the user to add a second component of the same type if desired.
- **AC-013** (safe if component not found): Extended — RemoveComponentCommand also checks that the component at the stored index matches the expected type name. If indices shift, the command is a no-op with a warning.

These deviations are a direct consequence of D-10 (index-based component identification), which was decided because the engine supports multiple components of the same type on the same entity.

---

### 5. Auto-expand behavior

Use `pending_auto_expand_type_` member to track the most recently added component type name. Set it in `draw_add_component_popup()` immediately after executing AddComponentCommand. In `draw_component_sections()`, for the component whose `type_name` matches `pending_auto_expand_type_`, pass `ImGuiTreeNodeFlags_DefaultOpen` and clear `pending_auto_expand_type_`. This ensures the newly added section is expanded on its first draw, and the flag is consumed exactly once.

---

### 6. Index stability considerations

These are critical design invariants that the Code Agent must preserve:

1. **Index shifts on remove**: When component at index 2 is removed, all subsequent components (indices 3, 4, 5...) shift down by 1. Each RemoveComponentCommand stores the index at creation time, which is correct at execute time because the command is created and executed in the same UI frame. No other operation changes component ordering between creation and execution in the single-frame UI flow.

2. **Undo restores at original position**: RemoveComponentCommand stores `component_index_` at construction time. On undo, `insert_component_raw_at(entity_id_, component_index_, ...)` inserts at that exact position, restoring the original ordering.

3. **Add then remove**: If the user adds a component (gets index N-1), then removes a component at a different index, each command's stored index remains valid. The AddCommand's index is correct for its own undo context because removal of OTHER components does not affect the add command's stored index (the component was added at the back and stays at a stable position unless it itself is removed).

4. **Safety check**: Before removing, RemoveComponentCommand verifies that the component at `component_index_` matches the expected `type_name` via type_index comparison. If index shifting occurred (e.g., due to external operations not going through the command stack), the command logs a warning and becomes a no-op.

5. **Multiple adds of same type**: Each AddComponentCommand stores its own index. Undoing the first add removes the component at that index. The second add's component remains. This works correctly because each command's index is independent.

---

### 7. Selection preservation after remove

The RemoveComponentCommand saves the selection snapshot at the start of `execute()`. The remove operation itself does NOT modify the selection — the entity remains selected because the component removal does not go through the selection system. On undo, the selection is restored from the saved snapshot (which matches the selection at time of removal). This ensures G-06 (Selection preserved after remove) works correctly.

---

## Required tests

### Unit tests — `tests/editor/add_remove_component_commands_tests.cpp`

Follow the existing `TestContext` pattern from `component_property_commands_tests.cpp`.

#### AddComponentCommand tests

| Test | Spec AC | Description |
|------|---------|-------------|
| Compile check | AC-001 | Construct `AddComponentCommand(EntityId{1,0}, "camera")`, verify non-null, name == "Add Component". |
| Execute creates component and stores index | AC-002 | Create entity, execute AddComponentCommand("camera"), verify entity now has CameraComponent via `entity.get_component<CameraComponent>()`, verify `component_index_` was stored. Verify `editor.is_dirty()`. |
| Undo removes component at stored index | AC-003 | Add component, undo, verify component count returns to original. Verify component removed at the stored index. Verify editor is dirty after undo. |
| Safe on invalid entity | AC-004 | Execute AddComponentCommand with invalid EntityId — verify no crash. |
| Allows adding a second instance of same type | (contradicts AC-005) | Entity with CameraComponent already, execute AddComponentCommand("camera") again — verify component count increases by 1 (duplicate NOT prevented). |
| Unregistered type handled | AC-006 | Execute AddComponentCommand("nonexistent") — verify no crash. |
| try_update_new_value returns false | AC-008 | Call `cmd->try_update_new_value(YAML::Node(), ctx, "")`, verify returns false. |
| Execute marks dirty | AC-027 | Clear dirty, execute, verify dirty set again. |
| Undo also marks dirty | AC-027 | Execute, clear dirty, undo, verify dirty set. |
| Component index correctness after multi-add | — | Add two components sequentially, verify first command's index is 0 (or whatever count was before), second command's index is 1. Undo first, verify index 0 is removed and second component shifts to index 0. |

#### RemoveComponentCommand tests

| Test | Spec AC | Description |
|------|---------|-------------|
| Compile check | AC-009 | Construct `RemoveComponentCommand(entity_id, "camera", 0)`, verify name == "Remove Component". |
| Execute with index 0 removes first component | AC-010 | Create entity with two components, execute RemoveComponentCommand("camera", 0), verify first component removed, second component still present. Verify `serialized_state_` stored. |
| Execute with index last removes last component | AC-010 | Create entity with two components, execute RemoveComponentCommand("point_light", 1) (assuming point_light is second), verify last component removed. |
| Undo restores at same position | AC-011 | Set intensity=2.0 on PointLightComponent at index 1, remove it, undo, verify component is back at index 1 with same intensity value. |
| Safe on invalid entity | AC-012 | Execute with invalid EntityId — no crash. |
| Safe when index out of bounds | AC-013 variant | Entity with 1 component, execute RemoveComponentCommand("camera", 5) — out of bounds, no crash, warning logged. |
| Safe when type at index doesn't match | — | Entity has CameraComponent at index 0, execute RemoveComponentCommand("point_light", 0) — type mismatch, no crash, warning logged. |
| Unregistered type handled | AC-014 | Execute RemoveComponentCommand("nonexistent", 0) — no crash. |
| try_update_new_value returns false | AC-016 | Call cmd->try_update_new_value(...), verify returns false. |
| Execute marks dirty | AC-027 | Execute, verify dirty set. Undo, verify dirty still set. |
| Selection preserved after remove | AC-026 | Select entity, remove component, verify `editor.selection().primary()` returns the entity ID. |

#### World method tests

| Test | Description |
|------|-------------|
| remove_component_at: valid | Create entity with CameraComponent, call `world.remove_component_at(id, 0)`, verify returns true and component count decreased by 1. |
| remove_component_at: out of bounds | Call with index == component_count, verify returns false. |
| remove_component_at: invalid entity | Call with EntityId::none(), verify returns false. |
| remove_component_at: pending_destroy entity | Destroy entity (but don't flush), call remove_component_at, verify returns false. |
| insert_component_raw_at: insert at 0 | Create entity, insert a CameraComponent at index 0, verify it's the first component. |
| insert_component_raw_at: insert in middle | Create entity with 2 components, insert a third at index 1, verify ordering. |
| insert_component_raw_at: insert at end | Insert at index == component_count, verify it's appended at the back. |
| insert_component_raw_at: insert past end | Insert at index > component_count, verify it's clamped to the back. |
| remove then insert at same index | Remove component at index 0, then insert new component at index 0, verify restored at correct position. |

### E2E / Integration verification

Verification of the UI elements (Add Component button, popup, remove ⓧ button) requires a display (ImGui) and is deferred to manual smoke testing per the spec. The following manual steps verify the full feature:

1. Run `buddd edit`, open a scene, select an entity.
2. Verify "Add Component" button is visible at bottom of Properties Panel (AC-020).
3. Click "Add Component" — verify popup opens with filter field (AC-021).
4. Type in filter field — verify list filters case-insensitively (AC-023).
5. Verify ALL registered types are shown (including already-present ones — duplicate is allowed).
6. Click a type — verify component added, section auto-expanded (AC-024, AC-025).
7. Verify ⓧ button on non-Transform sections (AC-018).
8. Verify NO ⓧ button on Transform section (AC-017).
9. Click ⓧ — verify component removed, entity stays selected (AC-019, AC-026).
10. Ctrl+Z — verify component restored with exact property values at same position (AC-003, AC-011).
11. Ctrl+Shift+Z — verify component removed again.
12. Remove last non-Transform component — verify only Transform remains.
13. Add the same component type twice — verify both instances appear and can be individually removed.

---

## Edge cases

| Case | Expected behaviour |
|------|-------------------|
| Entity with no components (only Transform) | Only Transform section and Add Component button shown. No remove buttons anywhere. |
| Entity already has all registered types | Add Component popup still shows all types (duplicates allowed). |
| Filter text matches no types | Popup list shows "No matching components". |
| Rapid add/remove of same type | Each operation creates a separate Command with its own index snapshot. Undo steps back through each. |
| Component with complex property state | Full YAML serialization round-trip captures all property state via `ComponentInfoBase::serialize()/deserialize()`. |
| Entity destroyed between command creation and execution | Command detects invalid entity, logs warning (`BUDDD_LOG_TAGGED_WARN`), returns without action. |
| RemoveComponentCommand: component type at index doesn't match expected type | Command detects type mismatch via type_index comparison, logs warning, returns without action (no crash, no data loss). |
| Undo after scene reload (World replaced) | CommandStack is cleared on scene load per existing invariants. No stale commands. |
| Multiple undo/redo cycles | Commands store idempotent state (type name + index for add, serialized YAML + index for remove). Multiple cycles work correctly. |
| Component type with zero properties | After adding, section appears expanded but shows "No editable properties" text (existing behavior). Can still be removed via ⓧ. |
| Non-Transform component is the only component | Remove it → only Transform remains. Undo restores it with serialized state at its original index. |
| Empty filter field in popup | All registered types shown, alphabetically sorted. |
| Index shift after external component manipulation | RemoveComponentCommand's safety check catches index shifts, logs warning, no-ops. |
| Adding component when entity already has many components | New component is added at the back (`component_count` before addition becomes its index). |
| `ComponentRegistry::create()` fails (unregistered type) | Command logs ERROR, returns without adding. No crash. |
| `ComponentInfoBase::deserialize()` fails during undo | Command logs WARNING, still attaches component with default properties. |
| `World::add_component_raw()` fails | Assertion (`BUDDD_ASSERT`) — this indicates a programming error (null node or pending_destroy). Command does not catch it. |

---

## Security impact

None. All operations are in-memory. No file I/O, authentication, or authorization boundaries are crossed. Command serialization uses in-memory YAML nodes only.

## Data and migration impact

None. No schema changes, no data migrations, no seed data changes. The `serialized_state_` YAML stored in `RemoveComponentCommand` is temporary (lifetime of the command on the undo stack, max ~128 entries).

## API compatibility impact

- **`World` class gets two new public methods**: `remove_component_at()` and `insert_component_raw_at()`. Backward compatible — no existing API is modified or removed.
- **Two new Command classes** added to `buddd::editor` namespace. No existing API changed.
- **PropertiesPanel** gets two new private methods and two new private data members. No public interface change.
- The new methods on World are in the engine library, which is linked by both `buddd_engine` and `buddd_editor`. No ABI concerns for static linking.
- Note: The old type-name-based `remove_component_by_type_name()` from the previous contract is NOT added. Only index-based methods are added.

## Documentation impact

- **Wiki page `docs/wiki/editor/editor-panels.md`**: Update the Inspector Panel section to document the Add Component button/popup and Remove Component ⓧ button.
- **Wiki page `docs/wiki/domain/glossary.md`**: Add `AddComponentCommand`, `RemoveComponentCommand`, `remove_component_at`, `insert_component_raw_at`.
- **Wiki page `docs/wiki/architecture/module-map.md`**: Update Editor section to include `add_component_command.h` and `remove_component_command.h` as new files in `src/editor/commands/`.
- **Other specs**: The north-star UX spec (`.specs/sprint-2026-06/editor-ux-design/spec.md`) and the F-06 spec (`.specs/sprint-2026-06/inspector-component-properties/spec.md`) should be updated by the wiki-agent or human validator to remove "No Add Component" and "No Remove Component" non-goals. Additionally, AC-005 (prevent duplicates) and AC-022 (filter already-present types) in the spec.md for this feature should be reviewed for consistency with the index-based approach.

## ADR impact

This implementation does NOT require a new ADR. The design is consistent with:
- ADR-027 (Editor Architecture) — commands in `src/editor/commands/`, no SDL3/OpenGL/GLM headers.
- ADR-028 (Component Type Registry) — uses `ComponentRegistry`, `ComponentInfoBase`, `SerializationContext` as designed.
- ADR-029 (Editor UX Decisions) — extends Properties Panel UI consistent with the north-star UX spec.

## Done criteria

- [ ] **IC-001**: `src/engine/scene/world.h` has `remove_component_at(EntityId, size_t) -> bool` and `insert_component_raw_at(EntityId, size_t, unique_ptr<Component>) -> Component&` declarations after `add_component_raw()`.
- [ ] **IC-002**: `src/engine/scene/world.cpp` implements `remove_component_at()` — validates node, checks index bounds, handles Updatable cleanup via `dynamic_cast`, erases from `components_`.
- [ ] **IC-003**: `src/engine/scene/world.cpp` implements `insert_component_raw_at()` — validates node, sets `world_` and `entity_id_` on the injected component, clamps index to valid range, inserts at position, calls `on_attach()`, auto-registers Updatable.
- [ ] **IC-004**: `src/editor/commands/add_component_command.h` exists with class `AddComponentCommand final : public Command` and member `std::optional<size_t> component_index_`.
- [ ] **IC-005**: `AddComponentCommand::execute()` validates entity, checks type exists (no duplicate check), creates via `registry.create()`, records `component_count_before`, attaches via `world.add_component_raw()`, stores `component_index_ = count_before`, saves selection snapshot, marks dirty.
- [ ] **IC-006**: `AddComponentCommand::undo()` removes component via `world.remove_component_at(entity_id_, *component_index_)`, restores selection, marks dirty.
- [ ] **IC-007**: `AddComponentCommand::name()` returns `"Add Component"`, `try_update_new_value()` returns false.
- [ ] **IC-008**: `src/editor/commands/remove_component_command.h` exists with class `RemoveComponentCommand final : public Command`, constructor taking `(EntityId, string type_name, size_t component_index)`, storing `component_index_` at construction.
- [ ] **IC-009**: `RemoveComponentCommand::execute()` validates entity, checks type exists, verifies component at `component_index_` matches expected type (type_index safety check), serializes full state via `info->serialize()` and stores `serialized_state_` as `YAML::Clone`, removes via `world.remove_component_at(entity_id_, component_index_)`, saves selection snapshot, marks dirty.
- [ ] **IC-010**: `RemoveComponentCommand::undo()` creates component via `registry.create()`, deserializes via `info->deserialize()` (warning on failure, continues), inserts via `world.insert_component_raw_at(entity_id_, component_index_, ...)`, restores selection, marks dirty.
- [ ] **IC-011**: `RemoveComponentCommand::name()` returns `"Remove Component"`, `try_update_new_value()` returns false.
- [ ] **IC-012**: `src/editor/panels/properties_panel.h` has new private methods `draw_add_component_button()`, `draw_add_component_popup()` and new state `add_component_filter_[64]`, `pending_auto_expand_type_`.
- [ ] **IC-013**: `src/editor/panels/properties_panel.cpp` `draw_ui()` calls `draw_add_component_button()` after component sections.
- [ ] **IC-014**: `draw_component_sections()` shows ⓧ remove button on each non-Transform component header, right-aligned in the same header line, using the loop index `i` as the `component_index` parameter for RemoveComponentCommand. Button has tooltip "Remove %s component".
- [ ] **IC-015**: `draw_component_sections()` checks `pending_auto_expand_type_` and passes `ImGuiTreeNodeFlags_DefaultOpen` for the matching component.
- [ ] **IC-016**: `draw_add_component_button()` renders a full-width "+ Add Component" button below a separator at the bottom of the component list.
- [ ] **IC-017**: `draw_add_component_popup()` renders a modal popup with: title "Add Component", filter text input, scrollable list of ALL registered types (not filtering out already-present ones), alphabetically sorted, filtered by case-insensitive substring match, "No matching components" when empty, Close button. On type click: pushes `AddComponentCommand` and sets `pending_auto_expand_type_`.
- [ ] **IC-018**: Safety check in RemoveComponentCommand: type mismatch between stored `component_index_` and expected `type_name` → no-op with WARN log. Invalid entity → no-op with WARN log. Unregistered type → no-op with ERROR log. Deserialize failure on undo → WARN + continue with defaults.
- [ ] **IC-019**: All debug log messages use `BUDDD_LOG_TAGGED_DEBUG("Editor:Command", ...)` matching spec observability table.
- [ ] **IC-020**: `tests/editor/add_remove_component_commands_tests.cpp` exists with at least the test cases listed in "Required tests" section above (AddComponentCommand tests, RemoveComponentCommand tests, World method tests).
- [ ] **IC-021**: All tests in the new test file pass: `buddd_tests [editor]` — zero failures.
- [ ] **IC-022**: Zero new compiler warnings from `src/engine/scene/`, `src/editor/panels/`, `src/editor/commands/`, and `tests/`.
- [ ] **IC-023**: Wiki pages updated as specified in "Documentation impact" section.

Each checklist item is verifiable by reading the file (IC-001–IC-019), running tests (IC-020–IC-021), checking build output (IC-022), or inspecting the wiki (IC-023).
