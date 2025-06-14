# Function for adding a new sample
function(add_sample SAMPLE_NAME)
    set(SAMPLE_DIR ./${SAMPLE_NAME})
    set(EXE_NAME sample-${SAMPLE_NAME})

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

    add_subdirectory(${SAMPLE_NAME})
endfunction()

add_sample(testing)
