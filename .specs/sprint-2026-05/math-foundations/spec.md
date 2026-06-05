# SPEC-004 — Math Foundations (Vec2, Vec3, Vec4, Mat4, Quat, Camera)

## Status

`Accepted`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|---|
| Approved by | Guillaume (user) |
| Date | 2026-05-29 |
| Time | (approved) |

## Problem

The Buddd Engine has no linear algebra types. Without vectors, matrices, and quaternions:

- Vertex positions, normals, UVs, and colours cannot be represented in engine code.
- There is no way to compute model, view, or projection transformations for rendering.
- There is no camera abstraction — every rendering path would need to build its own.
- Engine code that needs to compute transforms, interpolate between orientations, or perform spatial calculations must either use raw `float` arrays (error-prone, unreadable) or pull in GLM directly (violating the OO style preferred by the project).

## Goals

- Provide `Vec2`, `Vec3`, `Vec4` types with standard vector operations (arithmetic, dot, cross, length, normalize, component access).
- Provide a `Mat4` type with matrix operations (multiply, transpose, inverse, determinant, perspective, ortho, lookAt, translate, rotate, scale).
- Provide a `Quat` type with quaternion operations (multiply, rotate a vector, conjugate, inverse, slerp, to rotation matrix).
- Provide a `Camera` type that computes view and projection matrices.
- Wrap GLM (`glm`) as the underlying implementation — zero-overhead, header-only, thin wrapper so that code reads as `my_vec.normalized()` rather than `glm::normalize(my_vec)`.
- Add GLM as a build dependency via `FetchContent`, consistent with how SDL3 and Catch2 are already managed.
- All math types live under `src/engine/math/` in namespace `buddd::engine::math`.
- The `Camera` type lives under `src/engine/math/` in namespace `buddd::engine::math`.
- All operations are `constexpr`-friendly where GLM supports it.
- No GLM types leak into public headers of other engine subsystems (architecture boundary).
- Headless/unit-test compatible — no GPU or display required to use math types.

## Non-goals

- No render pipeline changes (no draw calls, shaders, buffers, vertex arrays).
- No mesh, model, or geometry abstractions.
- No material or texture support.
- No ECS, scene graph, or transform hierarchy.
- No physics or collision detection.
- No SIMD-optimised or hand-tuned platform-specific math (GLM already handles this).
- No double-precision variants (all types use `float` — the GLM default and OpenGL convention).
- No graphics API interop beyond the natural column-major layout of `Mat4` (OpenGL-friendly).
- No change to existing rendering, window, or platform abstractions.
- No `constexpr` guarantee for operations that GLM does not support as `constexpr` (inverse, determinant may not be `constexpr` in the fetched GLM version).

## Actors

| Actor | Description |
|---|---|
| Engine developer | A developer adding features that require spatial math (rendering, camera, transforms). Creates vectors, matrices, quaternions, cameras. Reads `.normalized()`, `.length()`, `.cross()` style code. |
| Application developer | A developer building on top of the engine who needs to define positions, orientations, and camera transforms. |
| Build system | CMake + Ninja that fetches GLM via `FetchContent` and provides the header-only library to `buddd_engine`. GLM is a PUBLIC dependency so downstream targets can include math headers. |
| Test suite | Catch2 v3 tests that exercise every math operation — no display, no GPU required. |

## User-visible behavior

### Type overview

All math types are `struct` types with the **same memory layout** as their corresponding GLM type — same members in the same order. Each method delegates to the equivalent GLM function by casting `*this` to the GLM type via `reinterpret_cast`, which is safe because `static_assert` guarantees identical layout and standard-layout conformance.

| C++ type | Wraps | GLM type | Purpose |
|---|---|---|---|
| `Vec2` | `glm::vec2` | 2D vector (x, y) | UVs, screen coordinates, 2D transforms |
| `Vec3` | `glm::vec3` | 3D vector (x, y, z) | Positions, directions, colours, normals |
| `Vec4` | `glm::vec4` | 4D vector (x, y, z, w) | Homogeneous coordinates, shader uniforms |
| `Mat4` | `glm::mat4` | 4×4 column-major matrix | Model, view, projection transforms |
| `Quat` | `glm::quat` | Quaternion (w, x, y, z) | Rotations, interpolation (slerp) |
| `Camera` | (user-defined) | Perspective camera | View & projection matrix computation |

### Utility functions, constants, and common math

