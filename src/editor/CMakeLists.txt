add_library(buddd_editor STATIC
    editor.cpp
    command.cpp
    command_stack.cpp
    ${ImGuiFileDialog_SOURCE_DIR}/ImGuiFileDialog.cpp
)

target_link_libraries(buddd_editor
    PUBLIC
        buddd_engine
)

target_include_directories(buddd_editor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# ImGuiFileDialog include path
target_include_directories(buddd_editor SYSTEM PRIVATE
    ${ImGuiFileDialog_SOURCE_DIR}
)
