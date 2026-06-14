# SPEC-2026-06-ColorType - Engine Color Type

## Problem

The engine currently uses `Vec3` (for RGB light colors) and `Vec4` (for PBR `base_color_factor`) to represent colors. This is semantically imprecise: vectors represent directions/magnitudes, while colors have their own domain (alpha compositing, sRGB ↔ linear conversion, luminance, HSV). The absence of a dedicated `Color` type leads to:

- Color-vector confusion — developers may mix color values with position/direction values at the call site.
- Missing color-specific operations — sRGB/linear conversion, alpha blending, luminance, and HSV helpers have no natural home.
- Editor ergonomics — the Vec3/Vec4 editor widgets show axis labels (X, Y, Z/W) rather than a color picker; a Color type enables ColorEdit3/ColorEdit4.

## Goals

1. Introduce a single `Color` struct (RGBA, 4 floats) wrapping `glm::vec4` using the same `reinterpret_cast` pattern as Vec3/Vec4/Quat (ADR-002).
2. Provide a full color-specific API: constructors, component-wise arithmetic, sRGB↔linear conversion, luminance, darken/lightened, alpha blending, HSV helpers, named color presets, `to_rgba32`/`from_rgba32`, and `.to_vec3()`.
3. YAML serialization: flow sequence `[r, g, b, a]` on save; accept both `[r, g, b, a]` and `[r, g, b]` (alpha defaults to 1.0) on load.
4. Editor integration: register an inspector editor using `ImGui::ColorEdit4` (or `ImGui::ColorEdit3` when the `Color` property has the `"rgb"` tag set via `PropertyFlags::tag("rgb")`).
5. Migrate `PointLightComponent`, `DirectionalLightComponent`, `SpotLightComponent`, and `PbrMaterialData::base_color_factor` from `Vec3`/`Vec4` to `Color`.
6. Verify with Catch2 unit tests: math correctness, YAML roundtrip, backward compat.

## Non-goals

- No `Color3` type. Only a single `Color{r,g,b,a}` type. The distinction between "no alpha" and "with alpha" is handled by setting the `"rgb"` tag via `PropertyFlags::tag("rgb")` in the editor registration.
- No implicit conversion between `Color` and `Vec4`. Conversion is explicit only — `Color::to_vec3()` and explicit `Color(glm::vec4)` constructor.
- No migration of `Vertex::color` or `LightData::color` — these stay as `Vec4` due to GPU layout coupling.
- No runtime performance regression — the type must maintain zero-overhead over raw `glm::vec4` usage.
- No sRGB-to-linear conversion applied automatically — conversion is an explicit method call.

## Actors

| Actor | Role |
|---|---|
| **Engine developer** | Writes C++ code using the `Color` type, calls color operations. |
| **Editor user** | Sees color pickers in the inspector panel, edits color values. |
| **Scene file author** | Writes/reads YAML scene files with `[r, g, b, a]` or `[r, g, b]` color values. |

## User-visible behavior

- In the editor's inspector panel, a `Color` property is rendered as an ImGui color picker:
  - `ColorEdit3` (no alpha channel) when the property's `PropertyFlags` includes the `"rgb"` tag (set via `.tag("rgb")`).
  - `ColorEdit4` (with alpha channel) when the property does not have the `"rgb"` tag.
- Light components (`point_light`, `directional_light`, `spot_light`) have a `color` property that shows a 3-channel color picker (no alpha).
- PBR material `base_color_factor` shows a 4-channel color picker (with alpha).
- Scene files serialize colors as `[r, g, b, a]` flow sequences.
- Old scene files with `[r, g, b]` (3-element sequence) load correctly — alpha defaults to 1.0.
- Color values outside [0, 1] (HDR) are preserved in all operations except `to_rgba32` (which clamps).

## User stories

### Story 1 — Color math operations (Priority: P1)

As an engine developer, I want to perform component-wise arithmetic on colors (add, subtract, multiply by color/float) so that I can do simple color manipulation.

