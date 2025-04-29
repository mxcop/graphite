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

# Includes
target_include_directories(graphite PRIVATE "./src/")
target_include_directories(graphite PUBLIC "./src/shell/")

# Glob all source files
target_sources(graphite PRIVATE 
    "src/main.cc"
    "src/shell/graphite/gpu_adapter.cc"
)

# Target build platform
target_compile_definitions(graphite PRIVATE PLATFORM_EXT=vk)
target_compile_definitions(graphite PRIVATE PLATFORM=vulkan)

# External dependencies
add_subdirectory("extern")
