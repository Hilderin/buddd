# SPEC-2026-06-Vec2TypeRegistry — Vec2 TypeRegistry Registration

## Problem

The engine's `math::Vec2` type (`src/engine/math/vec2.h`) is a fully-featured 2D vector with `x, y` float members, GLM interop, arithmetic operators, and vector operations (length, normalize, dot, etc.). The editor's `InspectorTypeEditorRegistry` already has Vec2 registered with a drag-float editor widget.

However, the engine's `TypeRegistry` (which provides YAML serialization, string conversion, and validation callbacks for use in component properties) does **not** know about Vec2. This means:

- No component can use `Vec2` as a property type — properties of type `Vec2` fail to serialize/deserialize.
- No YAML converter exists for Vec2 — scene files cannot contain Vec2 values.
- No `to_string`/`from_string` conversion exists for Vec2 — the inspector text-input fallback (for types without a dedicated editor) would not work.
- Vec2 is the only vector type without TypeRegistry support: `Vec3`, `Vec4`, and `Quat` all have YAML converters and are registered; Vec2 is the missing piece.

## Goals

| ID | Goal |
|---|---|
| G-01 | Create `src/engine/math/vec2_yaml.h` with a `YAML::convert<math::Vec2>` specialization following the same pattern as `vec3_yaml.h` / `vec4_yaml.h` (flow sequence `[x, y]` + legacy mapping `{x, y}`). |
| G-02 | Add `TypeRegistry::register_type<math::Vec2>(...)` in `register_builtin_types()` in `register_all_components.cpp`, delegating YAML encode/decode to `YAML::convert<math::Vec2>`, with `to_string` returning `"(x, y)"` format, `from_string` parsing `"(x, y)"`, and a no-op `validate`. |
| G-03 | Verify via Catch2 unit tests: YAML roundtrip (sequence and mapping formats), string roundtrip, edge cases, error cases. |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | **No change to the editor's `InspectorTypeEditorRegistry`** — Vec2 is already registered there, no editor changes needed. |
| NG-02 | **No change to the `math::Vec2` type itself** — no new methods, constructors, or operators. The existing struct is complete. |
| NG-03 | **No change to existing YAML converters** — `vec3_yaml.h`, `vec4_yaml.h`, `quat_yaml.h`, `color_yaml.h` are unchanged. |
| NG-04 | **No migration of existing component properties** — no existing component currently uses `Vec2` as a property type. This registration enables future use. |
| NG-05 | **No change to the `TypeRegistry` API or `TypeInfo` struct** — the existing callback pattern is used without modification. |

## Actors

| Actor | Description |
|---|---|
| **Engine developer** | Writes C++ code that registers component properties of type `Vec2`. The property serialization/deserialization now works via TypeRegistry. |
| **Scene file author** | Writes/reads YAML scene files containing `Vec2` values in `[x, y]` flow sequence format. |

## User-visible behavior

### Before (current state)

- A component property declared as `math::Vec2` cannot be registered — `ComponentInfo<T>::add_property<math::Vec2>(...)` would fail because `TypeRegistry::register_type<math::Vec2>` has not been called, causing a runtime assertion or undefined behavior.
- No YAML format exists for Vec2 values in scene files.

### After (this feature)

- `math::Vec2` is a first-class TypeRegistry type, on par with `Vec3`, `Vec4`, and `Quat`.
- Component properties of type `math::Vec2` can be registered and will serialize/deserialize correctly.
- YAML serialization produces `[x, y]` flow sequences; YAML deserialization accepts both `[x, y]` sequences and legacy `{x, y}` mappings.
- String conversion produces `"(x, y)"` format; `from_string` parses it.

## User stories

### Story 1 — Vec2 YAML roundtrip (Priority: P1)

As a scene file author, I want Vec2 values in YAML scene files so that components with Vec2 properties can be saved and loaded.

**Given** a `math::Vec2` value `v{3.5, -1.25}`
**When** I encode it to YAML using `YAML::convert<math::Vec2>::encode(v)`
**Then** the output is a flow sequence `[3.5, -1.25]`.

