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
        return x * x + y * y + z * z;
    }
    auto normalize() noexcept -> Vec3& {
        glm() = glm::normalize(glm());
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
