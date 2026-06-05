include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.30
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
)
FetchContent_MakeAvailable(SDL3)

# ----- GLM (header-only math library) -----
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# ----- stb (single-header image library) -----
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 31c1ad37456438565541f4919958214b6e762fb4
)
FetchContent_MakeAvailable(stb)

# ----- yaml-cpp (YAML parsing for asset metadata) -----
set(CMAKE_POLICY_VERSION_MINIMUM 3.5) # yaml-cpp 0.8.0 requires CMake >= 3.5
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
              -DCMAKE_POLICY_VERSION_MINIMUM=3.5
              -DYAML_CPP_BUILD_TESTS=OFF
              -DYAML_CPP_BUILD_TOOLS=OFF
              -DYAML_CPP_INSTALL=OFF
)
FetchContent_MakeAvailable(yaml-cpp)

# ----- tinygltf (glTF 2.0 model loading, header-only) -----
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG v2.9.7
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
              -DTINYGLTF_BUILD_LOADER_EXAMPLE=OFF
              -DTINYGLTF_BUILD_LOADER_TEST=OFF
)
FetchContent_MakeAvailable(tinygltf)

find_package(OpenGL REQUIRED)

# Collect all engine source files automatically using GLOB.
# New files added to src/engine/ subdirectories are picked up on re-configure.
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(buddd_engine STATIC ${ENGINE_SOURCES})

# Enable test-only accessors (BUDDD_TESTING guards)
target_compile_definitions(buddd_engine PRIVATE BUDDD_TESTING)

# Propagate display backend flag so the engine library uses the right ShaderProgram code path
if(BUDDD_HAS_DISPLAY)
    target_compile_definitions(buddd_engine PRIVATE BUDDD_HAS_DISPLAY)
endif()

target_include_directories(buddd_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(buddd_engine PRIVATE
    ${stb_SOURCE_DIR}
    ${yaml-cpp_SOURCE_DIR}/include
    ${tinygltf_SOURCE_DIR}
)

target_link_libraries(buddd_engine PUBLIC
    SDL3::SDL3
    OpenGL::GL
    glm::glm
)

target_link_libraries(buddd_engine PRIVATE
    yaml-cpp
)