```cpp
namespace buddd::engine::math {

// -- Mathematical constants (constexpr variables) --
inline constexpr float pi       = 3.14159265358979323846f;
inline constexpr float half_pi  = 1.57079632679489661923f;
inline constexpr float two_pi   = 6.28318530717958647693f;
inline constexpr float epsilon  = 1.0e-6f;

// -- Conversion --
/// Convert degrees to radians.
auto radians(float degrees) noexcept -> float;

/// Convert radians to degrees.
auto degrees(float radians) noexcept -> float;

// -- Common math functions (delegate to GLM / cmath) --
auto sin(float angle) noexcept -> float;
auto cos(float angle) noexcept -> float;
auto tan(float angle) noexcept -> float;
auto asin(float x) noexcept -> float;
auto acos(float x) noexcept -> float;
auto atan(float y_over_x) noexcept -> float;
auto atan2(float y, float x) noexcept -> float;
auto sqrt(float x) noexcept -> float;

} // namespace buddd::engine::math
```

### Memory layout

| Type | Size | Alignment | Layout |
|---|---|---|---|---|
| `Vec2` | 8 bytes | 4 | Two `float` values: x, y |
| `Vec3` | 12 bytes | 4 | Three `float` values: x, y, z |
| `Vec4` | 16 bytes | 4 | Four `float` values: x, y, z, w |
| `Mat4` | 64 bytes | 4 | 16 floats in column-major order (`m[col][row]`), compatible with `glUniformMatrix4fv` with `GL_FALSE` for the transpose parameter |
| `Quat` | 16 bytes | 4 | Four `float` values: x, y, z, w (matching GLM's default `qua` layout) |

### Vec2 operations

```cpp
namespace buddd::engine::math {

struct Vec2 {
    // Public members (same layout as glm::vec2)
    float x, y;

    // Constructors
    Vec2() noexcept : x(0.0f), y(0.0f) {}
    Vec2(float x_, float y_) noexcept : x(x_), y(y_) {}

    // Index access
    auto operator[](int i) const noexcept -> float;
    auto operator[](int i) noexcept -> float&;

    // GLM interop — zero-overhead reinterpret (same ABI guaranteed by static_assert)
    auto glm() noexcept -> glm::vec2& { return reinterpret_cast<glm::vec2&>(*this); }
    auto glm() const noexcept -> const glm::vec2& { return reinterpret_cast<const glm::vec2&>(*this); }

    // Arithmetic operators (component-wise)
    friend auto operator+(Vec2 a, Vec2 b) -> Vec2;
    friend auto operator-(Vec2 a, Vec2 b) -> Vec2;
    friend auto operator*(Vec2 a, Vec2 b) -> Vec2;  // component-wise
    friend auto operator/(Vec2 a, Vec2 b) -> Vec2;  // component-wise
    friend auto operator*(Vec2 v, float s) -> Vec2;
    friend auto operator*(float s, Vec2 v) -> Vec2;
    friend auto operator/(Vec2 v, float s) -> Vec2;
    auto operator-() const -> Vec2;                  // unary minus
    auto operator+=(Vec2 other) -> Vec2&;
    auto operator-=(Vec2 other) -> Vec2&;
    auto operator*=(Vec2 other) -> Vec2&;            // component-wise
    auto operator/=(Vec2 other) -> Vec2&;            // component-wise
    auto operator*=(float s) -> Vec2&;
    auto operator/=(float s) -> Vec2&;

    // Comparison (exact float comparison)
    friend auto operator==(Vec2 a, Vec2 b) -> bool;
    friend auto operator!=(Vec2 a, Vec2 b) -> bool;

    // Vector operations
    auto length() const noexcept -> float;
    auto length_squared() const noexcept -> float;
    auto normalize() -> Vec2&;           // mutates in place, returns self
    auto normalized() const -> Vec2;     // returns a copy
    auto dot(Vec2 other) const -> float;

    // Constants (constexpr)
    static constexpr auto zero() noexcept -> Vec2 { return Vec2{0.0f, 0.0f}; }
    static constexpr auto one() noexcept -> Vec2 { return Vec2{1.0f, 1.0f}; }
    static constexpr auto unit_x() noexcept -> Vec2 { return Vec2{1.0f, 0.0f}; }
    static constexpr auto unit_y() noexcept -> Vec2 { return Vec2{0.0f, 1.0f}; }
};

static_assert(std::is_standard_layout_v<Vec2>, "Vec2 must be standard layout");
static_assert(sizeof(Vec2) == sizeof(glm::vec2), "Vec2 size must match glm::vec2");
static_assert(std::is_trivially_copyable_v<Vec2>, "Vec2 must be trivially copyable");

} // namespace buddd::engine::math
```

### Vec3 operations

Same pattern as Vec2, plus cross product and lerp. Members: x, y, z.

```cpp
namespace buddd::engine::math {

struct Vec3 {
    // Public members (same layout as glm::vec3)
    float x, y, z;

    // Constructors
    Vec3() noexcept : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float x_, float y_, float z_) noexcept : x(x_), y(y_), z(z_) {}

    // Index access
    auto operator[](int i) const noexcept -> float;
    auto operator[](int i) noexcept -> float&;

    // GLM interop — zero-overhead reinterpret
    auto glm() noexcept -> glm::vec3& { return reinterpret_cast<glm::vec3&>(*this); }
    auto glm() const noexcept -> const glm::vec3& { return reinterpret_cast<const glm::vec3&>(*this); }

    // Arithmetic: same pattern as Vec2 (component-wise + scalar)
    // Comparison: ==, !=
    // Length: length(), length_squared(), normalize(), normalized()

    auto dot(Vec3 other) const -> float;
    auto cross(Vec3 other) const -> Vec3;
    auto lerp(Vec3 other, float t) const -> Vec3;  // linear interpolation: (1-t)*this + t*other

    static constexpr auto zero() noexcept -> Vec3;
    static constexpr auto one() noexcept -> Vec3;
    static constexpr auto unit_x() noexcept -> Vec3;
    static constexpr auto unit_y() noexcept -> Vec3;
    static constexpr auto unit_z() noexcept -> Vec3;
};

static_assert(std::is_standard_layout_v<Vec3>, "Vec3 must be standard layout");
static_assert(sizeof(Vec3) == sizeof(glm::vec3), "Vec3 size must match glm::vec3");
static_assert(std::is_trivially_copyable_v<Vec3>, "Vec3 must be trivially copyable");

} // namespace buddd::engine::math
```

### Vec4 operations

Same pattern as Vec2/Vec3. Members: x, y, z, w.

```cpp
namespace buddd::engine::math {

struct Vec4 {
    // Public members (same layout as glm::vec4)
    float x, y, z, w;

    // Constructors
    Vec4() noexcept : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vec4(float x_, float y_, float z_, float w_) noexcept : x(x_), y(y_), z(z_), w(w_) {}

    // Index access
    auto operator[](int i) const noexcept -> float;
    auto operator[](int i) noexcept -> float&;

    // GLM interop — zero-overhead reinterpret
    auto glm() noexcept -> glm::vec4& { return reinterpret_cast<glm::vec4&>(*this); }
    auto glm() const noexcept -> const glm::vec4& { return reinterpret_cast<const glm::vec4&>(*this); }

    // Arithmetic: same pattern (component-wise + scalar)
    // Comparison: ==, !=
    // Length: length(), length_squared(), normalize(), normalized()

    auto dot(Vec4 other) const -> float;

    static constexpr auto zero() noexcept -> Vec4;
    static constexpr auto one() noexcept -> Vec4;
    static constexpr auto unit_x() noexcept -> Vec4;
    static constexpr auto unit_y() noexcept -> Vec4;
    static constexpr auto unit_z() noexcept -> Vec4;
    static constexpr auto unit_w() noexcept -> Vec4;
};

static_assert(std::is_standard_layout_v<Vec4>, "Vec4 must be standard layout");
static_assert(sizeof(Vec4) == sizeof(glm::vec4), "Vec4 size must match glm::vec4");
static_assert(std::is_trivially_copyable_v<Vec4>, "Vec4 must be trivially copyable");

} // namespace buddd::engine::math
```

### Mat4 operations

```cpp
namespace buddd::engine::math {

struct Mat4 {
    // Public members: 4 columns of Vec4, column-major order (same layout as glm::mat4)
    Vec4 cols[4];

    // Constructors
    /// Creates identity matrix.
    Mat4() noexcept : cols{Vec4{1,0,0,0}, Vec4{0,1,0,0}, Vec4{0,0,1,0}, Vec4{0,0,0,1}} {}
    /// Creates a diagonal scale matrix with the given value on the diagonal.
    explicit Mat4(float d) noexcept : cols{Vec4{d,0,0,0}, Vec4{0,d,0,0}, Vec4{0,0,d,0}, Vec4{0,0,0,d}} {}

    // Column access (index 0-3)
    auto operator[](int col) const noexcept -> const Vec4&;
    auto operator[](int col) noexcept -> Vec4&;

    // GLM interop — zero-overhead reinterpret (same ABI guaranteed by static_assert)
    auto glm() noexcept -> glm::mat4& { return reinterpret_cast<glm::mat4&>(*this); }
    auto glm() const noexcept -> const glm::mat4& { return reinterpret_cast<const glm::mat4&>(*this); }

    // Arithmetic
    friend auto operator+(Mat4 a, Mat4 b) -> Mat4;
    friend auto operator-(Mat4 a, Mat4 b) -> Mat4;
    friend auto operator*(Mat4 a, Mat4 b) -> Mat4;     // matrix-matrix multiply
    friend auto operator*(Mat4 m, Vec4 v) -> Vec4;     // matrix-vector multiply
    friend auto operator*(Vec4 v, Mat4 m) -> Vec4;     // vector-matrix multiply
    friend auto operator*(Mat4 m, Vec3 v) -> Vec3;     // (m * Vec4(v, 1.0f)).xyz — column vector
    friend auto operator*(Vec3 v, Mat4 m) -> Vec3;     // (Vec4(v, 1.0f) * m).xyz — row vector
    friend auto operator*(Mat4 m, float s) -> Mat4;
    auto operator+=(Mat4 other) -> Mat4&;
    auto operator-=(Mat4 other) -> Mat4&;
    auto operator*=(Mat4 other) -> Mat4&;

    // Comparison
    friend auto operator==(Mat4 a, Mat4 b) -> bool;
    friend auto operator!=(Mat4 a, Mat4 b) -> bool;

    // Matrix operations
    auto transpose() const -> Mat4;
    auto inverse() const -> Mat4;          // delegates to glm::inverse; singular matrices produce NaN/inf
    auto determinant() const -> float;

    // Transform construction (static factories)
    static auto identity() noexcept -> Mat4;
    static auto perspective(float fov_y, float aspect, float near, float far) -> Mat4;
    static auto ortho(float left, float right, float bottom, float top, float near, float far) -> Mat4;
    static auto look_at(Vec3 eye, Vec3 center, Vec3 up) -> Mat4;
    static auto translate(Vec3 offset) -> Mat4;
    static auto rotate(float angle, Vec3 axis) -> Mat4;          // angle in radians
    static auto scale(Vec3 factors) -> Mat4;
};

static_assert(std::is_standard_layout_v<Mat4>, "Mat4 must be standard layout");
static_assert(sizeof(Mat4) == sizeof(glm::mat4), "Mat4 size must match glm::mat4");
static_assert(std::is_trivially_copyable_v<Mat4>, "Mat4 must be trivially copyable");

} // namespace buddd::engine::math
```

### Quat operations

```cpp
namespace buddd::engine::math {

struct Quat {
    // Public members (same layout as glm::quat — w, x, y, z order)
    float w, x, y, z;

    // Constructors
    /// Default: identity quaternion (w=1, x=0, y=0, z=0).
    Quat() noexcept : w(1.0f), x(0.0f), y(0.0f), z(0.0f) {}
    /// Raw component constructor. GLM stores as (w, x, y, z).
    Quat(float w_, float x_, float y_, float z_) noexcept : w(w_), x(x_), y(y_), z(z_) {}

    // Index access
    auto operator[](int i) const noexcept -> float;
    auto operator[](int i) noexcept -> float&;

    // GLM interop — zero-overhead reinterpret
    auto glm() noexcept -> glm::quat& { return reinterpret_cast<glm::quat&>(*this); }
    auto glm() const noexcept -> const glm::quat& { return reinterpret_cast<const glm::quat&>(*this); }

    // Arithmetic
    friend auto operator*(Quat a, Quat b) -> Quat;      // composition
    friend auto operator*(Quat q, Vec3 v) -> Vec3;      // rotate vector
    auto operator*=(Quat other) -> Quat&;

    // Comparison
    friend auto operator==(Quat a, Quat b) -> bool;
    friend auto operator!=(Quat a, Quat b) -> bool;

    // Quaternion operations
    auto normalize() -> Quat&;
    auto normalized() const -> Quat;
    auto conjugate() const -> Quat;
    auto inverse() const -> Quat;
    auto to_mat4() const -> Mat4;
    static auto slerp(Quat a, Quat b, float t) -> Quat;

    // Static factories
    static auto identity() noexcept -> Quat;
    static auto angle_axis(float angle, Vec3 axis) -> Quat;   // angle in radians
    static auto from_euler(float pitch, float yaw, float roll) -> Quat;  // radians
};

static_assert(std::is_standard_layout_v<Quat>, "Quat must be standard layout");
static_assert(sizeof(Quat) == sizeof(glm::quat), "Quat size must match glm::quat");
static_assert(std::is_trivially_copyable_v<Quat>, "Quat must be trivially copyable");

} // namespace buddd::engine::math
```

### Camera type

```cpp
namespace buddd::engine::math {

class Camera {
public:
    /// Default: position (0,0,0), identity orientation, 60° FOV, 16:9 aspect, near 0.1, far 100.
    Camera() = default;

    /// Convenience constructor: sets position, orientation, and perspective parameters.
    Camera(Vec3 position, Quat orientation,
           float fov_y, float aspect, float near_plane, float far_plane);

    // Position / orientation
    auto position() const noexcept -> Vec3;
    auto set_position(Vec3 position) -> void;

    auto orientation() const noexcept -> Quat;
    auto set_orientation(Quat orientation) -> void;

    // Look-at convenience
    auto look_at(Vec3 target) -> void;
    auto look_at(Vec3 eye, Vec3 center, Vec3 up) -> void;  // sets both position and orientation

    // Projection parameters (perspective)
    auto set_perspective(float fov_y, float aspect, float near, float far) -> void;
    auto fov_y() const noexcept -> float;
    auto aspect() const noexcept -> float;
    auto near_plane() const noexcept -> float;
    auto far_plane() const noexcept -> float;

    // Matrix computation (recomputed on each call; no caching)
    auto view_matrix() const -> Mat4;
    auto projection_matrix() const -> Mat4;

    // Combined: projection_matrix() * view_matrix()
    auto view_projection_matrix() const -> Mat4;

private:
    Vec3 position_{0.0f, 0.0f, 0.0f};
    Quat orientation_{Quat::identity()};
    float fov_y_{1.0471975512f};        // 60 degrees in radians (π/3)
    float aspect_{16.0f / 9.0f};
    float near_{0.1f};
    float far_{100.0f};
};

} // namespace buddd::engine::math
```

### GLM integration

GLM is added as a build dependency via `FetchContent` in `src/engine/CMakeLists.txt`, with a pinned tag. The math wrapper headers include the required GLM headers internally — consumers of the engine never include GLM directly.

Each wrapper type has the **same memory layout** as its GLM counterpart (same members in the same order). The `.glm()` accessor uses `reinterpret_cast` to return a reference to the underlying GLM type — zero overhead, no copy. This is the **official interop path** for code that needs the raw GLM type:

```cpp
Vec3 v{1.0f, 2.0f, 3.0f};
glm::vec3& gv = v.glm();   // reinterpret_cast, zero-overhead
v.x = 5.0f;                 // direct member access, same as glm::vec3
```

Each wrapper type includes `static_assert` checks for `std::is_standard_layout_v`, `sizeof` equality, and `std::is_trivially_copyable_v` with the corresponding GLM type, guaranteeing identical ABI. The `reinterpret_cast` in `.glm()` is safe because:
- All wrapper types are standard-layout with the same member layout as their GLM counterpart.
- `sizeof(T) == sizeof(GlmType)` is verified at compile time.
- All types are trivially copyable.

### Architecture boundary

- GLM headers are included only inside `src/engine/math/` (the wrapper headers) and in `src/engine/` files that include the math wrappers.
- Outside `src/engine/`, no code may `#include` any GLM header directly. All math operations go through the wrapper types.
- Test files in `tests/` may include the math wrapper headers (e.g., `#include <engine/math/vec3.h>`) but NOT GLM headers directly.
- GLM interop is done exclusively via the `.glm()` accessor (which uses `reinterpret_cast` internally) — safe because `static_assert` guarantees identical layout.
- This is consistent with CONST-001 principle: abstraction over dependencies.

## User stories

### Story 1 — Create and use vectors (Priority: P1)

As an engine developer, I want to create vectors, compute their length, normalize them, and perform arithmetic, so that I can represent positions and directions in 3D space.

**Given** the math headers are included
**When** I write:
```cpp
auto pos = Vec3(1.0f, 2.0f, 3.0f);
auto dir = Vec3::unit_z();
auto len = pos.length();
auto n = pos.normalized();
auto sum = pos + dir;
auto dot = pos.dot(dir);
auto cross = pos.cross(dir);
```
**Then** each operation produces the correct mathematical result.

### Story 2 — Create and use matrices (Priority: P1)

As an engine developer, I want to create transformation matrices and combine them, so that I can build model, view, and projection matrices for rendering.

**Given** the math headers are included
**When** I write:
```cpp
auto model = Mat4::translate({1.0f, 0.0f, 0.0f}) * Mat4::rotate(half_pi, Vec3::unit_y());
auto view = Mat4::look_at({0.0f, 0.0f, 5.0f}, Vec3::zero(), Vec3::unit_y());
auto proj = Mat4::perspective(radians(60.0f), 16.0f/9.0f, 0.1f, 100.0f);
auto mvp = proj * view * model;
```
**Then** each matrix operation produces the correct mathematical result.

### Story 3 — Create and use quaternions (Priority: P1)

As an engine developer, I want to create quaternions, compose them, rotate vectors, and slerp between them, so that I can represent and interpolate orientations.

**Given** the math headers are included
**When** I write:
```cpp
auto q1 = Quat::angle_axis(radians(90.0f), Vec3::unit_y());
auto q2 = Quat::identity();
auto rotated = q1 * Vec3::unit_x();   // rotate (1,0,0) by 90° around Y → (0,0,-1)
auto blended = Quat::slerp(q1, q2, 0.5f);
auto rot_mat = q1.to_mat4();
```
**Then** each operation produces the correct mathematical result.

### Story 4 — Use a camera (Priority: P1)

As an engine developer, I want a Camera object that tracks a position and orientation and computes view and projection matrices, so that I can render a scene from a camera's perspective.

**Given** a Camera instance
**When** I configure it and compute matrices:
```cpp
Camera cam;
cam.set_position({0.0f, 2.0f, 5.0f});
cam.look_at({0.0f, 0.0f, 0.0f});
auto view = cam.view_matrix();
auto proj = cam.projection_matrix();
```
**Then** `view` is the inverse of the camera's transform (so objects in world space are transformed into view space) and `proj` is a perspective projection matrix based on the camera's configured FOV, aspect, near, and far.

### Story 5 — Zero-overhead with GLM interop (Priority: P2)

As an engine developer, I want to pass a `Mat4` to OpenGL's `glUniformMatrix4fv` without copying or transposing, so that there is no runtime overhead from the wrapper.

**Given** a `Mat4 m`
**When** I call `glUniformMatrix4fv(location, 1, GL_FALSE, &m[0][0])` or equivalent
**Then** the memory layout is already column-major and directly compatible.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|---|
| AC-001 | `Vec2` exists with constructors (`Vec2()`, `Vec2(x,y)`), public `x`, `y` members, arithmetic operators (+, -, *, /), scalar * (both sides), unary -, compound assignment (+=, -=, *=, /=), ==, !=, `length()`, `length_squared()`, `normalize()`, `normalized()`, `dot()`, `zero()`, `one()`, `unit_x()`, `unit_y()`. | Each operation result must match the equivalent GLM operation within a tolerance of `1e-5f` (e.g., `a.dot(b)` ≈ `glm::dot(glm_a, glm_b)`). |
| AC-002 | `Vec3` exists with constructors, public `x`, `y`, `z` members, same operations as Vec2 plus `cross()`, `lerp()`, `unit_z()`. | Each operation result must match equivalent GLM output within `1e-5f` tolerance. |
| AC-003 | `Vec4` exists with constructors, public `w` member, same operations as Vec2 plus `unit_w()`. | Each operation result must match equivalent GLM output within `1e-5f` tolerance. |
| AC-004 | `Mat4` exists with constructors, `operator[]`, +, -, * (matrix-matrix, matrix-vec4, vec4-matrix, matrix-vec3, vec3-matrix, scalar), ==, !=, `transpose()`, `inverse()`, `determinant()`, `glm()` accessor, `identity()`, `perspective()`, `ortho()`, `look_at()`, `translate()`, `rotate()`, `scale()`. | Each operation result must match equivalent GLM output within `1e-5f` tolerance. `inverse()` delegates to `glm::inverse` (NaN/inf for singular matrices, no detection). |
| AC-005 | `Quat` exists with constructors (identity, wxyz), `*` (quat-quat, quat-vec3), `*=` , ==, !=, `normalized()`, `conjugate()`, `inverse()`, `to_mat4()`, `slerp()`, `glm()` accessor, `identity()`, `angle_axis()`, `from_euler()`. | Each operation result must match equivalent GLM output within `1e-5f` tolerance. `slerp` produces correct intermediate orientations at t=0, 0.5, 1. |
| AC-006 | `Camera` exists with constructors (default, parameterized), `position()`, `set_position()`, `orientation()`, `set_orientation()`, `look_at()`, `set_perspective()`, `view_matrix()`, `projection_matrix()`, `view_projection_matrix()`. | `view_matrix()` must match `glm::lookAt(position, position + forward, up)` where `forward = orientation * Vec3(0, 0, -1)` and `up = orientation * Vec3(0, 1, 0)` (OpenGL convention). `projection_matrix()` must match `glm::perspective(fov_y, aspect, near, far)`. Tolerance `1e-5f`. |
| AC-007 | GLM is fetched via `FetchContent` from GitHub with a pinned tag (e.g., `master` or a specific tag). | CMake configure succeeds; GLM headers are available at build time. |
| AC-008 | All math wrapper headers live under `src/engine/math/`. | Files exist at `src/engine/math/vec2.h`, `vec3.h`, `vec4.h`, `mat4.h`, `quat.h`, `camera.h`. |
| AC-009 | All math types are in namespace `buddd::engine::math`. | Compilation of `buddd::engine::math::Vec3` succeeds. |
| AC-010 | No GLM types appear in the public API of math types except via the explicit `.glm()` interop accessor. All normal methods accept and return wrapper types only. | No `glm::` prefix appears in any public method signature of the wrapper headers, except the `.glm()` accessor return type. |
| AC-011 | `Mat4` data layout is column-major (compatible with `glUniformMatrix4fv` with `GL_FALSE`). | Unit test verifies that the internal storage order matches GLM's `glm::mat4` layout. |
| AC-012 | The API surface is header-only (no `.cpp` files required for math types). | All math type definitions and method implementations compile from headers alone. |
| AC-013 | No build warnings related to math types with `-Wall -Wextra`. | Build with `cmake --build --preset debug` produces zero warnings from math source files. |
| AC-014 | All math types are trivially copyable (or at least satisfy `std::is_trivially_copyable_v` for GLM compatibility). | `static_assert(std::is_trivially_copyable_v<Vec3>)` passes. |
| AC-015 | GLM headers are never included directly outside `src/engine/math/`. | Code review catches violations. (No automated guard at this stage.) |
| AC-016 | `Camera::view_projection_matrix()` returns `projection_matrix() * view_matrix()`. | Unit test verifies the multiplication order. |
| AC-017 | The `.glm()` accessor is present on all five math primitives (Vec2, Vec3, Vec4, Mat4, Quat) and returns a reference to the underlying GLM type. | `static_assert(std::is_same_v<decltype(v.glm()), glm::vec3&>)` passes for Vec3, and similarly for other types. |
| AC-018 | Each wrapper type includes `static_assert(std::is_standard_layout_v<T>)` and `static_assert(sizeof(T) == sizeof(GlmType))`. | Compile-time checks pass (verified by compilation). |
| AC-019 | `radians()` and `degrees()` utility functions and `pi`, `half_pi`, `two_pi`, `epsilon` constants exist. | `radians(180.0f) ≈ pi` within `1e-5f`; `degrees(pi) ≈ 180.0f` within `1e-5f`. `epsilon == 1e-6f`. |
| AC-020 | `Vec3::lerp()` produces `(1-t) * a + t * b`. | `Vec3(0,0,0).lerp(Vec3(10,10,10), 0.5f)` equals `Vec3(5,5,5)` within `1e-5f`. |
| AC-021 | `sin()`, `cos()`, `tan()`, `asin()`, `acos()`, `atan()`, `atan2()`, `sqrt()` exist in `buddd::engine::math`. | Each produces results matching GLM within `1e-5f`. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A developer can write `Vec3`, `Mat4`, `Quat`, and `Camera` code without knowing GLM exists. | All public API examples compile and produce correct results without any `glm::` prefix. |
| SC-002 | All math tests pass in headless CI (no display, no GPU). | `cmake --build --preset debug && ctest --preset debug` — all math tests pass. |
| SC-003 | The wrapper adds zero runtime overhead vs raw GLM for equivalent operations. | (Manual review) Each method is a single inline call to the equivalent GLM function; no branching, no allocation, no vtable. |

## Edge cases

| Case | Expected behavior |
|---|---|---|
| `Vec3::normalize()` on zero-length vector | Returns a vector with NaN components (GLM behaviour: division by zero produces NaN). The caller must check for zero length before normalizing. |
| `Mat4::inverse()` on singular matrix | Delegates to `glm::inverse`, which produces NaN/inf components for singular matrices. No error detection. The caller must ensure the matrix is invertible. |
| `Mat4 * Vec3` transform | Equivalent to `(m * Vec4(v, 1.0f)).xyz()` — the Vec3 is promoted to homogeneous, transformed as a column vector (OpenGL convention). |
| `Vec3 * Mat4` transform | Equivalent to `(Vec4(v, 1.0f) * m).xyz()` — the Vec3 is promoted to homogeneous and transformed as a row vector. This is the row-vector convention; for column-vector use `Mat4 * Vec3`. |
| `Vec2::operator*` with mixed component-wise and scalar types | All operators are unambiguous: component-wise `*` for `Vec * Vec`, scalar `*` for `Vec * float` and `float * Vec`. |
| `Camera` default construction | Position `(0,0,0)`, identity orientation, FOV 60° (1.047 rad), aspect 16:9, near 0.1, far 100. `view_matrix()` returns an identity view (camera at origin looking down -Z). |
| `Camera::look_at()` with up vector parallel to direction | The result is implementation-defined (GLM `glm::lookAt` behaviour — the up vector is adjusted). The wrapper delegates to GLM and accepts its behaviour. |
| `Quat::angle_axis()` with zero-length axis | Returns identity quaternion (GLM behaviour). |
| `Quat::slerp()` with identical quaternions | Returns the input quaternion (GLM handles this). |
| `Quat::slerp()` with opposite quaternions | GLM resolves by falling back to linear interpolation on the shortest path. Accepted as GLM behaviour. |
| `operator[]` with out-of-bounds index | Behavior is undefined (GLM uses `glm::assert` in debug builds; no bounds checking in release). The caller must ensure `0 <= i < dimension`. |
| `operator/(Vec2, 0.0f)` scalar division by zero | Produces inf/NaN components (GLM behaviour). No exception or error. |

## Error cases

| Case | Expected behavior |
|---|---|---|
| `Vec3::normalize()` on zero-length vector | Returns a vector with NaN components (GLM behaviour: division by 0 → NaN). No exception or error return. |
| Division by zero in scalar `operator/` (`Vec / 0.0f`) | Produces inf/NaN components. No exception. |
| `Mat4::inverse()` on singular matrix | Returns matrix with NaN/inf components (GLM `glm::inverse` behavior). No exception. |
| `Mat4::perspective()` with near <= 0 or far <= near | Returns a degenerate projection matrix (GLM behaviour). The caller is responsible for valid parameters. |
| `Mat4::perspective()` with fov_y <= 0.0f or fov_y >= π | Returns a degenerate projection matrix (GLM behaviour). |
| `Mat4::ortho()` with left >= right or bottom >= top | Returns a degenerate orthographic matrix (GLM behaviour). |

## Permissions and security

- No elevated privileges required.
- No secrets, credentials, or environment variables consumed.
- GLM is a header-only math library with no platform-specific dependencies.
- The math types are pure computation — no I/O, no network, no filesystem access.

## Observability

No observability output is generated by math types. They are pure computational primitives. Any debugging output is the responsibility of the calling code.

## Out of scope

- SIMD-optimised or platform-specific math implementations beyond what GLM provides.
- Double-precision types (`dvec2`, `dvec3`, `dmat4`, etc.).
- `Mat2`, `Mat3` types.
- `Vec1` type.
- Transform hierarchy or scene graph.
- Bounding volumes (AABB, sphere, frustum).
- Raycasting or intersection tests.
- Serialization of math types.
- Math type interop with non-OpenGL graphics APIs (Vulkan, Metal, DirectX).
- GPU-side math or compute shader integration.
- Input event handling, audio, ECS, physics, or any game runtime system.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | GLM is available via `FetchContent` from the GLM GitHub repository (`https://github.com/g-truc/glm`). A specific tag (e.g., the latest stable release or the `master` tip at a known SHA) is used for reproducibility. |
| A-02 | GLM is header-only and requires no compiled library. It is usable as a `INTERFACE` or `HEADER_ONLY` CMake target. |
| A-03 | All wrapper types have the same size and alignment as their GLM counterparts, verified by `static_assert`. The official interop path is the `.glm()` accessor, which is safe and zero-overhead. |
| A-04 | The project uses `float` precision throughout (GLM default). Double-precision variants are not needed at this stage. |
| A-05 | A `Vec3` stored in a `Vertex` struct with `x, y, z` floats has the expected 12-byte layout and 4-byte alignment. This is compatible with OpenGL vertex attribute specification. |
| A-06 | `Mat4` is column-major, stored as 4 `Vec4` columns. `m[col][row]` accesses the element at column `col`, row `row`. This matches GLM and OpenGL conventions. |
| A-07 | The Camera does not cache view or projection matrices. Each call to `view_matrix()` or `projection_matrix()` recomputes the result. Performance optimization (dirty flag caching) may be added later. |
| A-08 | The Camera uses a right-handed coordinate system with Y-up (OpenGL convention). View matrix looks down the -Z axis. This matches GLM's `glm::lookAt` convention. |
| A-09 | The wrapper provides `math::radians()`, `math::degrees()`, and `math::constants` (`pi()`, `half_pi()`, `two_pi()`). The wrapper API uses radians throughout for angular parameters (consistent with GLM and C++ math conventions). |
| A-10 | GLM is a PUBLIC dependency of `buddd_engine`, meaning any target linking `buddd_engine` transitively inherits GLM include paths. The architecture boundary (no direct GLM includes outside `src/engine/math/`) is enforced by convention and code review, not by compiler guards — matching the existing pattern for SDL3 and OpenGL. |

## Open questions

| ID | Question | Impact |
|---|---|---|
| Q-01 | [RESOLVED] GLM version to pin. **Resolution**: Use the latest tagged release (e.g., `1.0.1` or current `master` at time of implementation). The exact tag is specified in the implementation contract. | Build reproducibility. |
| Q-02 | [RESOLVED] Should `Mat4::inverse()` return a `Result<Mat4>` for singular matrices? **Resolution**: No — GLM does not return errors, and the convention of the math layer is to be a thin zero-overhead wrapper. Singular matrix handling is the caller's responsibility. |
| Q-03 | [RESOLVED] Should Camera matrices be cached with a dirty flag? **Resolution**: No — KISS. Recompute on each call. Caching can be added later if profiling shows it matters. |
| Q-04 | [RESOLVED] Namespace: should math types be in `buddd::engine` or `buddd::engine::math`? **Resolution**: `buddd::engine::math` — math is a distinct subsystem within the engine, consistent with the directory structure `src/engine/math/`. |
