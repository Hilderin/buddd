add_library(buddd_engine STATIC
    version.h
    version.cpp
)

target_include_directories(buddd_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
