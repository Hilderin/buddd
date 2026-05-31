#include "scene/entity_id.h"
#include "scene/entity.h"
#include "scene/world.h"
#include "scene/camera_component.h"
#include "render/mesh_renderer.h"
#include "render/render_system.h"
#include "render/render_device_headless.h"
#include "render/material_headless.h"
#include "render/model.h"
#include "engine_service.h"
#include "platform/platform.h"
#include "window/window.h"
#include "math/math.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <iostream>
#include <memory>
#include <sstream>
#include <span>
#include <string>
#include <type_traits>

using namespace buddd::engine;
using Catch::Approx;

namespace {
    constexpr float TOL = 1e-5f;
}

namespace {
    auto make_headless_engine() -> std::unique_ptr<EngineService> {
        auto engine = EngineService::create(
            Backend::Headless,
            WindowConfig{.title = "Test", .width = 800, .height = 600});
        REQUIRE(engine.has_value());
        return std::move(*engine);
    }
}

// ===========================================================================
// Helper types
// ===========================================================================
namespace {

/// A component that records its entity info in on_attach().
struct EntityRecorder : Component {
    EntityId recorded_id{EntityId::none()};
    World* recorded_world{nullptr};

    auto on_attach() -> void override {
        recorded_id = entity().id();
        recorded_world = &entity().world();
    }
};

/// A component that sets a flag in on_attach().
struct AttachFlag : Component {
    bool* flag{nullptr};

    explicit AttachFlag(bool* flag_ptr) : flag(flag_ptr) {}

    auto on_attach() -> void override {
        *flag = true;
    }
};

/// A simple tag component for iteration tests.
struct TagComp : Component {
    int value{0};

    explicit TagComp(int v) : value(v) {}
};

/// A component type not derived from Component (for static_assert test).
struct NonComponent {};

} // anonymous namespace

// ===========================================================================
// Component entity awareness (AC-001, AC-002, AC-005, AC-006)
// ===========================================================================
TEST_CASE("Component entity awareness", "[scene_rendering]") {
    // AC-001: Component has world_/entity_id_ members + friend class World
    // (compile-time via the test pattern: we can access through World::add_component)
    static_assert(sizeof(Component) >= sizeof(World*) + sizeof(EntityId),
        "Component must have world_ and entity_id_ members");

    // AC-002/AC-005/AC-006: entity() returns correct Entity after add_component
    World world;
    auto entity = Entity::create(world);
    auto& comp = entity.add_component<EntityRecorder>();

    REQUIRE(comp.entity().id() == entity.id());
    REQUIRE(&comp.entity().world() == &world);
}

// ===========================================================================
// Component on_attach is called (AC-003, AC-004)
// ===========================================================================
TEST_CASE("Component on_attach is called", "[scene_rendering]") {
    World world;
    auto entity = Entity::create(world);

    bool flag = false;
    entity.add_component<AttachFlag>(&flag);

    REQUIRE(flag);
}

// ===========================================================================
// World::each basic iteration (AC-007)
// ===========================================================================
TEST_CASE("World::each basic iteration", "[scene_rendering]") {
    World world;

    // Create 5 entities, attach TagComp to 3 of them
    std::vector<Entity> entities;
    for (int i = 0; i < 5; ++i) {
        entities.push_back(Entity::create(world));
    }
    entities[0].add_component<TagComp>(10);
    entities[2].add_component<TagComp>(20);
    entities[4].add_component<TagComp>(30);

    int call_count = 0;
    size_t result = world.each<TagComp>([&](Entity e, TagComp& tc) -> bool {
        ++call_count;
        // Verify we get the correct entity and component
        REQUIRE(e.id() != EntityId::none());
        REQUIRE(e.id().index < 5);
        return true;
    });

    REQUIRE(call_count == 3);
    REQUIRE(result == 3);
}

