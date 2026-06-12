#include "test_helpers.h"
#include "app_config.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <string>
#include <vector>

namespace bc = buddd::cmd;
namespace be = buddd::engine;

// ---------------------------------------------------------------------------
// Helper to call parse_running_args with a vector of strings
// ---------------------------------------------------------------------------
auto parse_args(const std::vector<std::string>& args)
    -> be::Result<bc::RunningArgs>
{
    // Build C-style argv array
    std::vector<const char*> argv_ptrs;
    argv_ptrs.reserve(args.size());
    for (const auto& s : args) {
        argv_ptrs.push_back(s.c_str());
    }
    int argc = static_cast<int>(argv_ptrs.size());
    // We pass a non-const version; parse_running_args needs char** not const char**
    // So we need to const_cast it (safe because we own the memory)
    return bc::parse_running_args(
        argc, const_cast<char**>(argv_ptrs.data()), 0);
}

// ---------------------------------------------------------------------------
// EF — CaptureSpec::effective_frame() unit tests
// ---------------------------------------------------------------------------

TEST_CASE("effective_frame(0) returns 2", "[cli][capture]") {
    REQUIRE(bc::CaptureSpec{0, ""}.effective_frame() == 2);
}

TEST_CASE("effective_frame(1) returns 2", "[cli][capture]") {
    REQUIRE(bc::CaptureSpec{1, ""}.effective_frame() == 2);
}

TEST_CASE("effective_frame(2) returns 2", "[cli][capture]") {
    REQUIRE(bc::CaptureSpec{2, ""}.effective_frame() == 2);
}

TEST_CASE("effective_frame(3) returns 3", "[cli][capture]") {
    REQUIRE(bc::CaptureSpec{3, ""}.effective_frame() == 3);
}

TEST_CASE("effective_frame(120) returns 120", "[cli][capture]") {
    REQUIRE(bc::CaptureSpec{120, ""}.effective_frame() == 120);
}

// ---------------------------------------------------------------------------
// CF — parse_running_args() capture-frame validation tests
// ---------------------------------------------------------------------------

TEST_CASE("auto-set frame_limit from single capture", "[cli][capture]") {
    auto result = parse_args({"--capture", "120:/tmp/out.png"});
    REQUIRE(result.has_value());
    REQUIRE(result->frame_limit == 120);
    REQUIRE(result->captures.size() == 1);
    REQUIRE(result->captures[0].frame == 120);
}

TEST_CASE("auto-set frame_limit from frame-1 capture (effective_frame=2)",
          "[cli][capture]") {
    auto result = parse_args({"--capture", "1:/tmp/out.png"});
    REQUIRE(result.has_value());
    REQUIRE(result->frame_limit == 2);
}

TEST_CASE("auto-set frame_limit from max of multiple captures",
          "[cli][capture]") {
    auto result = parse_args(
        {"--capture", "1:/tmp/a.png", "--capture", "50:/tmp/b.png"});
    REQUIRE(result.has_value());
    REQUIRE(result->frame_limit == 50);
}

TEST_CASE("error when explicit --frame < max_effective", "[cli][capture]") {
    auto result = parse_args(
        {"--frame", "1", "--capture", "1:/tmp/out.png"});
    REQUIRE(!result.has_value());
    std::string msg = be::to_string(result.error());
    REQUIRE(msg.find("too small") != std::string::npos);
    REQUIRE(msg.find("need at least 2") != std::string::npos);
}

TEST_CASE("error when explicit --frame 50 with capture 120", "[cli][capture]") {
    auto result = parse_args(
        {"--frame", "50", "--capture", "120:/tmp/out.png"});
    REQUIRE(!result.has_value());
    std::string msg = be::to_string(result.error());
    REQUIRE(msg.find("need at least 120") != std::string::npos);
}

TEST_CASE("success when explicit --frame equals max_effective",
          "[cli][capture]") {
    auto result = parse_args(
        {"--frame", "120", "--capture", "120:/tmp/out.png"});
    REQUIRE(result.has_value());
    REQUIRE(result->frame_limit == 120);
}

TEST_CASE("success when explicit --frame exceeds max_effective",
          "[cli][capture]") {
    auto result = parse_args(
        {"--frame", "200", "--capture", "120:/tmp/out.png"});
    REQUIRE(result.has_value());
    REQUIRE(result->frame_limit == 200);
}

TEST_CASE("success when explicit --frame >= effective_frame for capture 1",
          "[cli][capture]") {
    auto result = parse_args(
        {"--frame", "2", "--capture", "1:/tmp/out.png"});
    REQUIRE(result.has_value());
    REQUIRE(result->frame_limit == 2);
}

TEST_CASE("success when --frame 0 (explicit interactive) with captures",
          "[cli][capture]") {
    auto result = parse_args(
        {"--frame", "0", "--capture", "120:/tmp/out.png"});
    REQUIRE(result.has_value());
    REQUIRE(result->frame_limit == 0);
}

TEST_CASE("no auto-set when no captures present", "[cli][capture]") {
    auto result = parse_args({});
    REQUIRE(result.has_value());
    REQUIRE(result->frame_limit == 0);
    REQUIRE(result->captures.empty());
}

TEST_CASE("explicit --frame without captures uses user value",
          "[cli][capture]") {
    auto result = parse_args({"--frame", "60"});
    REQUIRE(result.has_value());
    REQUIRE(result->frame_limit == 60);
    REQUIRE(result->captures.empty());
}

TEST_CASE("auto-set with both captures having same effective frame (both 3)",
          "[cli][capture]") {
    auto result = parse_args(
        {"--capture", "3:/tmp/a.png", "--capture", "3:/tmp/b.png"});
    REQUIRE(result.has_value());
    REQUIRE(result->frame_limit == 3);
}

TEST_CASE(
    "auto-set with captures having mixed effective frames (1 and 2)",
    "[cli][capture]") {
    auto result = parse_args(
        {"--capture", "1:/tmp/a.png", "--capture", "2:/tmp/b.png"});
    REQUIRE(result.has_value());
    REQUIRE(result->frame_limit == 2);
}
