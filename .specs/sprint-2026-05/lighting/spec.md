# SPEC-018 — Phong Lighting System: Light Components, Standard Vertex, Phong Module

## Status

`Draft`

Allowed values: `Draft`, `In Review`, `Accepted`

## Problem

The Buddd Engine currently supports only unlit rendering. All objects render with flat per-vertex colours passed directly to the fragment shader. There is no concept of light sources, surface normals, or material reflectance. The rendering pipeline has:

- **No light sources**: No way to place directional or point lights in a scene.
- **No normals**: Vertex format has only position (Float3) and colour (Float3) — no normal vector for lighting calculations.
- **No lighting computation**: Fragment shaders output per-vertex colour directly; no ambient, diffuse, or specular terms.
- **No material properties**: Shininess, specular colour, ambient/diffuse reflectance are not represented.
- **No standard vertex format**: Mesh types define ad-hoc vertex layouts (e.g., `CubeVertex`, `LitCubeVertex`), preventing uniform shader interfaces and composability.

Every future visual feature (shadows, PBR, atmospheric effects, post-processing) depends on a lighting foundation. Without it, the engine cannot produce visually compelling scenes with depth and material definition.

## Goals

- **Standard Vertex struct** (`src/engine/render/vertex.h`): A single `Vertex` struct used by ALL meshes — position (loc 0), colour (loc 1, default white), normal (loc 2), texcoord (loc 3), tangent (loc 4, reserved for normal mapping), texcoord2 (loc 5, reserved for lightmaps). Stride: 72 bytes, 6 attributes. Existing unlit demos use position+colour and leave other fields as zero. The GPU safely ignores unbound attribute locations.
- **Phong module** (`src/engine/render/phong/`): Encapsulates all Phong lighting into a proper module with `PhongMaterial` (inheriting `Material`), embedded GLSL 450 core shader strings, and convenience setters (`set_camera_position()`, `set_lights()`). Declares all known uniforms so the headless backend can discover them.
- **Separate light components per type**: `DirectionalLightComponent`, `PointLightComponent`, `SpotLightComponent` — each in `src/engine/scene/`, each deriving `Component` from `scene/component.h`. Direction from entity rotation (directional), position from world_matrix translation (point/spot).
- **glsl_util refactoring** (`src/engine/render/glsl_util.h/.cpp`): `extract_uniform_names()` and `normalize_uniform_name()` shared by BOTH `RenderDeviceOpenGL` and `RenderDeviceHeadless` instead of duplicated code.
- **RenderSystem extension**: Collects lights (max 8) across all three light component types, flattens into `LightData` structs, sets lighting uniforms via `has_uniform("u_model")` sentinel. Ambient outside loop, Blinn-Phong specular, textures via `u_diffuse_texture`.
- **Phong demo** (`src/cmd/demo/phong_demo.cpp`): `buddd demo phong` — interactive free-camera (WASD + mouse), textured cube (assets/brick.png or procedural checkerboard fallback), orbiting `PointLightComponent`, static `DirectionalLightComponent` for fill. Uses ECS: World + RenderSystem + light components + MeshRenderer + PhongMaterial.
- **Backward compatibility**: All existing unlit demos (triangle, cube, cube-scene, textured-cube, free-camera) continue to work without modification using the same Standard Vertex (position+colour filled, normals/texcoords zeroed).

## Non-goals

- No shadow mapping or shadow volumes.
- No physically-based rendering (PBR) materials.
- No light cookies, gobos, or projector textures.
- No light culling, clustering, or spatial partitioning.
- No deferred rendering or light prepass.
- No HDR rendering or tone mapping.
- No light animation system — per-demo orbit is manual code in the demo.
- No per-entity material overrides beyond what the Phong shader uniforms provide.
- No per-vertex colour as diffuse input for the lit shader path (diffuse colour comes from texture sampling only).
- No change to existing unlit demos (`triangle`, `cube`, `cube-scene`, `textured-cube`, `free-camera`).
- No change to the existing `MeshRenderer` or `Material` abstract interfaces.
- No change to the existing `CameraComponent`, `World`, or `Entity` public APIs.
- No light editor UI or debug visualization.
- No struct-array uniforms (using flat arrays instead — see design).
- No per-entity material overrides beyond what `PhongMaterial` convenience setters provide.

## Relationship to existing specs

**SPEC-005 (Render Pipeline)** defines the abstract `Material`, `Shader`, `VertexBuffer`, `IndexBuffer` interfaces. SPEC-018 does not change these interfaces — it adds `PhongMaterial` as a new `Material` subclass and standard `Vertex` struct replacing previous ad-hoc vertex formats.

**SPEC-011 (Scene Rendering)** defines `MeshRenderer`, `CameraComponent`, `RenderSystem`, and explicitly lists "No lighting or shadow components" as a non-goal. SPEC-018 supersedes SPEC-011 on this point: it adds light components and extends `RenderSystem` with light collection and uniform passing, while leaving all other SPEC-011 contracts unchanged.

**SPEC-017 (Texture Support)** defines `Material::set_texture()`, `Texture` abstraction, and deferred uniform application in `bind()`. SPEC-018 depends on SPEC-017's texture system as the diffuse colour source for the Phong shader: The lit fragment shader samples `u_diffuse_texture` for diffuse colour, and the lit demo binds a texture (via `Material::set_texture()`) to the cube's material.

**SPEC-012 (Depth Buffer Support)** enables depth testing (`GL_DEPTH_TEST`, `GL_LESS`). SPEC-018 depends on depth testing being active for correct 3D lighting (depth buffer is already enabled in `RenderDeviceOpenGL`).

## Actors

| Actor | Description |
|---|---|
| Engine developer | Adds engine features that require lighting — scene lighting, material property configuration, light-based visual effects. Extends `RenderSystem` with light collection. |
| Application developer | Builds on top of the engine. Creates entities, attaches light components and `MeshRenderer`, and calls `RenderSystem::render()` each frame with automatic lighting computation. |
| Test suite | Catch2 v3 tests that verify light data collection, uniform setting, light component lifecycle, normal matrix computation, and backward compatibility — all in headless mode (no GPU required). |

## User-visible behaviour

### 1. Standard Vertex struct (`src/engine/render/vertex.h`)

A single standard vertex format used by ALL meshes in the engine:

```cpp
#pragma once

#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

namespace buddd::engine {

/// Standard vertex format used by all meshes.
/// Fields not used by a particular shader can be left as zero-initialized
/// — the GPU safely ignores unbound attribute locations.
struct Vertex {
    math::Vec3 position;     // offset 0   (12B)  location 0
    math::Vec4 color;        // offset 12  (16B)  location 1  (1.0, 1.0, 1.0, 1.0 = white default)
    math::Vec3 normal;       // offset 28  (12B)  location 2
    math::Vec2 texcoord;     // offset 40  (8B)   location 3
    math::Vec4 tangent;      // offset 48  (16B)  location 4  — reserved for future normal mapping
    math::Vec2 texcoord2;    // offset 64  (8B)   location 5  — reserved for future lightmap
};
// Stride: 72 bytes, 6 attributes

/// Vertex format descriptor for the standard Vertex.
/// Use this when creating vertex buffers for the Standard Vertex.
inline constexpr VertexFormat k_standard_vertex_format = [] {
    VertexFormat fmt;
    fmt.stride = sizeof(Vertex);
    fmt.attributes = {
        {0, VertexAttributeType::Float3, static_cast<uint32_t>(offsetof(Vertex, position)),  false},
        {1, VertexAttributeType::Float4, static_cast<uint32_t>(offsetof(Vertex, color)),     false},
        {2, VertexAttributeType::Float3, static_cast<uint32_t>(offsetof(Vertex, normal)),    false},
        {3, VertexAttributeType::Float2, static_cast<uint32_t>(offsetof(Vertex, texcoord)),  false},
        {4, VertexAttributeType::Float4, static_cast<uint32_t>(offsetof(Vertex, tangent)),   false},
        {5, VertexAttributeType::Float2, static_cast<uint32_t>(offsetof(Vertex, texcoord2)), false},
    };
    return fmt;
}();

} // namespace buddd::engine
```

