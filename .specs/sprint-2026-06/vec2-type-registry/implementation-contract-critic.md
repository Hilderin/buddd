# Implementation Contract Review — Vec2 TypeRegistry Registration

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] ~~**Missing test coverage for AC-009** (non-numeric sequence element)~~ → RESOLVED: Non-numeric element test `[1.0, "abc"]` added to `YAML_CONVERT_VEC2_REJECTS_INVALID` (lines 248-252).

- [x] ~~**Missing test coverage for AC-013** (`to_string` format)~~ → RESOLVED: `to_string(Vec2{1.5, -3.0})` → `"(1.500000, -3.000000)"` test added to `VEC2_TYPE_REGISTRY` (lines 264-266).

- [x] ~~**Missing test coverage for AC-014** (`from_string` parse)~~ → RESOLVED: `from_string("(1.5, -3.0)")` success test added to `VEC2_TYPE_REGISTRY` (lines 268-272).

- [x] ~~**Missing test coverage for AC-015** (`from_string` error cases)~~ → RESOLVED: `from_string("hello")` → error and `from_string("(1.5)")` → error tests added to `VEC2_TYPE_REGISTRY` (lines 274-278). Spec AC-015 updated (Option B — align on Vec3, no trailing-content check), human approved.

- [x] ~~**Missing test coverage for AC-016** (validate no-op)~~ → RESOLVED: Validate no-op test added to `VEC2_TYPE_REGISTRY` (lines 280-282).

- [x] ~~**Missing test coverage for AC-018** (YAML roundtrip for multiple values)~~ → RESOLVED: Multi-value roundtrip (zero, negative, mixed signs) added to `YAML_CONVERT_VEC2` (lines 203-215).

- [x] ~~**Missing test coverage for AC-019** (String roundtrip)~~ → RESOLVED: `from_string(to_string(v))` roundtrip test added to `VEC2_TYPE_REGISTRY` (lines 284-289).

- [x] ~~**Spec-level contradiction in AC-015 vs. `from_string` behavior**~~ → RESOLVED: Spec AC-015 updated per human decision (Option B — align on Vec3). Contract `from_string` now explicitly documents the Vec3-matching no-trailing-content-check behavior (lines 167, 330).

- [x] ~~**Include ordering inconsistency**~~ → RESOLVED: Convention #4 clarified to explicitly state `vec2_yaml.h` goes after `vec3_yaml.h` (before `vec4_yaml.h`), matching the insertion instruction. The ordering `vec3 → vec2 → vec4` is now the stated convention.

**No remaining blocking issues.**

## Warnings

Non-blocking concerns for awareness:

- **Existing Vec3/Vec4/Quat tests also lack string roundtrip coverage**: The existing test file does not contain `to_string`/`from_string` roundtrip tests for Vec3, Vec4, or Quat. Adding them for Vec2 only (per spec requirements) would introduce a coverage asymmetry — the new type would be better tested than the existing ones. This is acceptable but worth noting for consistency.

- **`from_string` trailing-content behavior**: The contract deliberately follows Vec3's pattern, which does not validate that the last `from_chars` consumed all inner content. This means inputs like `"(1, 2) extra"` are caught by the `s.back() != ')'` check, but `"(1.5, -3.0, 4.0)"` silently produces `Vec2{1.5, -3.0}`. This matches Vec3 behavior but may be surprising. The contract-author already flagged this; it remains a design tension between spec AC-015 and existing conventions.

- **No test for `from_string` whitespace handling**: The spec's edge cases mention `from_string("( 1.5 , -3.0 )")` should work, but the contract doesn't explicitly test whitespace handling.

## Required changes

All previously required changes have been implemented:

1. ✅ AC-009 test case (non-numeric element) added to `YAML_CONVERT_VEC2_REJECTS_INVALID`.
2. ✅ New `VEC2_TYPE_REGISTRY` test section covers AC-013 (`to_string`), AC-014 (`from_string` success), AC-015 (`from_string` errors), AC-016 (validate), AC-019 (string roundtrip).
3. ✅ AC-018 multi-value roundtrip added to `YAML_CONVERT_VEC2`.
4. ✅ AC-015 contradiction resolved (spec updated, Option B — align on Vec3).
5. ✅ Include ordering convention clarified (vec2 after vec3, before vec4).

No new required changes at this time.

## Suggested improvements

Optional ideas (not required):

- Consider adding a whitespace-handling test for `from_string` (e.g., `"( 1.5 , -3.0 )"`) to match the edge case table.

---

## Review history

| Date | Verdict | Summary |
|------|---------|---------|
| 2026-06-14 | **Rejected** | 8 blocking issues found: 7 missing test coverages (AC-009, 013, 014, 015, 016, 018, 019), 1 spec-level contradiction on trailing content, 1 include ordering inconsistency. Contract cannot be accepted until resolved. |
| 2026-06-14 | **Accepted** | Re-review: All 8 blocking issues verified as resolved. Spec AC-015 updated (Option B — align on Vec3, human approved). All missing test coverages added (YAML_CONVERT_VEC2_REJECTS_INVALID, VEC2_TYPE_REGISTRY). Include ordering convention clarified. No new issues found. Contract is complete and precise. |