**Given** a Color `c1{1.0, 0.5, 0.25, 1.0}` and Color `c2{0.1, 0.2, 0.3, 0.5}`
**When** I compute `c1 + c2`, `c1 - c2`, `c1 * c2`, `c1 / c2`, `c1 * 2.0f`, `2.0f * c1`, `c1 / 2.0f`
**Then** the result matches `glm::vec4` component-wise reference.

### Story 2 — YAML serialization roundtrip (Priority: P1)

As a scene file author, I want colors to be serialized as clean flow sequences so that the files are human-readable.

**Given** a Color `c{0.5, 0.75, 0.25, 1.0}`
**When** I encode it to YAML using `YAML::convert<Color>::encode`
**Then** the output is a flow sequence `[0.5, 0.75, 0.25, 1.0]`.

**Given** a YAML node `[0.5, 0.75, 0.25, 1.0]`
**When** I decode it using `YAML::convert<Color>::decode`
**Then** the result is `Color{0.5, 0.75, 0.25, 1.0}`.

### Story 3 — Backward-compatible YAML loading (Priority: P1)

As a scene file author migrating from Vec3-based colors, I want old 3-element `[r, g, b]` sequences to load correctly so that existing scene files work without modification.

**Given** a YAML node `[0.5, 0.75, 0.25]` (3 elements)
**When** I decode it using `YAML::convert<Color>::decode`
**Then** the result is `Color{0.5, 0.75, 0.25, 1.0}`.

### Story 4 — sRGB ↔ linear conversion (Priority: P2)

As an engine developer, I want to convert between sRGB and linear color spaces so that light colors and textures are displayed correctly.

**Given** a Color in sRGB space `srgb{0.5, 0.5, 0.5, 1.0}`
**When** I call `srgb.to_linear()`
**Then** each channel (r, g, b) is converted using the sRGB transfer function; alpha is unchanged.

### Story 5 — Luminance and color modifiers (Priority: P2)

As an engine developer, I want to compute luminance and apply darken/lighten modifiers to colors for UI and visual effects.

**Given** a Color `c{0.5, 0.3, 0.8, 1.0}`
**When** I call `c.luminance()`, `c.darkened(0.5f)`, `c.lightened(0.5f)`
**Then** `c.luminance()` matches `0.2126*R + 0.7152*G + 0.0722*B`; `c.darkened(0.5f)` equals `c * (1 - 0.5)`; `c.lightened(0.5f)` equals `c + (1 - c) * 0.5`.

### Story 6 — Alpha blending (Priority: P2)

As an engine developer, I want to perform alpha blending between two colors for UI compositing.

**Given** foreground `fg{1.0, 0.0, 0.0, 0.5}` and background `bg{0.0, 1.0, 0.0, 1.0}`
**When** I call `Color::blend(fg, bg)`
**Then** the result is the standard over-blend: `Color{0.5, 0.5, 0.0, 1.0}`.

### Story 7 — HSV helpers (Priority: P3)

As an engine developer, I want to convert between RGB and HSV so that I can create/interpret color pickers and rainbow effects.

**Given** a Color `c{0.5, 0.3, 0.8, 1.0}`
**When** I call `c.to_hsv()` and then `Color::from_hsv(h, s, v)` with the same values
**Then** the roundtripped color approximately equals `c` (within `1e-5f` epsilon).

### Story 8 — Named color presets (Priority: P3)

As an engine developer, I want to use named color constants so that common colors are easy to reference.

**Given** the static member functions `Color::white()`, `Color::black()`, `Color::red()`, `Color::green()`, `Color::blue()`, `Color::yellow()`, `Color::cyan()`, `Color::magenta()`
**When** I access them
**Then** they return predefined Color values with alpha = 1.0.

### Story 9 — RGBA32 conversion (Priority: P3)

As an engine developer, I want to convert between `Color` and 32-bit RGBA (uint8_t[4]) for pixel buffer operations.

