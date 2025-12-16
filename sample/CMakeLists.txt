set(SAMPLE_DIR .)
set(EXE_NAME sample)

# Executable target
project(${EXE_NAME})
add_executable(${EXE_NAME} "${SAMPLE_DIR}/src/main.cc")
set_target_properties(${EXE_NAME} PROPERTIES OUTPUT_NAME ${EXE_NAME})

# Add compiler definitions
target_compile_definitions(${EXE_NAME} PRIVATE $<$<CONFIG:Debug>:DEBUG=1>)
target_compile_definitions(${EXE_NAME} PRIVATE $<$<CONFIG:Release>:NDEBUG=1>)

# Include directories
target_include_directories(${EXE_NAME} PRIVATE "${SAMPLE_DIR}/src/")

# Link to the Graphite library
target_link_libraries(${EXE_NAME} PRIVATE graphite)

# External dependencies
add_subdirectory("extern")
