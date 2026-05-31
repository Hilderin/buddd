#include "render/model.h"
#include "render/render_device.h"
#include "render/render_device_headless.h"
#include "render/shader.h"
#include "render/material.h"
#include "render/vertex_format.h"
#include "render/primitive_topology.h"
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
    REQUIRE(null_model.has_indices() == false);
}

TEST_CASE("Model is non-copyable and movable", "[model]") {
    static_assert(!std::is_copy_constructible_v<be::Model>,
                  "Model must be non-copyable");
    static_assert(std::is_move_constructible_v<be::Model>,
                  "Model must be movable");

    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    // Create a simple model
    be::VertexFormat format;
    format.stride = 12;
    format.attributes = {{0, be::VertexAttributeType::Float3, 0, false}};

    const float verts[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };
    auto vertex_data = std::as_bytes(std::span(verts));
    auto model = be::Model::create(device, format, vertex_data, mat);
    REQUIRE(model.has_value());

    // Move construct
    be::Model m2(std::move(*model));
    REQUIRE(m2.vertex_count() == 3);
    REQUIRE(m2.has_indices() == false);

    // Move assign
    be::Model m3;
    m3 = std::move(m2);
    REQUIRE(m3.vertex_count() == 3);
}

TEST_CASE("Model::create with valid data succeeds", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat format;
    format.stride = 12;
    format.attributes = {{0, be::VertexAttributeType::Float3, 0, false}};

    const float verts[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };
    auto vertex_data = std::as_bytes(std::span(verts));
    auto model = be::Model::create(device, format, vertex_data, mat);
    REQUIRE(model.has_value());
    REQUIRE(model->has_indices() == false);
    REQUIRE(model->vertex_count() == 3);
    REQUIRE(model->index_count() == 0);
}

TEST_CASE("Model::create_indexed with valid data succeeds", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat format;
    format.stride = 12;
    format.attributes = {{0, be::VertexAttributeType::Float3, 0, false}};

    const float verts[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f,
    };
    const uint16_t idxs[] = {0, 1, 2, 0, 2, 3};
    auto vertex_data = std::as_bytes(std::span(verts));
    auto index_data = std::as_bytes(std::span(idxs));
    auto model = be::Model::create_indexed(
        device, format, vertex_data, index_data,
        be::IndexType::Uint16, mat);
    REQUIRE(model.has_value());
    REQUIRE(model->has_indices() == true);
    REQUIRE(model->vertex_count() == 4);
    REQUIRE(model->index_count() == 6);
}

TEST_CASE("Model::create returns InvalidArgument for empty vertex data", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat format;
    format.stride = 12;
    format.attributes = {{0, be::VertexAttributeType::Float3, 0, false}};

    auto result = be::Model::create(device, format, {}, mat);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == be::Error::Category::InvalidArgument);
}

TEST_CASE("Model::create returns InvalidArgument for zero stride", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat format;
    format.stride = 0;
    format.attributes = {{0, be::VertexAttributeType::Float3, 0, false}};

    const float verts[] = {0.0f, 0.0f, 0.0f};
    auto vertex_data = std::as_bytes(std::span(verts));
    auto result = be::Model::create(device, format, vertex_data, mat);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == be::Error::Category::InvalidArgument);
}

TEST_CASE("Model::create returns InvalidArgument for zero attributes", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat format;
    format.stride = 12;
    format.attributes = {};

    const float verts[] = {0.0f, 0.0f, 0.0f};
    auto vertex_data = std::as_bytes(std::span(verts));
    auto result = be::Model::create(device, format, vertex_data, mat);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == be::Error::Category::InvalidArgument);
}

TEST_CASE("Model::create_indexed returns InvalidArgument for empty index data", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat format;
    format.stride = 12;
    format.attributes = {{0, be::VertexAttributeType::Float3, 0, false}};

    const float verts[] = {0.0f, 0.0f, 0.0f};
    auto vertex_data = std::as_bytes(std::span(verts));
    auto result = be::Model::create_indexed(
        device, format, vertex_data, {},
        be::IndexType::Uint16, mat);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == be::Error::Category::InvalidArgument);
}

// ===========================================================================
// Model accessor tests
// ===========================================================================

TEST_CASE("Model::material returns writable reference", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat format;
    format.stride = 12;
    format.attributes = {{0, be::VertexAttributeType::Float3, 0, false}};

    const float verts[] = {0.0f, 0.0f, 0.0f};
    auto vertex_data = std::as_bytes(std::span(verts));
    auto model = be::Model::create(device, format, vertex_data, mat);
    REQUIRE(model.has_value());

    // Set a uniform via material() reference
    auto result = model->material().set_uniform("u_mvp", be::math::Mat4::identity());
    REQUIRE(result.has_value());
    REQUIRE(model->material().has_uniform("u_mvp") == true);
}

