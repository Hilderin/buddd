# IMPL-004 — Math Foundations (Vec2, Vec3, Vec4, Mat4, Quat, Camera)

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

## Source spec

`.specs/sprint-2026-05/math-foundations/spec.md` (SPEC-004), accepted (`.specs/sprint-2026-05/math-foundations/spec-critic.md` verdict: `Accepted`, all 8 blocking issues resolved). All 4 re-review issues resolved. All non-blocking warnings are acknowledged.

## Goal

Provide a linear algebra foundation for the Buddd Engine by:

1. Implementing thin C++ wrapper types (`Vec2`, `Vec3`, `Vec4`, `Mat4`, `Quat`) around GLM (`glm`) — header-only, zero-overhead, in namespace `buddd::engine::math`.
2. Implementing a perspective `Camera` class that uses these wrapper types to compute view and projection matrices.
3. Adding GLM as a build dependency via `FetchContent` with pinned tag `1.0.1`.
4. Placing all math types under `src/engine/math/`, with GLM headers included only inside that directory.
5. Providing a convenience header `math.h` that includes all math types and provides utility functions (`radians`, `degrees`, `constants`).

## Non-goals

- No `.cpp` files for Vec2, Vec3, Vec4, Mat4, or Quat (pure header-only). `Camera` is the only type with a `.cpp`.
- No render pipeline, draw calls, shaders, buffers, or vertex arrays.
- No mesh, model, geometry, material, or texture abstractions.
- No ECS, scene graph, or transform hierarchy.
- No physics, collision detection, or spatial queries.
- No SIMD-optimised or platform-specific math beyond what GLM provides.
- No double-precision variants (`dvec`, `dmat`).
- No `Mat2`, `Mat3`, or `Vec1` types.
- No bounding volumes (AABB, sphere, frustum).
- No raycasting or intersection tests.
- No serialization of math types.
- No GPU-side math or compute shader integration.
- No change to `.clang-format`, `CMakePresets.json`, root `CMakeLists.txt`, `.vscode/`, or any file outside `src/engine/`.
- No modification of existing source files except `src/engine/CMakeLists.txt`.
- No test files (tests are specified for the test-author only).
- No wiki or documentation files.
- No `constexpr` guarantee for operations that GLM itself does not support as `constexpr` (`inverse`, `determinant`, `slerp`, `perspective`, `ortho`, `look_at`, `rotate`, `scale`).

## Relevant constitution rules

- **CONST-001-architecture-boundaries.md**: Enforces the architecture boundary — no code outside `src/engine/` may include GLM headers directly. This contract implements that rule by placing all GLM includes inside `src/engine/math/` and providing wrapper types as the public API.
- **CONST-002-testing-policy.md**: Requires unit tests for all testable code. This contract specifies required tests (see Required tests section).
- **Engineering principles** (`docs/constitution/principles.md`): Prefer explicit contracts, small scoped changes, existing conventions, and testable requirements.

## Relevant ADRs

- **ADR-001**: `docs/adr/001-result-error-pattern.md` — Establishes `Result<T>` / `Error` for fallible engine APIs. Math functions are pure computation with no error returns, so `Result<T>` is NOT used in the math layer. This is consistent with ADR-001's "Where this does NOT apply" clause (functions that cannot logically fail return plain values).

## Files to inspect

| File | Purpose |
|---|---|
| `src/engine/CMakeLists.txt` | Current CMake config. Must be modified to add GLM FetchContent. Uses `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` — new `.h`/`.cpp` files under `src/engine/math/` are automatically picked up. |
| `src/engine/version.h` | Style reference for engine headers (`#pragma once`, namespace `buddd::engine`, trailing return types). |
| `src/engine/error.h` | Style reference — shows existing engine code conventions. |
| `.specs/sprint-2026-05/platform-abstraction/implementation-contract.md` | Style reference (IMPL-002) — level of detail and format to match. |
| `.specs/sprint-2026-05/math-foundations/spec.md` | Authoritative spec for behavior. |

## Files allowed to change

### New files to create (8 files)

All paths are relative to the repository root.

| # | File | Purpose |
|---|---|---|
| 1 | `src/engine/math/math.h` | Convenience header — includes all math types, provides `radians()`, `degrees()`, `constants`. |
| 2 | `src/engine/math/vec2.h` | `Vec2` — 2D vector wrapper around `glm::vec2`. Header-only. |
| 3 | `src/engine/math/vec3.h` | `Vec3` — 3D vector wrapper around `glm::vec3`. Header-only. |
| 4 | `src/engine/math/vec4.h` | `Vec4` — 4D vector wrapper around `glm::vec4`. Header-only. |
| 5 | `src/engine/math/mat4.h` | `Mat4` — 4x4 column-major matrix wrapper around `glm::mat4`. Header-only. |
| 6 | `src/engine/math/quat.h` | `Quat` — quaternion wrapper around `glm::quat`. Header-only. |
| 7 | `src/engine/math/camera.h` | `Camera` — perspective camera class. Declarations only. |
| 8 | `src/engine/math/camera.cpp` | `Camera` — method implementations. |

### Files to modify (1 file)

| # | File | Change |
|---|---|---|
| 9 | `src/engine/CMakeLists.txt` | Add FetchContent for GLM (`1.0.1`), link `glm::glm` to `buddd_engine` as PUBLIC. |

## Files forbidden to change

- Any file outside `src/engine/` (including tests, root `CMakeLists.txt`, `CMakePresets.json`, `.clang-format`, `.vscode/`, `docs/`, `AGENTS.md`, `opencode.json`).
- `src/engine/version.h`
- `src/engine/version.cpp`
- `src/engine/error.h`
- `src/engine/CMakeLists.txt` — except for the GLM FetchContent addition specified in this contract. No other changes.
- `src/engine/platform/` (any file)
- `src/engine/window/` (any file)
- `src/engine/render/` (any file)
- `src/cmd/` (any file)
- `src/editor/` (any file)
- `tests/` (any file — test files will be created by the test-author)

## Existing conventions to follow

| Convention | Rule |
|---|---|
| Namespace | `buddd::engine::math` for all math types. |
| File naming | `snake_case` (lowercase ASCII, digits, underscores) — `vec2.h`, `vec3.h`, `mat4.h`, etc. |
| Directory naming | `snake_case` — `src/engine/math/`. |
| Class/struct naming | PascalCase — `Vec2`, `Vec3`, `Vec4`, `Mat4`, `Quat`, `Camera`. |
| Header guards | `#pragma once` (no `#ifndef` guards). |
| Function style | Trailing return type syntax (`auto foo() -> int`). |
| Formatting | `.clang-format` at repo root: LLVM style, 4-space indent, 100 column limit. |
| GLM includes | Use specific GLM sub-headers (`<glm/vec3.hpp>`, `<glm/gtc/matrix_transform.hpp>`) NOT the umbrella `<glm/glm.hpp>` except in `math.h`. |
| Local includes | Use `#include "vec2.h"` (quoted, relative to `src/engine/` — resolved via PUBLIC include directory of `buddd_engine`). |
| GLM interop | `.glm()` accessor returning `GlmType&` / `const GlmType&` is the official interop path. |
| Static assertions | Each wrapper type must have `static_assert` for `std::is_standard_layout_v`, `sizeof` equality with GLM counterpart, and `std::is_trivially_copyable_v`. |
| Include order | 1. GLM headers (angle brackets), 2. Standard library headers, 3. Local engine headers (quotes). Empty line between groups. |
| constexpr | Mark methods `constexpr` only when the underlying GLM operation is `constexpr` in the fetched version. `normalize`, `length`, `dot`, `cross`, arithmetic operators are safe. `inverse`, `determinant`, `perspective`, `ortho`, `look_at`, `rotate`, `scale`, `slerp` are NOT `constexpr`. |
| noexcept | All math operations are `noexcept`. No math function allocates memory, throws, or calls I/O. Exception: GLM assertions on out-of-bounds `operator[]` (debug builds only). |
| Temporary constructor from GLM type | Provide an `explicit` constructor that takes the underlying GLM type by const reference. This is used internally for delegating from GLM operations. Implementation copies components or uses `reinterpret_cast` (for Mat4, which has identical ABI). Example: `explicit Vec3(const glm::vec3& v) noexcept : x(v.x), y(v.y), z(v.z) {}`. |

## Required implementation behavior

### 1. `src/engine/CMakeLists.txt` (modified)

Insert the GLM FetchContent block AFTER the existing SDL3 block and BEFORE `find_package(OpenGL REQUIRED)`.

The modified file must be:

```cmake
include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.30
)
FetchContent_MakeAvailable(SDL3)

# ----- GLM (header-only math library) -----
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

find_package(OpenGL REQUIRED)

# Collect all engine source files automatically using GLOB.
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(buddd_engine STATIC ${ENGINE_SOURCES})

target_include_directories(buddd_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(buddd_engine PUBLIC
    SDL3::SDL3
    OpenGL::GL
    glm::glm
)
```

