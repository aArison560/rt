# RT - Ray Tracer

A modern C++23 CPU-based ray tracing engine with SDL2 graphics library.

## Features

- **Ray Casting & Tracing**: Full ray tracing with recursive reflections and refractions
- **Geometric Primitives**: Sphere, Plane, Cylinder, Cone with transformation support
- **Lighting Model**: Phong illumination with ambient, diffuse, and specular components
- **Light Sources**: Point lights, directional lights, ambient lighting
- **Shadows**: Real-time shadow casting with configurable samples
- **Transformations**: Full 3D transformation support (translation, rotation, scaling)
- **Camera System**: Perspective camera with interactive controls
- **Texture Support**: PNG and JPEG texture loading and UV mapping
- **SDL2 Rendering**: Hardware-accelerated window display
- **Scene Parser**: Custom .rt scene configuration file format

## Build Requirements

### Dependencies
- **C++ Compiler**: GCC/Clang with C++23 support
- **SDL2**: Graphics library (`libSDL2-dev`)
- **libpng**: PNG image support
- **libjpeg**: JPEG image support
- **libm**: Math library (standard)

### Installation (Ubuntu/Debian)
```bash
sudo apt-get install libsdl2-dev libpng-dev libjpeg-dev
```

### Installation (macOS)
```bash
brew install sdl2 libpng jpeg
```

## Building

```bash
# Build the project
make

# Clean build artifacts
make clean

# Clean everything and rebuild
make fclean
make all

# Rebuild from scratch
make re
```

## Usage

```bash
# Run with default scene
./rt

# Run with custom scene
./rt scenes/my_scene.rt

# Specify resolution
./rt scenes/my_scene.rt 1920 1080

# Get help
./rt --help
```

## Scene File Format

Create `.rt` files in the `scenes/` directory:

```
# Comments start with #

# Background color
bg 50,50,100

# Ambient light: intensity, color
A 0.2 255,255,255

# Point light: position, color, intensity
L 10,10,10 255,255,255 0.8

# Sphere: center, radius, color
sp 0,0,-5 2 255,0,0

# Plane: point, normal, color
pl 0,-1,0 0,1,0 128,128,128

# Cylinder: center, axis, radius, height, color
cy 0,0,-10 0,1,0 1.5 4 0,255,0

# Cone: apex, axis, half-angle(degrees), height, color
co 0,0,-8 0,1,0 30 5 0,0,255

# Camera: position, direction, FOV(degrees)
c 5,3,10 -0.5,-0.5,-1 45
```

## Project Structure

```
rt/
├── Makefile              # Build configuration
├── README.md
├── include/              # Header files (.hpp)
│   ├── core/             # Vec3, Ray, Matrix4x4, Material, HitRecord
│   ├── geometry/         # AObject and primitives (Sphere, Plane, ...)
│   ├── lighting/         # ALight and light types
│   ├── scene/            # Camera, Scene, SceneParser, Transform
│   ├── rendering/        # Renderer, ImageBuffer, Texture
│   └── platform/         # Window, EventHandler (SDL integration)
├── src/                  # Implementation files (.cpp)
│   ├── app/              # main.cpp
│   ├── core/
│   ├── geometry/
│   ├── lighting/
│   ├── scene/
│   ├── rendering/
│   └── platform/
├── scenes/              # Scene configuration files (.rt)
├── textures/            # Texture assets
└── tests/              # Unit tests
```

Headers should be included with their module path, for example:

```cpp
#include "core/Vec3.hpp"
#include "scene/Scene.hpp"
#include "rendering/Renderer.hpp"
```

## Development

Three-developer collaboration with clear workstream separation.

## Build
```bash
make
```

## Memory Management

- No memory leaks allowed
- Smart pointers used throughout
- RAII pattern for all resources