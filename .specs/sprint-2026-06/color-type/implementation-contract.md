# IMPL-2026-06-ColorType — Engine Color Type

## Source spec

`.specs/sprint-2026-06/color-type/spec.md`

## Goal

Implement a dedicated `Color` struct wrapping `glm::vec4` via the ADR-002 `reinterpret_cast` pattern, with a full color-specific API (sRGB↔linear, luminance, darken/lighten, blend, HSV, named presets, RGBA32 conversion), YAML serialization (flow sequence `[r,g,b,a]` with backward-compat `[r,g,b]` loading), TypeRegistry registration as the 9th built-in type, PropertyFlags tag system (`.tag("rgb")`) for editor alpha control, migration of light components and `PbrMaterialData::base_color_factor` from `Vec3`/`Vec4` to `Color`, and an ImGui color-picker editor widget.

## Non-goals

- No `Color3` type — only a single RGBA `Color{r,g,b,a}` type.
- No implicit conversion between `Color` and `Vec4` — explicit only via `.to_vec3()`, `.glm()`, and `explicit Color(glm::vec4)`.
- No migration of `Vertex::color` or `LightData::color` — these stay `Vec4` due to GPU layout coupling.
- No automatic sRGB-to-linear conversion — conversion is an explicit method call.
- No rounding of YAML output — full yaml-cpp default precision.
- No editing of existing test files, wiki pages, or ADRs (these are handled by other agents).
- No implementation of component property drawing loop in the editor (future work — only the Color editor registration is done here).

## Relevant ADRs

- **ADR-002** (`docs/adr/ADR-002-glm-wrapper-math.md`): Color follows the exact `reinterpret_cast` wrapper pattern — triple `static_assert`, `.glm()` accessor, header-only, public float members in same layout as `glm::vec4`.
- **ADR-019** (`docs/adr/ADR-019-architecture-boundaries.md`): No GLM headers outside `src/engine/math/`. The `color_yaml.h` header must not include GLM directly (it uses `reinterpret_cast` to access the underlying `glm::vec4`).
- **ADR-028** (`docs/adr/ADR-028-component-type-registry.md`): Color is registered as the 9th built-in type in `TypeRegistry` with `yaml_encode`/`yaml_decode`/`to_string`/`from_string`/`validate` callbacks.

## Files to inspect

- `src/engine/math/vec4.h` — The Vec4 wrapper pattern (triple static_assert, `.glm()`, arithmetic ops, member layout).
- `src/engine/math/vec3.h` — Vec3 wrapper pattern for reference.
- `src/engine/math/quat.h` — Quat wrapper pattern (out-of-body inline implementations).
- `src/engine/math/vec4_yaml.h` — YAML `convert<Vec4>` specialization pattern.
- `src/engine/math/vec3_yaml.h` — YAML `convert<Vec3>` specialization pattern.
- `src/engine/math/math.h` — Umbrella header (current includes).
- `src/engine/scene/component_registry/property.h` — PropertyFlags struct (to add `tags_`).
- `src/engine/scene/component_registry/type_registry.h` — TypeRegistry API (for registration pattern).
- `src/engine/scene/component_registry/register_all_components.cpp` — The `register_builtin_types()` and `register_all_components()` functions.
- `src/engine/scene/component_registry/register_all_components.h` — Header comment mentioning "eight".
- `src/engine/scene/point_light_component.h` — Vec3-based color member/accessor (to migrate).
- `src/engine/scene/point_light_component.cpp` — Constructor and accessor implementations.
- `src/engine/scene/directional_light_component.h` — Same pattern.
- `src/engine/scene/directional_light_component.cpp` — Same pattern.
- `src/engine/scene/spot_light_component.h` — Same pattern.
- `src/engine/scene/spot_light_component.cpp` — Same pattern.
- `src/engine/render/pbr/pbr_material.h` — `PbrMaterialData::base_color_factor` field.
- `src/engine/render/pbr/pbr_material.cpp` — `set_data()` uniform setting.
- `src/engine/render/pbr/material.h` — Abstract `Material::set_uniform` overloads for Vec4.
- `src/engine/asset/model_loader.cpp` — Vec4 assignments for `base_color_factor`.
- `src/engine/render/render_system.cpp` — Light color extraction (`.x`/`.y`/`.z` → `.r`/`.g`/`.b`).
- `src/editor/inspector_editors.h` — `EditorFlags` struct, `InspectorTypeEditorRegistry`.
- `src/editor/inspector_editors.cpp` — Existing editor registrations, `register_builtin_inspector_editors()`.
- `src/engine/render/light_data.h` — Confirm `LightData::color` stays Vec4 (NOT modified).
- `src/engine/render/vertex.h` — Confirm `Vertex::color` stays Vec4 (NOT modified).
- `tests/engine/component_registry_tests.cpp` — Test patterns for serialization roundtrip.
- `tests/engine/math_tests.cpp` — Test patterns and helper functions (`require_approx`, `TOL`).