**Given** a YAML node `[3.5, -1.25]`
**When** I decode it using `YAML::convert<math::Vec2>::decode`
**Then** the result is `math::Vec2{3.5, -1.25}`.

### Story 2 — Vec2 YAML legacy mapping format (Priority: P2)

As a scene file author migrating from a hypothetical older format, I want the legacy `{x:, y:}` mapping format to still load correctly.

**Given** a YAML node `{x: 1.0, y: 2.0}` (legacy mapping format)
**When** I decode it using `YAML::convert<math::Vec2>::decode`
**Then** the result is `math::Vec2{1.0, 2.0}`.

### Story 3 — Vec2 string conversion (Priority: P1)

As an engine developer, I want to convert Vec2 values to and from strings for use in the inspector's text-input fallback.

**Given** a `math::Vec2` value `v{1.5, -3.0}`
**When** I call `TypeRegistry::to_string(v, ctx)`
**Then** the result is `"(1.500000, -3.000000)"`.

**Given** the string `"(1.5, -3.0)"`
**When** I call `TypeRegistry::from_string<math::Vec2>(s, ctx)`
**Then** the result is `math::Vec2{1.5, -3.0}`.

### Story 4 — Vec2 registration in TypeRegistry (Priority: P1)

As an engine developer, I want Vec2 to be registered as a built-in type so that I can use it as a component property type.

**Given** the engine has started up and `register_builtin_types()` has been called
**When** I call `TypeRegistry::has_type<math::Vec2>()`
**Then** it returns `true`.

**Given** a component property of type `math::Vec2` (e.g., an `offset` property)
**When** the component is serialized and deserialized
**Then** the Vec2 value roundtrips correctly.

### Story 5 — Vec2 YAML validation rejects invalid formats (Priority: P2)

As an engine developer, I want invalid YAML Vec2 values to be rejected with a clear error.

**Given** a YAML node `[1.0]` (single-element sequence)
**When** I decode it using `YAML::convert<math::Vec2>::decode`
**Then** decode returns `false`.

**Given** a YAML node `[1.0, 2.0, 3.0]` (three-element sequence)
**When** I decode it using `YAML::convert<math::Vec2>::decode`
**Then** decode returns `false`.

**Given** a YAML node `{x: 1.0}` (incomplete mapping)
**When** I decode it using `YAML::convert<math::Vec2>::decode`
**Then** decode returns `false`.

