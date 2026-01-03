if(GRAPHITE_PLATFORM STREQUAL "Vulkan")
    # Meta-loader for Vulkan
    add_subdirectory("volk")
    target_link_libraries(graphite PUBLIC volk)
    target_compile_definitions(graphite PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK)

    # VMA
    target_compile_definitions(graphite PRIVATE NOMINMAX)
    target_include_directories(graphite PUBLIC "./vma")

    # ImGUI (optional)
    if(GRAPHITE_IMGUI_DIR)
        target_sources(graphite PRIVATE "${GRAPHITE_IMGUI_DIR}/backends/imgui_impl_vulkan.cpp")
        target_include_directories(graphite
            PUBLIC "${GRAPHITE_IMGUI_DIR}"
            PUBLIC "${GRAPHITE_IMGUI_DIR}/backends"
        )
        target_compile_definitions(graphite PUBLIC GRAPHITE_IMGUI=1)
    endif()

    # Tracy (optional)
    if(GRAPHITE_TRACY_DIR)
        target_include_directories(graphite
            PUBLIC "${GRAPHITE_TRACY_DIR}/public"
        )
        target_compile_definitions(graphite PUBLIC GRAPHITE_TRACY=1) 
        target_compile_definitions(graphite PUBLIC TRACY_ENABLE=1)
    endif()
endif()
