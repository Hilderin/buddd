# Implementation Contract Review — Scene Source Tracking and Saver

## Blocking issues

No blocking issues found.

## Warnings

- **tests/component_registry_tests.cpp modified (not in allowed list):** The file `tests/component_registry_tests.cpp` was modified to update an existing mesh renderer test — the original test expected a null model property to serialize as empty string `""`, but the new default-checking behavior correctly omits it. This change is a necessary consequence of the contract's Step 4b (default-valued properties are now omitted during serialization). While not explicitly listed as "allowed to change", it is also not in the "forbidden" list. This is acceptable but should be noted as a scope boundary deviation.

- **`root_entity_count()` / `get_root_entity()` placed in public instead of private:** The contract specifies these should be in the private helper section (accessible via `friend class SceneSaver`). The implementer placed them in the public section instead. This is a minor deviation from the contract's access level specification, but it is functionally harmless and makes the methods reusable beyond SceneSaver.

- **`property.cpp` modified but not in explicit allowed list:** Changes to `property.cpp` (updated constructor signature, `serialize()` default check, `has_default()` method) are a direct consequence of the header changes in `property.h` (which IS in the allowed list). Acceptable since the .cpp must match the .h declarations.

- **`friend class SceneSaver` declaration is technically unnecessary:** Since `SceneSaver` accesses all World internals through public Entity API methods (`entity.source()`, `entity.component_at()`, etc.) and the now-public `root_entity_count()`/`get_root_entity()`, it never directly calls any private World method. The friend declaration is a no-op and can be kept for forward-compatibility.

## Required changes

None.

## Suggested improvements

- **Test 8 (Model source save) should also verify `components:` is absent:** Test 8 checks `CHECK_FALSE(ent["children"].IsDefined())` but does not explicitly check `CHECK_FALSE(ent["components"].IsDefined())` for Model-source entities. Currently the entity has no components so this naturally passes, but adding the explicit check would make the test more robust against regressions.
