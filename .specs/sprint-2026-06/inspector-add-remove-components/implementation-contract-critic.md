# Implementation Contract Review — Inspector — Add/Remove Components — Re-review (Index-Based)

## Blocking issues

Items that must be resolved before the artifact can be accepted.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

None — no blocking issues found.

## Warnings

Non-blocking concerns for awareness:

### Pre-existing (from previous review)

- **`SerializationContext` namespace qualification**: The contract still uses unqualified `SerializationContext` in `RemoveComponentCommand::execute()` and `undo()` code snippets (lines 198, 208). The existing codebase consistently uses `buddd::engine::SerializationContext` (see `set_component_property_command.h` line 103, `properties_panel.cpp` line 350). The Code Agent must use the fully qualified name to avoid a compile error. Not fixed since previous review.
- **`Result` checking for `registry.create()`**: The contract specifies "if error, log ERROR... return" without showing the exact `Result` API check. Acceptable for a contract — the Code Agent must look at existing `Result` usage patterns.
- **`YAML::Clone` usage**: Correct for deep-copying. The Code Agent must ensure `serialized_state_` is stored before local `ser_ctx` goes out of scope (it already is).
- **`const_cast` on `ComponentInfoBase*` for `create()`**: Established "SceneSaver pattern" throughout the codebase. Inherently fragile but pre-existing.
- **UI ACs deferred to manual smoke testing**: AC-017/018/020/021/023/025 labeled as "Snapshot test" in spec but deferred to manual verification. No automated safety net yet.

### New (this review)

- **Contract's "spec deviation" notes are misleading**: The contract says AC-005 (prevent duplicates) and AC-022 (filter present types) are "not implemented" and labeled as intentional deviations. However, the spec was UPDATED to match the index-based approach — AC-005 now reads *"allows adding a component type even if the entity already has that type (duplicates permitted)"* and AC-022 now reads *"lists all types from ComponentRegistry::all_types() (no filtering by already-present)"*. The contract's behavior aligns with the updated spec, so these are not deviations. The misleading notes should be removed or reworded. **(Low severity — behavior is correct, framing is wrong.)**
- **Spec has contradictory edge case entries**: The spec's edge case table contains two contradictory entries:
  - Line 337: *"Entity already has all registered component types → duplicates are allowed. The user can add another instance of any type."* ✅ (matches AC-005)
  - Line 342: *"Add Component on entity that already has that component type → No-op: command detects duplicate, logs warning, skips creation."* ❌ (contradicts AC-005)
  - The contract correctly follows AC-005 (duplicates allowed), but this spec contradiction should be resolved. **(Spec issue, not contract issue — the contract is correct.)**
- **Stale edge case "eligible" filtering**: Spec edge case line 348: *"Empty filter field → All eligible (not already present) component types are shown"*. This references the old pre-index filtering behavior. With the index-based approach, ALL types are shown (not "eligible"). The contract correctly shows all types. **(Spec issue — contract is correct.)**

## Required changes

Concrete, actionable changes requested:

None — all ACs are covered, all non-goals are respected, and the implementation detail is complete and consistent with the updated spec.

## Suggested improvements

Optional ideas (not required):

- Consider updating the contract's "NOTE on spec deviations" section to accurately reflect that AC-005 and AC-022 have been updated in the spec and the contract aligns with them (rather than framing them as deviations).
- Consider adding a compile-time `static_assert` or unit test that verifies `remove_component_at()` and `insert_component_raw_at()` are callable with the expected signatures.
- The `draw_component_sections()` loop modifies the existing `ImGui::CollapsingHeader` call to pass a `flags` variable instead of `ImGuiTreeNodeFlags_None`. The Code Agent should verify that existing component sections still render identically when `flags` is `ImGuiTreeNodeFlags_None` (i.e., no accidental DefaultOpen applied to non-new sections).
