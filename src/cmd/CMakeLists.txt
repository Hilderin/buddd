file(GLOB_RECURSE CMD_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/commands/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/demo/*.cpp
)

add_executable(buddd ${CMD_SOURCES})

target_include_directories(buddd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(buddd PRIVATE buddd_engine)

if(BUDDD_HAS_DISPLAY)
    target_compile_definitions(buddd PRIVATE BUDDD_HAS_DISPLAY)
    message(STATUS "buddd: BUDDD_HAS_DISPLAY=ON (SDL3 backend)")
else()
    message(STATUS "buddd: BUDDD_HAS_DISPLAY=OFF (headless backend)")
endif()
