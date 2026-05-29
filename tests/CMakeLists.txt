if(BUDDD_HAS_DISPLAY)
    message(STATUS "BUDDD_HAS_DISPLAY=ON: SDL3 backend tests enabled")

    add_executable(buddd_tests
        version_test.cpp
        platform_abstraction_test.cpp
        sdl3_backend_test.cpp
    )

    target_compile_definitions(buddd_tests PRIVATE BUDDD_HAS_DISPLAY)
else()
    message(STATUS "BUDDD_HAS_DISPLAY=OFF: SDL3 backend tests excluded")

    add_executable(buddd_tests
        version_test.cpp
        platform_abstraction_test.cpp
    )
endif()

target_link_libraries(buddd_tests PRIVATE
    buddd_engine
    Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(buddd_tests)
