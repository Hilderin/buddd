# Code Review — model-multi-material (SPEC-020 / IMPL-020)

## Summary

The implementation of the multi-material Model redesign is **accepted**. The code faithfully implements SPEC-020 and IMPL-020 across all 29+ files. All 317 tests pass (100%). The multi-material demo runs correctly and shows the expected red/green/blue face pairs. All 24 acceptance criteria are covered. Existing demos (triangle, cube) continue to work without visual regression. No blocking issues were found.

## Blocking issues

- [ ] (none)

## Warnings

Non-blocking concerns for awareness:

- **W-001 (carried forward from critics)**: Fallback material verification in headless tests (AC-007, AC-008, AC-014) — the headless backend's `draw_indexed()` only increments `draw_call_count()` and does not track which material was bound during a draw call. Tests verify no crash and correct draw call count but cannot programmatically verify that the *correct* fallback material was used (e.g., bound material matches fallback). This is a test infrastructure limitation, not a code defect. The fallback material binding logic is correct by code inspection.

- **W-002 (minor)**: `Model::vertices()` and `Model::indices()` accessors dereference `vb_` and `ib_` without null guards. Calling these on a default-constructed (null) Model would produce undefined behavior (null dereference). The spec does not define behavior for this case, and tests avoid calling these on null models. Consider adding a guard or documenting preconditions. Existing behavior matches the old code.

- **W-003 (minor)**: `demo_helpers.h` and `demo_helpers.cpp` are kept as empty placeholder files with comments. They could be deleted entirely to complete the cleanup, but keeping them is harmless and provides documentation about what was removed.

- **W-004 (carried forward)**: The implementation contract listed `triangle_app.h` twice in its "Files to modify" list (W-003 from critic). The actual implementation correctly modified it once only. No actual duplication in code.

## Required changes

None.

## Suggested improvements

Optional ideas (not required):

- Add `[[maybe_unused]]` attributes to unused parameters in headless `draw()`/`draw_indexed()` for consistency.
- Consider adding a `last_bound_material()` diagnostic accessor to the headless backend to enable stronger AC-007/AC-008/AC-014 verification in automated tests.
- The fallback material shaders in both OpenGL and Headless backends are duplicated verbatim. Could be shared via a common header, though the current approach is acceptable for a simple magenta shader.

## Detailed review

### Spec compliance (SPEC-020)

| Check | Result | Details |
|-------|--------|---------|
| SubMesh struct | ✅ | `{uint32_t index_start, index_count, material_index}` — matches spec exactly |
| `Model::create_indexed()` | ✅ | Takes `vector<SubMesh>` + `vector<shared_ptr<Material>>`, validates empty data, returns `Result<Model>` |
| `Model::draw()` | ✅ | Iterates submeshes, binds by `material_index`, fallback for null/oob, no-op for empty/moved-from |
| `Model::material()` removed | ✅ | No occurrences in any `.h` or `.cpp` (only `asset.material()` on MaterialAsset in tests) |
| `Model::create()` removed | ✅ | No occurrences anywhere in codebase |
| `Model::has_indices()` removed | ✅ | No occurrences anywhere in codebase |
| Primitive helpers | ✅ | `create_cube()`, `create_triangle()`, `create_quad()` in `src/engine/render/primitives.h/.cpp` |
| Fallback material | ✅ | `RenderDevice::fallback_material()` → `Material&`, magenta shader, lazy-created, cached |
| Multi-material demo | ✅ | `MultiMaterialApp` with 3 submeshes (12+12+12 indices) and 3 materials (red/green/blue) |
| Migration completeness | ✅ | All 11+ apps migrated, demo_helpers cleaned |

### Contract compliance (IMPL-020)

| Check | Result | Details |
|-------|--------|---------|
| Created files | ✅ | `primitives.h`, `primitives.cpp`, `multi_material_app.h`, `multi_material_app.cpp` |
| Modified files | ✅ | All 26 listed files modified as specified |
| `model.h` API | ✅ | Matches contract pseudocode exactly |
| `model.cpp` implementation | ✅ | Factory, draw, accessors, move ctor/assign match contract |
| `render_system.cpp` migration | ✅ | Uses `materials()[0]` with null-guard — correctly addresses W-002 |
| Step-by-step matching | ✅ | All 18 steps implemented faithfully |

