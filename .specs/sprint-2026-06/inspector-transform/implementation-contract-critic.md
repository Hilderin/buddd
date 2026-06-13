# Implementation Contract Review — IMPL-F-05 Inspector — Transform

## Re-review (loop-back #2) — ACCEPTED ✅

All 4 previously identified blocking issues (B-01 through B-04) are confirmed resolved. No new blocking issues found.

| Issue | Status | Verification |
|-------|--------|-------------|
| **B-01** Fallback rendering unwired | **✅ Resolved** | `draw<T>()` template now calls `draw_fallback_readonly()` (ser_ctx null → disabled text) and `draw_fallback_editable()` (ser_ctx non-null → InputText with typed TypeRegistry lambdas). Private static helpers declared in header (lines 271–279), implemented in `.cpp` (lines 382–433). |
| **B-02** `set_selection(span)` uses unordered_set::begin | **✅ Resolved** | Contract line 166: `primary_id_ = ids[0];` — first element of input span, not arbitrary hash order. |
| **B-03** Missing `#include <limits>` | **✅ Resolved** | Header line 201: `#include <limits>` present. |
| **B-04** Missing math includes | **✅ Resolved** | `.cpp` lines 345–348: `"math/quat.h"`, `"math/vec2.h"`, `"math/vec3.h"`, `"math/vec4.h"`. Also `<glm/gtc/matrix_transform.hpp>` (line 352) for `glm::degrees`/`glm::radians` (backed up by `glm/glm.hpp` → `trigonometric.hpp`). |

### Previously flagged warnings (carried forward)

- **W-01**: Test coverage gaps for AC-05/06/19/20/21/22/23/25/26/28 — partially addressed (AC-05/06 fallback tests now feasible). AC-19/25 direct-mutation deviation documented in non-goals.
- **W-02**: `const std::string&` vs spec's `std::string_view` — still `std::string`, unresolved.
- **W-03**: `IsItemActive()` before `InputText` in `draw_entity_name()` — unchanged.
- **W-04**: AC-25 (rapid edits, one Command) documented as inapplicable (direct mutation) in contract non-goals. ✓
- **W-05**: AC-30 code-review only — unchanged.
- **W-06**: `engine_context.h` include with `(needed?)` comment in header — still present, not resolved.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **B-01: Fallback rendering path is UNIMPLEMENTED in `InspectorTypeEditorRegistry::draw<T>()`**
  The spec (AC-05, AC-06) requires `InspectorTypeEditorRegistry::draw<T>()` to render fallback text when no editor is registered for type T:
  - If `ser_ctx` is null: render read-only text
  - If `ser_ctx` is non-null: render editable `ImGui::InputText` with `TypeRegistry::to_string`/`from_string`
  
  The contract's template `draw<T>()` (line 289-305 of the header) returns `false` unconditionally when no editor is found, without rendering any ImGui widget. A `draw_fallback_text_input()` helper exists in the `.cpp` but is **never wired into the template** — it is a standalone function with no caller.
  
  The contract's comment states that fallback is "the caller's responsibility" and requires manual dispatch via `get(type_index)`. This contradicts the spec's AC-05 and AC-06, which explicitly require `draw<T>()` itself to handle fallback. The spec was intentionally written this way (it was reviewed in spec-critic's BI-02 resolution).
  
  **Impact**: AC-05 and AC-06 cannot be verified as specified. The registry is not reusable for custom types without the caller reimplementing fallback logic.
  
  **Required action**: Either (a) move the `draw<T>()` template implementation to the `.cpp` with explicit instantiations and have it call `draw_fallback_text_input()`, (b) make the template call a non-template function declared in the header and defined in the `.cpp` that performs the ImGui fallback rendering, or (c) add the ImGui fallback rendering directly in the template (requires `#include <imgui.h>` in the header, or restructure the approach).

