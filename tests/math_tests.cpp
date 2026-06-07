#include "math/math.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cstring>
#include <cmath>
#include <type_traits>

using namespace buddd::engine::math;
using Catch::Approx;

// ---------------------------------------------------------------------------
// Helper: tolerance constant and comparison utilities
// ---------------------------------------------------------------------------
namespace {
    constexpr float TOL = 1e-5f;

    void require_approx(const Vec2& a, const glm::vec2& b) {
        REQUIRE(a.x == Approx(b.x).margin(TOL));
        REQUIRE(a.y == Approx(b.y).margin(TOL));
    }

    void require_approx(const Vec3& a, const glm::vec3& b) {
        REQUIRE(a.x == Approx(b.x).margin(TOL));
        REQUIRE(a.y == Approx(b.y).margin(TOL));
        REQUIRE(a.z == Approx(b.z).margin(TOL));
    }

    void require_approx(const Vec4& a, const glm::vec4& b) {
        REQUIRE(a.x == Approx(b.x).margin(TOL));
        REQUIRE(a.y == Approx(b.y).margin(TOL));
        REQUIRE(a.z == Approx(b.z).margin(TOL));
        REQUIRE(a.w == Approx(b.w).margin(TOL));
    }

    void require_approx(const Mat4& a, const glm::mat4& b) {
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                REQUIRE(a[c][r] == Approx(b[c][r]).margin(TOL));
            }
        }
    }

    void require_approx(const Quat& a, const glm::quat& b) {
        REQUIRE(a.w == Approx(b.w).margin(TOL));
        REQUIRE(a.x == Approx(b.x).margin(TOL));
        REQUIRE(a.y == Approx(b.y).margin(TOL));
        REQUIRE(a.z == Approx(b.z).margin(TOL));
    }
} // anonymous namespace

// ===========================================================================
// Vec2 tests (T-01 to T-09)
// ===========================================================================

TEST_CASE("Vec2 default constructor creates zero vector", "[math][vec2]") {
    const Vec2 v;
    REQUIRE(v.x == 0.0f);
    REQUIRE(v.y == 0.0f);
}

TEST_CASE("Vec2(x,y) constructor", "[math][vec2]") {
    const Vec2 v(3.0f, 4.0f);
    REQUIRE(v.x == 3.0f);
    REQUIRE(v.y == 4.0f);
}

TEST_CASE("Vec2 arithmetic operators match GLM", "[math][vec2]") {
    const Vec2 a{3.0f, 4.0f};
    const Vec2 b{1.0f, 2.0f};
    const float s = 2.0f;

    // Component-wise
    require_approx(a + b, glm::vec2(3, 4) + glm::vec2(1, 2));
    require_approx(a - b, glm::vec2(3, 4) - glm::vec2(1, 2));
    require_approx(a * b, glm::vec2(3, 4) * glm::vec2(1, 2));
    require_approx(a / b, glm::vec2(3, 4) / glm::vec2(1, 2));

    // Scalar
    require_approx(a * s, glm::vec2(3, 4) * s);
    require_approx(s * a, s * glm::vec2(3, 4));
    require_approx(a / s, glm::vec2(3, 4) / s);

    // Unary minus
    require_approx(-a, -glm::vec2(3, 4));

    // Compound assignment
    Vec2 tmp;
    tmp = a; tmp += b; require_approx(tmp, glm::vec2(3, 4) + glm::vec2(1, 2));
    tmp = a; tmp -= b; require_approx(tmp, glm::vec2(3, 4) - glm::vec2(1, 2));
    tmp = a; tmp *= b; require_approx(tmp, glm::vec2(3, 4) * glm::vec2(1, 2));
    tmp = a; tmp /= b; require_approx(tmp, glm::vec2(3, 4) / glm::vec2(1, 2));
    tmp = a; tmp *= s; require_approx(tmp, glm::vec2(3, 4) * s);
    tmp = a; tmp /= s; require_approx(tmp, glm::vec2(3, 4) / s);
}

TEST_CASE("Vec2 length/length_squared matches GLM", "[math][vec2]") {
    const Vec2 v{3.0f, 4.0f};
    REQUIRE(v.length() == Approx(5.0f).margin(TOL));
    REQUIRE(v.length_squared() == Approx(25.0f).margin(TOL));
    // Also match GLM directly
    REQUIRE(v.length() == Approx(glm::length(glm::vec2(3, 4))).margin(TOL));
    REQUIRE(v.length_squared() == Approx(25.0f).margin(TOL)); // 3^2 + 4^2
}

TEST_CASE("Vec2 normalize/normalized matches GLM", "[math][vec2]") {
    const Vec2 v{3.0f, 4.0f};

    // normalized() returns copy, does not mutate
    const Vec2 n = v.normalized();
    REQUIRE(n.x == Approx(0.6f).margin(TOL));
    REQUIRE(n.y == Approx(0.8f).margin(TOL));
    // Matches GLM
    require_approx(n, glm::normalize(glm::vec2(3, 4)));

    // normalize() mutates in place
    Vec2 m{3.0f, 4.0f};
    Vec2& ref = m.normalize();
    REQUIRE(&ref == &m); // returns *this
    require_approx(m, glm::normalize(glm::vec2(3, 4)));
}

TEST_CASE("Vec2 dot matches GLM", "[math][vec2]") {
    const Vec2 a{1.0f, 2.0f};
    const Vec2 b{3.0f, 4.0f};
    const float d = a.dot(b);
    const float g = glm::dot(glm::vec2(1, 2), glm::vec2(3, 4));
    REQUIRE(d == Approx(g).margin(TOL));
    REQUIRE(d == Approx(11.0f).margin(TOL)); // 1*3 + 2*4 = 11
}

TEST_CASE("Vec2 constants", "[math][vec2]") {
    REQUIRE(Vec2::zero()   == Vec2(0.0f, 0.0f));
    REQUIRE(Vec2::one()    == Vec2(1.0f, 1.0f));
    REQUIRE(Vec2::unit_x() == Vec2(1.0f, 0.0f));
    REQUIRE(Vec2::unit_y() == Vec2(0.0f, 1.0f));
}

