# Anvil Example Projects

This directory contains standalone sample applications that demonstrate how to use the Anvil Vulkan framework.
Each example increases in complexity to show different aspects of the pipeline.

## Example Projects

### 01. HelloTriangle

- **Path:** [HelloTriangle](HelloTriangle)
- **Focus:** Basic pipeline setup and shader execution.
- **Description:** Renders a single triangle with vertex data hardcoded directly inside the Slang vertex shader. Demonstrates basic render loop orchestration, explicit `ShaderProgram` creation, and `AnvilMaterial` usage without requiring external vertex buffers or push constants.

### 02. HelloCube

- **Path:** [HelloCube](HelloCube)
- **Focus:** GPU buffers and push constants.
- **Description:** Renders a 3D cube by manually defining vertex and index arrays and uploading them to a `GPUBuffer`. Uses push constants to pass rotation matrices from the CPU to the vertex shader each frame, and reads a basic global `Scene` uniform buffer to tint the cube.

### 03. glTFBox
- **Path:** [glTFBox](glTFBox)
- **Focus:** Asset loading and indexed meshes.
- **Description:** Replaces manual vertex definition by loading a `.gltf` model from disk using `CPUModel`. Converts the parsed data into device-local `GPUMesh` buffers via the `GPUModel` abstraction for efficient rendering.

### 04. TextureCube

- **Path:** [TextureCube](TextureCube)
- **Focus:** Texture loading and descriptor images.
- **Description:** Introduces image loading using `stb_image` integrated into `GPUTexture`. Demonstrates generating mipmaps, transitioning image layouts, and binding image samplers to explicit descriptor sets to map textures onto glTF UV coordinates.

### 05. ShaderReflectionCube

- **Path:** [ShaderReflectionCube](ShaderReflectionCube)
- **Focus:** Automated shader reflection and multi-set descriptor layouts.
- **Description:** Uses the Slang API within `ShaderProgram` to reflect shader parameters dynamically. Demonstrates `AnvilMaterial` automatically generating `VkDescriptorSetLayout` and `VkPipelineLayout` bindings, eliminating the need for manual Vulkan descriptor configuration.

### 06. DirectionalLight

- **Path:** [DirectionalLight](DirectionalLight)
- **Focus:** Basic lighting and Scene UBOs.
- **Description:** Introduces the `GlobalSceneData` struct and `Scene` buffer to pass lighting direction, light color, and ambient color to the GPU. Uses glTF normal data in the fragment shader to calculate simple diffuse (N dot L) directional lighting.

### 07. glTFTruck

- **Path:** [glTFTruck](glTFTruck)
- **Focus:** Complex meshes, scene graph nodes, and Material Instances.
- **Description:** Demonstrates loading a multi-part glTF file with parent-child transform nodes. Uses `GPUModel` to flatten the node graph, compute absolute world matrices, and bind individual `MaterialInstance` descriptor sets for different mesh primitives.

### 08. BoxAnimated

- **Path:** [BoxAnimated](BoxAnimated)
- **Focus:** Node-based glTF animations.
- **Description:** Introduces Translation, Rotation, and Scale (TRS) animations. Demonstrates reading animation channels and keyframes from `CPUModel`, interpolating values based on delta time, and dynamically updating the model matrix SSBO via `GPUModel`.

### 09. RiggedSimple

- **Path:** [RiggedSimple](RiggedSimple)
- **Focus:** Basic skeletal animation and joint matrices.
- **Description:** Introduces vertex bone weights and joint indices. Shows how to compute inverse bind matrices combined with animated node transforms to deform a rigged mesh, passing the calculated joint arrays to the GPU via a Storage Buffer (SSBO).

### 10. CesiumMan

- **Path:** [CesiumMan](CesiumMan)
- **Focus:** Comprehensive skeletal animation and texturing.
- **Description:** Combines texturing, complex skeletal skinning, and glTF animations into a single scene. Demonstrates a fully animated and textured character model walking, utilizing the complete multi-set descriptor architecture (Scene, Model, and Material sets).

### 11. Sponza

- **Path:** [Sponza](Sponza)
- **Focus:** Large-scale scene loading and Forward PBR rendering.
- **Description:** Loads the classic Sponza atrium to test engine performance and frustum culling. Demonstrates multi-material handling, normal mapping with tangent space calculations, and a full Forward Physically Based Rendering (PBR) pipeline utilizing metallic-roughness workflows.

### 12. SponzaDeferred

- **Path:** [SponzaDeferred](SponzaDeferred)
- **Focus:** Deferred rendering architecture.
- **Description:** Replaces the forward pass with a deferred rendering pipeline. Populates a G-Buffer (Albedo, Normals, PBR parameters, World Position, Depth) during the geometry pass, and resolves the final lighting equation in a separate fullscreen pass.

### 13. PBRTests

- **Path:** [PBRTests](PBRTests)
- **Focus:** Physically Based Rendering validation.
- **Description:** Renders a grid of spheres with varying metallic and roughness values to visually validate the engine's BRDF, Fresnel (Schlick), geometry (Smith), and normal distribution (GGX) shader implementations against standard PBR lighting models.

---
