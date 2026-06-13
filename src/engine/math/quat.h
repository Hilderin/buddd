#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <type_traits>

#include "vec3.h"
#include "mat4.h"

namespace buddd::engine::math {

struct Quat {
    // -- Public members (same layout as glm::quat: x, y, z, w order) --
    float x, y, z, w;

    // -- Constructors --
    /// Default: identity quaternion (w=1, x=0, y=0, z=0).
    /// NOTE: GLM stores quaternions as (x, y, z, w) by default.
    Quat() noexcept : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
    /// Raw component constructor. Parameters are in mathematical order (w, x, y, z)
    /// but stored internally in GLM order (x, y, z, w) for ABI compatibility.
    Quat(float w_, float x_, float y_, float z_) noexcept : x(x_), y(y_), z(z_), w(w_) {}
    /// Internal: construct from GLM type.
    explicit Quat(const glm::quat& q) noexcept : x(q.x), y(q.y), z(q.z), w(q.w) {}

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

    /// Convert quaternion to Euler angles (pitch, yaw, roll) in radians.
    /// Convention: pitch around X, yaw around Y, roll around Z, in XYZ order.
    /// Matches the convention of from_euler().
    [[nodiscard]] auto to_euler() const noexcept -> Vec3;
};

// -- Out-of-body inline implementations (use .glm() accessor) --
inline auto Quat::normalize() noexcept -> Quat& {
    glm() = glm::normalize(glm());
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

inline auto Quat::to_euler() const noexcept -> Vec3 {
    auto const euler = glm::eulerAngles(glm());
    return Vec3{euler.x, euler.y, euler.z};
}

// -- Static assertions --
static_assert(std::is_standard_layout_v<Quat>, "Quat must be standard layout");
static_assert(sizeof(Quat) == sizeof(glm::quat), "Quat size must match glm::quat");
static_assert(std::is_trivially_copyable_v<Quat>, "Quat must be trivially copyable");

} // namespace buddd::engine::math
