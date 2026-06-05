# IMPL-018-002 — Phong Lighting System (Rearchitected)

## Source spec

`.specs/sprint-2026-05/lighting/spec.md` (SPEC-018, accepted)

## Goal

Implement a Phong lighting system according to the rearchitected SPEC-018: a standard `Vertex` struct (72B stride) in `src/engine/render/vertex.h` used by ALL meshes; a `phong/` module with `PhongMaterial` (self-contained `Material` subclass + embedded GLSL shaders) and `phong_shaders.h`; three separate light components (`DirectionalLightComponent`, `PointLightComponent`, `SpotLightComponent`) each deriving `Component`; `glsl_util.h/.cpp` with shared `extract_uniform_names()` and `normalize_uniform_name()` for both OpenGL and Headless backends; `RenderSystem` extended to collect all three light types into flattened `LightData` array (max 8) and set lighting uniforms; procedural checkerboard / asset-backed `buddd demo phong` demo with orbiting point light + directional fill + interactive free-camera. All existing unlit demos remain unchanged.

## Non-goals

- No shadow mapping, PBR, light cookies, light culling, deferred rendering, HDR/tone mapping, light animation system, per-vertex colour as diffuse input, editor UI, or debug visualization.
- No changes to existing unlit demos (`triangle`, `cube`, `cube-scene`, `textured-cube`, `free-camera`).
- No changes to `MeshRenderer`, `Material`, `CameraComponent`, `World`, `Entity`, or `Component` abstract interfaces.
- No changes to `MaterialOpenGL` or `ShaderOpenGL` — only `MaterialHeadless` diagnostic accessors + array subscript normalization are added.
- No change to the existing `RenderDevice::create_material()` API — `PhongMaterial` uses its own constructor, not `create_material()`.
- No CMake changes — `file(GLOB_RECURSE)` auto-discovers new files.

## Relevant constitution rules

- **CONST-001** (`docs/constitution/rules/CONST-001-architecture-boundaries.md`): No SDL3/OpenGL/GLM types in public engine headers. All new `scene/` headers use only `math/*.h` and `scene/component.h`. Backend types stay in `src/engine/render/`.

## Relevant ADRs

- **ADR-003** (Render Pipeline Architecture): Draw methods return `void`; interface-backend pattern.
- **ADR-005** (Optional Ref Component API): Accessor patterns for component members.
- **ADR-010** (No Raw Pointers in Public API): All public API uses references, `optional<T&>`, `shared_ptr`, or value types. The `PhongMaterial` public header must not expose raw pointers.

## Files to inspect

The Code Agent MUST read ALL of these files before making any edits:

1. `src/engine/scene/component.h` — base Component class
2. `src/engine/scene/camera_component.h` — CameraComponent pattern (default ctor, accessor pair, on_attach override, destructor that accesses world_)
3. `src/engine/scene/camera_component.cpp` — CameraComponent implementation pattern
4. `src/engine/render/material.h` — abstract Material interface (6 set_uniform overloads, has_uniform, set_texture, bind)
5. `src/engine/render/material_headless.h` — existing diagnostic getters, uniform value variant types
6. `src/engine/render/material_headless.cpp` — all 6 set_uniform overloads, has_uniform, get_uniform_mat4 implementation patterns
7. `src/engine/render/render_device_headless.cpp` — `create_material()` collects known uniforms via `extract_uniform_names()`
8. `src/engine/render/render_device_opengl.cpp` — `create_material()` currently ignores `known_uniforms`
9. `src/engine/render/render_system.h` — existing RenderSystem interface
10. `src/engine/render/render_system.cpp` — existing render() method
11. `src/engine/render/vertex_format.h` — VertexFormat, VertexAttribute, VertexAttributeType definitions
12. `src/engine/render/model.h` — Model class (create, create_indexed, draw)
13. `src/engine/render/mesh_renderer.h` — MeshRenderer component
14. `src/cmd/demo/demo_helpers.h` — CubeResources, setup_triangle(), setup_cube() declarations
15. `src/cmd/demo/demo_helpers.cpp` — existing vertex data patterns, shader creation, vertex format, model creation
16. `src/cmd/demo/textured_cube_demo.cpp` — textured cube demo pattern (texture loading, vertex format with texcoords, RenderSystem, frame loop)
17. `src/cmd/demo/free_camera_demo.cpp` — interactive demo pattern (WASD + mouse free-camera, poll_events, delta_time, input_system)
18. `src/cmd/demo/free_camera_demo.h` — demo header pattern (forward declare RenderDevice)
19. `src/cmd/commands/demo_command.cpp` — dispatch chain, validation list, usage text
20. `src/engine/scene/entity.h` — `world_matrix()` returns `math::Mat4`, `transform()` accessor
21. `src/engine/scene/transform.h` — Transform struct with position, rotation, scale, local_matrix(), world_matrix()
22. `src/engine/scene/world.h` — `each<T>()`, `active_camera()`, `register_camera()`, `create_entity()`
23. `tests/scene_rendering_tests.cpp` — headless test patterns, `make_headless_engine()` helper, `MaterialHeadless` dynamic_cast pattern
24. `docs/adr/003-render-pipeline-architecture.md` — draw returns void
25. `docs/adr/010-no-raw-pointers-in-public-api.md` — no raw pointers in public API

## Files allowed to change

### New files to create (18 files):

- `src/engine/render/vertex.h` — Standard Vertex struct + `k_standard_vertex_format`
- `src/engine/render/glsl_util.h` — `extract_uniform_names()` + `normalize_uniform_name()` declarations
- `src/engine/render/glsl_util.cpp` — implementations
- `src/engine/render/light_data.h` — `LightData` struct + `k_max_lights`
- `src/engine/render/phong/phong_shaders.h` — Embedded GLSL 450 core shader string constants
- `src/engine/render/phong/phong_material.h` — `PhongMaterial` class (Material subclass)
- `src/engine/render/phong/phong_material.cpp` — PhongMaterial implementation
- `src/engine/scene/directional_light_component.h` — DirectionalLightComponent
- `src/engine/scene/directional_light_component.cpp` — implementation
- `src/engine/scene/point_light_component.h` — PointLightComponent
- `src/engine/scene/point_light_component.cpp` — implementation
- `src/engine/scene/spot_light_component.h` — SpotLightComponent
- `src/engine/scene/spot_light_component.cpp` — implementation
- `src/cmd/demo/phong_demo.h` — Phong demo header
- `src/cmd/demo/phong_demo.cpp` — Phong demo implementation
- `tests/lighting_tests.cpp` — Headless tests

### Existing files to modify (7 files):

