#pragma once

#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <type_traits>

#include "vec3.h"
#include "vec4.h"

namespace buddd::engine::math {

struct Color {
    // -- Public members (same layout as glm::vec4) --
    float r, g, b, a;

    // -- Constructors --
    Color() noexcept : r(0.0f), g(0.0f), b(0.0f), a(0.0f) {}
    Color(float r_, float g_, float b_) noexcept : r(r_), g(g_), b(b_), a(1.0f) {}
    Color(float r_, float g_, float b_, float a_) noexcept : r(r_), g(g_), b(b_), a(a_) {}
    explicit Color(const glm::vec4& v) noexcept : r(v.x), g(v.y), b(v.z), a(v.w) {}

    // -- Index access --
    auto operator[](int i) const noexcept -> float { return reinterpret_cast<const glm::vec4&>(*this)[i]; }
    auto operator[](int i) noexcept -> float& { return reinterpret_cast<glm::vec4&>(*this)[i]; }

    // -- GLM interop --
    auto glm() noexcept -> glm::vec4& { return reinterpret_cast<glm::vec4&>(*this); }
    auto glm() const noexcept -> const glm::vec4& { return reinterpret_cast<const glm::vec4&>(*this); }

    // -- Arithmetic operators (component-wise) --
    friend auto operator+(Color a, Color b) noexcept -> Color {
        return Color{a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a};
    }
    friend auto operator-(Color a, Color b) noexcept -> Color {
        return Color{a.r - b.r, a.g - b.g, a.b - b.b, a.a - b.a};
    }
    friend auto operator*(Color a, Color b) noexcept -> Color {
        return Color{a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a};
    }
    friend auto operator/(Color a, Color b) noexcept -> Color {
        return Color{a.r / b.r, a.g / b.g, a.b / b.b, a.a / b.a};
    }
    friend auto operator*(Color v, float s) noexcept -> Color {
        return Color{v.r * s, v.g * s, v.b * s, v.a * s};
    }
    friend auto operator*(float s, Color v) noexcept -> Color {
        return Color{s * v.r, s * v.g, s * v.b, s * v.a};
    }
    friend auto operator/(Color v, float s) noexcept -> Color {
        return Color{v.r / s, v.g / s, v.b / s, v.a / s};
    }

    auto operator-() const noexcept -> Color { return Color{-r, -g, -b, -a}; }

    auto operator+=(Color other) noexcept -> Color& {
        r += other.r; g += other.g; b += other.b; a += other.a; return *this;
    }
    auto operator-=(Color other) noexcept -> Color& {
        r -= other.r; g -= other.g; b -= other.b; a -= other.a; return *this;
    }
    auto operator*=(Color other) noexcept -> Color& {
        r *= other.r; g *= other.g; b *= other.b; a *= other.a; return *this;
    }
    auto operator/=(Color other) noexcept -> Color& {
        r /= other.r; g /= other.g; b /= other.b; a /= other.a; return *this;
    }
    auto operator*=(float s) noexcept -> Color& { r *= s; g *= s; b *= s; a *= s; return *this; }
    auto operator/=(float s) noexcept -> Color& { r /= s; g /= s; b /= s; a /= s; return *this; }

    // -- Comparison --
    friend auto operator==(Color a, Color b) noexcept -> bool {
        return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
    }
    friend auto operator!=(Color a, Color b) noexcept -> bool { return !(a == b); }

    // -- Color-specific operations --

    /// Returns a Vec3 containing r, g, b (discards alpha).
    auto to_vec3() const noexcept -> Vec3 {
        return Vec3{r, g, b};
    }

    /// Returns a Vec4 containing r, g, b, a.
    auto to_vec4() const noexcept -> Vec4 {
        return Vec4{r, g, b, a};
    }

    /// Applies sRGB→linear transfer function to r, g, b; alpha unchanged.
    auto to_linear() const noexcept -> Color {
        auto srgb_to_linear = [](float c) noexcept -> float {
            if (c <= 0.04045f) {
                return c / 12.92f;
            }
            return std::pow((c + 0.055f) / 1.055f, 2.4f);
        };
        return Color{srgb_to_linear(r), srgb_to_linear(g), srgb_to_linear(b), a};
    }

    /// Applies linear→sRGB transfer function to r, g, b; alpha unchanged.
    auto to_srgb() const noexcept -> Color {
        auto linear_to_srgb = [](float c) noexcept -> float {
            if (c <= 0.0031308f) {
                return c * 12.92f;
            }
            return 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
        };
        return Color{linear_to_srgb(r), linear_to_srgb(g), linear_to_srgb(b), a};
    }

    /// Luminance (Rec. 709 / sRGB coefficients).
    auto luminance() const noexcept -> float {
        return 0.2126f * r + 0.7152f * g + 0.0722f * b;
    }

    /// Darkens by factor: rgb * (1 - factor). Godot-style.
    auto darkened(float factor) const noexcept -> Color {
        float f = 1.0f - factor;
        return Color{r * f, g * f, b * f, a};
    }