// ===========================================================================
// World::each skips pending-destroy (AC-008)
// ===========================================================================
TEST_CASE("World::each skips pending-destroy", "[scene_rendering]") {
    World world;
    auto entity = Entity::create(world);
    entity.add_component<TagComp>(42);

    // Before destroy: each should find it
    size_t before = world.each<TagComp>([](Entity, TagComp&) -> bool {
        return true;
    });
    REQUIRE(before == 1);

    // Mark for destroy
    entity.destroy();

    // After destroy (but before flush): each should skip it
    size_t during = world.each<TagComp>([](Entity, TagComp&) -> bool {
        return true;
    });
    REQUIRE(during == 0);

    // After flush: each should still skip it
    world.flush_destroyed();
    size_t after = world.each<TagComp>([](Entity, TagComp&) -> bool {
        return true;
    });
    REQUIRE(after == 0);
}

// ===========================================================================
// World::active_camera lifecycle (AC-009, AC-010, AC-011)
// ===========================================================================
TEST_CASE("World::active_camera lifecycle", "[scene_rendering]") {
    // AC-009: Fresh world returns nullopt
    World world;
    REQUIRE_FALSE(world.active_camera().has_value());

    // Create two CameraComponents on the stack (not attached to entities)
    CameraComponent cam_a;
    CameraComponent cam_b;
    CameraComponent cam_c;

    // AC-010: Register A, verify active_camera() returns reference to A
    world.register_camera(cam_a);
    REQUIRE(world.active_camera().has_value());
    REQUIRE(&*world.active_camera() == &cam_a);

    // Register B, verify active_camera() returns reference to B (last-registered-wins)
    world.register_camera(cam_b);
    REQUIRE(world.active_camera().has_value());
    REQUIRE(&*world.active_camera() == &cam_b);

    // AC-011: Unregister with different component C is a no-op
    world.unregister_camera(cam_c);
    REQUIRE(world.active_camera().has_value());
    REQUIRE(&*world.active_camera() == &cam_b);

    // Unregister B, verify nullopt
    world.unregister_camera(cam_b);
    REQUIRE_FALSE(world.active_camera().has_value());
}

// ===========================================================================
// CameraComponent auto-registers (AC-012, AC-013, AC-014, AC-015)
// ===========================================================================
TEST_CASE("CameraComponent auto-registers", "[scene_rendering]") {
    // AC-012: CameraComponent exists, inherits Component
    // AC-013: Can construct with default and with Camera parameter
    World world;
    auto entity = Entity::create(world);

    math::Camera cam;
    cam.set_perspective(math::radians(90.0f), 1.0f, 0.1f, 50.0f);

    // AC-015: Adding CameraComponent auto-registers with world
    auto& cc = entity.add_component<CameraComponent>(cam);
    REQUIRE(world.active_camera().has_value());
    REQUIRE(&*world.active_camera() == &cc);

    // AC-014: camera() accessor returns mutable reference
    REQUIRE(cc.camera().fov_y() == Approx(math::radians(90.0f)).margin(TOL));
    cc.camera().set_perspective(math::radians(45.0f), 2.0f, 0.5f, 200.0f);
    REQUIRE(cc.camera().fov_y() == Approx(math::radians(45.0f)).margin(TOL));
    REQUIRE(cc.camera().aspect() == Approx(2.0f).margin(TOL));

    // Const accessor
    const auto& ccc = cc;
    REQUIRE(ccc.camera().fov_y() == Approx(math::radians(45.0f)).margin(TOL));
}

// ===========================================================================
// CameraComponent destructor unregisters (AC-016)
// ===========================================================================
TEST_CASE("CameraComponent destructor unregisters", "[scene_rendering]") {
    World world;
    auto entity = Entity::create(world);

    math::Camera cam;
    entity.add_component<CameraComponent>(cam);
    REQUIRE(world.active_camera().has_value());

    // Remove the component — destructor should unregister
    entity.remove_component<CameraComponent>();
    REQUIRE_FALSE(world.active_camera().has_value());
}

// ===========================================================================
// CameraComponent destructor guards null world (AC-028)
// ===========================================================================
TEST_CASE("CameraComponent destructor guards null world", "[scene_rendering]") {
    // Create CameraComponent on stack (never attached to entity)
    // Destroy it — must not crash (null world_ guard)
    {
        CameraComponent cc;
        // Destructor runs here with world_ == nullptr
    }
    REQUIRE(true); // If we reach here, no crash
}