- `src/engine/render/material_headless.h` — Add `get_uniform_vec3()`, `get_uniform_vec4()`, `get_uniform_float()`, `get_uniform_int()` declarations
- `src/engine/render/material_headless.cpp` — Implement 4 new diagnostic accessors; normalize array subscript in `set_uniform()`, `has_uniform()`, and all diagnostic getters using `glsl_util::normalize_uniform_name()`
- `src/engine/render/render_device_headless.cpp` — Replace local `extract_uniform_names()` with `#include "render/glsl_util.h"` and call `detail::extract_uniform_names()`
- `src/engine/render/render_device_opengl.cpp` — Replace local uniform name extraction (if any) with `#include "render/glsl_util.h"`; add known_uniforms parsing from shader source for material creation
- `src/engine/render/render_system.cpp` — Extend `render()` with light collection and lighting uniform setting
- `src/cmd/demo/demo_helpers.cpp` — Update `setup_cube()` and `setup_triangle()` to use `Vertex` struct (position+color, other fields zero); update vertex format to use `k_standard_vertex_format` attributes for position+color
- `src/cmd/commands/demo_command.cpp` — Add `"phong"` to validation list, dispatch, usage text, include `phong_demo.h`

## Files forbidden to change

- `src/engine/render/material.h` — abstract Material interface
- `src/engine/render/shader.h`, `shader_opengl.*`, `shader_headless.*` — shader interfaces
- `src/engine/render/material_opengl.*` — OpenGL material backend
- `src/engine/render/vertex_format.h` — vertex format definition
- `src/engine/render/vertex_buffer.h`, `vertex_buffer_opengl.*`, `vertex_buffer_headless.*`
- `src/engine/render/index_buffer.h`, `index_buffer_opengl.*`, `index_buffer_headless.*`
- `src/engine/render/model.*` — model abstraction
- `src/engine/render/mesh_renderer.*` — mesh renderer component
- `src/engine/render/render_device.h` — device abstraction
- `src/engine/render/render_device_headless.h` — headless device header
- `src/engine/render/render_device_opengl.h` — OpenGL device header
- `src/engine/render/texture.h`, `texture_opengl.*`, `texture_headless.*` — texture system
- `src/engine/scene/component.h` — base component
- `src/engine/scene/entity.*`, `entity_id.h`, `world.*` — ECS core
- `src/engine/scene/camera_component.*` — camera component
- `src/engine/scene/transform.h` — transform struct
- `src/engine/math/*` — math library
- `src/cmd/demo/triangle_demo.*`, `cube_demo.*`, `cube_scene_demo.*`, `textured_cube_demo.*`, `free_camera_demo.*` — existing demos
- Any existing `tests/` file other than the new `tests/lighting_tests.cpp`
- `docs/` files other than this contract and `.specs/sprint-2026-05/lighting/coordination.md`

## Existing conventions to follow

1. **Namespace**: `buddd::engine` for engine code, `buddd::cmd::demo` for demo code. Inside `.cpp` files use `namespace be = buddd::engine;` alias.
2. **Include style**: Project-local headers via `"render/render_system.h"`, not path-relative.
3. **Accessor pattern**: Mutable + const overloads using trailing return type, matching `CameraComponent::camera()` pattern:
   ```cpp
   auto colour() noexcept -> math::Vec3&;
   auto colour() const noexcept -> const math::Vec3&;
   ```
4. **Component lifecycle**: Light components DO NOT override destructor (base `~Component() = default` suffices). `on_attach()` is explicitly overridden as a no-op `{}`. Light components do NOT register/unregister with World (unlike CameraComponent).
5. **Embedded shaders**: `constexpr std::string_view` constants in a header file as C++ raw string literals.
6. **Demo function signature**: `auto run_phong_demo(RenderDevice& device, int argc, const char* const* argv) -> int` — matching existing demo pattern.
7. **Demo header**: Forward-declare `RenderDevice`, no backend headers (CONST-001).
8. **Test pattern**: `TEST_CASE("Description", "[lighting]")` in `tests/lighting_tests.cpp`. Helper `make_headless_engine()` defined in anonymous namespace in `lighting_tests.cpp`.
9. **Headless uniform query**: Cast `Material*` to `MaterialHeadless*` via `dynamic_cast`, exactly as done in `scene_rendering_tests.cpp` line 454.
10. **Error handling**: `Result<T>` pattern for factory methods. `std::exit(EXIT_FAILURE)` on fatal errors (matching existing demo code).
11. **Model creation**: Use `Model::create_indexed()` for indexed geometry.
12. **Textured demo pattern**: `Image::load` → `device.create_texture()` → `material.set_texture()`.
13. **`be::math` vs `math`**: Use `be::math::Vec3`, `be::math::Mat4` etc. in `.cpp` files when `be = buddd::engine` is aliased. In headers, use `math::Vec3` (the namespace is `buddd::engine::math`).
14. **Inline namespace for detail**: Internal implementation details go in `namespace buddd::engine::detail { ... }`.
15. **Light component file naming**: One file per class, lowercase + underscores: `directional_light_component.h/.cpp`, `point_light_component.h/.cpp`, `spot_light_component.h/.cpp`.

## Required implementation behavior

### 1. `src/engine/render/vertex.h` (NEW)

Define the standard Vertex struct used by ALL meshes:

```cpp
#pragma once

#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

namespace buddd::engine {

struct Vertex {
    math::Vec3 position;     // offset 0   (12B)  location 0
    math::Vec4 color;        // offset 12  (16B)  location 1
    math::Vec3 normal;       // offset 28  (12B)  location 2
    math::Vec2 texcoord;     // offset 40  (8B)   location 3
    math::Vec4 tangent;      // offset 48  (16B)  location 4  (reserved)
    math::Vec2 texcoord2;    // offset 64  (8B)   location 5  (reserved)
};
static_assert(sizeof(Vertex) == 72, "Vertex must be 72 bytes");

/// Vertex format descriptor for the standard Vertex.
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
- Existing demos fill only `position` + `color` (leave other fields zero). The GPU safely ignores unbound attribute locations.
- Phong material uses `position` (loc 0) + `normal` (loc 2) + `texcoord` (loc 3). Ignores `color`, `tangent`, `texcoord2`.

### 2. `src/engine/render/glsl_util.h/.cpp` (NEW)

Shared GLSL utility functions used by both `RenderDeviceOpenGL` and `RenderDeviceHeadless`.

**`glsl_util.h`**:
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
/// Also handles `layout(...) uniform type name;` by skipping the layout qualifier.
/// Returns base names (no array suffix, no default value).
auto extract_uniform_names(std::string_view glsl_source) -> std::unordered_set<std::string>;

/// Strips trailing `[N]` array subscript suffix.
/// E.g., "u_light_colours[3]" → "u_light_colours".
/// Returns name unchanged if no array subscript suffix.
auto normalize_uniform_name(std::string_view name) -> std::string;

} // namespace buddd::engine::detail
```