**Requirements:**
- GLM tag MUST be `1.0.1` (pinned tag for reproducibility).
- `FetchContent_MakeAvailable(glm)` is called BEFORE `add_library(buddd_engine)` so the `glm::glm` target exists at link time.
- `glm::glm` is linked as PUBLIC so downstream targets that transitively include math headers via `buddd_engine` can resolve GLM types (through the wrappers only — see architecture boundary).
- The `file(GLOB_RECURSE ...)` already picks up all `.h` and `.cpp` files under `src/engine/`, including the new `src/engine/math/` directory. No additional source file listing is needed.
- No other changes to this file. The existing SDL3, OpenGL, and target settings remain unchanged.

### 2. `src/engine/math/vec2.h`

Complete header-only definition. Every method is `inline` and delegates to the equivalent `glm::` function.

```cpp
#pragma once

#include <glm/vec2.hpp>
#include <glm/geometric.hpp>

#include <type_traits>

namespace buddd::engine::math {

struct Vec2 {
    // -- Public members (same layout as glm::vec2) --
    float x, y;

    // -- Constructors --
    Vec2() noexcept : x(0.0f), y(0.0f) {}
    Vec2(float x_, float y_) noexcept : x(x_), y(y_) {}
    /// Construct from GLM type (copies components).
    explicit Vec2(const glm::vec2& v) noexcept : x(v.x), y(v.y) {}

    // -- Index access --
    auto operator[](int i) const noexcept -> float { return reinterpret_cast<const glm::vec2&>(*this)[i]; }
    auto operator[](int i) noexcept -> float& { return reinterpret_cast<glm::vec2&>(*this)[i]; }

    // -- GLM interop (safe because static_assert guarantees identical layout) --
    auto glm() noexcept -> glm::vec2& { return reinterpret_cast<glm::vec2&>(*this); }
    auto glm() const noexcept -> const glm::vec2& { return reinterpret_cast<const glm::vec2&>(*this); }

    // -- Arithmetic operators (component-wise) --
    friend auto operator+(Vec2 a, Vec2 b) noexcept -> Vec2 {
        return Vec2{a.x + b.x, a.y + b.y};
    }
    friend auto operator-(Vec2 a, Vec2 b) noexcept -> Vec2 {
        return Vec2{a.x - b.x, a.y - b.y};
    }
    friend auto operator*(Vec2 a, Vec2 b) noexcept -> Vec2 {
        return Vec2{a.x * b.x, a.y * b.y};
    }
    friend auto operator/(Vec2 a, Vec2 b) noexcept -> Vec2 {
        return Vec2{a.x / b.x, a.y / b.y};
    }
    friend auto operator*(Vec2 v, float s) noexcept -> Vec2 { return Vec2{v.x * s, v.y * s}; }
    friend auto operator*(float s, Vec2 v) noexcept -> Vec2 { return Vec2{s * v.x, s * v.y}; }
    friend auto operator/(Vec2 v, float s) noexcept -> Vec2 { return Vec2{v.x / s, v.y / s}; }

    auto operator-() const noexcept -> Vec2 { return Vec2{-x, -y}; }

    auto operator+=(Vec2 other) noexcept -> Vec2& { x += other.x; y += other.y; return *this; }
    auto operator-=(Vec2 other) noexcept -> Vec2& { x -= other.x; y -= other.y; return *this; }
    auto operator*=(Vec2 other) noexcept -> Vec2& { x *= other.x; y *= other.y; return *this; }
    auto operator/=(Vec2 other) noexcept -> Vec2& { x /= other.x; y /= other.y; return *this; }
    auto operator*=(float s) noexcept -> Vec2& { x *= s; y *= s; return *this; }
    auto operator/=(float s) noexcept -> Vec2& { x /= s; y /= s; return *this; }

    // -- Comparison (exact float) --
    friend auto operator==(Vec2 a, Vec2 b) noexcept -> bool { return a.x == b.x && a.y == b.y; }
    friend auto operator!=(Vec2 a, Vec2 b) noexcept -> bool { return !(a == b); }

    // -- Vector operations --
    auto length() const noexcept -> float;
    auto length_squared() const noexcept -> float;
    auto normalize() noexcept -> Vec2&;
    auto normalized() const noexcept -> Vec2;
    auto dot(Vec2 other) const noexcept -> float;

    // -- Constants (constexpr) --
    static constexpr auto zero() noexcept -> Vec2 { return Vec2{0.0f, 0.0f}; }
    static constexpr auto one() noexcept -> Vec2 { return Vec2{1.0f, 1.0f}; }
    static constexpr auto unit_x() noexcept -> Vec2 { return Vec2{1.0f, 0.0f}; }
    static constexpr auto unit_y() noexcept -> Vec2 { return Vec2{0.0f, 1.0f}; }
};

// -- Out-of-body inline implementations (use reinterpret_cast to delegate to GLM) --
inline auto Vec2::length() const noexcept -> float {
    return glm::length(reinterpret_cast<const glm::vec2&>(*this));
}
inline auto Vec2::length_squared() const noexcept -> float {
    return glm::length2(reinterpret_cast<const glm::vec2&>(*this));
}
inline auto Vec2::normalize() noexcept -> Vec2& {
    reinterpret_cast<glm::vec2&>(*this) = glm::normalize(glm::vec2(*this));
    return *this;
}
inline auto Vec2::normalized() const noexcept -> Vec2 {
    return Vec2{glm::normalize(reinterpret_cast<const glm::vec2&>(*this))};
}
inline auto Vec2::dot(Vec2 other) const noexcept -> float {
    return glm::dot(reinterpret_cast<const glm::vec2&>(*this),
                    reinterpret_cast<const glm::vec2&>(other));
}

// -- Static assertions --
static_assert(std::is_standard_layout_v<Vec2>, "Vec2 must be standard layout");
static_assert(sizeof(Vec2) == sizeof(glm::vec2), "Vec2 size must match glm::vec2");
static_assert(std::is_trivially_copyable_v<Vec2>, "Vec2 must be trivially copyable");

} // namespace buddd::engine::math
```

**Requirements:**
- Public members `x`, `y` match `glm::vec2` layout exactly.
- `.glm()` uses `reinterpret_cast<glm::vec2&>(*this)` — safe due to static_assert.
- `normalize()` normalizes in place and returns `*this`.
- `normalized()` returns a copy, does NOT mutate.
- `operator==` and `operator!=` compare components directly (exact float comparison).
- `operator[]` with out-of-bounds index is undefined behavior (GLM assertion in debug builds).
- Division by zero in `operator/(Vec2, float)` produces inf/NaN (GLM behavior).
- `normalize()` / `normalized()` on zero-length vector produces NaN components (GLM: division by zero).
- All three `static_assert` checks must be present at file scope (after the struct).
- The `explicit Vec2(const glm::vec2&)` constructor enables internal delegation.

### 3. `src/engine/math/vec3.h`

Same pattern as `Vec2`, plus `cross()`, `lerp()`, `unit_z()`.

