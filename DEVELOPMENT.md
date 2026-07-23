# Development Guide

## Project Structure

```
anvil-vk/
│
├── CMakeLists.txt
│
├── external/
│   ├── cgltf/
│   ├── glfw/
│   ├── glm/
│   ├── slang/       # Run fetch_slang.py to get the directory
│   ├── imgui/
│   ├── vk-bootstrap/
│   ├── volk/
│   ├── Vulkan-Headers/
│   ├── VulkanMemoryAllocator/
│   └── # More to come
│ 
├── examples # How to use anvil
│   ├── HelloTriangle/      # Triangle vertices in shader, no buffers, no push contants
│   │   ├── HelloTriangle.h/.cpp
│   │   ├── HelloTriangle.slang
│   │   └── main.cpp
│   ├── HelloCube/          # Hard coded cube, buffer creation, rotation push constants
│   │   ├── HelloCube.h/.cpp
│   │   ├── HelloCube.slang
│   │   └── main.cpp
│   ├── glTFBox/            # Load model from gltf, mesh buffers
│   │   ├── Box/ # glTF model
│   │   ├── BoxModel.h/.cpp
│   │   ├── BoxModel.slang
│   │   └── main.cpp
│   └── # More to come   
│      
└── source/
    ├── main.cpp
    ├── core/       # Engine lifecycle and OS-level stuff
    │   ├── AnvilApplication.h/.cpp
    │   ├── AnvilInput.h/.cpp
    │   └── AnvilWindow.h/.cpp
    │
    ├──utilities/
    │   ├── AnvilFileIO.h/.cpp
    │   ├── AnvilMeshLoader.h/.cpp
    │   ├── AnvilShaderCompiler.h/.cpp
    │   ├── AnvilShaders.h/.cpp
    │   └── AnvilUILogger.h/.cpp
    │
    └── vulkan/     # The Vulkan abstraction layers
        ├── AnvilVulkanDebug.h/.cpp
        ├── AnvilVulkanContext.h/.cpp    # Instance, Device, VMA allocator
        ├── AnvilUIRenderer.h/.cpp
        ├── AnvilSwapchain.h/.cpp        # Swapchain and recreation logic
        ├── AnvilShaderModule.h/.cpp     # Shader modules
        ├── AnvilRenderer.h/.cpp         # Command buffers, sync structures, draw loop
        ├── AnvilPipeline.h/.cpp         # Shader loading and VkPipeline creation
        ├── AnvilMeshBuffer.h/.cpp       
        ├── AnvilDeletionQueue.h         # Pattern for safe resource cleanup
        └── AnvilBuffer.h/.cpp           

```

## Naming Conventions

- directories - `camelCase`
- files - `PascalCase`

- namespace - `PascalCase`
- classes - `PascalCase`
- struct - `PascalCase`

- global - `CAPITALCASE`
- \#defines - `MACRO_CASE`
- enums name - `PascalCase`
- enums values - `MACRO_CASE` or WEIRD stuff

- class functions - `camelCase`
- individual functions - `snake_case`
- static function - `PascalCase`
- namespace function - `PascalCase`

- variables:
  - class member- `camelCase`
  - struct member- `camelCase`
  - local- `snake_case`

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
