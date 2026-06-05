# [Spec | Implementation Contract] Review — 3D Cube Demo (SPEC-009 / IMPL-009)

## Status

`Rejected`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [ ] **B-01: Contradiction in "Files to modify" heading (3 vs 4 entries)**

  In the "Files allowed to change" section (line 97), the heading says:

  > ### Files to modify (3 files)

  but the table immediately below it lists **4 items** (1. demo_helpers.h, 2. demo_helpers.cpp, 3. demo_command.cpp, 4. tests/CMakeLists.txt). The overview section at lines 40–44 correctly says "Files to modify **(4)**".

  This contradiction creates confusion about whether `tests/CMakeLists.txt` is intended to be modified. The heading should be corrected to "(4 files)".

  **Fix**: Change `(3 files)` to `(4 files)` on line 97.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Warnings

Non-blocking concerns for awareness:

- **W-01: Missing observability requirement — Model creation success log (spec contradiction)**

  The accepted spec's [Observability section](.specs/sprint-2026-05/3d-cube-demo/spec.md#observability) (lines 530–539) explicitly requires:

  > `std::cerr << "Model created (" << vertex_count << " vertices"; if (has_indices) { std::cerr << ", " << index_count << " indices"; } std::cerr << ")\n";`

  The implementation contract never mentions this logging requirement. The contract's `Model::create()` and `Model::create_indexed()` factory method specifications (sections 1–2) make no reference to any `std::cerr` output. An implementer following only the contract would omit this observability signal.

  While the demo start/end/abort messages are correctly specified in the cube_demo section, the Model creation log is absent. The contract should either (a) add the observability requirement to the Model factory sections, or (b) explicitly document why this spec requirement is intentionally omitted (if it is).

- **W-02: `index_count()` accessor on a null/moved-from model returns 0 — gap between accessor UB contract**

  The contract says `material()` and `vertices()` dereference their pointers (UB on null/moved-from models), and `indices()` also has UB on non-indexed models. However, `index_count()` returns member `index_count_` which is `0` for null/moved-from models — this is safe (the member is `0`). But `vertex_count()` similarly returns `vertex_count_` which would also be `0` for a null model.

  The contract should clarify: is calling `index_count()` or `vertex_count()` on a null/moved-from model defined behavior (returning 0) or UB? The spec (line 208) says "accessors return null references (undefined behaviour)" which applies to the reference-returning accessors (`material()`, `vertices()`, `indices()`), but the value-returning accessors (`vertex_count()`, `index_count()`) might be safe. The edge case table could benefit from clarifying this nuance.

- **W-03: Test T-10 verification is weak ("compiles" without semantic assertion)**

  T-10 ("Model::material const overload") verification says "Compiles" without verifying the const-correctness semantically. Consider adding a `static_assert` or const-correctness check (e.g., ensuring the returned reference cannot be used to mutate through the const path). This is a minor test quality concern.

- **W-04: `tests/CMakeLists.txt` modification position could use more precise instruction**

  The contract says to add `model_tests.cpp` "after `scene_graph_tests.cpp`" in both branches. However, the concrete example in section 0 shows `model_tests.cpp` after `scene_graph_tests.cpp` but before the closing parenthesis. A note about not removing the trailing `)` from the `add_executable` block would prevent a common copy-paste error.

## Required changes

Concrete, actionable changes requested:

- **[B-01]** Change heading `### Files to modify (3 files)` to `### Files to modify (4 files)` at line 97 in `.specs/sprint-2026-05/3d-cube-demo/implementation-contract.md`.

## Suggested improvements

Optional ideas (not required):

- Add the Model creation success logging requirement (from SPEC-009 Observability) to the Model factory sections, or document why it is intentionally excluded.
- Add a note clarifying that `vertex_count()` and `index_count()` on a null/moved-from model are safe (return 0), unlike the reference-returning accessors which are UB.
- Add an explicit static_assert or const-correctness check to test T-10 to verify the const overload is truly const-immutable.
- Consider adding a clarifying sentence in the `tests/CMakeLists.txt` section that the `)` closing the `add_executable` block must not be removed when inserting the new line.
