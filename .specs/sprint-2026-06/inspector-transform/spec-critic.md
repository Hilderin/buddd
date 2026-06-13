# Spec Review — SPEC-F-05 Inspector — Transform

## Re-review (human-validation design change) — ACCEPTED

**Verdict**: The design change from `const SerializationContext* ser_ctx = nullptr` to `const EditorContext& ctx` has been applied **consistently** throughout the spec. No new blocking issues introduced. Spec passes the re-review.

**Summary of change verification**:

| Location | Status | Notes |
|---|---|---|
| `InspectorTypeEditor::draw()` | ✅ | Lines 94-95, 314-315: `const EditorContext& ctx` |
| `TypedInspectorEditor<T>::draw()` | ✅ | Lines 112-113, 321-322: `const EditorContext& ctx` |
| `TypedInspectorEditor<T>::draw_typed()` | ✅ | Lines 325-326: `const EditorContext& ctx` |
| `InspectorTypeEditorRegistry::draw<T>()` | ✅ | Lines 140-141, 343-344: `const EditorContext& ctx` |
| Fallback behavior (lines 156-162) | ✅ | Uses `ctx.engine.services.assets()` to construct `SerializationContext`; delegates to `ctx.editor.mark_dirty()` |
| Assumption A-02 (line 637) | ✅ | Correctly describes `ctx` as `EditorContext` parameter of `draw()` |
| Unregistered type error case (line 580) | ✅ | Uses `ctx.engine.services.assets()` for `SerializationContext` construction |
| PropertiesPanel descriptions | ✅ | Uses `ctx.editor.world()`, `ctx.editor.command_stack()`, `ctx.editor.mark_dirty()`, `ctx.editor.selection()` throughout |

**Stale reference check**: Zero occurrences of `SerializationContext*` or `ser_ctx` found in `spec.md`. ✅

**EditorContext include check**: The `EditorContext` struct (`src/editor/editor_context.h`) is an editor-layer aggregate with `Editor&` and `EngineContext const&` fields, confirmed by the wiki. The draw signatures are correctly scoped to the editor layer. ✅

