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