## Files allowed to change

1. `src/engine/math/color.h` — **Create.** Color struct (header-only).
2. `src/engine/math/color_yaml.h` — **Create.** YAML `convert<Color>` specialization (header-only).
3. `tests/engine/color_tests.cpp` — **Create.** Catch2 unit tests.
4. `tests/engine/component_color_registry_tests.cpp` — **Create.** Integration roundtrip test for Color with TypeRegistry/ComponentRegistry.
5. `src/engine/math/math.h` — **Edit:** Add `#include "color.h"` in alphabetical position after `"quat.h"`.
6. `src/engine/scene/component_registry/property.h` — **Edit:** Add `std::vector<std::string> tags_` member, `.tag()` fluent setter, `.has_tag()` checker to `PropertyFlags`.
7. `src/engine/scene/component_registry/register_all_components.cpp` — **Edit:** Add `#include "math/color_yaml.h"`, register `math::Color` as 9th built-in type, update light component property registrations to use `Color` + `.tag("rgb")`.
8. `src/engine/scene/component_registry/register_all_components.h` — **Edit:** Update "eight built-in types" → "nine built-in types".
9. `src/engine/scene/point_light_component.h` — **Edit:** Change `#include "math/vec3.h"` to `#include "math/color.h"`, change `color_` member type from `Vec3` to `Color`, change `color()` return types from `Vec3&`/`const Vec3&` to `Color&`/`const Color&`.
10. `src/engine/scene/point_light_component.cpp` — **Edit:** Change constructor parameter type `math::Vec3 color` to `math::Color color`, change `color()` return types, change default from `math::Vec3{1.0f, 1.0f, 1.0f}` to `math::Color{1.0f, 1.0f, 1.0f}`.
11. `src/engine/scene/directional_light_component.h` — **Edit:** Same changes as point_light_component.h.
12. `src/engine/scene/directional_light_component.cpp` — **Edit:** Same changes as point_light_component.cpp.
13. `src/engine/scene/spot_light_component.h` — **Edit:** Same changes as point_light_component.h.
14. `src/engine/scene/spot_light_component.cpp` — **Edit:** Same changes as point_light_component.cpp.
15. `src/engine/render/pbr/pbr_material.h` — **Edit:** Add `#include "math/color.h"`, change `base_color_factor` type from `math::Vec4` to `math::Color`.
16. `src/engine/render/pbr/pbr_material.cpp` — **Edit:** Update `set_data()` to convert `Color` to `Vec4` via `math::Vec4{data.base_color_factor.glm()}` when calling `set_uniform("u_base_color_factor", ...)`.
17. `src/engine/asset/model_loader.cpp` — **Edit:** Change two `math::Vec4{...}` assignments to `math::Color{...}`.
18. `src/engine/render/render_system.cpp` — **Edit:** Change `lc.color().x`/`.y`/`.z` to `lc.color().r`/`.g`/`.b` in light collection lambdas (three locations).
19. `src/editor/inspector_editors.h` — **Edit:** Add `std::vector<std::string> tags_` member, `.tag()` setter, `.has_tag()` checker to `EditorFlags`.
20. `src/editor/inspector_editors.cpp` — **Edit:** Add `#include "math/color.h"`, register Color editor with ImGui color picker, update log message from "8" to "9".

## Files forbidden to change

- `src/engine/render/vertex.h` — `Vertex::color` stays `Vec4` (GPU layout coupling).
- `src/engine/render/light_data.h` — `LightData::color` stays `Vec4` (GPU layout coupling).
- `src/engine/render/material.h` — No change; `set_uniform(Vec4)` overload stays as-is, Color is converted at call site.
- `src/engine/math/vec3.h`, `src/engine/math/vec4.h`, `src/engine/math/quat.h`, `src/engine/math/vec2.h`, `src/engine/math/mat4.h` — Existing math types unchanged.
- Any `.yaml` scene files, demo source files, or wiki documentation (handled by other agents).

## Existing conventions to follow

