file(GLOB_RECURSE CMD_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/commands/*.cpp
)

add_executable(buddd ${CMD_SOURCES})

target_include_directories(buddd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(buddd PRIVATE buddd_engine)