TEST_CASE("Vec2 comparison operators", "[math][vec2]") {
    const Vec2 a{1.0f, 2.0f};
    const Vec2 b{1.0f, 2.0f};
    const Vec2 c{3.0f, 4.0f};

    REQUIRE(a == b);
    REQUIRE_FALSE(a == c);
    REQUIRE(a != c);
    REQUIRE_FALSE(a != b);
}

TEST_CASE("Vec2 operator[] access", "[math][vec2]") {
    Vec2 v{3.0f, 4.0f};
    REQUIRE(v[0] == v.x);
    REQUIRE(v[1] == v.y);

    // Non-const returns mutable reference
    v[0] = 10.0f;
    v[1] = 20.0f;
    REQUIRE(v.x == 10.0f);
    REQUIRE(v.y == 20.0f);

    // Const access
    const Vec2 cv{5.0f, 6.0f};
    REQUIRE(cv[0] == 5.0f);
    REQUIRE(cv[1] == 6.0f);
}

// ===========================================================================
// Vec3 tests (T-10 to T-16)
// ===========================================================================

TEST_CASE("Vec3 constructors", "[math][vec3]") {
    const Vec3 v0;
    REQUIRE(v0.x == 0.0f);
    REQUIRE(v0.y == 0.0f);
    REQUIRE(v0.z == 0.0f);

    const Vec3 v1(1.0f, 2.0f, 3.0f);
    REQUIRE(v1.x == 1.0f);
    REQUIRE(v1.y == 2.0f);
    REQUIRE(v1.z == 3.0f);
}

TEST_CASE("Vec3 arithmetic matches GLM", "[math][vec3]") {
    const Vec3 a{3.0f, 4.0f, 5.0f};
    const Vec3 b{1.0f, 2.0f, 3.0f};
    const float s = 2.0f;

    // Component-wise
    require_approx(a + b, glm::vec3(3, 4, 5) + glm::vec3(1, 2, 3));
    require_approx(a - b, glm::vec3(3, 4, 5) - glm::vec3(1, 2, 3));
    require_approx(a * b, glm::vec3(3, 4, 5) * glm::vec3(1, 2, 3));
    require_approx(a / b, glm::vec3(3, 4, 5) / glm::vec3(1, 2, 3));

    // Scalar
    require_approx(a * s, glm::vec3(3, 4, 5) * s);
    require_approx(s * a, s * glm::vec3(3, 4, 5));
    require_approx(a / s, glm::vec3(3, 4, 5) / s);

    // Unary minus
    require_approx(-a, -glm::vec3(3, 4, 5));

    // Compound assignment
    Vec3 tmp;
    tmp = a; tmp += b; require_approx(tmp, glm::vec3(3, 4, 5) + glm::vec3(1, 2, 3));
    tmp = a; tmp -= b; require_approx(tmp, glm::vec3(3, 4, 5) - glm::vec3(1, 2, 3));
    tmp = a; tmp *= b; require_approx(tmp, glm::vec3(3, 4, 5) * glm::vec3(1, 2, 3));
    tmp = a; tmp /= b; require_approx(tmp, glm::vec3(3, 4, 5) / glm::vec3(1, 2, 3));
    tmp = a; tmp *= s; require_approx(tmp, glm::vec3(3, 4, 5) * s);
    tmp = a; tmp /= s; require_approx(tmp, glm::vec3(3, 4, 5) / s);
}

TEST_CASE("Vec3 cross matches GLM", "[math][vec3]") {
    const Vec3 a{1.0f, 0.0f, 0.0f};
    const Vec3 b{0.0f, 1.0f, 0.0f};
    const Vec3 c = a.cross(b);
    REQUIRE(c.x == Approx(0.0f).margin(TOL));
    REQUIRE(c.y == Approx(0.0f).margin(TOL));
    REQUIRE(c.z == Approx(1.0f).margin(TOL));
    require_approx(c, glm::cross(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0)));
}

TEST_CASE("Vec3 lerp matches GLM", "[math][vec3]") {
    const Vec3 a{0.0f, 0.0f, 0.0f};
    const Vec3 b{10.0f, 10.0f, 10.0f};

    // t = 0 returns a
    const Vec3 r0 = a.lerp(b, 0.0f);
    require_approx(r0, glm::vec3(0, 0, 0));

    // t = 1 returns b
    const Vec3 r1 = a.lerp(b, 1.0f);
    require_approx(r1, glm::vec3(10, 10, 10));

    // t = 0.5 is halfway
    const Vec3 r5 = a.lerp(b, 0.5f);
    require_approx(r5, glm::vec3(5, 5, 5));

    // All match GLM
    require_approx(r5, glm::mix(glm::vec3(0, 0, 0), glm::vec3(10, 10, 10), 0.5f));
}

TEST_CASE("Vec3 constants", "[math][vec3]") {
    REQUIRE(Vec3::zero()   == Vec3(0.0f, 0.0f, 0.0f));
    REQUIRE(Vec3::one()    == Vec3(1.0f, 1.0f, 1.0f));
    REQUIRE(Vec3::unit_x() == Vec3(1.0f, 0.0f, 0.0f));
    REQUIRE(Vec3::unit_y() == Vec3(0.0f, 1.0f, 0.0f));
    REQUIRE(Vec3::unit_z() == Vec3(0.0f, 0.0f, 1.0f));
}

TEST_CASE("Vec3 normalize on zero length returns NaN", "[math][vec3]") {
    const Vec3 n = Vec3{}.normalized();
    REQUIRE(std::isnan(n.x));
    REQUIRE(std::isnan(n.y));
    REQUIRE(std::isnan(n.z));
}

TEST_CASE("Vec3 dot/cross/length matches GLM", "[math][vec3]") {
    const Vec3 a{1.0f, 2.0f, 3.0f};
    const Vec3 b{4.0f, 5.0f, 6.0f};

    // dot
    REQUIRE(a.dot(b) == Approx(glm::dot(glm::vec3(1,2,3), glm::vec3(4,5,6))).margin(TOL));
    REQUIRE(a.dot(b) == Approx(32.0f).margin(TOL)); // 1*4 + 2*5 + 3*6 = 32

    // cross
    require_approx(a.cross(b), glm::cross(glm::vec3(1,2,3), glm::vec3(4,5,6)));

    // length
    REQUIRE(a.length() == Approx(glm::length(glm::vec3(1,2,3))).margin(TOL));
    REQUIRE(a.length() == Approx(std::sqrt(14.0f)).margin(TOL));
    REQUIRE(a.length_squared() == Approx(14.0f).margin(TOL));
}