1. **GLM wrapper pattern** (ADR-002): Standard-layout struct, public float members (`r, g, b, a`), `auto glm() noexcept -> glm::vec4&` via `reinterpret_cast`, triple `static_assert` at bottom of struct: `is_standard_layout_v`, `sizeof(T) == sizeof(glm::vec4)`, `is_trivially_copyable_v`.
2. **Header-only**: All Color code must be inline/header-only — no `.cpp` file.
3. **Trailing return types**: `auto method() noexcept -> ReturnType` style throughout.
4. **`noexcept` on all methods**: All Color methods are `noexcept`.
5. **No GLM outside `src/engine/math/`**: `color.h` includes `<glm/vec4.hpp>` but `color_yaml.h` must NOT include GLM (uses only `yaml-cpp/yaml.h` and `math/color.h`).
6. **YAML convert specializations** are in separate `*_yaml.h` headers, use `YAML::EmitterStyle::Flow`, and return `bool` from `decode()`.
7. **TypeRegistry callbacks**: Follow the same lambda pattern as Vec3/Vec4 — `yaml_encode`/`yaml_decode` delegate to `YAML::convert<Color>`, `to_string`/`from_string` use `(r, g, b, a)` format matching Vec4 `(x, y, z, w)` format, `from_string` parsing matches the Vec4 pattern (parentheses + comma-separated values).
8. **Getter/setter naming**: Light component accessors return `Color&` (mutable) and `const Color&` (const) — matching current Vec3 pattern.
9. **PropertyFlags fluent API**: `PropertyFlags{}.min(...).max(...).tag("rgb")` chain.
10. **InspectorTypeEditorWidget pattern**: `TypedInspectorEditor<T>::DrawFn` with `(id, value, flags, ctx) -> bool` signature.

## Required implementation behavior

### 1. `src/engine/math/color.h` — Color struct

**Layout & static_asserts:**
```cpp
struct Color {
    float r, g, b, a;
    // triple static_assert after struct definition
};
static_assert(std::is_standard_layout_v<Color>, "Color must be standard layout");
static_assert(sizeof(Color) == sizeof(glm::vec4), "Color size must match glm::vec4");
static_assert(std::is_trivially_copyable_v<Color>, "Color must be trivially copyable");
```

**Constructors:**
- `Color() noexcept` — zero-initializes all components (r=g=b=a=0.0f).
- `Color(float r_, float g_, float b_) noexcept` — alpha defaults to 1.0f.
- `Color(float r_, float g_, float b_, float a_) noexcept` — all four components.
- `explicit Color(const glm::vec4& v) noexcept` — from GLM vec4.

**Access:**
- `auto operator[](int i) const noexcept -> float` — delegates to `reinterpret_cast<const glm::vec4&>(*this)[i]`.
- `auto operator[](int i) noexcept -> float&` — mutable index access.

**GLM interop:**
- `auto glm() noexcept -> glm::vec4&` — `reinterpret_cast<glm::vec4&>(*this)`.
- `auto glm() const noexcept -> const glm::vec4&` — const version.

**Arithmetic operators (component-wise, matching Vec4 pattern):**
- `friend auto operator+(Color, Color) -> Color`
- `friend auto operator-(Color, Color) -> Color`
- `friend auto operator*(Color, Color) -> Color`
- `friend auto operator/(Color, Color) -> Color`
- `friend auto operator*(Color, float) -> Color`
- `friend auto operator*(float, Color) -> Color`
- `friend auto operator/(Color, float) -> Color`
- `auto operator-() const -> Color` — unary negation.
- `auto operator+=(Color) -> Color&`
- `auto operator-=(Color) -> Color&`
- `auto operator*=(Color) -> Color&`
- `auto operator/=(Color) -> Color&`
- `auto operator*=(float) -> Color&`
- `auto operator/=(float) -> Color&`

**Comparison:**
- `friend auto operator==(Color, Color) -> bool` — exact float comparison.
- `friend auto operator!=(Color, Color) -> bool`

**Color-specific operations:**
- `auto to_vec3() const noexcept -> Vec3` — returns `Vec3{r, g, b}` (discards alpha).
- `auto to_linear() const noexcept -> Color` — applies sRGB→linear transfer function to r, g, b; alpha unchanged.
- `auto to_srgb() const noexcept -> Color` — applies linear→sRGB transfer function to r, g, b; alpha unchanged.
  - sRGB transfer function (IEC 61966-2-1): linear threshold at `0.04045` (for `to_srgb`) and `0.0031308` (for `to_linear`). Gamma = 2.4, linear segment slope = 12.92.