**`glsl_util.cpp`** — implement both functions:

For `extract_uniform_names()`:
1. Scan for `uniform` keyword (word boundary: not preceded by alphanumeric or `_`).
2. Skip preceding `layout(...)` qualifier if present before `uniform`.
3. Skip type token (alphanumeric + underscore sequence after `uniform`).
4. Skip whitespace.
5. Skip optional `[N]` array suffix (if `[` is found, skip to `]` and past it). This skip happens **before** reading the variable name — **but only for layout/type parsing**. Actually, this is wrong. The array suffix comes AFTER the variable name in GLSL. Let me be precise:

The correct parsing order:
1. Find `uniform` keyword.
2. Skip type (e.g., `vec4`, `float`, `sampler2D`, `mat4`).
3. Skip whitespace.
4. Read variable name (alphanumeric + underscore).
5. Skip optional `[N]` array suffix **after** the name.
6. Skip whitespace.
7. Skip optional `= ... ;` default value clause.
8. If terminated by `;` or `,`, register the name.

For `normalize_uniform_name()`:
1. Find `[` in the name.
2. If found, return substring before `[`.
3. If not found, return a copy of the input.

**Important**: The existing `extract_uniform_names()` function in `render_device_headless.cpp` (lines 118-193) has **two bugs**: (a) it skips `[N]` before the variable name instead of after, and (b) it does not handle `= default_value;` syntax. The implementation in `glsl_util.cpp` must be correct. See the test cases in `tests/lighting_tests.cpp` for verification.

After creating `glsl_util.h/.cpp`:
- `render_device_headless.cpp`: Remove the local `extract_uniform_names()` function (lines 118-193). Add `#include "render/glsl_util.h"` and change calls to use `detail::extract_uniform_names()`.
- `render_device_opengl.cpp`: Add `#include "render/glsl_util.h"` and use `detail::extract_uniform_names()` in `create_material()` to parse known uniforms from shader source. Currently `create_material()` ignores the `known_uniforms` span — extend it to collect uniforms from both shader sources.

### 3. Light components (3 new pairs of files)

All three components follow the same pattern as `CameraComponent`:
- Derive `Component`.
- Default constructor + parameterized constructor.
- Mutable + const accessor pairs returning references for non-trivial types, values for trivially-copyable types (float).
- `on_attach() -> void override {}` — documented no-op.
- Do NOT declare a destructor (compiler-generated default is sufficient — no resources, no world registration).

#### 3a. `DirectionalLightComponent` (`src/engine/scene/directional_light_component.h/.cpp`)

```cpp
#pragma once

#include "math/vec3.h"
#include "scene/component.h"

namespace buddd::engine {

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

Implementation in `.cpp`:
```cpp
DirectionalLightComponent::DirectionalLightComponent(math::Vec3 colour, float intensity)
    : colour_(colour), intensity_(intensity) {}

auto DirectionalLightComponent::colour() noexcept -> math::Vec3& { return colour_; }
auto DirectionalLightComponent::colour() const noexcept -> const math::Vec3& { return colour_; }
auto DirectionalLightComponent::intensity() noexcept -> float& { return intensity_; }
auto DirectionalLightComponent::intensity() const noexcept -> float { return intensity_; }
```

#### 3b. `PointLightComponent` (`src/engine/scene/point_light_component.h/.cpp`)

Same as DirectionalLightComponent plus `range()`:

```cpp
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
```

#### 3c. `SpotLightComponent` (`src/engine/scene/spot_light_component.h/.cpp`)

Same as PointLightComponent plus `inner_angle()` and `outer_angle()` (in radians):

```cpp
class SpotLightComponent : public Component {
public:
    explicit SpotLightComponent(
        math::Vec3 colour = math::Vec3{1.0f, 1.0f, 1.0f},
        float intensity = 1.0f,
        float range = 10.0f,
        float inner_angle = 0.785f,    // ~45 degrees
        float outer_angle = 1.047f     // ~60 degrees
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
```

### 4. `LightData` struct (`src/engine/render/light_data.h`)

Create as a detail header:

```cpp
#pragma once

#include "math/vec4.h"
#include "math/vec2.h"

#include <cstdint>

namespace buddd::engine::detail {

constexpr int k_max_lights = 8;

/// Per-light data packed for GPU uniform passing.
/// Each field maps to the corresponding GLSL flat array.
struct LightData {
    math::Vec4 position_or_dir; // .xyz = position/direction, .w = type: 0=directional, 1=point, 2=spot
    math::Vec4 colour;          // .rgb = colour * intensity pre-multiplied, .a = unused
    float range;                // Attenuation range (ignored for directional)
    math::Vec4 spot_direction;  // For spot lights: normalized direction (.w unused)
    float inner_cone_cos;       // Cosine of inner half-angle (spot only)
    float outer_cone_cos;       // Cosine of outer half-angle (spot only)
};
static_assert(sizeof(LightData) == sizeof(math::Vec4) * 3 + sizeof(float) * 3,
              "LightData struct size must match packed layout");

} // namespace buddd::engine::detail
```

### 5. `src/engine/render/phong/` module

#### 5a. `phong_shaders.h` (NEW)

Embedded GLSL 450 core shader strings as `constexpr std::string_view` constants:

```cpp
#pragma once

#include <string_view>

namespace buddd::engine::detail {

constexpr std::string_view k_phong_vertex_shader_source = R"(#version 450 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec2 a_texcoord;

out vec3 v_world_pos;
out vec3 v_normal;
out vec2 v_texcoord;

uniform mat4 u_mvp;
uniform mat4 u_model;
uniform mat4 u_normal_mat;

void main() {
    vec4 world_pos = u_model * vec4(a_position, 1.0);
    v_world_pos = world_pos.xyz;
    v_normal = normalize(mat3(u_normal_mat) * a_normal);
    v_texcoord = a_texcoord;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

constexpr std::string_view k_phong_fragment_shader_source = R"(#version 450 core

#define MAX_LIGHTS 8

in vec3 v_world_pos;
in vec3 v_normal;
in vec2 v_texcoord;

out vec4 frag_color;

uniform int      u_light_count;
uniform vec4     u_light_positions_or_dir[MAX_LIGHTS];
uniform vec4     u_light_colours[MAX_LIGHTS];
uniform float    u_light_ranges[MAX_LIGHTS];
uniform vec4     u_light_spot_directions[MAX_LIGHTS];
uniform float    u_light_inner_cones[MAX_LIGHTS];
uniform float    u_light_outer_cones[MAX_LIGHTS];

uniform vec3     u_camera_pos;

uniform vec3     u_material_ambient   = vec3(0.1);
uniform vec4     u_material_specular  = vec4(1.0);
uniform float    u_material_shininess = 32.0;

uniform sampler2D u_diffuse_texture;
uniform vec4      u_material_diffuse_tint = vec4(1.0);

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

            float normalized_dist = clamp(dist / range, 0.0, 1.0);
            attenuation = 1.0 - normalized_dist * normalized_dist;
        } else {
            // Spot light (w == 2.0)
            vec3 light_to_frag = pos_or_dir.xyz - v_world_pos;
            float dist = length(light_to_frag);
            L = light_to_frag / dist;

            float normalized_dist = clamp(dist / range, 0.0, 1.0);
            attenuation = 1.0 - normalized_dist * normalized_dist;

            // Spot cone falloff
            vec3 spot_dir = normalize(u_light_spot_directions[i].xyz);
            float cos_angle = max(dot(-L, spot_dir), 0.0);
             float cos_inner = u_light_inner_cones[i];
             float cos_outer = u_light_outer_cones[i];
            attenuation *= spot_cone_attenuation(cos_angle, cos_inner, cos_outer);
        }

        // Diffuse (Lambert)
        float NdotL = max(dot(N, L), 0.0);
        vec3 diffuse = diffuse_colour * light_col * NdotL;

        // Specular (Blinn-Phong)
        vec3 H = normalize(L + V);
        float NdotH = max(dot(N, H), 0.0);
        vec3 specular = u_material_specular.rgb * light_col * pow(NdotH, u_material_shininess);

        final_colour += (diffuse + specular) * attenuation;
    }

    frag_color = vec4(final_colour, 1.0);
}
)";

} // namespace buddd::engine::detail
```

**Uniform list** declared in the shaders (must all be in known_uniforms):
- Vertex: `u_mvp`, `u_model`, `u_normal_mat`
- Fragment: `u_light_count`, `u_light_positions_or_dir`, `u_light_colours`, `u_light_ranges`, `u_light_spot_directions`, `u_light_inner_cones`, `u_light_outer_cones`, `u_camera_pos`, `u_material_ambient`, `u_material_specular`, `u_material_shininess`, `u_diffuse_texture`, `u_material_diffuse_tint`

#### 5b. `PhongMaterial` (`src/engine/render/phong/phong_material.h/.cpp`)

`PhongMaterial` is a self-contained `Material` subclass that creates its own vertex + fragment shaders from embedded GLSL strings. It implements all pure virtual methods from `Material`.

**Header**:
```cpp
#pragma once

