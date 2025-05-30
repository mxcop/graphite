if(GRAPHITE_PLATFORM STREQUAL "Vulkan")
    # Meta-loader for Vulkan
    add_subdirectory("volk")
    target_link_libraries(graphite PRIVATE volk)
endif()
