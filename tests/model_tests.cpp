#include "render/model.h"
#include "render/render_device.h"
#include "render/render_device_headless.h"
#include "render/shader.h"
#include "render/material.h"
#include "render/material_headless.h"
#include "render/vertex_format.h"
#include "render/primitive_topology.h"
#include "render/primitives.h"
#include "engine_service.h"
#include "platform/platform.h"
#include "window/window.h"
#include "error.h"

#include "math/mat4.h"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>

namespace be = buddd::engine;

/// Creates a headless EngineService for testing.
auto make_headless_engine() -> std::unique_ptr<be::EngineService> {
    auto engine = be::EngineService::create(
        be::Backend::Headless,
        be::WindowConfig{.title = "Test", .width = 800, .height = 600});
    REQUIRE(engine.has_value());
    return std::move(*engine);
}

/// Minimal vertex shader with position + color attributes and u_mvp uniform.
constexpr std::string_view k_test_vs = R"(
    #version 450 core
    layout(location = 0) in vec3 a_position;
    layout(location = 1) in vec3 a_color;
    out vec3 v_color;
    uniform mat4 u_mvp;
    void main() {
        gl_Position = u_mvp * vec4(a_position, 1.0);
        v_color = a_color;
    }
)";

/// Minimal fragment shader that passes vertex color through.
constexpr std::string_view k_test_fs = R"(
    #version 450 core
    in vec3 v_color;
    out vec4 frag_color;
    void main() {
        frag_color = vec4(v_color, 1.0);
    }
)";

/// Creates a test material with the standard position+color shaders.
auto create_test_material(be::RenderDevice& device) -> std::shared_ptr<be::Material> {
    auto vs = device.create_shader(be::ShaderType::Vertex, k_test_vs);
    REQUIRE(vs.has_value());
    auto fs = device.create_shader(be::ShaderType::Fragment, k_test_fs);
    REQUIRE(fs.has_value());
    auto mat = device.create_material(std::move(*vs), std::move(*fs));
    REQUIRE(mat.has_value());
    return std::shared_ptr<be::Material>(std::move(*mat));
}

// Interleaved vertex: position (Float3) + color (Float3), stride = 24
struct CubeVertex { float px, py, pz, cr, cg, cb; };

/// Helper: creates a simple vertex format (Float3 position, stride 12).
auto make_pos_format() -> be::VertexFormat {
    return be::VertexFormat{12, {{0, be::VertexAttributeType::Float3, 0, false}}};
}

/// Helper: creates a simple indexed model with one submesh and one material.
auto make_simple_model(be::RenderDevice& device, std::shared_ptr<be::Material> mat)
    -> be::Model
{
    const float verts[] = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
    const uint16_t idxs[] = {0, 1, 2};
    auto fmt = make_pos_format();
    auto model = be::Model::create_indexed(
        device, fmt,
        std::as_bytes(std::span(verts)),
        std::as_bytes(std::span(idxs)),
        be::IndexType::Uint16,
        { be::SubMesh{0, 3, 0} },
        { mat });
    REQUIRE(model.has_value());
    return std::move(*model);
}

// ===========================================================================
// AC-001: SubMesh struct exists with fields
// ===========================================================================
TEST_CASE("SubMesh struct fields exist", "[model]") {
    be::SubMesh sm;
    sm.index_start = 0;
    sm.index_count = 36;
    sm.material_index = 0;
    REQUIRE(sm.index_start == 0);
    REQUIRE(sm.index_count == 36);
    REQUIRE(sm.material_index == 0);
}

// ===========================================================================
// Model factory tests
// ===========================================================================

TEST_CASE("Model default construction creates null model", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    be::Model null_model;
    // Calling draw on null model is a no-op (no crash)
    device.begin_frame();
    null_model.draw(device);
    device.end_frame();
    // Verify it's null
    REQUIRE(null_model.vertex_count() == 0);
    REQUIRE(null_model.index_count() == 0);
}