**Usage conventions**:

- The existing cube demo (unlit) fills `position` and `color`, leaves `normal`, `texcoord`, `tangent`, `texcoord2` as zero-initialized. The GPU safely ignores unbound attribute locations — location 2 (normal) is not bound in the unlit shader, so the zeros are never read.
- The Phong material uses `position` (loc 0) + `normal` (loc 2) + `texcoord` (loc 3). It ignores `color`, `tangent`, `texcoord2`.
- Future materials (normal mapping) can use `tangent` (loc 4); future lightmapped materials can use `texcoord2` (loc 5).

### 2. Light components — separate per type

#### 2a. DirectionalLightComponent (`src/engine/scene/directional_light_component.h`)

```cpp
#pragma once

#include "math/vec3.h"
#include "scene/component.h"

namespace buddd::engine {

/// Directional light: infinite light source.
/// Direction is derived from the entity's world rotation (-Z forward).
/// No position or range — affects all surfaces equally regardless of distance.
class DirectionalLightComponent : public Component {
public:
    explicit DirectionalLightComponent(
        math::Vec3 colour = math::Vec3{1.0f, 1.0f, 1.0f},
        float intensity = 1.0f
    );

    DirectionalLightComponent() = default;

    auto colour() noexcept -> math::Vec3&;
    auto colour() const noexcept -> const math::Vec3&;
    auto intensity() noexcept -> float&;
    auto intensity() const noexcept -> float;

    auto on_attach() -> void override {}

private:
    math::Vec3 colour_{1.0f, 1.0f, 1.0f};
    float intensity_ = 1.0f;
};

} // namespace buddd::engine
```

#### 2b. PointLightComponent (`src/engine/scene/point_light_component.h`)

```cpp
#pragma once

#include "math/vec3.h"
#include "scene/component.h"

namespace buddd::engine {

/// Point light: omni-directional light with position and range.
/// Position is derived from the entity's world_matrix() translation.
class PointLightComponent : public Component {
public:
    explicit PointLightComponent(
        math::Vec3 colour = math::Vec3{1.0f, 1.0f, 1.0f},
        float intensity = 1.0f,
        float range = 10.0f
    );

    PointLightComponent() = default;

    auto colour() noexcept -> math::Vec3&;
    auto colour() const noexcept -> const math::Vec3&;
    auto intensity() noexcept -> float&;
    auto intensity() const noexcept -> float;
    auto range() noexcept -> float&;
    auto range() const noexcept -> float;

    auto on_attach() -> void override {}

private:
    math::Vec3 colour_{1.0f, 1.0f, 1.0f};
    float intensity_ = 1.0f;
    float range_ = 10.0f;
};

} // namespace buddd::engine
```

#### 2c. SpotLightComponent (`src/engine/scene/spot_light_component.h`)

```cpp
#pragma once

#include "math/vec3.h"
#include "scene/component.h"

namespace buddd::engine {

/// Spot light: conical light with position, direction, and cone angles.
/// Position from entity world_matrix() translation.
/// Direction from entity world rotation (-Z forward).
/// inner_angle and outer_angle define the cone falloff in radians.
class SpotLightComponent : public Component {
public:
    explicit SpotLightComponent(
        math::Vec3 colour = math::Vec3{1.0f, 1.0f, 1.0f},
        float intensity = 1.0f,
        float range = 10.0f,
        float inner_angle = 0.785f,    // 45 degrees
        float outer_angle = 1.047f     // 60 degrees
    );

    SpotLightComponent() = default;

    auto colour() noexcept -> math::Vec3&;
    auto colour() const noexcept -> const math::Vec3&;
    auto intensity() noexcept -> float&;
    auto intensity() const noexcept -> float;
    auto range() noexcept -> float&;
    auto range() const noexcept -> float;
    auto inner_angle() noexcept -> float&;
    auto inner_angle() const noexcept -> float;
    auto outer_angle() noexcept -> float&;
    auto outer_angle() const noexcept -> float;

    auto on_attach() -> void override {}

private:
    math::Vec3 colour_{1.0f, 1.0f, 1.0f};
    float intensity_ = 1.0f;
    float range_ = 10.0f;
    float inner_angle_ = 0.785f;
    float outer_angle_ = 1.047f;
};

} // namespace buddd::engine
```

All three light components:
- Do **not** override the destructor — the base `Component` destructor is sufficient.
- `on_attach()` is explicitly overridden as a no-op.
- Properties are publicly mutable via non-const accessor references, matching the `CameraComponent` pattern.

### 3. Light data structure for uniform passing

Internal to `RenderSystem`, defined in `src/engine/render/light_data.h`:

```cpp
namespace buddd::engine::detail {

/// Maximum number of lights supported by the Phong shader (must match GLSL #define).
constexpr int k_max_lights = 8;

/// Per-light data packed for GPU uniforms.
/// Each field maps to the corresponding GLSL flat array.
struct LightData {
    math::Vec4 position_or_dir;  // .xyz = position/direction, .w = type: 0=directional, 1=point, 2=spot
    math::Vec4 colour;           // .xyz = colour * intensity pre-multiplied, .w = unused
    float range;                 // Attenuation range (ignored for directional)
    math::Vec4 spot_direction;   // .xyz = normalized spot direction (ignored for non-spot)
    float inner_cone;            // Cosine of inner angle (for spot)
    float outer_cone;            // Cosine of outer angle (for spot)
};

} // namespace buddd::engine::detail
```

The `w` component of `position_or_dir` distinguishes light type: 0 = directional, 1 = point, 2 = spot.

### 4. Phong module (`src/engine/render/phong/`)

#### 4a. PhongMaterial (`src/engine/render/phong/phong_material.h/.cpp`)

`PhongMaterial` inherits `Material` and embeds its own Phong vertex + fragment shaders. It provides convenience setters for all Phong uniforms and declares all known uniform names up-front.

```cpp
#pragma once

#include "render/material.h"

namespace buddd::engine {

/// Material implementing the Phong reflection model.
/// Embeds its own vertex and fragment shaders.
/// Use convenience setters to configure lighting and material properties.
class PhongMaterial : public Material {
public:
    /// Creates a PhongMaterial with embedded shaders.
    /// @param device The render device (used to create the shader program).
    /// @param known_uniforms Optional extra uniforms beyond the standard Phong set
    ///                       (e.g., user-defined uniforms on a derived material).
    explicit PhongMaterial(RenderDevice& device,
                           std::span<const std::string> known_uniforms = {});

    ~PhongMaterial() override;

    // -- Disable copy/move --
    PhongMaterial(const PhongMaterial&) = delete;
    auto operator=(const PhongMaterial&) -> PhongMaterial& = delete;
    PhongMaterial(PhongMaterial&&) = delete;
    auto operator=(PhongMaterial&&) -> PhongMaterial& = delete;

    // -- Material interface --

    auto set_uniform(std::string_view name, float value) -> Result<void> override;
    auto set_uniform(std::string_view name, int32_t value) -> Result<void> override;
    auto set_uniform(std::string_view name, bool value) -> Result<void> override;
    auto set_uniform(std::string_view name, const math::Vec3& value) -> Result<void> override;
    auto set_uniform(std::string_view name, const math::Vec4& value) -> Result<void> override;
    auto set_uniform(std::string_view name, const math::Mat4& value) -> Result<void> override;

    auto has_uniform(std::string_view name) const -> bool override;

    auto set_texture(std::string_view name, std::shared_ptr<Texture> texture) -> Result<void> override;
    auto has_texture(std::string_view name) const -> bool override;

    auto bind() const -> void override;

    // -- Convenience setters --

    /// Set the camera world position for specular calculation.
    auto set_camera_position(const math::Vec3& position) -> void;

    /// Set the light data array (up to k_max_lights entries, flattened).
    /// @param lights Pointer to LightData array.
    /// @param count Number of active lights (0..k_max_lights).
    auto set_lights(const detail::LightData* lights, int count) -> void;

    /// Set the model matrix, normal matrix, and combined MVP matrix.
    auto set_transforms(const math::Mat4& model, const math::Mat4& view_projection) -> void;

    /// Returns the list of known uniform names declared in the Phong shaders.
    static auto known_uniform_names() -> const std::vector<std::string>&;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace buddd::engine
```

