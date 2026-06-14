#include "math/color.h"
#include "math/color_yaml.h"

#include <glm/vec4.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <array>
#include <cmath>
#include <type_traits>

using namespace buddd::engine::math;
using Catch::Approx;

namespace {
    constexpr float TOL = 1e-5f;
}

// ===========================================================================
// Layout checks (AC-001)
// ===========================================================================
TEST_CASE("Color layout satisfies triple static_assert", "[math][color]") {
    REQUIRE(std::is_standard_layout_v<Color>);
    REQUIRE(sizeof(Color) == sizeof(glm::vec4));
    REQUIRE(std::is_trivially_copyable_v<Color>);
}

// ===========================================================================
// GLM interop (AC-002)
// ===========================================================================
TEST_CASE("Color GLM interop via .glm()", "[math][color]") {
    Color c{1.0f, 0.5f, 0.25f, 0.75f};
    auto& glm_ref = c.glm();
    glm_ref.x = 0.0f;
    glm_ref.y = 0.0f;
    glm_ref.z = 0.0f;
    glm_ref.w = 0.0f;
    REQUIRE(c.r == 0.0f);
    REQUIRE(c.g == 0.0f);
    REQUIRE(c.b == 0.0f);
    REQUIRE(c.a == 0.0f);

    // const version
    const Color cc{0.2f, 0.4f, 0.6f, 0.8f};
    const auto& gcc = cc.glm();
    REQUIRE(gcc.x == Approx(0.2f).margin(TOL));
    REQUIRE(gcc.y == Approx(0.4f).margin(TOL));
    REQUIRE(gcc.z == Approx(0.6f).margin(TOL));
    REQUIRE(gcc.w == Approx(0.8f).margin(TOL));
}

// ===========================================================================
// Default constructor (AC-003)
// ===========================================================================
TEST_CASE("Color default constructor creates zero color", "[math][color]") {
    const Color c;
    REQUIRE(c.r == 0.0f);
    REQUIRE(c.g == 0.0f);
    REQUIRE(c.b == 0.0f);
    REQUIRE(c.a == 0.0f);
}

// ===========================================================================
// RGBA/RGB constructors (AC-004)
// ===========================================================================
TEST_CASE("Color RGBA/RGB constructors", "[math][color]") {
    const Color rgb{1.0f, 0.0f, 0.0f};
    REQUIRE(rgb.r == 1.0f);
    REQUIRE(rgb.g == 0.0f);
    REQUIRE(rgb.b == 0.0f);
    REQUIRE(rgb.a == 1.0f);

    const Color rgba{1.0f, 0.0f, 0.0f, 0.5f};
    REQUIRE(rgba.r == 1.0f);
    REQUIRE(rgba.g == 0.0f);
    REQUIRE(rgba.b == 0.0f);
    REQUIRE(rgba.a == 0.5f);
}

// ===========================================================================
// Explicit Vec4 construction
// ===========================================================================
TEST_CASE("Color explicit Vec4 construction", "[math][color]") {
    glm::vec4 glm_v{1.0f, 0.0f, 0.0f, 1.0f};
    Color c{glm_v};
    REQUIRE(c.r == 1.0f);
    REQUIRE(c.g == 0.0f);
    REQUIRE(c.b == 0.0f);
    REQUIRE(c.a == 1.0f);
}

// ===========================================================================
// Index access
// ===========================================================================
TEST_CASE("Color index access", "[math][color]") {
    Color c{0.1f, 0.2f, 0.3f, 0.4f};
    REQUIRE(c[0] == Approx(0.1f).margin(TOL));
    REQUIRE(c[1] == Approx(0.2f).margin(TOL));
    REQUIRE(c[2] == Approx(0.3f).margin(TOL));
    REQUIRE(c[3] == Approx(0.4f).margin(TOL));

    c[0] = 0.5f;
    REQUIRE(c.r == Approx(0.5f).margin(TOL));
}