**Given** a YAML scalar node `"hello"`
**When** I decode it using `YAML::convert<math::Vec2>::decode`
**Then** decode returns `false`.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `vec2_yaml.h` exists at `src/engine/math/vec2_yaml.h` with `YAML::convert<math::Vec2>` specialization. | File exists, contains `YAML::convert<buddd::engine::math::Vec2>` template specialization. |
| AC-002 | `YAML::convert<math::Vec2>::encode` produces a flow-sequence node `[x, y]`. | Unit test: `YAML::convert<math::Vec2>::encode(Vec2{3.5, -1.25})` → node `[3.5, -1.25]`. |
| AC-003 | `YAML::convert<math::Vec2>::decode` accepts a two-element sequence `[x, y]`. | Unit test: decode `[3.5, -1.25]` → `Vec2{3.5, -1.25}` succeeds (returns true). |
| AC-004 | `YAML::convert<math::Vec2>::decode` accepts a legacy mapping `{x: , y: }`. | Unit test: decode `{x: 1.0, y: 2.0}` → `Vec2{1.0, 2.0}` succeeds (returns true). |
| AC-005 | `YAML::convert<math::Vec2>::decode` returns false for a single-element sequence `[x]`. | Unit test: decode `[3.5]` → returns false. |
| AC-006 | `YAML::convert<math::Vec2>::decode` returns false for a three-element sequence `[x, y, z]`. | Unit test: decode `[1, 2, 3]` → returns false. |
| AC-007 | `YAML::convert<math::Vec2>::decode` returns false for an incomplete mapping `{x: 1.0}`. | Unit test: decode `{x: 1.0}` → returns false. |
| AC-008 | `YAML::convert<math::Vec2>::decode` returns false for a scalar node. | Unit test: decode `"hello"` → returns false. |
| AC-009 | `YAML::convert<math::Vec2>::decode` returns false for a non-numeric sequence element. | Unit test: decode `[1.0, "abc"]` → returns false. |
| AC-010 | `TypeRegistry::register_type<math::Vec2>(...)` is called in `register_builtin_types()`. | Unit test (or code inspection): the registration block exists in `register_all_components.cpp` with type `math::Vec2`. |
| AC-011 | YAML encode delegates to `YAML::convert<math::Vec2>::encode`. | Code inspection: `.yaml_encode = [](const math::Vec2& v, ...) { return YAML::convert<math::Vec2>::encode(v); }`. |
| AC-012 | YAML decode delegates to `YAML::convert<math::Vec2>::decode`. | Code inspection: `.yaml_decode = [](const YAML::Node& n, ...) { math::Vec2 v; if (!YAML::convert<math::Vec2>::decode(n, v)) return error; return v; }`. |
| AC-013 | `to_string` returns `"(x, y)"` format. | Unit test: `to_string(Vec2{1.5, -3.0})` returns `"(1.500000, -3.000000)"`. |
| AC-014 | `from_string` parses `"(x, y)"` format. | Unit test: `from_string("(1.5, -3.0)")` succeeds and returns `Vec2{1.5, -3.0}`. |
| AC-015 | `from_string` returns error for malformed input (no trailing-content check — matches Vec3 behavior). | Unit test: `from_string("hello")` → error; `from_string("(1.5)")` → error. |
| AC-016 | `validate` is a no-op (returns success). | Unit test: `validate(Vec2{1,2})` returns `Result<void>{}`. |
| AC-017 | `#include "math/vec2_yaml.h"` is added to `register_all_components.cpp`. | Code inspection: the new include line exists between existing YAML includes. |
| AC-018 | YAML roundtrip preserves values — encode then decode returns the original Vec2. | Unit test: for multiple Vec2 values (zero, positive, negative, with mixed signs), encode → decode → compare with original. |
| AC-019 | String roundtrip preserves values — `from_string(to_string(v))` returns the original Vec2 (within float precision). | Unit test: for multiple Vec2 values, string roundtrip produces the original value. |
| AC-020 | Zero new compiler warnings from `src/engine/math/` and `src/engine/scene/component_registry/`. | Build with `cmake --build --preset debug` and verify no new warnings. |

## E2E Verification

| Method | Description |
|---|---|
| **Catch2 unit tests (CI)** | Run `buddd_tests` with a Vec2-specific test section covering YAML encode/decode (both formats), string roundtrip, error cases, and TypeRegistry registration. |
| **Build verification (CI)** | Run `cmake --build --preset debug` and verify zero new warnings from modified files. |
| **Existing component tests** | Existing `component_registry_tests` continue to pass — Vec2 registration does not affect existing types. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | All Vec2-specific acceptance criteria pass in CI (Catch2 tests). | Run `buddd_tests` with relevant tags/sections. |
| SC-002 | All existing engine tests pass unchanged. | Run `buddd_tests` full suite — no regressions. |
| SC-003 | Vec2 is usable as a component property type (serializes/deserializes via TypeRegistry). | Unit test: register a test component with a Vec2 property, serialize to YAML, deserialize back, verify value preserved. |
| SC-004 | YAML backward compatibility: legacy `{x, y}` mapping format loads correctly. | Unit test: decode `{x: 1, y: 2}` → `Vec2{1, 2}`. |

## Edge cases