**Design notes**:
- `PhongMaterial` creates its shaders internally (vertex and fragment from embedded GLSL strings). The caller does not need to create shaders separately.
- All standard Phong uniforms (see shader section) are declared in the known_uniforms list so the headless backend can find them.
- The `has_uniform("u_model")` sentinel pattern works identically to the original spec — `PhongMaterial` declares `u_model` so the check returns true.

#### 4b. Phong shaders (`src/engine/render/phong/phong_shaders.h`)

Embedded GLSL 450 core shader strings as `constexpr std::string_view` constants.

##### Vertex shader

```glsl
#version 450 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec2 a_texcoord;

out vec3 v_world_pos;
out vec3 v_normal;
out vec2 v_texcoord;

uniform mat4 u_mvp;
uniform mat4 u_model;
uniform mat4 u_normal_mat;  // Inverse-transpose of upper-left 3×3 of u_model; shader extracts mat3()

void main() {
    vec4 world_pos = u_model * vec4(a_position, 1.0);
    v_world_pos = world_pos.xyz;
    // Normal matrix is computed CPU-side as inverse-transpose of upper-left 3×3.
    v_normal = normalize(mat3(u_normal_mat) * a_normal);
    v_texcoord = a_texcoord;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
```

Note: `a_color` (location 1) is declared but unused. The Phong shader does not use per-vertex colour as diffuse input — diffuse colour comes from texture sampling. Declaring `a_color` makes the shader compatible with the Standard Vertex without requiring a separate vertex format.

##### Fragment shader (Phong model)

```glsl
#version 450 core

#define MAX_LIGHTS 8

in vec3 v_world_pos;
in vec3 v_normal;
in vec2 v_texcoord;

out vec4 frag_color;

uniform int      u_light_count;
uniform vec4     u_light_positions_or_dir[MAX_LIGHTS];  // .xyz = position/direction, .w = type (0=directional, 1=point, 2=spot)
uniform vec4     u_light_colours[MAX_LIGHTS];            // .rgb = colour * intensity pre-multiplied, .a = unused
uniform float    u_light_ranges[MAX_LIGHTS];             // attenuation range (ignored for directional)
uniform float    u_light_inner_cones[MAX_LIGHTS];        // cosine of inner angle (spot only)
uniform float    u_light_outer_cones[MAX_LIGHTS];        // cosine of outer angle (spot only)
uniform vec4     u_light_spot_directions[MAX_LIGHTS];   // .xyz = spot direction (normalized, spot only)
uniform vec3     u_camera_pos;

// Material properties
uniform vec3     u_material_ambient   = vec3(0.1, 0.1, 0.1);
uniform vec3     u_material_specular  = vec3(1.0, 1.0, 1.0);
uniform float    u_material_shininess = 32.0;

// Diffuse colour source: texture sampling
uniform sampler2D u_diffuse_texture;
// Optional global tint that multiplies with sampled texture colour (white = no tint)
uniform vec4      u_material_diffuse_tint = vec4(1.0, 1.0, 1.0, 1.0);

// Spot light cone falloff (smooth transition between inner and outer cone)
float spot_cone_attenuation(float cos_angle, float cos_inner, float cos_outer) {
    return clamp((cos_angle - cos_outer) / (cos_inner - cos_outer), 0.0, 1.0);
}

void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(u_camera_pos - v_world_pos);

    // Sample diffuse colour from texture, apply global tint
    vec3 diffuse_colour = texture(u_diffuse_texture, v_texcoord).rgb;
    diffuse_colour *= u_material_diffuse_tint.rgb;

    // Ambient term — constant, applied regardless of light count
    vec3 final_colour = u_material_ambient * diffuse_colour;

    for (int i = 0; i < u_light_count; ++i) {
        vec4 pos_or_dir = u_light_positions_or_dir[i];
        vec3 light_col  = u_light_colours[i].rgb;
        float range     = u_light_ranges[i];
        vec3 L;
        float attenuation = 1.0;

        if (pos_or_dir.w == 0.0) {
            // Directional light
            L = normalize(pos_or_dir.xyz);
        } else if (pos_or_dir.w == 1.0) {
            // Point light
            vec3 light_to_frag = pos_or_dir.xyz - v_world_pos;
            float dist = length(light_to_frag);
            L = light_to_frag / dist;

            // Squared distance-normalized falloff
            float normalized_dist = clamp(dist / range, 0.0, 1.0);
            attenuation = 1.0 - normalized_dist * normalized_dist;
        } else {
            // Spot light (w == 2.0)
            vec3 light_to_frag = pos_or_dir.xyz - v_world_pos;
            float dist = length(light_to_frag);
            L = light_to_frag / dist;

            // Squared distance-normalized falloff
            float normalized_dist = clamp(dist / range, 0.0, 1.0);
            attenuation = 1.0 - normalized_dist * normalized_dist;

            // Cone falloff: direction from separate uniform array
            vec3 spot_dir = normalize(u_light_spot_directions[i].xyz);
            float cos_angle = max(dot(-L, spot_dir), 0.0);
            attenuation *= spot_cone_attenuation(cos_angle,
                u_light_inner_cones[i], u_light_outer_cones[i]);
        }

        // Diffuse (Lambert)
        float NdotL = max(dot(N, L), 0.0);
        vec3 diffuse = diffuse_colour * light_col * NdotL;

        // Specular (Blinn-Phong)
        vec3 H = normalize(L + V);
        float NdotH = max(dot(N, H), 0.0);
        vec3 specular = u_material_specular * light_col * pow(NdotH, u_material_shininess);

        final_colour += (diffuse + specular) * attenuation;
    }

    frag_color = vec4(final_colour, 1.0);
}
```

**Key design decisions in the shader**:

- **Spot light direction handling**: Spot light direction is passed via a separate flat array `u_light_spot_directions[MAX_LIGHTS]`. The fragment shader uses it to compute cone angle falloff via `spot_cone_attenuation()`. The light type is encoded in `u_light_positions_or_dir[i].w` (2 = spot).
- **Ambient * diffuse**: Ambient term is multiplied by the sampled diffuse colour (`u_material_ambient * diffuse_colour`) so that the ambient light has the correct surface colour tint. This is the standard Phong ambient approach.
- **Texture-based diffuse colour**: The fragment shader samples `u_diffuse_texture` for per-pixel diffuse colour. This reuses the texture system from SPEC-017.
- **Default material uniforms**: Reasonable defaults are set in the shader — `u_material_ambient = 0.1`, `u_material_specular = 1.0`, `u_material_shininess = 32.0`.
- **Attenuation for point/spot lights**: Squared distance-normalized falloff: `1 - (clamp(dist/range, 0, 1))^2`.

### 5. glsl_util refactoring (`src/engine/render/glsl_util.h/.cpp`)

Shared utility functions used by both `RenderDeviceOpenGL` and `RenderDeviceHeadless` to avoid duplicated GLSL parsing code.

```cpp
#pragma once

#include <string>
#include <string_view>
#include <unordered_set>

namespace buddd::engine::detail {

/// Extracts uniform names from GLSL source code.
/// Handles:
///   - `uniform type name;`
///   - `uniform type name[N];`        (array — stores base name)
///   - `uniform type name = default_value;`
///   - `uniform type name[N] = default_values;`
/// Strips array suffixes (`[N]`) and default values, returns base names.
/// Also strips layout(...) qualifiers before the uniform keyword.
auto extract_uniform_names(std::string_view glsl_source) -> std::unordered_set<std::string>;

/// Normalizes a uniform name by stripping `[N]` array subscript suffix.
/// Used for internal lookup when the caller holds a name like `u_light_positions_or_dir[0]`
/// and needs to find the base declaration `u_light_positions_or_dir`.
/// Returns the name unchanged if no array subscript suffix is present.
auto normalize_uniform_name(std::string_view name) -> std::string;

} // namespace buddd::engine::detail
```