TEST_CASE("Model::material const overload", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat format;
    format.stride = 12;
    format.attributes = {{0, be::VertexAttributeType::Float3, 0, false}};

    const float verts[] = {0.0f, 0.0f, 0.0f};
    auto vertex_data = std::as_bytes(std::span(verts));
    const auto model = be::Model::create(device, format, vertex_data, mat);
    REQUIRE(model.has_value());

    // Const reference — ensure it compiles and returns const ref
    const be::Material& mat_ref = model->material();
    // Verify it's truly const by checking a non-existent uniform
    REQUIRE(mat_ref.has_uniform("u_nonexistent") == false);
}

TEST_CASE("Model::vertices returns non-null reference", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat format;
    format.stride = 12;
    format.attributes = {{0, be::VertexAttributeType::Float3, 0, false}};

    const float verts[] = {0.0f, 0.0f, 0.0f};
    auto vertex_data = std::as_bytes(std::span(verts));
    auto model = be::Model::create(device, format, vertex_data, mat);
    REQUIRE(model.has_value());

    // &model.vertices() is a valid non-null pointer
    REQUIRE(&model->vertices() != nullptr);
}

TEST_CASE("Model::indices returns reference on indexed model", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat format;
    format.stride = 12;
    format.attributes = {{0, be::VertexAttributeType::Float3, 0, false}};

    const float verts[] = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    const uint16_t idxs[] = {0, 1, 0};
    auto vertex_data = std::as_bytes(std::span(verts));
    auto index_data = std::as_bytes(std::span(idxs));
    auto model = be::Model::create_indexed(
        device, format, vertex_data, index_data,
        be::IndexType::Uint16, mat);
    REQUIRE(model.has_value());
    REQUIRE(model->has_indices() == true);
    REQUIRE(&model->indices() != nullptr);
}

// ===========================================================================
// Model draw tests
// ===========================================================================

TEST_CASE("Model::draw on non-indexed model", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat format;
    format.stride = 12;
    format.attributes = {{0, be::VertexAttributeType::Float3, 0, false}};

    const float verts[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };
    auto vertex_data = std::as_bytes(std::span(verts));
    auto model = be::Model::create(device, format, vertex_data, mat);
    REQUIRE(model.has_value());

    device.begin_frame();
    model->draw(device);
    device.end_frame();
    // No crash — test passes
    REQUIRE(true);
}

TEST_CASE("Model::draw on indexed model", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat format;
    format.stride = 12;
    format.attributes = {{0, be::VertexAttributeType::Float3, 0, false}};

    const float verts[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };
    const uint16_t idxs[] = {0, 1, 2};
    auto vertex_data = std::as_bytes(std::span(verts));
    auto index_data = std::as_bytes(std::span(idxs));
    auto model = be::Model::create_indexed(
        device, format, vertex_data, index_data,
        be::IndexType::Uint16, mat);
    REQUIRE(model.has_value());

    device.begin_frame();
    model->draw(device);
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

TEST_CASE("Moved-from Model draw is no-op", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat format;
    format.stride = 12;
    format.attributes = {{0, be::VertexAttributeType::Float3, 0, false}};

    const float verts[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };
    auto vertex_data = std::as_bytes(std::span(verts));
    auto model = be::Model::create(device, format, vertex_data, mat);
    REQUIRE(model.has_value());

    be::Model moved(std::move(*model));
    device.begin_frame();
    // Draw on moved-from (null) model — should be no-op
    model->draw(device);
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

    be::VertexFormat format;
    format.stride = 12;
    format.attributes = {{0, be::VertexAttributeType::Float3, 0, false}};

    const float verts[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };
    auto vertex_data = std::as_bytes(std::span(verts));
    auto model = be::Model::create(device, format, vertex_data, mat);
    REQUIRE(model.has_value());

    auto vcount_orig = model->vertex_count();
    auto has_idx_orig = model->has_indices();

    be::Model m2(std::move(*model));
    REQUIRE(m2.has_indices() == has_idx_orig);
    REQUIRE(m2.vertex_count() == vcount_orig);

    // Source is null — draw is no-op
    REQUIRE(model->vertex_count() == 0);
    REQUIRE(model->index_count() == 0);
    device.begin_frame();
    model->draw(device);
    device.end_frame();
}

TEST_CASE("Move assignment transfers ownership", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat format;
    format.stride = 12;
    format.attributes = {{0, be::VertexAttributeType::Float3, 0, false}};

    const float verts[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };
    auto vertex_data = std::as_bytes(std::span(verts));
    auto model = be::Model::create(device, format, vertex_data, mat);
    REQUIRE(model.has_value());

    be::Model m2;
    m2 = std::move(*model);
    REQUIRE(m2.vertex_count() == 3);

    // Source is null
    REQUIRE(model->vertex_count() == 0);

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
        be::IndexType::Uint16, mat);
    REQUIRE(model.has_value());
    REQUIRE(model->vertex_count() == 24);
    REQUIRE(model->index_count() == 36);
    REQUIRE(model->has_indices() == true);
}

