cmake_minimum_required(VERSION 3.25)

project(anvil-vk LANGUAGES C CXX)

# --------------------------------------------------
# C++ standard
# --------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------
# Project Directory Variables
# --------------------------------------------------
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/source")
set(EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")
set(BINARIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/binaries")
set(EXAMPLE_DIR "${SOURCE_DIR}/examples")

# --------------------------------------------------
# Automated Source File Collection
# --------------------------------------------------
file(GLOB_RECURSE APP_SOURCES
        "${SOURCE_DIR}/*.cpp"
        "${SOURCE_DIR}/*.c"
        "${SOURCE_DIR}/*.h"
        "${SOURCE_DIR}/*.hpp"

        "${EXAMPLE_DIR}/*.cpp"
        "${EXAMPLE_DIR}/*.h"
)

# Prevent find_package(Vulkan) from looking for system SDKs
set(CMAKE_DISABLE_FIND_PACKAGE_Vulkan ON)
unset(ENV{VULKAN_SDK})

# --------------------------------------------------
# GLFW configuration
# --------------------------------------------------
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

if (APPLE)
    set(GLFW_BUILD_COCOA ON CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WIN32 OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)

elseif (WIN32)
    set(GLFW_BUILD_COCOA OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WIN32 ON CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)

elseif (UNIX)
    set(GLFW_BUILD_COCOA OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WIN32 OFF CACHE BOOL "" FORCE)
    if (USE_WAYLAND)
        set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE)
        set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
    else ()
        set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
    endif ()
endif ()

# --------------------------------------------------
# External libraries
# --------------------------------------------------
add_subdirectory(${EXTERNAL_DIR}/Vulkan-Headers SYSTEM)
add_subdirectory(${EXTERNAL_DIR}/volk SYSTEM)
add_subdirectory(${EXTERNAL_DIR}/glfw SYSTEM)
add_subdirectory(${EXTERNAL_DIR}/vk-bootstrap SYSTEM)
add_subdirectory(${EXTERNAL_DIR}/glm SYSTEM)

# Volk uses Vulkan-Headers instead of find_package(Vulkan)
target_link_libraries(volk PUBLIC Vulkan::Headers)

# --------------------------------------------------
# Vulkan Memory Allocator (header-only)
# --------------------------------------------------
add_library(VulkanMemoryAllocator INTERFACE)
target_include_directories(VulkanMemoryAllocator SYSTEM INTERFACE
        ${EXTERNAL_DIR}/VulkanMemoryAllocator/include
)

# ----------------------------------------------
# Slang Setup
# ----------------------------------------------
add_library(slang INTERFACE)
target_include_directories(slang SYSTEM INTERFACE  ${EXTERNAL_DIR}/slang/include)

if (WIN32)
    target_link_libraries(slang INTERFACE ${EXTERNAL_DIR}/slang/lib/slang.lib)
elseif (UNIX)
    # Unimplemented
endif ()

# --------------------------------------------------
# Application target
# --------------------------------------------------
# Use the automated source list here
add_executable(anvil-vk ${APP_SOURCES})

# Target-specific includes
target_include_directories(anvil-vk PRIVATE
        ${SOURCE_DIR}/core
        ${SOURCE_DIR}/vulkan
        ${SOURCE_DIR}/utilities

        # Testing user renderer
        # Figure out a better way to structure non anvil side
        ${SOURCE_DIR}/examples
)

# --------------------------------------------------
# Target properties & compile definitions
# --------------------------------------------------
target_compile_definitions(anvil-vk PRIVATE
        GLFW_INCLUDE_NONE
        VK_NO_PROTOTYPES
        VK_ENABLE_BETA_EXTENSIONS
)

if (WIN32)
    target_compile_definitions(anvil-vk PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
    )
endif ()

target_compile_definitions(anvil-vk PRIVATE
        ANVIL_SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders"
)

# --------------------------------------------------
# Compiler Warnings & Sanitizers
# --------------------------------------------------
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Strict warnings for GCC/Clang
    target_compile_options(anvil-vk PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wno-missing-field-initializers
    )
    # Enable sanitizers only in Debug builds
    if ((CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "") AND NOT MINGW)
        target_compile_options(anvil-vk PRIVATE
                -g
                -fno-omit-frame-pointer
                -fsanitize=address,undefined
        )
        target_link_options(anvil-vk PRIVATE
                -fsanitize=address,undefined
        )
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # TODO: add MSVC stuff
endif ()

# --------------------------------------------------
# Link libraries
# --------------------------------------------------
target_link_libraries(anvil-vk PRIVATE
        glfw
        volk
        vk-bootstrap::vk-bootstrap
        VulkanMemoryAllocator
        glm
)

# Slang linking
target_link_libraries(anvil-vk PRIVATE slang)

# 3. The Runtime Gotcha: Copy Slang dynamic libraries to output folder
# Slang loads compiler modules dynamically at runtime.
# If the .dll/.so isn't sitting next to the .exe, the engine will crash.
# Crucial: Copy dynamic libraries to executable directory
add_custom_command(TARGET anvil-vk POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${EXTERNAL_DIR}/slang/bin $<TARGET_FILE_DIR:anvil-vk>
)
