# IMPL-2026-06-Vec2TypeRegistry — Vec2 TypeRegistry Registration

## Source spec

`.specs/sprint-2026-06/vec2-type-registry/spec.md`

## Goal

Add `math::Vec2` to the engine's `TypeRegistry` so it can be used as a component property type with full YAML serialization (`[x, y]` flow sequence + legacy `{x, y}` mapping), string conversion (`to_string`/`from_string` in `"(x, y)"` format), and no-op validation. This involves creating `src/engine/math/vec2_yaml.h` (YAML convert specialization following the `vec3_yaml.h` pattern for 2 components), modifying `register_all_components.cpp` to include the new header and register the type, and adding Catch2 tests for YAML roundtrip, string roundtrip, and edge/error cases.

## Non-goals

- No change to `src/engine/math/vec2.h` — the Vec2 struct remains unchanged.
- No change to `src/editor/inspector_editors.cpp` — Vec2 already has an editor registered in `InspectorTypeEditorRegistry`.
- No change to existing YAML converters (`vec3_yaml.h`, `vec4_yaml.h`, `quat_yaml.h`, `color_yaml.h`).
- No change to the `TypeRegistry` API, `TypeInfo` struct, or `SerializationContext` — the existing callback pattern is used without modification.
- No migration of existing component properties — no existing component currently uses Vec2 as a property type.
- No change to `src/engine/scene/component_registry/register_all_components.h` — the comment about built-in types is not updated (Vec2 is not a new type category, it completes the vector family).

## Relevant ADRs

- **ADR-028** (`docs/adr/ADR-028-component-type-registry.md`): Defines the TypeRegistry pattern with five callbacks per type. The Vec2 registration must follow the same lambda pattern as Vec3/Vec4.
- **ADR-016** (`docs/adr/ADR-016-yaml-cpp-dependency.md`): yaml-cpp is the project's YAML library. The `vec2_yaml.h` header must only use yaml-cpp types.

## Files to inspect

- `src/engine/math/vec3_yaml.h` — Exact pattern for YAML convert specialization (flow sequence `[x, y, z]` + legacy mapping `{x, y, z}`).
- `src/engine/math/vec2.h` — Vec2 struct definition (has `x`, `y` public float members, namespace `buddd::engine::math`).
- `src/engine/scene/component_registry/register_all_components.cpp` — Existing TypeRegistry registrations (Vec3 lines 114-157, Vec4 lines 159-209). Must see include order and registration pattern.
- `tests/engine/component_registry_tests.cpp` — Existing YAML convert test sections (YAML_CONVERT_VEC3 at line 873, YAML_CONVERT_VEC4 at line 902, YAML_CONVERT_QUAT at line 935). Must see test pattern and includes.
- `src/engine/math/vec4_yaml.h` — Confirm 4-component pattern (for reference).
- `src/engine/scene/component_registry/register_all_components.h` — Confirm no comment change needed (header comment does not mention specific built-in type count for vector types).

## Files allowed to change

1. `src/engine/math/vec2_yaml.h` — **Create.** YAML `convert<Vec2>` specialization (header-only).
2. `src/engine/scene/component_registry/register_all_components.cpp` — **Edit:** Add `#include "math/vec2_yaml.h"`, add `TypeRegistry::register_type<math::Vec2>(...)` block.
3. `tests/engine/component_registry_tests.cpp` — **Edit:** Add `#include "math/vec2_yaml.h"`, add test sections for Vec2 YAML convert and invalid input rejection.

## Files forbidden to change

- `src/engine/math/vec2.h` — Vec2 struct is complete.
- `src/engine/math/vec3_yaml.h`, `src/engine/math/vec4_yaml.h`, `src/engine/math/quat_yaml.h`, `src/engine/math/color_yaml.h` — Existing YAML converters unchanged.
- `src/editor/inspector_editors.cpp` — Vec2 editor already registered.
- Any test file other than `tests/engine/component_registry_tests.cpp`.
- Any `.yaml` scene files, demo source files, or wiki documentation.
- `src/engine/scene/component_registry/register_all_components.h` — No changes to header comments.

