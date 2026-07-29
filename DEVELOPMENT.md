# Development Guide

## Project Status & Features

### Implemented

- Vulkan instance, device, and swapchain bootstrapping (`vk-bootstrap`, `volk`)
- Automatic swapchain recreation on window resize
- Per-frame synchronization and command buffer orchestration
- Slang shader compilation (`.slang` to SPIR-V)
- Shader hot reloading
- Basic glTF 2.0 mesh loading (`cgltf`)
- ImGui integration and overlay rendering

### Work in Progress (WIP)

- Directional lighting support
- Normals and normal maps
- Uniform Buffers support
- Complex glTF models support

### Future

- Unified asset loading directory (`utilities`/`assets`)
- Dedicated UI subsystem directory
- Configurable INI file parsing for runtime settings

## Project Structure

*Project structure and file structure is in the process of changing. As a result, this page is not upto date.*

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

*This section is currently being restructured as we are changing some architectural 
structure of anvil and core vulkan resources.*

## Documentation

### Doxygen Code Comments

- @brief
- @params
- @todo
- @bug
- @attention
- @warning