**Given** a Color `c{1.0, 0.5, 0.25, 1.0}`
**When** I call `c.to_rgba32()`
**Then** the result is `{255, 127, 63, 255}` (clamped, rounded).

**Given** an RGBA32 value `{255, 127, 63, 255}`
**When** I call `Color::from_rgba32(rgba32)`
**Then** the result is `Color{1.0, 127.0/255.0, 63.0/255.0, 1.0}`.

### Story 10 — Migration of light components (Priority: P1)

As an engine developer, I want `PointLightComponent`, `DirectionalLightComponent`, `SpotLightComponent`, and `PbrMaterialData::base_color_factor` to use `Color` instead of `Vec3`/`Vec4` so that the editor shows color pickers and the type semantics are correct.

**Given** a `PointLightComponent` with a color property registered as `Color`
**When** I open the inspector panel
**Then** the color property is rendered with an ImGui color picker (3-channel, no alpha).

**Given** a scene file containing a `point_light` with `color: [0.5, 0.3, 0.8]` (old Vec3 format)
**When** the scene is loaded
**Then** the color is correctly decoded as `Color{0.5, 0.3, 0.8, 1.0}` via backward-compat YAML loading.

### Story 11 — E2E screenshot verification of light migration (Priority: P1)

As an engine developer, I want to verify that the migration of light components from `Vec3` to `Color` produces identical rendered output so that the migration is safe and does not introduce visual regressions.

**Given** a demo app with lights (PointLight, DirectionalLight, SpotLight) that uses RGB values for light colors
**When** I run the demo app with `--capture` before migration and capture a baseline screenshot
**Then** the baseline screenshot is stored as a reference image.

**Given** a baseline reference screenshot taken before migration
**When** I run the same demo app with `--capture` after migration (same RGB values)
**Then** I compare the post-migration screenshot with the baseline using the vision analysis tool, and the rendered output is visually identical (pixel match within ±1 LSB tolerance).

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `Color` is a standard-layout struct with `r, g, b, a` public float members in the same layout as `glm::vec4`. | Three `static_assert`: `is_standard_layout_v`, `sizeof(Color) == sizeof(glm::vec4)`, `is_trivially_copyable_v`. |
| AC-002 | `Color` has a `.glm()` accessor returning `glm::vec4&` via `reinterpret_cast`. | Unit test: `c.glm()` returns a reference that modifies the original Color. |
| AC-003 | Default constructor `Color()` initializes all components to 0.0f. | Unit test: `Color{}` has r=g=b=a=0.0f. |
| AC-004 | RGBA constructor `Color(r, g, b, a)` and RGB constructor `Color(r, g, b)` where alpha defaults to 1.0. | Unit test: `Color{1,0,0}` has a=1.0; `Color{1,0,0,0.5}` has a=0.5. |
| AC-005 | Component-wise arithmetic operators (`+`, `-`, `*`, `/` with Color and float) produce correct results. | Unit test: results match reference GLM component-wise computation. |
| AC-006 | YAML encode produces `[r, g, b, a]` flow sequence. | Unit test: `YAML::convert<Color>::encode(Color{0.5,0.75,0.25,1.0})` emits `[0.5, 0.75, 0.25, 1.0]`. |
| AC-007 | YAML decode accepts both 4-element `[r,g,b,a]` and 3-element `[r,g,b]` sequences. | Unit tests: decode `[0.5, 0.75, 0.25, 1.0]` → `Color{0.5,0.75,0.25,1.0}`; decode `[0.5, 0.75, 0.25]` → `Color{0.5,0.75,0.25,1.0}`. |
| AC-008 | YAML decode returns false for invalid sequences (wrong size, non-sequence, non-numeric). | Unit test: decode `[0.5, "abc"]`, `{r: 1, g: 2, b: 3}`, `[1,2]` → false. |
| AC-009 | `Color::to_linear()` and `Color::to_srgb()` apply the sRGB transfer function per-channel (r, g, b only; alpha unchanged). | Unit test: roundtrip `srgb → linear → srgb` approximates identity. |
| AC-010 | `Color::blend(fg, bg)` performs standard over-blend: `out.a = fg.a + bg.a*(1-fg.a)`, `out.rgb = (fg.rgb*fg.a + bg.rgb*bg.a*(1-fg.a)) / out.a`. | Unit test: expected values as defined in Story 6. |
| AC-011 | Inspector editor registered for `Color` type uses `ImGui::ColorEdit4` when no `"rgb"` tag is present, `ColorEdit3` when `property.flags.has_tag("rgb")` is true. | Unit test: editor registration exists for `Color`. (Manual verification in editor: visual inspection.) |
| AC-012 | Migrated components (`PointLightComponent`, `DirectionalLightComponent`, `SpotLightComponent`) use `Color` for their `color` property. | Unit test: compile-time type check and serialization roundtrip. |
| AC-013 | `PbrMaterialData::base_color_factor` is changed from `Vec4` to `Color`. | Unit test: compile-time type check and serialization roundtrip. |
| AC-014 | Static named colors (`white()`, `black()`, `red()`, `green()`, `blue()`, `yellow()`, `cyan()`, `magenta()`) return correct RGBA values with alpha=1.0. | Unit test: each constant matches expected float values. |
| AC-015 | Light rendering is visually identical before and after Color migration when the same RGB values are used. | Run demo app with `--capture` before and after migration; use vision analysis tool to compare screenshots. Pixels must be identical within ±1 LSB tolerance. |