// ===========================================================================
// Vec4 tests (T-17 to T-20)
// ===========================================================================

TEST_CASE("Vec4 constructors", "[math][vec4]") {
    const Vec4 v0;
    REQUIRE(v0.x == 0.0f);
    REQUIRE(v0.y == 0.0f);
    REQUIRE(v0.z == 0.0f);
    REQUIRE(v0.w == 0.0f);

    const Vec4 v1(1.0f, 2.0f, 3.0f, 4.0f);
    REQUIRE(v1.x == 1.0f);
    REQUIRE(v1.y == 2.0f);
    REQUIRE(v1.z == 3.0f);
    REQUIRE(v1.w == 4.0f);
}

TEST_CASE("Vec4 arithmetic matches GLM", "[math][vec4]") {
    const Vec4 a{3.0f, 4.0f, 5.0f, 6.0f};
    const Vec4 b{1.0f, 2.0f, 3.0f, 4.0f};
    const float s = 2.0f;

    // Component-wise
    require_approx(a + b, glm::vec4(3, 4, 5, 6) + glm::vec4(1, 2, 3, 4));
    require_approx(a - b, glm::vec4(3, 4, 5, 6) - glm::vec4(1, 2, 3, 4));
    require_approx(a * b, glm::vec4(3, 4, 5, 6) * glm::vec4(1, 2, 3, 4));
    require_approx(a / b, glm::vec4(3, 4, 5, 6) / glm::vec4(1, 2, 3, 4));

    // Scalar
    require_approx(a * s, glm::vec4(3, 4, 5, 6) * s);
    require_approx(s * a, s * glm::vec4(3, 4, 5, 6));
    require_approx(a / s, glm::vec4(3, 4, 5, 6) / s);

    // Unary minus
    require_approx(-a, -glm::vec4(3, 4, 5, 6));

    // Compound assignment
    Vec4 tmp;
    tmp = a; tmp += b; require_approx(tmp, glm::vec4(3, 4, 5, 6) + glm::vec4(1, 2, 3, 4));
    tmp = a; tmp -= b; require_approx(tmp, glm::vec4(3, 4, 5, 6) - glm::vec4(1, 2, 3, 4));
    tmp = a; tmp *= b; require_approx(tmp, glm::vec4(3, 4, 5, 6) * glm::vec4(1, 2, 3, 4));
    tmp = a; tmp /= b; require_approx(tmp, glm::vec4(3, 4, 5, 6) / glm::vec4(1, 2, 3, 4));
    tmp = a; tmp *= s; require_approx(tmp, glm::vec4(3, 4, 5, 6) * s);
    tmp = a; tmp /= s; require_approx(tmp, glm::vec4(3, 4, 5, 6) / s);
}