```cpp
#pragma once

#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

#include <type_traits>

namespace buddd::engine::math {

struct Vec3 {
    // -- Public members (same layout as glm::vec3) --
    float x, y, z;

    // -- Constructors --
    Vec3() noexcept : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float x_, float y_, float z_) noexcept : x(x_), y(y_), z(z_) {}
    explicit Vec3(const glm::vec3& v) noexcept : x(v.x), y(v.y), z(v.z) {}

    // -- Index access --
    auto operator[](int i) const noexcept -> float { return reinterpret_cast<const glm::vec3&>(*this)[i]; }
    auto operator[](int i) noexcept -> float& { return reinterpret_cast<glm::vec3&>(*this)[i]; }

    // -- GLM interop --
    auto glm() noexcept -> glm::vec3& { return reinterpret_cast<glm::vec3&>(*this); }
    auto glm() const noexcept -> const glm::vec3& { return reinterpret_cast<const glm::vec3&>(*this); }

    // -- Arithmetic operators (component-wise) --
    friend auto operator+(Vec3 a, Vec3 b) noexcept -> Vec3 {
        return Vec3{a.x + b.x, a.y + b.y, a.z + b.z};
    }
    friend auto operator-(Vec3 a, Vec3 b) noexcept -> Vec3 {
        return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
    }
    friend auto operator*(Vec3 a, Vec3 b) noexcept -> Vec3 {
        return Vec3{a.x * b.x, a.y * b.y, a.z * b.z};
    }
    friend auto operator/(Vec3 a, Vec3 b) noexcept -> Vec3 {
        return Vec3{a.x / b.x, a.y / b.y, a.z / b.z};
    }
    friend auto operator*(Vec3 v, float s) noexcept -> Vec3 {
        return Vec3{v.x * s, v.y * s, v.z * s};
    }
    friend auto operator*(float s, Vec3 v) noexcept -> Vec3 {
        return Vec3{s * v.x, s * v.y, s * v.z};
    }
    friend auto operator/(Vec3 v, float s) noexcept -> Vec3 {
        return Vec3{v.x / s, v.y / s, v.z / s};
    }

    auto operator-() const noexcept -> Vec3 { return Vec3{-x, -y, -z}; }

    auto operator+=(Vec3 other) noexcept -> Vec3& {
        x += other.x; y += other.y; z += other.z; return *this;
    }
    auto operator-=(Vec3 other) noexcept -> Vec3& {
        x -= other.x; y -= other.y; z -= other.z; return *this;
    }
    auto operator*=(Vec3 other) noexcept -> Vec3& {
        x *= other.x; y *= other.y; z *= other.z; return *this;
    }
    auto operator/=(Vec3 other) noexcept -> Vec3& {
        x /= other.x; y /= other.y; z /= other.z; return *this;
    }
    auto operator*=(float s) noexcept -> Vec3& { x *= s; y *= s; z *= s; return *this; }
    auto operator/=(float s) noexcept -> Vec3& { x /= s; y /= s; z /= s; return *this; }

    // -- Comparison --
    friend auto operator==(Vec3 a, Vec3 b) noexcept -> bool {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    friend auto operator!=(Vec3 a, Vec3 b) noexcept -> bool { return !(a == b); }

    // -- Vector operations (inline, using reinterpret_cast to delegate to GLM) --
    auto length() const noexcept -> float {
        return glm::length(reinterpret_cast<const glm::vec3&>(*this));
    }
    auto length_squared() const noexcept -> float {
        return glm::length2(reinterpret_cast<const glm::vec3&>(*this));
    }
    auto normalize() noexcept -> Vec3& {
        reinterpret_cast<glm::vec3&>(*this) = glm::normalize(glm::vec3(*this));
        return *this;
    }
    auto normalized() const noexcept -> Vec3 {
        return Vec3{glm::normalize(reinterpret_cast<const glm::vec3&>(*this))};
    }
    auto dot(Vec3 other) const noexcept -> float {
        return glm::dot(reinterpret_cast<const glm::vec3&>(*this),
                        reinterpret_cast<const glm::vec3&>(other));
    }
    auto cross(Vec3 other) const noexcept -> Vec3 {
        return Vec3{glm::cross(reinterpret_cast<const glm::vec3&>(*this),
                                reinterpret_cast<const glm::vec3&>(other))};
    }
    auto lerp(Vec3 other, float t) const noexcept -> Vec3 {
        return Vec3{glm::mix(reinterpret_cast<const glm::vec3&>(*this),
                              reinterpret_cast<const glm::vec3&>(other), t)};
    }

    // -- Constants (constexpr) --
    static constexpr auto zero() noexcept -> Vec3 { return Vec3{0.0f, 0.0f, 0.0f}; }
    static constexpr auto one() noexcept -> Vec3 { return Vec3{1.0f, 1.0f, 1.0f}; }
    static constexpr auto unit_x() noexcept -> Vec3 { return Vec3{1.0f, 0.0f, 0.0f}; }
    static constexpr auto unit_y() noexcept -> Vec3 { return Vec3{0.0f, 1.0f, 0.0f}; }
    static constexpr auto unit_z() noexcept -> Vec3 { return Vec3{0.0f, 0.0f, 1.0f}; }
};

static_assert(std::is_standard_layout_v<Vec3>, "Vec3 must be standard layout");
static_assert(sizeof(Vec3) == sizeof(glm::vec3), "Vec3 size must match glm::vec3");
static_assert(std::is_trivially_copyable_v<Vec3>, "Vec3 must be trivially copyable");

} // namespace buddd::engine::math
```

**Requirements:**
- `lerp()` uses `glm::mix()` which computes `(1-t) * a + t * b`.
- `cross()` uses `glm::cross()`.
- All other methods match the `Vec2` pattern.

### 4. `src/engine/math/vec4.h`

Same pattern as `Vec2`, plus public `w` member and `unit_w()`.

```cpp
#pragma once

#include <glm/vec4.hpp>
#include <glm/geometric.hpp>

#include <type_traits>

namespace buddd::engine::math {

struct Vec4 {
    // -- Public members (same layout as glm::vec4) --
    float x, y, z, w;

    // -- Constructors --
    Vec4() noexcept : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vec4(float x_, float y_, float z_, float w_) noexcept : x(x_), y(y_), z(z_), w(w_) {}
    explicit Vec4(const glm::vec4& v) noexcept : x(v.x), y(v.y), z(v.z), w(v.w) {}

    // -- Index access --
    auto operator[](int i) const noexcept -> float { return reinterpret_cast<const glm::vec4&>(*this)[i]; }
    auto operator[](int i) noexcept -> float& { return reinterpret_cast<glm::vec4&>(*this)[i]; }

    // -- GLM interop --
    auto glm() noexcept -> glm::vec4& { return reinterpret_cast<glm::vec4&>(*this); }
    auto glm() const noexcept -> const glm::vec4& { return reinterpret_cast<const glm::vec4&>(*this); }

    // -- Arithmetic operators (component-wise) --
    friend auto operator+(Vec4 a, Vec4 b) noexcept -> Vec4 {
        return Vec4{a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
    }
    friend auto operator-(Vec4 a, Vec4 b) noexcept -> Vec4 {
        return Vec4{a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
    }
    friend auto operator*(Vec4 a, Vec4 b) noexcept -> Vec4 {
        return Vec4{a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
    }
    friend auto operator/(Vec4 a, Vec4 b) noexcept -> Vec4 {
        return Vec4{a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w};
    }
    friend auto operator*(Vec4 v, float s) noexcept -> Vec4 {
        return Vec4{v.x * s, v.y * s, v.z * s, v.w * s};
    }
    friend auto operator*(float s, Vec4 v) noexcept -> Vec4 {
        return Vec4{s * v.x, s * v.y, s * v.z, s * v.w};
    }
    friend auto operator/(Vec4 v, float s) noexcept -> Vec4 {
        return Vec4{v.x / s, v.y / s, v.z / s, v.w / s};
    }

    auto operator-() const noexcept -> Vec4 { return Vec4{-x, -y, -z, -w}; }

    auto operator+=(Vec4 other) noexcept -> Vec4& {
        x += other.x; y += other.y; z += other.z; w += other.w; return *this;
    }
    auto operator-=(Vec4 other) noexcept -> Vec4& {
        x -= other.x; y -= other.y; z -= other.z; w -= other.w; return *this;
    }
    auto operator*=(Vec4 other) noexcept -> Vec4& {
        x *= other.x; y *= other.y; z *= other.z; w *= other.w; return *this;
    }
    auto operator/=(Vec4 other) noexcept -> Vec4& {
        x /= other.x; y /= other.y; z /= other.z; w /= other.w; return *this;
    }
    auto operator*=(float s) noexcept -> Vec4& { x *= s; y *= s; z *= s; w *= s; return *this; }
    auto operator/=(float s) noexcept -> Vec4& { x /= s; y /= s; z /= s; w /= s; return *this; }

    // -- Comparison --
    friend auto operator==(Vec4 a, Vec4 b) noexcept -> bool {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
    }
    friend auto operator!=(Vec4 a, Vec4 b) noexcept -> bool { return !(a == b); }

    // -- Vector operations (inline, using reinterpret_cast) --
    auto length() const noexcept -> float {
        return glm::length(reinterpret_cast<const glm::vec4&>(*this));
    }
    auto length_squared() const noexcept -> float {
        return glm::length2(reinterpret_cast<const glm::vec4&>(*this));
    }
    auto normalize() noexcept -> Vec4& {
        reinterpret_cast<glm::vec4&>(*this) = glm::normalize(glm::vec4(*this));
        return *this;
    }
    auto normalized() const noexcept -> Vec4 {
        return Vec4{glm::normalize(reinterpret_cast<const glm::vec4&>(*this))};
    }
    auto dot(Vec4 other) const noexcept -> float {
        return glm::dot(reinterpret_cast<const glm::vec4&>(*this),
                        reinterpret_cast<const glm::vec4&>(other));
    }

    // -- Constants (constexpr) --
    static constexpr auto zero() noexcept -> Vec4 { return Vec4{0.0f, 0.0f, 0.0f, 0.0f}; }
    static constexpr auto one() noexcept -> Vec4 { return Vec4{1.0f, 1.0f, 1.0f, 1.0f}; }
    static constexpr auto unit_x() noexcept -> Vec4 { return Vec4{1.0f, 0.0f, 0.0f, 0.0f}; }
    static constexpr auto unit_y() noexcept -> Vec4 { return Vec4{0.0f, 1.0f, 0.0f, 0.0f}; }
    static constexpr auto unit_z() noexcept -> Vec4 { return Vec4{0.0f, 0.0f, 1.0f, 0.0f}; }
    static constexpr auto unit_w() noexcept -> Vec4 { return Vec4{0.0f, 0.0f, 0.0f, 1.0f}; }
};

static_assert(std::is_standard_layout_v<Vec4>, "Vec4 must be standard layout");
static_assert(sizeof(Vec4) == sizeof(glm::vec4), "Vec4 size must match glm::vec4");
static_assert(std::is_trivially_copyable_v<Vec4>, "Vec4 must be trivially copyable");

} // namespace buddd::engine::math
```

