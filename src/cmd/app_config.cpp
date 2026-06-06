#include "app_config.h"
#include "log/log.h"
#include "log/file_sink.h"

#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>

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

// ---------------------------------------------------------------------------
// Helper: parse a level string into LogLevel
// ---------------------------------------------------------------------------
static auto parse_level(std::string_view s) -> std::optional<buddd::log::LogLevel> {
    if (s == "trace") return buddd::log::LogLevel::Trace;
    if (s == "debug") return buddd::log::LogLevel::Debug;
    if (s == "info")  return buddd::log::LogLevel::Info;
    if (s == "warn")  return buddd::log::LogLevel::Warn;
    if (s == "error") return buddd::log::LogLevel::Error;
    return std::nullopt;
}

// ---------------------------------------------------------------------------
// parse_logging_args() — parse --log-level, --log-file, --log-filter
// ---------------------------------------------------------------------------
auto buddd::cmd::parse_logging_args(int argc, char* argv[], int start)
    -> engine::Result<buddd::log::LogConfig>
{
    buddd::log::LogConfig config;

    for (int i = start; i < argc; ++i) {
        std::string_view arg{argv[i]};

        // --log-level=<level>
        if (arg.substr(0, 12) == "--log-level=") {
            std::string_view level_str = arg.substr(12);
            auto level = parse_level(level_str);
            if (!level) {
                std::string msg = "Error: invalid --log-level value '";
                msg += level_str;
                msg += "'. Expected one of: trace, debug, info, warn, error";
                return be::make_error(
                    be::Error::Category::InvalidArgument, std::move(msg));
            }
            config.global_min_level = *level;
        }
        // --log-file=<path>
        else if (arg.substr(0, 11) == "--log-file=") {
            std::string_view path_str = arg.substr(11);
            auto sink = buddd::log::FileSink::create(path_str);
            if (sink) {
                config.sinks.push_back(std::move(sink));
            }
            // If sink is null, FileSink::create already warned on stderr
        }
        // --log-filter=<pattern>=<level>
        else if (arg.substr(0, 13) == "--log-filter=") {
            std::string_view value = arg.substr(13);

            // Split on the LAST '=' to handle colons in tag prefixes
            auto eq_pos = value.rfind('=');
            if (eq_pos == std::string_view::npos || eq_pos == 0) {
                // No level specified — pattern only, use global level (no-op effectively)
                std::string pattern(value);
                config.tag_overrides.emplace_back(std::move(pattern), config.global_min_level);
            } else {
                std::string_view pattern_str = value.substr(0, eq_pos);
                std::string_view level_str = value.substr(eq_pos + 1);

                auto level = parse_level(level_str);
                if (!level) {
                    std::string msg = "Error: invalid level '";
                    msg += level_str;
                    msg += "' in --log-filter, expected one of: trace, debug, info, warn, error";
                    return be::make_error(
                        be::Error::Category::InvalidArgument, std::move(msg));
                }
                config.tag_overrides.emplace_back(std::string(pattern_str), *level);
            }
        }
        // Unknown flags are silently ignored (existing convention)
    }

    return config;
}
