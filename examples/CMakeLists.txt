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

    if (MINGW)
        target_link_options(${EXAMPLE_NAME} PRIVATE
                -static-libgcc
                -static-libstdc++
                -Wl,-Bstatic -lpthread -Wl,-Bdynamic
        )
    endif ()

    target_compile_definitions(${EXAMPLE_NAME} PUBLIC
            PROJECT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME}\"
    )

    # The Runtime Gotcha: Copy Slang dynamic libraries to output folder
    # Crucial: Copy dynamic libraries to executable directory so the .exe doesn't crash
    add_custom_command(TARGET ${EXAMPLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${EXTERNAL_DIR}/slang/bin $<TARGET_FILE_DIR:${EXAMPLE_NAME}>
    )

    if (MINGW)
        # Automatically find where CLion/MinGW keeps its DLLs
        get_filename_component(MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)

        # Copy libwinpthread-1.dll alongside the executable
        add_custom_command(TARGET ${EXAMPLE_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MINGW_BIN_DIR}/libwinpthread-1.dll"
                "$<TARGET_FILE_DIR:${EXAMPLE_NAME}>"
        )
    endif ()
endmacro()

# --------------------------------------------------
# Register Active Sandboxes / Examples
# --------------------------------------------------
# Just call the macro with the name of the folder containing the specific example.
add_anvil_example(HelloTriangle)
add_anvil_example(HelloCube)
add_anvil_example(glTFBox)
add_anvil_example(TextureCube)
add_anvil_example(ShaderReflectionCube)
add_anvil_example(DirectionalLight)
add_anvil_example(glTFTruck)
add_anvil_example(BoxAnimated)
add_anvil_example(RiggedSimple)
add_anvil_example(CesiumMan)
add_anvil_example(Sponza)