### 5. `src/engine/math/mat4.h`

`Mat4` has the same layout as `glm::mat4`: 4 `Vec4` column vectors in column-major order. Column access returns `Vec4&`. The `.glm()` accessor uses `reinterpret_cast` — safe because `static_assert` guarantees identical storage layout between `Mat4`/`glm::mat4` and `Vec4`/`glm::vec4`.

```cpp
#pragma once

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <type_traits>

#include "vec3.h"
#include "vec4.h"

namespace buddd::engine::math {

struct Mat4 {
    // -- Public members: 4 column vectors (same layout as glm::mat4) --
    Vec4 cols[4];

    // -- Constructors --
    /// Creates identity matrix.
    Mat4() noexcept : cols{Vec4{1,0,0,0}, Vec4{0,1,0,0}, Vec4{0,0,1,0}, Vec4{0,0,0,1}} {}
    /// Creates a diagonal scale matrix with the given value on the diagonal.
    explicit Mat4(float d) noexcept
        : cols{Vec4{d,0,0,0}, Vec4{0,d,0,0}, Vec4{0,0,d,0}, Vec4{0,0,0,d}} {}
    /// Internal: construct from GLM type (via reinterpret — same ABI).
    explicit Mat4(const glm::mat4& m) noexcept {
        reinterpret_cast<glm::mat4&>(*this) = m;
    }

    // -- Column access (index 0-3) --
    auto operator[](int col) const noexcept -> const Vec4& { return cols[col]; }
    auto operator[](int col) noexcept -> Vec4& { return cols[col]; }

    // -- GLM interop (reinterpret_cast — safe due to static_assert) --
    auto glm() noexcept -> glm::mat4& { return reinterpret_cast<glm::mat4&>(*this); }
    auto glm() const noexcept -> const glm::mat4& { return reinterpret_cast<const glm::mat4&>(*this); }

    // -- Arithmetic (delegates via .glm()) --
    friend auto operator+(Mat4 a, Mat4 b) noexcept -> Mat4 { return Mat4{a.glm() + b.glm()}; }
    friend auto operator-(Mat4 a, Mat4 b) noexcept -> Mat4 { return Mat4{a.glm() - b.glm()}; }
    friend auto operator*(Mat4 a, Mat4 b) noexcept -> Mat4 { return Mat4{a.glm() * b.glm()}; }

    friend auto operator*(Mat4 m, Vec4 v) noexcept -> Vec4 { return Vec4{m.glm() * v.glm()}; }
    friend auto operator*(Vec4 v, Mat4 m) noexcept -> Vec4 { return Vec4{v.glm() * m.glm()}; }

    /// Transforms Vec3 as column vector: (m * Vec4(v, 1.0f)).xyz()
    friend auto operator*(Mat4 m, Vec3 v) noexcept -> Vec3 {
        auto r = m.glm() * glm::vec4(v.glm(), 1.0f);
        return Vec3{glm::vec3(r)};
    }

    /// Transforms Vec3 as row vector: (Vec4(v, 1.0f) * m).xyz()
    friend auto operator*(Vec3 v, Mat4 m) noexcept -> Vec3 {
        auto r = glm::vec4(v.glm(), 1.0f) * m.glm();
        return Vec3{glm::vec3(r)};
    }

    friend auto operator*(Mat4 m, float s) noexcept -> Mat4 { return Mat4{m.glm() * s}; }

    auto operator+=(Mat4 other) noexcept -> Mat4& { glm() += other.glm(); return *this; }
    auto operator-=(Mat4 other) noexcept -> Mat4& { glm() -= other.glm(); return *this; }
    auto operator*=(Mat4 other) noexcept -> Mat4& { glm() *= other.glm(); return *this; }

    // -- Comparison --
    friend auto operator==(Mat4 a, Mat4 b) noexcept -> bool { return a.glm() == b.glm(); }
    friend auto operator!=(Mat4 a, Mat4 b) noexcept -> bool { return a.glm() != b.glm(); }

    // -- Matrix operations --
    auto transpose() const noexcept -> Mat4;
    auto inverse() const noexcept -> Mat4;
    auto determinant() const noexcept -> float;

    // -- Static factories (all noexcept — pure GLM delegation) --
    static auto identity() noexcept -> Mat4 { return Mat4{}; }

    static auto perspective(float fov_y, float aspect, float near, float far) noexcept -> Mat4;
    static auto ortho(float left, float right, float bottom, float top,
                      float near, float far) noexcept -> Mat4;
    static auto look_at(Vec3 eye, Vec3 center, Vec3 up) noexcept -> Mat4;
    static auto translate(Vec3 offset) noexcept -> Mat4;
    static auto rotate(float angle, Vec3 axis) noexcept -> Mat4;    // angle in radians
    static auto scale(Vec3 factors) noexcept -> Mat4;
};

// -- Out-of-body inline implementations (use .glm() accessor to delegate to GLM) --
inline auto Mat4::transpose() const noexcept -> Mat4 { return Mat4{glm::transpose(glm())}; }
inline auto Mat4::inverse() const noexcept -> Mat4 { return Mat4{glm::inverse(glm())}; }
inline auto Mat4::determinant() const noexcept -> float { return glm::determinant(glm()); }

inline auto Mat4::perspective(float fov_y, float aspect, float near, float far) noexcept -> Mat4 {
    return Mat4{glm::perspective(fov_y, aspect, near, far)};
}
inline auto Mat4::ortho(float left, float right, float bottom, float top,
                        float near, float far) noexcept -> Mat4 {
    return Mat4{glm::ortho(left, right, bottom, top, near, far)};
}
inline auto Mat4::look_at(Vec3 eye, Vec3 center, Vec3 up) noexcept -> Mat4 {
    return Mat4{glm::lookAt(eye.glm(), center.glm(), up.glm())};
}
inline auto Mat4::translate(Vec3 offset) noexcept -> Mat4 {
    return Mat4{glm::translate(glm::mat4{1.0f}, offset.glm())};
}
inline auto Mat4::rotate(float angle, Vec3 axis) noexcept -> Mat4 {
    return Mat4{glm::rotate(glm::mat4{1.0f}, angle, axis.glm())};
}
inline auto Mat4::scale(Vec3 factors) noexcept -> Mat4 {
    return Mat4{glm::scale(glm::mat4{1.0f}, factors.glm())};
}

// -- Static assertions --
static_assert(std::is_standard_layout_v<Mat4>, "Mat4 must be standard layout");
static_assert(sizeof(Mat4) == sizeof(glm::mat4), "Mat4 size must match glm::mat4");
static_assert(std::is_trivially_copyable_v<Mat4>, "Mat4 must be trivially copyable");

} // namespace buddd::engine::math
```

**Requirements:**
- `Mat4()` creates identity: column 0 = (1,0,0,0), column 1 = (0,1,0,0), column 2 = (0,0,1,0), column 3 = (0,0,0,1).
- `operator*(Mat4, Vec3)` promotes Vec3 to homogeneous `Vec4(v, 1.0f)`, multiplies as column vector, returns `.xyz()`.
- `operator*(Vec3, Mat4)` promotes Vec3 to homogeneous `Vec4(v, 1.0f)`, multiplies as row vector, returns `.xyz()`.
- `perspective()` delegates to `glm::perspective` (right-handed, OpenGL convention).
- `look_at()` delegates to `glm::lookAt`.
- `translate()`, `rotate()`, `scale()` construct transform from identity (`glm::mat4{1.0f}`).
- `inverse()` on singular matrix produces NaN/inf (GLM behavior, no detection).
- `determinant()` of singular matrix returns 0.0f.
- `operator[]` returns `cols[col]` as `Vec4&` for both const and non-const.
- All static factory methods are `noexcept` (pure GLM delegation, no allocation or I/O).
- The `explicit Mat4(const glm::mat4&)` constructor enables internal delegation from GLM functions.
- Includes `"vec3.h"` (for `Mat4 * Vec3` and `Vec3 * Mat4` operators) and `"vec4.h"`.

### 6. `src/engine/math/quat.h`

