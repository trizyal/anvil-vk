# Anvil Example Projects

This directory contains standalone sample applications that demonstrate how to use the Anvil Vulkan framework. 
Each example increases in complexity to show different aspects of the pipeline.

## Example Projects

### 01. HelloTriangle
- **Path:** [HelloTriangle](HelloTriangle)
- **Focus:** Basic pipeline setup and shader execution.
- **Description:** Renders a single triangle with vertex data hardcoded directly inside the Slang vertex shader. 
Demonstrates basic render loop orchestration without vertex buffers or push constants.

### 02. HelloCube
- **Path:** [HelloCube](HelloCube)
- **Focus:** GPU buffers and push constants.
- **Description:** Renders a 3D cube using an `AnvilBuffer` for vertex data. Uses push constants to pass rotation 
matrices from the CPU to the vertex shader each frame.

### 03. glTFBox
- **Path:** [glTFBox](glTFBox)
- **Focus:** Asset loading and indexed meshes.
- **Description:** Loads a `.gltf` model from disk using `AnvilMeshLoader` and populates an `AnvilMeshBuffer` with vertex and index data.

### 04. TextureCube
- **Path:** [TextureCube](TextureCube)
- **Focus:** Texture loading and DescriptorImages
- **Description:** Loads a texture using the `AnvilTextureLoader` and uv coordinates from the glTF file.

### 05. ShaderReflectionCube
- **Path:** [ShaderReflectionCube](ShaderReflectionCube)
- **Focus:** Create shader reflection and load data using that.
- **Description:** Uses `AnvilMaterial` to get all descriptor set, shader modules and mesh data into a material. Then
uses reflected data to bind descriptors.

### 06. DirectionalLight
- **Path:** [DirectionalLight](DirectionalLight)
- **Focus:** Directional light shader and scene uniform buffer.
- **Description:** Uniform buffer creation. Read normals from glTF file. Directional Light shaders doing a simple lighting equation. 
Passing data via a uniform buffer `SceneData` with light direction, color and ambient color.

### 07. glTFTruck
- **Path:** [glTFTruck](glTFTruck)
- **Focus:** Complex meshes with scene nodes.
- **Description:** Material refactored into material instance. Added simple rotation animation.

### 08. BoxAnimated
- **Path:** [BoxAnimated](BoxAnimated)
- **Focus:** Simple shapes with animation.
- **Description:** 

### 09. RiggedSimple
- **Path:** [RiggedSimple](RiggedSimple)
- **Focus:** Rigging a simple model.
- **Description:**

### 10. CesiumMan
- **Path:** [CesiumMan](CesiumMan)
- **Focus:** Texturing, skeletal skinning matrices, SSBO joint buffers, and animations together.
- **Description:**

---

## Project Asset Licenses

| Asset Name   | Used In   | Author / Source                  | License   |
|:-------------|:----------|:---------------------------------|:----------|
| **Box.gltf** | `glTFBox` | Khronos Group glTF Sample Models | CC BY 4.0 |