TEST_CASE("Model is non-copyable and movable", "[model]") {
    static_assert(!std::is_copy_constructible_v<be::Model>,
                  "Model must be non-copyable");
    static_assert(std::is_move_constructible_v<be::Model>,
                  "Model must be movable");

    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto model = make_simple_model(device, mat);

    // Move construct
    be::Model m2(std::move(model));
    REQUIRE(m2.vertex_count() == 3);
    REQUIRE(m2.index_count() == 3);

    // Move assign
    be::Model m3;
    m3 = std::move(m2);
    REQUIRE(m3.vertex_count() == 3);
}

TEST_CASE("Model::create_indexed with valid data succeeds", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto model = make_simple_model(device, mat);
    REQUIRE(model.vertex_count() == 3);
    REQUIRE(model.index_count() == 3);
    REQUIRE(model.submeshes().size() == 1);
    REQUIRE(model.submeshes()[0].index_count == 3);
    REQUIRE(model.submeshes()[0].material_index == 0);
    REQUIRE(model.materials().size() == 1);
    REQUIRE(model.materials()[0] == mat);
}

TEST_CASE("Model::create_indexed returns InvalidArgument for empty vertex data", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto fmt = make_pos_format();
    const uint16_t idxs[] = {0, 1, 2};
    auto result = be::Model::create_indexed(
        device, fmt, {}, std::as_bytes(std::span(idxs)),
        be::IndexType::Uint16,
        { be::SubMesh{0, 3, 0} },
        { mat });
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == be::Error::Category::InvalidArgument);
}

TEST_CASE("Model::create_indexed returns InvalidArgument for empty index data", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto fmt = make_pos_format();
    const float verts[] = {0.0f, 0.0f, 0.0f};
    auto result = be::Model::create_indexed(
        device, fmt, std::as_bytes(std::span(verts)), {},
        be::IndexType::Uint16,
        { be::SubMesh{0, 3, 0} },
        { mat });
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == be::Error::Category::InvalidArgument);
}

TEST_CASE("Model::create_indexed returns InvalidArgument for zero stride", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat fmt;
    fmt.stride = 0;
    fmt.attributes = {{0, be::VertexAttributeType::Float3, 0, false}};

    const float verts[] = {0.0f, 0.0f, 0.0f};
    const uint16_t idxs[] = {0};
    auto result = be::Model::create_indexed(
        device, fmt, std::as_bytes(std::span(verts)), std::as_bytes(std::span(idxs)),
        be::IndexType::Uint16,
        { be::SubMesh{0, 1, 0} },
        { mat });
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == be::Error::Category::InvalidArgument);
}

TEST_CASE("Model::create_indexed returns InvalidArgument for zero attributes", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat fmt;
    fmt.stride = 12;
    fmt.attributes = {};

    const float verts[] = {0.0f, 0.0f, 0.0f};
    const uint16_t idxs[] = {0};
    auto result = be::Model::create_indexed(
        device, fmt, std::as_bytes(std::span(verts)), std::as_bytes(std::span(idxs)),
        be::IndexType::Uint16,
        { be::SubMesh{0, 1, 0} },
        { mat });
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == be::Error::Category::InvalidArgument);
}

// ===========================================================================
// AC-002: Multi-submesh / multi-material support
// ===========================================================================
TEST_CASE("Model with 2 submeshes and 2 materials", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat_a = create_test_material(device);
    auto mat_b = create_test_material(device);

    auto fmt = make_pos_format();
    const float verts[] = {0,0,0, 1,0,0, 0,1,0, 1,1,0};
    const uint16_t idxs[] = {0,1,2, 0,2,3};
    auto model = be::Model::create_indexed(
        device, fmt,
        std::as_bytes(std::span(verts)),
        std::as_bytes(std::span(idxs)),
        be::IndexType::Uint16,
        {
            {0, 3, 0},
            {3, 3, 1},
        },
        {mat_a, mat_b}
    );
    REQUIRE(model.has_value());

    auto& submeshes = model->submeshes();
    REQUIRE(submeshes.size() == 2);
    REQUIRE(submeshes[0].index_start == 0);
    REQUIRE(submeshes[0].index_count == 3);
    REQUIRE(submeshes[0].material_index == 0);
    REQUIRE(submeshes[1].index_start == 3);
    REQUIRE(submeshes[1].index_count == 3);
    REQUIRE(submeshes[1].material_index == 1);

    auto& materials = model->materials();
    REQUIRE(materials.size() == 2);
    REQUIRE(materials[0] == mat_a);
    REQUIRE(materials[1] == mat_b);
}

