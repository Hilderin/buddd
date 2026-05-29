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
        return x * x + y * y + z * z + w * w;
    }
    auto normalize() noexcept -> Vec4& {
        glm() = glm::normalize(glm());
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
