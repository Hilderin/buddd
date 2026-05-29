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
