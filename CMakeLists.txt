cmake_minimum_required(VERSION 3.21)

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
# Git submodules (optional but safe)
# --------------------------------------------------
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Updating submodules")
        execute_process(
                COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE GIT_SUBMOD_RESULT
        )
        if(NOT GIT_SUBMOD_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to update git submodules")
        endif()
    endif()
endif()

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

if(APPLE)
    set(GLFW_BUILD_COCOA ON  CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WIN32 OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)

elseif(WIN32)
    set(GLFW_BUILD_COCOA OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WIN32 ON  CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)

elseif(UNIX)
    set(GLFW_BUILD_COCOA OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WIN32 OFF CACHE BOOL "" FORCE)
    if(USE_WAYLAND)
        set(GLFW_BUILD_WAYLAND ON  CACHE BOOL "" FORCE)
        set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
    else()
        set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_X11 ON  CACHE BOOL "" FORCE)
    endif()
endif()

# --------------------------------------------------
# External libraries
# --------------------------------------------------
add_subdirectory(${EXTERNAL_DIR}/Vulkan-Headers)
add_subdirectory(${EXTERNAL_DIR}/volk)
add_subdirectory(${EXTERNAL_DIR}/glfw)
add_subdirectory(${EXTERNAL_DIR}/vk-bootstrap)

# Volk uses Vulkan-Headers instead of find_package(Vulkan)
target_link_libraries(volk PUBLIC Vulkan::Headers)

# --------------------------------------------------
# Vulkan Memory Allocator (header-only)
# --------------------------------------------------
add_library(VulkanMemoryAllocator INTERFACE)
target_include_directories(VulkanMemoryAllocator INTERFACE
        ${EXTERNAL_DIR}/VulkanMemoryAllocator/include
)

# --------------------------------------------------
# Shaderc (Precompiled Binary from Submodule)
# --------------------------------------------------
add_library(shaderc INTERFACE)
target_include_directories(shaderc INTERFACE
        ${EXTERNAL_DIR}/shaderc/libshaderc/include
)

# Link the shaderc binary extracted via the setup script
if(WIN32)
    target_link_libraries(shaderc INTERFACE ${BINARIES_DIR}/shaderc/shaderc_combined.lib)
elseif(UNIX)
    target_link_libraries(shaderc INTERFACE ${BINARIES_DIR}/shaderc/libshaderc_combined.a)
endif()

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

if(WIN32)
    target_compile_definitions(anvil-vk PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
    )
endif()

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
