add_executable(buddd_tests version_test.cpp)

target_link_libraries(buddd_tests PRIVATE
    buddd_engine
    Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(buddd_tests)