**`extract_uniform_names()` implementation notes**:

1. Scan for `uniform` keyword.
2. Skip preceding layout qualifier `layout(...)` if present.
3. Skip type token (e.g., `vec4`, `float`, `sampler2D`, `mat4`).
4. Skip optional array suffix `[N]` (including `[N]` where N is a number or identifier).
5. Read variable name (alphanumeric + underscore).
6. Skip optional `= ... ;` default value clause.
7. Register the name if terminated by `;` or `,`.
8. Return `std::unordered_set<std::string>` of base names (no array suffix, no default value).

**`normalize_uniform_name()` implementation notes**:

1. If the name contains `[`, find the `[` position.
2. Return the substring before `[`.
3. If no `[` is found, return a copy of the input.

### 6. RenderSystem extension (`src/engine/render/render_system.cpp`)

The `RenderSystem::render()` method is extended to:

1. **Collect lights**: Before the `MeshRenderer` iteration, iterate `World::each<DirectionalLightComponent>()`, `World::each<PointLightComponent>()`, and `World::each<SpotLightComponent>()` to gather light data. Build an array of `LightData` structs (up to `k_max_lights`). Lights beyond `k_max_lights` are silently ignored (a warning is logged in debug builds).

2. **Flatten into LightData**: Each light type contributes to the same `LightData` array:
   - **Directional**: `position_or_dir.w = 0`, direction from entity forward (-Z) transformed by world rotation.
   - **Point**: `position_or_dir.w = 1`, position from entity world_matrix translation.
   - **Spot**: `position_or_dir.w = 2`, position from entity world_matrix translation, direction from forward (-Z) transformed by rotation. `inner_cone` and `outer_cone` set from component angles (stored as cosines of the angles).

3. **Per-entity lighting uniforms**: For each `MeshRenderer` entity, check if the material has `u_model` (sentinel):
   - If `has_uniform("u_model")` is true, set all lighting uniforms: `u_model`, `u_normal_mat`, `u_camera_pos`, `u_light_count`, flat array uniforms (`u_light_positions_or_dir[i]`, `u_light_colours[i]`, `u_light_ranges[i]`, `u_light_inner_cones[i]`, `u_light_outer_cones[i]`, `u_light_spot_directions[i]`), and material property uniforms.
   - If `has_uniform("u_model")` is false, skip all lighting uniforms (backward compatibility: only `u_mvp` is set).

4. **Normal matrix computation**: For each lit entity, compute `normal_mat = world_mat.inverse().transpose()`.

**Detailed pseudo-code for the extended `render()`**:

```cpp
auto RenderSystem::render() -> void {
    device_->begin_frame();

    auto cam_opt = world_->active_camera();
    if (!cam_opt.has_value()) {
        std::cerr << "RenderSystem: no active camera — rendering skipped\n";
        device_->end_frame();
        return;
    }
    auto& cam_comp = *cam_opt;
    auto vp = cam_comp.camera().view_projection_matrix();
    auto camera_pos = cam_comp.camera().position();

    // --- 1. Collect lights ---
    std::array<detail::LightData, detail::k_max_lights> light_data{};
    int light_count = 0;

    auto collect_light = [&](auto& lc, math::Vec3 position, math::Vec3 direction,
                             float range, float inner_angle, float outer_angle, float type_w) {
        if (light_count >= detail::k_max_lights) {
    #ifndef NDEBUG
            std::cerr << "RenderSystem: max lights (" << detail::k_max_lights
                      << ") reached — ignoring additional lights\n";
    #endif
            return;  // stop collecting
        }
        auto& ld = light_data[light_count];
        if (type_w == 0.0f) {
            // Directional: use direction
            ld.position_or_dir = {direction.x, direction.y, direction.z, 0.0f};
        } else if (type_w == 2.0f) {
            // Spot: use position, store direction separately
            ld.position_or_dir = {position.x, position.y, position.z, type_w};
            ld.spot_direction = {direction.x, direction.y, direction.z, 0.0f};
        } else {
            // Point: use position only
            ld.position_or_dir = {position.x, position.y, position.z, type_w};
        }
        ld.colour = {lc.colour().x * lc.intensity(),
                     lc.colour().y * lc.intensity(),
                     lc.colour().z * lc.intensity(),
                     1.0f};
        ld.range = range;
        ld.inner_cone = std::cos(inner_angle);
        ld.outer_cone = std::cos(outer_angle);
        ++light_count;
    };

    // Directional lights
    world_->each<DirectionalLightComponent>([&](Entity entity, DirectionalLightComponent& lc) -> bool {
        auto world_mat = entity.world_matrix();
        auto forward = math::Vec4{0.0f, 0.0f, -1.0f, 0.0f};
        auto dir = math::normalize(world_mat * forward);
        collect_light(lc, {}, {dir.x, dir.y, dir.z}, 0.0f, 0.0f, 0.0f, 0.0f);
        return light_count < detail::k_max_lights;
    });

    // Point lights
    world_->each<PointLightComponent>([&](Entity entity, PointLightComponent& lc) -> bool {
        auto world_mat = entity.world_matrix();
        auto world_pos = world_mat * math::Vec4{0.0f, 0.0f, 0.0f, 1.0f};
        collect_light(lc, {world_pos.x, world_pos.y, world_pos.z}, {},
                      lc.range(), 0.0f, 0.0f, 1.0f);
        return light_count < detail::k_max_lights;
    });

    // Spot lights
    world_->each<SpotLightComponent>([&](Entity entity, SpotLightComponent& lc) -> bool {
        auto world_mat = entity.world_matrix();
        auto world_pos = world_mat * math::Vec4{0.0f, 0.0f, 0.0f, 1.0f};
        auto forward = math::Vec4{0.0f, 0.0f, -1.0f, 0.0f};
        auto dir = math::normalize(world_mat * forward);
        collect_light(lc, {world_pos.x, world_pos.y, world_pos.z},
                      {dir.x, dir.y, dir.z},
                      lc.range(), lc.inner_angle(), lc.outer_angle(), 2.0f);
        return light_count < detail::k_max_lights;
    });

    // --- 2. Iterate MeshRenderers ---
    world_->each<MeshRenderer>([&](Entity entity, MeshRenderer& mr) -> bool {
        auto world_mat = entity.world_matrix();
        auto mvp = vp * world_mat;
        auto& material = mr.model().material();

        // Always set u_mvp (backward compat). Log warning and skip on failure (per SPEC-011 AC-024).
        auto mvp_result = material.set_uniform("u_mvp", mvp);
        if (!mvp_result) {
            std::cerr << "[warning] Failed to set u_mvp for entity " << entity.id()
                      << ": " << mvp_result.error().message() << "\n";
            return true;  // skip this entity, continue iteration
        }

        // Check if this material supports lighting (has u_model uniform)
        if (material.has_uniform("u_model")) {
            // Set world and normal matrices
            material.set_uniform("u_model", world_mat);
            auto normal_mat_full = world_mat.inverse().transpose();
            material.set_uniform("u_normal_mat", normal_mat_full);

            // Camera position
            material.set_uniform("u_camera_pos", camera_pos);

            // Light count and data
            material.set_uniform("u_light_count", light_count);
            for (int i = 0; i < light_count; ++i) {
                auto const& ld = light_data[i];
                material.set_uniform("u_light_positions_or_dir[" + std::to_string(i) + "]", ld.position_or_dir);
                material.set_uniform("u_light_colours[" + std::to_string(i) + "]",          ld.colour);
                material.set_uniform("u_light_ranges[" + std::to_string(i) + "]",           ld.range);
                material.set_uniform("u_light_spot_directions[" + std::to_string(i) + "]",  ld.spot_direction);
                material.set_uniform("u_light_inner_cones[" + std::to_string(i) + "]",      ld.inner_cone);
                material.set_uniform("u_light_outer_cones[" + std::to_string(i) + "]",      ld.outer_cone);
            }

            // Material defaults
            material.set_uniform("u_material_ambient",        math::Vec3{0.1f, 0.1f, 0.1f});
            material.set_uniform("u_material_specular",       math::Vec3{1.0f, 1.0f, 1.0f});
            material.set_uniform("u_material_shininess",      32.0f);
            material.set_uniform("u_material_diffuse_tint",   math::Vec4{1.0f, 1.0f, 1.0f, 1.0f});
        }

        mr.model().draw(*device_);
        return true;
    });

    device_->end_frame();
}
```