- [x] **B-02: `EditorSelection::set_selection(span)` sets `primary_id_` from unordered_set, NOT from the input span**
  The spec requires: "`set_selection(ids)`: If the span is non-empty, sets `primary_id_` to the **first element of the span**."
  
  The contract at Step C (`set_selection`) uses:
  ```cpp
  primary_id_ = *current_.selected_.begin();  // first element of unordered_set
  ```
  
  `std::unordered_set::begin()` returns an iterator to an **unspecified/arbitrary** element, not necessarily the first element of the input span `ids`. The contract should use `ids[0]` (or `ids.front()`) instead.
  
  **Impact**: AC-14 may exhibit flaky behavior — `primary()` could return a different entity than the first element of the input span, depending on hash ordering. In practice the unordered_set order is deterministic for a given set of inputs, but the behavior may not match user expectations (the "last-selected" entity should be predictable).

- [x] **B-03: Missing `#include <limits>` in `inspector_editors.h`**
  `EditorFlags` uses `std::numeric_limits<float>::max()` in member initializers (lines 211-212 of the contract) but `<limits>` is not listed in the includes. This may fail to compile on some toolchains.

- [x] **B-04: Missing includes for math types in `inspector_editors.cpp`**
  The `.cpp` file uses `math::Vec2`, `math::Vec3`, `math::Vec4`, `math::Quat`, `glm::degrees()`, and `glm::radians()` but does **not** include `"math/quat.h"`, `"math/vec2.h"`, `"math/vec3.h"`, `"math/vec4.h"`, or `<glm/gtc/matrix_transform.hpp>` (which provides `glm::degrees`/`glm::radians`). The includes listed in the contract are:
  ```cpp
  #include <glm/glm.hpp>
  #include <glm/gtc/type_ptr.hpp>
  ```
  Neither of these guarantee `glm::degrees()` or `glm::radians()`. The math type headers are not included at all (only transitively possible through `type_registry.h` → ... → `quat.h`, which is fragile). This must be fixed with explicit includes.

## Warnings

Non-blocking concerns for awareness:

- **W-01: Missing test coverage for several ACs**
  | AC | Description | Status |
  |----|-------------|--------|
  | AC-05 | Fallback text input for unregistered types | **Not testable** (blocked by B-01) |
  | AC-06 | Fallback red-text error state | **Not testable** (blocked by B-01) |
  | AC-19 | Position edit pushes Command | Contract uses direct mutation (no Command) — no test |
  | AC-20 | Rotation edit changes Quat | No explicit round-trip integration test |
  | AC-21 | Angle wrapping [-180, 180] | No explicit wrapping test (only gimbal lock) |
  | AC-22 | Rename via PropertiesPanel | No rename simulation test |
  | AC-23 | Empty rename reverts | No rename simulation test |
  | AC-25 | Rapid edits push one Command | Direct mutation approach makes this not applicable — should be documented |
  | AC-26 | Editing destroyed entity no-op | No test |
  | AC-28 | EditorFlags clamp tested | No test for min/max clamping |
  
  The contract should explicitly map AC-19, AC-20, AC-21, AC-25, AC-26, AC-28 to tests or document why they are not tested.

- **W-02: `draw()` API uses `const std::string&` instead of spec's `std::string_view`**
  The spec defines `draw(std::string_view label, ...)` but the contract uses `draw(const std::string& label, ...)`. This is a minor efficiency concern (forces string construction for literal labels) and a spec deviation. Should be aligned with the spec unless there's a good reason (e.g., ImGui API compatibility — ImGui uses `const char*`).

- **W-03: `ImGui::IsItemActive()` called before `InputText` in `draw_entity_name()`**
  In `properties_panel.cpp` (contract line 690), `ImGui::IsItemActive()` is called **before** `ImGui::InputText()` to decide whether to sync the rename buffer with external name changes. This is an unconventional pattern — `IsItemActive()` typically refers to the last-submitted widget, which at this point in the frame is an unrelated widget (or nothing). The standard pattern is to check `IsItemActive()` after the widget call using a persistent ID. While this may work due to ImGui's item ID retention, it is fragile and should follow the established scene panel pattern (`scene_panel.cpp` lines 68-86) where the rename is either active or not, driven by explicit `start_rename`/`confirm_rename` state.