#include "render/material.h"
#include "render/light_data.h"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace buddd::engine {

class PhongMaterial final : public Material {
public:
    /// Creates a PhongMaterial with embedded Phong vertex/fragment shaders.
    /// @param device          RenderDevice used to create shader program.
    /// @param known_uniforms  Additional known uniform names beyond the standard Phong set.
    explicit PhongMaterial(RenderDevice& device,
                           std::span<const std::string> known_uniforms = {});

    ~PhongMaterial() override;

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
    auto set_camera_position(const math::Vec3& position) -> void;
    auto set_lights(const detail::LightData* lights, int count) -> void;
    auto set_transforms(const math::Mat4& model, const math::Mat4& view_projection) -> void;

    /// Returns the list of known uniform names declared in the Phong shaders.
    static auto known_uniform_names() -> const std::vector<std::string>&;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace buddd::engine
```

**Implementation notes**:
- Constructor creates vertex + fragment shaders from `detail::k_phong_vertex_shader_source` and `detail::k_phong_fragment_shader_source`.
- Creates a material via `device.create_material(vs, fs, known_uniforms)` OR directly creates a `MaterialHeadless`/`MaterialOpenGL` internally. The simplest approach: create shaders, then create material via `device.create_material()`, and store the resulting material internally. All `set_uniform` calls delegate to the inner material.
- `set_lights()`: for each light i, calls `set_uniform("u_light_positions_or_dir[" + i + "]", ...)` etc. For cones, calls `set_uniform("u_light_inner_cones[" + i + "]", inner_cone_cos)` and `set_uniform("u_light_outer_cones[" + i + "]", outer_cone_cos)`.
- `set_transforms()`: sets `u_model`, `u_mvp`, `u_normal_mat` (= `model.inverse().transpose()`).
- `known_uniform_names()` returns a static vector containing all standard Phong shader uniform names (see uniform list above).

### 6. `MaterialHeadless` diagnostic accessors + array subscript normalization

**`material_headless.h`** — Add 4 new public method declarations after `get_uniform_mat4()`:
```cpp
auto get_uniform_vec3(std::string_view name) const -> std::optional<math::Vec3>;
auto get_uniform_vec4(std::string_view name) const -> std::optional<math::Vec4>;
auto get_uniform_float(std::string_view name) const -> std::optional<float>;
auto get_uniform_int(std::string_view name) const -> std::optional<int32_t>;
```

**`material_headless.cpp`** — Implement the 4 accessors following the pattern of `get_uniform_mat4()`:
```cpp
auto MaterialHeadless::get_uniform_vec3(std::string_view name) const -> std::optional<math::Vec3> {
    auto key = std::string(name);
    auto it = uniform_values_.find(key);
    if (it == uniform_values_.end()) {
        // Try normalized key (strip array subscript)
        key = detail::normalize_uniform_name(name);
        it = uniform_values_.find(key);
        if (it == uniform_values_.end()) return std::nullopt;
    }
    if (!std::holds_alternative<math::Vec3>(it->second)) return std::nullopt;
    return std::get<math::Vec3>(it->second);
}
// Same pattern for vec4, float, int
```

**Array subscript normalization in all existing methods**:
ALL of `set_uniform()` overloads, `has_uniform()`, and `get_uniform_mat4()` must use `glsl_util::normalize_uniform_name()` to strip array subscripts before checking `known_uniforms_`.

Pattern for `set_uniform` overloads:
```cpp
auto MaterialHeadless::set_uniform(std::string_view name, float value) -> Result<void> {
    auto exact_key = std::string(name);
    auto norm_key = detail::normalize_uniform_name(name);
    if (known_uniforms_.count(norm_key) == 0 && uniform_values_.count(exact_key) == 0) {
        return make_error(Error::Category::UniformNotFound, ...);
    }
    known_uniforms_.insert(norm_key);
    uniform_values_[exact_key] = value;
    return {};
}
```

Pattern for `has_uniform`:
```cpp
auto MaterialHeadless::has_uniform(std::string_view name) const -> bool {
    auto norm_key = detail::normalize_uniform_name(name);
    return known_uniforms_.count(norm_key) > 0
        || uniform_values_.count(std::string(name)) > 0;
}
```

Pattern for `get_uniform_mat4`:
```cpp
auto MaterialHeadless::get_uniform_mat4(std::string_view name) const -> std::optional<math::Mat4> {
    auto key = std::string(name);
    auto it = uniform_values_.find(key);
    if (it == uniform_values_.end()) {
        key = detail::normalize_uniform_name(name);
        it = uniform_values_.find(key);
        if (it == uniform_values_.end()) return std::nullopt;
    }
    if (!std::holds_alternative<math::Mat4>(it->second)) return std::nullopt;
    return std::get<math::Mat4>(it->second);
}
```

**Include requirement**: Add `#include "render/glsl_util.h"` to `material_headless.cpp`.

### 7. `RenderSystem` extension (`src/engine/render/render_system.cpp`)

Extend `render()` method:

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

