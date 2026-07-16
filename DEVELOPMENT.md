# Development Guide

## Project Structure

```
anvil-vk/
│
├── CMakeLists.txt
│
├── external/
│   ├── glfw/
│   ├── shaderc/
│   ├── slang/       # Run fetch_slang.py to get the directory
│   ├── SPIRV-Reflect/
│   ├── vk-bootstrap/
│   ├── volk/
│   ├── Vulkan-Headers/
│   ├── VulkanMemoryAllocator/
│   └── # More to come
│ 
├── source/
│   ├── main.cpp
│   ├── core/       # Engine lifecycle and OS-level stuff
│   │   ├── AnvilApplication.h/.cpp
│   │   └── AnvilWindow.h/.cpp
│   │
│   ├── examples/   # How to use anvil
│   │   ├── HelloTriangle.h/.cpp
│   │   └── # More to come
│   │
│   ├──utilities/
│   │   ├── AnvilFileIO.h/.cpp
│   │   ├── AnvilShaderCompiler.h/.cpp
│   │   └── AnvilShaders.h/.cpp
│   │
│   └── vulkan/     # The Vulkan abstraction layers
│       ├── AnvilVulkanContext.h/.cpp    # Instance, Device, VMA allocator
│       ├── AnvilSwapchain.h/.cpp        # Swapchain and recreation logic
│       ├── AnvilShaderModule.h/.cpp     # Shader modules
│       ├── AnvilRenderer.h/.cpp         # Command buffers, sync structures, draw loop
│       ├── AnvilPipeline.h/.cpp         # Shader loading and VkPipeline creation
│       └── AnvilDeletionQueue.h         # Pattern for safe resource cleanup
│
└── shaders/        # Shader Files
    ├── HelloTriangle.slang
    └── glsl/
        ├── HelloTriangle.vert
        └── HelloTriangle.frag
```

## Separation of Context

### AnvilVulkanContext

1. Initialize Volk.
2. Create `VkInstance`.
3. Create debug messenger in debug builds.
4. Create window surface.
5. Pick physical device.
6. Create logical device.
7. Get graphics queue.
8. Create VMA allocator.
9. Register destruction in the deletion queue.

### AnvilWindow

- Initialize GLFW once.
- Create a no-API window.
- Create a Vulkan surface.
- Destroy the window.
- Terminate GLFW.

### AnvilSwapchain

- Build swapchain using `vk-bootstrap`.
- Store images.
- Store image views.
- Cleanup image views and swapchain.
- Support `recreate`.

The render loop should detect:

```c++
VK_ERROR_OUT_OF_DATE_KHR
VK_SUBOPTIMAL_KHR
```


and then call swapchain recreation.

## Naming Conventions

- directories - `camelCase`
- files - `PascalCase`
- namespace - `PascalCase`
- classes - `PascalCase`
- struct - `PascalCase`
- class functions - `camelCase`
- individual functions - `snake_case`
- global - `CAPITALCASE`
- \#defines - `MACRO_CASE`
- enums name - `PascalCase`
- enums values - `MACRO_CASE`


- variables:
  - in class - `camelCase`
  - in struct - `snake_case`