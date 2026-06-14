# SPEC-F-06b — Inspector — Add/Remove Components

## Problem

The Properties Panel (F-06) renders each entity's components as collapsible sections with property editors and supports undo/redo of property edits via `SetComponentPropertyCommand`. However, there is no way to:

- Add a new component to the selected entity from the panel.
- Remove an existing component from the selected entity from the panel.
- Undo/redo component addition or removal.

A user must currently edit scene YAML files manually or write code to attach/detach components. This breaks the interactive editing workflow and prevents rapid iteration on entity composition.

## Goals

| ID | Goal |
|---|---|
| G-01 | **AddComponentCommand**: A new Command that creates a component of a given type name on the selected entity, supporting undo via removal of the newly added component. |
| G-02 | **RemoveComponentCommand**: A new Command that removes a component from the selected entity, supporting undo via reconstruction from a serialized YAML snapshot taken at removal time. |
| G-03 | **Add Component button**: A button at the bottom of the Properties Panel's component section area that opens a searchable popup listing all registered component types not already present on the entity. |
| G-04 | **Remove Component button**: A small "ⓧ" button on each component section header (except Transform) that immediately removes that component. |
| G-05 | **New component auto-expand**: After adding a component via AddComponentCommand, its section is automatically expanded. |
| G-06 | **Selection preserved after remove**: After removing a component, the entity remains selected. |
| G-07 | **Non-regression**: All existing tests pass. Zero new warnings from modified directories. |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | **No confirmation dialog for remove** — removal is direct and undoable. |
| NG-02 | **No "last component" guard** — removing the last non-Transform component is allowed (undoable). |
| NG-03 | **No component reordering** — added components appear at the end of the component list. |
| NG-04 | **No multi-select component add/remove** — only operates on the primary selected entity. |
| NG-05 | **No play-mode read-only enforcement** — deferred to F-15. Add/Remove buttons visible and functional regardless of play state in this feature. |
| NG-06 | **No "Add Component" from entity creation defaults** — initial components are not populated from a template. |
| NG-07 | **No component reordering** — components are always added to the end. The RemoveComponentCommand stores the index at creation time for correct undo reconstruction. |
| NG-08 | **No drag-and-drop component reordering or transfer** — deferred. |
| NG-09 | **No component presets or favorites** — all registered types listed equally. |
| NG-10 | **No changes to the Transform section** — Transform remains always present, non-removable, with no remove button. |

## Actors

| Actor | Description |
|---|---|
| **Editor user** | Selects an entity in the Scene Panel, sees component sections in the Properties Panel, adds new components via the Add Component button, or removes existing components via the ⓧ button. Undoes/redoes these operations via Ctrl+Z / Ctrl+Shift+Z. |
| **Engine developer** | Registers new component types in `ComponentRegistry`. They automatically appear in the Add Component popup without code changes to the editor. |

## User-visible behavior

### A. Remove Component button placement

Each non-Transform component section header shows a small "ⓧ" remove button on the right side, same line as the collapsible header text and expand triangle.

```
▼ camera                             [ⓧ]
   fov_y      [  1.05]
   aspect     [  1.78]
```

- Uses a small inline button (e.g., `ImGui::SmallButton` with a cross/X icon or unicode ⓧ U+2A09, or `"X"` text).
- Button is positioned at the far right of the header row.
- **Hidden on the Transform section** (no remove button).
- Clicking the ⓧ button immediately removes the component and pushes a `RemoveComponentCommand` to the CommandStack.
- No confirmation dialog.
- The entity remains selected.

### B. Add Component button

Below the last component section (or directly below the Transform section if no other components), a full-width button appears:

```
───────────────────────────────────────
[            + Add Component           ]
```

- Button text: `"Add Component"` (preceded by a `+` icon or similar).
- Button width spans the full content area of the panel.
- Always visible when an entity is selected, regardless of how many components are already present.
- Clicking the button opens the Add Component popup (see below).

### C. Add Component popup (type selection)

The popup is a modal or dropdown that opens on click of the Add Component button:

```
┌─── Add Component ──────────────────┐
│ [ Filter...                  ]     │
│ ─────────────────────────────────  │
│ ○ camera                           │
│ ○ directional_light                │
│ ○ free_camera_movement             │
│ ○ mesh_renderer                    │
│ ○ point_light                      │
│ ○ spot_light                       │
└────────────────────────────────────┘
```

- **Title**: "Add Component" in the popup header.
- **Filter field**: A text input at the top. As the user types, the list filters to show only types whose name contains the typed substring (case-insensitive).
- **List**: All types registered in `ComponentRegistry::all_types()` are listed in alphabetical order.
- **Duplicates allowed**: Types already present on the entity are still shown — the user can add multiple instances of the same component type.
- **Click behavior**: Clicking a type name closes the popup, creates an `AddComponentCommand`, and pushes it to the CommandStack.
- **Empty state**: If the filter text matches no types, the popup shows "No matching components". If there are no registered types at all (unlikely outside of tests), the popup shows "No components available".
- **Esc/Ctrl+Click outside**: Closes the popup without action (standard ImGui popup behavior).

### D. New component auto-expand

Immediately after a component is added, its collapsible section opens automatically (expanded). Other sections retain their previous expand/collapse state.

### E. Layout reference (complete Properties Panel)

```
┌─────────────────────────────────────┐
│ [Entity Name]                [input]│
├─────────────────────────────────────┤
│ ▼ Transform                         │
│   Position  [■ X][0.00][■ Y][0.00]…│
│   Rotation  [■ X][0.00][■ Y][0.00]…│
│   Scale     [■ X][1.00][■ Y][1.00]…│
├─────────────────────────────────────┤
│ ▼ camera                        [ⓧ]│
│   fov_y      [  1.05]               │
│   aspect     [  1.78]               │
├─────────────────────────────────────┤
│ ▼ point_light                   [ⓧ]│
│   color      [■ ColorPicker]        │
│   intensity  [  1.00]               │
├─────────────────────────────────────┤
│       [   + Add Component    ]      │
└─────────────────────────────────────┘
```

## Key entities

### AddComponentCommand

```
AddComponentCommand
├── entity_id: EntityId
├── component_type_name: string       // e.g., "camera"
├── component_index: size_t           // stored on execute, for undo
├── execute(ctx)
│   ├── Validate entity exists
│   ├── ComponentRegistry::create(type_name) → Component
│   ├── World::add_component_raw(entity_id, component) — attached at back
│   ├── Store component_index = entity.component_count() - 1 for undo
│   └── ctx.editor.mark_dirty()
├── undo(ctx)
│   ├── Look up entity
│   ├── World::remove_component_at(entity_id, component_index)
│   └── ctx.editor.mark_dirty()
└── name() → "Add Component"
```

### RemoveComponentCommand

```
RemoveComponentCommand
├── entity_id: EntityId
├── component_type_name: string       // e.g., "camera"
├── component_index: size_t           // position in the component vector
├── serialized_state: YAML::Node      // full serialized component state before removal
├── execute(ctx)
│   ├── Validate entity exists
│   ├── (Safety check) Verify component at component_index matches expected type_name
│   ├── Serialize full component state via ComponentInfoBase::serialize()
│   ├── Store serialized_state via YAML::Clone()
│   ├── World::remove_component_at(entity_id, component_index)
│   └── ctx.editor.mark_dirty()
├── undo(ctx)
│   ├── Look up entity
│   ├── ComponentRegistry::create(type_name) → Component
│   ├── ComponentInfoBase::deserialize(component, serialized_state, ctx)
│   ├── World::insert_component_raw_at(entity_id, component_index, component)
│   └── ctx.editor.mark_dirty()
└── name() → "Remove Component"
```

### PropertiesPanel (UI additions)

```
PropertiesPanel
├── draw_ui(ctx)
│   ├── ... (existing: entity name, transform)
│   ├── draw_component_sections(ctx)        // existing, modified with remove buttons
│   │   └── For each component section:
│   │       ├── CollapsingHeader + remove ⓧ button (not on Transform)
│   │       └── Property table (existing)
│   └── draw_add_component_button(ctx)      // NEW
│       ├── Button "Add Component"
│       └── On click: open popup
├── draw_add_component_popup(ctx)           // NEW
│   ├── Filter text input
│   ├── Filtered list of ComponentRegistry::all_types()
│   └── On type click: push AddComponentCommand
└── draw_remove_component_button(...)       // NEW (inline in draw_component_sections)
```