- `auto luminance() const noexcept -> float` — `0.2126f * r + 0.7152f * g + 0.0722f * b`.
- `auto darkened(float factor) const noexcept -> Color` — `r * (1-factor), g * (1-factor), b * (1-factor), a`. Godot-style multiply.
- `auto lightened(float factor) const noexcept -> Color` — `r + (1-r) * factor, g + (1-g) * factor, b + (1-b) * factor, a`. Godot-style.
- `static auto blend(Color fg, Color bg) noexcept -> Color` — standard over-blend.
  - `out.a = fg.a + bg.a * (1 - fg.a)`
  - If `out.a == 0`: `out.rgb = {0,0,0}` (avoid division by zero).
  - Else: `out.rgb = (fg.rgb * fg.a + bg.rgb * bg.a * (1 - fg.a)) / out.a`
- `auto to_hsv() const noexcept -> Vec3` — returns `Vec3{h, s, v}` with H in `[0, 1)` where 0=red, 1/3=green, 2/3=blue; S and V in `[0, 1]`. Standard RGB→HSV algorithm.
- `static auto from_hsv(float h, float s, float v) noexcept -> Color` — HSV→RGB, alpha = 1.0. Standard HSV→RGB algorithm.
- `auto to_rgba32() const noexcept -> std::array<uint8_t, 4>` — clamps each channel to `[0, 1]`, multiplies by 255, rounds to nearest integer.
- `static auto from_rgba32(std::array<uint8_t, 4> rgba) noexcept -> Color` — divides each uint8 by 255.0f.

**Named color presets (static, return by value, alpha = 1.0f):**
- `static constexpr auto white() noexcept -> Color { return Color{1.0f, 1.0f, 1.0f}; }`
- `static constexpr auto black() noexcept -> Color { return Color{0.0f, 0.0f, 0.0f}; }`
- `static constexpr auto red() noexcept -> Color { return Color{1.0f, 0.0f, 0.0f}; }`
- `static constexpr auto green() noexcept -> Color { return Color{0.0f, 1.0f, 0.0f}; }`
- `static constexpr auto blue() noexcept -> Color { return Color{0.0f, 0.0f, 1.0f}; }`
- `static constexpr auto yellow() noexcept -> Color { return Color{1.0f, 1.0f, 0.0f}; }`
- `static constexpr auto cyan() noexcept -> Color { return Color{0.0f, 1.0f, 1.0f}; }`
- `static constexpr auto magenta() noexcept -> Color { return Color{1.0f, 0.0f, 1.0f}; }`

**Additional constants (matching Vec4 pattern):**
- `static constexpr auto zero() noexcept -> Color { return Color{0.0f, 0.0f, 0.0f, 0.0f}; }`
- `static constexpr auto one() noexcept -> Color { return Color{1.0f, 1.0f, 1.0f, 1.0f}; }`

**Includes:** `<glm/vec4.hpp>`, `<array>`, `<type_traits>`, `<cstdint>`, `"vec3.h"` (for `to_vec3()` return type).

### 2. `src/engine/math/color_yaml.h` — YAML convert

**Format:**
- Encode: flow sequence `[r, g, b, a]` with `YAML::EmitterStyle::Flow`.
- Decode: accepts flow sequences of size 3 (`[r, g, b]` → alpha defaults to 1.0f) or 4 (`[r, g, b, a]`). Use `try`/`catch` to handle non-float elements — `catch(...) { return false; }`.
- Does NOT support legacy mapping format `{r: , g: , b: , a: }` (unlike Vec3/Vec4).
- Returns `false` for: non-sequence nodes, sequences with size < 3 or > 4, nodes with non-float elements.

**Includes:** `"math/color.h"`, `<yaml-cpp/yaml.h>`. No `<glm>` headers.

### 3. PropertyFlags changes (`property.h`)

Add to `PropertyFlags` struct (after `enum_choices`):
```cpp
std::vector<std::string> tags_;

auto tag(std::string t) noexcept -> PropertyFlags& {
    tags_.push_back(std::move(t));
    return *this;
}
auto has_tag(const std::string& t) const noexcept -> bool {
    return std::find(tags_.begin(), tags_.end(), t) != tags_.end();
}
```

Note: `tags_` member added after `enum_choices` to minimize risk of breaking positional aggregate initialization (the existing 4 fields already mean no call site uses positional initialization of all fields). Add `#include <algorithm>` for `std::find`.

### 4. EditorFlags changes (`inspector_editors.h`)

Add to `EditorFlags` struct (after `step_value`):
```cpp
std::vector<std::string> tags_;

auto tag(std::string t) noexcept -> EditorFlags& {
    tags_.push_back(std::move(t));
    return *this;
}
auto has_tag(const std::string& t) const noexcept -> bool {
    return std::find(tags_.begin(), tags_.end(), t) != tags_.end();
}
```

