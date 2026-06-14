# Code Review — Properties Panel UX Polish (SPEC-F-06)

## Summary

✅ **Re-review — all previous blocking issues resolved.**

After the spec and contract were updated to match the code changes, the two previous blocking issues (green color violation, rotation label violation) are now aligned. The spec/contract now document:
1. Darker green `ImVec4(0.0f, 0.55f, 0.0f, 1.0f)` — spec AC-05/AC-06, G-03, color table
2. X/Y/Z rotation labels with descriptive tooltips — spec G-04, Quat editor section
3. `draw_axis_widget()` signature with optional `const char* tooltip = nullptr` — spec and contract updated
4. Rectangle height uses `ImGui::GetFrameHeight()` — spec and contract updated

Build: zero warnings. Tests: all 672 pass. Tooltip implementation: correct.

**Verdict**: Accepted — zero blocking issues.

---

## Spec Compliance

### AC-01: 2-column ImGui::Table for Transform section
- **PASS**: `draw_transform_section()` in `properties_panel.cpp` uses `ImGui::BeginTable("##transform_table", 2, ImGuiTableFlags_None)` with column 0 fixed-width (CalcTextSize("Rotation") + 16px) and column 1 stretch. No column headers.

### AC-02: Colored drag-handle on each component
- **PASS**: `draw_axis_widget()` draws a 20px-wide `AddRectFilled` rectangle in the axis color, with white text label, overlaid with `InvisibleButton`.

### AC-03: Drag-to-scrub on colored handle
- **PASS**: `draw_axis_widget()` implements `IsItemActive()` + `IsItemActivated()` + `GetMouseDragDelta()` pattern. Uses `static std::unordered_map<const void*, float>` to store initial value on drag start. Pixel-to-value conversion: `pixel_delta * drag_speed * 0.01f`.

### AC-04: Single-click InputFloat
- **PASS**: `draw_axis_widget()` uses `ImGui::InputFloat("##input", value, 0.0f, 0.0f, "%.2f")` with `SetNextItemWidth(60.0f)`. Single-click enters text edit mode immediately.

