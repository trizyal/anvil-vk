
# Target name for project
set(EXAMPLE_NAME RenderGraphStub)

# Collect source files
file(GLOB_RECURSE PROJECT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Add executable
add_executable(${EXAMPLE_NAME} ${PROJECT_SOURCES})

# Include local project folder header
target_include_directories(${EXAMPLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link against the core Anvil engine library
target_link_libraries(${EXAMPLE_NAME} PRIVATE ${PROJECT_NAME})

target_compile_definitions(${EXAMPLE_NAME} PUBLIC
        PROJECT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
)

# Crucial: Copy dynamic libraries to executable directory so the .exe doesn't crash
if(WIN32)
    add_custom_command(TARGET ${EXAMPLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/external/slang/windows/bin $<TARGET_FILE_DIR:${EXAMPLE_NAME}>
    )
elseif((UNIX))
    add_custom_command(TARGET ${EXAMPLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/external/slang/linux/bin $<TARGET_FILE_DIR:${EXAMPLE_NAME}>
    )
endif ()

if (MINGW)
    target_link_options(${EXAMPLE_NAME} PRIVATE
            -static-libgcc
            -static-libstdc++
            -Wl,-Bstatic -lpthread -Wl,-Bdynamic
    )

    get_filename_component(MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
    add_custom_command(TARGET ${EXAMPLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_BIN_DIR}/libwinpthread-1.dll"
            "$<TARGET_FILE_DIR:${EXAMPLE_NAME}>"
    )
endif ()