## User stories

### Story 1 — Add a camera component to an entity (Priority: P1)

As an editor user, I want to add a camera component to an entity so that it becomes a camera in the scene.

**Given** an entity without a CameraComponent is selected
**When** I click the "Add Component" button at the bottom of the Properties Panel
**Then** a popup opens titled "Add Component" with a filter text field at the top and a list of available component types

**Given** the Add Component popup is open
**When** I type "camera" in the filter field
**Then** the list filters to show only "camera"

**Given** the filter shows "camera"
**When** I click "camera"
**Then** the popup closes
**And** a CameraComponent is attached to the entity
**And** a new collapsible "camera" section appears in the Properties Panel
**And** the "camera" section is expanded (visible properties)
**And** the scene is marked dirty

### Story 2 — Undo adding a component (Priority: P1)

As an editor user, I want to undo an accidental component addition.

**Given** I have just added a CameraComponent to an entity
**When** I press Ctrl+Z
**Then** the CameraComponent is removed from the entity
**And** the "camera" section disappears from the Properties Panel
**And** the scene retains its dirty state (still dirty from the addition that was undone)

**Given** the addition was undone
**When** I press Ctrl+Shift+Z (redo)
**Then** the CameraComponent is re-added
**And** the "camera" section reappears

### Story 3 — Remove a component (Priority: P1)

As an editor user, I want to remove a light component from an entity.

**Given** an entity with a PointLightComponent is selected
**When** I click the ⓧ button on the "point_light" section header
**Then** the PointLightComponent is removed
**And** the "point_light" section disappears from the Properties Panel
**And** the entity remains selected
**And** the scene is marked dirty

### Story 4 — Undo removing a component (Priority: P1)

As an editor user, I want to undo an accidental component removal and restore its exact property values.

**Given** I have just removed a PointLightComponent that had color=red and intensity=2.0
**When** I press Ctrl+Z
**Then** the PointLightComponent is restored
**And** its "color" property shows red
**And** its "intensity" property shows 2.0
**And** the "point_light" section reappears in the Properties Panel

### Story 5 — Remove the last non-Transform component (Priority: P2)

As an editor user, I want to remove all components from an entity, leaving only Transform.

**Given** an entity with only one component (e.g., CameraComponent) plus Transform
**When** I click the ⓧ button on the "camera" section header
**Then** the CameraComponent is removed
**And** the entity now only has Transform
**And** the Properties Panel shows only the Transform section and the Add Component button

**Given** the last component was removed
**When** I press Ctrl+Z
**Then** the CameraComponent is restored with its previous state

### Story 6 — Add component when entity already has all types (Priority: P3)

As an editor user, I want to see an informative empty state when there are no new components to add.

