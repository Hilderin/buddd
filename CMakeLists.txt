cmake_minimum_required(VERSION 3.28)
project(buddd VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src/engine)
add_subdirectory(src/cmd)
add_subdirectory(src/editor)

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.0
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