    // Directional lights (type 0)
    world_->each<DirectionalLightComponent>([&](Entity entity, DirectionalLightComponent& lc) -> bool {
        if (light_count >= detail::k_max_lights) return false;
        auto world_mat = entity.world_matrix();
        auto forward = math::Vec4{0.0f, 0.0f, -1.0f, 0.0f};
        auto dir_v4 = world_mat * forward;
        math::Vec3 dir = {dir_v4.x, dir_v4.y, dir_v4.z};
        dir.normalize();
        auto& ld = light_data[light_count];
        ld.position_or_dir = {dir.x, dir.y, dir.z, 0.0f};
        ld.colour = {lc.colour().x * lc.intensity(),
                     lc.colour().y * lc.intensity(),
                     lc.colour().z * lc.intensity(), 1.0f};
        ld.range = 0.0f;
        ld.spot_direction = {0.0f, 0.0f, 0.0f, 0.0f};
        ld.inner_cone_cos = 1.0f;
        ld.outer_cone_cos = 1.0f;
        ++light_count;
        return true;
    });

    // Point lights (type 1)
    world_->each<PointLightComponent>([&](Entity entity, PointLightComponent& lc) -> bool {
        if (light_count >= detail::k_max_lights) return false;
        auto world_mat = entity.world_matrix();
        auto pos_v4 = world_mat * math::Vec4{0.0f, 0.0f, 0.0f, 1.0f};
        auto& ld = light_data[light_count];
        ld.position_or_dir = {pos_v4.x, pos_v4.y, pos_v4.z, 1.0f};
        ld.colour = {lc.colour().x * lc.intensity(),
                     lc.colour().y * lc.intensity(),
                     lc.colour().z * lc.intensity(), 1.0f};
        ld.range = lc.range();
        ld.spot_direction = {0.0f, 0.0f, 0.0f, 0.0f};
        ld.inner_cone_cos = 1.0f;
        ld.outer_cone_cos = 1.0f;
        ++light_count;
        return true;
    });

    // Spot lights (type 2)
    world_->each<SpotLightComponent>([&](Entity entity, SpotLightComponent& lc) -> bool {
        if (light_count >= detail::k_max_lights) return false;
        auto world_mat = entity.world_matrix();
        auto pos_v4 = world_mat * math::Vec4{0.0f, 0.0f, 0.0f, 1.0f};
        auto forward = math::Vec4{0.0f, 0.0f, -1.0f, 0.0f};
        auto dir_v4 = world_mat * forward;
        math::Vec3 dir = {dir_v4.x, dir_v4.y, dir_v4.z};
        dir.normalize();
        auto& ld = light_data[light_count];
        ld.position_or_dir = {pos_v4.x, pos_v4.y, pos_v4.z, 2.0f};
        ld.colour = {lc.colour().x * lc.intensity(),
                     lc.colour().y * lc.intensity(),
                     lc.colour().z * lc.intensity(), 1.0f};
        ld.range = lc.range();
        ld.spot_direction = {dir.x, dir.y, dir.z, 0.0f};
        ld.inner_cone_cos = std::cos(lc.inner_angle());
        ld.outer_cone_cos = std::cos(lc.outer_angle());
        ++light_count;
        return true;
    });

#ifndef NDEBUG
    if (light_count > 0) {
        std::cerr << "RenderSystem: collected " << light_count << " lights\n";
    }
#endif

    // --- 2. Iterate MeshRenderers ---
    world_->each<MeshRenderer>([&](Entity entity, MeshRenderer& mr) -> bool {
        auto world_mat = entity.world_matrix();
        auto mvp = vp * world_mat;
        auto& material = mr.model().material();

        // Always set u_mvp (backward compat)
        auto r = material.set_uniform("u_mvp", mvp);
        if (!r) {
            std::cerr << "RenderSystem: set_uniform(u_mvp) failed for entity "
                      << entity.id().index << ": "
                      << to_string(r.error()) << "\n";
            return true;
        }

        // Check if this material supports lighting (has u_model uniform)
        if (material.has_uniform("u_model")) {
            material.set_uniform("u_model", world_mat);
            auto normal_mat = world_mat.inverse().transpose();
            material.set_uniform("u_normal_mat", normal_mat);
            material.set_uniform("u_camera_pos", camera_pos);

            material.set_uniform("u_light_count", light_count);
            for (int i = 0; i < light_count; ++i) {
                auto const& ld = light_data[i];
                material.set_uniform(
                    "u_light_positions_or_dir[" + std::to_string(i) + "]", ld.position_or_dir);
                material.set_uniform(
                    "u_light_colours[" + std::to_string(i) + "]", ld.colour);
                material.set_uniform(
                    "u_light_ranges[" + std::to_string(i) + "]", ld.range);
                material.set_uniform(
                    "u_light_spot_directions[" + std::to_string(i) + "]", ld.spot_direction);
                material.set_uniform(
                    "u_light_inner_cones[" + std::to_string(i) + "]", ld.inner_cone_cos);
                material.set_uniform(
                    "u_light_outer_cones[" + std::to_string(i) + "]", ld.outer_cone_cos);
            }

            // Material defaults
            material.set_uniform("u_material_ambient", math::Vec3{0.1f, 0.1f, 0.1f});
            material.set_uniform("u_material_specular", math::Vec4{1.0f, 1.0f, 1.0f, 1.0f});
            material.set_uniform("u_material_shininess", 32.0f);
            material.set_uniform("u_material_diffuse_tint", math::Vec4{1.0f, 1.0f, 1.0f, 1.0f});
        }

        mr.model().draw(*device_);
        return true;
    });