Add `#include <vector>` and `#include <algorithm>` to the header if not already present.

### 5. TypeRegistry registration (`register_all_components.cpp`)

Add as 9th built-in type (after `Quat`, before `std::shared_ptr<Model>`):
```cpp
// ── math::Color ──
TypeRegistry::register_type<math::Color>({
    .yaml_encode = [](const math::Color& v, const SerializationContext&) -> YAML::Node {
        return YAML::convert<math::Color>::encode(v);
    },
    .yaml_decode = [](const YAML::Node& n, const SerializationContext&) -> Result<math::Color> {
        math::Color v;
        if (!YAML::convert<math::Color>::decode(n, v)) {
            return make_error(Error::Category::InvalidArgument,
                "Color: failed to decode YAML node (expected [r, g, b] or [r, g, b, a])");
        }
        return v;
    },
    .to_string = [](const math::Color& v, const SerializationContext&) -> std::string {
        return "(" + std::to_string(v.r) + ", " + std::to_string(v.g) + ", "
               + std::to_string(v.b) + ", " + std::to_string(v.a) + ")";
    },
    .from_string = [](const std::string& s, const SerializationContext&) -> Result<math::Color> {
        // Parse "(r, g, b, a)" format — same pattern as Vec4::from_string but with r/g/b/a.
        if (s.size() < 2 || s.front() != '(' || s.back() != ')') {
            return make_error(Error::Category::InvalidArgument,
                "Color: cannot parse '" + s + "' (expected format '(r, g, b, a)')");
        }
        auto inner = s.substr(1, s.size() - 2);
        float r, g, b, a;
        auto [pr, er] = std::from_chars(inner.data(), inner.data() + inner.size(), r);
        if (er != std::errc()) {
            return make_error(Error::Category::InvalidArgument,
                "Color: cannot parse '" + s + "' (expected format '(r, g, b, a)')");
        }
        while (pr < inner.data() + inner.size() && (*pr == ' ' || *pr == ',')) ++pr;
        auto [pg, eg] = std::from_chars(pr, inner.data() + inner.size(), g);
        if (eg != std::errc()) {
            return make_error(Error::Category::InvalidArgument,
                "Color: cannot parse '" + s + "' (expected format '(r, g, b, a)')");
        }
        while (pg < inner.data() + inner.size() && (*pg == ' ' || *pg == ',')) ++pg;
        auto [pb, eb] = std::from_chars(pg, inner.data() + inner.size(), b);
        if (eb != std::errc()) {
            return make_error(Error::Category::InvalidArgument,
                "Color: cannot parse '" + s + "' (expected format '(r, g, b, a)')");
        }
        while (pb < inner.data() + inner.size() && (*pb == ' ' || *pb == ',')) ++pb;
        auto [pa, ea] = std::from_chars(pb, inner.data() + inner.size(), a);
        if (ea != std::errc()) {
            return make_error(Error::Category::InvalidArgument,
                "Color: cannot parse '" + s + "' (expected format '(r, g, b, a)')");
        }
        return math::Color{r, g, b, a};
    },
    .validate = [](const math::Color&, const SerializationContext&) -> Result<void> { return {}; }
});
```

Add `#include "math/color_yaml.h"` with the other yaml includes.

### 6. Light component property registration updates

For each light component color property, change from:
```cpp
info.add_property<math::Vec3>("color",
    [](const PointLightComponent& c) -> math::Vec3 { return c.color(); },
    [](PointLightComponent& c, const math::Vec3& v) -> Result<void> { c.color() = v; return {}; }
);
```
To:
```cpp
info.add_property<math::Color>("color",
    [](const PointLightComponent& c) -> math::Color { return c.color(); },
    [](PointLightComponent& c, const math::Color& v) -> Result<void> { c.color() = v; return {}; },
    PropertyFlags{}.tag("rgb")
);
```

Do this for all three light components (point_light, directional_light, spot_light).

### 7. Light component migration (headers and cpps)

- Change `#include "math/vec3.h"` → `#include "math/color.h"` in each header.
- Change constructor parameter `math::Vec3 color` → `math::Color color`.
- Change member `math::Vec3 color_{1.0f, 1.0f, 1.0f}` → `math::Color color_{1.0f, 1.0f, 1.0f}`.
- Change return type of `color()` from `math::Vec3&`/`const math::Vec3&` to `math::Color&`/`const math::Color&`.
- In `.cpp` files: update constructor signature and implementation to use `math::Color`.