**Minor wording fossil** (not blocking): Line 162 still says "If no valid engine context" — this dates from the old nullable `SerializationContext*` design. With `EditorContext& ctx` (a non-nullable reference), the engine context is always valid. The actual condition should be "If no valid `AssetManager`" (consistent with line 580's wording). This does not affect implementability since the intended behavior (read-only text when AssetManager unavailable) is clear.

**Definition of Ready check**: All criteria satisfied. Scope clearly defined (goals/non-goals), dependencies identified (F-00–F-04), edge cases and error conditions described (22 edge cases, 10 error cases), behavior unambiguous and testable (32 ACs), E2E verification defined, interface changes documented, documentation impact listed, technical constraints identified, risks/unknowns surfaced. ✅

## Re-review (loop-back #1) — ACCEPTED

**Verdict**: All 3 previously identified blocking issues (BI-01, BI-02, BI-03) are confirmed **resolved**. No new blocking issues introduced. The spec passes the Definition of Ready check and is ready for implementation-contract authoring.

**Summary of changes verified**:
- **BI-01**: `World::find_entity()` references replaced with `World::entity(EntityId)` throughout the spec (Interface Changes, Assumption A-06, ImGui Layout Details pseudocode).
- **BI-02**: `const SerializationContext* ser_ctx = nullptr` added to `InspectorTypeEditor::draw()`, `TypedInspectorEditor<T>::draw()`, and `InspectorTypeEditorRegistry::draw<T>()`. Fallback behavior documented for both null (read-only) and non-null (editable text input) cases. Error cases updated accordingly.
- **BI-03**: "Differences from north-star UX spec" subsection added under Non-goals (D-01), documenting the rotation-editable deviation with rationale and required wiki updates.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **BI-01: No public API to retrieve an `Entity` handle from an `EntityId`** — Assumption A-06 states that `World::find_entity(EntityId)` exists, but the `World` class (`src/engine/scene/world.h`) has **no such public method**. The `Entity` constructor `Entity(World&, EntityId)` is `private` with `friend class World`. Additionally, `World::get_name(EntityId)` and `World::set_name(EntityId)` are **private** methods. The PropertiesPanel needs to (a) read the entity's current name to populate the `ImGui::InputText` field, and (b) pass an `EntityId` to `RenameEntityCommand`. It can read the transform via the public `World::get_transform(EntityId)`, but cannot construct an `Entity` handle to call `Entity::name()`. This makes AC-16, AC-22, AC-23, and the entity-name-field implementation untestable as specified. **Required**: Either add a public `World::entity(EntityId) -> Entity` method, or promote `World::get_name(EntityId)` / `World::set_name(EntityId)` to public, or document an alternative approach.
  - **RESOLVED (loop-back #1)**: Spec now adds `World::entity(EntityId) -> Entity` as a new public factory method on `World` (Interface Changes, A-06). All `find_entity()` references replaced with `entity()`. The `Entity(World&, EntityId)` constructor remains private. Implementation is straightforward given the existing `friend class World` pattern.

- [x] **BI-02: TypeRegistry fallback path cannot obtain a `SerializationContext`** — The fallback text-input path in `InspectorTypeEditorRegistry::draw<T>()` calls `TypeRegistry::to_string(value, ctx)` and `TypeRegistry::from_string<T>(text, ctx)`, both of which require `const SerializationContext&` (defined in `src/engine/scene/component_registry/serialization_context.h`). However, the `draw()` API signature `draw(std::string_view label, void* value, EditorFlags flags)` does not accept a `SerializationContext`, and `InspectorTypeEditorRegistry::draw<T>(label, value, flags)` likewise has no way to pass one. The `EditorContext` does not expose an `AssetManager` or `SerializationContext`. The spec does not describe how to obtain a `SerializationContext` in the editor, and the existing `EngineContext` may or may not provide access to `AssetManager`. This makes AC-05 and AC-06 (fallback test cases) unimplementable without either: (a) changing the `draw()` signature to accept a context, (b) making `SerializationContext` default-constructible with a null `AssetManager`, or (c) documenting a different fallback approach.
  - **RESOLVED (loop-back #1)**: `const SerializationContext* ser_ctx = nullptr` added to all `draw()` signatures. Fallback behavior fully specified for both null (read-only text, no edit) and non-null (editable `InputText` with `TypeRegistry::to_string`/`from_string`). Red-error state documented. A-02 notes the PropertiesPanel obtains `SerializationContext` via `ctx.engine.services.assets()` (available through the existing `EngineService` → `AssetManager` chain).

- [x] **BI-03: Contradiction with accepted north-star UX spec — rotation editable vs. read-only** — The north-star UX spec (`docs/.specs/sprint-2026-06/editor-ux-design/spec.md`, Inspector section, bullet 2 on Transform) explicitly states: *"In MVP1, rotation fields are read-only (no rotate gizmo)."* The north-star wiki (`docs/wiki/editor/editor-panels.md`, Inspector Property Editors table) also lists `Quat` as *"Euler angles (read-only in MVP1)"*. This F-05 spec makes **rotation editable** via DragFloat Euler fields (G-06, G-02, User Story 3, AC-20, AC-21). While the grill-me in coordination.md decided "Rotation: Editable, Euler angles in degrees", this deviation from the north-star is **not acknowledged** in the spec's "Non-goals" or "Contradictions" section. At a minimum, the spec should note that this overrides the north-star read-only constraint for rotation, and the north-star documents must be updated accordingly. (This was an intentional design decision, but the spec must document the deviation explicitly.)
  - **RESOLVED (loop-back #1)**: "Differences from north-star UX spec" subsection added under Non-goals (D-01). Documents the rotation-editable deviation, rationale (grill-me decision), and notes that north-star docs must be updated.

## Warnings

Non-blocking concerns for awareness:

- **W-01: No existing headless ImGui snapshot-test infrastructure** — AC-15, AC-16, AC-17, AC-18, AC-24 reference "Snapshot test (headless): verify ImGui draw output contains …" and "verify `ImGui::BeginDisabled` / `EndDisabled`". No existing test framework or helper for ImGui snapshot testing (e.g., inspecting ImGui draw commands or rendering to an offscreen buffer) was found in the project. The implementation contract must either create this infrastructure or define an alternative verification approach.

- **W-02: Entity name field revert-on-empty behavior is incompletely specified** — The spec says "If the new name is empty: the input reverts to the previous name (no command pushed)". It does not specify whether this happens on **Enter**, on **focus loss**, or both. It also says "The field label is 'Name' (or no label — implementation choice, documented as a decision for the contract)" — leaving this open-ended is acceptable but should be pinned down in the implementation contract to avoid ambiguity.

- **W-03: `SerializationContext` dependency for built-in TypeRegistry types** — Assumption A-02 assumes that `TypeRegistry::to_string<T>()` and `TypeRegistry::from_string<T>()` exist for all 8 built-in editor types. However, the `TypeRegistry`'s built-in registration (`register_builtin_types()`) may not include `Vec2` — ADR-028 lists built-in types as `float, int32_t, bool, std::string, Vec3, Vec4, Quat, shared_ptr<Model>`. `Vec2` has a dedicated editor but may not be in TypeRegistry for fallback. This only matters if fallback is invoked for Vec2, which is unlikely but should be verified at implementation time.

- **W-04: `EditorFlags` default min/max values use `std::numeric_limits<float>::max()` — no negative constraint sentinel** — `EditorFlags{}.min_value` defaults to `-std::numeric_limits<float>::max()` which is approximately `-3.4e38`. This is a valid approach, but differs from the common "no clamp" sentinel of `-inf` or a dedicated flag. This works for the stated use cases (position unbounded, rotation wrapping-handled) but could cause subtle issues if a future type uses a narrower range that includes -FLT_MAX. Documented in Assumption A-14, so this is acceptable, but worth awareness.

- **W-05: `ImGui::BeginDisabled`/`EndDisabled` usage requires ImGui ≥ 1.91** — This API was only added in Dear ImGui 1.91 (docking branch). The project uses `v1.91.8-docking`, so this should be available, but it must be confirmed during implementation. The spec does not note this version requirement.

- **W-06: `SetTransformCommand` is mentioned conceptually but does not yet exist** — This is fine (NG-10 explicitly defers the command class to the implementation contract). However, the spec's AC-19 and AC-20 imply that a Command is executed on edit, and the testing framework must be able to verify this. This will require mocking or creating `SetTransformCommand` or equivalent in the test infrastructure.

## Required changes (all resolved in loop-back #1)

The following required changes from the initial review have been verified as implemented in the updated spec:

1. ✅ **Add a public `World::entity(EntityId) -> Entity` method** — Done. `World::entity(EntityId)` added as a new public factory method in Interface Changes and A-06. All `find_entity()` references replaced with `entity()`.

2. ✅ **Resolve the `SerializationContext` gap for the TypeRegistry fallback path** — Done. `const SerializationContext* ser_ctx = nullptr` added to all `draw()` signatures. Fallback behavior fully specified for both null (read-only) and non-null (editable) cases. A-02 provides the `ctx.engine.services.assets()` chain.

3. ✅ **Document the north-star contradiction** — Done. "Differences from north-star UX spec" subsection added under Non-goals (D-01). Documents the rotation-editable deviation with rationale.

## Suggested improvements

Optional ideas (not required):

- Consider adding a `SerializationContext` default/null state (e.g., a static `SerializationContext::empty()` that wraps a null `AssetManager` or a no-op manager) to unblock the editor fallback path without requiring a real `AssetManager`.

- For entity name reading, an alternative is to store the last-known name in the PropertiesPanel's per-entity state, updated whenever the selection changes. This avoids the need for a `World::entity(EntityId)` public API. However, it adds complexity and the name can go stale.

- The AC set is already strong (32 criteria). Consider adding an AC for gimbal-lock behavior (currently only in Edge Cases, not in Acceptance Criteria) — e.g., "AC-33: Editing rotation near gimbal lock (pitch ≈ ±90°) does not crash and produces a valid quaternion".
