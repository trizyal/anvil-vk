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
- Directional lighting support
- Uniform Buffers support
- Complex glTF models support

### Work in Progress (WIP)

- Normals and normal maps

### Future

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
│   ├── imgui/
│   ├── slang/                   # Run fetch_slang.py to get the directory
│   ├── vk-bootstrap/
│   ├── volk/
│   ├── Vulkan-Headers/
│   ├── VulkanMemoryAllocator/
│   └── # More to come
│ 
├── examples/                    # How to use anvil
│   ├── HelloTriangle/           # Triangle vertices in shader, no buffers, no push contants
│   ├── HelloCube/               # Hard coded cube, buffer creation, rotation push constants
│   ├── glTFBox/                 # Load model from gltf, mesh buffers
│   ├── TextureCube/             # Load texture from gltf, map uv
│   ├── ShaderReflectionCube/    # Same as TextureCube, but using shader reflection instead
│   ├── DirectionalLight/        # Load model, set up scene lighting and uniform buffers
│   ├── glTFTruck
│   ├── BoxAnimated/        
│   ├── RiggedSimple/        
│   └── # More to come   
│      
└── source/
    │
    ├── core/
    │   ├── Anvil.cpp
    │   ├── Anvi.h
    │   ├── Input.cpp
    │   ├── Input.h
    │   ├── Window.cpp
    │   └── Window.h
    │
    ├── rendercore/
    │   ├── AnvilMaterial.cpp
    │   ├── AnvilMaterial.h
    │   ├── AnvilShaders.cpp
    │   ├── AnvilShaders.h
    │   ├── CPUModel.cpp
    │   ├── CPUModel.h
    │   ├── GPUMesh.cpp
    │   ├── GPUMesh.h
    │   ├── GPUModel.cpp
    │   ├── GPUModel.h
    │   ├── MaterialInstance.cpp
    │   ├── MaterialInstance.h
    │   ├── ShaderCompiler.cpp
    │   ├── ShaderCompiler.h
    │   ├── TextureLoader.cpp
    │   ├── TextureLoader.h
    │   │
    │   ├── context/
    │   │   ├── DebugNames.cpp
    │   │   ├── DebugNames.h
    │   │   ├── Swapchain.cpp
    │   │   ├── Swapchain.h
    │   │   ├── VulkanConfig.h
    │   │   ├── VulkanContext.cpp
    │   │   ├── VulkanContext.h
    │   │   ├── VulkanResult.cpp
    │   │   └── VulkanResult.h
    │   │
    │   └── resources/
    │       ├── GPUBuffer.cpp
    │       ├── GPUBuffer.h
    │       ├── PipelineBuilder.cpp
    │       ├── PipelineBuilder.h
    │       ├── ShaderModule.cpp
    │       └── ShaderModule.h
    │
    ├── renderer/
    │   ├── AnvilRenderer.cpp
    │   ├── AnvilRenderer.h
    │   ├── FrameStats.h
    │   ├── ScreenLogger.cpp
    │   ├── ScreenLogger.h
    │   ├── UIElements.cpp
    │   ├── UIElements.h
    │   ├── UIRenderer.cpp
    │   └── UIRenderer.h
    │
    └── scene/
        ├── Camera.cpp
        ├── Camera.h
        ├── Scene.cpp
        └── Scene.h
```

## Conventions

### Naming

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
  - class member - `camelCase`
  - struct member - `camelCase`
  - local - `snake_case`

---

## Separation of Context

*This section is currently being restructured as we are changing some architectural 
structure of anvil and core vulkan resources.*

---

## Documentation

### Doxygen Code Comments

- @brief
- @params
- @todo
- @bug
- @attention
- @warning