- **W-04: AC-25 (rapid edits push one Command) is not addressed**
  The contract's direct-mutation approach means no Command is pushed for transform edits, making AC-25's requirement (one Command per DragFloat end-drag) inapplicable. This should be explicitly called out in the contract's non-goals or edge cases, with a note that the AC is satisfied differently (direct mutation is inherently single-assignment per frame, and Idempotent `mark_dirty()` prevents issues).

- **W-05: No test for AC-30 (code review: `RenameEntityCommand` usage)**
  The contract specifies AC-30 as "Code review: verify include and usage." The done criteria correctly list this as a code-review check. However, the contract could add a compile-time test (e.g., `static_assert` or checking that the include resolves) to make it automatable.

- **W-06: Dead/stale includes in `inspector_editors.cpp`**
  `#include "commands/rename_entity_command.h"` is included with the comment "not needed here". This should be removed. The `#include "engine_context.h"` in `inspector_editors.h` has a comment "(needed?)" — it should be removed if not needed, or the comment should be resolved.

## Required changes

1. **Fix fallback path** (B-01): Implement the spec-required fallback rendering in `InspectorTypeEditorRegistry::draw<T>()`. The template must either call a non-template fallback function (declared in the header, defined in `.cpp`) or be moved to `.cpp` with explicit instantiations.

2. **Fix `set_selection` primary tracking** (B-02): Use `ids[0]` (the first element of the input span) instead of `*current_.selected_.begin()`.

3. **Add `#include <limits>`** (B-03) to `inspector_editors.h`.

4. **Add math type includes** (B-04) to `inspector_editors.cpp`: `"math/quat.h"`, `"math/vec2.h"`, `"math/vec3.h"`, `"math/vec4.h"`, and `<glm/gtc/matrix_transform.hpp>` (for `glm::degrees`/`glm::radians`).

5. **Add or document missing test coverage** for AC-19, AC-20, AC-21, AC-22, AC-23, AC-25, AC-26, AC-28 (see W-01).

6. **Align label type with spec**: Change `const std::string& label` to `std::string_view label` throughout the `InspectorTypeEditor` hierarchy, or explicitly document the deviation.

7. **Clean up unused includes** in `inspector_editors.cpp`: remove `#include "commands/rename_entity_command.h"`.

## Suggested improvements

- Move `draw_fallback_text_input()` from a standalone function into a non-template `InspectorTypeEditorRegistry::draw_fallback()` (declared in header as private static, defined in `.cpp`) and call it from the `draw<T>()` template.

- Consider using `EditorFlags step_value` as the DragFloat speed for Quat's built-in editor (currently hardcoded to 0.5) to make it consistent with other editors.

- Add an explicit test for gimbal-lock angle wrapping behavior (AC-21 edge case: yaw and roll sum around ±90° pitch).

- The `rename_buffer_` sync logic in `draw_entity_name` could be simplified by storing the last-known `current_name` alongside the buffer, similar to the Scene Panel's explicit `start_rename`/`confirm_rename` state machine.

- Add a `static_assert` or `constexpr` version check for `ImGui::BeginDisabled` availability (ImGui ≥ 1.91) to make W-05 from spec-critic fail at compile time rather than silently.

## Re-review (human-validation design change) — ACCEPTED ✅

The design change from `const SerializationContext* ser_ctx = nullptr` to `const EditorContext& ctx` has been applied consistently across the contract. No new blocking issues.

### Consistency verification