The `has_uniform("u_model")` sentinel approach:
- `PhongMaterial` declares `u_model` in its known uniform list → `has_uniform` returns true → lighting uniforms are set.
- Unlit materials (old cube shaders) do NOT declare `u_model` → `has_uniform` returns false → only `u_mvp` is set.

### 7. Phong demo (`src/cmd/demo/phong_demo.h`, `src/cmd/demo/phong_demo.cpp`)

A new interactive demo registered as `buddd demo phong`:

**Scene setup**:
- A `World` with one entity for the camera (`CameraComponent` at position `(3, 2, 3)` looking at origin).
- A textured cube entity with `MeshRenderer` (using the Standard Vertex and `PhongMaterial`).
  - Vertex data: position (loc 0) + normal (loc 2) + texcoord (loc 3). Colour (loc 1) is default white.
  - The cube has a texture bound via `Material::set_texture("u_diffuse_texture", texture)` — reusing the same checkerboard or demo texture from SPEC-017's textured-cube demo.
- A `PointLightComponent` entity at position `(2, 2, 2)`, colour `(1, 1, 1)`, intensity `1.5`, range `8.0`.
- A `DirectionalLightComponent` entity at a fixed rotation for fill lighting.

**Per-frame update**:
- The point light entity orbits the cube using a time-driven animation: `position = (2 * cos(t), 2 * sin(t) + 1, 2 * sin(t * 0.7))`, updated by setting `entity.transform().position`.
- Free-camera controls: WASD for movement, mouse for look (right-click to capture/release mouse). Interactive exploration.

**Texturing approach**: The cube uses the Standard Vertex with texture coordinates (Float2, loc 3) mapped to the 6 cube faces. Same texture from SPEC-017 or procedural fallback.

**Rendering**: `RenderSystem::render()` handles all lighting automatically. `PhongMaterial` has both a texture bound (`u_diffuse_texture`) and lighting uniforms set.

**Registration**: Wired into `demo_command.cpp` as `"phong"`.

## User stories

### Story 1 — Standard Vertex and PhongMaterial (Priority: P1)

As an engine developer, I want a single `Vertex` struct used by all meshes and a `PhongMaterial` that encapsulates Phong shaders, so that adding lighting to any mesh requires only switching to `PhongMaterial` and providing normals.

**Given** a `World` with an entity that has `CameraComponent`, and an entity with `MeshRenderer` using `Vertex` data (position + normal + texcoord) and `PhongMaterial` with a diffuse texture bound
**When** I call `RenderSystem::render()`
**Then** the cube renders with Phong lighting (ambient + diffuse + specular) using the texture as diffuse colour.

### Story 2 — DirectionalLightComponent (Priority: P1)

As an application developer, I want to create a directional light that casts parallel light in a fixed direction derived from entity rotation.

**Given** a `World` with a lit entity and a `DirectionalLightComponent` entity rotated 45° around Y
**When** `RenderSystem::render()` collects lights
**Then** the light direction is `normalize(rotation_matrix * (0, 0, -1))` and all lit surfaces are illuminated from that direction regardless of position.

### Story 3 — PointLightComponent (Priority: P1)

As an application developer, I want to create a point light that illuminates surfaces within a radius based on its world position.

**Given** a `World` with a lit entity and a `PointLightComponent` entity at position `(5, 3, 1)` with range `10.0`
**When** `RenderSystem::render()` collects lights
**Then** the light position is `(5, 3, 1)` and surfaces within range receive diffuse and specular illumination that attenuates with distance.

### Story 4 — Backward compatibility: unlit demos continue working (Priority: P1)

As an application developer, I want existing unlit demos (triangle, cube, cube-scene, textured-cube) to run without modification after the lighting system is added.

**Given** the original `cube_demo.cpp` (unlit, using `Vertex` with only position+color filled, old shaders)
**When** I run `buddd demo cube`
**Then** the cube renders identically to before — flat per-face colours with no lighting computation. No shader compilation errors, no missing uniform errors.

### Story 5 — Phong demo with orbiting light and free-camera (Priority: P2)

As an application developer, I want a new `buddd demo phong` command that shows a textured cube lit by an orbiting point light, proving the Phong system works end-to-end with interactive exploration.

**Given** the new Phong demo
**When** I run `buddd demo phong`
**Then** a textured cube appears with dynamic lighting from a point light that orbits the cube. The lit face changes as the light moves. I can move around the scene using WASD + mouse (free-camera). A directional fill light provides ambient-like illumination. The demo runs interactively until I close the window.

### Story 6 — Light count limit (Priority: P3)

As an engine developer, I want to define a maximum light count (8) and have additional lights beyond the limit silently ignored.

**Given** a scene with 10 light component entities (mixed types) and a cube with lit material
**When** `RenderSystem::render()` is called
**Then** the first 8 lights are applied to the cube. The remaining 2 are silently ignored (debug build logs a warning). The demo runs without errors.

### Story 7 — SpotLightComponent (Priority: P3)

As an application developer, I want to create a spot light with a conical beam defined by inner and outer angles.