TEST_CASE("Vec4 unit_w", "[math][vec4]") {
    REQUIRE(Vec4::unit_w() == Vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

TEST_CASE("Vec4 dot/length matches GLM", "[math][vec4]") {
    const Vec4 v{1.0f, 2.0f, 3.0f, 4.0f};

    // dot
    REQUIRE(v.dot(v) == Approx(glm::dot(glm::vec4(1,2,3,4), glm::vec4(1,2,3,4))).margin(TOL));
    REQUIRE(v.dot(v) == Approx(30.0f).margin(TOL)); // 1+4+9+16

    // length
    REQUIRE(v.length() == Approx(glm::length(glm::vec4(1,2,3,4))).margin(TOL));
    REQUIRE(v.length_squared() == Approx(30.0f).margin(TOL)); // 1^2 + 2^2 + 3^2 + 4^2
    REQUIRE(v.length_squared() == Approx(30.0f).margin(TOL));
}

// ===========================================================================
// Mat4 tests (T-21 to T-37)
// ===========================================================================

TEST_CASE("Mat4 default constructor is identity", "[math][mat4]") {
    const Mat4 m;
    require_approx(m, glm::mat4(1.0f));
}

TEST_CASE("Mat4(diagonal) constructor", "[math][mat4]") {
    const Mat4 m(5.0f);
    require_approx(m, glm::mat4(5.0f));
}

TEST_CASE("Mat4 arithmetic matches GLM", "[math][mat4]") {
    const Mat4 a{};
    const Mat4 b = Mat4::translate(Vec3{1.0f, 2.0f, 3.0f});

    require_approx(a + b, a.glm() + b.glm());
    require_approx(a - b, a.glm() - b.glm());
    require_approx(a * b, a.glm() * b.glm());

    Mat4 tmp;
    tmp = a; tmp += b; require_approx(tmp, a.glm() + b.glm());
    tmp = a; tmp -= b; require_approx(tmp, a.glm() - b.glm());
    tmp = a; tmp *= b; require_approx(tmp, a.glm() * b.glm());

    require_approx(a * 2.0f, a.glm() * 2.0f);
}

TEST_CASE("Mat4 * Vec4 matches GLM", "[math][mat4]") {
    const Mat4 m = Mat4::translate(Vec3{1.0f, 2.0f, 3.0f});
    const Vec4 v{1.0f, 2.0f, 3.0f, 1.0f};
    const Vec4 r = m * v;
    require_approx(r, m.glm() * glm::vec4(1, 2, 3, 1));
}

TEST_CASE("Mat4 * Vec3 matches GLM", "[math][mat4]") {
    const Mat4 m = Mat4::translate(Vec3{1.0f, 2.0f, 3.0f});
    const Vec3 v{1.0f, 2.0f, 3.0f};
    const Vec3 r = m * v;

    // Identity: Mat4{} * Vec3(1,2,3) == Vec3(1,2,3)
    const Mat4 id;
    const Vec3 r_id = id * v;
    REQUIRE(r_id.x == Approx(1.0f).margin(TOL));
    REQUIRE(r_id.y == Approx(2.0f).margin(TOL));
    REQUIRE(r_id.z == Approx(3.0f).margin(TOL));

    // Compare with GLM's homogeneous promotion
    const glm::vec3 g = glm::vec3(glm::mat4(1.0f) * glm::vec4(1, 2, 3, 1));
    require_approx(r_id, g);

    // Non-identity case
    require_approx(r, glm::vec3(glm::translate(glm::mat4(1.0f), glm::vec3(1,2,3)) * glm::vec4(1,2,3,1)));
}

TEST_CASE("Vec3 * Mat4 matches GLM", "[math][mat4]") {
    const Mat4 m = Mat4::translate(Vec3{1.0f, 2.0f, 3.0f});
    const Vec3 v{1.0f, 2.0f, 3.0f};
    const Vec3 r = v * m;

    // Compare with GLM row-vector promotion
    const glm::vec3 g = glm::vec3(glm::vec4(1, 2, 3, 1) * glm::translate(glm::mat4(1.0f), glm::vec3(1,2,3)));
    require_approx(r, g);
}

TEST_CASE("Mat4 transpose matches GLM", "[math][mat4]") {
    const Mat4 m = Mat4::translate(Vec3{1.0f, 2.0f, 3.0f});
    require_approx(m.transpose(), glm::transpose(m.glm()));
}

TEST_CASE("Mat4 determinant matches GLM", "[math][mat4]") {
    const Mat4 m = Mat4::translate(Vec3{1.0f, 2.0f, 3.0f});
    REQUIRE(m.determinant() == Approx(glm::determinant(m.glm())).margin(TOL));
    REQUIRE(m.determinant() == Approx(1.0f).margin(TOL)); // translation has det=1
}

TEST_CASE("Mat4 inverse matches GLM", "[math][mat4]") {
    const Mat4 m = Mat4::translate(Vec3{1.0f, 2.0f, 3.0f});
    require_approx(m.inverse(), glm::inverse(m.glm()));

    // Verify inverse * original ≈ identity
    const Mat4 inv = m.inverse();
    require_approx(inv * m, glm::mat4(1.0f));
}

TEST_CASE("Mat4 perspective matches GLM", "[math][mat4]") {
    const float fov_y = 1.0471975512f; // ~60 degrees
    const float aspect = 16.0f / 9.0f;
    const float near_p = 0.1f;
    const float far_p = 100.0f;

    const Mat4 m = Mat4::perspective(fov_y, aspect, near_p, far_p);
    const glm::mat4 g = glm::perspective(fov_y, aspect, near_p, far_p);
    require_approx(m, g);
}

TEST_CASE("Mat4 ortho matches GLM", "[math][mat4]") {
    const Mat4 m = Mat4::ortho(-10.0f, 10.0f, -5.0f, 5.0f, 0.1f, 100.0f);
    const glm::mat4 g = glm::ortho(-10.0f, 10.0f, -5.0f, 5.0f, 0.1f, 100.0f);
    require_approx(m, g);
}

TEST_CASE("Mat4 look_at matches GLM", "[math][mat4]") {
    const Vec3 eye{0.0f, 2.0f, 5.0f};
    const Vec3 center{0.0f, 0.0f, 0.0f};
    const Vec3 up{0.0f, 1.0f, 0.0f};

    const Mat4 m = Mat4::look_at(eye, center, up);
    const glm::mat4 g = glm::lookAt(glm::vec3(0,2,5), glm::vec3(0,0,0), glm::vec3(0,1,0));
    require_approx(m, g);
}

TEST_CASE("Mat4 translate matches GLM", "[math][mat4]") {
    const Mat4 m = Mat4::translate(Vec3{1.0f, 2.0f, 3.0f});
    const glm::mat4 g = glm::translate(glm::mat4(1.0f), glm::vec3(1, 2, 3));
    require_approx(m, g);
}

TEST_CASE("Mat4 rotate matches GLM", "[math][mat4]") {
    const float angle = pi / 2.0f;
    const Vec3 axis = Vec3::unit_y();

    const Mat4 m = Mat4::rotate(angle, axis);
    const glm::mat4 g = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0));
    require_approx(m, g);
}

TEST_CASE("Mat4 scale matches GLM", "[math][mat4]") {
    const Mat4 m = Mat4::scale(Vec3{2.0f, 3.0f, 4.0f});
    const glm::mat4 g = glm::scale(glm::mat4(1.0f), glm::vec3(2, 3, 4));
    require_approx(m, g);
}

TEST_CASE("Mat4 column-major layout", "[math][mat4]") {
    Mat4 m;
    // Fill each column with known values
    m[0] = Vec4{ 1.0f,  2.0f,  3.0f,  4.0f};
    m[1] = Vec4{ 5.0f,  6.0f,  7.0f,  8.0f};
    m[2] = Vec4{ 9.0f, 10.0f, 11.0f, 12.0f};
    m[3] = Vec4{13.0f, 14.0f, 15.0f, 16.0f};

    // Verify column-major access: m[col][row]
    REQUIRE(m[0][0] == 1.0f);  REQUIRE(m[0][1] == 2.0f);
    REQUIRE(m[0][2] == 3.0f);  REQUIRE(m[0][3] == 4.0f);

    REQUIRE(m[1][0] == 5.0f);  REQUIRE(m[1][1] == 6.0f);
    REQUIRE(m[1][2] == 7.0f);  REQUIRE(m[1][3] == 8.0f);

    REQUIRE(m[2][0] == 9.0f);  REQUIRE(m[2][1] == 10.0f);
    REQUIRE(m[2][2] == 11.0f); REQUIRE(m[2][3] == 12.0f);

    REQUIRE(m[3][0] == 13.0f); REQUIRE(m[3][1] == 14.0f);
    REQUIRE(m[3][2] == 15.0f); REQUIRE(m[3][3] == 16.0f);

    // Byte-for-byte compare with equivalent glm::mat4
    glm::mat4 glm_m;
    glm_m[0] = glm::vec4( 1,  2,  3,  4);
    glm_m[1] = glm::vec4( 5,  6,  7,  8);
    glm_m[2] = glm::vec4( 9, 10, 11, 12);
    glm_m[3] = glm::vec4(13, 14, 15, 16);
    REQUIRE(std::memcmp(&m, &glm_m, sizeof(Mat4)) == 0);
}