| Case | Expected behavior |
|---|---|
| **Vec2 with zero values** `{0.0, 0.0}` | Encodes to `[0.0, 0.0]`. Decodes back correctly. |
| **Vec2 with negative values** `{-1.5, -3.0}` | Encodes to `[-1.5, -3.0]`. Decodes back correctly. |
| **Vec2 with NaN values** | Encodes/decodes NaN via yaml-cpp (IEEE 754). Decode with non-numeric YAML input returns false. |
| **Vec2 with Inf values** | Encodes/decodes Inf via yaml-cpp. Decode with non-numeric YAML input returns false. |
| **Very large float values** (e.g., `{1e20, -1e20}`) | Encoded/decoded with full yaml-cpp precision — no rounding beyond what `double` intermediate in yaml-cpp introduces. |
| **Whitespace in string format** | `from_string("( 1.5 , -3.0 )")` — leading/trailing whitespace within parens is handled by `std::from_chars` (skips leading whitespace). Commas followed by spaces are handled by the skip-loop pattern (matching Vec3). |
| **Extra trailing content in string** | `from_string("(1.5, -3.0) extra")` — initial `s.back() != ')'` check catches this (last char is `l` not `)`). `from_string("(1.5, -3.0, 4.0)")` parses Vec2{1.5, -3.0} silently (matching Vec3 pattern — no trailing-content check after last component). |
| **YAML legacy mapping with extra keys** `{x: 1, y: 2, z: 3}` | The `node["x"] && node["y"]` check succeeds, so `{x: 1, y: 2, z: 3}` decodes as `Vec2{1, 2}` (extra keys are ignored). This matches Vec3/Vec4 behavior. |
| **Empty YAML sequence `[]`** | `node.IsSequence() && node.size() == 2` fails → decode returns false. |
| **Null/undefined YAML node** | Try-catch in decode catches `YAML::Exception` and returns false. |

## Error cases

| Error | Handling |
|---|---|
| YAML sequence has size != 2 | `decode` returns false (no error message — caller generates context-aware message). |
| YAML mapping is missing `x` or `y` | `decode` returns false (the `node["x"] && node["y"]` check fails). |
| YAML sequence element is not a float (e.g., string) | `node[i].as<float>()` throws `YAML::Exception` caught by the try-catch; `decode` returns false. |
| YAML node is neither sequence nor map | `decode` returns false. |
| `from_string` input does not start with `(` or end with `)` | Error returned with message: `"Vec2: cannot parse '%s' (expected format '(x, y)')"`. |
| `from_string` input has fewer than 2 float values | Second (or third) `std::from_chars` fails → error returned. |
| Extra content after the second component (e.g., `"(1.5, -3.0, 4.0)"` or `"(1.5, -3.0)xyz"`) | Silently ignored (matching existing Vec3/Vec4 behavior — no trailing-content check). The `s.back() != ')'` check only catches content outside the closing paren (e.g., `"(1.5, -3.0) extra"` has last char `l`). |

## Permissions and security

- No authentication or authorization required — this is a pure data type registration.
- No data validation beyond YAML parse error handling and string format checks.
- No sensitive data exposure — Vec2 values are positional data only.

## Observability

- A one-time `BUDDD_LOG_TAGGED_DEBUG` at startup confirming Vec2 registration success (log tag `"ComponentRegistry"` already exists for other registrations).
- YAML decode failures for Vec2 properties are surfaced through the existing error handling in `component_registry` (logged at WARN level with context).
- String parse failures return `Error` with descriptive message — caller decides how to log/display.
- No runtime metrics needed — all operations are pure computation with no side effects.

## Out of scope

- Changes to `math::Vec2` struct itself.
- Changes to editor's `InspectorTypeEditorRegistry` (already has Vec2).
- Changes to existing YAML converters (`vec3_yaml.h`, `vec4_yaml.h`, `quat_yaml.h`, `color_yaml.h`).
- Migration of existing component properties to use Vec2 (no existing property currently needs it).
- Changes to `TypeRegistry`, `TypeInfo`, or `SerializationContext` APIs.
- Changes to `ComponentInfo::add_property` or property registration mechanism.
- Any other vector type (Vec3, Vec4, Quat, Color) — only Vec2 is missing.

## Assumptions