**Given** a `World` with a lit entity and a `SpotLightComponent` entity at position `(0, 2, 0)` rotated to face downwards, with `inner_angle = 0.35` (20°) and `outer_angle = 0.79` (45°)
**When** `RenderSystem::render()` collects lights
**Then** surfaces within the inner cone receive full illumination, surfaces between inner and outer cone receive smooth falloff, and surfaces outside the outer cone receive no illumination from this light.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `Vertex` struct is defined in `src/engine/render/vertex.h` with fields `position` (Vec3, loc 0), `color` (Vec4, loc 1), `normal` (Vec3, loc 2), `texcoord` (Vec2, loc 3), `tangent` (Vec4, loc 4), `texcoord2` (Vec2, loc 5). Stride 72 bytes. `k_standard_vertex_format` constant describes the 6-attribute layout. | Static inspection: file exists, struct layout matches spec, vertex format descriptor compiles. |
| AC-002 | `DirectionalLightComponent` class exists in `src/engine/scene/directional_light_component.h`, inherits `Component`, has `colour` (Vec3) and `intensity` (float) properties with mutable and const accessors. | Code compiles. Unit test: create component, verify initial values, mutate via accessors. |
| AC-003 | `PointLightComponent` class exists in `src/engine/scene/point_light_component.h`, inherits `Component`, has `colour` (Vec3), `intensity` (float), `range` (float, default 10.0) properties with mutable and const accessors. | Code compiles. Unit test: create component, verify initial values, mutate via accessors. |
| AC-004 | `SpotLightComponent` class exists in `src/engine/scene/spot_light_component.h`, inherits `Component`, has `colour` (Vec3), `intensity` (float), `range` (float, default 10.0), `inner_angle` (float, default 0.785), `outer_angle` (float, default 1.047) properties with mutable and const accessors. | Code compiles. Unit test: create component, verify initial values, mutate via accessors. |
| AC-005 | All three light components have `on_attach()` explicitly overridden as a no-op. | Unit test: create each component type, add to entity, verify no crash and no side effects. |
| AC-006 | `PhongMaterial` class exists in `src/engine/render/phong/phong_material.h`, inherits `Material`, implements all pure virtual methods of `Material`. | Code compiles. Unit test: create `PhongMaterial` via `RenderDevice::create_material` or its own constructor, verify it is a valid `Material` subclass. |
| AC-007 | `PhongMaterial` automatically creates its own vertex and fragment shaders from embedded GLSL strings — no external shader creation required. | Code review: constructor does not accept shader parameters. Unit test: create `PhongMaterial`, verify `has_uniform("u_model")` returns true. |
| AC-008 | `PhongMaterial` has convenience setters: `set_camera_position(Vec3)`, `set_lights(const LightData*, int)`, `set_transforms(Mat4, Mat4)`. | Code review: header declares these methods. |
| AC-009 | `PhongMaterial` declares standard known uniforms: `u_mvp`, `u_model`, `u_normal_mat`, `u_camera_pos`, `u_light_count`, `u_light_positions_or_dir`, `u_light_colours`, `u_light_ranges`, `u_light_spot_directions`, `u_light_inner_cones`, `u_light_outer_cones`, `u_material_ambient`, `u_material_specular`, `u_material_shininess`, `u_material_diffuse_tint`, `u_diffuse_texture`. | Code review: `known_uniform_names()` returns the expected set. Unit test (headless): verify `has_uniform()` returns true for each. |
| AC-010 | Embedded Phong vertex shader has inputs `a_position` (loc 0), `a_color` (loc 1), `a_normal` (loc 2), `a_texcoord` (loc 3). Uniforms `u_mvp`, `u_model`, `u_normal_mat`. Outputs `v_world_pos`, `v_normal`, `v_texcoord`. | Static inspection of `src/engine/render/phong/phong_shaders.h`. Headless compile test. |
| AC-011 | Embedded Phong fragment shader has `#define MAX_LIGHTS 8`, flat array uniforms for light data, Phong model (ambient * diffuse + diffuse Lambert + specular Blinn-Phong), `sampler2D u_diffuse_texture`, material property uniforms with defaults. Spot light cone falloff via `u_light_inner_cones`/`u_light_outer_cones`. | Static inspection of shader source. Headless compile test. |
| AC-012 | `glsl_util.h/.cpp` exist in `src/engine/render/` with functions `extract_uniform_names(std::string_view) -> std::unordered_set<std::string>` and `normalize_uniform_name(std::string_view) -> std::string`. | Files exist, compile, and are used by both `RenderDeviceOpenGL` and `RenderDeviceHeadless`. |
| AC-013 | `extract_uniform_names()` handles: `uniform type name;`, `uniform type name[N];`, `uniform type name = default;`, `uniform type name[N] = default;`, and `layout(...) uniform type name;`. Returns base names without array suffixes or default values. | Unit test: parse known GLSL snippets, verify extracted names match expected set. |
| AC-014 | `normalize_uniform_name()` strips `[N]` array subscript suffixes. E.g., `"u_light_positions_or_dir[0]"` → `"u_light_positions_or_dir"`. Returns name unchanged if no array suffix. | Unit test: verify function with array and non-array inputs. |
| AC-015 | `LightData` struct defined in `src/engine/render/light_data.h` with fields: `position_or_dir` (Vec4, .w = type), `colour` (Vec4), `range` (float), `inner_cone` (float), `outer_cone` (float). `k_max_lights = 8`. | Static inspection. Header compiles. |
| AC-016 | `RenderSystem` collects all `DirectionalLightComponent` entities before iterating `MeshRenderer` entities, up to `k_max_lights` (8). Direction from entity world rotation (-Z forward). | Unit test (headless): create world with 3 directional lights and 1 mesh renderer. Call render, verify `u_light_count` uniform is set to 3 on the material. Verify first light direction matches expected rotation. |
| AC-017 | `RenderSystem` collects all `PointLightComponent` entities. Position from entity world_matrix translation. `position_or_dir.w = 1.0`. | Unit test (headless): create entity with `PointLightComponent` at position (5, 3, 1). Verify `u_light_positions_or_dir[0].xyz` matches and `w == 1.0`. |
| AC-018 | `RenderSystem` collects all `SpotLightComponent` entities. Position from translation, direction from rotation, `inner_cone`/`outer_cone` as cosines of angles. `position_or_dir.w = 2.0`. | Unit test (headless): create entity with `SpotLightComponent` at known position/rotation with known angles. Verify position, direction, cone cosines, and type flag. |
| AC-019 | Lights beyond `k_max_lights` (8) across all three types combined are silently ignored (debug build logs warning). | Unit test (headless): create world with 10 lights (e.g., 4 directional, 4 point, 2 spot), verify `u_light_count` is 8. |
| AC-020 | Light colour * intensity is pre-multiplied: `u_light_colours[i] = (colour.r * intensity, colour.g * intensity, colour.b * intensity, 1.0)`. | Unit test (headless): verify uniform values for each light type. |
| AC-021 | `u_normal_mat` is computed as `world_mat.inverse().transpose()` per entity in `RenderSystem::render()`. | Unit test (headless): create entity with known world matrix, call render, verify material `get_uniform_mat4("u_normal_mat")` equals `world_mat.inverse().transpose()`. |
| AC-022 | Backward compatibility: `has_uniform("u_model")` returns false for unlit materials (old cube shaders). | Code review: unlit demos unchanged. Unit test (headless): create unlit material, verify `has_uniform("u_model")` returns false, call render, verify no lighting uniforms are set. |
| AC-023 | `RenderSystem` sets `u_camera_pos` uniform when lighting is enabled (material has `u_model`). | Unit test (headless): verify `get_uniform_vec3("u_camera_pos")` matches camera position from active `CameraComponent`. |
| AC-024 | Material property uniforms (`u_material_ambient`, `u_material_specular`, `u_material_shininess`, `u_material_diffuse_tint`) are set with defaults when lighting is enabled. | Unit test (headless): verify values match documented defaults. |
| AC-025 | Light component entity destruction removes it from the current frame's light list (collected fresh each frame via `World::each<>`). | Unit test (headless): destroy a light entity, flush, call render, verify `u_light_count` decreases. |
| AC-026 | No light component entities exist of any type: lit objects render with only ambient term (`u_material_ambient * diffuse_colour`, defaults to `vec3(0.1)` times texture colour). | Unit test (headless): world with lit cube but no light component entities, call render, verify `final_colour = u_material_ambient * diffuse_colour` and no crash. |
| AC-027 | Phong demo (`buddd demo phong`) renders a textured cube with an orbiting `PointLightComponent` and a static `DirectionalLightComponent` fill, supports free-camera (WASD + mouse) for interactive exploration. | Manual verification: run demo, observe textured lit cube with orbiting light, move with WASD/mouse. |
| AC-028 | Phong shader source file `src/engine/render/phong/phong_shaders.h` exists and contains `constexpr std::string_view` constants for vertex and fragment shaders. | File exists, compiles. |
| AC-029 | `RenderSystem` sets `u_model` uniform (world matrix) when lighting is enabled. | Unit test (headless): verify `get_uniform_mat4("u_model")` matches `entity.world_matrix()`. |
| AC-030 | Phong fragment shader samples diffuse colour from `u_diffuse_texture` sampler and multiplies by `u_material_diffuse_tint`. | Static inspection of shader source. |
| AC-031 | `PhongMaterial::set_camera_position()` sets the `u_camera_pos` uniform. | Unit test (headless): call setter, `get_uniform_vec3("u_camera_pos")` matches. |
| AC-032 | `PhongMaterial::set_lights()` sets all per-light uniforms (position_or_dir, colour, range, inner_cone, outer_cone). | Unit test (headless): create `LightData` array, call `set_lights()`, verify each uniform is set correctly. |
| AC-033 | `PhongMaterial::set_transforms()` sets `u_model`, `u_mvp`, and `u_normal_mat` uniforms. | Unit test (headless): call setter, verify all three uniforms match expected values. |
| AC-034 | Existing unlit demos compile and render without modification: `triangle`, `cube`, `cube-scene`, `textured-cube`, `free-camera`. | Manual verification for each demo. CI compile check. |
| AC-035 | No new SDL3, OpenGL, or GLM types are exposed in public engine headers — all new types use only abstract wrappers (`Vec3`, `Vec4`, `Mat4`). | Code review: public headers do not include backend headers. |
| AC-036 | `Material::set_uniform("u_light_positions_or_dir[0]", someVec4)` succeeds in headless backend — flattened array bracket-syntax naming convention works with the material layer. | Unit test (headless): call `material.set_uniform("u_light_positions_or_dir[0]", Vec4{1,1,1,1})`, verify `get_uniform_vec4("u_light_positions_or_dir[0]")` returns the expected value after normalization. |
| AC-037 | Existing `MaterialHeadless` supports uniform name normalization via `normalize_uniform_name()` so that bracket-syntax uniform names resolve to their base declarations. | Unit test (headless): set uniform with bracket syntax, verify `has_uniform("u_light_positions_or_dir")` returns true. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | An application developer can create a lit scene by adding light component entities to a `World` — no per-frame light management or manual uniform setup. `RenderSystem` handles all lighting automatically. | Code review of a minimal program: create world, add camera entity, add lit cube entity, add light entity, call `render()` in a loop. |
| SC-002 | All new tests pass in headless CI (no GPU, no display). | `cmake --build --preset debug && ctest --preset debug` — all lighting tests pass. |
| SC-003 | The `buddd demo phong` demo runs and displays a cube with dynamic lighting from an orbiting point light, visually distinct from the unlit cube demo, with directional fill light. | Manual visual verification. |
| SC-004 | All existing demos (`buddd demo triangle`, `buddd demo cube`, `buddd demo cube-scene`, `buddd demo textured-cube`, `buddd demo free-camera`) continue to work identically to before. | Manual visual verification for each demo. |
| SC-005 | No memory leaks in any lighting-related code path (light component lifecycle, light data collection, shader creation/destruction, PhongMaterial). | ASAN build shows no leaks in a test that creates and destroys 100 worlds with lights and lit mesh renderers. |
| SC-006 | The lighting system adds no measurable overhead to unlit rendering paths (unlit materials are skipped via `has_uniform("u_model")` check, no light collection overhead for unlit-only scenes). | Unit test (headless): measure draw call count and frame count in an unlit-only scene before and after SPEC-018 — identical numbers. |

