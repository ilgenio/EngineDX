# DirectX 12 Rendering Framework

## Overview

This project is a modular DirectX 12 rendering framework written in C++. It provides a foundation for real-time 3D graphics applications, supporting modern rendering techniques, resource management, and extensibility for exercises and demos. The framework is designed for educational and experimental purposes, making it suitable for learning DirectX 12 and advanced rendering concepts.

## Features

- **DirectX 12 Core**: Device, swap chain, command queue, and resource management.
- **Scene Management**: Load and manage 3D scenes, meshes, materials, and skyboxes (supports glTF format).
- **Camera System**: Interactive camera controls with mouse, keyboard, and gamepad support.
- **Shader Management**: Modular shader and descriptor management for flexible rendering pipelines.
- **Render Passes**: Support for multiple render passes (e.g., skybox, irradiance map, prefilter environment map).
- **ImGui Integration**: Built-in support for ImGui docking for real-time UI and debugging.
- **Resource Handling**: Efficient GPU resource and descriptor heap management.
- **Sample Exercises**: Includes a set of exercises and demos to illustrate various rendering techniques.

## Directory Structure

- `Exercises/` — Example exercises and shaders for learning and experimentation.
- `3rdParty/` — Third-party libraries (e.g., ImGui, tinygltf, DirectX headers).
- `Shaders/` — HLSL shaders for different rendering passes.
- `EngineDX.cpp/h` — Main engine logic and entry point.
- `Module*.cpp/h` — Modular components for rendering, input, resources, etc.
- `Scene.cpp/h` — Scene graph and asset management.
- `Demo*.cpp/h` — Example demos and rendering techniques.

## Getting Started

### Prerequisites

- **Windows 10/11**
- **Visual Studio 2022** (with Desktop development with C++ workload)
- **DirectX 12 compatible GPU**

### Building

1. Open the solution in Visual Studio 2022.
2. Restore NuGet packages if prompted.
3. Build the solution (`Ctrl+Shift+B`).

### Running

- Set the desired demo or exercise as the startup project.
- Run (`F5`) to launch the application.

## Usage

- Use mouse, keyboard, or gamepad to control the camera and interact with the scene.
- Modify or add exercises in the `Exercises/` folder to experiment with new rendering techniques.
- Use ImGui panels for debugging and real-time parameter tweaking.

## Dependencies

- [DirectX 12](https://docs.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-graphics)
- [ImGui](https://github.com/ocornut/imgui) (docking branch)
- [tinygltf](https://github.com/syoyo/tinygltf)

## License

This project is intended for educational and research purposes. Please refer to individual third-party library licenses for their respective terms.

## Credits

- DirectX 12 samples and documentation by Microsoft.
- ImGui by Omar Cornut.
- tinygltf by Syoyo Fujita.