### ADR compliance

| ADR | Check | Details |
|-----|-------|---------|
| ADR-001 (Result pattern) | ✅ | All fallible paths return `Result<T>` |
| ADR-003 (draw returns void) | ✅ | `Model::draw()` returns void |
| ADR-010 (no raw pointers) | ✅ | Materials owned via `shared_ptr<Material>`, no raw pointers in public API |

### Acceptance criteria coverage

| AC | Test exists | Verification method |
|----|-------------|-------------------|
| AC-001 | ✅ | Compile-time + runtime test: SubMesh struct fields |
| AC-002 | ✅ | Headless: 2 submeshes, 2 materials, verify submeshes/materials match |
| AC-003 | ✅ | Headless: empty vertex data → InvalidArgument |
| AC-004 | ✅ | Headless: empty index data → InvalidArgument |
| AC-005 | ✅ | Headless: 3 submeshes → draw_call_count +3 |
| AC-006 | ✅ | Headless: draw_call_count increases (material tracking limited — see W-001) |
| AC-007 | ✅ | Headless: null material → draw_call_count == 1 (no crash) |
| AC-008 | ✅ | Headless: oob material_index → draw_call_count == 1 (no crash) |
| AC-009 | ✅ | Headless: empty submeshes → draw_call_count unchanged |
| AC-010 | ✅ | Headless: moved-from model → draw is no-op |
| AC-011 | ✅ | create_cube → 1 submesh, 1 material, 36 indices, 24 vertices |
| AC-012 | ✅ | create_triangle → 1 submesh, 1 material, 3 indices, 3 vertices |
| AC-013 | ✅ | create_quad → 1 submesh, 1 material, 6 indices, 4 vertices |
| AC-014 | ✅ | fallback_material() returns valid reference |
| AC-015 | ✅ | demo_helpers cleaned — no setup_cube/setup_triangle |
| AC-016 | ✅ | CubeResources removed — not found in codebase |
| AC-017 | ✅ | No Model::material() in codebase |
| AC-018 | ✅ | No Model::create() in codebase |
| AC-019 | ✅ | No Model::has_indices() in codebase |
| AC-020 | ✅ | `buddd run triangle` runs without crash (verified manually) |
| AC-021 | ✅ | `buddd run cube` runs without crash (verified manually + visual) |
| AC-022 | ✅ | `buddd run multi-material` runs without crash (verified visually) |
| AC-023 | ✅ | Move preserves submeshes/materials, source becomes empty |
| AC-024 | ✅ | Const-correct access compiles: `const auto& sm = model.submeshes()` |

### Code quality

- **Memory safety**: ✅ All buffers owned by `unique_ptr`, materials by `shared_ptr`. Move semantics correctly transfer ownership. No raw `new`/`delete`.
- **Const-correctness**: ✅ Accessors are `const noexcept`, draw is `const`.
- **Error handling**: ✅ Factory validates inputs and returns `Result<Model>`. Buffer creation failures are propagated via `std::unexpected`. Fallback material failures call `std::terminate()` — acceptable for fatal errors.
- **Naming conventions**: ✅ Follows project naming: PascalCase for types, snake_case for members/functions.
- **Include discipline**: ✅ Minimal includes, forward declarations where possible.
- **Static analysis**: ✅ `static_assert` for non-copyable, movable enforced.

### Visual verification

The multi-material demo was run with `--capture 30` and the output image was analyzed:

- Frame 30 of `buddd run multi-material` shows a rotating cube with **red**, **green**, and **blue** face pairs against a dark background from a 3/4 perspective — matches SPEC-020 Story 5 and AC-022.
- Frame 5 of `buddd run cube` shows a per-vertex colored cube — matches AC-021.
- `buddd run triangle` runs without crash — matches AC-020.

### Git diff analysis

The implementation modifies only the files listed in IMPL-020's "Files to create" and "Files to modify" sections. No forbidden files (error.h, render_device draw() signature, etc.) were modified. No unexpected architectural changes were introduced.
