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

# Glob all source files
file(GLOB_RECURSE SOURCES "src/*.cc")
# target_sources(graphite PRIVATE ${SOURCES})
target_sources(graphite PRIVATE 
    "src/main.cc"
    "src/core/test.cc"
)

target_compile_definitions(graphite PRIVATE PLATFORM_EXT=gl)
target_compile_definitions(graphite PRIVATE PLATFORM=opengl)