### 8. PBR material migration

- `pbr_material.h`: Add `#include "math/color.h"`, change `math::Vec4 base_color_factor{1.0f, 1.0f, 1.0f, 1.0f}` → `math::Color base_color_factor{1.0f, 1.0f, 1.0f, 1.0f}`.
- `pbr_material.cpp` line 108: Change `(void)impl_->inner->set_uniform("u_base_color_factor", data.base_color_factor);` to explicit conversion: `(void)impl_->inner->set_uniform("u_base_color_factor", math::Vec4{data.base_color_factor.glm()});`
- `model_loader.cpp` lines 462 and 545: Change `data.base_color_factor = math::Vec4{...}` to `data.base_color_factor = math::Color{...}` (same values).

### 9. Render system light extraction

In `render_system.cpp`, three locations where light color components are read from component accessors:
- Line 55-57: `lc.color().x/lc.color().y/lc.color().z` → `lc.color().r/lc.color().g/lc.color().b`
- Line 73-75: Same change.
- Line 95-97: Same change.

### 10. Editor Color widget registration

In `register_builtin_inspector_editors()` in `inspector_editors.cpp`:

```cpp
// math::Color
InspectorTypeEditorRegistry::register_editor<buddd::engine::math::Color>(
    [](const std::string& id, buddd::engine::math::Color& value,
       const EditorFlags& flags,
       const EditorContext& ctx) -> bool {
        float vals[4] = {value.r, value.g, value.b, value.a};
        ImGui::PushID(id.c_str());
        bool changed = false;

        if (flags.has_tag("rgb")) {
            // 3-channel color picker (no alpha)
            changed = ImGui::ColorEdit3("##color", vals,
                ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
            if (changed) {
                value.r = vals[0];
                value.g = vals[1];
                value.b = vals[2];
                // alpha unchanged
            }
        } else {
            // 4-channel color picker (with alpha)
            changed = ImGui::ColorEdit4("##color", vals,
                ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
            if (changed) {
                value.r = vals[0];
                value.g = vals[1];
                value.b = vals[2];
                value.a = vals[3];
            }
        }

        ImGui::PopID();
        if (changed) {
            ctx.editor.mark_dirty();
        }
        return changed;
    }
);
```

Add `#include "math/color.h"` to `inspector_editors.cpp`.
Update log message: `"Registered 8 built-in inspector editors"` → `"Registered 9 built-in inspector editors"`.

## Required tests

### Unit tests (in `tests/engine/color_tests.cpp`)

All tests must use `Catch2` with `Catch::Approx` with a tolerance of `1e-5f`.

| Test | Spec AC | Description |
|------|---------|-------------|
| **Layout checks** | AC-001 | `static_assert` checks: `is_standard_layout_v`, `sizeof(Color) == sizeof(glm::vec4)`, `is_trivially_copyable_v`. |
| **GLM interop** | AC-002 | `.glm()` returns reference that modifies original Color. |
| **Default constructor** | AC-003 | `Color{}` has all components 0.0f. |
| **RGBA/RGB constructors** | AC-004 | `Color{1,0,0}` has a=1.0; `Color{1,0,0,0.5}` has a=0.5. |
| **Arithmetic** | AC-005 | All 7 arithmetic operators produce correct results matching GLM reference. |
| **YAML encode** | AC-006 | `YAML::convert<Color>::encode(Color{0.5,0.75,0.25,1.0})` emits `[0.5, 0.75, 0.25, 1.0]`. |
| **YAML decode 4-element** | AC-007 | Decode `[0.5, 0.75, 0.25, 1.0]` → `Color{0.5, 0.75, 0.25, 1.0}`. |
| **YAML decode 3-element** | AC-007 | Decode `[0.5, 0.75, 0.25]` → `Color{0.5, 0.75, 0.25, 1.0}`. |
| **YAML decode invalid** | AC-008 | Decode wrong-size `[1,2]`, non-sequence, non-numeric → `false`. |
| **YAML roundtrip** | AC-007 | Encode then decode returns original value (within float precision). |
| **to_linear / to_srgb** | AC-009 | Roundtrip `srgb → linear → srgb` approximates identity within 1e-5f. alpha unchanged. |
| **Blend** | AC-010 | `Color::blend({1,0,0,0.5}, {0,1,0,1})` → `{0.5, 0.5, 0.0, 1.0}`. |
| **Luminance** | Story 5 | `Color{0.5, 0.3, 0.8, 1.0}.luminance()` matches `0.2126*0.5 + 0.7152*0.3 + 0.0722*0.8`. |
| **Darken/Lighten** | Story 5 | `darkened(0.5)` = `c * (1-0.5)`; `lightened(0.5)` = `c + (1-c) * 0.5`. |
| **HSV roundtrip** | Story 7 | `from_hsv(to_hsv(c))` ≈ `c` within 1e-5f epsilon. |
| **Named colors** | AC-014 | All 8 named colors have correct RGBA values with alpha=1.0. |
| **RGBA32 roundtrip** | Story 9 | `from_rgba32(to_rgba32(c))` clamped roundtrip. `Color{1, 0.5, 0.25, 1}` → `{255, 127, 63, 255}` → `Color{1, 127/255, 63/255, 1}`. |
| **to_vec3** | Spec | Returns `Vec3{r, g, b}`. |
| **Explicit Vec4 construction** | Non-goal #2 | `Color{glm::vec4(1,0,0,1)}` is valid and explicit. No implicit conversion. |
| **Alpha edge cases** | Edge cases | `blend` with zero alpha produces correct output. HDR values preserved through arithmetic. `to_rgba32` clamps. |
| **Divide by zero** | Edge cases | `/` with zero component produces inf/nan (no guard, matching GLM). |