| Check | Status | Location |
|-------|--------|----------|
| `InspectorTypeEditor::draw()` signature | ✅ `const EditorContext& ctx` | Line 223 |
| `TypedInspectorEditor<T>::DrawFn` typedef | ✅ `const EditorContext&` | Line 231 |
| `TypedInspectorEditor<T>::draw()` override | ✅ `const EditorContext& ctx` | Line 237 |
| `InspectorTypeEditorRegistry::draw<T>()` signature | ✅ `const EditorContext& ctx` | Line 259 |
| `draw_fallback_readonly()` signature | ✅ `const EditorContext& ctx` | Line 272 |
| `draw_fallback_editable()` signature | ✅ `const EditorContext& ctx` | Line 277 |
| 8 built-in editor lambdas | ✅ All use `const EditorContext&` | Lines 449–617 |
| PropertiesPanel `draw_ui()` | ✅ `EditorContext const& ctx` | Line 652 |
| PropertiesPanel `draw_entity_name()` | ✅ `EditorContext const& ctx` | Line 660 |
| PropertiesPanel `draw_transform_section()` | ✅ `EditorContext const& ctx` | Line 661 |
| Stale `SerializationContext*` (pointer) parameter | ✅ **Zero remaining** | — |
| Stale `ser_ctx` as parameter name | ✅ **Zero remaining** — local `ser_ctx` in fallback is a correct value-type construction from `ctx.engine.services.assets()` | — |
| Header includes | ✅ `"editor_context.h"` replaces old `"engine_context.h"` | Line 197 |

### Fallback path verification

`draw_fallback_editable()` (line 390) correctly constructs `SerializationContext{ctx.engine.services.assets()}` from the non-nullable `EditorContext&`. The `draw<T>()` template (line 315) unconditionally calls `draw_fallback_editable()` when no editor is registered. This is correct — since `EditorContext&` is non-nullable, there is no read-only fallback scenario.

### New finding (W-07): `draw_fallback_readonly()` is dead code

`draw_fallback_readonly()` is declared (line 271) and defined (line 379) but **never called**. The `draw<T>()` template only calls `draw_fallback_editable()`. Previously it served the read-only branch when `ser_ctx` was null; now that `EditorContext&` is non-nullable, this function is unreachable. Should be removed or explicitly documented as reserved for future use.

### Previously flagged issues — re-check

| Issue | Previous Status | Current Status |
|-------|----------------|----------------|
| **B-01** Fallback unwired | ✅ Resolved (loop-back #2) | ✅ Still resolved |
| **B-02** `set_selection(span)` uses `ids[0]` | ✅ Resolved (line 166) | ✅ Still resolved |
| **B-03** Missing `<limits>` | ✅ Resolved (line 201) | ✅ Still resolved |
| **B-04** Missing math includes | ✅ Resolved (lines 342–345) | ✅ Still resolved |
| **W-01** Test coverage gaps | ⚠️ Carried forward | ⚠️ Unchanged |
| **W-02** `std::string` vs `string_view` | ⚠️ Carried forward | ⚠️ Unchanged |
| **W-03** `IsItemActive()` before `InputText` | ⚠️ Carried forward | ⚠️ Unchanged |
| **W-04** AC-25 inapplicable (direct mutation) | ⚠️ Carried forward | ⚠️ Unchanged |
| **W-05** AC-30 code-review only | ⚠️ Carried forward | ⚠️ Unchanged |
| **W-06** Dead/stale includes | ⚠️ Carried forward | ⚠️ **Partially resolved**: `rename_entity_command.h` removed from `.cpp`; `engine_context.h` still in `.cpp` line 340 (redundant since `editor_context.h` is transitively included via `inspector_editors.h`, but harmless) |

### Blocking issues

None. The design change is consistently applied. All previous blocking issues remain resolved.

### Warnings (updated)

Carried forward from previous reviews:
- **W-01**: Test coverage gaps for AC-05/06/19/20/21/22/23/25/26/28 — partially addressed.
- **W-02**: `const std::string&` vs spec's `std::string_view` — still `std::string`, unresolved.
- **W-03**: `IsItemActive()` before `InputText` in `draw_entity_name()` — unchanged.
- **W-04**: AC-25 (rapid edits, one Command) documented as inapplicable (direct mutation). ✓
- **W-05**: AC-30 code-review only — unchanged.
- **W-06**: `engine_context.h` include in `.cpp` (line 340) is likely redundant but harmless. Remove if confirmed unnecessary.

New:
- **W-07**: `draw_fallback_readonly()` is dead code — declared (line 271) and defined (line 379) but never called. Should be removed or explicitly documented as reserved for future use.
