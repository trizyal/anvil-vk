cmake_minimum_required(VERSION 3.25)

project(anvil-vk LANGUAGES C CXX)

# --------------------------------------------------
# C++ standard
# --------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------
# Build Options
# --------------------------------------------------
# Toggle this via CLI: cmake -B build -DUSE_SHADERC=ON
option(USE_SHADERC "Compile with Shaderc and SPIRV-Reflect for GLSL" OFF)

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
# Conditional shaderc and spirv-reflect
# --------------------------------------------------
if (USE_SHADERC)
    # --------------------------------------------------
    # Shaderc (Precompiled Binary from Submodule)
    # --------------------------------------------------
    add_library(shaderc INTERFACE)
    target_include_directories(shaderc SYSTEM INTERFACE
            ${EXTERNAL_DIR}/shaderc/libshaderc/include
    )

    # Select the shaderc binary that matches the active C++ toolchain
    #
    # Windows toolchain cases:
    #   MSVC        -> cl.exe, MSVC ABI, .lib
    #   clang-msvc  -> clang-cl.exe, MSVC ABI, .lib
    #   mingw       -> g++/MingW, GNU ABI, .a
    #   clang-mingw -> clang++, MingW/GNU ABI, .a
    if (WIN32)
        if (MSVC)
            set(SHADERC_TOOLCHAIN_DIR "msvc")
            set(SHADERC_LIB_NAME "shaderc_combined.lib")
            set(SHADERC_SETUP_HINT "setupshaderc.bat msvc")

        elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
            set(SHADERC_TOOLCHAIN_DIR "clang-msvc")
            set(SHADERC_LIB_NAME "shaderc_combined.lib")
            set(SHADERC_SETUP_HINT "setupshaderc.bat clang-msvc")

        elseif (MINGW)
            set(SHADERC_TOOLCHAIN_DIR "mingw")
            set(SHADERC_LIB_NAME "libshaderc_combined.a")
            set(SHADERC_SETUP_HINT "setupshaderc.bat mingw")

        elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            set(SHADERC_TOOLCHAIN_DIR "clang-mingw")
            set(SHADERC_LIB_NAME "libshaderc_combined.a")
            set(SHADERC_SETUP_HINT "setupshaderc.bat clang-mingw")

        else ()
            message(FATAL_ERROR
                    "Unsupported Windows compiler for Shaderc prebuilt binary selection.\n"
                    "Compiler ID: ${CMAKE_CXX_COMPILER_ID}\n"
                    "Simulate ID: ${CMAKE_CXX_SIMULATE_ID}\n"
                    "MINGW: ${MINGW}\n"
            )
        endif ()

    else ()
        message(FATAL_ERROR
                "Shaderc prebuilt binary selection is currently configured for Windows only.\n"
        )
    endif ()

    set(SHADERC_LIB_PATH "${BINARIES_DIR}/shaderc/${SHADERC_TOOLCHAIN_DIR}/${SHADERC_LIB_NAME}")

    if (NOT EXISTS "${SHADERC_LIB_PATH}")
        message(FATAL_ERROR
                "Shaderc static library not found:\n"
                "  ${SHADERC_LIB_PATH}\n"
                "Compiler detected: ${CMAKE_CXX_COMPILER_ID}\n"
                "Run the setup script from the project root to generate it:\n"
                "  ${SHADERC_SETUP_HINT}\n"
                "Note: the MSVC .lib and the MinGW/GCC .a are NOT interchangeable; "
                "you must generate the file that matches your active toolchain."
        )
    endif ()

    target_link_libraries(shaderc INTERFACE "${SHADERC_LIB_PATH}")

    # --------------------------------------------------
    # SPIRV-Reflect
    # --------------------------------------------------
    add_library(spirv-reflect STATIC
            ${EXTERNAL_DIR}/SPIRV-Reflect/spirv_reflect.h
            ${EXTERNAL_DIR}/SPIRV-Reflect/spirv_reflect.c
    )
    target_include_directories(spirv-reflect SYSTEM PUBLIC ${EXTERNAL_DIR}/SPIRV-Reflect)

else ()
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

# Define the SHADERC macro in C++ if the CMake option is toggled ON
if (USE_SHADERC)
    target_compile_definitions(anvil-vk PRIVATE SHADERC)
else()
    target_compile_definitions(anvil-vk PRIVATE SLANG)
endif()

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
)

# Conditional linking
if (USE_SHADERC)
    target_link_libraries(anvil-vk PRIVATE shaderc)
else ()
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
endif ()