## Edge cases

| Case | Expected behaviour |
|---|---|
| No light component entities exist of any type | `u_light_count` = 0. The ambient term (`u_material_ambient * diffuse_colour`, defaults to `vec3(0.1) * texture_colour`) is still applied. No crash. |
| More than `k_max_lights` (8) lights across all types | First 8 are collected; remainder silently ignored. Debug build logs warning to `std::cerr`. |
| Light ordering in the uniform array | Lights are collected in type-group order: first directional, then point, then spot; within each type, entity creation order. The order is **not guaranteed** to be stable across engine versions — do not rely on a specific light index for visual correctness. |
| Directional light with identity rotation | Direction is `(0, 0, -1)` (forward = -Z). |
| Directional light with non-uniform scale on entity | Scale distorts direction. The `RenderSystem` normalizes the direction vector after `world_mat * Vec4(0,0,-1,0)` to mitigate mild scale distortion. |
| Point light at origin `(0, 0, 0)` | Valid. Light contributes from origin. |
| Point light with zero `range` | Division by zero in shader (`dist / range`). The spec documents that `range` must be > 0. Default is 10.0. Zero or negative range is undefined behaviour. |
| Point light with negative `range` | Undefined behaviour (same as zero range). |
| Light component with zero intensity | Light contributes nothing (colour * 0 = black). Valid — effectively disables the light. |
| Light component added after `RenderSystem` creation | Collected on the next `render()` call (lights are gathered fresh each frame). No issue. |
| Light component destroyed during render | Not possible — `each<>` iterations are before `each<MeshRenderer>`. The light list is captured once per frame. |
| Entity with both `MeshRenderer` (lit) and a light component | Both roles: the entity acts as a light source and is also rendered as a lit object. Since lights are collected first, the entity's own light WILL affect itself. Accepted behaviour (e.g., a glowing object). |
| Lit material without vertex normals (no `a_normal` attribute in vertex buffer) | Undefined behaviour (GPU reads uninitialized data for `a_normal`). The vertex buffer must bind location 2 for lit materials. This is a programmer error. |
| Lit material without `u_model` uniform | Detected via `has_uniform("u_model")` — lighting uniforms are skipped, only `u_mvp` is set. Renders as unlit. |
| `RenderSystem::render()` with both lit and unlit materials in the same scene | Works correctly: `has_uniform("u_model")` check per-entity. Lit entities get all lighting uniforms; unlit entities get only `u_mvp`. |
| Camera position not available (e.g., `CamComponent` detached during frame) | Camera is captured once per frame at the top of `render()`. If camera is detached during `each<MeshRenderer>`, the stored `vp` and `camera_pos` reflect the state at the start of the frame. |
| Lit material without `u_diffuse_texture` bound (no texture set via `set_texture`) | Sampling an unbound texture produces undefined results. The demo always binds a texture. A default white texture should be bound for production use (future improvement, not v1). |
| Spot light with `inner_angle >= outer_angle` | The cone falloff function computes `(cos_angle - cos_outer) / (cos_inner - cos_outer)`. If `inner_angle >= outer_angle`, then `cos_inner <= cos_outer` and the denominator is zero or negative, producing undefined attenuation. This is a programmer error — `inner_angle` must be strictly less than `outer_angle`. |
| Mixed light types totalling > 8 | First 8 collected across all types. The collection order is: directional, point, spot. |
| All three light component types on the same entity | Multiple components on a single entity is allowed (Component design does not restrict multiple components per entity type). Each is collected independently by its type's `each<>` iteration. The lights will be at the same position/direction. This is valid but unusual. |

## Error cases

| Case | Expected behaviour |
|---|---|
| `set_uniform("u_light_count", ...)` fails | The entity is skipped (draw call not issued). Warning logged to `std::cerr`. Other entities are rendered normally. |
| `set_uniform` for any lighting uniform fails | Same as above: entity skipped, warning logged. Other entities unaffected. |
| `world_mat.inverse()` fails (singular matrix, all-zero scale) | `Mat4::inverse()` returns a mathematically undefined matrix. If scale is `(0,0,0)`, the normal matrix is degenerate and lighting is undefined. Documented as undefined behaviour. |
| Light component `range` <= 0 | Undefined behaviour (division by zero in shader). Documented precondition. |
| `a_normal` attribute absent from lit vertex format | GPU undefined behaviour (reading uninitialized vertex attribute). |
| Phong shader compilation fails | `device.create_shader()` returns `Result<Shader>` — failure handled by demo code (print error and exit). |
| `Material::set_texture("u_diffuse_texture", ...)` fails | Texture binding failure is handled by the demo (print error and exit). For programmatic use, the material renders without a texture (undefined sampler behaviour). |
| `PhongMaterial` constructor fails (shader creation fails) | Constructor returns via `Result<>` or throws — follows the same error pattern as existing `Material` creation in the codebase. |
| `SpotLightComponent` with `inner_angle >= outer_angle` | Undefined cone falloff behaviour. Documented precondition. |

## Permissions and security

- No elevated privileges required.
- No network, filesystem, or secret access involved.
- No new third-party dependencies beyond those already in use (GLM remains the only math library, stb_image for texture loading from SPEC-017).
- Architecture boundary (CONST-001) is maintained: new public headers include only `math/*.h` and `scene/component.h` — no backend-specific types.
- `phong/phong_shaders.h` is an internal file containing only GLSL string literals — no headers leaked.
- Light component properties (colour, intensity, range, angles) are pure data with no security implications.
- The fixed maximum light count (8) prevents unbounded uniform allocation.
- Texture sampling (`u_diffuse_texture`) is a read-only GPU resource with no new security implications beyond those already established by SPEC-017.
- `glsl_util.h` is an internal utility header with no public API exposure.

## Observability

All observability uses `std::cerr`, consistent with the project pattern.