**Given** an entity already has every registered component type
**When** I click the "Add Component" button
**Then** the popup opens showing "No components to add" (or an empty filtered list)
**And** clicking outside or pressing Esc closes the popup

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `AddComponentCommand` exists with constructor taking (entity_id, component_type_name). | Unit test: compile check. |
| AC-002 | `AddComponentCommand::execute()` creates a component via `ComponentRegistry::create(type_name)` and attaches it via `World::add_component_raw()`. | Integration test: create entity, execute AddComponentCommand for "camera", verify entity now has a CameraComponent. |
| AC-003 | `AddComponentCommand::undo()` removes the component that was added. | Integration test: execute then undo, verify component count returns to original. |
| AC-004 | `AddComponentCommand::execute()` is safe if entity is invalid/stale (no crash, warning logged). | Integration test: destroy entity, execute AddComponentCommand → no crash, warning logged. |
| AC-005 | `AddComponentCommand::execute()` allows adding a component type even if the entity already has that type (duplicates permitted). | Integration test: entity already has CameraComponent, execute AddComponentCommand("camera") → entity now has 2 CameraComponent instances. |
| AC-006 | `AddComponentCommand::execute()` handles unregistered type names gracefully (no crash, error logged). | Unit test: execute AddComponentCommand("nonexistent") → no crash, error logged. |
| AC-007 | `AddComponentCommand::name()` returns `"Add Component"`. | Unit test. |
| AC-008 | `AddComponentCommand::try_update_new_value()` returns false (override not needed; default returns false). | Unit test: verify no-op merge. |
| AC-009 | `RemoveComponentCommand` exists with constructor taking (entity_id, component_type_name). | Unit test: compile check. |
| AC-010 | `RemoveComponentCommand::execute()` serializes the component state via `ComponentInfoBase::serialize()`, stores it, then removes the component. | Integration test: create entity with PointLightComponent, set intensity=2.0, execute RemoveComponentCommand("point_light"), verify component removed and serialized_state stored. |
| AC-011 | `RemoveComponentCommand::undo()` creates a fresh component via `ComponentRegistry::create()`, deserializes stored state via `ComponentInfoBase::deserialize()`, and attaches via `World::add_component_raw()`. | Integration test: remove PointLightComponent, then undo, verify component is back with intensity=2.0. |
| AC-012 | `RemoveComponentCommand::execute()` is safe if entity is invalid/stale (no crash, warning logged). | Integration test: destroy entity, execute RemoveComponentCommand → no crash, warning logged. |
| AC-013 | `RemoveComponentCommand::execute()` is safe if component not found on entity (no crash, warning logged). | Integration test: entity without CameraComponent, execute RemoveComponentCommand("camera") → no crash, warning logged. |
| AC-014 | `RemoveComponentCommand::execute()` handles unregistered type names gracefully (no crash, error logged). | Unit test: execute RemoveComponentCommand("nonexistent") → no crash, error logged. |
| AC-015 | `RemoveComponentCommand::name()` returns `"Remove Component"`. | Unit test. |
| AC-016 | `RemoveComponentCommand::try_update_new_value()` returns false (default behavior). | Unit test. |
| AC-017 | Transform section has NO remove ⓧ button. | Snapshot test: select entity, verify Transform header has no remove button. |
| AC-018 | Each non-Transform component section header shows a ⓧ remove button on the right side. | Snapshot test: select entity with CameraComponent, verify "camera" header contains a clickable remove button. |
| AC-019 | Clicking the ⓧ button on a component section header removes that component and pushes a `RemoveComponentCommand`. | Integration test: set up entity with CameraComponent, simulate button click, verify RemoveComponentCommand pushed and component removed. |
| AC-020 | The "Add Component" button is visible at the bottom of the Properties Panel when an entity is selected. | Snapshot test: select entity, verify "Add Component" button appears below all component sections. |
| AC-021 | Clicking the "Add Component" button opens a popup titled "Add Component" with a filter text input. | Snapshot test: click Add Component button, verify popup appears with filter field. |
| AC-022 | The Add Component popup lists all types from `ComponentRegistry::all_types()`. | Integration test: entity has CameraComponent, open popup, verify all registered types appear in the list (no filtering by already-present). |
| AC-023 | The Add Component popup filters as the user types (case-insensitive substring match). | Snapshot test: open popup, type "light", verify only "point_light", "directional_light", "spot_light" shown (or subset depending on registry). |
| AC-024 | Clicking a type in the Add Component popup pushes an `AddComponentCommand` and closes the popup. | Integration test: open popup, click "point_light", verify AddComponentCommand pushed and popup closed. |
| AC-025 | After adding a component, the new component section is expanded (auto-expand). | Snapshot test: add CameraComponent, verify its section is expanded showing properties. |
| AC-026 | After removing a component, the entity remains selected. | Integration test: remove component, verify `editor.selection().primary()` returns the entity ID. |
| AC-027 | Both AddComponentCommand and RemoveComponentCommand call `ctx.editor.mark_dirty()` on execute and undo. | Unit test: execute command, verify dirty flag; then undo, verify dirty flag still set. |
| AC-028 | All existing tests still pass. | Run `buddd_tests`. |
| AC-029 | Zero new warnings from `src/editor/`, `src/engine/scene/component_registry/`, and `tests/`. | Build with `cmake --build --preset debug`. |

