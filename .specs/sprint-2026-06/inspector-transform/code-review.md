# Code Review — F-05 Inspector — Transform

## Summary

The implementation covers the full scope of the accepted spec and contract: `Quat::to_euler()`, `World::entity(EntityId)`, `EditorSelection::primary()` tracking, the `InspectorTypeEditor` registry with 8 built-in editors, and `PropertiesPanel` with entity name field + Transform section (Position/Rotation/Scale all editable). All 672 tests pass (22582 assertions) with zero new warnings from our code. Two known deviations from the spec (editable TypeRegistry fallback) follow the contract's explicit design choice and were accepted during contract review.

## Blocking issues

No blocking issues found.

None of the issues listed below prevent acceptance: the implementation faithfully follows the accepted contract, all tests pass, and warnings from spec/contract deviations were already identified, documented, and accepted in earlier review stages.

## Warnings

Non-blocking concerns for awareness:

- **W-01: Editable TypeRegistry fallback not implemented (spec AC-05, AC-06)** — The spec §154-162 requires editable text input via `TypeRegistry::to_string()/from_string()` when no editor is registered. The implementation uses a read-only `draw_fallback_readonly()` free function that shows "(no editor for type %s)" in disabled text. This matches the accepted contract's explicit `draw<T>()` template code ("fallback shows read-only text"). The deviation was documented (yaml-cpp include paths being private to the engine library) and accepted during contract review (W-01, W-07). Not blocking, but a future feature could add bridge support.

- **W-02: `draw_fallback_readonly` is a free function, not a static method** — The contract specified `draw_fallback_readonly` as a `private static` method of `InspectorTypeEditorRegistry`, but the implementation declares it as a free function (forward-declared in `.h`, defined in `.cpp`). Functionally equivalent and correctly called from the template. Minor structural deviation from the contract.

- **W-03: `draw_fallback_editable` removed entirely** — The contract declared a `draw_fallback_editable` private static method with implementation code, but this was removed from the actual code as dead code (never called — `draw<T>()` only calls `draw_fallback_readonly()`). Consistent with the read-only fallback design choice.

- **W-04: PropertiesPanel snapshot tests deferred** — AC-15..18, AC-24 require ImGui snapshot tests that cannot run in headless mode. The implementer deferred these to manual smoke testing and substituted compile-time signature checks. This was documented as a known limitation in the contract (Non-goals: "No headless ImGui snapshot-test infrastructure creation"). Not blocking, but the wiki-agent should verify these manual tests pass.

- **W-05: Entity name `ImGui::IsItemActive()` before `InputText`** — `properties_panel.cpp` line 84 calls `ImGui::IsItemActive()` before `ImGui::InputText()` is called, which is a non-standard pattern. The intent is to avoid overwriting the user's in-progress edit with an external name change. Verified to work correctly because ImGui preserves `LastItemData` across frame boundaries. Previously flagged as W-03 in contract-critic. Harmless, but fragile.

- **W-06: No test coverage for AC-19, AC-20, AC-21, AC-22, AC-23, AC-25, AC-26, AC-28** — Integration and behavioral tests for transform editing, rotation wrapping, rename via panel, empty-rename protection, and clamp-bound editors are deferred. The contract acknowledged this (W-01). Compile-time and data-path tests substituted where possible.

## Required changes

None.

The implementation correctly follows the accepted contract. All deviations from the spec (editable TypeRegistry fallback) were intentional decisions documented and accepted during contract review. No new architectural decisions were introduced. All 672 tests pass with zero new warnings.

## Suggested improvements

Optional ideas (not required):

- Consider restoring `draw_fallback_editable` from the contract and adding a type-erased bridge to enable editable text fallback for unregistered types. This would satisfy spec AC-05/AC-06 and close W-01.
- Consider adding a `static_assert` or concept check in `InspectorTypeEditorRegistry::draw<T>()` to ensure `T` is not a pointer or reference type, preventing common misuse.
- Once headless ImGui snapshot infrastructure is available, add snapshot tests for `PropertiesPanel::draw_ui()` to cover AC-15..18, AC-24.