    device_->end_frame();
}
```

**Required new includes** in `render_system.cpp`:
```cpp
#include "scene/directional_light_component.h"
#include "scene/point_light_component.h"
#include "scene/spot_light_component.h"
#include "render/light_data.h"
#include <array>
#include <cmath>  // std::cos
```

### 8. Update `demo_helpers.cpp` to use Vertex struct

**`setup_triangle()`**: Replace the local `struct Vertex { float x, y, z, r, g, b; };` with `buddd::engine::Vertex`:
- Fill `position.x/y/z` and `color.r/g/b/a` (a = 1.0), leave `normal`, `texcoord`, `tangent`, `texcoord2` zero.
- Vertex format: use only location 0 (Float3, offsetof position) and location 1 (Float4, offsetof color). Stride = sizeof(Vertex) = 72.
- This produces a larger vertex buffer (72B per vertex instead of 24B) but maintains identical rendering behavior because the shader only binds locations 0 and 1.

**`setup_cube()`**: Same conversion:
- Replace `CubeVertex` struct with `Vertex`.
- Fill `position.xyz` from `px,py,pz`, fill `color` as `{cr, cg, cb, 1.0f}`, leave other fields zero.
- Vertex format stride = sizeof(Vertex) = 72. Attributes: location 0 Float3 at offsetof(Vertex, position), location 1 Float4 at offsetof(Vertex, color).

**Include**: Add `#include "render/vertex.h"` to `demo_helpers.cpp`.

### 9. Demo `src/cmd/demo/phong_demo.h/.cpp` (NEW)

**`phong_demo.h`**:
```cpp
#pragma once

namespace buddd::engine { class RenderDevice; }

namespace buddd::cmd::demo {

[[nodiscard]] auto run_phong_demo(buddd::engine::RenderDevice& device,
                                  int argc, const char* const* argv) -> int;

} // namespace buddd::cmd::demo
```

**`phong_demo.cpp`**: Follow `free_camera_demo.cpp` pattern:

1. **World + Camera**: Create `World`, camera entity at position `(3, 2, 3)` looking at origin, perspective 60° FOV, 800×600, near 0.1, far 100.

2. **Texture**: Attempt to load `assets/brick.png` via `Image::load`. On failure, generate a procedural checkerboard texture (8×8, alternating black/white squares, 3 channels). Create texture via `device.create_texture()`.

3. **Phong cube**: Create a cube entity with `MeshRenderer` using `PhongMaterial`:
   - Vertex data: 24 vertices using `Vertex` struct, with position, normal (face normals), texcoord (per-face [0,1]²). Color left as white default.
   - 36 indices, same winding as `setup_cube()`.
   - Vertex format: `k_standard_vertex_format`.
   - Model created via `Model::create_indexed()`.
   - Bind texture: `material.set_texture("u_diffuse_texture", texture)`.

4. **Orbiting PointLightComponent**: Entity at `(2, 2, 2)` with `PointLightComponent(Vec3{1,1,1}, 1.5f, 8.0f)`. In the frame loop, update position:
   ```cpp
   float t = elapsed_seconds;
   light_entity.transform().position = math::Vec3{
       2.0f * std::cos(t),
       2.0f * std::sin(t) + 1.0f,
       2.0f * std::sin(t * 0.7f)
   };
   ```

5. **DirectionalLightComponent fill**: Entity with `DirectionalLightComponent(Vec3{0.5f, 0.5f, 0.7f}, 0.5f)` rotated ~45° around Y, ~30° down.

6. **RenderSystem**: `RenderSystem render_system(device, world)`.

7. **Interactive loop** (matching `free_camera_demo.cpp`):
   - `poll_events()` — exit on close/Escape.
   - `delta_time()` for frame timing.
   - WASD + mouse right-click capture for free-camera.
   - Update orbit light position.
   - `render_system.render()`.
   - Frame rate limiter (16ms target).
   - No fixed frame count — loops until user exits.

8. **Includes**: `"math/vec3.h"`, `"math/math.h"`, `"math/quat.h"`, `"math/camera.h"`, `"render/vertex.h"`, `"render/light_data.h"`, `"render/phong/phong_material.h"`, `"render/phong/phong_shaders.h"`, `"scene/directional_light_component.h"`, `"scene/point_light_component.h"`, `"scene/spot_light_component.h"`, etc.

### 10. Demo command dispatch (`src/cmd/commands/demo_command.cpp`)

1. Add `#include "demo/phong_demo.h"`.
2. Add `"phong"` to the validation check on line 59.
3. Add dispatch branch (in alphabetical position, after `"free-camera"`):
   ```cpp
   } else if (demo_name == "phong") {
       return buddd::cmd::demo::run_phong_demo(**device, argc - 2, argv + 2);
   ```
4. Add `"  phong         Run the Phong lighting demo (interactive, textured cube with orbiting point light)\n"` to `k_demo_usage`.

### 11. `render_device_opengl.cpp` — Add uniform name extraction