## E2E Verification

| Method | Description |
|---|---|
| **Headless unit test (CI)** | Run `buddd_tests` with `[editor][commands]` and `[editor][properties-panel]` tags. Verify all AC tests pass. |
| **Manual smoke test (display)** | Run `buddd edit` with a scene. Select an entity. Verify: (1) Add Component button visible at bottom; (2) Click it → popup opens with filterable list; (3) Add a camera component → section appears expanded; (4) Click ⓧ on camera → component removed, section gone, entity stays selected; (5) Ctrl+Z → component restored with exact property values; (6) Ctrl+Shift+Z → component removed again; (7) Remove last non-Transform component → entity still has Transform; (8) Undo restores it. |
| **Build verification** | `cmake --build --preset debug` with zero new warnings from affected directories. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A user can add any registered component type from the Properties Panel in under 3 clicks. | Manual: select entity, click Add Component, click type in popup. |
| SC-002 | A user can remove any non-Transform component from the Properties Panel in 1 click (ⓧ). | Manual: click ⓧ on a component header → component removed. |
| SC-003 | Both add and remove operations are undoable via Ctrl+Z with correct property state restoration. | Manual: add component → Ctrl+Z → removed; remove component → Ctrl+Z → restored with exact properties. |
| SC-004 | The Add Component popup search filters effectively — typing filters the list to matching types in <100ms for up to 50 registered types. | Manual: type in filter, observe immediate list filtering. |
| SC-005 | Adding a component never crashes or produces duplicate/invalid entity states. | Automated: stress test add/remove/undo/redo cycle on same entity 100 times. |

## Edge cases

| Case | Expected behaviour |
|---|---|
| **Entity with no components (only Transform)** | Only Transform section and Add Component button shown. No remove buttons. |
| **Entity already has all registered component types** | All types are still shown in the popup — duplicates are allowed. The user can add another instance of any type. |
| **Filter text matches no types** | The Add Component popup list shows "No matching components" text. |
| **Rapid add/remove of the same component type** | Each operation creates a separate Command. Undo steps back through each. |
| **Component with complex property state** (e.g., `std::shared_ptr<Model>`) | Full YAML serialization round-trip captures all property state including asset references. |
| **Entity destroyed between command creation and execution** | Command detects invalid entity, logs warning, does nothing. |
| **Add Component on entity that already has that component type** | Duplicates are allowed — the component is added again at the end. |
| **Remove Component on entity that never had that component** | Warning logged, command is no-op. |
| **Undo after scene reload** (World replaced) | Invariant: CommandStack is cleared on scene load. No stale commands. |
| **Multiple undo/redo cycles** | Commands store idempotent state (serialized YAML for removal, type name string for addition). Multiple cycles work correctly. |
| **Component type with zero properties** | After adding, section appears expanded but shows "No editable properties". Can still be removed via ⓧ. |
| **Non-Transform component is the only component** | Remove it → only Transform remains. Undo restores it. |
| **Empty filter field in popup** | All registered component types are shown, alphabetically sorted. |

## Error cases

| Case | Expected behaviour |
|---|---|
| **ComponentRegistry::create() fails** (e.g., unregistered type) | AddComponentCommand::execute() logs error and returns without adding. No crash. |
| **ComponentInfoBase::deserialize() fails during undo** | RemoveComponentCommand::undo() logs a warning but still attaches the component (with default properties). The command is not partially applied. |
| **World::add_component_raw() fails** | Command logs error, does not mark dirty. |
| **Entity lookup fails during command execute** | Warning logged: `BUDDD_LOG_TAGGED_WARN("Editor:Command", "AddComponent: entity {} not found")` or similar. No-op. |
| **Out of memory during YAML serialization** | `std::bad_alloc` may be thrown. Consistent with existing editor behaviour. |
| **ImGui popup not rendered (panel hidden)** | `draw_ui()` is only called when panel is visible. Guarded by panel system. |
| **Component type name mismatch between serialization and registry** | If registry returns nullptr for `describe(type_name)`, the command logs error and aborts. |

