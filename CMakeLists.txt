cmake_minimum_required(VERSION 3.25)

project(anvil-vk LANGUAGES C CXX)

# --------------------------------------------------
# Project Directory Variables
# --------------------------------------------------
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/source")
set(EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")
set(BINARIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/binaries")

# --------------------------------------------------
# Automated Source File Collection
# --------------------------------------------------
file(GLOB_RECURSE APP_SOURCES
        "${SOURCE_DIR}/*.cpp"
        "${SOURCE_DIR}/*.c"
        "${SOURCE_DIR}/*.h"
        "${SOURCE_DIR}/*.hpp"
)

# --------------------------------------------------
# C++ standard
# --------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

# Volk uses Vulkan-Headers instead of find_package(Vulkan)
target_link_libraries(volk PUBLIC Vulkan::Headers)

# --------------------------------------------------
# Vulkan Memory Allocator (header-only)
# --------------------------------------------------
add_library(VulkanMemoryAllocator INTERFACE)
target_include_directories(VulkanMemoryAllocator SYSTEM INTERFACE
        ${EXTERNAL_DIR}/VulkanMemoryAllocator/include
)

# --------------------------------------------------
# Shaderc (Precompiled Binary from Submodule)
# --------------------------------------------------
add_library(shaderc INTERFACE)
target_include_directories(shaderc SYSTEM INTERFACE
        ${EXTERNAL_DIR}/shaderc/libshaderc/include
)

# Link the shaderc binary extracted via the setup script
if (MSVC)
    target_link_libraries(shaderc INTERFACE ${BINARIES_DIR}/shaderc/shaderc_combined.lib)
else()
    target_link_libraries(shaderc INTERFACE ${BINARIES_DIR}/shaderc/libshaderc_combined.a)
endif ()

# --------------------------------------------------
# SPIRV-Reflect
# --------------------------------------------------
add_library(spirv-reflect STATIC
        ${EXTERNAL_DIR}/SPIRV-Reflect/spirv_reflect.h
        ${EXTERNAL_DIR}/SPIRV-Reflect/spirv_reflect.c
)
target_include_directories(spirv-reflect SYSTEM PUBLIC ${EXTERNAL_DIR}/SPIRV-Reflect)

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
    )
    # Enable sanitizers only in Debug builds
    if((CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "") AND NOT MINGW)
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
            shaderc
    )