Currently `RenderDeviceOpenGL::create_material()` uses the `known_uniforms` span but does not parse shader source. Extend it to:
1. Add `#include "render/glsl_util.h"`.
2. Parse both vertex and fragment shader sources using `detail::extract_uniform_names()`.
3. Merge with the `known_uniforms` parameter.
4. (The actual uniform storage is only relevant for `MaterialOpenGL`'s implementation; if `MaterialOpenGL` currently uses `glGetUniformLocation` at `set_uniform` time, the known_uniforms may be ignored — but adding the parsing ensures consistency with the headless backend.)

### 12. `render_device_headless.cpp` — Migrate to glsl_util

1. Add `#include "render/glsl_util.h"`.
2. Remove the local `extract_uniform_names()` function (lines 118-193 of the current file).
3. Replace calls from `extract_uniform_names(vs.source())` to `detail::extract_uniform_names(vs.source())`.

## Required tests

All tests live in `tests/lighting_tests.cpp` and run headless (no GPU required).

**Helper** (anonymous namespace):
```cpp
namespace {
    auto make_headless_engine() -> std::unique_ptr<EngineService> {
        auto engine = EngineService::create(
            Backend::Headless,
            WindowConfig{.title = "Test", .width = 800, .height = 600});
        REQUIRE(engine.has_value());
        return std::move(*engine);
    }
    constexpr float TOL = 1e-5f;
}
```

### AC-to-test mapping

| AC ID | Test case name | Description |
|-------|---------------|-------------|
| AC-001 | `"Vertex struct layout"` | `static_assert(sizeof(Vertex) == 72)`. Verify `offsetof` for each field. Verify `k_standard_vertex_format` has 6 attributes with correct stride. |
| AC-002 | `"DirectionalLightComponent construction and accessors"` | Create with params, verify values via const accessors, mutate via non-const accessors. |
| AC-003 | `"PointLightComponent construction and accessors"` | Same as AC-002, plus range (default 10.0). |
| AC-004 | `"SpotLightComponent construction and accessors"` | Same as AC-003, plus inner_angle (default 0.785), outer_angle (default 1.047). |
| AC-005 | `"Light component on_attach no-op"` | Create each component type, add to entity, verify no crash, no side effects (no world registration). |
| AC-006 | `"PhongMaterial is a valid Material subclass"` | Create `PhongMaterial` via its constructor, verify `has_uniform("u_model")` returns true. |
| AC-007 | `"PhongMaterial embedded shaders"` | Constructor does not accept external shader parameters. `has_uniform("u_model")` is true. |
| AC-008 | `"PhongMaterial convenience setters"` | Header declares `set_camera_position`, `set_lights`, `set_transforms`. |
| AC-009 | `"PhongMaterial known_uniform_names"` | `known_uniform_names()` returns expected set including all 17 uniforms. Each name from AC-009's list returns true from `has_uniform`. |
| AC-010 | `"glsl_util extract_uniform_names"` | Parse known GLSL snippets: `uniform float x;` → {"x"}; `uniform vec4 arr[N];` → {"arr"}; `uniform vec3 def = vec3(0.1);` → {"def"}; `uniform vec4 both[N] = ...;` → {"both"}. Also verify `layout(...) uniform vec4 x;` → {"x"}. |
| AC-011 | `"glsl_util normalize_uniform_name"` | `"foo[0]"` → `"foo"`, `"foo[123]"` → `"foo"`, `"foo"` → `"foo"`, `"u_light_colours[3]"` → `"u_light_colours"`. |
| AC-012 | `"LightData struct"` | Header has `k_max_lights = 8`. `LightData` has all 6 fields. |
| AC-013 | `"RenderSystem collects directional lights"` | 3 directional lights + 1 mesh renderer. Verify `u_light_count` = 3. Verify first light direction from rotation. |
| AC-014 | `"RenderSystem collects point lights"` | Point light at (5,3,1). Verify `u_light_positions_or_dir[0].w == 1.0` and xyz ≈ (5,3,1). |
| AC-015 | `"RenderSystem collects spot lights"` | Spot light at known position/rotation with known angles. Verify position, direction, cone cosines, type flag w == 2.0. |
| AC-016 | `"RenderSystem caps at 8 lights"` | 10 lights total (4 dir + 4 point + 2 spot). Verify `u_light_count` == 8. |
| AC-017 | `"Light colour * intensity premultiplied"` | Light with colour (0.5, 0.5, 0.5), intensity 2.0. Verify `u_light_colours[0].rgb` ≈ (1.0, 1.0, 1.0). |
| AC-018 | `"Normal matrix computation"` | Render with lit mesh. `get_uniform_mat4("u_normal_mat")` ≈ `world_mat.inverse().transpose()`. |
| AC-019 | `"Backward compat: unlit material"` | Unlit material (no `u_model`). `has_uniform("u_model")` false. Render works, draw call count > 0. No lighting uniforms set. |
| AC-020 | `"RenderSystem sets u_camera_pos"` | `get_uniform_vec3("u_camera_pos")` matches camera position. |
| AC-021 | `"RenderSystem sets material property defaults"` | `u_material_ambient` ≈ (0.1, 0.1, 0.1), `u_material_specular` ≈ (1,1,1,1), `u_material_shininess` ≈ 32.0, `u_material_diffuse_tint` ≈ (1,1,1,1). |
| AC-022 | `"Light component entity destruction"` | 2 lights, count=2. Destroy one, flush, render, count=1. |
| AC-023 | `"Zero lights renders with ambient only"` | Lit mesh but no light components. `u_light_count` = 0. Material has `u_mvp` set. No crash. |
| AC-024 | `"phong_shaders.h exists and compiles"` | Include header, verify both constants are non-empty. |
| AC-025 | `"RenderSystem sets u_model"` | `get_uniform_mat4("u_model")` matches entity world_matrix. |
| AC-026 | `"MaterialHeadless array subscript normalization"` | `set_uniform("u_light_positions_or_dir[0]", Vec4{1,2,3,4})` succeeds. `get_uniform_vec4("u_light_positions_or_dir[0]")` returns expected value. `has_uniform("u_light_positions_or_dir")` returns true. |
| AC-027 | `"MaterialHeadless diagnostic accessors"` | Set Vec3/Vec4/float/int values, retrieve via getters, verify match. Non-existent name → nullopt. Type mismatch → nullopt. |
| AC-028 | `"Phong demo exists and compiles"` | `phong_demo.h` declares `run_phong_demo`. Demo_command dispatches `"phong"`. (Manual verification of demo rendering.) |
| AC-029 | `"glsl_util used by both backends"` | Both `render_device_headless.cpp` and `render_device_opengl.cpp` include and use `detail::extract_uniform_names()` from `glsl_util.h`. |
| AC-030 | `"Demo helpers use Vertex struct"` | `setup_triangle()` and `setup_cube()` use `Vertex` struct with stride 72. Attributes are Float3 at position offset + Float4 at color offset. |
| AC-031 | `"Spot light cone uniforms"` | Spot light with known angles. `u_light_inner_cones[0]` ≈ cos(inner_angle), `u_light_outer_cones[0]` ≈ cos(outer_angle). `u_light_spot_directions[0].xyz` ≈ normalized direction. |
| AC-032 | `"glsl_util handles layout qualifiers"` | Parse `layout(location=0) uniform vec4 u_thing;` → {"u_thing"}. |

### Additional test requirements

- **extract_uniform_names edge cases**: Empty source → empty set. Multiple uniforms with same base name → single entry. Sampler uniforms → name extracted. Struct uniforms (not handled, verify they don't crash).
- **normalize_uniform_name edge cases**: Empty string → empty string. Name starting with `[` → `[` (no strip). Multiple `[` → strips only first.
- **MaterialHeadless edge cases**: `set_uniform` with unknown normalized name that was never declared returns error. `get_uniform_float` on int value → nullopt. `has_uniform` with array subscript name → true if base name is known.

## Edge cases

All edge cases from SPEC-018 (lines 826-847) must be handled:

| Case | Expected behavior |
|------|------------------|
| No light component entities of any type | `u_light_count` = 0. Ambient term still applied (`u_material_ambient * diffuse_colour`). No crash. |
| More than `k_max_lights` (8) lights across all types | First 8 collected across type groups (dir→point→spot). Silently ignored. Debug build logs warning. |
| Directional light with identity rotation | Direction is `(0, 0, -1)`. |
| Directional light with non-uniform scale | Direction normalized after transform to mitigate scale distortion. |
| Point light at origin `(0, 0, 0)` | Valid. |
| Point/spot light with zero/negative range | Undefined behaviour (division by zero in shader). Documented precondition. |
| Light with zero intensity | Contributes nothing (`colour * 0 = black`). Valid. |
| Light added/removed between frames | Collected fresh each frame. Removed lights reflected on next render. |
| Mixed lit and unlit materials in same scene | `has_uniform("u_model")` check per-entity. Lit gets lighting uniforms, unlit gets only `u_mvp`. |
| Camera detached during MeshRenderer iteration | Camera captured once at render start. Consistent with frame-bounded rendering. |
| Lit material without `u_diffuse_texture` bound | Undefined GPU behaviour. Demo always binds a texture. |
| `set_uniform` failure for lighting uniform | Entity skipped, warning logged. Other entities unaffected. |
| `world_mat.inverse()` fails (singular matrix) | Undefined behaviour. Documented extreme edge case. |
| Spot with `inner_angle >= outer_angle` | Programmer error — undefined cone falloff. |
| All three light component types on same entity | Each collected independently by type iteration. Valid but unusual. |
| `get_uniform_float("nonexistent")` | Returns `std::nullopt`. |
| `get_uniform_float("u_mvp")` (type mismatch) | Returns `std::nullopt`. |

## Security impact

None. No elevated privileges, network access, filesystem secrets, or new dependencies. CONST-001 is maintained: all new `scene/` headers include only `math/*.h` and `scene/component.h` — no backend types. `phong_shaders.h` is in `render/` — internal header with only GLSL string literals.

## Data and migration impact

None. No persistent data, no schema changes, no migrations.

## API compatibility impact

- **New public API**: `DirectionalLightComponent`, `PointLightComponent`, `SpotLightComponent` in `buddd::engine` namespace. `PhongMaterial` in `buddd::engine`. `LightData` in `buddd::engine::detail`.
- **New internal API**: `vertex.h`, `glsl_util.h`, `light_data.h`, `phong_shaders.h`.
- **Backward compatible**: All existing unlit demos compile and run unchanged. The `has_uniform("u_model")` gate ensures unlit materials never receive lighting uniforms.
- **MaterialHeadless** gains 4 new diagnostic methods but the abstract `Material` interface is unchanged.
- **Vertex struct changes demo_helpers.cpp** (stride goes from 24B to 72B) — existing unlit demos now use more memory for vertex data but render identically. This is an acceptable trade-off for a unified vertex format.
- **`render_device_opengl.cpp`** now parses shader sources for uniform names — no behavioral change for OpenGL backend (uniforms are resolved lazily via `glGetUniformLocation`), but ensures `known_uniforms` is populated for possible future use.

## Documentation impact

- `.specs/sprint-2026-05/lighting/implementation-contract.md` — this file.
- `.specs/sprint-2026-05/lighting/coordination.md` — update `## implementation-contract-author` section.

## ADR impact

None. The implementation follows existing ADRs. No new architectural decisions warrant a new ADR.

## Constitution impact

None. CONST-001 is fully respected. No amendment needed.

## Done criteria

- [ ] **DC-001**: `src/engine/render/vertex.h` exists with `Vertex` struct (72B stride, 6 attributes) and `k_standard_vertex_format` constant.
- [ ] **DC-002**: `src/engine/render/glsl_util.h/.cpp` exist with `extract_uniform_names()` and `normalize_uniform_name()` in `buddd::engine::detail`.
- [ ] **DC-003**: `src/engine/render/light_data.h` exists with `k_max_lights = 8` and `LightData` struct with all 6 fields.
- [ ] **DC-004**: `src/engine/render/phong/phong_shaders.h` exists with `k_phong_vertex_shader_source` and `k_phong_fragment_shader_source` constants. Fragment shader has flat array uniforms matching the spec: `u_light_positions_or_dir`, `u_light_colours`, `u_light_ranges`, `u_light_spot_directions`, `u_light_inner_cones`, `u_light_outer_cones`.
- [ ] **DC-005**: `src/engine/render/phong/phong_material.h/.cpp` exists with `PhongMaterial` class (Material subclass), implements all pure virtual methods, has convenience setters `set_camera_position`, `set_lights`, `set_transforms`.
- [ ] **DC-006**: `src/engine/scene/directional_light_component.h/.cpp` exists with `DirectionalLightComponent` (colour + intensity, default white 1.0).
- [ ] **DC-007**: `src/engine/scene/point_light_component.h/.cpp` exists with `PointLightComponent` (colour + intensity + range, default 10.0).
- [ ] **DC-008**: `src/engine/scene/spot_light_component.h/.cpp` exists with `SpotLightComponent` (colour + intensity + range + inner_angle 0.785 + outer_angle 1.047).
- [ ] **DC-009**: All three light components have `on_attach()` as a no-op `{}`, no destructor declared, follow the `CameraComponent` accessor pattern.
- [ ] **DC-010**: `src/engine/render/material_headless.h` declares `get_uniform_vec3`, `get_uniform_vec4`, `get_uniform_float`, `get_uniform_int`.
- [ ] **DC-011**: `src/engine/render/material_headless.cpp` implements the 4 new accessors and normalizes array subscripts in ALL `set_uniform` overloads, `has_uniform`, and ALL diagnostic getters using `detail::normalize_uniform_name()`.
- [ ] **DC-012**: `src/engine/render/render_system.cpp` extended with light collection (`World::each<DirectionalLightComponent>`, `World::each<PointLightComponent>`, `World::each<SpotLightComponent>`) up to `k_max_lights` (8) and per-entity lighting uniform setting gated by `has_uniform("u_model")`.
- [ ] **DC-013**: `src/engine/render/render_device_headless.cpp` migrated: local `extract_uniform_names()` removed, uses `detail::extract_uniform_names()` from `glsl_util.h`.
- [ ] **DC-014**: `src/engine/render/render_device_opengl.cpp` updated: includes `glsl_util.h`, parses shader sources for known uniforms.
- [ ] **DC-015**: `src/cmd/demo/demo_helpers.cpp` updated: `setup_triangle()` and `setup_cube()` use `Vertex` struct (stride 72), attribute locations 0 (Float3) + 1 (Float4).
- [ ] **DC-016**: `src/cmd/demo/phong_demo.h/.cpp` exists with interactive free-camera loop, orbiting `PointLightComponent`, static `DirectionalLightComponent` fill, `PhongMaterial`, texture loading with procedural fallback.
- [ ] **DC-017**: `src/cmd/commands/demo_command.cpp` updated: includes `phong_demo.h`, validates `"phong"`, dispatches to `run_phong_demo`, adds usage text.
- [ ] **DC-018**: `tests/lighting_tests.cpp` exists with headless tests covering AC-001 through AC-032.
- [ ] **DC-019**: All headless tests pass: `cmake --build --preset debug && ctest --preset debug --tests-regex lighting`.
- [ ] **DC-020**: All existing tests still pass: `ctest --preset debug` shows same pass count as before.
- [ ] **DC-021**: All existing unlit demos compile and run without modification (`triangle`, `cube`, `cube-scene`, `textured-cube`, `free-camera`).
- [ ] **DC-022**: CONST-001 compliance: no SDL3/OpenGL/GLM types in any `src/engine/scene/` new header.
- [ ] **DC-023**: `buddd demo phong` runs and shows a textured cube with Phong lighting from an orbiting point light, interactive WASD + mouse free-camera (manual verification).
