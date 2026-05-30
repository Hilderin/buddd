#include "version.h"
#include "error.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"
#include "render/shader.h"
#include "render/material.h"
#include "render/vertex_buffer.h"
#include "render/vertex_format.h"
#include "render/primitive_topology.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>
#include <thread>


namespace be = buddd::engine;

// ============================================================================
// Shared setup: create shaders, material, and vertex buffer for a triangle
// ============================================================================
static auto setup_triangle(
    be::RenderDevice& device
) -> std::pair<
        std::unique_ptr<be::Material>,
        std::unique_ptr<be::VertexBuffer>>
{
    constexpr std::string_view vertex_source = R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec3 a_color;
        out vec3 v_color;
        void main() {
            gl_Position = vec4(a_position, 1.0);
            v_color = a_color;
        }
    )";

    constexpr std::string_view fragment_source = R"(
        #version 450 core
        in vec3 v_color;
        out vec4 frag_color;
        void main() {
            frag_color = vec4(v_color, 1.0);
        }
    )";

    auto vs = device.create_shader(be::ShaderType::Vertex, vertex_source);
    if (!vs) {
        std::fprintf(stderr, "FATAL: %s\n", be::to_string(vs.error()).c_str());
        std::exit(EXIT_FAILURE);
    }

    auto fs = device.create_shader(be::ShaderType::Fragment, fragment_source);
    if (!fs) {
        std::fprintf(stderr, "FATAL: %s\n", be::to_string(fs.error()).c_str());
        std::exit(EXIT_FAILURE);
    }

    auto material = device.create_material(std::move(*vs), std::move(*fs));
    if (!material) {
        std::fprintf(stderr, "FATAL: %s\n", be::to_string(material.error()).c_str());
        std::exit(EXIT_FAILURE);
    }

    // Create vertex buffer: triangle with position (Float3) and color (Float3)
    struct Vertex { float x, y, z, r, g, b; };
    const Vertex vertices[] = {
        { 0.0f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f },  // top, red
        {-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f },  // bottom-left, green
        { 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f },  // bottom-right, blue
    };

    be::VertexFormat format;
    format.stride = sizeof(Vertex);
    format.attributes = {
        {0, be::VertexAttributeType::Float3, static_cast<uint32_t>(offsetof(Vertex, x)), false},
        {1, be::VertexAttributeType::Float3, static_cast<uint32_t>(offsetof(Vertex, r)), false},
    };

    auto vertex_data = std::as_bytes(std::span(vertices));
    auto vb = device.create_vertex_buffer(format, vertex_data);
    if (!vb) {
        std::fprintf(stderr, "FATAL: %s\n", be::to_string(vb.error()).c_str());
        std::exit(EXIT_FAILURE);
    }

    return {std::move(*material), std::move(*vb)};
}

// ============================================================================
// Test mode: automated 120-frame render
// ============================================================================
static auto run_test_mode() -> int {
    auto platform = be::Platform::create(be::Backend::SDL3);
    if (!platform) {
        std::cerr << "Failed to create platform: "
                  << be::to_string(platform.error()) << "\n";
        return EXIT_FAILURE;
    }

    auto window = (*platform)->create_window({
        .title = "Buddd Engine — Render Test",
        .width = 800,
        .height = 600
    });
    if (!window) {
        std::cerr << "Failed to create window: "
                  << be::to_string(window.error()) << "\n";
        return EXIT_FAILURE;
    }

    auto device = be::RenderDevice::create(**window);
    if (!device) {
        std::cerr << "Failed to create render device: "
                  << be::to_string(device.error()) << "\n";
        return EXIT_FAILURE;
    }

    auto [material, vb] = setup_triangle(**device);

    // Render loop: ~120 frames at 60 FPS (~2 seconds)
    constexpr int target_frames = 120;
    constexpr auto frame_duration = std::chrono::milliseconds(16); // ~60 FPS

    std::cerr << "Render test started: " << target_frames << " frames\n";

    for (int frame = 0; frame < target_frames; ++frame) {
        auto frame_start = std::chrono::steady_clock::now();

        if (!(*platform)->poll_events()) {
            std::cerr << "Render test aborted by user (frame " << frame << ")\n";
            return EXIT_SUCCESS;
        }

        (*device)->begin_frame();
        (*device)->draw(
            be::PrimitiveTopology::Triangles,
            *vb, *material, 3);
        (*device)->end_frame();

        // Frame rate limiting
        auto elapsed = std::chrono::steady_clock::now() - frame_start;
        if (elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - elapsed);
        }
    }

    std::cerr << "Render test complete: " << target_frames << " frames rendered\n";
    return EXIT_SUCCESS;
}

// ============================================================================
// Default interactive mode: render until window is closed
// ============================================================================
static auto run_interactive() -> int {
    auto platform = be::Platform::create(be::Backend::SDL3);
    if (!platform) {
        std::cerr << "FATAL: " << be::to_string(platform.error()) << "\n";
        return EXIT_FAILURE;
    }

    auto window = (*platform)->create_window({
        .title = "Buddd Engine",
        .width = 1024,
        .height = 768
    });
    if (!window) {
        std::cerr << "FATAL: " << be::to_string(window.error()) << "\n";
        return EXIT_FAILURE;
    }

    std::printf("Window opened: %dx%d\n", (*window)->width(), (*window)->height());

    auto device = be::RenderDevice::create(**window);
    if (!device) {
        std::cerr << "FATAL: " << be::to_string(device.error()) << "\n";
        return EXIT_FAILURE;
    }

    auto [material, vb] = setup_triangle(**device);

    // Render loop: runs until the window is closed by the user
    while ((*platform)->poll_events()) {
        (*device)->begin_frame();
        (*device)->draw(
            be::PrimitiveTopology::Triangles,
            *vb, *material, 3);
        (*device)->end_frame();
    }

    std::printf("Window closed, shutting down.\n");
    return EXIT_SUCCESS;
}

// ============================================================================
// Entry point
// ============================================================================
auto main(int argc, char* argv[]) -> int {
    // --version mode (preserved from existing behavior)
    if (argc == 2 && std::string_view{argv[1]} == "--version") {
        std::printf("buddd %s\n", be::version().data());
        return 0;
    }

    // --test mode: automated 120-frame render then exit
    if (argc == 2 && std::string_view{argv[1]} == "--test") {
        return run_test_mode();
    }

    // Default mode: render until window is closed by the user
    return run_interactive();
}