// ===========================================================================
// Arithmetic operators (AC-005)
// ===========================================================================
TEST_CASE("Color arithmetic operators match GLM reference", "[math][color]") {
    const Color a{3.0f, 4.0f, 5.0f, 6.0f};
    const Color b{1.0f, 2.0f, 3.0f, 4.0f};
    const float s = 2.0f;

    glm::vec4 ga{3.0f, 4.0f, 5.0f, 6.0f};
    glm::vec4 gb{1.0f, 2.0f, 3.0f, 4.0f};

    auto approx_color = [](const Color& c, const glm::vec4& g) {
        REQUIRE(c.r == Approx(g.x).margin(TOL));
        REQUIRE(c.g == Approx(g.y).margin(TOL));
        REQUIRE(c.b == Approx(g.z).margin(TOL));
        REQUIRE(c.a == Approx(g.w).margin(TOL));
    };

    SECTION("Color + Color") {
        approx_color(a + b, ga + gb);
    }
    SECTION("Color - Color") {
        approx_color(a - b, ga - gb);
    }
    SECTION("Color * Color") {
        approx_color(a * b, ga * gb);
    }
    SECTION("Color / Color") {
        approx_color(a / b, ga / gb);
    }
    SECTION("Color * float") {
        approx_color(a * s, ga * s);
    }
    SECTION("float * Color") {
        approx_color(s * a, s * ga);
    }
    SECTION("Color / float") {
        approx_color(a / s, ga / s);
    }
    SECTION("Unary minus") {
        approx_color(-a, -ga);
    }
    SECTION("Compound +=)") {
        Color tmp = a; tmp += b;
        approx_color(tmp, ga + gb);
    }
    SECTION("Compound -=)") {
        Color tmp = a; tmp -= b;
        approx_color(tmp, ga - gb);
    }
    SECTION("Compound *= Color") {
        Color tmp = a; tmp *= b;
        approx_color(tmp, ga * gb);
    }
    SECTION("Compound /= Color") {
        Color tmp = a; tmp /= b;
        approx_color(tmp, ga / gb);
    }
    SECTION("Compound *= float") {
        Color tmp = a; tmp *= s;
        approx_color(tmp, ga * s);
    }
    SECTION("Compound /= float") {
        Color tmp = a; tmp /= s;
        approx_color(tmp, ga / s);
    }
}

// ===========================================================================
// Comparison operators
// ===========================================================================
TEST_CASE("Color comparison operators", "[math][color]") {
    Color a{1.0f, 0.5f, 0.25f, 0.75f};
    Color b{1.0f, 0.5f, 0.25f, 0.75f};
    Color c{1.0f, 0.0f, 0.25f, 0.75f};

    REQUIRE(a == b);
    REQUIRE(a != c);
}

// ===========================================================================
// YAML encode (AC-006)
// ===========================================================================
TEST_CASE("Color YAML encode produces flow sequence", "[math][color]") {
    Color c{0.5f, 0.75f, 0.25f, 1.0f};
    YAML::Node node = YAML::convert<Color>::encode(c);
    REQUIRE(node.IsSequence());
    REQUIRE(node.size() == 4);
    REQUIRE(node[0].as<float>() == Approx(0.5f).margin(TOL));
    REQUIRE(node[1].as<float>() == Approx(0.75f).margin(TOL));
    REQUIRE(node[2].as<float>() == Approx(0.25f).margin(TOL));
    REQUIRE(node[3].as<float>() == Approx(1.0f).margin(TOL));
    REQUIRE(node.Style() == YAML::EmitterStyle::Flow);
}

// ===========================================================================
// YAML decode 4-element (AC-007)
// ===========================================================================
TEST_CASE("Color YAML decode 4-element sequence", "[math][color]") {
    YAML::Node node = YAML::Load("[0.5, 0.75, 0.25, 1.0]");
    Color c;
    bool ok = YAML::convert<Color>::decode(node, c);
    REQUIRE(ok);
    REQUIRE(c.r == Approx(0.5f).margin(TOL));
    REQUIRE(c.g == Approx(0.75f).margin(TOL));
    REQUIRE(c.b == Approx(0.25f).margin(TOL));
    REQUIRE(c.a == Approx(1.0f).margin(TOL));
}

