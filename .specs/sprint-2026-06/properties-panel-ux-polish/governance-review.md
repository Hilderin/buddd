# Governance Review — Properties Panel UX Polish (SPEC-F-06)

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [ ] **No blocking contradictions found.** All three documents (spec, implementation contract, code) agree on the design: 2-column table layout, composite axis widgets (colored drag-handle + InputFloat), axis colors (X/Pitch=red, Y/Yaw=green, Z/Roll=blue, W=gray), label→id semantic rename, rotation as Pitch/Yaw/Roll, Scale min_value=0.001 enforcement, and per-property undo via `mark_dirty()`. The code review confirms all 14 acceptance criteria and 26 done criteria are satisfied.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

### Detailed consistency checks

| Check | Result |
|---|---|
| **Spec → Contract** | Contract faithfully translates all spec requirements (G-01 through G-07, AC-01 through AC-14, NG-01 through NG-11). The `label`→`id` rename was applied to both spec and contract at human request. |
| **Contract → Code** | Code review verifies all contract done criteria (1–26). `draw_axis_widget()` helper, Vec2/Vec3/Vec4/Quat editors, and `draw_transform_section()` table layout all match the contract code samples exactly. |
| **F-06 → F-05** | F-06 is an intentional UX evolution of F-05's Transform section. No contradictions. Scale min_value (0.001) was spec'd in F-05 but never implemented — F-06 correctly adds it (human-approved). |
| **F-06 → ADR-029** | ADR-029 constrains tab system, panel layouts, and Play mode behavior — none of which are affected by F-06. No conflicts. |
| **Post-loop-back fixes** | The `mark_dirty()` contradiction in the contract (Section A step 5 vs Section B) was resolved in loop-back iteration 2. Vec2/Vec4 `std::clamp()` was added. Vec3 clamping is covered by edge cases section and done criteria; actual code includes it. |

### Archival completeness

All required artifacts are present in `SPEC_DIR`:

- `coordination.md` ✓
- `spec.md` ✓
- `spec-critic.md` ✓
- `implementation-contract.md` ✓
- `implementation-contract-critic.md` ✓
- `code-review.md` ✓
- `governance-review.md` ✓ (this file)

## ADR alignment

Required ADRs exist or are proposed:

- [ ] **ADR-029 (Editor UX Decisions)**: The implementation is fully compliant. ADR-029 defines tab types, fixed layouts, Play mode behavior, and entity creation rules — none of which are affected by F-06's Transform section UX improvements. No new ADR is required.
- [ ] **ADR-026 (ImGui Integration)**: The implementation uses `ImGui::BeginTable`, `ImGui::InvisibleButton`, `ImGui::GetMouseDragDelta()`, and `ImGui::InputFloat` — all available in the `v1.91.8-docking` branch specified by ADR-026. No ADR amendment needed.
- [ ] **ADR-027 (Editor Architecture)**: No header changes to `inspector_editors.h` or `properties_panel.h`. No engine files modified. Architecture boundary preserved. No ADR amendment needed.

## Wiki alignment

Wiki reflects current state and does not become law:

- [ ] **Wiki updated correctly**: `docs/wiki/editor/editor-panels.md` was updated by the wiki-agent to reflect:
  - Table layout for Transform section (2-column `ImGui::Table`, no headers)
  - Composite axis input widgets with axis colors
  - `label`→`id` semantic rename
  - Scale `min_value=0.001` enforcement
  - Status banner, Property Editors table, v1 foundation section, Related specs, and Last reviewed date all updated.
- [ ] **Wiki does not contradict ADRs or specs**: The wiki describes F-06 as an implemented feature under the existing F-05/F-06 foundation, consistent with the spec and ADR-029's north-star vision.
- [ ] **Wiki does not become law**: The wiki accurately reflects the current implementation state and does not introduce new requirements or constraints.

## Warnings

Non-blocking concerns for awareness:

- **Scale min_value (0.001) is behaviorally new**: F-05 spec required Scale min_value of 0.001, but F-05 actual code never implemented this constraint. F-06 adds it for the first time. This was flagged by spec-critic, contract-critic, and code-reviewer, and was explicitly approved by human (Hilderin) during human validation. Not a regression, but a deliberate addition.
- **Vec3 Section C code sample lacks `std::clamp()`**: The implementation contract's Vec3 code sample (Section C) does not include explicit `std::clamp()` calls, while Vec2 and Vec4 samples do. The edge cases section and done criteria #11 correctly specify it, and the actual implementation includes clamping. This is a minor documentation inconsistency within the contract, not a code issue.
- **`id` parameter is a semantic rename, not an API rename**: The spec and contract refer to the parameter as `id` (clarifying it is used only for ImGui PushID scoping). However, the C++ `InspectorTypeEditor::draw()` base class signature still uses `label` as the parameter name (since `inspector_editors.h` is not modified per contract non-goals). The editor lambdas in `inspector_editors.cpp` do use `id` as their parameter name, which is consistent with the documentation. Developers reading the spec may expect the base class API to also be renamed.
- **Visual/screenshot verification not performed**: The spec's E2E Verification section (AC-01 through AC-08, AC-14) requires manual visual inspection of the new composite axis widgets, table layout, and axis colors. The code-reviewer could not perform this in a headless environment. Human should perform a manual smoke test and screenshot capture before final sign-off.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- [ ] **No governance document changes required.** The wiki has already been updated. No new ADRs are needed. No existing ADRs contradict the implementation. The governance documentation accurately reflects the current state of the feature.