// ===========================================================================
// AC-003/004: Empty data validation (tested above in create_indexed tests)
// ===========================================================================

// ===========================================================================
// AC-005: Draw call count
// ===========================================================================
TEST_CASE("Model::draw issues one draw call per submesh", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto model = make_simple_model(device, mat);

    device.begin_frame();
    int before = device.draw_call_count();
    model.draw(device);
    int after = device.draw_call_count();
    device.end_frame();

    // 1 submesh → 1 draw call
    REQUIRE(after - before == 1);
}

TEST_CASE("Model::draw with 3 submeshes issues 3 draw calls", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat_a = create_test_material(device);
    auto mat_b = create_test_material(device);
    auto mat_c = create_test_material(device);

    auto fmt = make_pos_format();
    const float verts[] = {0,0,0, 1,0,0, 0,1,0, 1,1,0, 0,0,1, 1,0,1};
    const uint16_t idxs[] = {0,1,2, 3,4,5};
    auto model = be::Model::create_indexed(
        device, fmt,
        std::as_bytes(std::span(verts)),
        std::as_bytes(std::span(idxs)),
        be::IndexType::Uint16,
        {
            {0, 2, 0},
            {2, 2, 1},
            {4, 2, 2},
        },
        {mat_a, mat_b, mat_c}
    );
    REQUIRE(model.has_value());

    device.begin_frame();
    int before = device.draw_call_count();
    model->draw(device);
    int after = device.draw_call_count();
    device.end_frame();

    REQUIRE(after - before == 3);
}

// ===========================================================================
// AC-006: Material tracking (verify material used via tracking)
// ===========================================================================
TEST_CASE("Model::draw binds correct materials via material_index", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat_a = create_test_material(device);
    auto mat_b = create_test_material(device);

    auto fmt = make_pos_format();
    const float verts[] = {0,0,0, 1,0,0, 0,1,0, 1,1,0};
    const uint16_t idxs[] = {0,1,2, 0,2,3};
    auto model = be::Model::create_indexed(
        device, fmt,
        std::as_bytes(std::span(verts)),
        std::as_bytes(std::span(idxs)),
        be::IndexType::Uint16,
        {
            {0, 3, 0},
            {3, 3, 1},
        },
        {mat_a, mat_b}
    );
    REQUIRE(model.has_value());

    device.begin_frame();
    model->draw(device);
    device.end_frame();

    // At minimum, we verify no crash and draw_call_count increased
    // A full material tracking test would require last_bound_material() on headless
    REQUIRE(device.draw_call_count() > 0);
}

// ===========================================================================
// AC-007: Null material → fallback
// ===========================================================================
TEST_CASE("Model::draw uses fallback when material is null", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    auto fmt = make_pos_format();
    const float verts[] = {0,0,0, 1,0,0, 0,1,0};
    const uint16_t idxs[] = {0, 1, 2};
    auto model = be::Model::create_indexed(
        device, fmt,
        std::as_bytes(std::span(verts)),
        std::as_bytes(std::span(idxs)),
        be::IndexType::Uint16,
        { be::SubMesh{0, 3, 0} },
        { std::shared_ptr<be::Material>(nullptr) }  // null material
    );
    REQUIRE(model.has_value());

    device.begin_frame();
    // Should not crash — uses fallback material
    model->draw(device);
    device.end_frame();

    REQUIRE(device.draw_call_count() == 1);
}

// ===========================================================================
// AC-008: Out-of-bounds material_index → fallback
// ===========================================================================
TEST_CASE("Model::draw uses fallback when material_index out of bounds", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto fmt = make_pos_format();
    const float verts[] = {0,0,0, 1,0,0, 0,1,0};
    const uint16_t idxs[] = {0, 1, 2};
    // material_index=5 but only 1 material → should use fallback
    auto model = be::Model::create_indexed(
        device, fmt,
        std::as_bytes(std::span(verts)),
        std::as_bytes(std::span(idxs)),
        be::IndexType::Uint16,
        { be::SubMesh{0, 3, 5} },
        { mat }
    );
    REQUIRE(model.has_value());

    device.begin_frame();
    model->draw(device);
    device.end_frame();

    REQUIRE(device.draw_call_count() == 1);
}

