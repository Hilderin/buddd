#include "app.h"
#include "app_config.h"
#include "apps/asset_demo_app.h"
#include "apps/hot_reload_app.h"
#include "apps/cube_app.h"
#include "apps/cube_scene_app.h"
#include "apps/free_camera_app.h"
#include "apps/phong_app.h"
#include "apps/run_app.h"
#include "apps/textured_cube_app.h"
#include "apps/triangle_app.h"

#include "commands/help_command.h"
#include "commands/version_command.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string_view>

namespace bc = buddd::cmd;
namespace be = buddd::engine;

auto main(int argc, char* argv[]) -> int {
    if (argc <= 0) return EXIT_FAILURE;  // defensive

    // No positional argument -> default to run with no scene
    if (argc < 2 || argv[1] == nullptr) {
        bc::app::RunApp run_app_instance;
        auto args = bc::parse_running_args(argc, argv, 1);
        if (!args) {
            std::fprintf(stderr, "Error: %s\n", be::to_string(args.error()).c_str());
            return EXIT_FAILURE;
        }
        return bc::run_app(run_app_instance, *args);
    }

    std::string_view cmd{argv[1]};

    if (cmd == "version")
        return bc::VersionCommand{}.run(argc, argv);
    if (cmd == "help")
        return bc::HelpCommand{}.run(argc, argv);

    if (cmd != "run") {
        std::fprintf(stderr, "Unknown command: '%s'\n\n", argv[1]);
        std::fwrite(bc::k_usage_text.data(), 1, bc::k_usage_text.size(), stderr);
        return EXIT_FAILURE;
    }

    // "run" command
    std::unique_ptr<bc::App> app;
    int flags_start;
    if (argc < 3 || argv[2] == nullptr || argv[2][0] == '-') {
        // No scene name given (or first positional arg is a flag)
        app = std::make_unique<bc::app::RunApp>();
        flags_start = 2;
    } else {
        std::string_view scene{argv[2]};
        flags_start = 3;

        if (scene == "triangle")
            app = std::make_unique<bc::app::TriangleApp>();
        else if (scene == "cube")
            app = std::make_unique<bc::app::CubeApp>();
        else if (scene == "cube-scene")
            app = std::make_unique<bc::app::CubeSceneApp>();
        else if (scene == "textured-cube")
            app = std::make_unique<bc::app::TexturedCubeApp>();
        else if (scene == "free-camera")
            app = std::make_unique<bc::app::FreeCameraApp>();
        else if (scene == "phong")
            app = std::make_unique<bc::app::PhongApp>();
        else if (scene == "asset-demo")
            app = std::make_unique<bc::app::AssetDemoApp>();
        else if (scene == "hot-reload")
            app = std::make_unique<bc::app::HotReloadApp>();
        else {
            std::fprintf(stderr, "Unknown scene: '%s'\n\n", argv[2]);
            std::fprintf(stderr,
                "Usage: buddd run [<scene>] [--frame N] [--capture N:path]...\n"
                "\n"
                "Available scenes:\n"
                "  (empty)      Interactive empty window (no scene)\n"
                "  triangle     Coloured triangle (120 frames)\n"
                "  cube         Rotating cube demo (120 frames)\n"
                "  cube-scene   Cube demo via scene graph (World + RenderSystem, 120 frames)\n"
                "  textured-cube  Textured cube with UV-mapped brick texture (120 frames)\n"
                "  free-camera  Interactive free camera (WASD + mouse look, ESC to exit)\n"
                "  phong        Phong lighting demo (interactive, 5 cubes + 5 lights)\n"
                "  asset-demo   Asset pipeline demo: textured cube loaded via YAML metadata (120 frames)\n"
                "  hot-reload   Hot-reload test: swaps texture at frame 30, use --capture 30 and 60 to verify\n"
                "\n"
                "Flags:\n"
                "  --frame N        Render exactly N frames, then exit (default: interactive)\n"
                "  --capture N:path  Capture frame N to path; can be repeated\n"
                "\n"
                "Scene names are case-sensitive.\n");
            return EXIT_FAILURE;
        }
    }

    // Parse running arguments (--frame, --capture)
    auto args = bc::parse_running_args(argc, argv, flags_start);
    if (!args) {
        std::fprintf(stderr, "Error: %s\n", be::to_string(args.error()).c_str());
        return EXIT_FAILURE;
    }

    // Collect unexpected extra positional arguments (not flags and not flag values)
    {
        bool has_unexpected = false;
        for (int i = flags_start; i < argc; ++i) {
            if (argv[i][0] == '-') {
                ++i; // skip flag and its value
                continue;
            }
            // Check if this looks like a flag value (previous arg starts with --)
            if (i > flags_start && argv[i-1][0] == '-')
                continue;
            has_unexpected = true;
            break;
        }
        if (has_unexpected) {
            std::fprintf(stderr, "Warning: unexpected arguments after '%s':", argv[flags_start - 1]);
            for (int i = flags_start; i < argc; ++i)
                std::fprintf(stderr, " %s", argv[i]);
            std::fprintf(stderr, "\n");
        }
    }

    return bc::run_app(*app, *args);
}