```cpp
#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <type_traits>

#include "vec3.h"
#include "mat4.h"

namespace buddd::engine::math {

struct Quat {
    // -- Public members (same layout as glm::quat: w, x, y, z order) --
    float w, x, y, z;

    // -- Constructors --
    /// Default: identity quaternion (w=1, x=0, y=0, z=0).
    Quat() noexcept : w(1.0f), x(0.0f), y(0.0f), z(0.0f) {}
    /// Raw component constructor. GLM stores as (w, x, y, z).
    Quat(float w_, float x_, float y_, float z_) noexcept : w(w_), x(x_), y(y_), z(z_) {}
    /// Internal: construct from GLM type.
    explicit Quat(const glm::quat& q) noexcept : w(q.w), x(q.x), y(q.y), z(q.z) {}

    // -- Index access --
    auto operator[](int i) const noexcept -> float { return reinterpret_cast<const glm::quat&>(*this)[i]; }
    auto operator[](int i) noexcept -> float& { return reinterpret_cast<glm::quat&>(*this)[i]; }

    // -- GLM interop --
    auto glm() noexcept -> glm::quat& { return reinterpret_cast<glm::quat&>(*this); }
    auto glm() const noexcept -> const glm::quat& { return reinterpret_cast<const glm::quat&>(*this); }

    // -- Arithmetic --
    /// Quaternion composition (Hamilton product).
    friend auto operator*(Quat a, Quat b) noexcept -> Quat { return Quat{a.glm() * b.glm()}; }
    /// Rotate a vector by this quaternion: q * v * q^-1.
    friend auto operator*(Quat q, Vec3 v) noexcept -> Vec3 { return Vec3{q.glm() * v.glm()}; }
    auto operator*=(Quat other) noexcept -> Quat& { glm() *= other.glm(); return *this; }

    // -- Comparison --
    friend auto operator==(Quat a, Quat b) noexcept -> bool { return a.glm() == b.glm(); }
    friend auto operator!=(Quat a, Quat b) noexcept -> bool { return a.glm() != b.glm(); }

    // -- Quaternion operations --
    auto normalize() noexcept -> Quat&;
    auto normalized() const noexcept -> Quat;
    auto conjugate() const noexcept -> Quat;
    auto inverse() const noexcept -> Quat;
    auto to_mat4() const noexcept -> Mat4;

    /// Spherical linear interpolation between `a` and `b` at parameter `t` in [0, 1].
    static auto slerp(Quat a, Quat b, float t) noexcept -> Quat;

    // -- Static factories --
    static auto identity() noexcept -> Quat { return Quat{}; }

    /// Create a quaternion from an angle (radians) and axis.
    static auto angle_axis(float angle, Vec3 axis) noexcept -> Quat;

    /// Create a quaternion from Euler angles (pitch, yaw, roll) in radians.
    /// Convention: pitch around X, yaw around Y, roll around Z, applied in XYZ order.
    static auto from_euler(float pitch, float yaw, float roll) noexcept -> Quat;
};

// -- Out-of-body inline implementations (use .glm() accessor) --
inline auto Quat::normalize() noexcept -> Quat& {
    reinterpret_cast<glm::quat&>(*this) = glm::normalize(glm::quat(*this));
    return *this;
}
inline auto Quat::normalized() const noexcept -> Quat { return Quat{glm::normalize(glm())}; }
inline auto Quat::conjugate() const noexcept -> Quat { return Quat{glm::conjugate(glm())}; }
inline auto Quat::inverse() const noexcept -> Quat { return Quat{glm::inverse(glm())}; }
inline auto Quat::to_mat4() const noexcept -> Mat4 { return Mat4{glm::mat4_cast(glm())}; }

inline auto Quat::slerp(Quat a, Quat b, float t) noexcept -> Quat {
    return Quat{glm::slerp(a.glm(), b.glm(), t)};
}

inline auto Quat::angle_axis(float angle, Vec3 axis) noexcept -> Quat {
    return Quat{glm::angleAxis(angle, axis.glm())};
}

inline auto Quat::from_euler(float pitch, float yaw, float roll) noexcept -> Quat {
    return Quat{glm::quat(glm::vec3{pitch, yaw, roll})};
}

// -- Static assertions --
static_assert(std::is_standard_layout_v<Quat>, "Quat must be standard layout");
static_assert(sizeof(Quat) == sizeof(glm::quat), "Quat size must match glm::quat");
static_assert(std::is_trivially_copyable_v<Quat>, "Quat must be trivially copyable");

} // namespace buddd::engine::math
```

**Requirements:**
- `Quat()` creates identity `(w=1, x=0, y=0, z=0)` via `glm::quat(1.0f, 0.0f, 0.0f, 0.0f)`.
- `angle_axis()` uses `glm::angleAxis`.
- `from_euler()` uses `glm::quat(glm::vec3(pitch, yaw, roll))` — XYZ order (pitch->yaw->roll).
- `to_mat4()` uses `glm::mat4_cast`.
- `slerp()` uses `glm::slerp`. Handles identical quaternions (returns the input) and opposite quaternions (shortest path via GLM).
- `operator*(Quat, Vec3)` applies the rotation to the vector.
- `inverse()` for unit quaternions = conjugate; for non-unit, computes full inverse.
- All static methods are `noexcept` (pure GLM delegation).
- Includes `"mat4.h"` (for `to_mat4()` return type).

### 7. `src/engine/math/math.h`

Convenience header that includes all math types and provides utility functions and constants.

```cpp
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

#include "vec2.h"
#include "vec3.h"
#include "vec4.h"
#include "mat4.h"
#include "quat.h"
#include "camera.h"

namespace buddd::engine::math {

// -- Mathematical constants (constexpr variables) --
inline constexpr float pi       = 3.14159265358979323846f;
inline constexpr float half_pi  = 1.57079632679489661923f;
inline constexpr float two_pi   = 6.28318530717958647693f;
inline constexpr float epsilon  = 1.0e-6f;

// -- Conversion --
inline auto radians(float degrees) noexcept -> float { return glm::radians(degrees); }
inline auto degrees(float radians) noexcept -> float { return glm::degrees(radians); }

// -- Common math functions --
inline auto sin(float angle) noexcept -> float { return glm::sin(angle); }
inline auto cos(float angle) noexcept -> float { return glm::cos(angle); }
inline auto tan(float angle) noexcept -> float { return glm::tan(angle); }
inline auto asin(float x) noexcept -> float { return glm::asin(x); }
inline auto acos(float x) noexcept -> float { return glm::acos(x); }
inline auto atan(float y_over_x) noexcept -> float { return glm::atan(y_over_x); }
inline auto atan2(float y, float x) noexcept -> float { return glm::atan(y, x); }
inline auto sqrt(float x) noexcept -> float { return glm::sqrt(x); }

} // namespace buddd::engine::math
```

**Requirements:**
- Must include ALL math headers (`vec2.h`, `vec3.h`, `vec4.h`, `mat4.h`, `quat.h`, `camera.h`).
- `pi`, `half_pi`, `two_pi`, `epsilon` are `inline constexpr float` variables, NOT functions.
- `radians()` and `degrees()` delegate to `glm::radians` and `glm::degrees`.
- `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `sqrt` delegate to GLM (`glm::sin`, etc.).
- All functions use `inline` (header-only).
- Includes `<glm/glm.hpp>`, `<glm/gtc/constants.hpp>`, `<glm/trigonometric.hpp>`.

### 8. `src/engine/math/camera.h`

Declaration-only header. No GLM includes.

```cpp
#pragma once

#include "vec3.h"
#include "quat.h"
#include "mat4.h"

namespace buddd::engine::math {

class Camera {
public:
    /// Default: position (0,0,0), identity orientation, 60 FOV, 16:9 aspect, near 0.1, far 100.
    Camera() = default;

    /// Convenience constructor: sets position, orientation, and perspective parameters.
    Camera(Vec3 position, Quat orientation,
           float fov_y, float aspect, float near_plane, float far_plane);

    // -- Position / orientation --
    auto position() const noexcept -> Vec3;
    auto set_position(Vec3 position) -> void;

    auto orientation() const noexcept -> Quat;
    auto set_orientation(Quat orientation) -> void;

    // -- Look-at convenience --
    /// Orients the camera to look at `target` without changing position.
    auto look_at(Vec3 target) -> void;
    /// Sets both position and orientation to look from `eye` at `center` with given `up`.
    auto look_at(Vec3 eye, Vec3 center, Vec3 up) -> void;

    // -- Projection parameters (perspective) --
    auto set_perspective(float fov_y, float aspect, float near, float far) -> void;
    auto fov_y() const noexcept -> float;
    auto aspect() const noexcept -> float;
    auto near_plane() const noexcept -> float;
    auto far_plane() const noexcept -> float;

    // -- Matrix computation (recomputed on each call; no caching) --
    auto view_matrix() const -> Mat4;
    auto projection_matrix() const -> Mat4;
    auto view_projection_matrix() const -> Mat4;

private:
    Vec3 position_{0.0f, 0.0f, 0.0f};
    Quat orientation_{Quat::identity()};
    float fov_y_{1.0471975512f};        // 60 degrees in radians (pi/3)
    float aspect_{16.0f / 9.0f};
    float near_{0.1f};
    float far_{100.0f};
};

} // namespace buddd::engine::math
```

**Requirements:**
- No GLM includes in `camera.h` — only engine math wrapper types.
- All method signatures match the spec exactly.
- The `Camera` class is NOT header-only (methods are implemented in `camera.cpp`).

### 9. `src/engine/math/camera.cpp`

```cpp
#include "camera.h"