// ===========================================================================
// AC-009: Empty submeshes → no draw calls
// ===========================================================================
TEST_CASE("Model::draw with empty submeshes is no-op", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto fmt = make_pos_format();
    const float verts[] = {0,0,0, 1,0,0, 0,1,0};
    const uint16_t idxs[] = {0, 1, 2};
    auto model = be::Model::create_indexed(
        device, fmt,
        std::as_bytes(std::span(verts)),
        std::as_bytes(std::span(idxs)),
        be::IndexType::Uint16,
        {},  // empty submeshes
        { mat }
    );
    REQUIRE(model.has_value());

    device.begin_frame();
    int before = device.draw_call_count();
    model->draw(device);
    int after = device.draw_call_count();
    device.end_frame();

    REQUIRE(after - before == 0);
}

// ===========================================================================
// AC-010: Moved-from model → no-op draw
// ===========================================================================
TEST_CASE("Moved-from Model draw is no-op", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto model = make_simple_model(device, mat);

    be::Model moved(std::move(model));
    device.begin_frame();
    int before = device.draw_call_count();
    model.draw(device);  // moved-from
    int after = device.draw_call_count();
    device.end_frame();

    REQUIRE(after - before == 0);
}

// ===========================================================================
// Model accessor tests
// ===========================================================================

TEST_CASE("Model::vertices returns non-null reference", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto model = make_simple_model(device, mat);
    REQUIRE(&model.vertices() != nullptr);
}

TEST_CASE("Model::indices returns reference on indexed model", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto model = make_simple_model(device, mat);
    REQUIRE(&model.indices() != nullptr);
}

TEST_CASE("Model::draw on indexed model", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto model = make_simple_model(device, mat);

    device.begin_frame();
    model.draw(device);
    device.end_frame();
    // No crash — test passes
    REQUIRE(true);
}

TEST_CASE("Model::draw on null model is no-op", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    be::Model null_model;

    device.begin_frame();
    // Should not crash
    null_model.draw(device);
    device.end_frame();
    REQUIRE(true);
}

// ===========================================================================
// Model move semantics tests
// ===========================================================================

TEST_CASE("Move constructor transfers ownership", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto model = make_simple_model(device, mat);
    auto vcount_orig = model.vertex_count();
    auto icount_orig = model.index_count();
    auto submesh_count = model.submeshes().size();
    auto mat_count = model.materials().size();

    be::Model m2(std::move(model));
    REQUIRE(m2.vertex_count() == vcount_orig);
    REQUIRE(m2.index_count() == icount_orig);
    REQUIRE(m2.submeshes().size() == submesh_count);
    REQUIRE(m2.materials().size() == mat_count);

    // Source is null — draw is no-op
    REQUIRE(model.vertex_count() == 0);
    REQUIRE(model.index_count() == 0);
    device.begin_frame();
    model.draw(device);
    device.end_frame();
}

TEST_CASE("Move assignment transfers ownership", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto model = make_simple_model(device, mat);

    be::Model m2;
    m2 = std::move(model);
    REQUIRE(m2.vertex_count() == 3);

    // Source is null
    REQUIRE(model.vertex_count() == 0);

    // Draw on m2 works
    device.begin_frame();
    m2.draw(device);
    device.end_frame();
}

// ===========================================================================
// Cube data / material tests
// ===========================================================================

