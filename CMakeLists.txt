cmake_minimum_required(VERSION 3.25)

project(anvil-vk LANGUAGES C CXX)

# Global engine macros based on active configuration
add_compile_definitions(
    # Debug builds get debug macros + validation layers
    $<$<CONFIG:Debug>:ANVIL_DEBUG>
    $<$<CONFIG:Debug>:ANVIL_ENABLE_VALIDATION>

    # RelWithDebInfo gets release macros + validation layers
    $<$<CONFIG:RelWithDebInfo>:ANVIL_RELEASE>
    $<$<CONFIG:RelWithDebInfo>:ANVIL_ENABLE_VALIDATION>

    # Pure Release gets only release macros
    $<$<CONFIG:Release>:ANVIL_RELEASE>
)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project Directory Variables
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/source")
set(SHARED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shared")
set(EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")
set(BINARIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/binaries")

# Prevent find_package(Vulkan) from looking for system SDKs
set(CMAKE_DISABLE_FIND_PACKAGE_Vulkan ON)
unset(ENV{VULKAN_SDK})

# External dependencies
add_subdirectory(external)

# Automated Source File Collection
file(GLOB_RECURSE SOURCES
    "${SOURCE_DIR}/*.cpp"
    "${SOURCE_DIR}/*.c"
    "${SOURCE_DIR}/*.h"
    "${SOURCE_DIR}/*.hpp"
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Target-specific includes
target_include_directories(${PROJECT_NAME} PUBLIC
    ${SOURCE_DIR}/core
    ${SOURCE_DIR}/scene
    ${SOURCE_DIR}/diagnostics
    ${SOURCE_DIR}/programs
    ${SOURCE_DIR}/rendercore
    ${SOURCE_DIR}/rendercore/context
    ${SOURCE_DIR}/rendercore/resources
    ${SOURCE_DIR}/renderer
    ${SHARED_DIR}
)

# Engine properties & compile definitions
target_compile_definitions(${PROJECT_NAME} PUBLIC
        UNIMPLEMENTED=0
        DEPRECATED=0
        SHADER_DIR=\"${CMAKE_SOURCE_DIR}/shaders\"
        ASSETS_DIR=\"${CMAKE_SOURCE_DIR}/assets\"
        TOOLS_DIR=\"${CMAKE_SOURCE_DIR}/tools\"
)

# OS specific API rules
if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
endif ()

# Build diagnostics
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(ENABLE_SANITIZERS  "Enable Address (ASan) and Undefined Behavior (UBSan) Sanitizers" OFF)

# Compiler Warnings & Sanitizers
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")

    # Comprehensive Warning Flags
    set(WARNING_FLAGS
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow # Warn when a variable shadows another in an outer scope
        -Wnon-virtual-dtor # Warn if a class with virtual functions has a non-virtual destructor
        -Wold-style-cast # Warn on C-style casts (e.g. (int)x -> prefer static_cast)
        -Wcast-align # Warn on casts that increase alignment requirements
        -Wunused # Warn on unused variables, functions, and parameters
        -Woverloaded-virtual # Warn when a function hides a virtual function from a base class
        -Wconversion # Warn on implicit conversions that may alter a value (e.g. double to float)
        -Wsign-conversion # Warn on implicit conversions between signed/unsigned
        -Wnull-dereference # Warn if a null pointer dereference is detected
        -Wdouble-promotion # Warn when float is implicitly promoted to double
        -Wformat=2 # Strict format string checks (printf, snprintf, etc.)
        -Wimplicit-fallthrough # Warn on unannotated switch case fallthrough
        -Wno-missing-field-initializers # Disable noise for aggregate init like VkStructInfo{}
    )

    if (WARNINGS_AS_ERROR)
        list(APPEND WARNING_FLAGS -Werror)
    endif ()

    # Strict warnings for GCC/Clang
    target_compile_options(${PROJECT_NAME} PRIVATE ${WARNING_FLAGS})

    # Linker Hardening Checks
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<PLATFORM_ID:Linux>:-Wl,--no-undefined> # Fail if shared objects have undefined symbols
        $<$<PLATFORM_ID:Windows>:-Wl,--fatal-warnings> # Fail linking if the linker emits warnings
    )

    # Sanitizers (ASan & UBSan)
    if (ENABLE_SANITIZERS)
        message(STATUS "Enabling AddressSanitizer (ASan) and UndefinedBehaviourSanitizer (UBSan)")

        # Compile options for sanitizers
        target_compile_options(${PROJECT_NAME} PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer # Retain frame pointers for clean stack traces
            -fno-optimize-sibling-calls # Prevents tail-call elimination in debug callstacks
        )

        # Link options for sanitizers
        target_link_options(${PROJECT_NAME} PRIVATE
                -fsanitize=address,undefined
        )
    endif ()

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # TODO: add MSVC stuff
endif ()

# Link libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
        glfw
        volk
        vk-bootstrap::vk-bootstrap
        vma
        glm
        slang
        imgui
        cgltf
        stb
        renderdoc
        tracy_client
        tml
)

# Examples
add_subdirectory(examples)