## Permissions and security

- No changes to permissions or security posture.
- Component add/remove commands go through the Command system, which is bounded (128 entries).
- All operations are to in-memory data only. No file I/O during add/remove.
- Serialized YAML snapshots are stored in memory only, within the Command objects.
- No authentication or authorization boundaries are crossed.

## Observability

| Signal | Source |
|---|---|
| **AddComponentCommand execute** | Debug-level log: `BUDDD_LOG_TAGGED_DEBUG("Editor:Command", "AddComponent: entity={} type={}", entity_id.index, type_name)` |
| **AddComponentCommand undo** | Debug-level log: `BUDDD_LOG_TAGGED_DEBUG("Editor:Command", "AddComponent UNDO: entity={} type={}", entity_id.index, type_name)` |
| **RemoveComponentCommand execute** | Debug-level log: `BUDDD_LOG_TAGGED_DEBUG("Editor:Command", "RemoveComponent: entity={} type={} properties={}", entity_id.index, type_name, serialized_state.size())` |
| **RemoveComponentCommand undo** | Debug-level log: `BUDDD_LOG_TAGGED_DEBUG("Editor:Command", "RemoveComponent UNDO: entity={} type={}", entity_id.index, type_name)` |
| **Add Component popup opened** | Debug-level log: `BUDDD_LOG_TAGGED_DEBUG("Editor:Properties", "AddComponent popup opened, {} eligible types", eligible_count)` |
| **Duplicate component add prevented** | Warning: `BUDDD_LOG_TAGGED_WARN("Editor:Command", "AddComponent: entity {} already has component '{}'", entity_id.index, type_name)` |
| **Invalid entity in command** | Warning: `BUDDD_LOG_TAGGED_WARN("Editor:Command", "AddComponent/RemoveComponent: entity {} not found", entity_id.index)` |
| **Component not found for removal** | Warning: `BUDDD_LOG_TAGGED_WARN("Editor:Command", "RemoveComponent: component '{}' not found on entity {}", type_name, entity_id.index)` |
| **Unregistered type name** | Error: `BUDDD_LOG_TAGGED_ERROR("Editor:Command", "AddComponent: unregistered type '{}'", type_name)` |

## Documentation impact

The following existing wiki pages must be updated when this spec is implemented:

| Document | Reason for update |
|---|---|
| `docs/wiki/editor/editor-panels.md` | Update the Inspector Panel section to document the Add Component button/popup and Remove Component ⓧ button. |
| `docs/wiki/domain/glossary.md` | Add `AddComponentCommand`, `RemoveComponentCommand`. |
| `docs/wiki/architecture/module-map.md` | Update Editor section to include `add_component_command.h` and `remove_component_command.h` as new files. |

The north-star UX spec (`.specs/sprint-2026-06/editor-ux-design/spec.md`) should be updated to reflect that Add/Remove Component is now implemented (deviation D-02 from the F-06 spec is resolved).

The F-06 spec (`.specs/sprint-2026-06/inspector-component-properties/spec.md`) should be updated to remove the "No Add Component" and "No Remove Component" non-goals (NG-01, NG-02) and deviation D-02, since these are now addressed.

## Out of scope