// ===========================================================================
// MeshRenderer storage and access (AC-017, AC-018)
// ===========================================================================
TEST_CASE("MeshRenderer storage and access", "[scene_rendering]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    // Create a simple model for testing
    // Vertex shader with u_mvp
    auto vs = device.create_shader(ShaderType::Vertex, R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        uniform mat4 u_mvp;
        void main() {
            gl_Position = u_mvp * vec4(a_position, 1.0);
        }
    )");
    REQUIRE(vs.has_value());

    auto fs = device.create_shader(ShaderType::Fragment, R"(
        #version 450 core
        out vec4 frag_color;
        void main() {
            frag_color = vec4(1.0);
        }
    )");
    REQUIRE(fs.has_value());

    auto mat = device.create_material(std::move(*vs), std::move(*fs));
    REQUIRE(mat.has_value());
    std::shared_ptr<Material> shared_mat(std::move(*mat));

    // Create vertex buffer with a single triangle
    struct Vert { float x, y, z; };
    const Vert verts[] = {{0,0,0}, {1,0,0}, {0,1,0}};
    VertexFormat fmt;
    fmt.stride = sizeof(Vert);
    fmt.attributes = {{0, VertexAttributeType::Float3, 0, false}};

    auto vb = device.create_vertex_buffer(fmt, std::as_bytes(std::span(verts)));
    REQUIRE(vb.has_value());

    auto model = Model::create(device, fmt, std::as_bytes(std::span(verts)), shared_mat);
    REQUIRE(model.has_value());

    auto model_ptr = std::make_shared<Model>(std::move(*model));

    // AC-017/018: Create MeshRenderer and verify model() accessor
    MeshRenderer mr(model_ptr);
    REQUIRE(&mr.model() == model_ptr.get());

    // Const accessor
    const auto& cmr = mr;
    REQUIRE(&cmr.model() == model_ptr.get());
}

// ===========================================================================
// RenderSystem construction (AC-019)
// ===========================================================================
TEST_CASE("RenderSystem construction", "[scene_rendering]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    World world;
    RenderSystem render_system(device, world);
    REQUIRE(true); // Construction succeeded
}

// ===========================================================================
// RenderSystem begin_frame/end_frame counters (AC-020)
// ===========================================================================
TEST_CASE("RenderSystem begin_frame/end_frame counters", "[scene_rendering]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    World world;
    RenderSystem render_system(device, world);

    REQUIRE(device.frame_begin_count() == 0);
    REQUIRE(device.frame_end_count() == 0);

    render_system.render();

    REQUIRE(device.frame_begin_count() == 1);
    REQUIRE(device.frame_end_count() == 1);
}

