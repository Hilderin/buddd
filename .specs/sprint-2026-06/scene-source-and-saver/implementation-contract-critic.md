# Implementation Contract Review — Scene Source Tracking and Saver

## Blocking issues

Items that must be resolved before the artifact can be accepted.

### Previously resolved (carried forward from earlier review cycles)

- [x] **Const-correctness violation in `build_type_to_info_map()`** — Fixed: contract now uses `const_cast` with safety explanation. ✓
- [x] **Missing test coverage for AC-007 and AC-008** — Fixed: Tests 4 and 4b added. ✓

### Previously blocking issues (now resolved)

- [x] **AC-020/021/022 missing from Done criteria checklist**: Fixed — AC-020, AC-021, and AC-022 are now present as `- [ ]` checklist items at lines 1349–1351. ✓
- [x] **No test coverage for AC-020, AC-021, AC-022**: Fixed — Tests 12, 13, 14 added with full code for default transform omission, default property omission, and all-default component no-properties-key. ✓
- [x] **Contradiction between contract and spec on `transform:` omission** (4 locations): All four locations verified correct:
  1. `save_entity()` now uses a conditional `maybe_transform` lambda (line 631–634) that checks `t.IsMap() && t.size() > 0` before returning a non-null node. Usage: `if (auto t = maybe_transform(); !t.IsNull()) node["transform"] = t;` — correctly omits the key when all fields are default. ✓
  2. `save_transform()` description now says "Fields that match their default values are omitted" (line 789). ✓
  3. Edge case table now reads: "`transform:` key omitted entirely when all fields are at defaults" (line 1287). ✓
  4. AC-018 Done criteria now references AC-020 and states "Default-valued fields are OMITTED from saved YAML" (line 1347). ✓
- [x] **SerializationContext missing in default value computation**: Fixed — Step 4b no longer calls `TypeRegistry::yaml_encode()` during registration. Instead, it computes `PropType default_raw = getter(default_instance)` (a raw C++ value) and stores a `DefaultChecker` lambda that compares `getter(typed) == default_raw` using raw `operator==`. No `SerializationContext` needed at registration time. Overload C uses `static thread_local std::optional<PropType>` for lazy default computation. ✓

## Warnings

Non-blocking concerns for awareness:

- **AC-014 (`save_to_file`) not in minimum test set**: The Done criteria checklist requires AC-014 but the minimum test set does not include it — Test 11 is listed only as "recommended". The implementer should ensure AC-014 is tested despite being outside the minimum set.

- **`build_type_to_info_map()` creates temporary component instances at construction time**: For each registered component type, `create()` is called and a `unique_ptr<Component>` is constructed then discarded. Functionally correct but a minor performance consideration.

- **Duplicate "Test 5" numbering**: Two tests are labeled "Test 5" ("Save empty World" and "Save None-source entity with components"). The duplicate numbering could cause confusion. The second "Test 5" should be renumbered (e.g., "Test 8").

- **Stale duplicate minimum set text**: Line 1267 of the contract contains two conflicting "Required minimum set" sentences — the first (correct) one lists Tests 1, 2, 3, 4, 5, 6, 7, 12, 13, 14; the second (stale) one lists the old set with a reference to non-existent "Test 8". The stale sentence should be removed.

- **Spec AC-021 mentions TypeRegistry::yaml_encode, contract uses raw comparison**: The spec says defaults are computed "via `TypeRegistry::yaml_encode` on a default-constructed component" but the contract correctly uses raw PropType comparison (avoiding the SerializationContext dependency). Behaviorally identical — the contract's approach is strictly better. The spec AC-021 implementation detail could be updated for accuracy but is not blocking.

## Required changes

Concrete, actionable changes requested:

### Previously resolved

1. ~~**Fix `build_type_to_info_map()` to compile**: Use `const_cast<ComponentInfoBase*>(info)->create()` since the underlying objects are non-const and `create()` does not modify state.~~ ✅ Resolved — contract now uses `const_cast` with safety explanation.

2. ~~**Add test code for AC-007 and AC-008**: Either provide unit tests that verify model source tracking and model-child None source, or document why these cannot be unit-tested and specify alternative verification.~~ ✅ Resolved — Tests 4 and 4b added for AC-007 and AC-008 with graceful skip logic.

### This cycle (final re-review 2026-06-11)

All 4 previously blocking issues have been resolved in the contract update:

3. ~~**Add AC-020/021/022 to Done criteria checklist**: Insert three new checklist items after the existing AC-019 item.~~ ✅ Resolved — AC-020, AC-021, AC-022 checklist items present at lines 1349-1351.
4. ~~**Add tests for AC-020/021/022**: Create test(s) verifying omission of default transform fields, default component properties, and all-default component no-properties-key.~~ ✅ Resolved — Tests 12, 13, 14 added with full code.
5. ~~**Fix `save_entity()` to conditionally emit `transform:`**: Omit the key entirely when all transform fields are at defaults.~~ ✅ Resolved — `maybe_transform` lambda + conditional assignment at lines 631-655.
6. ~~**Fix contradictory `save_transform()` description**:~~ ✅ Resolved — description now says "Fields that match their default values are omitted" (line 789).
7. ~~**Fix edge case table**:~~ ✅ Resolved — line 1287 says "`transform:` key omitted entirely".
8. ~~**Fix AC-018 Done criteria description**:~~ ✅ Resolved — line 1347 references AC-020, says "Default-valued fields are OMITTED".
9. ~~**Fix SerializationContext gap in Step 4b**:~~ ✅ Resolved — Step 4b uses raw PropType comparison, avoids `TypeRegistry::yaml_encode()` at registration time entirely.

No new blocking issues found.

## Suggested improvements

Optional ideas (not required):

- Consider whether the `type_to_info_` map could be built lazily (first `save_entity()` call) rather than eagerly in the constructor. This avoids creating temporary component instances if the SceneSaver is constructed but `save_to_yaml()` is never called.
- The `serialize_component` call in `save_entity()` uses `comp` (a `Component&`) passed as `const Component&`. This is correct but may be surprising — consider noting explicitly that `serialize_component` takes `const Component&` and does not modify the component.
- For the SerializationContext issue in Step 4b, a pragmatic solution is to compute the default PropType value in `add_property()` overload B (simple lambdas, no context needed), then lazily encode it to YAML during the first `Property::serialize()` call. This avoids needing a context at registration time entirely.

---

**Review history**:
- **2026-06-11**: Initial review. Found 2 blocking issues (const-correctness, missing AC-007/AC-008 tests), 3 warnings.
- **2026-06-11**: Re-review. Both blocking issues confirmed fixed. Verdict: accepted.
- **2026-06-11**: Second re-review after human feedback update. Found 4 new blocking issues: (1) AC-020/021/022 missing from Done criteria, (2) no tests for AC-020/021/022, (3) multiple contradictions on transform omission, (4) SerializationContext missing from `TypeRegistry::yaml_encode` call in Step 4b. Verdict: **rejected**.
- **2026-06-11**: Final re-review. All 4 previously blocking issues confirmed fixed. No new blocking issues. Contract is consistent with the spec on all behavioral requirements. Minor cosmetic warnings remain (duplicate test numbering, stale duplicate minimum set text). Verdict: **accepted**.
