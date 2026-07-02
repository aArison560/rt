# RT — Ray Tracer (C++23)

## Build & Commands

| Command | Action |
|---------|--------|
| `make` | Build `./rt` with `-Wall -Wextra -Werror -O2` |
| `make test` | Build `./rt_test` (test binary, **no** strict warning flags) |
| `make clean` / `make fclean` | Remove objects / all artifacts |
| `make re` / `make retest` | Clean rebuild (main / test) |
| `./rt scenes/foo.rt [w] [h]` | Run with scene file |
| `valgrind --leak-check=full ./rt scenes/simple_spheres.rt 100 100` | Memory check |

**Dependencies**: `libsdl2-dev`, `libpng-dev`, `libjpeg-dev`.

## Project State — Fully implemented

All modules are **fully implemented**. No stubs remain.

### What was implemented

| Module | Status |
|--------|--------|
| **Core** — Vec3, Ray, Matrix4x4, HitRecord, Material | Arithmetic, reflect/refract, transforms, data plumbing |
| **Geometry** — Sphere, Plane, Cylinder, Cone | Full intersection, normals, UV, bounding boxes |
| **Lighting** — AmbientLight, PointLight, DirectionalLight | Phong-ready with attenuation |
| **Scene** — Camera, Scene, SceneParser, Transform | Ray generation, `.rt` file parsing (all directives), camera movement |
| **Renderer** — Renderer, ImageBuffer | Full ray tracing with shadows, reflections, refractions, PNG export |
| **Platform** — Window, EventHandler (SDL2) | Display with WASD camera controls, resize, ESC quit |

### `.rt` Scene Format — All Directives

| Directive | Example | Description |
|-----------|---------|-------------|
| `bg r g b` | `bg 0.3 0.3 0.35` | Background color |
| `A r g b` | `A 0.4 0.4 0.4` | Ambient light (or `A intensity` for white) |
| `L px py pz r g b intensity` | `L 8.0 8.0 8.0 1.0 1.0 0.9 1.0` | Point light |
| `directional dx dy dz r g b intensity` | `directional 1.0 1.0 0.5 1.0 1.0 0.8 0.95` | Directional light |
| `c px py pz lx ly lz ux uy uz fov` | `c 0.0 2.5 10.0 0.0 1.0 -1.2 0.0 1.0 0.0 45.0` | Camera (10 values) |
| `c px py pz lx ly lz fov` | `c 0.0 3.0 12.0 0.0 0.5 -2.0 40.0` | Camera (7 values, up=(0,1,0)) |
| `sp cx cy cz radius` | `sp 0.0 0.5 0.0 1.0` | Sphere |
| `pl px py pz nx ny nz` | `pl 0.0 -2.0 0.0 0.0 1.0 0.0` | Plane |
| `cy cx cy cz ax ay az radius height` | `cy 0.0 -1.5 -4.0 0.0 1.0 0.0 0.5 1.5` | Cylinder |
| `co ax ay az dx dy dz halfAngle height` | `co 2.5 -1.8 -6.0 0.0 1.0 0.0 25.0 1.8` | Cone |
| `material r g b amb diff spec shininess reflect` | `material 0.9 0.2 0.1 0.8 0.6 0.1 32.0 0.1` | Postfix material |

**Parser quirk**: `material` is **postfix** — applies to the most recently declared object.

### Runtime Controls

| Input | Action |
|-------|--------|
| **W/A/S/D** | Move camera forward/left/back/right |
| **Q/E** | Move camera down/up |
| **↑/↓/←/→** | Rotate camera pitch/yaw |
| **ESC** | Quit |
| **S** | Save screenshot (`screenshot_N.png`) |
| **R** | Force re-render |

### Reflections & Refractions

- Reflection: `ray.getDirection().reflect(normal)` via Vec3::reflect, weighted by `material.getReflectivity()`
- Refraction: `ray.getDirection().refract(normal, n1/n2, ...)` via Vec3::refract (Snell's law), weighted by `material.getTransparency()`
- Recursion depth capped at `maxRecursionDepth` (default 4)
- Total internal reflection: discriminant check in Vec3::refract

### Rendering Pipeline

`main` loop: poll SDL events → detect camera movement → `Renderer::render()` → `trace()` per pixel → `castRay()` finds closest hit → `calculateLighting()` (Phong) + optional reflection/refraction → `window.updateDisplay()`

## Architecture

- **Entrypoint**: `src/app/main.cpp` → includes `"scene/Scene.hpp"` etc.
- **Module layout** (include/ and src/ mirror this structure):
  - `core/` — Vec3, Ray, Matrix4x4, HitRecord, Material
  - `geometry/` — AObject base + Sphere, Plane, Cylinder, Cone
  - `lighting/` — ALight base + PointLight, DirectionalLight, AmbientLight
  - `scene/` — Camera, Scene, SceneParser, Transform
  - `rendering/` — Renderer, ImageBuffer, Texture
  - `platform/` — Window, EventHandler (SDL2)
- **Include style**: `#include "module/File.hpp"` (path relative to `include/`)
- **All includes use `#pragma once`**.
- **Smart pointers throughout** (`std::shared_ptr` for objects, lights, materials).
- **`Vec3::EPSILON = 1e-6`** for float comparisons.

## Scene File Format (`.rt`)

Custom grammar in `scenes/`. Example directives: `bg`, `A`, `L`, `sp`, `pl`, `cy`, `co`, `c`, `material`. 5 example scenes provided. Parsed by `SceneParser`.

**Parser quirk**: The `material` directive is **postfix** — it applies to the most recently declared object. The parser must track the last created object to attach the material to it.

## Testing

- `tests/` has **`test_main.cpp`** with 13 tests (Cylinder + Cone intersection, normals, caps).
- `make test` → `./rt_test` — test binary (excludes `src/app/main.cpp`, includes `tests/*.cpp`).

## Git Conventions (from docs)

- **Branch naming**: `<type>/<developer>/<feature>` — e.g. `feature/dev-a/vec3-implementation`
- **Commits**: `<type>(<scope>): <subject>` — e.g. `feat(dev-a): implement Vec3 dot product`
- **PR target**: merge into `develop`, then to `main` at releases
- **Three workstreams**: Dev A (math/camera), Dev B (geometry/objects), Dev C (rendering/parsing)

## Notable

- `.gitignore` ignores **all `rt*` executables** (matches both `rt` and `rt_test`).
- Build uses `-fPIC` even for executables (keep as-is to match Makefile).
- Doxygen comments on **all public methods** are expected.
- The `default.rt` scene contains a **complex demo scene** (not a minimal default); the `createDefaultScene()` in main is also a stub.
