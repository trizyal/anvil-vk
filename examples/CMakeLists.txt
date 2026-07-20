# --------------------------------------------------
# Automated Example Generation Macro
# --------------------------------------------------
macro(add_anvil_example EXAMPLE_NAME)
    # Collect files in the example directory
    file(GLOB_RECURSE EXAMPLE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME}/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME}/*.hpp"
    )

    # Compile the standalone example target executable
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCES})

    target_include_directories(${EXAMPLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME})

    # Link against the engine library target
    target_link_libraries(${EXAMPLE_NAME} PRIVATE ${PROJECT_NAME})

    # The Runtime Gotcha: Copy Slang dynamic libraries to output folder
    # Crucial: Copy dynamic libraries to executable directory so the .exe doesn't crash
    add_custom_command(TARGET ${EXAMPLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${EXTERNAL_DIR}/slang/bin $<TARGET_FILE_DIR:${EXAMPLE_NAME}>
    )
endmacro()

# --------------------------------------------------
# Register Active Sandboxes / Examples
# --------------------------------------------------
# Just call the macro with the name of the folder containing the specific example.
add_anvil_example(HelloTriangle)
add_anvil_example(HelloCube)