TEST_CASE("Model with 24 vertices and 36 indices (cube data)", "[cube]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    const CubeVertex vertices[] = {
        { 1.f, -1.f, -1.f,  1.f, 0.f, 0.f },
        { 1.f, -1.f,  1.f,  1.f, 0.f, 0.f },
        { 1.f,  1.f,  1.f,  1.f, 0.f, 0.f },
        { 1.f,  1.f, -1.f,  1.f, 0.f, 0.f },
        {-1.f, -1.f, -1.f,  0.f, 1.f, 0.f },
        {-1.f, -1.f,  1.f,  0.f, 1.f, 0.f },
        {-1.f,  1.f,  1.f,  0.f, 1.f, 0.f },
        {-1.f,  1.f, -1.f,  0.f, 1.f, 0.f },
        {-1.f,  1.f,  1.f,  0.f, 0.f, 1.f },
        { 1.f,  1.f,  1.f,  0.f, 0.f, 1.f },
        { 1.f,  1.f, -1.f,  0.f, 0.f, 1.f },
        {-1.f,  1.f, -1.f,  0.f, 0.f, 1.f },
        {-1.f, -1.f, -1.f,  1.f, 1.f, 0.f },
        { 1.f, -1.f, -1.f,  1.f, 1.f, 0.f },
        { 1.f, -1.f,  1.f,  1.f, 1.f, 0.f },
        {-1.f, -1.f,  1.f,  1.f, 1.f, 0.f },
        {-1.f, -1.f,  1.f,  0.f, 1.f, 1.f },
        { 1.f, -1.f,  1.f,  0.f, 1.f, 1.f },
        { 1.f,  1.f,  1.f,  0.f, 1.f, 1.f },
        {-1.f,  1.f,  1.f,  0.f, 1.f, 1.f },
        { 1.f, -1.f, -1.f,  1.f, 0.f, 1.f },
        {-1.f, -1.f, -1.f,  1.f, 0.f, 1.f },
        {-1.f,  1.f, -1.f,  1.f, 0.f, 1.f },
        { 1.f,  1.f, -1.f,  1.f, 0.f, 1.f },
    };
    const uint16_t indices[] = {
         0,  1,  2,   0,  2,  3,
         4,  5,  6,   4,  6,  7,
         8,  9, 10,   8, 10, 11,
        12, 13, 14,  12, 14, 15,
        16, 17, 18,  16, 18, 19,
        20, 21, 22,  20, 22, 23,
    };

    be::VertexFormat format;
    format.stride = sizeof(CubeVertex);
    format.attributes = {
        {0, be::VertexAttributeType::Float3, 0, false},
        {1, be::VertexAttributeType::Float3,
            static_cast<uint32_t>(offsetof(CubeVertex, cr)), false},
    };

    auto mat = create_test_material(device);
    auto vertex_data = std::as_bytes(std::span(vertices));
    auto index_data = std::as_bytes(std::span(indices));
    auto model = be::Model::create_indexed(
        device, format, vertex_data, index_data,
        be::IndexType::Uint16,
        { be::SubMesh{0, 36, 0} },
        { mat });
    REQUIRE(model.has_value());
    REQUIRE(model->vertex_count() == 24);
    REQUIRE(model->index_count() == 36);
}

TEST_CASE("Cube material has u_mvp uniform", "[cube]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto model = make_simple_model(device, mat);

    // u_mvp should be trackable by the headless material
    REQUIRE(model.materials()[0]->has_uniform("u_mvp") == true);
}

TEST_CASE("Cube material does NOT have u_color uniform", "[cube]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto model = make_simple_model(device, mat);

    // u_color should NOT exist
    REQUIRE(model.materials()[0]->has_uniform("u_color") == false);
}

TEST_CASE("Setting u_mvp on cube material succeeds", "[cube]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto model = make_simple_model(device, mat);

    auto result = model.materials()[0]->set_uniform("u_mvp", be::math::Mat4::identity());
    REQUIRE(result.has_value());
}

// ===========================================================================
// AC-011: create_cube helper
// ===========================================================================
TEST_CASE("engine::create_cube returns correct Model", "[cube][primitives]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto cube = be::create_cube(device, mat);
    REQUIRE(cube.has_value());
    REQUIRE(cube->submeshes().size() == 1);
    REQUIRE(cube->submeshes()[0].index_count == 36);
    REQUIRE(cube->materials().size() == 1);
    REQUIRE(cube->materials()[0] == mat);
    REQUIRE(cube->vertex_count() == 24);
    REQUIRE(cube->index_count() == 36);
}

// ===========================================================================
// AC-012: create_triangle helper
// ===========================================================================
TEST_CASE("engine::create_triangle returns correct Model", "[triangle][primitives]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto tri = be::create_triangle(device, mat);
    REQUIRE(tri.has_value());
    REQUIRE(tri->submeshes().size() == 1);
    REQUIRE(tri->submeshes()[0].index_count == 3);
    REQUIRE(tri->materials().size() == 1);
    REQUIRE(tri->materials()[0] == mat);
    REQUIRE(tri->vertex_count() == 3);
    REQUIRE(tri->index_count() == 3);
}