## E2E Verification

This feature is verified through:

- **Method**: Catch2 unit tests covering all math operations, YAML encode/decode (including backward compat), sRGB/linear roundtrip, blending, HSV roundtrip, RGBA32 conversion, and named colors.
- **Method**: Manual verification in the editor: open an entity with a light component, observe the color picker rendering.
- **Method**: Automated test for component serialization roundtrip (existing `component_registry_tests` infrastructure), ensuring that light component and PBR material colors serialize/deserialize correctly.
- **Method**: E2E screenshot capture comparison — run demo apps (with PointLight, DirectionalLight, SpotLight) using `--capture` CLI argument before and after migration to produce screenshots, then compare them visually using the vision analysis tool to confirm identical rendered output.

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | All new acceptance criteria pass in CI (Catch2 tests). |
| SC-002 | All existing engine tests pass unchanged. |
| SC-003 | Zero runtime overhead: `sizeof(Color) == sizeof(glm::vec4)` and `std::is_trivially_copyable_v<Color>` confirmed. |
| SC-004 | Backward-compatible: existing scene files with 3-element `[r,g,b]` light colors load without errors and produce correct colors. |
| SC-005 | Migration complete: all four target types (`PointLightComponent`, `DirectionalLightComponent`, `SpotLightComponent`, `PbrMaterialData`) use `Color` instead of `Vec3`/`Vec4`. |

## Edge cases