// ===========================================================================
// YAML decode 3-element (backward compat) (AC-007)
// ===========================================================================
TEST_CASE("Color YAML decode 3-element sequence (backward compat)", "[math][color]") {
    YAML::Node node = YAML::Load("[0.5, 0.75, 0.25]");
    Color c;
    bool ok = YAML::convert<Color>::decode(node, c);
    REQUIRE(ok);
    REQUIRE(c.r == Approx(0.5f).margin(TOL));
    REQUIRE(c.g == Approx(0.75f).margin(TOL));
    REQUIRE(c.b == Approx(0.25f).margin(TOL));
    REQUIRE(c.a == Approx(1.0f).margin(TOL));
}

// ===========================================================================
// YAML decode invalid (AC-008)
// ===========================================================================
TEST_CASE("Color YAML decode invalid returns false", "[math][color]") {
    SECTION("Wrong size (2 elements)") {
        YAML::Node node = YAML::Load("[1.0, 2.0]");
        Color c;
        REQUIRE_FALSE(YAML::convert<Color>::decode(node, c));
    }
    SECTION("Non-sequence (map)") {
        YAML::Node node = YAML::Load("{r: 1.0, g: 2.0, b: 3.0}");
        Color c;
        REQUIRE_FALSE(YAML::convert<Color>::decode(node, c));
    }
    SECTION("Non-numeric elements") {
        YAML::Node node = YAML::Load("[0.5, \"abc\", 0.25, 1.0]");
        Color c;
        REQUIRE_FALSE(YAML::convert<Color>::decode(node, c));
    }
}

// ===========================================================================
// YAML roundtrip (AC-007)
// ===========================================================================
TEST_CASE("Color YAML roundtrip", "[math][color]") {
    Color original{0.3f, 0.6f, 0.9f, 0.5f};
    YAML::Node encoded = YAML::convert<Color>::encode(original);
    Color decoded;
    bool ok = YAML::convert<Color>::decode(encoded, decoded);
    REQUIRE(ok);
    REQUIRE(decoded.r == Approx(original.r).margin(TOL));
    REQUIRE(decoded.g == Approx(original.g).margin(TOL));
    REQUIRE(decoded.b == Approx(original.b).margin(TOL));
    REQUIRE(decoded.a == Approx(original.a).margin(TOL));
}

// ===========================================================================
// to_linear / to_srgb (AC-009)
// ===========================================================================
TEST_CASE("Color to_linear / to_srgb roundtrip", "[math][color]") {
    Color srgb{0.5f, 0.3f, 0.8f, 0.7f};
    Color linear = srgb.to_linear();
    Color back = linear.to_srgb();

    REQUIRE(back.r == Approx(srgb.r).margin(TOL));
    REQUIRE(back.g == Approx(srgb.g).margin(TOL));
    REQUIRE(back.b == Approx(srgb.b).margin(TOL));
    // Alpha unchanged
    REQUIRE(back.a == Approx(srgb.a).margin(TOL));
}

TEST_CASE("Color to_linear/to_srgb alpha unchanged", "[math][color]") {
    Color c{0.2f, 0.4f, 0.6f, 0.0f};
    Color lin = c.to_linear();
    REQUIRE(lin.a == Approx(0.0f).margin(TOL));
    Color srgb = c.to_srgb();
    REQUIRE(srgb.a == Approx(0.0f).margin(TOL));
}

// ===========================================================================
// Blend (AC-010)
// ===========================================================================
TEST_CASE("Color blend produces correct over-blend", "[math][color]") {
    Color fg{1.0f, 0.0f, 0.0f, 0.5f};
    Color bg{0.0f, 1.0f, 0.0f, 1.0f};
    Color result = Color::blend(fg, bg);

    REQUIRE(result.r == Approx(0.5f).margin(TOL));
    REQUIRE(result.g == Approx(0.5f).margin(TOL));
    REQUIRE(result.b == Approx(0.0f).margin(TOL));
    REQUIRE(result.a == Approx(1.0f).margin(TOL));
}

