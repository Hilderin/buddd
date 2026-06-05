#include "app_config.h"

#include <cstdlib>
#include <string>
#include <string_view>

namespace be = buddd::engine;

auto buddd::cmd::parse_running_args(int argc, char* argv[], int start)
    -> engine::Result<RunningArgs>
{
    RunningArgs args;
    bool frame_explicit = false;

    for (int i = start; i < argc; ++i) {
        std::string_view arg{argv[i]};

        if (arg == "--frame") {
            if (i + 1 >= argc) {
                return be::make_error(
                    be::Error::Category::InvalidArgument,
                    "Error: --frame requires a number");
            }
            char* end = nullptr;
            long n = std::strtol(argv[i + 1], &end, 10);
            if (end == argv[i + 1] || *end != '\0' || n < 0) {
                std::string msg = "Error: --frame requires a non-negative integer, got '";
                msg += argv[i + 1];
                msg += "'";
                return be::make_error(
                    be::Error::Category::InvalidArgument, std::move(msg));
            }
            args.frame_limit = static_cast<int>(n);
            frame_explicit = true;
            ++i; // consume the value
        } else if (arg == "--capture") {
            if (i + 1 >= argc) {
                return be::make_error(
                    be::Error::Category::InvalidArgument,
                    "Error: --capture requires a frame number (format: N:path)");
            }
            std::string_view capture_arg{argv[i + 1]};
            auto colon_pos = capture_arg.find(':');
            if (colon_pos == std::string_view::npos || colon_pos == 0) {
                return be::make_error(
                    be::Error::Category::InvalidArgument,
                    "Error: --capture requires a frame number (format: N:path)");
            }

            std::string frame_str(capture_arg.substr(0, colon_pos));
            std::string path_str(capture_arg.substr(colon_pos + 1));

            char* end = nullptr;
            long n = std::strtol(frame_str.c_str(), &end, 10);
            if (end == frame_str.c_str() || *end != '\0' || n < 1) {
                return be::make_error(
                    be::Error::Category::InvalidArgument,
                    "Error: --capture requires a positive integer frame number");
            }

            args.captures.push_back({
                static_cast<int>(n),
                std::move(path_str)
            });
            ++i; // consume the value
        }
        // Unknown flags are silently ignored
    }

    // Post-parse validation for capture-frame interaction
    if (!args.captures.empty()) {
        int max_effective = 0;
        for (const auto& spec : args.captures) {
            int eff = spec.effective_frame();
            if (eff > max_effective)
                max_effective = eff;
        }

        if (!frame_explicit) {
            args.frame_limit = max_effective;
        } else if (args.frame_limit > 0 && args.frame_limit < max_effective) {
            std::string msg = "Error: --frame "
                + std::to_string(args.frame_limit)
                + " is too small for captures (need at least "
                + std::to_string(max_effective) + ")";
            return be::make_error(
                be::Error::Category::InvalidArgument, std::move(msg));
        }
    }

    return args;
}
