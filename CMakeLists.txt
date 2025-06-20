cmake_minimum_required(VERSION 3.15)
project(graphite)

# C++ standard version
set(CMAKE_CXX_STANDARD 17 CACHE STRING "" FORCE)

# Library target
add_library(graphite)
# set_target_properties(graphite PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME})

# Add compiler definitions
target_compile_definitions(graphite PRIVATE $<$<CONFIG:Debug>:DEBUG=1>)
target_compile_definitions(graphite PRIVATE $<$<CONFIG:Release>:NDEBUG=1>)

# Target build platform
set(GRAPHITE_PLATFORM "Vulkan")

if(GRAPHITE_PLATFORM STREQUAL "Vulkan")
    set(PLATFORM_EXT vk)
    set(PLATFORM vulkan)
endif()

target_compile_definitions(graphite PUBLIC PLATFORM_EXT=${PLATFORM_EXT})
target_compile_definitions(graphite PUBLIC PLATFORM=${PLATFORM})

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
    ${CORE_DIR}/render_target.cc
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
    ${PLATFORM_DIR}/${PLATFORM}/render_target_${PLATFORM_EXT}.cc
)

# Additional platform-specific source files
if(GRAPHITE_PLATFORM STREQUAL "Vulkan")
    target_sources(graphite PRIVATE
        # Wrapper components
        ${PLATFORM_DIR}/vulkan/wrapper/device_selection_vk.cc
        ${PLATFORM_DIR}/vulkan/wrapper/queue_selection_vk.cc
        ${PLATFORM_DIR}/vulkan/wrapper/extensions_vk.cc
    )
endif()

# If in vscode, add the samples so we can run them
if($ENV{EXECUTABLE})
    add_subdirectory("samples")
endif()

# External dependencies
add_subdirectory("extern")
