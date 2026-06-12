#include "engine_service.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"
#include "render/texture.h"
#include "render/texture_headless.h"
#include "render/material_headless.h"
#include "image/image_buffer.h"
#include "image/image.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

using namespace buddd::engine;
using Catch::Approx;

namespace {

    constexpr float TOL = 1e-5f;

    auto make_headless_engine() -> std::unique_ptr<EngineService> {
        auto engine = EngineService::create(
            Backend::Headless,
            WindowConfig{.title = "Test", .width = 800, .height = 600});
        REQUIRE(engine.has_value());
        return std::move(*engine);
    }

    /// Creates a 2x2 RGBA checkerboard image with known pixel values.
    auto make_checkerboard_image(int channels) -> Image {
        ImageBuffer buffer;
        buffer.width = 2;
        buffer.height = 2;
        buffer.channels = channels;
        buffer.data.resize(static_cast<size_t>(buffer.width) * buffer.height * channels);

        // Fill with known pattern
        for (int y = 0; y < buffer.height; ++y) {
            for (int x = 0; x < buffer.width; ++x) {
                size_t idx = static_cast<size_t>((y * buffer.width + x) * channels);
                for (int c = 0; c < channels; ++c) {
                    // Create a distinctive pattern: each pixel unique
                    buffer.data[idx + static_cast<size_t>(c)] = static_cast<std::byte>(
                        static_cast<unsigned char>((x + y * buffer.width + c) * 64 + 32));
                }
            }
        }

        auto image = Image::create(buffer);
        REQUIRE(image.has_value());
        return std::move(*image);
    }

    /// Creates an Image with custom dimensions and channels, empty data.
    auto make_empty_data_image(int width, int height, int channels) -> Image {
        ImageBuffer buffer;
        buffer.width = width;
        buffer.height = height;
        buffer.channels = channels;
        // data remains empty
        auto image = Image::create(buffer);
        REQUIRE(image.has_value());
        return std::move(*image);
    }

} // anonymous namespace

// ===========================================================================
// Test 1: TextureHeadless stores correct dimensions and channels
// ===========================================================================
TEST_CASE("TextureHeadless stores correct dimensions and channels", "[texture][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    auto image = make_checkerboard_image(4);
    auto texture = device.create_texture(image);
    REQUIRE(texture.has_value());

    REQUIRE((*texture)->width() == 2);
    REQUIRE((*texture)->height() == 2);
    REQUIRE((*texture)->channels() == 4);
}

// ===========================================================================
// Test 2: TextureHeadless stores pixel data correctly
// ===========================================================================
TEST_CASE("TextureHeadless stores pixel data correctly", "[texture][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    auto image = make_checkerboard_image(4);
    auto texture = device.create_texture(image);
    REQUIRE(texture.has_value());

    auto* headless_tex = dynamic_cast<TextureHeadless*>(texture->get());
    REQUIRE(headless_tex != nullptr);

    // Data should match the image data
    auto& stored_data = headless_tex->data();
    REQUIRE(stored_data.size() == image.data().size());
    for (size_t i = 0; i < stored_data.size(); ++i) {
        REQUIRE(stored_data[i] == image.data()[i]);
    }
}

// ===========================================================================
// Test 3: set_texture and get_texture on Headless material
// ===========================================================================
TEST_CASE("set_texture and get_texture on Headless material", "[texture][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    // Create shaders with a sampler2D uniform
    auto vs = device.create_shader(ShaderType::Vertex, R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        void main() {
            gl_Position = vec4(a_position, 1.0);
        }
    )");
    REQUIRE(vs.has_value());

    auto fs = device.create_shader(ShaderType::Fragment, R"(
        #version 450 core
        out vec4 frag_color;
        uniform sampler2D u_tex;
        void main() {
            frag_color = vec4(1.0);
        }
    )");
    REQUIRE(fs.has_value());

    auto mat = device.create_material(std::move(*vs), std::move(*fs));
    REQUIRE(mat.has_value());

    auto* headless_mat = dynamic_cast<MaterialHeadless*>(mat->get());
    REQUIRE(headless_mat != nullptr);

    // Create texture
    auto image = make_checkerboard_image(4);
    auto texture = device.create_texture(image);
    REQUIRE(texture.has_value());

    std::shared_ptr<Texture> shared_tex(std::move(*texture));

    // Set texture
    auto set_result = mat->get()->set_texture("u_tex", shared_tex);
    REQUIRE(set_result.has_value());

    // Get texture
    auto retrieved = headless_mat->get_texture("u_tex");
    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->get() == shared_tex.get());
}

