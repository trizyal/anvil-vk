# anvil-vk

Anvil is a boilerplate framework and template repository for Vulkan rendering in C++.

It is designed to strip away the initial overhead of Vulkan setup—such as instance bootstrapping, 
swapchain management, synchronization, and basic pipeline compilation—while remaining modular 
enough for developers to build their own custom renderers or engines on top.

## Intention

Anvil is intended to be a starting point rather than a rigid, all-in-one renderer or game engine.

- **Decoupled Backend:** The core `vulkan/` abstraction layer is independent of scene graphs, camera logic, or UI frameworks.
- **Modern Tooling:** Uses Slang for shader compilation and standard C++ RAII patterns for Vulkan resource management.
- **Template First:** Examples are separated from the engine source so the core codebase can be extracted easily into standalone projects.



## Building from Source

### Prerequisites

- CMake 3.20 or newer
- A C++20 compatible compiler (MSVC, Clang, or GCC)
- Python 3 (required for fetching the Slang shader compiler binaries)

### Build Instructions

1. Clone the repository

```bash
git clone git@github.com:trizyal/anvil-vk.git
cd anvil-vk
```

2. Fetch Slang binaries into the `external/` directory:

```bash
python fetch_slang.py
```

3. Configure and build using CMake: (NEED TO UPDATE)

```bash
cmake -B build
cmake --build build --config Release
```

## Examples

The `examples/` directory contains sample projects demonstrating how to use the framework, 
ranging from simple hardcoded geometry to loading glTF scenes.

See [examples](examples/README.md) for detailed 
descriptions of each example and third-party asset licenses.

## License

Copyright (C) 2026 trizyal.

This project is licensed under the GNU General Public License v3.0 (GPL-3.0-only). 
See [LICENSE](LICENSE) for more details.

*Copyright (C) 2026 trizyal. This project is licensed under the GPLv3.*
