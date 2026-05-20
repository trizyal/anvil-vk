# Development Guide

## Project Structure

```
anvil-vk/
│
├── CMakeLists.txt
│
├── external/
│   ├── glfw/
│   ├── vk-bootstrap/
│   ├── volk/
│   ├── Vulkan-Headers/
│   ├── VulkanMemoryAllocator/
│   └── # More to come
│ 
├── source/
│   ├── main.cpp
│   ├── core/               # Engine lifecycle and OS-level stuff
│   │   ├── Application.h/.cpp
│   │   └── Window.h/.cpp
│   │
│   ├── vulkan/             # The Vulkan abstraction layers
│   │   ├── VulkanContext.h/.cpp    # Instance, Device, VMA allocator
│   │   ├── Swapchain.h/.cpp        # Swapchain and recreation logic
│   │   ├── Renderer.h/.cpp         # Command buffers, sync structures, draw loop
│   │   ├── Pipeline.h/.cpp         # Shader loading and VkPipeline creation
│   │   └── DeletionQueue.h         # Pattern for safe resource cleanup
│   │
│   └── utils/
│       └── FileIO.h/.cpp           # Helper to read compiled .spv files
│ 
└── shaders/ # Raw GLSL files (.vert, .frag)
```

## Naming Conventions

- directories - `camelCase`
- files - `camelCase`
- namespace - `lowercase`
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