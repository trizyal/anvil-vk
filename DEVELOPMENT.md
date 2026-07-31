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
│   ├── HelloCube/          # Hard coded cube, buffer creation, rotation push constants
│   ├── glTFBox/            # Load model from gltf, mesh buffers
│   ├── TextureCube/        # Load texture from gltf, map uv
│   ├── ShaderReflectionCube/            # Same as TextureCube, but using shader reflection instead
│   └── # More to come   
│      
└── source/
    ├── core/       # Engine lifecycle and OS-level stuff
    │   ├── AnvilApplication.h/.cpp #Commented
    │   ├── AnvilInput.h/.cpp #Commented
    │   └── AnvilWindow.h/.cpp #Commented
    │
    ├── scene/       
    │   └── AnvilCamera.h/.cpp #Commented
    │
    ├──utilities/
    │   ├── AnvilModelLoader.h/.cpp #Commented
    │   ├── AnvilShaderCompiler.h/.cpp #Commented
    │   ├── AnvilShaders.h/.cpp #Commented
    │   └── AnvilUILogger.h/.cpp #Commented
    │
    └── vulkan/     # The Vulkan abstraction layers
        ├── AnvilResult.h/.cpp
        ├── AnvilVulkanDebug.h/.cpp
        ├── AnvilVulkanContext.h/.cpp
        ├── AnvilUIRenderer.h/.cpp
        ├── AnvilTextureLoader.h/.cpp
        ├── AnvilSwapchain.h/.cpp     
        ├── AnvilShaderModule.h/.cpp   
        ├── AnvilRenderer.h/.cpp    #Commented
        ├── AnvilPipeline.h/.cpp   
        ├── AnvilMeshBuffer.h/.cpp  #Commented    
        ├── AnvilMaterial.h/.cpp  #Commented
        ├── AnvilDeletionQueue.h 
        └── AnvilBuffer.h/.cpp   #Commented    

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
