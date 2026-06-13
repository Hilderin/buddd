#include "inspector_editors.h"
#include "editor.h"
#include "editor_context.h"
#include "scene/component_registry/type_registry.h"
#include "scene/component_registry/serialization_context.h"
#include "math/quat.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "engine_context.h"
#include "engine_service.h"
#include "render/render_system.h"
#include "platform/platform.h"
#include "window/window.h"

#include <glm/gtc/constants.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <memory>

namespace ed = buddd::editor;
namespace be = buddd::engine;
using Catch::Approx;

// ── Headless test context for editor integration tests ──
struct TestContext {
    std::unique_ptr<be::EngineService> engine;
    std::unique_ptr<be::World> engine_world;
    std::unique_ptr<be::RenderSystem> render_system;
    std::unique_ptr<be::EngineContext> ctx;
    std::unique_ptr<ed::Editor> editor;
    std::unique_ptr<ed::EditorContext> editor_ctx;

    TestContext() {
        auto eng = be::EngineService::create(
            be::Backend::Headless,
            be::WindowConfig{.title = "F-05 Test", .width = 128, .height = 128});
        REQUIRE(eng.has_value());
        engine = std::move(*eng);

        engine_world = std::make_unique<be::World>();
        render_system = std::make_unique<be::RenderSystem>(engine->device(), *engine_world);

        ctx = std::make_unique<be::EngineContext>(be::EngineContext{
            *engine, engine->window(), engine->device(), *engine_world,
            *render_system, 0.016f, 0});

        editor = std::make_unique<ed::Editor>();
        // Editor::setup() may fail in headless mode (no ImGui), but that's OK
        // for tests that don't need full ImGui interaction.
        [[maybe_unused]] auto setup_result = editor->setup(*ctx);

        editor_ctx = std::make_unique<ed::EditorContext>(ed::EditorContext{
            *editor, *ctx});
    }
};

// ═════════════════════════════════════════════════════════════════════
// InspectorTypeEditorRegistry: register and has_editor
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("InspectorTypeEditorRegistry: register and has_editor", "[editor][inspector]") {
    using namespace buddd::engine::math;

    // Register built-in editors (idempotent — safe to call multiple times)
    ed::register_builtin_inspector_editors();

    // After registration, all 8 types should be registered
    REQUIRE(ed::InspectorTypeEditorRegistry::has_editor<float>());
    REQUIRE(ed::InspectorTypeEditorRegistry::has_editor<int>());
    REQUIRE(ed::InspectorTypeEditorRegistry::has_editor<bool>());
    REQUIRE(ed::InspectorTypeEditorRegistry::has_editor<std::string>());
    REQUIRE(ed::InspectorTypeEditorRegistry::has_editor<Vec2>());
    REQUIRE(ed::InspectorTypeEditorRegistry::has_editor<Vec3>());
    REQUIRE(ed::InspectorTypeEditorRegistry::has_editor<Vec4>());
    REQUIRE(ed::InspectorTypeEditorRegistry::has_editor<Quat>());

    // Unregistered type should return false
    REQUIRE_FALSE(ed::InspectorTypeEditorRegistry::has_editor<double>());
}

// ═════════════════════════════════════════════════════════════════════
// InspectorTypeEditorRegistry: register custom editor and draw
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("InspectorTypeEditorRegistry: register custom editor and draw", "[editor][inspector]") {
    // Register a mock editor for int that always returns true
    ed::InspectorTypeEditorRegistry::register_editor<int>(
        [](const std::string&, int&, const ed::EditorFlags&,
           const ed::EditorContext&) -> bool {
            return true;
        }
    );

    // The editor should be registered
    REQUIRE(ed::InspectorTypeEditorRegistry::has_editor<int>());

    // Create a minimal context for the draw call
    TestContext tctx;

    int test_value = 42;
    bool result = ed::InspectorTypeEditorRegistry::draw<int>(
        "test_label", test_value, ed::EditorFlags{}, *tctx.editor_ctx);

    // The mock always returns true
    REQUIRE(result);
}

// ═════════════════════════════════════════════════════════════════════
// Quat::to_euler round-trip precision
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("Quat::to_euler round-trip precision", "[editor][inspector][math]") {
    using namespace buddd::engine::math;

    constexpr float EPSILON = 1e-6f;

    // Identity: (0,0,0) -> (0,0,0)
    {
        Quat q = Quat::from_euler(0.0f, 0.0f, 0.0f);
        Vec3 euler = q.to_euler();
        REQUIRE(euler.x == Approx(0.0f).margin(EPSILON));
        REQUIRE(euler.y == Approx(0.0f).margin(EPSILON));
        REQUIRE(euler.z == Approx(0.0f).margin(EPSILON));
    }

    // Known values: pitch=0.5, yaw=-1.2, roll=0.3
    {
        float pitch = 0.5f;
        float yaw = -1.2f;
        float roll = 0.3f;
        Quat q = Quat::from_euler(pitch, yaw, roll);
        Vec3 euler = q.to_euler();
        REQUIRE(euler.x == Approx(pitch).margin(EPSILON));
        REQUIRE(euler.y == Approx(yaw).margin(EPSILON));
        REQUIRE(euler.z == Approx(roll).margin(EPSILON));
    }

    // Multiple random values — each round-trip within epsilon
    struct TestAngle { float pitch, yaw, roll; };
    TestAngle angles[] = {
        {0.1f, 0.2f, 0.3f},
        {1.5f, -0.8f, 2.3f},
        {-1.0f, 0.5f, -0.5f},
        {0.8f, -1.6f, 0.4f},
    };
    for (const auto& a : angles) {
        Quat original = Quat::from_euler(a.pitch, a.yaw, a.roll);
        Vec3 euler = original.to_euler();
        Quat roundtrip = Quat::from_euler(euler.x, euler.y, euler.z);

        // Quaternions have double-cover: q and -q represent the same rotation.
        // Compare using dot product absolute value ≈ 1.
        float dot = roundtrip.x * original.x + roundtrip.y * original.y
                  + roundtrip.z * original.z + roundtrip.w * original.w;
        REQUIRE(std::abs(dot) >= 1.0f - EPSILON);
    }
}

// ═════════════════════════════════════════════════════════════════════
// Quat::to_euler gimbal lock handling
// ═════════════════════════════════════════════════════════════════════

TEST_CASE("Quat::to_euler gimbal lock handling", "[editor][inspector][math]") {
    using namespace buddd::engine::math;

    // Pitch = π/2 (90°): should not crash and produce valid finite numbers
    Quat q = Quat::from_euler(glm::pi<float>() / 2.0f, 0.0f, 0.0f);
    Vec3 euler = q.to_euler();

    // All values should be finite (no NaN, no inf)
    REQUIRE(std::isfinite(euler.x));
    REQUIRE(std::isfinite(euler.y));
    REQUIRE(std::isfinite(euler.z));

    // Also test negative gimbal lock
    Quat q2 = Quat::from_euler(-glm::pi<float>() / 2.0f, 1.0f, 0.5f);
    Vec3 euler2 = q2.to_euler();
    REQUIRE(std::isfinite(euler2.x));
    REQUIRE(std::isfinite(euler2.y));
    REQUIRE(std::isfinite(euler2.z));
}
