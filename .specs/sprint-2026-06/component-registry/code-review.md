# Implementation Contract Review — Component Registration & Property System

## Blocking issues

No blocking issues found.

**All acceptance criteria (AC-001 through AC-039) are satisfied** — verified against:
- Spec and implementation contract requirements
- Code structure and API signatures
- Unit tests (36 test cases, 209 assertions)
- Full test suite (474 tests, 21693 assertions pass with zero regressions)
- Build produces zero warnings from `src/` or `tests/` code

## Warnings

Non-blocking concerns for awareness:

- **yaml-cpp in public headers (ADR-019 deviation)**: `type_registry.h` and `component_info.h` include `<yaml-cpp/yaml.h>` directly. ADR-019 mandates yaml-cpp headers only in `.cpp` files. This is a necessary deviation because template method definitions are inline in these headers and require the complete `YAML::Node` type. Known and documented by the implementer. No change requested.

- **Light component min constraint vs spec**: AC-011/012/013 in the spec use notation `min>0` for `intensity`, `range`, `inner_angle`, `outer_angle`, and EC-08 says setting zero should produce an error. However, the implementation uses `PropertyFlags{}.min(0.0f)` which allows zero values. The implementation contract explicitly specifies `min(0.0f)` (line 1188) and the spec's registration examples also show `min(0.0f)`. This is a pre-existing spec inconsistency resolved by the contract. Not an implementation defect.

- **`resolve_model()` destructive move**: `AssetManager::resolve_model()` moves the Model out of the cached ModelAsset (`root.model.reset()`). If two components reference the same model, the second resolution will fail because the cached model data has been moved out. Documented as v1 limitation in the contract (line 1253).

- **`PropertyFlags::min_value` default edge case**: The default `min_value = -std::numeric_limits<float>::max()` means values at or below approximately `-3.4e+38` would be rejected even on unconstrained float properties. This is an extreme edge case and not practically reachable with normal component data.

- **Test double-registration**: `TestEngine` in the test file calls `register_builtin_types()` + `register_all_components()` during construction (via `EngineService::create()`), and then individual test sections also call `register_builtin_types()`. This triggers harmless duplicate registration warnings. Functionally fine but pollutes test logs.

## Required changes

No required changes.

All acceptance criteria (AC-001 through AC-039) are covered by automated tests. All code compiles cleanly. All tests pass.

## Suggested improvements

Optional ideas (not required):

- **Quat/Vec3/4 `from_string` parsing robustness**: The string parsers in `register_all_components.cpp` use sequential `std::from_chars` calls with manual whitespace/comma skipping. The format is fixed as `"(x, y, z)"` by spec, so this works correctly. However, consider extracting a shared helper function to avoid the repetitive parsing logic across Vec3/Vec4/Quat.

- **Property `to_string`/`from_string`/`validate` stubs**: These are currently empty stubs returning `{}` / empty string. When the editor property panel is implemented later, these should be wired to TypeRegistry callbacks for proper functionality.

- **Consider storing ComponentRegistry as a member of EngineService**: Currently the registry is a local variable in `EngineService::create()` and destroyed after startup. If future features need runtime registry access (e.g., editor, scene serialization), it should be promoted to a member.

- **Consider `static inline` for template definitions**: The template method definitions in `type_registry.h` and `component_registry.h` are implicitly inline by virtue of being defined in a header. No ODR issues expected with modern compilers, but adding `inline` explicitly would be more defensive.
