cmake_minimum_required(VERSION 3.21)

project(anvil-vk LANGUAGES C CXX)

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

set(CMAKE_DISABLE_FIND_PACKAGE_Vulkan ON)

# --------------------------------------------------
# GLFW configuration (MUST be before add_subdirectory)
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
add_subdirectory(External/Vulkan-Headers)

set(CMAKE_DISABLE_FIND_PACKAGE_Vulkan ON) # ?
unset(ENV{VULKAN_SDK}) # ?
#set(VOLK_USE_ENV_VULKAN_SDK OFF CACHE BOOL "Ignore Vulkan SDK env variable") #?

add_subdirectory(External/volk)
add_subdirectory(External/glfw)
add_subdirectory(External/vk-bootstrap)

# Volk uses Vulkan-Headers instead of find_package(Vulkan)
target_link_libraries(volk PUBLIC Vulkan::Headers)

# --------------------------------------------------
# Vulkan Memory Allocator (header-only)
# --------------------------------------------------
add_library(VulkanMemoryAllocator INTERFACE)
target_include_directories(VulkanMemoryAllocator INTERFACE
        External/VulkanMemoryAllocator/include
)

# --------------------------------------------------
# Application target
# --------------------------------------------------
add_executable(anvil-vk
        Source/main.cpp
        Source/VulkanUtilities/anvilVkHandle.cpp
        Source/VulkanUtilities/anvilVkHandle.hpp
        # add other .cpp files here
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
)