## Existing conventions to follow

1. **YAML convert specialization pattern**: Separate `*_yaml.h` header with `YAML::convert<T>` template specialization in `namespace YAML`. Encode: flow sequence with `YAML::EmitterStyle::Flow`. Decode: try/catch block, sequence format check `IsSequence() && size() == N`, legacy mapping format check `IsMap() && node["x"] && node["y"]`, return `bool`.
2. **Trailing return types**: `auto method() noexcept -> ReturnType` style throughout.
3. **Namespace**: All code in `namespace buddd::engine::math` (Vec2 struct) and `namespace YAML` (convert specialization).
4. **Include ordering**: Within `register_all_components.cpp`, YAML includes are grouped as `"math/vec3_yaml.h"`, `"math/vec4_yaml.h"`, `"math/quat_yaml.h"`, `"math/color_yaml.h"`. New `"math/vec2_yaml.h"` goes after `"math/vec3_yaml.h"` (before `"math/vec4_yaml.h"`), keeping Vec2 adjacent to the other vector-type YAML includes.
5. **Test naming**: `TEST_CASE("YAML_CONVERT_VEC3", "[component-registry]")` pattern. Test sections use ALL_CAPS names with underscores.
6. **Test tolerance**: `Approx(value).margin(1e-5f)` for float comparisons.
7. **Register comment headers**: Each registration block prefixed with `// ── math::Vec3 ──` style comment.
8. **Error message format**: `"Vec2: failed to decode YAML node (expected mapping with x, y)"` and `"Vec2: cannot parse '%s' (expected format '(x, y)')"` using `'" + s + "'` string concatenation pattern.
9. **TypeRegistry lambda pattern**: All five callbacks use `const Type& v, const SerializationContext&` parameters. YAML encode/delegate pattern followed for all vector types.

## Required implementation behavior

### 1. `src/engine/math/vec2_yaml.h` — YAML convert specialization (NEW file)

**File content** — must be identical in style to `vec3_yaml.h` but with 2 components:

```cpp
#pragma once

#include "math/vec2.h"
#include <yaml-cpp/yaml.h>

namespace YAML {

template<>
struct convert<buddd::engine::math::Vec2> {
    static auto encode(const buddd::engine::math::Vec2& v) -> Node {
        Node node;
        node.SetStyle(YAML::EmitterStyle::Flow);
        node.push_back(v.x);
        node.push_back(v.y);
        return node;
    }

    static auto decode(const Node& node, buddd::engine::math::Vec2& v) -> bool {
        try {
            // Sequence format: [x, y]
            if (node.IsSequence() && node.size() == 2) {
                v.x = node[0].as<float>();
                v.y = node[1].as<float>();
                return true;
            }
            // Legacy mapping format: {x: , y: }
            if (node.IsMap() && node["x"] && node["y"]) {
                v.x = node["x"].as<float>();
                v.y = node["y"].as<float>();
                return true;
            }
            return false;
        } catch (...) {
            return false;
        }
    }
};

} // namespace YAML
```

**Key behaviors:**
- Encode produces a flow sequence `[x, y]` with `YAML::EmitterStyle::Flow`.
- Decode accepts two-element sequence `[x, y]` — returns `true` on success.
- Decode accepts legacy mapping `{x: value, y: value}` — returns `true` on success.
- Decode returns `false` for: sequences with size != 2, mappings missing `x` or `y`, non-sequence/non-map nodes, nodes with non-float elements (exception caught by try-catch).
- Legacy mapping with extra keys (e.g., `{x: 1, y: 2, z: 3}`) still decodes successfully (only `x` and `y` are checked) — matching Vec3/Vec4 behavior.

### 2. `src/engine/scene/component_registry/register_all_components.cpp` — Registration

**Include change (line 14):** Add `#include "math/vec2_yaml.h"` after `#include "math/vec3_yaml.h"` (line 14), before `#include "math/vec4_yaml.h"` (line 15).

**Registration block:** Add after the Vec4 registration (after line 209, before the Quat registration at line 211). The block must be:

```cpp
    // ── math::Vec2 ──
    TypeRegistry::register_type<math::Vec2>({
        .yaml_encode = [](const math::Vec2& v, const SerializationContext&) -> YAML::Node {
            return YAML::convert<math::Vec2>::encode(v);
        },
        .yaml_decode = [](const YAML::Node& n, const SerializationContext&) -> Result<math::Vec2> {
            math::Vec2 v;
            if (!YAML::convert<math::Vec2>::decode(n, v)) {
                return make_error(Error::Category::InvalidArgument,
                    "Vec2: failed to decode YAML node (expected mapping with x, y)");
            }
            return v;
        },
        .to_string = [](const math::Vec2& v, const SerializationContext&) -> std::string {
            return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
        },
        .from_string = [](const std::string& s, const SerializationContext&) -> Result<math::Vec2> {
            if (s.size() < 2 || s.front() != '(' || s.back() != ')') {
                return make_error(Error::Category::InvalidArgument,
                    "Vec2: cannot parse '" + s + "' (expected format '(x, y)')");
            }
            auto inner = s.substr(1, s.size() - 2);
            float x, y;
            auto [px, ex] = std::from_chars(inner.data(), inner.data() + inner.size(), x);
            if (ex != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "Vec2: cannot parse '" + s + "' (expected format '(x, y)')");
            }
            while (px < inner.data() + inner.size() && (*px == ' ' || *px == ',')) ++px;
            auto [py, ey] = std::from_chars(px, inner.data() + inner.size(), y);
            if (ey != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "Vec2: cannot parse '" + s + "' (expected format '(x, y)')");
            }
            return math::Vec2{x, y};
        },
        .validate = [](const math::Vec2&, const SerializationContext&) -> Result<void> { return {}; }
    });
```

**Key behaviors:**
- `.yaml_encode` delegates directly to `YAML::convert<math::Vec2>::encode(v)`.
- `.yaml_decode` delegates to `YAML::convert<math::Vec2>::decode(n, v)`; on failure, returns error with message `"Vec2: failed to decode YAML node (expected mapping with x, y)"`.
- `.to_string` returns `"(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")"` — exactly the Vec3 pattern minus `.z`.
- `.from_string` follows the Vec3 pattern exactly: checks `s.front() != '('` and `s.back() != ')'`, strips outer parens, parses two floats with `std::from_chars` interleaved with comma/space skipping. Returns error with message `"Vec2: cannot parse '%s' (expected format '(x, y)')"` on malformed input (using `'" + s + "'` concatenation). Does NOT check for trailing content after the last `from_chars` — matches Vec3 behavior exactly. For example, `"(1.5, -3.0, 4.0)"` silently returns `Vec2{1.5, -3.0}` because the extra `, 4.0` is not validated (no end-of-string check after the second component parse).
- `.validate` returns `{}` (no-op, matching all other vector types).

### 3. `tests/engine/component_registry_tests.cpp` — Tests

**Include change (line 21-24):** Add `#include "math/vec2_yaml.h"` after `#include "math/vec3_yaml.h"`.

**Test section placement:** Add after the `YAML_CONVERT_QUAT` test section (after line 966), before the error cases separator (line 968).

**Test section ``YAML_CONVERT_VEC2``** — following the exact pattern of `YAML_CONVERT_VEC3` but for 2 components:

```cpp
TEST_CASE("YAML_CONVERT_VEC2", "[component-registry]") {
    using namespace YAML;

    math::Vec2 original{1.0f, 2.0f};
    Node node = convert<math::Vec2>::encode(original);
    REQUIRE(node.IsSequence());
    REQUIRE(node.size() == 2);
    REQUIRE(node[0].as<float>() == Approx(1.0f).margin(1e-5f));  // x
    REQUIRE(node[1].as<float>() == Approx(2.0f).margin(1e-5f));  // y

    // Also accept legacy mapping format
    math::Vec2 decoded;
    YAML::Node legacy_map;
    legacy_map["x"] = 1.0f;
    legacy_map["y"] = 2.0f;
    REQUIRE(convert<math::Vec2>::decode(legacy_map, decoded));
    REQUIRE(decoded.x == Approx(original.x).margin(1e-5f));
    REQUIRE(decoded.y == Approx(original.y).margin(1e-5f));

    // Also decode sequence format
    REQUIRE(convert<math::Vec2>::decode(node, decoded));
    REQUIRE(decoded.x == Approx(original.x).margin(1e-5f));
    REQUIRE(decoded.y == Approx(original.y).margin(1e-5f));

    // Multi-value roundtrip: encode → decode preserves Vec2 (AC-018)
    for (auto& test_vec : {
             math::Vec2{0.0f, 0.0f},          // zero
             math::Vec2{-1.5f, -3.0f},         // negative
             math::Vec2{-1.0f, 2.5f},          // mixed signs
             math::Vec2{3.14159f, -2.71828f}   // with PI and e
         }) {
        Node encoded = convert<math::Vec2>::encode(test_vec);
        math::Vec2 rt{};
        REQUIRE(convert<math::Vec2>::decode(encoded, rt));
        REQUIRE(rt.x == Approx(test_vec.x).margin(1e-5f));
        REQUIRE(rt.y == Approx(test_vec.y).margin(1e-5f));
    }
}
```

**Test section ``YAML_CONVERT_VEC2_REJECTS_INVALID``** — new test section for invalid input rejection:

```cpp
TEST_CASE("YAML_CONVERT_VEC2_REJECTS_INVALID", "[component-registry]") {
    using namespace YAML;

    math::Vec2 decoded;

    // Single-element sequence [x] → returns false
    YAML::Node single_seq;
    single_seq.push_back(1.0f);
    REQUIRE_FALSE(convert<math::Vec2>::decode(single_seq, decoded));

    // Three-element sequence [x, y, z] → returns false
    YAML::Node triple_seq;
    triple_seq.push_back(1.0f);
    triple_seq.push_back(2.0f);
    triple_seq.push_back(3.0f);
    REQUIRE_FALSE(convert<math::Vec2>::decode(triple_seq, decoded));

    // Incomplete mapping {x: 1.0} → returns false
    YAML::Node incomplete_map;
    incomplete_map["x"] = 1.0f;
    REQUIRE_FALSE(convert<math::Vec2>::decode(incomplete_map, decoded));

    // Scalar node → returns false
    YAML::Node scalar = YAML::Node("hello");
    REQUIRE_FALSE(convert<math::Vec2>::decode(scalar, decoded));

    // Non-numeric element [1.0, "abc"] → returns false (AC-009)
    YAML::Node non_numeric;
    non_numeric.push_back(1.0f);
    non_numeric.push_back("abc");
    REQUIRE_FALSE(convert<math::Vec2>::decode(non_numeric, decoded));
}
```

**Test section ``VEC2_TYPE_REGISTRY``** — tests TypeRegistry integration (to_string format, from_string success/error, validate no-op, string roundtrip). Covers AC-013, AC-014, AC-015, AC-016, AC-019:

```cpp
TEST_CASE("VEC2_TYPE_REGISTRY", "[component-registry]") {
    register_builtin_types();
    TestEngine test_engine;
    SerializationContext ctx{test_engine.mock_assets};

    // AC-013: to_string format
    auto str = TypeRegistry::to_string<math::Vec2>({1.5f, -3.0f}, ctx);
    REQUIRE(str == "(1.500000, -3.000000)");

    // AC-014: from_string success
    auto result = TypeRegistry::from_string<math::Vec2>("(1.5, -3.0)", ctx);
    REQUIRE(result.has_value());
    REQUIRE(result->x == Approx(1.5f).margin(1e-5f));
    REQUIRE(result->y == Approx(-3.0f).margin(1e-5f));

    // AC-015: from_string error cases (malformed)
    auto r1 = TypeRegistry::from_string<math::Vec2>("hello", ctx);
    REQUIRE_FALSE(r1.has_value());
    auto r2 = TypeRegistry::from_string<math::Vec2>("(1.5)", ctx);
    REQUIRE_FALSE(r2.has_value());

    // AC-016: validate is no-op
    auto valid = TypeRegistry::validate<math::Vec2>({1.0f, 2.0f}, ctx);
    REQUIRE(valid.has_value());

    // AC-019: string roundtrip
    auto rt = TypeRegistry::from_string<math::Vec2>(
        TypeRegistry::to_string<math::Vec2>({1.5f, -3.0f}, ctx), ctx);
    REQUIRE(rt.has_value());
    REQUIRE(rt->x == Approx(1.5f).margin(1e-5f));
    REQUIRE(rt->y == Approx(-3.0f).margin(1e-5f));
}
```

