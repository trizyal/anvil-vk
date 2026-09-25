# External Libraries

# -- Vulkan Headers
add_subdirectory(Vulkan-Headers SYSTEM)

# -- Volk
add_subdirectory(volk SYSTEM)

# Volk uses Vulkan-Headers instead of find_package(Vulkan)
target_link_libraries(volk PUBLIC Vulkan::Headers)

target_compile_definitions(volk PUBLIC
        VK_NO_PROTOTYPES
        VK_ENABLE_BETA_EXTENSIONS
)

# GLFW Configuration
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

# -- glfw
add_subdirectory(glfw SYSTEM)
target_compile_definitions(glfw INTERFACE
        GLFW_INCLUDE_NONE
)

# -- vk-bootstrap
add_subdirectory(vk-bootstrap SYSTEM)

# -- glm
add_subdirectory(glm SYSTEM)

# -- tml
add_subdirectory(tml)

# -- vma
add_library(vma INTERFACE)
target_include_directories(vma SYSTEM INTERFACE VulkanMemoryAllocator/include)

# -- cgltf
add_library(cgltf INTERFACE)
target_include_directories(cgltf SYSTEM INTERFACE cgltf)

# -- stb
add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE stb)

# -- renderdoc
add_library(renderdoc INTERFACE)
target_include_directories(renderdoc SYSTEM INTERFACE renderdoc)

# -- slang
add_library(slang INTERFACE)
target_include_directories(slang SYSTEM INTERFACE slang/include)
# linking slang with the precompiled library
if (WIN32)
    target_link_libraries(slang INTERFACE ${CMAKE_SOURCE_DIR}/external/slang/lib/slang.lib)
elseif (UNIX)
    # unsure
    target_link_libraries(slang INTERFACE slang/lib/libslang.so)
endif ()

# -- imgui
set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_demo.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_vulkan.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
# disable default vulkan function prototypes
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_NO_PROTOTYPES)
target_include_directories(imgui PUBLIC imgui imgui/backends)
# imgui uses volk and glfw
target_link_libraries(imgui PUBLIC volk glfw)

# -- tracy
option(TRACY "Enable Tracy Profiler" ON)
if(TRACY)
    option (TRACY_ENABLE "" ON)
    option (TRACY_ON_DEMAND "" ON)

    add_library(tracy_client STATIC tracy/TracyClient.cpp)
    target_include_directories(tracy_client PUBLIC tracy)

    # PUBLIC definitions propagate automatically to your engine when linked!
    # target_compile_definitions(tracy_client PUBLIC TRACY_ENABLE)

    # Disable all warnings for Tracy's internal code
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(tracy_client PRIVATE "-w")
    endif()

    find_package(Threads REQUIRED)
    target_link_libraries(tracy_client PUBLIC Threads::Threads)
    if (UNIX AND NOT APPLE)
        target_link_libraries(tracy_client PUBLIC dl)
    endif()
else()
    # Dummy interface so the engine doesn't break if TRACY is OFF
    add_library(tracy_client INTERFACE)
endif()