| Signal | Source |
|---|---|
| `RenderSystem::render()` — max lights reached (debug only) | `std::cerr << "RenderSystem: max lights (8) reached — ignoring additional lights\n"` |
| `RenderSystem::render()` — lighting uniform failure per entity | `std::cerr << "RenderSystem: set_uniform(<name>) failed for entity <id>: <error>\n"` |
| `RenderSystem::render()` — light count per frame (debug only) | `std::cerr << "RenderSystem: collected " << count << " lights\n"` |
| Phong demo — light entity position updates | Not logged (hot path). |
| Light component `on_attach()` | Not logged (no-op). |
| Lit material — texture binding | Not explicitly logged in v1. SPEC-017's texture system handles binding internally. |
| Phong demo — free-camera status | Not logged (hot path). |

## Out of scope

- Shadow mapping or shadow volumes of any kind.
- Physically-based rendering (PBR) materials (metalness, roughness, IBL).
- Light cookies, gobos, or projector textures.
- Light culling, clustering, or spatial partitioning (frustum, portal, PVS).
- Deferred rendering or light prepass.
- HDR rendering, tone mapping, or exposure control.
- Light animation system or timeline-based light control.
- Per-vertex colour as diffuse input for the lit shader path (diffuse comes exclusively from texture sampling).
- Normal mapping, bump mapping, or displacement mapping.
- Environment maps, reflection probes, or specular IBL.
- Volumetric lighting or light shafts.
- Light gizmos or debug visualization.
- Editor UI for light properties.
- Light batching or instancing optimizations.
- GPU light management beyond the uniform-array approach (SSBOs, UBOs).
- Changes to `CameraComponent`, `World`, `Entity`, or `Component` public APIs.
- Changes to existing `MeshRenderer` or `Material` abstract interfaces.
- Changes to existing unlit demos.
- GLSL struct-array uniform member access (using flat arrays instead — see design).
- Spot light cone visualization or debug rendering.
- Default white texture for untextured materials.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `math::Camera::position()` returns `math::Vec3` — confirmed by code inspection of `src/engine/math/camera.h` line 19. |
| A-02 | `math::Mat4::inverse()` and `math::Mat4::transpose()` are implemented and correct (from SPEC-004). |
| A-03 | `math::Vec4` is constructible from `(float, float, float, float)` and has public `x`, `y`, `z`, `w` members. |
| A-04 | `Material::has_uniform(name)` correctly returns `false` for uniforms not present in the shader (both OpenGL and Headless backends). |
| A-05 | `Material::set_uniform(name, const Vec3&)` and `Material::set_uniform(name, const Vec4&)` are available and functional (from SPEC-005). |
| A-06 | `MaterialOpenGL::bind()` supports flush of cached uniforms named with array subscript syntax (e.g., `"u_light_positions_or_dir[0]"`). If not supported, the implementation falls back to a manual naming scheme (e.g., `"u_light_0_pos_or_dir"`). |
| A-07 | For directional lights, the direction is extracted from the entity's rotation-only component. The implementation uses `world_mat * Vec4(dir, 0)` for direction and normalizes after transform. Non-uniform scale is accepted as an edge case. |
| A-08 | The `Material::set_uniform(Mat4)` overload works for setting the normal matrix as a full 4×4. The shader uses `mat3(u_normal_mat)` to extract the upper-left 3×3. |
| A-09 | `MaterialHeadless` currently has `get_uniform_mat4()` and `get_texture()` diagnostic accessors. SPEC-018 may need to add `get_uniform_vec3()`, `get_uniform_vec4()`, `get_uniform_float()`, and `get_uniform_int()` accessors for testing. The existing `uniform_values_` map already stores float/int/Vec3/Vec4/Mat4 variants. |
| A-10 | The new demo (`phong_demo`) follows the same pattern as `cube_scene_demo` and `textured_cube_demo` — uses `World` + `Entity` + `RenderSystem`. |
| A-11 | All new files are automatically discovered by the existing `file(GLOB_RECURSE CONFIGURE_DEPENDS ...)` pattern in CMakeLists.txt files — no CMake changes needed for new files. |
| A-12 | New files to be created: `src/engine/render/vertex.h`, `src/engine/render/light_data.h`, `src/engine/render/glsl_util.h`, `src/engine/render/glsl_util.cpp`, `src/engine/render/phong/phong_material.h`, `src/engine/render/phong/phong_material.cpp`, `src/engine/render/phong/phong_shaders.h`, `src/engine/scene/directional_light_component.h`, `src/engine/scene/directional_light_component.cpp`, `src/engine/scene/point_light_component.h`, `src/engine/scene/point_light_component.cpp`, `src/engine/scene/spot_light_component.h`, `src/engine/scene/spot_light_component.cpp`, `src/cmd/demo/phong_demo.h`, `src/cmd/demo/phong_demo.cpp`. Modified files: `src/engine/render/render_system.cpp`, `src/cmd/commands/demo_command.cpp`, `src/engine/render/material_headless.h`, `src/engine/render/material_headless.cpp`, `src/engine/render/render_device_headless.cpp` (migrate to glsl_util), `src/engine/render/render_device_opengl.cpp` (migrate to glsl_util). |
| A-13 | Tests for SPEC-018 live in a new file `tests/lighting_tests.cpp`. New file follows the `*_tests.cpp` pattern for automatic CMake discovery. |
| A-14 | The GLSL `mat3(u_normal_mat)` conversion in the vertex shader correctly extracts the upper-left 3×3 of the 4×4 normal matrix for transforming normals. |
| A-15 | The headless material backend (`MaterialHeadless`) supports `has_uniform` lookups for arbitrary uniform names by checking the shader's known uniform list plus any previously set uniform names. |
| A-16 | GLSL uniform array naming convention for the flat-array approach is `u_light_positions_or_dir[0]`, `u_light_positions_or_dir[1]`, etc. `MaterialOpenGL::set_uniform` resolves via `glGetUniformLocation` using bracket syntax, which GLSL/OpenGL supports natively. |
| A-17 | The `Material::set_texture()` interface from SPEC-017 binds the texture to the `u_diffuse_texture` sampler uniform. The texture system's `bind()` call handles the OpenGL texture unit and sampler binding. |
| A-18 | Unlit materials MUST NOT declare a uniform named `u_model`. The `has_uniform("u_model")` check is the sentinel that distinguishes lit from unlit materials. |
| A-19 | `u_material_ambient` defaults to `vec3(0.1)` in the fragment shader. Ambient is multiplied by sampled diffuse colour. |
| A-20 | The demo texture (`assets/brick.png` or procedural checkerboard) may not exist yet — implementation should include a procedural checkerboard fallback texture generated at runtime. |
| A-21 | `std::cos` is available via `<cmath>` for converting spot light angles to cosines in the RenderSystem. |
| A-22 | `PhongMaterial` embeds its own shaders and manages its own `MaterialOpenGL` or `MaterialHeadless` internally. The `Material` interface is used polymorphically. |

## Open questions

All open questions from the initial design have been resolved:

| ID | Resolution |
|---|---|
| Q-01 | ✅ `Camera::position()` exists in `src/engine/math/camera.h` line 19. |
| Q-02 | ✅ `MaterialHeadless` needs `get_uniform_vec3()`, `get_uniform_vec4()`, `get_uniform_float()`, and `get_uniform_int()` added. |
| Q-03 | ✅ Demo is interactive (free-camera: WASD + mouse). |
| Q-04 | ✅ Diffuse colour comes from texture sampling. Per-vertex colour is NOT used in the lit path. An optional `u_material_diffuse_tint` uniform provides global tint. |
| Q-05 | Spot light direction stored in a separate flat array `u_light_spot_directions[MAX_LIGHTS]` (Vec4). Append-only — adds 4 floats per light for spot-specific direction. Directional/point ignore this array. ✅ Décidé: tableau séparé. |
| Q-06 | `PhongMaterial(RenderDevice&, ...)` — self-contained. Shaders créés en interne, pas de `create_material()` externe. ✅ |
| Q-07 | Remplacement incrémental: les démos existantes continuent avec leurs données actuelles (exprimées via `Vertex` avec champs pertinents remplis). Les nouvelles utilisent `Vertex` complet. ✅ |