TEST_CASE("Mat4 operator[] returns Vec4& for non-const", "[math][mat4]") {
    Mat4 m;
    const Vec4 new_col{9.0f, 8.0f, 7.0f, 6.0f};
    m[1] = new_col; // modify second column
    REQUIRE(m[1][0] == 9.0f);
    REQUIRE(m[1][1] == 8.0f);
    REQUIRE(m[1][2] == 7.0f);
    REQUIRE(m[1][3] == 6.0f);

    // Other columns remain identity
    REQUIRE(m[0][0] == 1.0f);
    REQUIRE(m[2][2] == 1.0f);
    REQUIRE(m[3][3] == 1.0f);

    // Ensure modifying through operator[] mutates the matrix
    m[1][0] = 42.0f;
    REQUIRE(m[1][0] == 42.0f);
}

// ===========================================================================
// Quat tests (T-38 to T-48)
// ===========================================================================

TEST_CASE("Quat default constructor is identity", "[math][quat]") {
    const Quat q;
    REQUIRE(q.w == Approx(1.0f).margin(TOL));
    REQUIRE(q.x == Approx(0.0f).margin(TOL));
    REQUIRE(q.y == Approx(0.0f).margin(TOL));
    REQUIRE(q.z == Approx(0.0f).margin(TOL));
    require_approx(q, glm::quat(1, 0, 0, 0));
}

TEST_CASE("Quat component constructor", "[math][quat]") {
    const Quat q(0.5f, 0.1f, 0.2f, 0.3f);
    REQUIRE(q.w == 0.5f);
    REQUIRE(q.x == 0.1f);
    REQUIRE(q.y == 0.2f);
    REQUIRE(q.z == 0.3f);
}

TEST_CASE("Quat composition matches GLM", "[math][quat]") {
    const Quat a = Quat::angle_axis(pi / 3.0f, Vec3::unit_x());
    const Quat b = Quat::angle_axis(pi / 4.0f, Vec3::unit_y());
    const Quat c = a * b;
    require_approx(c, a.glm() * b.glm());
}

TEST_CASE("Quat rotate vector matches GLM", "[math][quat]") {
    const Quat q = Quat::angle_axis(pi / 2.0f, Vec3::unit_y());
    const Vec3 v{1.0f, 0.0f, 0.0f};
    const Vec3 r = q * v;

    // Rotating (1,0,0) by 90 deg around Y should give (0,0,-1)
    REQUIRE(r.x == Approx(0.0f).margin(TOL));
    REQUIRE(r.y == Approx(0.0f).margin(TOL));
    REQUIRE(r.z == Approx(-1.0f).margin(TOL));

    // Match GLM
    require_approx(r, q.glm() * glm::vec3(1, 0, 0));
}

TEST_CASE("Quat conjugate matches GLM", "[math][quat]") {
    const Quat q(1.0f, 2.0f, 3.0f, 4.0f);
    // Normalize first so it's a valid quaternion (conjugate doesn't require unit)
    const Quat n = q.normalized();
    require_approx(n.conjugate(), glm::conjugate(n.glm()));
}

TEST_CASE("Quat inverse matches GLM", "[math][quat]") {
    const Quat q = Quat::angle_axis(pi / 3.0f, Vec3(1.0f, 1.0f, 1.0f).normalized());
    require_approx(q.inverse(), glm::inverse(q.glm()));

    // q * q.inverse() ≈ identity
    const Quat inv = q.inverse();
    const Quat ident = q * inv;
    REQUIRE(ident.w == Approx(1.0f).margin(TOL));
    REQUIRE(ident.x == Approx(0.0f).margin(TOL));
    REQUIRE(ident.y == Approx(0.0f).margin(TOL));
    REQUIRE(ident.z == Approx(0.0f).margin(TOL));
}

TEST_CASE("Quat to_mat4 matches GLM", "[math][quat]") {
    const Quat q = Quat::angle_axis(pi / 4.0f, Vec3::unit_y());
    require_approx(q.to_mat4(), glm::mat4_cast(q.glm()));
}

TEST_CASE("Quat slerp matches GLM", "[math][quat]") {
    const Quat a = Quat::identity();
    const Quat b = Quat::angle_axis(pi / 2.0f, Vec3::unit_y());

    // t = 0 returns a
    require_approx(Quat::slerp(a, b, 0.0f), glm::slerp(a.glm(), b.glm(), 0.0f));
    require_approx(Quat::slerp(a, b, 0.0f), a.glm());
    // t = 1 returns b
    require_approx(Quat::slerp(a, b, 1.0f), b.glm());

    // t = 0.5 is halfway
    require_approx(Quat::slerp(a, b, 0.5f), glm::slerp(a.glm(), b.glm(), 0.5f));
}

TEST_CASE("Quat angle_axis matches GLM", "[math][quat]") {
    const float angle = pi / 2.0f;
    const Vec3 axis = Vec3::unit_y();
    const Quat q = Quat::angle_axis(angle, axis);
    const glm::quat g = glm::angleAxis(angle, glm::vec3(0, 1, 0));
    require_approx(q, g);
}

TEST_CASE("Quat from_euler matches GLM", "[math][quat]") {
    const Quat q = Quat::from_euler(0.0f, pi / 2.0f, 0.0f);
    const glm::quat g = glm::quat(glm::vec3(0, pi / 2.0f, 0));
    require_approx(q, g);
}

TEST_CASE("Quat normalize matches GLM", "[math][quat]") {
    const Quat q(2.0f, 0.0f, 0.0f, 0.0f); // non-unit
    require_approx(q.normalized(), glm::normalize(q.glm()));

    // In-place normalize
    Quat m{2.0f, 0.0f, 0.0f, 0.0f};
    m.normalize();
    require_approx(m, glm::normalize(glm::quat(2,0,0,0)));
}

// ===========================================================================
// Free function view_matrix / look_at_rotation tests (replaces old Camera tests)
// ===========================================================================

TEST_CASE("view_matrix matches GLM lookAt", "[math][camera]") {
    const Vec3 pos{0.0f, 2.0f, 5.0f};
    const Quat orient{Quat::identity()};
    // Identity orientation → forward = (0,0,-1), up = (0,1,0)
    // So lookAt center is position + forward = (0,2,4)

    const Mat4 view = view_matrix(pos, orient);
    const glm::mat4 gview = glm::lookAt(
        glm::vec3(0, 2, 5),
        glm::vec3(0, 2, 4),
        glm::vec3(0, 1, 0));
    require_approx(view, gview);
}

