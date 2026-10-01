
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