// ===========================================================================
// Test 4: has_texture returns true for valid uniform name
// ===========================================================================
TEST_CASE("has_texture returns correct results", "[texture][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    // Create shaders with a sampler2D uniform
    auto vs = device.create_shader(ShaderType::Vertex, R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        void main() {
            gl_Position = vec4(a_position, 1.0);
        }
    )");
    REQUIRE(vs.has_value());

    auto fs = device.create_shader(ShaderType::Fragment, R"(
        #version 450 core
        out vec4 frag_color;
        uniform sampler2D u_tex;
        void main() {
            frag_color = vec4(1.0);
        }
    )");
    REQUIRE(fs.has_value());

    auto mat = device.create_material(std::move(*vs), std::move(*fs));
    REQUIRE(mat.has_value());

    // Create texture
    auto image = make_checkerboard_image(4);
    auto texture = device.create_texture(image);
    REQUIRE(texture.has_value());

    std::shared_ptr<Texture> shared_tex(std::move(*texture));

    // Before set_texture, has_texture should check name existence
    // (the headless implementation checks known_uniforms_ or texture_values_)
    // Due to how headless create_material works, the uniform name is parsed
    // from the shader source, so "u_tex" should already be known.
    REQUIRE(mat->get()->has_texture("u_tex"));

    // Set texture
    auto set_result = mat->get()->set_texture("u_tex", shared_tex);
    REQUIRE(set_result.has_value());

    // After set_texture, has_texture should return true
    REQUIRE(mat->get()->has_texture("u_tex"));

    // Non-existent name should return false
    REQUIRE_FALSE(mat->get()->has_texture("nonexistent"));
}

// ===========================================================================
// Test 5: bind() is no-op on Headless
// ===========================================================================
TEST_CASE("bind() is no-op on Headless", "[texture][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    // Create shaders with a sampler2D uniform
    auto vs = device.create_shader(ShaderType::Vertex, R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        void main() {
            gl_Position = vec4(a_position, 1.0);
        }
    )");
    REQUIRE(vs.has_value());

    auto fs = device.create_shader(ShaderType::Fragment, R"(
        #version 450 core
        out vec4 frag_color;
        uniform sampler2D u_tex;
        void main() {
            frag_color = vec4(1.0);
        }
    )");
    REQUIRE(fs.has_value());

    auto mat = device.create_material(std::move(*vs), std::move(*fs));
    REQUIRE(mat.has_value());

    auto* headless_mat = dynamic_cast<MaterialHeadless*>(mat->get());
    REQUIRE(headless_mat != nullptr);

    // Create texture and set it
    auto image = make_checkerboard_image(4);
    auto texture = device.create_texture(image);
    REQUIRE(texture.has_value());

    std::shared_ptr<Texture> shared_tex(std::move(*texture));
    auto set_result = mat->get()->set_texture("u_tex", shared_tex);
    REQUIRE(set_result.has_value());

    // Call bind() — should be no-op
    REQUIRE_NOTHROW(mat->get()->bind());

    // After bind(), the texture should still be set
    auto retrieved = headless_mat->get_texture("u_tex");
    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->get() == shared_tex.get());
}

