if(GRAPHITE_PLATFORM STREQUAL "Vulkan")
    # Meta-loader for Vulkan
    add_subdirectory("volk")
    target_link_libraries(graphite PUBLIC volk)
    target_compile_definitions(graphite PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK)

    # VMA
    target_compile_definitions(graphite PRIVATE NOMINMAX)
    target_include_directories(graphite PUBLIC "./vma")
endif()