**Note:** Use `YAML::Node` construction matching the existing test conventions (building nodes inline with `push_back`, `operator[]`, etc., rather than YAML string parsing).

## Required tests

### Unit tests (in `tests/engine/component_registry_tests.cpp`)

All tests must use `Catch2` with `Catch::Approx` with a tolerance of `1e-5f` margin.

| Test section | Spec AC | Description |
|---|---|---|
| `YAML_CONVERT_VEC2` | AC-002, AC-003, AC-004, AC-018 | Encode Vec2{1,2} → sequence [1,2] of size 2 with correct values. Decode mapping {x:1, y:2} → Vec2{1,2}. Decode sequence [1,2] → Vec2{1,2}. Multi-value roundtrip (zero, negative, mixed signs). |
| `YAML_CONVERT_VEC2_REJECTS_INVALID` | AC-005, AC-006, AC-007, AC-008, AC-009 | Single-element [x] → false. Three-element [x,y,z] → false. Incomplete mapping {x:1} → false. Scalar node "hello" → false. Non-numeric element [1,"abc"] → false. |
| `VEC2_TYPE_REGISTRY` | AC-013, AC-014, AC-015, AC-016, AC-019 | `to_string` format `"(1.500000, -3.000000)"`. `from_string("(1.5, -3.0)")` → Vec2{1.5,-3.0}. `from_string` error cases ("hello", "(1.5)") → error. `validate` returns success. String roundtrip (`from_string(to_string(v))` preserves value). |
| Existing `YAML_CONVERT_VEC3` (unchanged) | — | Must continue to pass — Vec3 YAML is not modified. |
| Existing `YAML_CONVERT_VEC4` (unchanged) | — | Must continue to pass — Vec4 YAML is not modified. |
| Existing `YAML_CONVERT_QUAT` (unchanged) | — | Must continue to pass — Quat YAML is not modified. |

### Integration / E2E verification

- **Build verification**: `cmake --build --preset debug` must complete with zero new compiler warnings from `src/engine/math/`, `src/engine/scene/component_registry/`, and `tests/engine/`.
- **Test suite pass**: Run `buddd_tests` and verify all existing tests pass unchanged and the new Vec2 test sections pass.
- **Zero regressions**: All existing component registration tests (serialization roundtrip, deserialize unknown key, type mismatch, out of range, factory creation) continue to pass.

## Edge cases

| Edge case | Expected behavior |
|---|---|
| **Vec2 with zero values** `{0.0, 0.0}` | Encodes to `[0.0, 0.0]`. Decodes back correctly. |
| **Vec2 with negative values** `{-1.5, -3.0}` | Encodes to `[-1.5, -3.0]`. Decodes back correctly. |
| **Vec2 with NaN/Inf values** | Encodes/decodes via yaml-cpp (IEEE 754). Non-numeric YAML input returns false. |
| **Very large float values** (e.g., `{1e20, -1e20}`) | Encoded/decoded with full yaml-cpp precision. |
| **Whitespace in string format** | `from_string("( 1.5 , -3.0 )")` — `std::from_chars` skips leading whitespace; comma/space skip loop handles delimiters (matching Vec3). |
| **Extra trailing content in string** | `from_string("(1.5, -3.0) extra")` — initial `s.back() != ')'` check catches this (since last char is `l` not `)`). `from_string("(1.5, -3.0)")` works correctly. Vec2 does NOT add a trailing content check (matching Vec3 behavior). |
| **Legacy mapping with extra keys** `{x: 1, y: 2, z: 3}` | Decodes successfully as Vec2{1,2} — extra keys ignored (matching Vec3/Vec4 behavior). |
| **Empty sequence `[]`** | `node.IsSequence() && node.size() == 2` fails → returns false. |
| **Null/undefined YAML node** | Try-catch catches `YAML::Exception` → returns false. |
| **from_string with single element** `"(1.5)"` | Second `std::from_chars` fails → error returned. |
| **from_string with three elements** `"(1.5, -3.0, 4.0)"` | Second `std::from_chars` succeeds but following check... Actually, the Vec3 pattern does NOT check trailing content, so `"(1.5, -3.0)"` parses correctly. `"(1.5, -3.0, 4.0)"` would parse `x=1.5`, skip to y, parse `y=-3.0`, and return Vec2{1.5, -3.0} — the `, 4.0` is silently ignored because the Vec3 pattern does not validate that the last `from_chars` consumed all input. This matches Vec3 behavior exactly. |