| Edge case | Expected behavior |
|---|---|
| Alpha = 0 (transparent) | All operations handle alpha=0 correctly: blending divides by `out.a` (defined as 0 → output rgb = 0). `to_linear()`/`to_srgb()` do not modify alpha. |
| HDR values (channel > 1.0) | Arithmetic preserves HDR values. `to_rgba32()` clamps to [0, 1] per channel. `to_srgb()`/`to_linear()` apply transfer function to HDR values as-is (extended sRGB domain). |
| Negative color values | Arithmetic produces negative values (same as GLM). `to_rgba32()` clamps to [0, 1] (negative becomes 0). |
| Floating-point precision | YAML roundtrip may introduce minor precision loss. Acceptance tolerance: 1e-5f relative error. |
| Division by zero | `/` operator on `Color` with zero component produces inf/nan (same as GLM). No guard. |
| NaN inputs | No validation — NaN propagates through operations (same as GLM behavior). |
| `to_linear()` on already-linear color | Applying `to_linear()` to a linear color produces incorrect values — this is a caller responsibility. No auto-detection of color space. |
| Empty or non-color YAML nodes | Decode returns false; caller (TypeRegistry) handles the error. |
| Old mapping format `{r: 1, g: 2, b: 3}` | NOT supported for backward compat (different from Vec3's legacy mapping format). Only flow sequence `[r,g,b]` and `[r,g,b,a]` are accepted. |

## Error cases

| Error | Handling |
|---|---|
| YAML node is not a sequence | `decode` returns false. |
| YAML sequence has size < 3 or > 4 | `decode` returns false. |
| YAML sequence elements are non-float (e.g., strings) | `decode` catches `YAML::Exception` and returns false. |
| YAML node has mixed types (some floats, some strings) | `decode` returns false at the first non-float element. |
| `to_rgba32` with NaN/Inf in channel | Clamped to [0, 255] — NaN becomes 0, Inf becomes 255 (standard C++ casting behavior). |

## Permissions and security

- No authentication or authorization required — this is a pure data type.
- No data validation beyond YAML parse error handling.
- No sensitive data exposure — colors are visual data only.

## Observability

- A one-time `BUDDD_LOG_TAGGED_WARN` during migration: if a scene file uses the old `Vec3` field name for a migrated component (e.g., a `point_light` with a Vec3-typed `color` property), the unknown-key warning in `ComponentInfo::deserialize` will surface it.
- No runtime metrics needed — Color operations are pure computation with no side effects.
- YAML decode failures for color properties are surfaced through the existing error handling in `component_registry` (logged at WARN level).

## Out of scope

- Automatic color-space tagging or detection.
- Color interpolation (beyond basic lerp available via `glm::mix`).
- Color gamut management or wide-gamut support.
- Pre-multiplied alpha variant of `Color` — only straight alpha.
- Editor undo/redo for color-specific changes (inherited from the existing system).
- Color blending modes other than standard over-blend.

## Assumptions

| Assumption | Rationale |
|---|---|
| `glm::vec4` memory layout is `{x, y, z, w}` (confirmed all current GLM versions). | Required for `reinterpret_cast` safety. Verified by `static_assert(sizeof(Color) == sizeof(glm::vec4))`. |
| Color is always in either sRGB or linear color space — caller is responsible for tracking. | Matching industry conventions (no color space tag). |
| sRGB transfer function: linear threshold at 0.0031308, gamma = 12.92 linear segment and exponent 2.4 for non-linear segment. | Standard IEC 61966-2-1. See `docs/wiki/domain/color-spaces.md` if exists. |
| Luminance formula: `0.2126*R + 0.7152*G + 0.0722*B` (sRGB/Rec. 709). | Standard luminance coefficients. |
| HSV conversion: H in [0, 1) normalized where 0=red, 1/3=green, 2/3=blue; S and V in [0, 1]. | Editor color picker convention. |
| `darkened(f) = rgb * (1 - f)`, `lightened(f) = rgb + (1 - rgb) * f`. Godot-style multiply semantics. | `darkened(0)` = identity; `darkened(1)` = black. `lightened(0)` = identity; `lightened(1)` = white. |
| YAML precision uses full `yaml-cpp` default (no rounding), matching existing Vec3/Vec4 behavior. | HDR values beyond [0,1] must roundtrip without precision loss. |
| `PropertyFlags` gains a `std::vector<std::string> tags_` member and a fluent `.tag("rgb")` setter. The inspector editor calls `property.flags.has_tag("rgb")` to decide between `ColorEdit3` and `ColorEdit4`. Tags are extensible: game code can add arbitrary string tags without recompiling the engine. | Positive naming (no double negative), extensible by game code. No tag = RGBA default (ColorEdit4). |
| YAML backward compat only applies to the 3-element `[r,g,b]` format, not to the old `{r:, g:, b:}` mapping format. | Vec3's YAML converter supports a legacy mapping format, but Color's does not — the mapping format was never used for color values in practice. If needed, this can be added later. |

## Open questions

All earlier clarifications have been resolved:

1. **Precision for YAML output**: Full `yaml-cpp` default precision. Match existing Vec3/Vec4 behavior. No rounding.
2. **HSV value range**: Normalized [0, 1) where 0=red, 1/3=green, 2/3=blue.
3. **`darkened`/`lightened` factor semantics**: Godot-style multiply — `darkened(f) = rgb * (1 - f)`, `lightened(f) = rgb + (1 - rgb) * f`.
4. **Flag placement**: String-based tag system on `PropertyFlags` (`std::vector<std::string> tags_` + `.tag("rgb")` fluent setter), not a hardcoded `bool hide_alpha` or a separate `EditorFlags` field.

## Files to create or modify

### New files

| File | Purpose |
|---|---|
| `src/engine/math/color.h` | Color struct definition (header-only, same pattern as vec3.h/vec4.h). |
| `src/engine/math/color_yaml.h` | YAML convert specialization for Color. |
| `tests/engine/color_tests.cpp` | Catch2 unit tests for math, YAML roundtrip, backward compat. |

### Modified files

| File | Change |
|---|---|
| `src/engine/math/math.h` | Add `#include "color.h"`. |
| `src/engine/render/pbr/pbr_material.h` | Change `PbrMaterialData::base_color_factor` from `Vec4` to `Color`. |
| `src/engine/scene/point_light_component.h` | Change `color()` return type and `color_` member from `Vec3` to `Color`. |
| `src/engine/scene/directional_light_component.h` | Same as point_light. |
| `src/engine/scene/spot_light_component.h` | Same as point_light. |
| `src/engine/scene/component_registry/register_all_components.cpp` | Include `color_yaml.h`. Register `Color` in TypeRegistry. Update property registrations for light components and PBR material (`base_color_factor`) to use `Color` type. Add `.tag("rgb")` to light color properties (no alpha for lights). |
| `src/editor/inspector_editors.cpp` | Register `Color` editor with ImGui color picker. Read `property.flags.has_tag("rgb")` to decide between `ColorEdit3` (has tag) / `ColorEdit4` (no tag). |
| `src/engine/scene/component_registry/property.h` | Add `std::vector<std::string> tags_` member and `.tag("rgb")` / `has_tag("rgb")` fluent methods to `PropertyFlags` struct. |
| `tests/engine/math_tests.cpp` | No change — color tests should go in a separate file `color_tests.cpp` to avoid bloating the existing math test file. |

### Files NOT modified

| File | Rationale |
|---|---|
| `src/engine/render/light_data.h` (assumed containing `LightData`) | Stays as Vec4 — GPU layout coupling (explicitly excluded). |
| `Vertex` struct / vertex input layout | Stays as Vec4 — GPU layout coupling (explicitly excluded). |
| Existing Vec3/Vec4 types | Unchanged — Vector3/4 remain for non-color use cases. |

## Document updates

The following existing documentation must be updated to reflect this feature:

| File | Action | Rationale |
|---|---|---|
| `docs/wiki/architecture/module-map.md` — TypeRegistry section | Change "Eight built-in types" to "Nine built-in types" and add `Color` to the list. | `Color` is being added as a 9th built-in type alongside float, int32_t, bool, std::string, Vec3, Vec4, Quat, and std::shared_ptr<Model>. |
| `docs/wiki/editor/editor-panels.md` — Inspector Property Editors table (line 233) | Update "Color picker + float fields (not yet implemented)" to reflect that the Color editor is now implemented using `ImGui::ColorEdit4` / `ColorEdit3` with tag-based alpha control. | The implementation status and editor widget description must match the final implementation. |
| `docs/adr/ADR-002-glm-wrapper-math.md` | No changes needed — confirmed compatible. | `Color` follows the same `reinterpret_cast` wrapper pattern as Vec3/Vec4/Quat. |
| `docs/adr/ADR-028-component-type-registry.md` | No changes needed — confirmed compatible. | `Color` adds a new built-in type but does not change the TypeRegistry registration mechanism. |