// ===========================================================================
// Blend with zero alpha (edge case)
// ===========================================================================
TEST_CASE("Color blend with zero alpha", "[math][color]") {
    Color fg{1.0f, 0.0f, 0.0f, 0.0f};
    Color bg{0.0f, 1.0f, 0.0f, 1.0f};
    Color result = Color::blend(fg, bg);
    // fg fully transparent so result = bg
    REQUIRE(result.r == Approx(bg.r).margin(TOL));
    REQUIRE(result.g == Approx(bg.g).margin(TOL));
    REQUIRE(result.b == Approx(bg.b).margin(TOL));
    REQUIRE(result.a == Approx(bg.a).margin(TOL));
}

TEST_CASE("Color blend both zero alpha", "[math][color]") {
    Color fg{1.0f, 0.0f, 0.0f, 0.0f};
    Color bg{0.0f, 1.0f, 0.0f, 0.0f};
    Color result = Color::blend(fg, bg);
    REQUIRE(result.r == Approx(0.0f).margin(TOL));
    REQUIRE(result.g == Approx(0.0f).margin(TOL));
    REQUIRE(result.b == Approx(0.0f).margin(TOL));
    REQUIRE(result.a == Approx(0.0f).margin(TOL));
}

// ===========================================================================
// Luminance (Story 5)
// ===========================================================================
TEST_CASE("Color luminance matches Rec. 709 coefficients", "[math][color]") {
    Color c{0.5f, 0.3f, 0.8f, 1.0f};
    float expected = 0.2126f * 0.5f + 0.7152f * 0.3f + 0.0722f * 0.8f;
    REQUIRE(c.luminance() == Approx(expected).margin(TOL));
}

// ===========================================================================
// Darken / Lighten (Story 5)
// ===========================================================================
TEST_CASE("Color darkened matches Godot-style multiply", "[math][color]") {
    Color c{0.5f, 0.3f, 0.8f, 1.0f};
    Color d = c.darkened(0.5f);
    REQUIRE(d.r == Approx(0.5f * 0.5f).margin(TOL));
    REQUIRE(d.g == Approx(0.3f * 0.5f).margin(TOL));
    REQUIRE(d.b == Approx(0.8f * 0.5f).margin(TOL));
    REQUIRE(d.a == Approx(1.0f).margin(TOL));
}

TEST_CASE("Color lightened matches Godot-style formula", "[math][color]") {
    Color c{0.5f, 0.3f, 0.8f, 1.0f};
    Color l = c.lightened(0.5f);
    REQUIRE(l.r == Approx(0.5f + (1.0f - 0.5f) * 0.5f).margin(TOL));
    REQUIRE(l.g == Approx(0.3f + (1.0f - 0.3f) * 0.5f).margin(TOL));
    REQUIRE(l.b == Approx(0.8f + (1.0f - 0.8f) * 0.5f).margin(TOL));
    REQUIRE(l.a == Approx(1.0f).margin(TOL));
}

// ===========================================================================
// Darken/Lighten edge cases
// ===========================================================================
TEST_CASE("Color darkened(0) and lightened(0) are identity", "[math][color]") {
    Color c{0.5f, 0.3f, 0.8f, 1.0f};
    REQUIRE(c.darkened(0.0f) == c);
    REQUIRE(c.lightened(0.0f) == c);
}

TEST_CASE("Color darkened(1) is black, lightened(1) is white", "[math][color]") {
    Color c{0.5f, 0.3f, 0.8f, 1.0f};
    Color d = c.darkened(1.0f);
    REQUIRE(d.r == Approx(0.0f).margin(TOL));
    REQUIRE(d.g == Approx(0.0f).margin(TOL));
    REQUIRE(d.b == Approx(0.0f).margin(TOL));

    Color l = c.lightened(1.0f);
    REQUIRE(l.r == Approx(1.0f).margin(TOL));
    REQUIRE(l.g == Approx(1.0f).margin(TOL));
    REQUIRE(l.b == Approx(1.0f).margin(TOL));
}