#include <glm/mat3x3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace buddd::engine::math {

Camera::Camera(Vec3 position, Quat orientation,
               float fov_y, float aspect, float near_plane, float far_plane)
    : position_(position)
    , orientation_(orientation)
    , fov_y_(fov_y)
    , aspect_(aspect)
    , near_(near_plane)
    , far_(far_plane)
{}

auto Camera::position() const noexcept -> Vec3 { return position_; }
auto Camera::set_position(Vec3 position) -> void { position_ = position; }

auto Camera::orientation() const noexcept -> Quat { return orientation_; }
auto Camera::set_orientation(Quat orientation) -> void { orientation_ = orientation; }

void Camera::look_at(Vec3 target) {
    look_at(position_, target, Vec3::unit_y());
}

void Camera::look_at(Vec3 eye, Vec3 center, Vec3 up) {
    position_ = eye;
    Vec3 forward = (center - eye).normalized();
    Vec3 right = forward.cross(up).normalized();
    Vec3 ortho_up = right.cross(forward);

    // Build column-major 3x3 rotation matrix: [right | ortho_up | -forward]
    glm::mat3 rot_mat(1.0f);
    rot_mat[0] = right.glm();
    rot_mat[1] = ortho_up.glm();
    rot_mat[2] = (-forward).glm();

    orientation_ = Quat{glm::quat_cast(rot_mat)};
}

auto Camera::fov_y() const noexcept -> float { return fov_y_; }
auto Camera::aspect() const noexcept -> float { return aspect_; }
auto Camera::near_plane() const noexcept -> float { return near_; }
auto Camera::far_plane() const noexcept -> float { return far_; }

void Camera::set_perspective(float fov_y, float aspect, float near, float far) {
    fov_y_ = fov_y;
    aspect_ = aspect;
    near_ = near;
    far_ = far;
}

auto Camera::view_matrix() const -> Mat4 {
    Vec3 forward = orientation_ * Vec3(0.0f, 0.0f, -1.0f);
    Vec3 up = orientation_ * Vec3(0.0f, 1.0f, 0.0f);
    return Mat4::look_at(position_, position_ + forward, up);
}

auto Camera::projection_matrix() const -> Mat4 {
    return Mat4::perspective(fov_y_, aspect_, near_, far_);
}

