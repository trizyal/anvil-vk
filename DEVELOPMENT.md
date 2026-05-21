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
│   ├── vk-bootstrap/
│   ├── volk/
│   ├── Vulkan-Headers/
│   ├── VulkanMemoryAllocator/
│   └── # More to come
│ 
├── source/
│   ├── main.cpp
│   ├── core/               # Engine lifecycle and OS-level stuff
│   │   ├── AnvilApplication.h/.cpp
│   │   └── AnvilWindow.h/.cpp
│   │
│   ├── vulkan/             # The Vulkan abstraction layers
│   │   ├── AnvilVulkanContext.h/.cpp    # Instance, Device, VMA allocator
│   │   ├── AnvilSwapchain.h/.cpp        # Swapchain and recreation logic
│   │   ├── AnvilRenderer.h/.cpp         # Command buffers, sync structures, draw loop
│   │   ├── AnvilPipeline.h/.cpp         # Shader loading and VkPipeline creation
│   │   └── AnvilDeletionQueue.h         # Pattern for safe resource cleanup
│   │
│   └── utilities/
│       └── AnvilFileIO.h/.cpp           # Helper to read compiled .spv files
│ 
└── shaders/ # Raw GLSL files (.vert, .frag)
```

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