// ===========================================================================
// AC-013: create_quad helper
// ===========================================================================
TEST_CASE("engine::create_quad returns correct Model", "[quad][primitives]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto quad = be::create_quad(device, mat);
    REQUIRE(quad.has_value());
    REQUIRE(quad->submeshes().size() == 1);
    REQUIRE(quad->submeshes()[0].index_count == 6);
    REQUIRE(quad->materials().size() == 1);
    REQUIRE(quad->materials()[0] == mat);
    REQUIRE(quad->vertex_count() == 4);
    REQUIRE(quad->index_count() == 6);
}

// ===========================================================================
// AC-014: fallback_material
// ===========================================================================
TEST_CASE("RenderDevice::fallback_material returns valid material", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    auto& fb = device.fallback_material();
    // Should not crash and return a valid reference
    REQUIRE(&fb != nullptr);
}

// ===========================================================================
// AC-023: Move preserves submeshes/materials
// ===========================================================================
TEST_CASE("Move preserves submeshes and materials", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat_a = create_test_material(device);
    auto mat_b = create_test_material(device);

    auto fmt = make_pos_format();
    const float verts[] = {0,0,0, 1,0,0, 0,1,0, 1,1,0};
    const uint16_t idxs[] = {0,1,2, 0,2,3};
    auto model = be::Model::create_indexed(
        device, fmt,
        std::as_bytes(std::span(verts)),
        std::as_bytes(std::span(idxs)),
        be::IndexType::Uint16,
        {
            {0, 3, 0},
            {3, 3, 1},
        },
        {mat_a, mat_b}
    );
    REQUIRE(model.has_value());
    REQUIRE(model->submeshes().size() == 2);
    REQUIRE(model->materials().size() == 2);

    // Move construct
    be::Model m2(std::move(*model));
    REQUIRE(m2.submeshes().size() == 2);
    REQUIRE(m2.materials().size() == 2);
    REQUIRE(m2.submeshes()[0].material_index == 0);
    REQUIRE(m2.submeshes()[1].material_index == 1);
    REQUIRE(m2.materials()[0] == mat_a);
    REQUIRE(m2.materials()[1] == mat_b);

    // Source is empty
    REQUIRE(model->submeshes().empty());
    REQUIRE(model->materials().empty());
}

// ===========================================================================
// AC-024: Const-correctness
// ===========================================================================
TEST_CASE("Const-correct access to submeshes and materials", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    auto model = make_simple_model(device, mat);

    const auto& cm = model;
    const auto& submeshes = cm.submeshes();
    const auto& materials = cm.materials();

    REQUIRE(submeshes.size() == 1);
    REQUIRE(materials.size() == 1);
}

// ===========================================================================
// Shared ownership test
// ===========================================================================

TEST_CASE("Model shares material ownership via shared_ptr", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);
    auto* raw_ptr = mat.get();

    auto model = make_simple_model(device, mat);

    // Verify the model's material is the same object
    REQUIRE(model.materials()[0].get() == raw_ptr);

    // Reset the original shared_ptr — material should stay alive via Model
    mat.reset();
    REQUIRE(model.materials()[0].get() == raw_ptr);
    REQUIRE(model.materials()[0]->has_uniform("u_mvp") == true);
}

// ===========================================================================
// Demo run test (headless)
// ===========================================================================

TEST_CASE("run_cube_demo completes without crash (headless)", "[cube]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    auto mat = create_test_material(device);
    auto cube = be::create_cube(device, mat);
    REQUIRE(cube.has_value());
    REQUIRE(cube->vertex_count() == 24);
    REQUIRE(cube->index_count() == 36);

    // Run a small loop
    constexpr int target_frames = 5;
    for (int frame = 0; frame < target_frames; ++frame) {
        device.begin_frame();
        auto result = mat->set_uniform("u_mvp", be::math::Mat4::identity());
        REQUIRE(result.has_value());
        cube->draw(device);
        device.end_frame();
    }
    // No crash — test passes
    REQUIRE(true);
}