// ===========================================================================
// HSV roundtrip (Story 7)
// ===========================================================================
TEST_CASE("Color HSV roundtrip approximates identity", "[math][color]") {
    // Test several colors covering different hue sectors
    std::vector<Color> colors = {
        Color{1.0f, 0.0f, 0.0f, 1.0f},  // red
        Color{0.0f, 1.0f, 0.0f, 1.0f},  // green
        Color{0.0f, 0.0f, 1.0f, 1.0f},  // blue
        Color{1.0f, 1.0f, 0.0f, 1.0f},  // yellow
        Color{0.0f, 1.0f, 1.0f, 1.0f},  // cyan
        Color{1.0f, 0.0f, 1.0f, 1.0f},  // magenta
        Color{0.5f, 0.3f, 0.8f, 1.0f},
        Color{0.2f, 0.6f, 0.9f, 0.5f},
        Color{0.0f, 0.0f, 0.0f, 1.0f},  // black
        Color{1.0f, 1.0f, 1.0f, 1.0f},  // white
        Color{0.5f, 0.5f, 0.5f, 1.0f},  // gray
    };

    for (const auto& c : colors) {
        Vec3 hsv = c.to_hsv();
        Color reconstructed = Color::from_hsv(hsv.x, hsv.y, hsv.z);
        REQUIRE(reconstructed.r == Approx(c.r).margin(TOL));
        REQUIRE(reconstructed.g == Approx(c.g).margin(TOL));
        REQUIRE(reconstructed.b == Approx(c.b).margin(TOL));
    }
}

// ===========================================================================
// Named colors (AC-014)
// ===========================================================================
TEST_CASE("Color named presets have correct values", "[math][color]") {
    SECTION("white") {
        auto c = Color::white();
        REQUIRE(c == Color{1.0f, 1.0f, 1.0f});
    }
    SECTION("black") {
        auto c = Color::black();
        REQUIRE(c == Color{0.0f, 0.0f, 0.0f});
    }
    SECTION("red") {
        auto c = Color::red();
        REQUIRE(c == Color{1.0f, 0.0f, 0.0f});
    }
    SECTION("green") {
        auto c = Color::green();
        REQUIRE(c == Color{0.0f, 1.0f, 0.0f});
    }
    SECTION("blue") {
        auto c = Color::blue();
        REQUIRE(c == Color{0.0f, 0.0f, 1.0f});
    }
    SECTION("yellow") {
        auto c = Color::yellow();
        REQUIRE(c == Color{1.0f, 1.0f, 0.0f});
    }
    SECTION("cyan") {
        auto c = Color::cyan();
        REQUIRE(c == Color{0.0f, 1.0f, 1.0f});
    }
    SECTION("magenta") {
        auto c = Color::magenta();
        REQUIRE(c == Color{1.0f, 0.0f, 1.0f});
    }
}

TEST_CASE("Color named presets have alpha=1.0", "[math][color]") {
    REQUIRE(Color::white().a == 1.0f);
    REQUIRE(Color::black().a == 1.0f);
    REQUIRE(Color::red().a == 1.0f);
    REQUIRE(Color::green().a == 1.0f);
    REQUIRE(Color::blue().a == 1.0f);
    REQUIRE(Color::yellow().a == 1.0f);
    REQUIRE(Color::cyan().a == 1.0f);
    REQUIRE(Color::magenta().a == 1.0f);
}

// ===========================================================================
// RGBA32 roundtrip (Story 9)
// ===========================================================================
TEST_CASE("Color RGBA32 conversion roundtrip", "[math][color]") {
    Color c{1.0f, 0.5f, 0.25f, 1.0f};
    auto rgba = c.to_rgba32();
    REQUIRE(rgba[0] == 255);
    REQUIRE(rgba[1] == 128);
    REQUIRE(rgba[2] == 64);
    REQUIRE(rgba[3] == 255);

    Color back = Color::from_rgba32(rgba);
    REQUIRE(back.r == Approx(1.0f).margin(TOL));
    REQUIRE(back.g == Approx(128.0f / 255.0f).margin(TOL));
    REQUIRE(back.b == Approx(64.0f / 255.0f).margin(TOL));
    REQUIRE(back.a == Approx(1.0f).margin(TOL));
}