### Integration / E2E verification

- **Component serialization roundtrip**: In a new separate test file `tests/engine/component_color_registry_tests.cpp`, add a test that creates a light component (e.g., `PointLightComponent`), sets a color, serializes to YAML, deserializes, and verifies the color matches. This confirms the TypeRegistry + YAML pipeline works end-to-end for Color.
- **Lighting visual parity (E2E screenshot)**: Run a demo app with lights (PointLight, DirectionalLight, SpotLight) before and after migration using `--capture` CLI argument. Compare screenshots using vision analysis tool — pixels must be identical within ±1 LSB tolerance.

## Edge cases

| Edge case | Expected behavior |
|-----------|------------------|
| Alpha = 0 (transparent) | `blend` handles `out.a == 0` → output rgb = 0. `to_linear`/`to_srgb` leave alpha unchanged. |
| HDR values (channel > 1.0) | Arithmetic preserves HDR. `to_rgba32()` clamps to [0,1]. `to_srgb`/`to_linear` apply transfer function to HDR values as-is. |
| Negative color values | Arithmetic produces negative values (same as GLM). `to_rgba32()` clamps (negative → 0). |
| Division by zero | `/` with zero component produces inf/nan (same as GLM, no guard). |
| NaN inputs | NaN propagates (same as GLM behavior, no validation). |
| Old YAML mapping format `{r: 1, g: 2, b: 3}` | NOT supported (different from Vec3's legacy format). Only flow sequences `[r,g,b]` and `[r,g,b,a]` accepted. |
| Floating-point precision | YAML roundtrip tolerance: 1e-5f relative error. |
| `darkened(0)` / `lightened(0)` | Identity. |
| `darkened(1)` / `lightened(1)` | Black / white. |
| HSV: H outside [0, 1) | `to_hsv` normalizes to [0, 1). `from_hsv` accepts any float (modulo 1 for H internally). |

## Security impact

None. Color is a pure data type with no authentication, authorization, or sensitive data exposure. YAML decode errors are surfaced through existing error handling (logged at WARN level). No input validation beyond YAML parse error handling.

## Data and migration impact

- **Schema change**: Color serializes as `[r, g, b, a]` flow sequence in YAML. Old light component colors serialized as `Vec3` sequences `[x, y, z]` will load correctly (backward-compatible YAML decode). New saves will produce `[r, g, b, a]` format.
- **Migration of scene files**: Existing scene files with light component colors in Vec3 format will load without errors. Upon re-save, they will be written in the new Color format (4-element). This is a one-time format migration.
- **`PbrMaterialData::base_color_factor`**: Changed from `Vec4` to `Color` in the C++ struct. Serialized format remains `[r, g, b, a]` (same as Vec4 flow sequence format). No schema incompatibility for PBR materials.
- **No data loss**: The backward-compatible YAML decode ensures no data is lost during migration. Alpha defaults to 1.0 for old 3-element values.

## API compatibility impact

- **`PointLightComponent::color()`**: Return type changes from `math::Vec3&` to `math::Color&`. Existing callers using `.x`/`.y`/`.z` must change to `.r`/`.g`/`.b`. Existing callers using `.glm()` continue to work (both Vec3 and Color have `.glm()`). Existing callers that pass the color value to functions expecting `Vec3` will need to use `.to_vec3()` explicitly.
- **Same for `DirectionalLightComponent` and `SpotLightComponent`**.
- **`PbrMaterialData::base_color_factor`**: Type changes from `math::Vec4` to `math::Color`. Existing code that assigns `Vec4` values needs to assign `Color` values instead. Code that passes the field to `set_uniform(Vec4)` needs explicit conversion.
- **No change to `Material::set_uniform` API** — the `Vec4` overload is unchanged. Color is converted at the call site.
- **No change to `LightData::color` or `Vertex::color`** — these remain `Vec4`.
- **`PropertyFlags`**: Adding `tags_` member may break any code using positional brace initialization `PropertyFlags{...}` that assumes exactly 4 fields. This is an accepted risk documented in the spec.

## Documentation impact

- **README**: No changes needed.
- **Wiki pages**: Updated by the wiki-agent. The contract notes these required changes:
  - `docs/wiki/architecture/module-map.md`: TypeRegistry section — change "Eight built-in types" to "Nine built-in types", add `Color` to the list.
  - `docs/wiki/editor/editor-panels.md`: Inspector Property Editors table — update "Color picker + float fields (not yet implemented)" to "Color picker (`ImGui::ColorEdit4`/`ColorEdit3` with tag-based alpha control)".
  - `docs/wiki/domain/glossary.md`: Update TypeRegistry entry from "Eight built-in types" to "Nine built-in types".
- **Other specs**: None.

## ADR impact

No new ADR required. The implementation follows existing ADR-002 (GLM wrapper pattern), ADR-019 (architecture boundaries), and ADR-028 (TypeRegistry). Confirmed compatible.

## Done criteria

- [ ] `src/engine/math/color.h` exists with: triple `static_assert`, `.glm()` accessor, all required constructors, arithmetic operators, color-specific operations (`to_linear`, `to_srgb`, `luminance`, `darkened`, `lightened`, `blend`, `to_hsv`, `from_hsv`, `to_rgba32`, `from_rgba32`, `to_vec3`), named color presets, `zero()`/`one()` constants.
- [ ] `src/engine/math/color_yaml.h` exists with `YAML::convert<Color>` supporting flow sequence encode and decode of both 3-element and 4-element sequences, rejecting invalid inputs.
- [ ] `src/engine/math/math.h` includes `"color.h"`.
- [ ] `src/engine/scene/component_registry/property.h` has `std::vector<std::string> tags_`, `.tag()`, `.has_tag()` on `PropertyFlags`.
- [ ] `src/editor/inspector_editors.h` has `std::vector<std::string> tags_`, `.tag()`, `.has_tag()` on `EditorFlags`.
- [ ] `src/engine/scene/component_registry/register_all_components.cpp` registers `math::Color` as 9th built-in type, includes `"math/color_yaml.h"`, has updated light component property registrations with `Color` type and `.tag("rgb")`.
- [ ] `src/engine/scene/component_registry/register_all_components.h` comment says "nine built-in types".
- [ ] All three light component headers (.h) and implementations (.cpp) use `math::Color` instead of `math::Vec3` for `color_` member, `color()` accessors, and constructor parameters.
- [ ] `src/engine/render/pbr/pbr_material.h` uses `math::Color base_color_factor` with `#include "math/color.h"`.
- [ ] `src/engine/render/pbr/pbr_material.cpp` converts Color to Vec4 at set_uniform call site.
- [ ] `src/engine/asset/model_loader.cpp` uses `math::Color{...}` for `base_color_factor` assignments.
- [ ] `src/engine/render/render_system.cpp` uses `.r/.g/.b` instead of `.x/.y/.z` for light color reads.
- [ ] `src/editor/inspector_editors.cpp` registers Color editor with `ImGui::ColorEdit4`/`ColorEdit3` based on `has_tag("rgb")`, log says "9 built-in inspector editors".
- [ ] `tests/engine/color_tests.cpp` exists with all required unit tests (layout, GLM interop, constructors, arithmetic, YAML encode/decode/backward-compat/invalid, sRGB/linear roundtrip, blend, luminance, darken/lighten, HSV roundtrip, named colors, RGBA32 roundtrip, to_vec3, edge cases).
- [ ] `tests/engine/component_color_registry_tests.cpp` exists with a light component serialization roundtrip test confirming the TypeRegistry + YAML pipeline works end-to-end for Color.
- [ ] All unit tests pass.
- [ ] All existing engine tests pass unchanged.
- [ ] E2E demo apps with lights produce visually identical screenshots before and after migration (verified by vision analysis tool, ±1 LSB tolerance).