auto Camera::view_projection_matrix() const -> Mat4 {
    return projection_matrix() * view_matrix();
}

} // namespace buddd::engine::math
```

**Requirements:**
- `camera.cpp` includes `<glm/gtc/quaternion.hpp>` and `<glm/gtc/matrix_transform.hpp>` directly (allowed — this is an implementation file inside `src/engine/math/`).
- `view_matrix()` computes `forward = orientation * (0, 0, -1)` and `up = orientation * (0, 1, 0)` using the wrapper's `operator*(Quat, Vec3)`, then delegates to `Mat4::look_at`.
- `projection_matrix()` delegates to `Mat4::perspective`.
- `view_projection_matrix()` returns `projection_matrix() * view_matrix()` (projection on the left — OpenGL convention).
- `look_at(Vec3 target)` delegates to `look_at(position_, target, Vec3::unit_y())`.
- `look_at(Vec3 eye, Vec3 center, Vec3 up)` constructs the orientation quaternion from the look-at rotation matrix via `glm::quat_cast`.
- No matrix caching — all `*_matrix()` methods recompute on each call.
- `Camera` is the ONLY type with a `.cpp` file. All other math types are header-only.

## Required tests

The following tests MUST be present in the `buddd_tests` binary. They are specified here for the test-author who will create the test files. The implementation-author does NOT create test files.

All tests comparing against GLM output must use a tolerance of `1e-5f`. All tests must be headless (no display, no GPU required).

### Vec2 tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-01 | `"Vec2 default constructor creates zero vector"` | `[math]` `[vec2]` | `Vec2{}` has `x == 0.0f` and `y == 0.0f`. |
| T-02 | `"Vec2(x,y) constructor"` | `[math]` `[vec2]` | `Vec2(3, 4)` has `x == 3` and `y == 4`. |
| T-03 | `"Vec2 arithmetic operators match GLM"` | `[math]` `[vec2]` | Each of `+`, `-`, `*`, `/` (component-wise), `*` (scalar both sides), unary `-`, compound assignment, matches equivalent GLM within `1e-5f`. |
| T-04 | `"Vec2 length/length_squared matches GLM"` | `[math]` `[vec2]` | For `Vec2(3, 4)`, `length() == 5.0f` and `length_squared() == 25.0f`. |
| T-05 | `"Vec2 normalize/normalized matches GLM"` | `[math]` `[vec2]` | `normalized()` returns a unit vector; `normalize()` mutates in-place. Both match GLM within `1e-5f`. |
| T-06 | `"Vec2 dot matches GLM"` | `[math]` `[vec2]` | `Vec2(1,2).dot(Vec2(3,4)) == glm::dot(glm::vec2(1,2), glm::vec2(3,4))` within `1e-5f`. |
| T-07 | `"Vec2 constants"` | `[math]` `[vec2]` | `zero()`, `one()`, `unit_x()`, `unit_y()` return correct values. |
| T-08 | `"Vec2 comparison operators"` | `[math]` `[vec2]` | `==` returns true for equal vectors, false for different. `!=` is the negation. |
| T-09 | `"Vec2 operator[] access"` | `[math]` `[vec2]` | `v[0] == v.x` and `v[1] == v.y`. Non-const `operator[]` returns a mutable reference. |

### Vec3 tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-10 | `"Vec3 constructors"` | `[math]` `[vec3]` | Default `Vec3()` is `(0,0,0)`. `Vec3(1,2,3)` has correct components. |
| T-11 | `"Vec3 arithmetic matches GLM"` | `[math]` `[vec3]` | Same pattern as T-03 for Vec3. |
| T-12 | `"Vec3 cross matches GLM"` | `[math]` `[vec3]` | `Vec3(1,0,0).cross(Vec3(0,1,0))` equals `Vec3(0,0,1)` within `1e-5f`. Matches `glm::cross`. |
| T-13 | `"Vec3 lerp matches GLM"` | `[math]` `[vec3]` | `Vec3(0,0,0).lerp(Vec3(10,10,10), 0.5)` equals `(5,5,5)` within `1e-5f`. At t=0 returns `a`, at t=1 returns `b`. |
| T-14 | `"Vec3 constants"` | `[math]` `[vec3]` | `unit_z()` returns `Vec3(0,0,1)`. |
| T-15 | `"Vec3 normalize on zero length returns NaN"` | `[math]` `[vec3]` | `Vec3{}.normalized()` produces NaN components (via `std::isnan`). |
| T-16 | `"Vec3 dot/cross/length matches GLM"` | `[math]` `[vec3]` | All match `glm::` equivalents within `1e-5f`. |

### Vec4 tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-17 | `"Vec4 constructors"` | `[math]` `[vec4]` | Default `Vec4()` is `(0,0,0,0)`. `Vec4(1,2,3,4)` has correct components. |
| T-18 | `"Vec4 arithmetic matches GLM"` | `[math]` `[vec4]` | Same pattern as Vec2/Vec3. |
| T-19 | `"Vec4 unit_w"` | `[math]` `[vec4]` | `Vec4::unit_w()` returns `Vec4(0,0,0,1)`. |
| T-20 | `"Vec4 dot/length matches GLM"` | `[math]` `[vec4]` | Matches `glm::` equivalents within `1e-5f`. |

### Mat4 tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-21 | `"Mat4 default constructor is identity"` | `[math]` `[mat4]` | `Mat4{}` equals `glm::mat4(1.0f)` within `1e-5f`. |
| T-22 | `"Mat4(diagonal) constructor"` | `[math]` `[mat4]` | `Mat4(5.0f)` equals `glm::mat4(5.0f)` within `1e-5f`. |
| T-23 | `"Mat4 arithmetic matches GLM"` | `[math]` `[mat4]` | `+`, `-`, `*`, `*=` operator results match GLM within `1e-5f`. |
| T-24 | `"Mat4 * Vec4 matches GLM"` | `[math]` `[mat4]` | `Mat4{} * Vec4(1,2,3,4)` matches `glm::mat4(1.0f) * glm::vec4(1,2,3,4)` within `1e-5f`. |
| T-25 | `"Mat4 * Vec3 matches GLM"` | `[math]` `[mat4]` | Identity `Mat4{} * Vec3(1,2,3)` equals `Vec3(1,2,3)` (promoted to homogeneous). Matches `glm::mat4(1.0f) * glm::vec3(1,2,3)`. |
| T-26 | `"Vec3 * Mat4 matches GLM"` | `[math]` `[mat4]` | Row-vector multiplication matches GLM within `1e-5f`. |
| T-27 | `"Mat4 transpose matches GLM"` | `[math]` `[mat4]` | `m.transpose()` matches `glm::transpose(glm_m)` within `1e-5f`. |
| T-28 | `"Mat4 determinant matches GLM"` | `[math]` `[mat4]` | `m.determinant()` matches `glm::determinant(glm_m)` within `1e-5f`. |
| T-29 | `"Mat4 inverse matches GLM"` | `[math]` `[mat4]` | `m.inverse()` matches `glm::inverse(glm_m)` within `1e-5f`. |
| T-30 | `"Mat4 perspective matches GLM"` | `[math]` `[mat4]` | `Mat4::perspective(...)` matches `glm::perspective(...)` within `1e-5f`. |
| T-31 | `"Mat4 ortho matches GLM"` | `[math]` `[mat4]` | `Mat4::ortho(...)` matches `glm::ortho(...)` within `1e-5f`. |
| T-32 | `"Mat4 look_at matches GLM"` | `[math]` `[mat4]` | `Mat4::look_at(...)` matches `glm::lookAt(...)` within `1e-5f`. |
| T-33 | `"Mat4 translate matches GLM"` | `[math]` `[mat4]` | `Mat4::translate(Vec3(1,2,3))` matches `glm::translate(glm::mat4(1.0f), glm::vec3(1,2,3))` within `1e-5f`. |
| T-34 | `"Mat4 rotate matches GLM"` | `[math]` `[mat4]` | `Mat4::rotate(pi/2, Vec3::unit_y())` matches `glm::rotate(glm::mat4(1.0f), pi/2, glm::vec3(0,1,0))` within `1e-5f`. |
| T-35 | `"Mat4 scale matches GLM"` | `[math]` `[mat4]` | `Mat4::scale(Vec3(2,3,4))` matches `glm::scale(glm::mat4(1.0f), glm::vec3(2,3,4))` within `1e-5f`. |
| T-36 | `"Mat4 column-major layout"` | `[math]` `[mat4]` | Verify that `m[col][row]` access follows column-major order; internal storage matches `glm::mat4` layout byte-for-byte via `memcmp`. |
| T-37 | `"Mat4 operator[] returns Vec4& for non-const"` | `[math]` `[mat4]` | Modifying `m[1]` modifies the second column of the matrix. |

### Quat tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-38 | `"Quat default constructor is identity"` | `[math]` `[quat]` | `Quat{}` equals `glm::quat(1,0,0,0)` within `1e-5f`. |
| T-39 | `"Quat component constructor"` | `[math]` `[quat]` | `Quat(w,x,y,z)` stores correct components. |
| T-40 | `"Quat composition matches GLM"` | `[math]` `[quat]` | `q1 * q2` matches `glm::quat` multiplication within `1e-5f`. |
| T-41 | `"Quat rotate vector matches GLM"` | `[math]` `[quat]` | `q * Vec3(1,0,0)` matches `glm::quat * glm::vec3` within `1e-5f`. |
| T-42 | `"Quat conjugate matches GLM"` | `[math]` `[quat]` | `q.conjugate()` matches `glm::conjugate(q.glm())` within `1e-5f`. |
| T-43 | `"Quat inverse matches GLM"` | `[math]` `[quat]` | `q.inverse()` matches `glm::inverse(q.glm())` within `1e-5f`. |
| T-44 | `"Quat to_mat4 matches GLM"` | `[math]` `[quat]` | `q.to_mat4()` matches `glm::mat4_cast(q.glm())` within `1e-5f`. |
| T-45 | `"Quat slerp matches GLM"` | `[math]` `[quat]` | `Quat::slerp(a,b,0.0) == a`, `slerp(a,b,1.0) == b`, `slerp(a,b,0.5)` is halfway. All match GLM within `1e-5f`. |
| T-46 | `"Quat angle_axis matches GLM"` | `[math]` `[quat]` | `Quat::angle_axis(pi/2, Vec3::unit_y())` matches `glm::angleAxis(pi/2, glm::vec3(0,1,0))` within `1e-5f`. |
| T-47 | `"Quat from_euler matches GLM"` | `[math]` `[quat]` | `Quat::from_euler(0, pi/2, 0)` matches `glm::quat(glm::vec3(0, pi/2, 0))` within `1e-5f`. |
| T-48 | `"Quat normalize matches GLM"` | `[math]` `[quat]` | `q.normalized()` matches `glm::normalize(q.glm())` within `1e-5f`. |

### Camera tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-49 | `"Camera default constructor"` | `[math]` `[camera]` | Default `Camera` has position `(0,0,0)`, identity orientation, FOV ~1.047 rad, aspect 16/9, near 0.1, far 100. |
| T-50 | `"Camera parameterized constructor"` | `[math]` `[camera]` | Constructor sets all fields correctly. |
| T-51 | `"Camera position/orientation getters/setters"` | `[math]` `[camera]` | `set_position()` and `set_orientation()` update, `position()` and `orientation()` reflect changes. |
| T-52 | `"Camera projection_matrix matches GLM"` | `[math]` `[camera]` | `cam.projection_matrix()` matches `glm::perspective(fov, aspect, near, far)` within `1e-5f`. |
| T-53 | `"Camera view_matrix matches GLM"` | `[math]` `[camera]` | With position `(0,2,5)` and identity orientation, `view_matrix()` matches `glm::lookAt((0,2,5), (0,2,4), (0,1,0))` (forward = (0,0,-1), up = (0,1,0)). Within `1e-5f`. |
| T-54 | `"Camera view_projection_matrix order"` | `[math]` `[camera]` | `view_projection_matrix()` equals `projection_matrix() * view_matrix()` within `1e-5f`. |
| T-55 | `"Camera look_at orients correctly"` | `[math]` `[camera]` | Calling `cam.look_at(target)` orients the camera so that `forward = orientation * (0,0,-1)` points toward target. |
| T-56 | `"Camera look_at(eye,center,up) sets position and orientation"` | `[math]` `[camera]` | After `cam.look_at({0,0,0}, {5,0,0}, {0,1,0})`, position is `(0,0,0)` and orientation rotates `(0,0,-1)` toward `(5,0,0)`. |

### Utility and integration tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-57 | `"radians and degrees conversions"` | `[math]` `[utility]` | `radians(180.0f)` approx equals `pi`. `degrees(pi)` approx equals `180.0f`. Tolerance `1e-5f`. |
| T-58 | `"constants values"` | `[math]` `[utility]` | `pi`, `half_pi`, `two_pi`, `epsilon` have expected values (pi ~3.14159, half_pi ~1.5708, two_pi ~6.28319, epsilon == 1e-6f). |
| T-59 | `"math sin/cos/tan match GLM"` | `[math]` `[utility]` | `sin(0) == 0`, `cos(0) == 1`, `tan(0) == 0`. `sin(pi/2) ≈ 1`, `cos(pi) ≈ -1`. All match GLM within `1e-5f`. |
| T-60 | `"math asin/acos/atan match GLM"` | `[math]` `[utility]` | `asin(0) == 0`, `acos(1) == 0`, `atan(0) == 0`, `atan2(1, 0) ≈ pi/2`. All match GLM within `1e-5f`. |
| T-61 | `"math sqrt matches GLM"` | `[math]` `[utility]` | `sqrt(4.0f) == 2.0f`, `sqrt(0.0f) == 0.0f`. Matches GLM within `1e-5f`. |
| T-62 | `"GLM interop via glm() accessor"` | `[math]` `[interop]` | For each type `T` (Vec2, Vec3, Vec4, Mat4, Quat), `v.glm()` returns a mutable reference to the underlying GLM type. Type trait checks: `std::is_same_v<decltype(v.glm()), GlmType&>` compiles. |
| T-63 | `"Static assertions compile"` | `[math]` `[compile]` | All `static_assert` checks for `is_standard_layout_v`, `sizeof` equality, and `is_trivially_copyable_v` pass (compile-time). |
| T-64 | `"Convenience header math.h includes all types"` | `[math]` `[integration]` | Including `"engine/math/math.h"` makes all math types available. Instantiating `buddd::engine::math::Vec3`, `Mat4`, `Quat`, `Camera` compiles. |
| T-65 | `"GLM types not in public API"` | `[math]` `[interop]` | No `glm::` prefix appears in any public method signature of the wrapper headers, except in the `.glm()` accessor's return type. Verified by scanning header declarations (e.g., grep for `glm::` excluding `.glm()` lines). |

### Edge case tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-66 | `"Vec3 normalize zero vector produces NaN"` | `[math]` `[edge]` | `Vec3{}.normalized()`: all components are NaN. `std::isnan` returns true. |
| T-67 | `"Mat4 inverse singular matrix"` | `[math]` `[edge]` | Create a zero matrix via `Mat4{} * 0.0f`. Calling `.inverse()` on it produces NaN/inf components (verified via `std::isnan` or `std::isinf`). |
| T-68 | `"Quat slerp identical quaternions"` | `[math]` `[edge]` | `Quat::slerp(q, q, 0.5)` returns `q` within `1e-5f`. |
| T-69 | `"Mat4 * Vec3 with zero matrix"` | `[math]` `[edge]` | `(Mat4{} * 0.0f) * Vec3(1,2,3)` equals Vec3(0,0,0). |
| T-70 | `"Division by zero in scalar operator/"` | `[math]` `[edge]` | `Vec3(1,2,3) / 0.0f` produces inf/NaN components (via `std::isinf` or `std::isnan`). |
| T-71 | `"Mat4::ortho degenerate parameters"` | `[math]` `[edge]` | `Mat4::ortho(1, -1, ...)` — GLM behavior is accepted (produces a degenerate matrix). No crash. |

## Edge cases

| Case | Expected behavior |
|---|---|
| `Vec3::normalize()` on zero-length vector | Returns a vector with NaN components (GLM behavior: division by zero produces NaN). The caller must check for zero length before normalizing. |
| `Mat4::inverse()` on singular matrix | Delegates to `glm::inverse`, which produces NaN/inf components for singular matrices. No error detection. |
| `Mat4 * Vec3` transform | Equivalent to `(m * Vec4(v, 1.0f)).xyz()` — Vec3 is promoted to homogeneous, transformed as a column vector (OpenGL convention). |
| `Vec3 * Mat4` transform | Equivalent to `(Vec4(v, 1.0f) * m).xyz()` — Vec3 is promoted to homogeneous, transformed as a row vector. |
| `Vec2::operator*` with mixed component-wise and scalar types | All operators are unambiguous: component-wise `*` for `Vec * Vec`, scalar `*` for `Vec * float` and `float * Vec`. |
| `Camera` default construction | Position `(0,0,0)`, identity orientation, FOV 60 deg (1.047 rad), aspect 16:9, near 0.1, far 100. `view_matrix()` returns an identity view (camera at origin looking down -Z). |
| `Camera::look_at()` with up vector parallel to direction | The result is implementation-defined (GLM `glm::lookAt` behavior — the up vector is adjusted). The wrapper delegates to GLM and accepts its behavior. |
| `Quat::angle_axis()` with zero-length axis | Returns identity quaternion (GLM behavior). |
| `Quat::slerp()` with identical quaternions | Returns the input quaternion (GLM handles this). |
| `Quat::slerp()` with opposite quaternions | GLM resolves by falling back to linear interpolation on the shortest path. Accepted as GLM behavior. |
| `operator[]` with out-of-bounds index | Behavior is undefined (GLM uses `glm::assert` in debug builds; no bounds checking in release). The caller must ensure `0 <= i < dimension`. |
| Division by zero in scalar `operator/` | Produces inf/NaN components (GLM behavior). No exception or error. |
| `Mat4::perspective()` with near <= 0 or far <= near | Returns a degenerate projection matrix (GLM behavior). The caller is responsible for valid parameters. |
| `Mat4::perspective()` with fov_y <= 0.0f or fov_y >= pi | Returns a degenerate projection matrix (GLM behavior). |
| `Mat4::ortho()` with left >= right or bottom >= top | Returns a degenerate orthographic matrix (GLM behavior). |

## Security impact

None. Math types are pure computation — no I/O, no network, no filesystem access. No elevated privileges required. No secrets, credentials, or environment variables consumed. GLM is a header-only math library with no platform-specific dependencies.

## Data and migration impact

None. No persistent state, database, or file format is introduced. Math types are purely in-memory computational primitives.

## API compatibility impact

The following public API surface is introduced. All types are in namespace `buddd::engine::math`.

```cpp
// Constants (inline constexpr float)
constexpr float pi;
constexpr float half_pi;
constexpr float two_pi;
constexpr float epsilon;