// ===========================================================================
// RGBA32 clamps (edge case)
// ===========================================================================
TEST_CASE("Color to_rgba32 clamps HDR and negative values", "[math][color]") {
    Color hdr{2.0f, -0.5f, 0.5f, 1.5f};
    auto rgba = hdr.to_rgba32();
    REQUIRE(rgba[0] == 255);  // clamped
    REQUIRE(rgba[1] == 0);    // negative → 0
    REQUIRE(rgba[2] == 128);  // 0.5 * 255 = 127.5 -> 128
    REQUIRE(rgba[3] == 255);  // 1.0 clamped
}

// ===========================================================================
// to_vec3
// ===========================================================================
TEST_CASE("Color to_vec3 discards alpha", "[math][color]") {
    Color c{0.2f, 0.4f, 0.6f, 0.8f};
    Vec3 v = c.to_vec3();
    REQUIRE(v.x == Approx(0.2f).margin(TOL));
    REQUIRE(v.y == Approx(0.4f).margin(TOL));
    REQUIRE(v.z == Approx(0.6f).margin(TOL));
}

TEST_CASE("Color to_vec4 preserves alpha", "[math][color]") {
    Color c{0.2f, 0.4f, 0.6f, 0.8f};
    Vec4 v = c.to_vec4();
    REQUIRE(v.x == Approx(0.2f).margin(TOL));
    REQUIRE(v.y == Approx(0.4f).margin(TOL));
    REQUIRE(v.z == Approx(0.6f).margin(TOL));
    REQUIRE(v.w == Approx(0.8f).margin(TOL));
}

TEST_CASE("Color to_vec4 defaults alpha to 1.0 for RGB constructor", "[math][color]") {
    Color c{0.1f, 0.3f, 0.5f};
    Vec4 v = c.to_vec4();
    REQUIRE(v.x == Approx(0.1f).margin(TOL));
    REQUIRE(v.y == Approx(0.3f).margin(TOL));
    REQUIRE(v.z == Approx(0.5f).margin(TOL));
    REQUIRE(v.w == Approx(1.0f).margin(TOL));
}

// ===========================================================================
// zero / one constants
// ===========================================================================
TEST_CASE("Color zero and one constants", "[math][color]") {
    REQUIRE(Color::zero() == Color{0.0f, 0.0f, 0.0f, 0.0f});
    REQUIRE(Color::one() == Color{1.0f, 1.0f, 1.0f, 1.0f});
}

// ===========================================================================
// Division by zero (edge case — produces inf/nan, no guard)
// ===========================================================================
TEST_CASE("Color division by zero produces inf/nan", "[math][color]") {
    Color a{1.0f, 1.0f, 1.0f, 1.0f};
    Color zero{0.0f, 0.0f, 0.0f, 0.0f};
    Color result = a / zero;
    REQUIRE(std::isinf(result.r));
    REQUIRE(std::isinf(result.g));
    REQUIRE(std::isinf(result.b));
    REQUIRE(std::isinf(result.a));
}

// ===========================================================================
// HDR values preserved through arithmetic
// ===========================================================================
TEST_CASE("Color arithmetic preserves HDR values", "[math][color]") {
    Color a{2.0f, 3.0f, 4.0f, 5.0f};
    Color b{0.5f, 0.5f, 0.5f, 0.5f};
    Color result = a * b;
    REQUIRE(result.r == Approx(1.0f).margin(TOL));
    REQUIRE(result.g == Approx(1.5f).margin(TOL));
    REQUIRE(result.b == Approx(2.0f).margin(TOL));
    REQUIRE(result.a == Approx(2.5f).margin(TOL));
}
