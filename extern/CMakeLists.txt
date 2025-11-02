if(GRAPHITE_PLATFORM STREQUAL "Vulkan")
    # Meta-loader for Vulkan
    add_subdirectory("volk")
    target_link_libraries(graphite PUBLIC volk)
endif()

# ImGUI
target_sources(graphite
    PRIVATE "./imgui/imgui_demo.cpp"
    PRIVATE "./imgui/imgui_draw.cpp"
    PRIVATE "./imgui/imgui_tables.cpp"
    PRIVATE "./imgui/imgui_widgets.cpp"
    PRIVATE "./imgui/imgui.cpp"
    # PRIVATE "./imgui/backends/imgui_impl_glfw.cpp"
    PRIVATE "./imgui/backends/imgui_impl_win32.cpp"
)
target_include_directories(graphite
    PUBLIC "./imgui"
    PRIVATE "./imgui/backends"
)
if(GRAPHITE_PLATFORM STREQUAL "Vulkan")
    # ImGUI Vulkan back-end
    target_sources(graphite
        PRIVATE "./imgui/backends/imgui_impl_vulkan.cpp"
    )
endif()
