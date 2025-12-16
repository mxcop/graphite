# GLFW
add_subdirectory("glfw")
target_link_libraries(sample PRIVATE glfw)

# ImGUI
target_sources(sample
    PRIVATE "./imgui/imgui_demo.cpp"
    PRIVATE "./imgui/imgui_draw.cpp"
    PRIVATE "./imgui/imgui_tables.cpp"
    PRIVATE "./imgui/imgui_widgets.cpp"
    PRIVATE "./imgui/imgui.cpp"
    PRIVATE "./imgui/backends/imgui_impl_glfw.cpp"
    PRIVATE "./imgui/backends/imgui_impl_vulkan.cpp"
    PRIVATE "./imgui/backends/imgui_impl_win32.cpp"
)
target_include_directories(sample
    PRIVATE "./imgui"
    PRIVATE "./imgui/backends"
)

# STB Image
target_include_directories(sample
    PRIVATE "./stb_image"
)