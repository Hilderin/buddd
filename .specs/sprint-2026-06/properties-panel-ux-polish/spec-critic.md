# Spec Review — Properties Panel UX Polish (SPEC-F-06)

## Blocking issues

Items that must be resolved before the artifact can be accepted.

*No blocking issues found.*

## Warnings

Non-blocking concerns for awareness:

- **Scale minimum value needs explicit carry-forward**: F-05 specifies a minimum value of `0.001` for Scale (prevent negative/zero scale). F-06 NG-08 says "No changes to Scale editing behavior" but does not explicitly restate this constraint. The implementation contract should confirm that the `0.001` minimum is preserved for the Scale Vec3 editor's flags.
- **Vec2/Vec3/Vec4 composite widgets will be used by future non-spatial properties**: The hardcoded axis colors (X=red, Y=green, Z=blue, W=gray) are semantically meaningful only for spatial/transform data. Future component editors (e.g., Light color, Velocity) that reuse `InspectorTypeEditorRegistry::draw<Vec3>()` will display meaningless axis-colored drag handles. NG-10 acknowledges "No per-component color customization" — this is a known limitation that should be revisited when component sections are added.
- **Label parameter is now ignored by Vec/Quat editors**: The editors no longer render the label text, but still receive it for API compatibility. The label is still useful for `ImGui::PushID()` scoping (the existing code uses `label.c_str()` for ID namespacing). The implementation must ensure the label is still consumed for ImGui ID isolation even though it is not displayed.
- **Display precision format change**: F-05's Quat editor uses `"Pitch: %.1f"` (one decimal). F-06 changes to `"%.2f"` (two decimals) on InputFloat. The spec-author's warning from the previous cycle ("verify this format change does not affect existing tests") is still relevant — no existing tests check display format directly, but the change from 1 to 2 decimal places may affect screenshot-based or snapshot-based verification.
- **Drag-handle sensitivity is underspecified**: The spec says drag-handle sensitivity should be "comparable to the existing DragFloat speeds (0.1 for position/scale, 0.5 for rotation)" but the conversion from `GetMouseDragDelta()` pixels to value delta is a free parameter not specified. The implementation contract should define this conversion factor explicitly.

## Required changes

Concrete, actionable changes requested:

- None — the spec satisfies all Definition of Ready criteria.

## Suggested improvements

Optional ideas (not required):

- Consider adding an explicit API signature for the composite axis widget helper function to make implementation clearer (e.g., `draw_axis_widget(const char* label, float* value, ImVec4 color, float speed, EditorContext& ctx) -> bool`).
- The "Empty string entry on InputFloat" edge case could note that `ImGui::InputFloat` natively rejects non-numeric input, but setting a value to "" and defocusing reverts to the last valid numeric value (not the previous semantic value). This is consistent with standard ImGui behavior.
- The Rotation section could clarify whether the `"%.2f"` format applies to all three Pitch/Yaw/Roll fields consistently (implied, but not explicit).
