# Auto-discover test files — new *_test.cpp files are picked up automatically.
# Requires a CMake re-configure when adding/removing test files (CONFIGURE_DEPENDS).
file(GLOB_RECURSE BUDDD_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*_tests.cpp
)

if(BUDDD_HAS_DISPLAY)
    message(STATUS "BUDDD_HAS_DISPLAY=ON: SDL3 backend tests enabled")
else()
    message(STATUS "BUDDD_HAS_DISPLAY=OFF: SDL3 backend tests excluded")
    # Remove SDL3 backend test in headless mode — it requires a display
    list(FILTER BUDDD_TEST_SOURCES EXCLUDE REGEX "sdl3_backend_test\\.cpp$")
endif()

add_executable(buddd_tests
    ${BUDDD_TEST_SOURCES}
    ${CMAKE_SOURCE_DIR}/src/cmd/app_config.cpp
)

if(BUDDD_HAS_DISPLAY)
    target_compile_definitions(buddd_tests PRIVATE BUDDD_HAS_DISPLAY)
endif()

target_include_directories(buddd_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/cmd
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# yaml-cpp include path (fetched in src/engine/CMakeLists.txt)
if(EXISTS "${CMAKE_BINARY_DIR}/_deps/yaml-cpp-src/include")
    target_include_directories(buddd_tests PRIVATE
        ${CMAKE_BINARY_DIR}/_deps/yaml-cpp-src/include
    )
endif()

target_link_libraries(buddd_tests PRIVATE
    buddd_engine
    buddd_editor
    Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(buddd_tests)