    /// Lightens by factor: rgb + (1 - rgb) * factor. Godot-style.
    auto lightened(float factor) const noexcept -> Color {
        return Color{
            r + (1.0f - r) * factor,
            g + (1.0f - g) * factor,
            b + (1.0f - b) * factor,
            a
        };
    }

    /// Standard over-blend (straight alpha).
    static auto blend(Color fg, Color bg) noexcept -> Color {
        float out_a = fg.a + bg.a * (1.0f - fg.a);
        if (out_a == 0.0f) {
            return Color{0.0f, 0.0f, 0.0f, 0.0f};
        }
        float inv_fg_a = 1.0f - fg.a;
        return Color{
            (fg.r * fg.a + bg.r * bg.a * inv_fg_a) / out_a,
            (fg.g * fg.a + bg.g * bg.a * inv_fg_a) / out_a,
            (fg.b * fg.a + bg.b * bg.a * inv_fg_a) / out_a,
            out_a
        };
    }

    /// RGB→HSV. H in [0, 1) where 0=red, 1/3=green, 2/3=blue; S and V in [0, 1].
    auto to_hsv() const noexcept -> Vec3 {
        float max_c = std::max({r, g, b});
        float min_c = std::min({r, g, b});
        float delta = max_c - min_c;

        float h = 0.0f;
        if (delta != 0.0f) {
            if (max_c == r) {
                h = std::fmod((g - b) / delta, 6.0f);
            } else if (max_c == g) {
                h = (b - r) / delta + 2.0f;
            } else {
                h = (r - g) / delta + 4.0f;
            }
            h /= 6.0f;
            if (h < 0.0f) h += 1.0f;
        }

        float s = (max_c == 0.0f) ? 0.0f : delta / max_c;
        float v = max_c;

        return Vec3{h, s, v};
    }

    /// HSV→RGB. h in [0, 1) where 0=red, 1/3=green, 2/3=blue; s and v in [0, 1].
    static auto from_hsv(float h, float s, float v) noexcept -> Color {
        float h6 = h * 6.0f;
        int sector = static_cast<int>(std::floor(h6));
        float f = h6 - static_cast<float>(sector);
        sector = sector % 6;
        if (sector < 0) sector += 6;

        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        switch (sector) {
            case 0: return Color{v, t, p, 1.0f};
            case 1: return Color{q, v, p, 1.0f};
            case 2: return Color{p, v, t, 1.0f};
            case 3: return Color{p, q, v, 1.0f};
            case 4: return Color{t, p, v, 1.0f};
            default: return Color{v, p, q, 1.0f};
        }
    }

    /// Convert to 32-bit RGBA (clamps each channel to [0, 1], multiplies by 255, rounds).
    auto to_rgba32() const noexcept -> std::array<uint8_t, 4> {
        auto clamp_round = [](float c) noexcept -> uint8_t {
            if (c < 0.0f) return 0;
            if (c > 1.0f) return 255;
            return static_cast<uint8_t>(std::round(c * 255.0f));
        };
        return {clamp_round(r), clamp_round(g), clamp_round(b), clamp_round(a)};
    }

    /// Create from 32-bit RGBA (divides each uint8 by 255.0f).
    static auto from_rgba32(std::array<uint8_t, 4> rgba) noexcept -> Color {
        return Color{
            rgba[0] / 255.0f,
            rgba[1] / 255.0f,
            rgba[2] / 255.0f,
            rgba[3] / 255.0f
        };
    }

    // -- Named color presets (alpha = 1.0f) --
    static constexpr auto white() noexcept -> Color { return Color{1.0f, 1.0f, 1.0f}; }
    static constexpr auto black() noexcept -> Color { return Color{0.0f, 0.0f, 0.0f}; }
    static constexpr auto red() noexcept -> Color { return Color{1.0f, 0.0f, 0.0f}; }
    static constexpr auto green() noexcept -> Color { return Color{0.0f, 1.0f, 0.0f}; }
    static constexpr auto blue() noexcept -> Color { return Color{0.0f, 0.0f, 1.0f}; }
    static constexpr auto yellow() noexcept -> Color { return Color{1.0f, 1.0f, 0.0f}; }
    static constexpr auto cyan() noexcept -> Color { return Color{0.0f, 1.0f, 1.0f}; }
    static constexpr auto magenta() noexcept -> Color { return Color{1.0f, 0.0f, 1.0f}; }

    // -- Constants --
    static constexpr auto zero() noexcept -> Color { return Color{0.0f, 0.0f, 0.0f, 0.0f}; }
    static constexpr auto one() noexcept -> Color { return Color{1.0f, 1.0f, 1.0f, 1.0f}; }
};

static_assert(std::is_standard_layout_v<Color>, "Color must be standard layout");
static_assert(sizeof(Color) == sizeof(glm::vec4), "Color size must match glm::vec4");
static_assert(std::is_trivially_copyable_v<Color>, "Color must be trivially copyable");

} // namespace buddd::engine::math
