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
│   ├── CMakeLists.txt
│   │ 
│   ├── cgltf/
│   ├── glfw/
│   ├── glm/
│   ├── imgui/
│   ├── renderdoc/
│   ├── slang/                   # Run fetch_slang.py to get the directory
│   ├── stb/
│   ├── tracy/                   # Run fetch_tracy.py to get the directory
│   ├── vk-bootstrap/
│   ├── volk/
│   ├── Vulkan-Headers/
│   ├── VulkanMemoryAllocator/
│   └── # More to come
│ 
├── examples/                    # How to use anvil
│   ├── CMakeLists.txt
│   │ 
│   ├── HelloTriangle/           # Triangle vertices in shader, no buffers, no push contants
│   ├── HelloCube/               # Hard coded cube, buffer creation, rotation push constants
│   ├── glTFBox/                 # Load model from gltf, mesh buffers
│   ├── TextureCube/             # Load texture from gltf, map uv
│   ├── ShaderReflectionCube/    # Same as TextureCube, but using shader reflection instead
│   ├── DirectionalLight/        # Load model, set up scene lighting and uniform buffers
│   ├── glTFTruck
│   ├── BoxAnimated/        
│   ├── RiggedSimple/        
│   ├── CesiumMan/        
│   ├── Sponza/        
│   ├── SponzaDeferred/        
│   ├── PBRTests/        
│   └── # More to come   
│ 
├── projects/
│   ├── CMakeLists.txt
│   │ 
│   ├── RenderGraphStub/
│   └── # More to come
│ 
├── scripts/          
│   ├── consolidate_code_one_file.py
│   ├── fetch_slang.py
│   ├── fetch_tracy.py
│   └── # More to come
│   
├── tools/
│   ├── tracy/
│   └── # More to come
│   
├── shaders/
│   ├── shared/
│   │   ├── DebugModes.h
│   │   ├── PushConstants.h
│   │   └── # More to come
│   │
│   ├── DebugDeferred.slang
│   ├── DebugForward.slang
│   ├── PBR.slang
│   └── # More to come
│      
└── source/
    │
    ├── core/
    │   ├── Anvil.cpp
    │   ├── Anvil.h
    │   ├── Console.cpp
    │   ├── Console.h
    │   ├── EngineConfig.h
    │   ├── Ensure.h
    │   ├── Input.cpp
    │   ├── Input.h
    │   ├── Logger.cpp
    │   ├── Logger.h
    │   ├── Window.cpp
    │   └── Window.h
    │
    ├── diagnostics/
    │   ├── FrameStats.h
    │   ├── GPUProfiler.cpp
    │   ├── GPUProfiler.h
    │   ├── RenderDoc.cpp
    │   ├── RenderDoc.h
    │   ├── Trace.h
    │   └── # More to come
    │
    ├── graphics/
    │   ├── DebugNames.cpp
    │   ├── DebugNames.h
    │   ├── PipelineBuilder.cpp
    │   ├── PipelineBuilder.h
    │   ├── Swapchain.cpp
    │   ├── Swapchain.h
    │   ├── VulkanConfig.h
    │   ├── VulkanContext.cpp
    │   ├── VulkanContext.h
    │   ├── VulkanResult.cpp
    │   └── VulkanResult.h
    │
    ├── math/
    │   ├── Frustum.h
    │   ├── Index32.h
    │   ├── stox.cpp
    │   ├── stox.h
    │   └── # More to come
    │
    ├── rendercore/
    │   ├── CPUModel.cpp
    │   ├── CPUModel.h
    │   ├── GPUModel.cpp
    │   ├── GPUModel.h
    │   ├── Material.cpp
    │   ├── Material.h
    │   ├── MaterialInstance.cpp
    │   ├── MaterialInstance.h
    │   ├── ShaderCompiler.cpp
    │   ├── ShaderCompiler.h
    │   ├── ShaderProgram.cpp
    │   ├── ShaderProgram.h
    │   ├── Shaders.cpp
    │   └── Shaders.h
    │
    ├── renderer/
    │   ├── DebugPass.cpp
    │   ├── DebugPass.h
    │   ├── Renderer.cpp
    │   ├── Renderer.h
    │   ├── RenderGraph.cpp
    │   └── RenderGraph.h
    │
    ├── resources/
    │   ├── GBuffer.cpp
    │   ├── GBuffer.h
    │   ├── GPUBuffer.cpp
    │   ├── GPUBuffer.h
    │   ├── GPUMesh.cpp
    │   ├── GPUMesh.h
    │   ├── GPUTexture.cpp
    │   ├── GPUTexture.h
    │   ├── ShaderModule.cpp
    │   └── ShaderModule.h
    │
    ├── scene/
    │    ├── Camera.cpp
    │    ├── Camera.h
    │    ├── Scene.cpp
    │    ├── Scene.h
    │    ├── SceneConfig.h
    │    ├── SceneManager.cpp
    │    └── SceneManager.h
    │
    └── ui/
         ├── ScreenLogger.cpp
         ├── ScreenLogger.h
         ├── UIElements.cpp
         ├── UIElements.h
         ├── UIRenderer.cpp
         └── UIRenderer.h
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
- free functions - `snake_case`
- static function - `PascalCase`
- namespace function - `PascalCase`
- anon namespace function - `snake_case`

- variables:
  - class member - `camelCase`
  - struct member - `camelCase`
  - local - `snake_case`

- iterators should be named.
  - e.g., if iterating to index into image, the iteration should be called: `image_index`.

- avoid naming vulkan info structs just `info` and prefer `image_info`.

---

## Separation of Context

*This section is currently being restructured as we are changing some architectural 
structure of anvil and core vulkan resources.*

### Descriptor Sets

Currently, anvil crams global scene data, model-specific bone matrices, and 
material-specific textures into a single Vulkan Descriptor Set (`Set 0`). 
We are going to split these into three frequencies:

* **Set 0:** Global Data (Scene UBO) — Managed by the Project (`CesiumMan` / `Sponza`).
* **Set 1:** Model Data (Joint Matrices) — Managed by `GPUModel`.
* **Set 2:** Material Data (Textures) — Managed by the individual `GPUModelMaterial` instances.

---

## Documentation

### Doxygen Code Comments

- @brief
- @params
- @todo
- @bug
- @attention
- @warning
