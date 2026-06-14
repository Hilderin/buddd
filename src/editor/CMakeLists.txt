file(GLOB_RECURSE BUDDD_EDITOR_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(buddd_editor STATIC
    ${BUDDD_EDITOR_SOURCES}
)

target_link_libraries(buddd_editor
    PUBLIC
        buddd_engine
)

target_include_directories(buddd_editor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# yaml-cpp include path (fetched in src/engine/CMakeLists.txt).
# Required for editor code that transitively pulls in engine headers
# (e.g., component_info.h) which include <yaml-cpp/yaml.h>.
if(EXISTS "${CMAKE_BINARY_DIR}/_deps/yaml-cpp-src/include")
    target_include_directories(buddd_editor PRIVATE
        ${CMAKE_BINARY_DIR}/_deps/yaml-cpp-src/include
    )
endif()