## Security impact

None. Vec2 is a pure data type with no authentication, authorization, or sensitive data exposure. YAML decode errors are surfaced through existing error handling (returns false from `YAML::convert<Vec2>::decode`; caller in TypeRegistry wraps with error message). String parse failures return `Error` with descriptive message. No input validation beyond type checks and format requirements.

## Data and migration impact

None. No existing component properties use Vec2. No schema changes to existing scene files. Vec2 serialization is new and unused until a component property of type Vec2 is registered in a future change.

## API compatibility impact

None. The `math::Vec2` struct is unchanged. The `TypeRegistry` API is unchanged (same `register_type<T>()` call). The `YAML::convert<Vec2>` specialization is new code in a new header — it does not change any existing API. No existing component or property uses Vec2, so there are no backward-compatibility concerns.

## Documentation impact

- **README**: No changes needed.
- **Wiki pages**: The wiki-agent will handle updates (noted in spec: docs/wiki/domain/glossary.md — TypeRegistry entry should reflect that Vec2 is now a built-in type).
- **Other specs**: None.

## ADR impact

No new ADR required. The implementation follows existing ADR-028 (TypeRegistry registration pattern), ADR-016 (yaml-cpp dependency), and existing conventions from vec3/vec4 serialization. The Vec2 type was already established in ADR-002 (GLM wrapper math types).

## Done criteria

- [ ] `src/engine/math/vec2_yaml.h` exists with `YAML::convert<buddd::engine::math::Vec2>` template specialization supporting flow sequence encode `[x, y]` and decode of both sequence `[x, y]` and legacy mapping `{x, y}` formats.
- [ ] `src/engine/scene/component_registry/register_all_components.cpp` includes `#include "math/vec2_yaml.h"` after `"math/vec3_yaml.h"`.
- [ ] `src/engine/scene/component_registry/register_all_components.cpp` has `TypeRegistry::register_type<math::Vec2>(...)` block after Vec4 registration, with all five callbacks (`yaml_encode`, `yaml_decode`, `to_string`, `from_string`, `validate`) following the Vec3 pattern.
- [ ] `.yaml_decode` error message is exactly `"Vec2: failed to decode YAML node (expected mapping with x, y)"`.
- [ ] `.from_string` error message format is exactly `"Vec2: cannot parse '...' (expected format '(x, y)')"`.
- [ ] `tests/engine/component_registry_tests.cpp` includes `#include "math/vec2_yaml.h"`.
- [ ] `YAML_CONVERT_VEC2` test section exists and tests: encode sequence, decode legacy mapping, decode sequence, multi-value roundtrip (zero, negative, mixed signs) — all with `Approx().margin(1e-5f)`.
- [ ] `YAML_CONVERT_VEC2_REJECTS_INVALID` test section exists and tests: single-element sequence, three-element sequence, incomplete mapping, scalar node, non-numeric element — all returning `false`.
- [ ] `VEC2_TYPE_REGISTRY` test section exists and tests: `to_string` format `"(1.500000, -3.000000)"`, `from_string("(1.5, -3.0)")` success, `from_string` error cases ("hello", "(1.5)"), validate no-op, string roundtrip.
- [ ] `cmake --build --preset debug` completes with zero new compiler warnings.
- [ ] `buddd_tests` full suite passes with no regressions.
