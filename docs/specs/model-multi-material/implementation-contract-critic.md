# Implementation Contract Review — model-multi-material (RE-REVIEW v3)

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **BL-001 (resolved)**: `main.cpp` contradiction — previous contract required both `#include "demo/multi_material_demo.h"` (forbidden) and a `buddd demo` command that didn't exist. Now uses `buddd run multi-material` scene dispatch via `MultiMaterialApp` subclass. **Resolved.**
- [x] **BL-002 (resolved)**: Previous contract referenced `triangle_demo.h` which does not exist. The contract now explicitly forbids any dependency on `triangle_demo.h`. **Resolved.**

No new blocking issues found in this re-review (v3).

## Warnings

Non-blocking concerns for awareness:

- **W-001 (carried forward)**: Fallback material verification in headless tests (AC-007, AC-008, AC-014) — the headless backend's `draw_indexed()` only increments `draw_call_count()` and does not track which material was bound during a draw call. There is no `last_bound_material()` or equivalent diagnostic accessor. These ACs as written require a capability the headless backend does not currently provide. The implementer must either (a) add a tracking mechanism to the headless backend, or (b) accept that these ACs are verified via code review rather than automated test. This was flagged in the previous review for the same reason.
- **W-002 (carried forward)**: `render_system.cpp` migration instruction (Step 8) says "Change all `mr.model().material()` calls to `model.materials()[0]`" but the existing code does `auto& material = mr.model().material()` (expecting `Material&`) followed by `material.set_uniform(...)`. After migration, `materials()[0]` returns `shared_ptr<Material>`, not `Material&`, so the implementer must change `.` to `->` and add a null-check guard. The instruction is technically imprecise; the implementer should be aware that the code transformation is not a simple find-and-replace. Verified against actual `render_system.cpp` line 113.
- **W-003 (carried forward)**: `triangle_app.h` is listed twice in the "Files to modify" list (entry #14 and entry #31). Documentation redundancy — not harmful but inconsistent.
- **W-004 (carried forward)**: `multi_material_app.cpp` includes `<span>` but `std::span` is already pulled in by engine headers — minor unnecessary include.
- **W-005 (carried forward)**: Multi-material demo vertex data uses only 2 attributes (position + colour) with 24-byte stride, which is correct and consistent with the spec's assumption A-02.

## Required changes

Concrete, actionable changes requested:

None. The contract is ready for implementation pending resolution of the caveat in W-001 (fallback verification approach).

## Suggested improvements

Optional ideas (not required):

- **Step 8 clarity**: Add a note in Step 8 explaining that `materials()[0]` returns `shared_ptr<Material>`, so the render_system.cpp migration becomes:
  ```cpp
  // Before:
  auto& material = mr.model().material();
  auto r = material.set_uniform("u_mvp", mvp);

  // After:
  auto mat = mr.model().materials()[0];
  if (mat) {
      auto r = mat->set_uniform("u_mvp", mvp);
      // ... rest of uniform logic
  }
  ```
- **Fallback test verification**: Consider noting in the test plan that AC-007 and AC-008 may need a `last_bound_material()` accessor on the headless backend, or explicitly mark them as code-review-only ACs.

## Review summary

- **Completeness**: All 24 ACs, all stories, all goals from the spec are covered in the contract. Migration paths for all 29+ files are listed. Spec's additional test file migration requirements (`tests/lighting_tests.cpp`, `tests/scene_rendering_tests.cpp`) are covered by the contract's Step 17 instruction to "search for existing Model tests."
- **Consistency**: API signatures match the spec exactly. SubMesh struct, Model factory, accessors, primitive helpers, and `fallback_material()` all match.
- **Dead dependencies**: None. Contract correctly removes `demo_helpers.h/cpp` dependencies and does not introduce any new external dependencies.
- **Feasibility**: The approach is sound. All code patterns (move semantics, polymorphic fallback material, submesh iteration during draw) are consistent with existing codebase conventions.
- **Convention compliance**: `Result<T>` pattern used throughout (ADR-001), no raw pointers in public API (ADR-010), `shared_ptr<Material>` for material ownership, `noexcept` on accessors, `const`-correct. CONST-001 compliance maintained (no backend headers leak into apps). ADR-003 draw() returns `void` exception respected.
- **Contradictions with spec**: None found. The contract faithfully implements SPEC-010.
- **Contradictions with constitution/ADRs**: None found.
- **Previous issues**: All previous blocking issues resolved. All 5 warnings (W-001 through W-005) remain valid — none resolved, none worsened.
- **Spec-critic alignment**: Spec-critic v4 declared spec ready with no blocking issues. Contract is aligned with the resolved spec (test files explicitly listed, missing edge case added). No new spec-level changes needed.