// ===========================================================================
// Test 6: set_texture with null shared_ptr returns InvalidArgument
// ===========================================================================
TEST_CASE("set_texture with null shared_ptr returns InvalidArgument", "[texture][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    auto vs = device.create_shader(ShaderType::Vertex, R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        void main() {
            gl_Position = vec4(a_position, 1.0);
        }
    )");
    REQUIRE(vs.has_value());

    auto fs = device.create_shader(ShaderType::Fragment, R"(
        #version 450 core
        out vec4 frag_color;
        uniform sampler2D u_tex;
        void main() {
            frag_color = vec4(1.0);
        }
    )");
    REQUIRE(fs.has_value());

    auto mat = device.create_material(std::move(*vs), std::move(*fs));
    REQUIRE(mat.has_value());

    auto result = mat->get()->set_texture("u_tex", nullptr);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::InvalidArgument);
}

// ===========================================================================
// Test 7: set_texture with unknown name returns UniformNotFound
// ===========================================================================
TEST_CASE("set_texture with unknown name returns UniformNotFound", "[texture][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    // Create a material with NO known uniforms (empty shader)
    auto vs = device.create_shader(ShaderType::Vertex, R"(
        #version 450 core
        void main() {}
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

    auto image = make_checkerboard_image(4);
    auto texture = device.create_texture(image);
    REQUIRE(texture.has_value());

    std::shared_ptr<Texture> shared_tex(std::move(*texture));

    auto result = mat->get()->set_texture("u_nonexistent", shared_tex);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::UniformNotFound);
}

// ===========================================================================
// Test 8: create_texture with zero width returns InvalidArgument
// Note: Image::create validates dimensions first, so we verify the chain
// rejects invalid images with InvalidArgument.
// ===========================================================================
TEST_CASE("create_texture with zero width returns InvalidArgument", "[texture][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    ImageBuffer buffer;
    buffer.width = 0;
    buffer.height = 2;
    buffer.channels = 4;
    buffer.data.resize(static_cast<size_t>(buffer.width) * buffer.height * buffer.channels);

    auto image = Image::create(buffer);
    // Image::create catches zero width first
    REQUIRE_FALSE(image.has_value());
    REQUIRE(image.error().category == Error::Category::InvalidArgument);
}

// ===========================================================================
// Test 9: create_texture with zero height returns InvalidArgument
// ===========================================================================
TEST_CASE("create_texture with zero height returns InvalidArgument", "[texture][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    ImageBuffer buffer;
    buffer.width = 2;
    buffer.height = 0;
    buffer.channels = 4;
    buffer.data.resize(static_cast<size_t>(buffer.width) * buffer.height * buffer.channels);

    auto image = Image::create(buffer);
    REQUIRE_FALSE(image.has_value());
    REQUIRE(image.error().category == Error::Category::InvalidArgument);
}

// ===========================================================================
// Test 10: create_texture with empty data returns InvalidArgument
// ===========================================================================
TEST_CASE("create_texture with empty data returns InvalidArgument", "[texture][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    ImageBuffer buffer;
    buffer.width = 2;
    buffer.height = 2;
    buffer.channels = 4;
    // Data is empty — Image::create will catch this

    auto image = Image::create(buffer);
    REQUIRE_FALSE(image.has_value());
    REQUIRE(image.error().category == Error::Category::InvalidArgument);
}

// ===========================================================================
// Test 11: create_texture with 2 channels returns Unsupported
// ===========================================================================
TEST_CASE("create_texture with 2 channels returns Unsupported", "[texture][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    auto image = make_checkerboard_image(2);
    auto result = device.create_texture(image);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::Unsupported);
}

// ===========================================================================
// Test 12: create_texture with >4 channels returns Unsupported
// ===========================================================================
TEST_CASE("create_texture with >4 channels returns Unsupported", "[texture][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    auto image = make_checkerboard_image(5);
    auto result = device.create_texture(image);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::Unsupported);
}

// ===========================================================================
// Test 13: set_uniform still validates existence
// ===========================================================================
TEST_CASE("set_uniform still validates existence", "[texture][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    // Create a material with no known uniforms
    auto vs = device.create_shader(ShaderType::Vertex, R"(
        #version 450 core
        void main() {}
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

    auto result = mat->get()->set_uniform("u_nonexistent", 1.0f);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::UniformNotFound);
}