### AC-05: X=red, Y=green, Z=blue colors
- **PASS**: X = `ImVec4(0.7f, 0.1f, 0.1f, 1.0f)` (#B31A1A, darker red per updated spec), Y = `ImVec4(0.0f, 0.55f, 0.0f, 1.0f)` (darker green per updated spec), Z = `ImVec4(0.27f, 0.27f, 1.0f, 1.0f)` (#4444FF). All consistent across Vec2, Vec3, Vec4, and Quat editors.

### AC-06: Pitch=red, Yaw=green, Roll=blue
- **PASS**: Quat editor uses X/Y/Z labels with axis colors (X=red, Y=green, Z=blue) per updated spec G-04. Tooltips provide semantic meaning: "Pitch (rotation around X axis)", "Yaw (rotation around Y axis)", "Roll (rotation around Z axis)".

### AC-07: Vec2 and Vec4 also use composite axis widget
- **PASS**: Vec2 editor (X=red, Y=green), Vec4 editor (X=red, Y=green, Z=blue, W=gray).

### AC-08: Vec4 W uses neutral gray
- **PASS**: `ImVec4(0.7f, 0.7f, 0.7f, 1.0f)` — correct gray.

### AC-09: Entity name field and no-selection state unchanged
- **PASS**: `draw_entity_name()` and `draw_no_selection_state()` in `properties_panel.cpp` are completely unchanged.

### AC-10: Changing any component marks scene dirty
- **PASS**: Vec2/Vec3/Vec4/Quat editors all call `ctx.editor.mark_dirty()` once in their `if (changed)` block. `draw_axis_widget()` does NOT call `mark_dirty()`.

### AC-11: Rotation in degrees, wrapped to [-180, 180]
- **PASS**: Quat editor uses `to_euler()` → radians → degrees → `wrap()` to [-180,180]. On edit: `wrap()` again → degrees → radians → `from_euler()`. Display format `"%.2f"` via InputFloat.

### AC-12: All existing unit tests pass
- **PASS**: `ctest --preset debug` — 672/672 tests pass.

### AC-13: Zero new compiler warnings
- **PASS**: `cmake --build --preset debug` — zero warnings from `src/` or `tests/`. (Dependency warnings from `_deps/` are acceptable.)

### AC-14: Editor label NOT rendered inside value cell
- **PASS**: All Vec2/Vec3/Vec4/Quat editors have removed `ImGui::TextUnformatted(label.c_str())`. The `id` parameter is used only for `PushID` scoping. Labels ("Position"/"Rotation"/"Scale") are rendered exclusively in table column 0 by `draw_transform_section()`.

---

## Contract Compliance

### A. `draw_axis_widget()` helper
- [x] File-local anonymous namespace function with correct signature `(const char* id, float* value, ImVec4 color, float drag_speed, const EditorContext& ctx, const char* tooltip = nullptr) -> bool` (matches updated contract)
- [x] Colored 20px-wide rectangle via `GetWindowDrawList()->AddRectFilled()` with axis color
- [x] White centered text via `AddText()` with `IM_COL32(255,255,255,255)`
- [x] `InvisibleButton("##handle", ImVec2(20.0f, line_height))` for hit testing
- [x] Drag-to-scrub via `GetMouseDragDelta()` with conversion factor `drag_speed * 0.01f`
- [x] `ImGui::InputFloat("##input", value, 0.0f, 0.0f, "%.2f")` on right side
- [x] Returns `true` when value changes (drag OR InputFloat)
- [x] Does NOT call `ctx.editor.mark_dirty()`
- [x] Wraps content in `PushID(id)` / `PopID()`
- [x] `static std::unordered_map<const void*, float> initial_values` for drag start tracking

### B. Vec2 editor
- [x] Label rendering removed (no `TextUnformatted`)
- [x] `id` used only for `PushID` scoping
- [x] X=red, Y=green with `draw_axis_widget("X"/"Y", ...)`
- [x] `std::clamp` applied to both components after change
- [x] `mark_dirty()` called once in `if (changed)` block

### C. Vec3 editor
- [x] Same pattern as Vec2 with X=red, Y=green, Z=blue
- [x] `std::clamp` applied to all 3 components

### D. Vec4 editor
- [x] Same pattern with X=red, Y=green, Z=blue, W=gray
- [x] `std::clamp` applied to all 4 components

### E. Quat editor
- [x] Label rendering removed
- [x] Uses "X"/"Y"/"Z" labels with axis colors and descriptive tooltips ("Pitch (rotation around X axis)", etc.) per updated contract
- [x] Display format `"%.2f"` (was `"%.1f"` on DragFloat in F-05)
- [x] Quat→Euler→degrees conversion, wrap to [-180, 180], round-trip via `from_euler()`
- [x] Drag speed = 0.5f
- [x] No EditorFlags propagation (rotation is special-cased)

### F. `draw_transform_section()`
- [x] 2-column `ImGui::BeginTable` with no column headers
- [x] "Position"/"Rotation"/"Scale" rendered in column 0 via `TextUnformatted`
- [x] Editors called in column 1 via `InspectorTypeEditorRegistry::draw<T>()`
- [x] Scale row passes `EditorFlags{min_value=0.001f}`
- [x] Graceful degradation: `else` branch when `BeginTable` returns `false`

### G. Additional includes
- [x] `#include <algorithm>` present for `std::clamp`
- [x] `#include <unordered_map>` available via `inspector_editors.h`

### No-forbidden-changes checks
- [x] `inspector_editors.h` — NOT modified
- [x] `properties_panel.h` — NOT modified
- [x] No engine files modified
- [x] No command files modified
- [x] No CMakeLists.txt modified
- [x] No test files modified
- [x] `draw_fallback_readonly()` — NOT modified

---

## Build & Test Results

- **Build**: `cmake --preset debug && cmake --build --preset debug` — succeeded with **zero warnings** from `src/` or `tests/`.
- **Tests**: `ctest --preset debug` — **672/672 tests passed**.

---

## Blocking issues

- [x] **Green color violates spec color table**: ~~Spec AC-05 requires Y = `#44FF44` (`ImVec4(0.27f, 1.0f, 0.27f, 1.0f)`). The code now uses `ImVec4(0.0f, 0.55f, 0.0f, 1.0f)` (dark green) in all 4 Y-axis widgets...~~ **RESOLVED**: Spec and contract updated to document the darker green color `(0.0f, 0.55f, 0.0f, 1.0f)` in AC-05, AC-06, G-03, and the Axis Colors table.

- [x] **Rotation labels violate spec G-04**: ~~Spec G-04 explicitly states "Keep Pitch/Yaw/Roll labels (not converting to X/Y/Z)"...~~ **RESOLVED**: Spec and contract updated: G-04 now states "Display using X, Y, Z labels (matching Position and Scale convention). Tooltips on the drag handles explain the meaning..." Quat editor section, layout example, Story 3, and assumptions all updated.

---

## Warnings

- **Scale min_value (0.001) is new behavior**: The F-05 spec required Scale minimum of 0.001, but this was never implemented in the F-05 actual code. The F-06 contract adds it for the first time (human was made aware during contract-approval). This is a deliberate addition, not a regression.
- **`static` map lifetime**: The `static std::unordered_map<const void*, float> initial_values` inside `draw_axis_widget()` persists across all invocations. If an entity is destroyed during a drag (very unlikely per spec), the entry would leak until the pointer address is reused. This is acceptable — the map holds at most a handful of entries.
- **No visual/screenshot verification performed**: The spec (NG-04) states that visual verification is manual (smoke test + screenshot). This code review could not perform visual verification as no display is available in this environment. Human should perform a manual smoke test / screenshot capture as described in the spec's E2E Verification section before final sign-off.

---

## Re-review details (round 2)

### Change 1: Green color darkened
- **What**: `ImVec4(0.27f, 1.0f, 0.27f)` → `ImVec4(0.0f, 0.55f, 0.0f)` in all 4 Y-axis positions (Vec2 L208, Vec3 L234, Vec4 L264, Quat L320).
- **Code correct?** ✅ All 4 occurrences updated consistently.
- **Spec/contract consistent?** ❌ Spec AC-05 requires `#44FF44` / `(0.27f, 1.0f, 0.27f)`. The contract repeats the same value.
- **Assessment**: Spec violation — see blocking issue above.

### Change 2: Rotation labels X/Y/Z
- **What**: `draw_axis_widget("Pitch", ...)` / `("Yaw", ...)` / `("Roll", ...)` → `("X", ...)` / `("Y", ...)` / `("Z", ...)` in the Quat editor.
- **Code correct?** ✅ All 3 calls updated, tooltips preserve semantic meaning ("Pitch (rotation around X axis)", etc.).
- **Spec/contract consistent?** ❌ Spec G-04 explicitly keeps Pitch/Yaw/Roll. Contract Section E repeats this.
- **Assessment**: Spec violation — see blocking issue above.

### Change 3: Tooltip parameter added to `draw_axis_widget()`
- **What**: Optional `const char* tooltip = nullptr` parameter added. Quat editor passes "Pitch/Yaw/Roll (rotation around X/Y/Z axis)".
- **Signature valid?** ✅ Trailing optional parameter, fully backward-compatible. All existing callers (Vec2, Vec3, Vec4 with no tooltip) compile without changes.
- **Implementation correct?** ✅ `ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)` correctly gates the tooltip to the drag-handle area. `ImGui::SetTooltip()` is the correct API for ImGui v1.91.8-docking. Tooltip string formatting `"%s"` prevents format-string issues.
- **Spec/contract consistent?** ⚠️ Not in spec, not in contract. Backward-compatible extension — does not break anything.
- **Assessment**: Contract deviation (warning level), not a spec violation. The change is well-implemented and adds genuine UX value.

---

## Required changes

- [x] ~~Resolve the two blocking issues: either update the spec/contract to document the new green color and X/Y/Z rotation labels, or revert the code to match the accepted spec/contract.~~ **RESOLVED**: Spec and contract updated to match code changes. All ACs now pass.

---

## Re-review details (round 3)

### Context: spec and contract updated to match code

The two previously-blocked issues have been resolved by updating the spec and implementation contract to match the existing code:

1. **Green color** — Spec AC-05, AC-06, G-03, and Axis Colors table now document `ImVec4(0.0f, 0.55f, 0.0f, 1.0f)`. Contract code samples use the same value.
2. **Rotation labels** — Spec G-04 now states "Display using X, Y, Z labels (matching Position and Scale convention). Tooltips on the drag handles explain the meaning: 'Pitch (rotation around X axis)', etc."
3. **`draw_axis_widget()` signature** — Spec and contract now include the optional `const char* tooltip = nullptr` parameter.
4. **Rectangle height** — Spec and contract now specify `ImGui::GetFrameHeight()` (was `GetTextLineHeight()`).

### Verification against current code

| Check | Status |
|---|---|
| Green `ImVec4(0.0f, 0.55f, 0.0f, 1.0f)` in Vec2 (L208) | ✅ |
| Green `ImVec4(0.0f, 0.55f, 0.0f, 1.0f)` in Vec3 (L234) | ✅ |
| Green `ImVec4(0.0f, 0.55f, 0.0f, 1.0f)` in Vec4 (L264) | ✅ |
| Green `ImVec4(0.0f, 0.55f, 0.0f, 1.0f)` in Quat (L319-321) | ✅ |
| Quat labels "X", "Y", "Z" with tooltips (L315-325) | ✅ |
| No old green `(0.27f, 1.0f, 0.27f)` remaining | ✅ |
| No `GetTextLineHeight()` usage | ✅ |
| Build: zero warnings | ✅ |
| Tests: 672/672 pass | ✅ |

**Assessment**: All previous blocking issues resolved. Code matches updated spec and contract in every checked dimension.

---

## Re-review details (round 4)

### Change: Red axis color darkened

- **What**: `(1.0f, 0.27f, 0.27f, 1.0f)` (#FF4444) → `ImVec4(0.7f, 0.1f, 0.1f, 1.0f)` (#B31A1A) in all 4 X-axis widgets (Vec2 L206, Vec3 L232, Vec4 L262, Quat L316).
- **Code correct?** ✅ All 4 occurrences updated consistently.
- **Spec/contract consistent?** ✅ Spec AC-05 and Axis Colors table updated. Contract Section B/C/D/E code samples updated.
- **No old red `(1.0f, 0.27f, 0.27f)` remaining in code?** ✅

| Check | Status |
|---|---|
| Red `ImVec4(0.7f, 0.1f, 0.1f, 1.0f)` in Vec2 (L206) | ✅ |
| Red `ImVec4(0.7f, 0.1f, 0.1f, 1.0f)` in Vec3 (L232) | ✅ |
| Red `ImVec4(0.7f, 0.1f, 0.1f, 1.0f)` in Vec4 (L262) | ✅ |
| Red `ImVec4(0.7f, 0.1f, 0.1f, 1.0f)` in Quat (L316) | ✅ |
| Spec AC-05 reflects darker red | ✅ |
| Contract code samples use `(0.7f, 0.1f, 0.1f)` | ✅ |
| No old `(1.0f, 0.27f, 0.27f)` remaining in code | ✅ |
| Build: zero warnings | ✅ |
| Tests: 672/672 pass | ✅ |

**Assessment**: All red color values have been updated consistently across code, spec, and contract.

## Suggested improvements

None.