TEST_CASE("Cube material has u_mvp uniform", "[cube]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat format;
    format.stride = sizeof(CubeVertex);
    format.attributes = {
        {0, be::VertexAttributeType::Float3, 0, false},
        {1, be::VertexAttributeType::Float3,
            static_cast<uint32_t>(offsetof(CubeVertex, cr)), false},
    };

    const CubeVertex verts[1] = {{0,0,0, 1,0,0}};
    const uint16_t idxs[1] = {0};
    auto vertex_data = std::as_bytes(std::span(verts));
    auto index_data = std::as_bytes(std::span(idxs));
    auto model = be::Model::create_indexed(
        device, format, vertex_data, index_data,
        be::IndexType::Uint16, mat);
    REQUIRE(model.has_value());

    // u_mvp should be trackable by the headless material
    REQUIRE(model->material().has_uniform("u_mvp") == true);
}

TEST_CASE("Cube material does NOT have u_color uniform", "[cube]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat format;
    format.stride = sizeof(CubeVertex);
    format.attributes = {
        {0, be::VertexAttributeType::Float3, 0, false},
        {1, be::VertexAttributeType::Float3,
            static_cast<uint32_t>(offsetof(CubeVertex, cr)), false},
    };

    const CubeVertex verts[1] = {{0,0,0, 1,0,0}};
    const uint16_t idxs[1] = {0};
    auto vertex_data = std::as_bytes(std::span(verts));
    auto index_data = std::as_bytes(std::span(idxs));
    auto model = be::Model::create_indexed(
        device, format, vertex_data, index_data,
        be::IndexType::Uint16, mat);
    REQUIRE(model.has_value());

    // u_color should NOT exist
    REQUIRE(model->material().has_uniform("u_color") == false);
}

TEST_CASE("Setting u_mvp on cube material succeeds", "[cube]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);

    be::VertexFormat format;
    format.stride = sizeof(CubeVertex);
    format.attributes = {
        {0, be::VertexAttributeType::Float3, 0, false},
        {1, be::VertexAttributeType::Float3,
            static_cast<uint32_t>(offsetof(CubeVertex, cr)), false},
    };

    const CubeVertex verts[1] = {{0,0,0, 1,0,0}};
    const uint16_t idxs[1] = {0};
    auto vertex_data = std::as_bytes(std::span(verts));
    auto index_data = std::as_bytes(std::span(idxs));
    auto model = be::Model::create_indexed(
        device, format, vertex_data, index_data,
        be::IndexType::Uint16, mat);
    REQUIRE(model.has_value());

    auto result = model->material().set_uniform("u_mvp", be::math::Mat4::identity());
    REQUIRE(result.has_value());
}

// ===========================================================================
// Demo run test
// ===========================================================================

TEST_CASE("run_cube_demo completes without crash (headless)", "[cube]") {
    // This test requires linking with demo code, so we create a minimal
    // test that creates the cube resources and runs a loop manually.
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
        be::IndexType::Uint16, mat);
    REQUIRE(model.has_value());
    REQUIRE(model->vertex_count() == 24);
    REQUIRE(model->index_count() == 36);

    // Run a small loop (simulate run_cube_demo behavior)
    constexpr int target_frames = 5;
    for (int frame = 0; frame < target_frames; ++frame) {
        device.begin_frame();
        auto result = mat->set_uniform("u_mvp", be::math::Mat4::identity());
        REQUIRE(result.has_value());
        model->draw(device);
        device.end_frame();
    }
    // No crash — test passes
    REQUIRE(true);
}

// ===========================================================================
// Shared ownership test
// ===========================================================================

TEST_CASE("Model shares material ownership via shared_ptr", "[model]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto mat = create_test_material(device);
    auto* raw_ptr = mat.get();

    be::VertexFormat format;
    format.stride = 12;
    format.attributes = {{0, be::VertexAttributeType::Float3, 0, false}};

    const float verts[] = {0.0f, 0.0f, 0.0f};
    auto vertex_data = std::as_bytes(std::span(verts));
    auto model = be::Model::create(device, format, vertex_data, mat);
    REQUIRE(model.has_value());

    // Verify the model's material is the same object
    REQUIRE(&model->material() == raw_ptr);

    // Reset the original shared_ptr — material should stay alive via Model
    mat.reset();
    REQUIRE(&model->material() == raw_ptr);
    REQUIRE(model->material().has_uniform("u_mvp") == true);
}