TEST_CASE("view_matrix non-identity orientation", "[math][camera]") {
    const Vec3 pos{0.0f, 0.0f, 5.0f};
    const Quat orient = Quat::angle_axis(pi / 4.0f, Vec3::unit_y());

    const Mat4 view = view_matrix(pos, orient);

    // Compute expected: lookAt from pos to pos + rot*(0,0,-1) with up = rot*(0,1,0)
    const Vec3 forward = orient * Vec3(0.0f, 0.0f, -1.0f);
    const Vec3 up = orient * Vec3(0.0f, 1.0f, 0.0f);
    const glm::mat4 gview = glm::lookAt(
        pos.glm(),
        (pos + forward).glm(),
        up.glm());
    require_approx(view, gview);
}

TEST_CASE("look_at_rotation basic", "[math][camera]") {
    const Vec3 eye{0.0f, 0.0f, 0.0f};
    const Vec3 center{5.0f, 0.0f, 0.0f};
    const Vec3 up{0.0f, 1.0f, 0.0f};

    const Quat rot = look_at_rotation(eye, center, up);
    const Vec3 forward = rot * Vec3(0.0f, 0.0f, -1.0f);

    // Forward should point toward (5,0,0), i.e., (1,0,0)
    REQUIRE(forward.x == Approx(1.0f).margin(TOL));
    REQUIRE(forward.y == Approx(0.0f).margin(TOL));
    REQUIRE(forward.z == Approx(0.0f).margin(TOL));
}

TEST_CASE("look_at_rotation from offset", "[math][camera]") {
    const Vec3 eye{3.0f, 2.0f, 3.0f};
    const Vec3 center{0.0f, 0.0f, 0.0f};
    const Vec3 up{0.0f, 1.0f, 0.0f};

    const Quat rot = look_at_rotation(eye, center, up);
    const Vec3 forward = rot * Vec3(0.0f, 0.0f, -1.0f);
    const Vec3 expected_dir = (center - eye).normalized();

    REQUIRE(forward.x == Approx(expected_dir.x).margin(1e-4f));
    REQUIRE(forward.y == Approx(expected_dir.y).margin(1e-4f));
    REQUIRE(forward.z == Approx(expected_dir.z).margin(1e-4f));
}

TEST_CASE("look_at_rotation identity", "[math][camera]") {
    // When eye + forward = center, rotation should be identity
    // forward = (0,0,-1), so eye (0,0,1) looking at center (0,0,0)
    const Vec3 eye{0.0f, 0.0f, 1.0f};
    const Vec3 center{0.0f, 0.0f, 0.0f};
    const Vec3 up{0.0f, 1.0f, 0.0f};

    const Quat rot = look_at_rotation(eye, center, up);
    const Quat identity;
    REQUIRE(rot.w == Approx(identity.w).margin(TOL));
    REQUIRE(rot.x == Approx(identity.x).margin(TOL));
    REQUIRE(rot.y == Approx(identity.y).margin(TOL));
    REQUIRE(rot.z == Approx(identity.z).margin(TOL));
}

// ===========================================================================
// Utility and integration tests (T-57 to T-61)
// ===========================================================================

TEST_CASE("radians and degrees conversions", "[math][utility]") {
    REQUIRE(radians(180.0f) == Approx(pi).margin(TOL));
    REQUIRE(degrees(pi) == Approx(180.0f).margin(TOL));
    REQUIRE(radians(0.0f) == Approx(0.0f).margin(TOL));
    REQUIRE(degrees(0.0f) == Approx(0.0f).margin(TOL));
    REQUIRE(radians(90.0f) == Approx(half_pi).margin(TOL));
    REQUIRE(degrees(half_pi) == Approx(90.0f).margin(TOL));
}

TEST_CASE("constants values", "[math][utility]") {
    REQUIRE(pi == Approx(3.14159265358979323846f).margin(TOL));
    REQUIRE(half_pi == Approx(1.57079632679489661923f).margin(TOL));
    REQUIRE(two_pi == Approx(6.28318530717958647693f).margin(TOL));
    REQUIRE(epsilon == 1e-6f);
}

TEST_CASE("math sin/cos/tan match GLM", "[math][utility]") {
    // At 0
    REQUIRE(sin(0.0f) == Approx(glm::sin(0.0f)).margin(TOL));
    REQUIRE(sin(0.0f) == Approx(0.0f).margin(TOL));
    REQUIRE(cos(0.0f) == Approx(glm::cos(0.0f)).margin(TOL));
    REQUIRE(cos(0.0f) == Approx(1.0f).margin(TOL));
    REQUIRE(tan(0.0f) == Approx(glm::tan(0.0f)).margin(TOL));
    REQUIRE(tan(0.0f) == Approx(0.0f).margin(TOL));

    // At pi/2
    REQUIRE(sin(half_pi) == Approx(glm::sin(half_pi)).margin(TOL));
    REQUIRE(sin(half_pi) == Approx(1.0f).margin(TOL));

    // At pi
    REQUIRE(cos(pi) == Approx(glm::cos(pi)).margin(TOL));
    REQUIRE(cos(pi) == Approx(-1.0f).margin(TOL));

    // At pi/4
    const float expected = std::sqrt(2.0f) / 2.0f;
    REQUIRE(sin(pi / 4.0f) == Approx(glm::sin(pi / 4.0f)).margin(TOL));
    REQUIRE(sin(pi / 4.0f) == Approx(expected).margin(TOL));
    REQUIRE(cos(pi / 4.0f) == Approx(glm::cos(pi / 4.0f)).margin(TOL));
    REQUIRE(cos(pi / 4.0f) == Approx(expected).margin(TOL));
}

