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
    return x * x + y * y;
}
inline auto Vec2::normalize() noexcept -> Vec2& {
    glm() = glm::normalize(glm());
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