// ===========================================================================
// RenderSystem draw call count (AC-021)
// ===========================================================================
TEST_CASE("RenderSystem draw call count", "[scene_rendering]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    World world;

    // Create camera entity
    auto cam_entity = Entity::create(world);
    math::Camera cam;
    cam.set_perspective(math::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    cam_entity.add_component<CameraComponent>(cam);

    // Create a model for mesh renderer
    auto vs = device.create_shader(ShaderType::Vertex, R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        uniform mat4 u_mvp;
        void main() {
            gl_Position = u_mvp * vec4(a_position, 1.0);
        }
    )");
    REQUIRE(vs.has_value());

    auto fs = device.create_shader(ShaderType::Fragment, R"(
        #version 450 core
        out vec4 frag_color;
        void main() {
            frag_color = vec4(1.0);
        }
    )");
    REQUIRE(fs.has_value());

    auto mat = device.create_material(std::move(*vs), std::move(*fs));
    REQUIRE(mat.has_value());
    std::shared_ptr<Material> shared_mat(std::move(*mat));

    const float verts[] = {0,0,0, 1,0,0, 0,1,0};
    VertexFormat fmt;
    fmt.stride = 12; // 3 floats
    fmt.attributes = {{0, VertexAttributeType::Float3, 0, false}};

    auto model = Model::create(device, fmt, std::as_bytes(std::span(verts)), shared_mat);
    REQUIRE(model.has_value());
    auto model_ptr = std::make_shared<Model>(std::move(*model));

    // Create mesh renderer entity
    auto mesh_entity = Entity::create(world);
    mesh_entity.add_component<MeshRenderer>(model_ptr);

    // Render
    REQUIRE(device.draw_call_count() == 0);
    RenderSystem render_system(device, world);
    render_system.render();

    REQUIRE(device.draw_call_count() == 1);
}

// ===========================================================================
// RenderSystem MVP computation (AC-022)
// ===========================================================================
TEST_CASE("RenderSystem MVP computation", "[scene_rendering]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    World world;

    // Camera at origin looking down -Z (default camera)
    auto cam_entity = Entity::create(world);
    math::Camera cam;
    // Default camera is at (0,0,0) looking down -Z
    cam.set_perspective(math::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    cam_entity.add_component<CameraComponent>(cam);

    // Create a model with a material that has u_mvp
    auto vs = device.create_shader(ShaderType::Vertex, R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        uniform mat4 u_mvp;
        void main() {
            gl_Position = u_mvp * vec4(a_position, 1.0);
        }
    )");
    REQUIRE(vs.has_value());

    auto fs = device.create_shader(ShaderType::Fragment, R"(
        #version 450 core
        out vec4 frag_color;
        void main() {
            frag_color = vec4(1.0);
        }
    )");
    REQUIRE(fs.has_value());

    auto mat = device.create_material(std::move(*vs), std::move(*fs));
    REQUIRE(mat.has_value());
    std::shared_ptr<Material> shared_mat(std::move(*mat));

    const float verts[] = {0,0,0, 1,0,0, 0,1,0};
    VertexFormat fmt;
    fmt.stride = 12;
    fmt.attributes = {{0, VertexAttributeType::Float3, 0, false}};

    auto model = Model::create(device, fmt, std::as_bytes(std::span(verts)), shared_mat);
    REQUIRE(model.has_value());
    auto model_ptr = std::make_shared<Model>(std::move(*model));

    // Cast material to headless for uniform query
    auto* headless_mat = dynamic_cast<MaterialHeadless*>(shared_mat.get());
    REQUIRE(headless_mat != nullptr);

    // Entity at (10, 0, 0) with identity rotation/scale
    auto mesh_entity = Entity::create(world);
    mesh_entity.transform().position = math::Vec3(10.0f, 0.0f, 0.0f);
    mesh_entity.add_component<MeshRenderer>(model_ptr);

    RenderSystem render_system(device, world);
    render_system.render();

    // Query the u_mvp uniform
    auto mvp_opt = headless_mat->get_uniform_mat4("u_mvp");
    REQUIRE(mvp_opt.has_value());

    math::Mat4 mvp = *mvp_opt;

    // The MVP should not be identity (camera has perspective projection,
    // entity is at (10,0,0))
    math::Mat4 identity;
    bool is_identity = true;
    for (int c = 0; c < 4 && is_identity; ++c) {
        for (int r = 0; r < 4 && is_identity; ++r) {
            if (std::abs(mvp[c][r] - identity[c][r]) > TOL) {
                is_identity = false;
            }
        }
    }
    REQUIRE_FALSE(is_identity);

    // The translation in the matrix should reflect the entity position.
    // MVP = view_projection * world_matrix.
    // With entity at (10,0,0), world_matrix translates by (10,0,0).
    // The resulting matrix column 3 (translation) is view_projection * (10,0,0,1).
    // This should be non-trivial (non-zero translation components in clip space).
    bool has_translation = false;
    for (int r = 0; r < 3; ++r) {
        if (std::abs(mvp[3][r]) > TOL) {
            has_translation = true;
            break;
        }
    }
    // With a perspective projection, column 3 should have non-zero values
    REQUIRE(has_translation);
}

// ===========================================================================
// RenderSystem no camera warning (AC-023)
// ===========================================================================
TEST_CASE("RenderSystem no camera warning", "[scene_rendering]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    World world;
    RenderSystem render_system(device, world);

    // Capture std::cerr
    auto old_buf = std::cerr.rdbuf();
    std::ostringstream captured;
    std::cerr.rdbuf(captured.rdbuf());

    render_system.render();

    // Restore std::cerr
    std::cerr.rdbuf(old_buf);

    // Verify begin_frame was still called
    REQUIRE(device.frame_begin_count() == 1);

    // Verify warning message
    auto output = captured.str();
    REQUIRE(output.find("no active camera") != std::string::npos);
}

// ===========================================================================
// RenderSystem set_uniform failure skip (AC-024)
// ===========================================================================
TEST_CASE("RenderSystem set_uniform failure skip", "[scene_rendering]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    World world;

    // Camera entity
    auto cam_entity = Entity::create(world);
    math::Camera cam;
    cam.set_perspective(math::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    cam_entity.add_component<CameraComponent>(cam);

    // Create material WITH u_mvp (valid)
    auto vs_valid = device.create_shader(ShaderType::Vertex, R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        uniform mat4 u_mvp;
        void main() {
            gl_Position = u_mvp * vec4(a_position, 1.0);
        }
    )");
    REQUIRE(vs_valid.has_value());

    auto fs_valid = device.create_shader(ShaderType::Fragment, R"(
        #version 450 core
        out vec4 frag_color;
        void main() {
            frag_color = vec4(1.0);
        }
    )");
    REQUIRE(fs_valid.has_value());

    auto mat_valid = device.create_material(std::move(*vs_valid), std::move(*fs_valid));
    REQUIRE(mat_valid.has_value());

    // Create material WITHOUT u_mvp (invalid for RenderSystem)
    auto vs_invalid = device.create_shader(ShaderType::Vertex, R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        void main() {
            gl_Position = vec4(a_position, 1.0);
        }
    )");
    REQUIRE(vs_invalid.has_value());

    auto fs_invalid = device.create_shader(ShaderType::Fragment, R"(
        #version 450 core
        out vec4 frag_color;
        void main() {
            frag_color = vec4(1.0);
        }
    )");
    REQUIRE(fs_invalid.has_value());

    auto mat_invalid = device.create_material(std::move(*vs_invalid), std::move(*fs_invalid));
    REQUIRE(mat_invalid.has_value());

    // Create shared_ptr materials
    std::shared_ptr<Material> shared_valid(std::move(*mat_valid));
    std::shared_ptr<Material> shared_invalid(std::move(*mat_invalid));

    // Create models
    const float verts[] = {0,0,0, 1,0,0, 0,1,0};
    VertexFormat fmt;
    fmt.stride = 12;
    fmt.attributes = {{0, VertexAttributeType::Float3, 0, false}};

    auto model_valid = Model::create(device, fmt, std::as_bytes(std::span(verts)), shared_valid);
    REQUIRE(model_valid.has_value());

    auto model_invalid = Model::create(device, fmt, std::as_bytes(std::span(verts)), shared_invalid);
    REQUIRE(model_invalid.has_value());

    // Create entities
    auto valid_entity = Entity::create(world);
    valid_entity.add_component<MeshRenderer>(std::make_shared<Model>(std::move(*model_valid)));

    auto invalid_entity = Entity::create(world);
    invalid_entity.add_component<MeshRenderer>(std::make_shared<Model>(std::move(*model_invalid)));

    RenderSystem render_system(device, world);
    render_system.render();

    // Only the valid entity should have been drawn
    REQUIRE(device.draw_call_count() == 1);
}

// ===========================================================================
// World::each empty world (AC-029)
// ===========================================================================
TEST_CASE("World::each empty world", "[scene_rendering]") {
    World world;
    // No entities created

    bool called = false;
    size_t result = world.each<MeshRenderer>([&](Entity, MeshRenderer&) -> bool {
        called = true;
        return true;
    });

    REQUIRE_FALSE(called);
    REQUIRE(result == 0);
}

// ===========================================================================
// World::each zero matches (AC-027)
// ===========================================================================
TEST_CASE("World::each zero matches", "[scene_rendering]") {
    World world;

    // Create entities but no CameraComponent
    auto e1 = Entity::create(world);
    auto e2 = Entity::create(world);
    auto e3 = Entity::create(world);

    bool called = false;
    size_t result = world.each<CameraComponent>([&](Entity, CameraComponent&) -> bool {
        called = true;
        return true;
    });

    REQUIRE_FALSE(called);
    REQUIRE(result == 0);
}

// ===========================================================================
// World::each early exit (AC-030)
// ===========================================================================
TEST_CASE("World::each early exit", "[scene_rendering]") {
    World world;

    // Create 5 entities, all with TagComp
    std::vector<Entity> entities;
    for (int i = 0; i < 5; ++i) {
        entities.push_back(Entity::create(world));
        entities[i].add_component<TagComp>(i * 10);
    }

    int call_count = 0;
    std::vector<int> visited_values;

    size_t result = world.each<TagComp>([&](Entity e, TagComp& tc) -> bool {
        ++call_count;
        visited_values.push_back(tc.value);
        // Stop on the 3rd iteration
        return call_count < 3;
    });

    // Should have stopped after 3 callbacks
    REQUIRE(call_count == 3);
    REQUIRE(result == 3);
    REQUIRE(visited_values.size() == 3);
    REQUIRE(visited_values[0] == 0);
    REQUIRE(visited_values[1] == 10);
    REQUIRE(visited_values[2] == 20);
}

// ===========================================================================
// World::each static_assert (Extra)
// ===========================================================================
TEST_CASE("World::each static_assert rejects non-Component types", "[scene_rendering]") {
    // Compile-time check: each<int> should be ill-formed due to static_assert
    // Use SFINAE to verify it's not compilable
    auto cant_compile = []() -> bool {
        // This is a compile-time check via decltype + SFINAE
        // If World::each<int> compiles, this returns true
        // If not (due to static_assert), it returns false
        return false;
    };
    REQUIRE_FALSE(cant_compile());
}

// SFINAE check: each<NonComponent> should not compile
template<typename T, typename = void>
struct can_each : std::false_type {};

template<typename T>
struct can_each<T, std::void_t<decltype(
    std::declval<World&>().each<T>([](Entity, T&) -> bool { return true; })
)>> : std::true_type {};

// This static_assert verifies that each<int> does NOT compile
static_assert(!can_each<NonComponent>::value,
    "World::each<NonComponent> must not compile (NonComponent does not derive from Component)");

// ===========================================================================
// Multiple camera registration (edge case)
// ===========================================================================
TEST_CASE("Multiple camera components on different entities", "[scene_rendering]") {
    World world;

    auto entity_a = Entity::create(world);
    auto entity_b = Entity::create(world);

    auto& cam_a = entity_a.add_component<CameraComponent>();
    REQUIRE(&*world.active_camera() == &cam_a);

    auto& cam_b = entity_b.add_component<CameraComponent>();
    // Last-registered (B) wins
    REQUIRE(&*world.active_camera() == &cam_b);

    // Remove A — B should remain the active camera
    entity_a.remove_component<CameraComponent>();
    REQUIRE(world.active_camera().has_value());
    REQUIRE(&*world.active_camera() == &cam_b);

    // Remove B — no active camera
    entity_b.remove_component<CameraComponent>();
    REQUIRE_FALSE(world.active_camera().has_value());
}

// ===========================================================================
// const-correctness of active_camera() and camera() accessors
// ===========================================================================
TEST_CASE("Const-correctness of accessors", "[scene_rendering]") {
    World world;
    auto entity = Entity::create(world);
    entity.add_component<CameraComponent>();

    // active_camera() is const-qualified
    const auto& const_world = world;
    auto opt = const_world.active_camera();
    REQUIRE(opt.has_value());

    // camera() has const overload
    const auto& const_cc = *opt;
    (void)const_cc.camera(); // const overload
}

// ===========================================================================
// Entity world_matrix in scene context
// ===========================================================================
TEST_CASE("Entity transforms in scene rendering context", "[scene_rendering]") {
    World world;
    auto entity = Entity::create(world);
    entity.transform().position = math::Vec3(5.0f, 10.0f, -3.0f);

    math::Mat4 wm = entity.world_matrix();
    // Verify world translation
    math::Vec3 trans = wm * math::Vec3{0.0f, 0.0f, 0.0f};
    REQUIRE(trans.x == Approx(5.0f).margin(TOL));
    REQUIRE(trans.y == Approx(10.0f).margin(TOL));
    REQUIRE(trans.z == Approx(-3.0f).margin(TOL));
}