TEST_CASE("math asin/acos/atan match GLM", "[math][utility]") {
    REQUIRE(asin(0.0f) == Approx(glm::asin(0.0f)).margin(TOL));
    REQUIRE(asin(0.0f) == Approx(0.0f).margin(TOL));
    REQUIRE(acos(1.0f) == Approx(glm::acos(1.0f)).margin(TOL));
    REQUIRE(acos(1.0f) == Approx(0.0f).margin(TOL));
    REQUIRE(atan(0.0f) == Approx(glm::atan(0.0f)).margin(TOL));
    REQUIRE(atan(0.0f) == Approx(0.0f).margin(TOL));

    REQUIRE(atan2(1.0f, 0.0f) == Approx(glm::atan(1.0f, 0.0f)).margin(TOL));
    REQUIRE(atan2(1.0f, 0.0f) == Approx(half_pi).margin(TOL));

    REQUIRE(asin(1.0f) == Approx(glm::asin(1.0f)).margin(TOL));
    REQUIRE(acos(0.0f) == Approx(glm::acos(0.0f)).margin(TOL));
}

TEST_CASE("math sqrt matches GLM", "[math][utility]") {
    REQUIRE(sqrt(4.0f) == Approx(glm::sqrt(4.0f)).margin(TOL));
    REQUIRE(sqrt(4.0f) == Approx(2.0f).margin(TOL));
    REQUIRE(sqrt(0.0f) == Approx(glm::sqrt(0.0f)).margin(TOL));
    REQUIRE(sqrt(0.0f) == Approx(0.0f).margin(TOL));
    REQUIRE(sqrt(2.0f) == Approx(glm::sqrt(2.0f)).margin(TOL));
}

// ===========================================================================
// Interop tests (T-62 to T-65)
// ===========================================================================

TEST_CASE("GLM interop via glm() accessor", "[math][interop]") {
    // Vec2
    Vec2 v2;
    static_assert(std::is_same_v<decltype(v2.glm()), glm::vec2&>);
    const Vec2 cv2;
    static_assert(std::is_same_v<decltype(cv2.glm()), const glm::vec2&>);
    glm::vec2& ref2 = v2.glm();
    ref2.x = 42.0f;
    REQUIRE(v2.x == 42.0f);

    // Vec3
    Vec3 v3;
    static_assert(std::is_same_v<decltype(v3.glm()), glm::vec3&>);
    const Vec3 cv3;
    static_assert(std::is_same_v<decltype(cv3.glm()), const glm::vec3&>);

    // Vec4
    Vec4 v4;
    static_assert(std::is_same_v<decltype(v4.glm()), glm::vec4&>);
    const Vec4 cv4;
    static_assert(std::is_same_v<decltype(cv4.glm()), const glm::vec4&>);

    // Mat4
    Mat4 m4;
    static_assert(std::is_same_v<decltype(m4.glm()), glm::mat4&>);
    const Mat4 cm4;
    static_assert(std::is_same_v<decltype(cm4.glm()), const glm::mat4&>);

    // Quat
    Quat q;
    static_assert(std::is_same_v<decltype(q.glm()), glm::quat&>);
    const Quat cq;
    static_assert(std::is_same_v<decltype(cq.glm()), const glm::quat&>);
}

TEST_CASE("Static assertions compile", "[math][compile]") {
    // These are compile-time checks that would fail compilation if missing.
    // They mirror the static_asserts in the implementation headers.
    static_assert(std::is_standard_layout_v<Vec2>, "Vec2 must be standard layout");
    static_assert(sizeof(Vec2) == sizeof(glm::vec2), "Vec2 size must match glm::vec2");
    static_assert(std::is_trivially_copyable_v<Vec2>, "Vec2 must be trivially copyable");

    static_assert(std::is_standard_layout_v<Vec3>, "Vec3 must be standard layout");
    static_assert(sizeof(Vec3) == sizeof(glm::vec3), "Vec3 size must match glm::vec3");
    static_assert(std::is_trivially_copyable_v<Vec3>, "Vec3 must be trivially copyable");

    static_assert(std::is_standard_layout_v<Vec4>, "Vec4 must be standard layout");
    static_assert(sizeof(Vec4) == sizeof(glm::vec4), "Vec4 size must match glm::vec4");
    static_assert(std::is_trivially_copyable_v<Vec4>, "Vec4 must be trivially copyable");

    static_assert(std::is_standard_layout_v<Mat4>, "Mat4 must be standard layout");
    static_assert(sizeof(Mat4) == sizeof(glm::mat4), "Mat4 size must match glm::mat4");
    static_assert(std::is_trivially_copyable_v<Mat4>, "Mat4 must be trivially copyable");

    static_assert(std::is_standard_layout_v<Quat>, "Quat must be standard layout");
    static_assert(sizeof(Quat) == sizeof(glm::quat), "Quat size must match glm::quat");
    static_assert(std::is_trivially_copyable_v<Quat>, "Quat must be trivially copyable");

    REQUIRE(true); // If we reach here, all static_asserts passed
}

TEST_CASE("Convenience header math.h includes all types", "[math][integration]") {
    // Verify all types are available through the math.h convenience header.
    // This test merely instantiates each type to confirm compilation.
    const Vec2 v2{1.0f, 2.0f};
    REQUIRE(v2.x == 1.0f);

    const Vec3 v3{1.0f, 2.0f, 3.0f};
    REQUIRE(v3.z == 3.0f);

    const Vec4 v4{1.0f, 2.0f, 3.0f, 4.0f};
    REQUIRE(v4.w == 4.0f);

    const Mat4 m4;
    REQUIRE(m4[0][0] == 1.0f);

    const Quat q;
    REQUIRE(q.w == 1.0f);

    // Camera class was removed in ADR-024; view_matrix free function is tested
    // in the math::view_matrix / look_at_rotation test cases above
    REQUIRE(true);
}