- Component reordering (drag and drop sections).
- Multi-select entity editing.
- Component presets or templates.
- Drag-and-drop asset references.
- Play-mode read-only enforcement.
- Component duplication (copy-paste).
- Removing the Transform component.
- Confirmation dialog for removal.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `ComponentRegistry::create(type_name)` returns a `std::unique_ptr<Component>` to a fully default-initialized component of the requested type. |
| A-02 | `World::add_component_raw(EntityId, unique_ptr<Component>)` is available and returns a `Component&` reference to the now-attached component. |
| A-03 | `World::remove_component_at(EntityId, size_t)` and `World::insert_component_raw_at(EntityId, size_t, unique_ptr<Component>)` are added to the engine for index-based component management. |
| A-04 | `ComponentInfoBase::serialize(comp, ctx)` returns a complete YAML representation of all component properties, suitable for reconstruction via `deserialize()`. |
| A-05 | `ComponentInfoBase::deserialize(comp, node, ctx)` can consume the YAML output of `serialize()` to restore the component to an identical state. |
| A-06 | The entity remains stable (not destroyed) between command construction and execution. If destroyed, the command handles it gracefully (see error cases). |
| A-07 | `ImGui::SmallButton` or equivalent is suitable for the ⓧ remove button. The exact visual representation (unicode character or text "X") is an implementation detail. |
| A-08 | ImGui popups (`ImGui::OpenPopup` / `ImGui::BeginPopupModal`) are the appropriate mechanism for the Add Component type selection UI. |
| A-09 | `ComponentRegistry::all_types()` returns `span<const ComponentInfoBase*>` — the pointers are valid for the lifetime of the editor session. |
| A-10 | Component types are identified by their `type_name()` string, which is unique and stable for the session. |
| A-11 | The `serialized_state` YAML stored in `RemoveComponentCommand` uses yaml-cpp shared node semantics (shallow copy), so storing it is cheap. |
| A-12 | Entity's component iteration order (`Entity::component_at(i)`) matches the attachment order. New components via `add_component_raw` are appended at the end. |

## Open questions

| ID | Question | Priority | Impact |
|---|---|---|---|---|
| Q-01 | **Should index-based removal use a safety check?** When RemoveComponentCommand::execute() runs, the index provided by the panel may no longer match the expected component type if another operation shifted indices between command creation and execution. A safety check (`component_at(index)` matches expected `type_name`) prevents accidental removal of the wrong component. | **Medium** — safety check adds ~5 lines and prevents a subtle bug. | Implementation detail resolved in contract: safety check included. |

## Self-validation checklist

| Check | Pass/Fail |
|---|---|
| Is every acceptance criterion testable? | ✅ Yes — all ACs have clear verification methods. |
| Are all edge cases and error cases covered? | ✅ Yes — 11 edge cases and 7 error cases listed. |
| Are there any hidden implementation decisions? | ✅ No — the spec specifies behavior, not implementation (popup vs dropdown, button style are flexible). |
| Are success criteria measurable and technology-agnostic? | ✅ Yes — SC-001 through SC-005 are about user outcomes, not implementation details. |
| Are user stories prioritized and independently testable? | ✅ Yes — P1/P2/P3 stories with Given/When/Then. |
| Are there no more than 10 `[NEEDS CLARIFICATION]` markers? | ✅ Yes — zero markers. |
| Does the spec contradict any accepted spec? | ✅ No — aligns with F-06 north-star UX. Explicitly fills the gap left by NG-01/NG-02 in inspector-component-properties. |
| Are assumptions documented for every reasonable default made? | ✅ Yes — all 12 assumptions documented. |
| Does the spec satisfy the Definition of Ready? | ✅ Yes — see DoR check below. |

### Definition of Ready check

| Criterion | Status |
|---|---|
| Scope is clearly defined (what is included and what is explicitly excluded) | ✅ Yes — Goals vs Non-goals clearly separate in-scope from out-of-scope. |
| Dependencies on other features, modules, or external systems are identified | ✅ Yes — depends on ComponentRegistry, CommandStack, PropertiesPanel, YAML serialization. |
| Edge cases and error conditions are described | ✅ Yes — 11 edge cases, 7 error cases. |
| The expected behavior is unambiguous and testable | ✅ Yes — ACs are specific, Gherkin stories, behavioral descriptions. |
| The spec defines how the feature will be verified end-to-end | ✅ Yes — manual smoke test + CI unit tests + build verification. |
| Interface changes (CLI flags, API signatures, config keys) are documented | ✅ Yes — new Command classes and PropertiesPanel UI. |
| Existing documentation that must be updated is listed | ✅ Yes — wiki pages, north-star UX spec, and F-06 spec listed. |
| Technical constraints are identified | ✅ Yes — type-erased component removal path, YAML serialization round-trip. |
| Risks or unknowns are surfaced | ✅ Yes — Q-01 about type-erased removal API. |
| Performance or resource implications are noted | ✅ Yes — assumptions about cheap YAML copy, small number of components. |