// Conversion
auto radians(float degrees) noexcept -> float;
auto degrees(float radians) noexcept -> float;

// Common math
auto sin(float angle) noexcept -> float;
auto cos(float angle) noexcept -> float;
auto tan(float angle) noexcept -> float;
auto asin(float x) noexcept -> float;
auto acos(float x) noexcept -> float;
auto atan(float y_over_x) noexcept -> float;
auto atan2(float y, float x) noexcept -> float;
auto sqrt(float x) noexcept -> float;
```

**Backward compatibility**: This is the first version of these APIs. All types are introduced in this contract. Once accepted, changing any of the following constitutes a breaking change:
- Namespace, struct/class name, or enum value name.
- Function signature, return type, or parameter type.
- Adding or removing public methods that existing consumers may depend on.
- Changing the memory layout (size, alignment, standard layout guarantees).
- Changing `.glm()` accessor behavior or removing it.

## Documentation impact

- No README, wiki page, or other documentation files are created or modified.
- The spec (`.specs/sprint-2026-05/math-foundations/spec.md`) remains authoritative.
- The `SpecKit.md` and `AGENTS.md` remain untouched.
- The API surface described above is the public contract.

## ADR impact

None. No architectural decision requires an ADR — the patterns (thin GLM wrappers, header-only design, FetchContent for GLM, Camera as a perspective view) are design decisions documented in the spec.

## Constitution impact

None. No constitution rules need to be added or amended.

## Done criteria

The implementation is complete when all of the following are true:

1. **Files exist**: All 9 files listed in "Files allowed to change" exist with correct content that matches the required implementation behavior.
2. **`src/engine/CMakeLists.txt` modified**: The updated file includes the GLM FetchContent block with tag `1.0.1` and links `glm::glm` as PUBLIC.
3. **Build succeeds**: `cmake --preset debug && cmake --build --preset debug` exits 0 with zero warnings related to the new math source files on the reference compiler.
4. **Release preset works**: `cmake --preset release && cmake --build --preset release` succeeds.
5. **Architecture boundary verified**: Running `grep -rn 'glm/' src/engine/ --include='*.h' --include='*.cpp' | grep -v 'src/engine/math/' | grep -v 'CMakeLists.txt'` returns zero matches. No GLM headers are included outside `src/engine/math/`.
6. **GLM types not in public method signatures** (`.glm()` accessor is the only exception): Running `grep -n 'glm::' src/engine/math/math.h src/engine/math/camera.h` should only match lines containing `.glm()`. Implementation bodies may use `glm::` freely (they are inside `src/engine/math/`). The check is that no GLM type appears as a parameter or return type of a public method outside the `.glm()` accessor.
7. **Static assertions compile**: All `static_assert` checks for standard layout, size, and trivially copyable pass for all 5 primitive types.
8. **`.glm()` accessor works**: Code like `Vec3 v; glm::vec3& ref = v.glm();` compiles and returns a valid reference.
9. **Camera compiles**: Code using `Camera` with position, orientation, look_at, view_matrix, projection_matrix, view_projection_matrix compiles.
10. **Convenience header works**: `#include <engine/math/math.h>` makes all math types available.
11. **No math `.cpp` files for primitives**: Only `camera.cpp` exists as a `.cpp` in `src/engine/math/`. All other types are header-only.
12. **Memory layout verified**: `Vec3` is 12 bytes (3 floats), `Mat4` is 64 bytes (16 floats), `Quat` is 16 bytes (4 floats).
13. **Non-modified forbidden files**: `src/engine/version.h`, `src/engine/version.cpp`, `src/engine/error.h`, root `CMakeLists.txt`, `CMakePresets.json`, and all files in `src/cmd/`, `src/editor/`, `tests/`, `src/engine/platform/`, `src/engine/window/`, `src/engine/render/` remain unchanged.

## Verification commands (copy-paste ready)

```bash
# Configure and build
cmake --preset debug
cmake --build --preset debug

# Verify architecture boundary (no GLM includes outside math/)
grep -rn 'glm/' src/engine/ --include='*.h' --include='*.cpp' | grep -v 'src/engine/math/' | grep -v 'CMakeLists.txt'
# Expected: zero matches

# Verify GLM types not in public API (only .glm() return types allowed)
grep -n 'glm::' src/engine/math/math.h src/engine/math/camera.h
# Expected: zero matches (these files should NOT use GLM types directly)

# Verify no unexpected .cpp files in math/
ls -la src/engine/math/*.cpp
# Expected: only camera.cpp

# Verify memory layout
grep -r 'sizeof' src/engine/math/ --include='*.h' | grep static_assert
# Expected: all static_assert(sizeof(...) == sizeof(...)) lines present

# Verify forbidden files are unchanged
git diff --name-only
# Should NOT include: version.h, version.cpp, error.h, root CMakeLists.txt, CMakePresets.json,
# anything in src/cmd/, src/editor/, tests/, src/engine/platform/, src/engine/window/, src/engine/render/

# Build release preset (optional, P2)
cmake --preset release
cmake --build --preset release
```