TEST_CASE("GLM types not in public API", "[math][interop]") {
    // Compile-time verification that the public API returns wrapper types,
    // not GLM types. The only exception is the .glm() accessor.
    // These static_asserts ensure we haven't accidentally exposed GLM types
    // in arithmetic operators or other methods.

    // Vec2 public API returns Vec2, not glm::vec2
    static_assert(std::is_same_v<decltype(Vec2{} + Vec2{}), Vec2>);
    static_assert(std::is_same_v<decltype(Vec2{} * 1.0f), Vec2>);
    static_assert(std::is_same_v<decltype(1.0f * Vec2{}), Vec2>);
    static_assert(std::is_same_v<decltype(-Vec2{}), Vec2>);
    static_assert(std::is_same_v<decltype(std::declval<Vec2>().normalized()), Vec2>);

    // Vec3 public API returns Vec3
    static_assert(std::is_same_v<decltype(Vec3{} + Vec3{}), Vec3>);
    static_assert(std::is_same_v<decltype(Vec3{} * 1.0f), Vec3>);
    static_assert(std::is_same_v<decltype(1.0f * Vec3{}), Vec3>);
    static_assert(std::is_same_v<decltype(-Vec3{}), Vec3>);
    static_assert(std::is_same_v<decltype(std::declval<Vec3>().normalized()), Vec3>);
    static_assert(std::is_same_v<decltype(std::declval<Vec3>().cross(Vec3{})), Vec3>);
    static_assert(std::is_same_v<decltype(std::declval<Vec3>().lerp(Vec3{}, 0.5f)), Vec3>);

    // Vec4 public API returns Vec4
    static_assert(std::is_same_v<decltype(Vec4{} + Vec4{}), Vec4>);
    static_assert(std::is_same_v<decltype(Vec4{} * 1.0f), Vec4>);
    static_assert(std::is_same_v<decltype(1.0f * Vec4{}), Vec4>);
    static_assert(std::is_same_v<decltype(-Vec4{}), Vec4>);

    // Mat4 public API returns Mat4 (or Vec4/Vec3 for products)
    static_assert(std::is_same_v<decltype(Mat4{} + Mat4{}), Mat4>);
    static_assert(std::is_same_v<decltype(Mat4{} * Mat4{}), Mat4>);
    static_assert(std::is_same_v<decltype(Mat4{} * Vec4{}), Vec4>);
    static_assert(std::is_same_v<decltype(Mat4{} * Vec3{}), Vec3>);
    static_assert(std::is_same_v<decltype(Vec3{} * Mat4{}), Vec3>);
    static_assert(std::is_same_v<decltype(Mat4{} * 1.0f), Mat4>);
    static_assert(std::is_same_v<decltype(std::declval<Mat4>().transpose()), Mat4>);
    static_assert(std::is_same_v<decltype(std::declval<Mat4>().inverse()), Mat4>);
    static_assert(std::is_same_v<decltype(std::declval<Mat4>().determinant()), float>);

    // Quat public API returns Quat (or Vec3 for rotate)
    static_assert(std::is_same_v<decltype(Quat{} * Quat{}), Quat>);
    static_assert(std::is_same_v<decltype(Quat{} * Vec3{}), Vec3>);
    static_assert(std::is_same_v<decltype(std::declval<Quat>().conjugate()), Quat>);
    static_assert(std::is_same_v<decltype(std::declval<Quat>().inverse()), Quat>);
    static_assert(std::is_same_v<decltype(std::declval<Quat>().to_mat4()), Mat4>);
    static_assert(std::is_same_v<decltype(std::declval<Quat>().normalized()), Quat>);

    REQUIRE(true);
}

// ===========================================================================
// Edge case tests (T-66 to T-71)
// ===========================================================================

TEST_CASE("Vec3 normalize zero vector produces NaN", "[math][edge]") {
    const Vec3 n = Vec3{}.normalized();
    REQUIRE(std::isnan(n.x));
    REQUIRE(std::isnan(n.y));
    REQUIRE(std::isnan(n.z));

    // In-place normalize also produces NaN
    Vec3 m;
    m.normalize();
    REQUIRE(std::isnan(m.x));
    REQUIRE(std::isnan(m.y));
    REQUIRE(std::isnan(m.z));
}

TEST_CASE("Mat4 inverse singular matrix", "[math][edge]") {
    // A zero matrix is singular
    const Mat4 zero = Mat4{} * 0.0f;
    REQUIRE(zero.determinant() == Approx(0.0f).margin(TOL));

    const Mat4 inv = zero.inverse();
    // GLM produces NaN/inf for singular matrix inverse
    bool any_nan_or_inf = std::isnan(inv[0][0]) || std::isinf(inv[0][0]) ||
                          std::isnan(inv[1][1]) || std::isinf(inv[1][1]) ||
                          std::isnan(inv[2][2]) || std::isinf(inv[2][2]) ||
                          std::isnan(inv[3][3]) || std::isinf(inv[3][3]);
    REQUIRE(any_nan_or_inf);
}

TEST_CASE("Quat slerp identical quaternions", "[math][edge]") {
    const Quat q = Quat::angle_axis(pi / 3.0f, Vec3::unit_y());

    // slerp(q, q, 0.5) should return q
    const Quat r = Quat::slerp(q, q, 0.5f);
    REQUIRE(r.w == Approx(q.w).margin(TOL));
    REQUIRE(r.x == Approx(q.x).margin(TOL));
    REQUIRE(r.y == Approx(q.y).margin(TOL));
    REQUIRE(r.z == Approx(q.z).margin(TOL));
}

TEST_CASE("Mat4 * Vec3 with zero matrix", "[math][edge]") {
    const Mat4 zero = Mat4{} * 0.0f;
    const Vec3 v{1.0f, 2.0f, 3.0f};
    const Vec3 r = zero * v;

    // Zero matrix should produce zero vector
    REQUIRE(r.x == Approx(0.0f).margin(TOL));
    REQUIRE(r.y == Approx(0.0f).margin(TOL));
    REQUIRE(r.z == Approx(0.0f).margin(TOL));
}

TEST_CASE("Division by zero in scalar operator/", "[math][edge]") {
    const Vec3 v{1.0f, 2.0f, 3.0f};
    const Vec3 r = v / 0.0f;

    // Division by zero produces inf
    REQUIRE(std::isinf(r.x));
    REQUIRE(std::isinf(r.y));
    REQUIRE(std::isinf(r.z));

    // Same sign as original
    REQUIRE(r.x > 0.0f);
    REQUIRE(r.y > 0.0f);
    REQUIRE(r.z > 0.0f);
}

TEST_CASE("Mat4::ortho degenerate parameters", "[math][edge]") {
    // Degenerate: left > right produces a matrix with negative scale.
    // GLM accepts this without crashing.
    const Mat4 m = Mat4::ortho(1.0f, -1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
    // Should not crash; verify the matrix has valid (but possibly degenerate) values
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            REQUIRE_FALSE(std::isnan(m[c][r]));
        }
    }
}