| ID | Assumption | Rationale |
|---|---|---|
| A-01 | The YAML converter follows the same pattern as `vec3_yaml.h` / `vec4_yaml.h` — flow sequence `[x, y]` on encode; sequence and mapping formats on decode. | Matches existing vector type conventions. The mapping format is "legacy" for future backward compat even though Vec2 has never been serialized before. |
| A-02 | The `to_string` format is `"(x, y)"` with comma + space separator, matching Vec3's `"(x, y, z)"` format minus the z component. | Consistency across vector types. |
| A-03 | The `from_string` parser follows the same pattern as Vec3's parser — strip outer `()`, then parse three floats (two for Vec2) with `std::from_chars` separated by comma/space delimiters. | Reuse the well-tested Vec3 parsing pattern. |
| A-04 | `validate` is a no-op, matching Vec3/Vec4/Quat behavior. | No Vec2-specific validation constraints exist (any float pair is valid). |
| A-05 | The TypeRegistry registration uses exactly the same lambda pattern as Vec3 (lines 114-157 of `register_all_components.cpp`), adapted for 2 components. | Consistency and maintainability — the pattern is well-established. |
| A-06 | The new `#include "math/vec2_yaml.h"` is inserted after `#include "math/vec3_yaml.h"` (alphabetical within the `math/` YAML includes block). | Maintains include ordering convention. |
| A-07 | YAML precision uses full `yaml-cpp` default (no rounding), matching existing Vec3/Vec4 behavior. | HDR values must roundtrip without precision loss. |
| A-08 | The `from_string` parser rejects content outside the closing `)` (e.g., `"(1, 2) extra"`) via `s.back() != ')'`, but does NOT check for trailing content inside the parens after the last component. | Matches Vec3 behavior — the delimiter check catches content outside parens, while content inside after the last component is silently ignored, consistent with A-10. |
| A-09 | Catch2 unit tests are added in an appropriate existing test file (e.g., `tests/engine/math_tests.cpp`) or a new dedicated test file following the project's test organization. | The test location is an implementation detail. |
| A-10 | The `from_string` parser does NOT check for trailing content after the last parsed component, matching Vec3's behavior exactly. Extra commas and values (e.g., `"(1, 2, 3)"`) are silently ignored. | Consistency with Vec3 — adding a check would make Vec2 stricter than other vector types, which contradicts the "same pattern" principle. |

## Open questions

All earlier clarifications have been resolved. No open questions remain.

## Files to create or modify

### New files

| File | Purpose |
|---|---|
| `src/engine/math/vec2_yaml.h` | YAML convert specialization for `math::Vec2` (flow sequence `[x, y]` + legacy mapping `{x, y}`). |

### Modified files

| File | Change |
|---|---|
| `src/engine/scene/component_registry/register_all_components.cpp` | Add `#include "math/vec2_yaml.h"`. Add `TypeRegistry::register_type<math::Vec2>(...)` block in `register_builtin_types()` after the Vec3 registration (or after Vec4, maintaining logical ordering). |

### Files NOT modified

| File | Rationale |
|---|---|
| `src/editor/inspector_editors.cpp` | Vec2 editor already registered — no change needed. |
| `src/engine/math/vec2.h` | Vec2 struct is complete — no change needed. |
| `src/engine/math/math.h` | Vec2 is already included via existing includes — no change needed. |
| All other files in `src/engine/scene/component_registry/` | No changes — the TypeRegistry, ComponentInfo, Property, etc. APIs are unchanged. |

## Document updates

| File | Action | Rationale |
|---|---|---|
| `docs/wiki/domain/glossary.md` — Component registration terms — TypeRegistry entry | Update "Eight built-in types" / "Nine built-in types" to reflect that Vec2 is now a built-in type, increasing the count. | Vec2 is being added alongside float, int32_t, bool, std::string, Vec3, Vec4, Quat, and std::shared_ptr<Model>. |
| `docs/wiki/editor/editor-panels.md` — Inspector Property Editors table | No change needed — Vec2 is already listed as a built-in editor. The table already shows "9 built-in inspector editors" including Vec2. | Vec2 is already tracked in the editor documentation. |
