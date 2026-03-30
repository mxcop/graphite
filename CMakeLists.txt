cmake_minimum_required(VERSION 3.15)
project(graphite)

# C++ standard version
set(CMAKE_CXX_STANDARD 17 CACHE STRING "" FORCE)

# Options
option(GRAPHITE_SAMPLES "Include Graphite samples directory." ON)
set(GRAPHITE_PLATFORM "Vulkan" CACHE STRING "Graphite rendering platform")

# Library target
add_library(graphite)

# Add compiler definitions
target_compile_definitions(graphite PRIVATE $<$<CONFIG:Debug>:DEBUG=1>)
target_compile_definitions(graphite PRIVATE $<$<CONFIG:Release>:NDEBUG=1>)

# Target build platform
if(GRAPHITE_PLATFORM STREQUAL "Vulkan")
    set(PLATFORM_EXT vk)
    set(PLATFORM vulkan)
endif()

target_compile_definitions(graphite PUBLIC PLATFORM_EXT=${PLATFORM_EXT})
target_compile_definitions(graphite PUBLIC PLATFORM=${PLATFORM})

# Samples directory
if(GRAPHITE_SAMPLES)
    add_subdirectory("sample")
    set(GRAPHITE_IMGUI_DIR "${CMAKE_SOURCE_DIR}/sample/extern/imgui/")
endif()

# ImGUI support
if(GRAPHITE_IMGUI_DIR)
    # Sources
    target_sources(graphite
        PRIVATE "${GRAPHITE_IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
    )

    # Includes
    target_include_directories(graphite
        PUBLIC "${GRAPHITE_IMGUI_DIR}"
        PUBLIC "${GRAPHITE_IMGUI_DIR}/backends"
    )

    target_compile_definitions(graphite PUBLIC GRAPHITE_IMGUI=1)
endif()

# Include directories
target_include_directories(graphite PUBLIC "./src/")
target_include_directories(graphite PUBLIC "./src/platform/")
target_include_directories(graphite PUBLIC "./src/core/")

# Source file directories
set(CORE_DIR src/core/graphite)
set(PLATFORM_DIR src/platform)

# Core source files
target_sources(graphite PRIVATE
    # Core components
    ${CORE_DIR}/gpu_adapter.cc
    ${CORE_DIR}/vram_bank.cc
    ${CORE_DIR}/render_graph.cc
    ${CORE_DIR}/nodes/node.cc
    ${CORE_DIR}/nodes/compute_node.cc
    ${CORE_DIR}/nodes/raster_node.cc
    # Utils
    ${CORE_DIR}/utils/debug.cc
    ${CORE_DIR}/utils/result.cc
)

# Core platform-specific source files
target_sources(graphite PRIVATE
    # Core components
    ${PLATFORM_DIR}/${PLATFORM}/gpu_adapter_${PLATFORM_EXT}.cc
    ${PLATFORM_DIR}/${PLATFORM}/vram_bank_${PLATFORM_EXT}.cc
    ${PLATFORM_DIR}/${PLATFORM}/render_graph_${PLATFORM_EXT}.cc
    ${PLATFORM_DIR}/${PLATFORM}/imgui_${PLATFORM_EXT}.cc
)

# Additional platform-specific source files
if(GRAPHITE_PLATFORM STREQUAL "Vulkan")
    target_sources(graphite PRIVATE
        # Wrapper components
        ${PLATFORM_DIR}/vulkan/api_vk.cc
        ${PLATFORM_DIR}/vulkan/wrapper/descriptor_vk.cc
        ${PLATFORM_DIR}/vulkan/wrapper/device_selection_vk.cc
        ${PLATFORM_DIR}/vulkan/wrapper/queue_selection_vk.cc
        ${PLATFORM_DIR}/vulkan/wrapper/extensions_vk.cc
        ${PLATFORM_DIR}/vulkan/wrapper/pipeline_cache_vk.cc
        ${PLATFORM_DIR}/vulkan/wrapper/shader_vk.cc
        ${PLATFORM_DIR}/vulkan/wrapper/translate_vk.cc
    )
endif()

# External dependencies
add_subdirectory("extern")

# Bundle vulkan crash diagnostics layer
function(graphite_bundle_crash_diagnostics OUTPUT_DIR)
    if(GRAPHITE_PLATFORM STREQUAL "Vulkan" AND DEFINED ENV{VULKAN_SDK})
        set(CDL_DLL "$ENV{VULKAN_SDK}/Bin/VkLayer_crash_diagnostic.dll")
        set(CDL_JSON "$ENV{VULKAN_SDK}/Bin/VkLayer_crash_diagnostic.json")

        message(STATUS "CDL: Looking for DLL at ${CDL_DLL}")
        message(STATUS "CDL: Looking for JSON at ${CDL_JSON}")
        message(STATUS "CDL: Output directory: ${OUTPUT_DIR}/layers")

        if(EXISTS "${CDL_DLL}" AND EXISTS "${CDL_JSON}")
            file(COPY "${CDL_DLL}" "${CDL_JSON}"
                DESTINATION "${OUTPUT_DIR}/layers"
            )
            message(STATUS "CDL: Successfully copied crash diagnostic layer files")
        else()
            message(WARNING "CDL not found in Vulkan SDK - crash diagnostics won't be bundled")
        endif()
    else()
        if(NOT DEFINED ENV{VULKAN_SDK})
            message(WARNING "VULKAN_SDK environment variable not set - crash diagnostics won't be bundled")
        elseif(NOT GRAPHITE_PLATFORM STREQUAL "Vulkan")
            message(WARNING "Crash diagnostics only supported on Vulkan platform")
        endif()
    endif()
endfunction()