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

# --------------------------------------------------
# Automated Source File Collection
# --------------------------------------------------
file(GLOB_RECURSE ANVIL_CORE_SOURCES
        "${SOURCE_DIR}/*.cpp"
        "${SOURCE_DIR}/*.c"
        "${SOURCE_DIR}/*.h"
        "${SOURCE_DIR}/*.hpp"
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

# --------------------------------------------------
# imgui
# --------------------------------------------------
add_library(imgui STATIC
        ${EXTERNAL_DIR}/imgui/imgui.cpp
        ${EXTERNAL_DIR}/imgui/imgui_draw.cpp
        ${EXTERNAL_DIR}/imgui/imgui_tables.cpp
        ${EXTERNAL_DIR}/imgui/imgui_widgets.cpp
        ${EXTERNAL_DIR}/imgui/imgui_demo.cpp        # optional
        ${EXTERNAL_DIR}/imgui/backends/imgui_impl_glfw.cpp
        ${EXTERNAL_DIR}/imgui/backends/imgui_impl_vulkan.cpp
)
target_compile_definitions(imgui PUBLIC
        IMGUI_IMPL_VULKAN_NO_PROTOTYPES
)
target_include_directories(imgui PUBLIC
        ${EXTERNAL_DIR}/imgui
        ${EXTERNAL_DIR}/imgui/backends)

# ImGui uses glfw and volk
target_link_libraries(imgui PUBLIC
        glfw
        volk
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
#add_executable(${PROJECT_NAME} ${ANVIL_CORE_SOURCES})
add_library(${PROJECT_NAME} STATIC ${ANVIL_CORE_SOURCES})

# Target-specific includes
target_include_directories(${PROJECT_NAME} PUBLIC
        ${SOURCE_DIR}/core
        ${SOURCE_DIR}/scene
        ${SOURCE_DIR}/vulkan
        ${SOURCE_DIR}/utilities
)

# Header only cgltf
target_include_directories(${PROJECT_NAME} PUBLIC
        ${EXTERNAL_DIR}/cgltf
)

# --------------------------------------------------
# Target properties & compile definitions
# --------------------------------------------------
target_compile_definitions(${PROJECT_NAME} PUBLIC
        GLFW_INCLUDE_NONE
        VK_NO_PROTOTYPES
        VK_ENABLE_BETA_EXTENSIONS
)

if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC
            WIN32_LEAN_AND_MEAN
            NOMINMAX
    )
endif ()

target_compile_definitions(${PROJECT_NAME} PUBLIC
        ANVIL_SHADER_DIR=\"${CMAKE_SOURCE_DIR}/shaders\"
)

# --------------------------------------------------
# Compiler Warnings & Sanitizers
# --------------------------------------------------
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Strict warnings for GCC/Clang
    target_compile_options(${PROJECT_NAME} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wno-missing-field-initializers
    )
    # Enable sanitizers only in Debug builds
    if ((CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "") AND NOT MINGW)
        target_compile_options(${PROJECT_NAME} PRIVATE
                -g
                -fno-omit-frame-pointer
                -fsanitize=address,undefined
        )
        target_link_options(${PROJECT_NAME} PRIVATE
                -fsanitize=address,undefined
        )
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # TODO: add MSVC stuff
endif ()

# --------------------------------------------------
# Link libraries
# --------------------------------------------------
target_link_libraries(${PROJECT_NAME} PUBLIC
        glfw
        volk
        vk-bootstrap::vk-bootstrap
        VulkanMemoryAllocator
        glm
        slang
        imgui
)

add_subdirectory(examples)
