# VulkanCube (plus a triangle) :)

A high-performance, cross-platform Vulkan 1.x renderer demonstrating core graphics concepts, modern C++ patterns, and a **zero-filesystem dependency** architecture.

This project renders a 3D scene featuring multiple rotating cubes and a bouncing double-sided triangle, all textured using an embedded asset pipeline.

## 🚀 Key Features

* **Self-Contained Binary:** All assets (GLSL shaders and JPG textures) are compiled into SPIR-V and byte arrays at build time and embedded directly into the executable. No external asset folders required.
* **Modern Vulkan Pipeline:** Implements Vulkan instances, logical devices, swapchains, render passes, and graphics pipelines using the `vulkan.hpp` C++ bindings.
* **Strict Lifecycle Management:** Explicit RAII-based destruction order ensures zero validation layer errors during shutdown (Swapchain → Surface → Device → Instance).
* **Free-Fly Camera:** Includes a scriptable camera behavior for navigating the 3D scene.
* **Cross-Platform Build System:** Automated CMake pipeline that handles shader compilation (supporting both `glslc` and `glslangValidator`) and asset hex-embedding.

## 🛠 Tech Stack

* **Language:** C++17
* **Graphics API:** Vulkan (via `vulkan.hpp`)
* **Windowing:** GLFW
* **Math:** GLM
* **Image Loading:** stb_image (modified for memory-loading)
* **Build System:** CMake 3.16+

## 📁 Repository Structure

* `vulkan/`: Core Vulkan abstraction classes (Device, Instance, Swapchain, etc.)
* `src/`: Scene management, Mesh/Material definitions, and High-level Renderer logic.
* `shaders/`: Raw GLSL vertex and fragment shaders.
* `cmake/`: Custom scripts for SPIR-V compilation and binary asset embedding.
* `include/`: External headers and local project definitions.

## 🔨 Building and Running

### Prerequisites

* Vulkan SDK installed.
* A C++17 compatible compiler (MSVC, Clang, or GCC).
* CMake installed.

### Build Commands

```bash
# Clone the repository
git clone https://github.com/thunderawesome/vulkan-cube.git
cd vulkan-cube

# Configure and build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

```

### Running

Once built, you can run the executable from the `build` directory. Because assets are embedded, you can move the binary anywhere on your system and it will still run without needing the source folder.

```bash
# Windows
./build/Release/vulkan_cube.exe

# macOS/Linux
./build/vulkan_cube

```

## 🕹 Controls

* **ESC:** Exit the application gracefully.
* **WASD:** Navigate the Free-Fly camera.
* **Mouse:** Look around.

## 🧪 Stress Testing

The renderer supports an automated stress test mode via environment variables:

```bash
# Runs the renderer for exactly 1000 frames then exits
STRESS_FRAMES=1000 ./vulkan_cube

```
