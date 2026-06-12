#include "version.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("engine version is non-empty", "[sanity]") {
    REQUIRE_FALSE(buddd::engine::version().empty());
}
