cmake_minimum_required(VERSION 3.15)
project(graphite)

# C++ standard version
set(CMAKE_CXX_STANDARD 17 CACHE STRING "" FORCE)

# Executable target
add_executable(graphite)
set_target_properties(graphite PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME})

# Add compiler definitions
target_compile_definitions(graphite PRIVATE $<$<CONFIG:Debug>:DEBUG=1>)
target_compile_definitions(graphite PRIVATE $<$<CONFIG:Release>:NDEBUG=1>)

# Target build platform
set(GRAPHITE_PLATFORM "Vulkan")

if(GRAPHITE_PLATFORM STREQUAL "Vulkan")
    set(PLATFORM_EXT vk)
    set(PLATFORM vulkan)
endif()

target_compile_definitions(graphite PRIVATE PLATFORM_EXT=${PLATFORM_EXT})
target_compile_definitions(graphite PRIVATE PLATFORM=${PLATFORM})

# Includes
target_include_directories(graphite PRIVATE "./src/")
target_include_directories(graphite PRIVATE "./src/platform")
target_include_directories(graphite PUBLIC "./src/shell/")

# Core source files
target_sources(graphite PRIVATE
    src/main.cc
    # Core components
    src/shell/graphite/gpu_adapter.cc
    src/shell/graphite/vram_bank.cc
    # Utils
    src/shell/graphite/debug.cc
    src/shell/graphite/result.cc
)

# Platform source files
target_sources(graphite PRIVATE
    # Core components
    src/platform/${PLATFORM}/gpu_adapter_${PLATFORM_EXT}.cc
    src/platform/${PLATFORM}/vram_bank_${PLATFORM_EXT}.cc
)
if(GRAPHITE_PLATFORM STREQUAL "Vulkan")
    target_sources(graphite PRIVATE
        # Wrapper components
        src/platform/vulkan/wrapper/device_selection_vk.cc
        src/platform/vulkan/wrapper/queue_selection_vk.cc
        src/platform/vulkan/wrapper/extensions_vk.cc
    )
endif()

# External dependencies
add_subdirectory("extern")
