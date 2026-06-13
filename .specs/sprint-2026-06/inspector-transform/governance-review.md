# Governance Review — F-05 Inspector — Transform

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] Spec §154-162 requires editable TypeRegistry fallback text input (AC-05, AC-06). Implementation uses read-only fallback. Deviation documented in contract and accepted during contract review (yaml-cpp include paths private to engine).
- [x] Spec uses `std::string_view` for draw() label parameter. Contract and implementation use `const std::string&`. Minor convention deviation — no functional impact.
- [x] Contract declared `draw_fallback_readonly` as private static method of `InspectorTypeEditorRegistry`. Implementation uses free function. Structural deviation, functionally equivalent.
- [x] Contract declared `draw_fallback_editable` which was removed in implementation as dead code. Consistent with read-only fallback design choice.
- [x] Spec (G-06, Story 4) lists Scale as editable. Contradicted by original north-star UX spec which listed Scale as read-only in MVP1. Resolved by D-01 deviation documentation and Human Validation approval.
- [x] **ARCHITECTURE BOUNDARY VIOLATION (resolved)**: `src/editor/inspector_editors.cpp` originally included `<glm/glm.hpp>` and `<glm/gtc/matrix_transform.hpp>` directly, violating ADR-019 and ADR-027 Decision 6. **Fixed in loop-back**: all GLM includes removed; `glm::degrees()`/`glm::radians()` replaced with inline `180.0 / PI` math.

## ADR alignment

Required ADRs exist or are proposed:

- [x] ADR-027 (Editor Architecture) — Implementation follows static library, namespace, App lifecycle patterns. Decision 6 ("No GLM headers in editor code") — **resolved in loop-back**.
- [x] ADR-028 (TypeRegistry) — InspectorTypeEditorRegistry follows the same static-registry pattern. Consistent.
- [x] ADR-026 (ImGui Integration) — PropertiesPanel uses ImGui widgets via EditorContext. Consistent.
- [x] ADR-019 (Architecture Boundaries) — **resolved in loop-back**: GLM includes removed from editor code.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] `docs/wiki/editor/editor-panels.md` — F-05 additions accurately documented (entity name field, Transform section with editable Position/Rotation/Scale, no-selection state, multi-select via primary(), TypeEditorRegistry, Quat editor with Euler degrees, deferred read-only mode for F-15).
- [x] `docs/wiki/editor/cross-panel-communication.md` — F-05 section correctly documents PropertiesPanel consumption of primary() and direct-mutation transform editing.
- [x] `docs/wiki/domain/glossary.md` — Entries for InspectorTypeEditor, InspectorTypeEditorRegistry, EditorFlags, primary() added correctly.
- [x] `docs/wiki/architecture/module-map.md` — Editor library source list updated: inspector_editors.h/.cpp, properties_panel.cpp added to concrete dockable panels table.
- [x] Wiki accurately notes Scale is editable (matching the accepted spec, not the original north-star UX).
- [x] No contradictions found between wiki content and current implementation state.

## Warnings

Non-blocking concerns for awareness:

- Editable TypeRegistry fallback not implemented (spec AC-05, AC-06) — matches accepted contract's read-only fallback design. Future work: add type-erased bridge.
- PropertiesPanel snapshot tests deferred (AC-15..18, AC-24) — no headless ImGui infrastructure. Manual smoke testing substituted.
- Integration tests for AC-19, AC-20, AC-21, AC-22, AC-23, AC-25, AC-26, AC-28 deferred — compile-time and data-path tests substituted.
- `draw_fallback_readonly()` is a free function, not a static method — functionally equivalent.
- Entity name `ImGui::IsItemActive()` called before `InputText()` — fragile but verified correct by code review.
- `draw() uses const std::string&` instead of spec's `std::string_view` — no functional impact.
- The `Vec2` type has a dedicated editor in InspectorTypeEditorRegistry but is NOT registered in the engine's TypeRegistry (per ADR-028). This means if Vec2 were ever used as a component property type, its fallback path would fail. Currently not an issue since Vec2 is not a component property type.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

1. ~~**Fix GLM boundary violation**~~ — **RESOLVED IN LOOP-BACK (14-Jun-2026)**: Removed `<glm/glm.hpp>` and `<glm/gtc/matrix_transform.hpp>` from `src/editor/inspector_editors.cpp`. Replaced `glm::degrees()`/`glm::radians()` with inline `180.0 / PI` math (approach a). Verified: no GLM references remain in `src/editor/`.

2. **No new ADRs required** — the GLM violation was a bug fix, not an architectural decision. No ADR needed.

3. **Wiki already accurate** — no additional wiki updates required for F-05 content.

## Loop-back verification (14-Jun-2026)

Verification of two fixes applied since first governance review:

### G-01: GLM headers in editor code (ADR-019 violation) — RESOLVED ✅

- **Fix applied**: Removed `#include <glm/glm.hpp>` and `#include <glm/gtc/matrix_transform.hpp>` from `src/editor/inspector_editors.cpp`. Replaced `glm::degrees()`/`glm::radians()` with inline math (`180.0 / PI` constants).
- **Verification**: `grep -rn "glm" src/editor/` returns zero results. No GLM references remain in editor code.
- **Build**: Clean incremental build (ninja: no work to do).
- **Tests**: 22582 assertions in 672 test cases — all pass.

### G-02: ImGui PushID/PopID for ID conflict prevention — RESOLVED ✅

- **Fix applied**: Added `ImGui::PushID(label.c_str())` / `ImGui::PopID()` pairs to all Vec2, Vec3, Vec4, and Quat editors in `src/editor/inspector_editors.cpp`.
- **Verification by editor**:
  - Vec2: lines 119/127
  - Vec3: lines 145/155
  - Vec4: lines 174/186
  - Quat: lines 225/238
- **Impact**: Prevents ImGui ID collisions when the same editor type is used multiple times in one window (e.g., Position + Scale both using Vec3 editor).

### Cross-document consistency (re-check)

- ✅ Spec ↔ ADRs: ADR-019 and ADR-027 Decision 6 no longer violated.
- ✅ Spec ↔ Contract ↔ Code: No new contradictions introduced.
- ✅ Tests: All 672 tests pass, confirming both fixes are non-breaking.
- ✅ Wiki: Already accurate per first review — no additional wiki changes needed.

### Blocking issues after loop-back

**None.** Both previously identified blocking issues (GLM violation, ImGui ID conflict) are now resolved.
