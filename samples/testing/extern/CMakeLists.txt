# GLFW
add_subdirectory("glfw")
target_link_libraries(sample-testing PRIVATE glfw)

# ImGUI
target_sources(sample-testing
    PRIVATE "./imgui/imgui_demo.cpp"
    PRIVATE "./imgui/imgui_draw.cpp"
    PRIVATE "./imgui/imgui_tables.cpp"
    PRIVATE "./imgui/imgui_widgets.cpp"
    PRIVATE "./imgui/imgui.cpp"
    PRIVATE "./imgui/backends/imgui_impl_glfw.cpp"
    PRIVATE "./imgui/backends/imgui_impl_vulkan.cpp"
    PRIVATE "./imgui/backends/imgui_impl_win32.cpp"
)
target_include_directories(sample-testing
    PRIVATE "./imgui"
    PRIVATE "./imgui/backends"
)

# STB Image
target_include_directories(sample-testing
    PRIVATE "./stb_image"
)