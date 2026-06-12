cmake_minimum_required(VERSION 3.28)
project(buddd VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src/engine)
add_subdirectory(src/cmd)

include(FetchContent)

# ImGuiFileDialog — needs to be populated before src/editor CMakeLists.txt
FetchContent_Declare(
    ImGuiFileDialog
    GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog.git
    GIT_TAG v0.6.7
)
FetchContent_GetProperties(ImGuiFileDialog)
if(NOT ImGuiFileDialog_POPULATED)
    FetchContent_Populate(ImGuiFileDialog)
endif()
# Manually set the source directory variable (FetchContent_Populate may not always set it)
if(NOT ImGuiFileDialog_SOURCE_DIR)
    set(ImGuiFileDialog_SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/imguifiledialog-src")
endif()
message(STATUS "ImGuiFileDialog source dir: ${ImGuiFileDialog_SOURCE_DIR}")

add_subdirectory(src/editor)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.0
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
)
FetchContent_MakeAvailable(Catch2)

option(BUDDD_HAS_DISPLAY "Enable SDL3 backend tests (requires display or offscreen driver)" ON)

enable_testing()

add_subdirectory(tests)

find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        src/*.cpp src/*.h src/*.hpp
        tests/*.cpp tests/*.h tests/*.hpp
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES}
        COMMENT "Formatting all source files with clang-format..."
    )
else()
    add_custom_target(format
        COMMAND ${CMAKE_COMMAND} -E echo "clang-format not found. Install clang-format >= 18."
        COMMAND false
    )
endif()
