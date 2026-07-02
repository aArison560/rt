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

## Project State — Mostly Stubs

Most `.cpp` files have **stub implementations** marked `// TODO:`. What is actually implemented:
- **Cylinder** / **Cone** — full intersection, normal, UV (`src/geometry/`)
- **Window** — SDL2 init + display (`src/platform/`)
- **Material**, **HitRecord**, **Scene**, **AObject** — data plumbing (ctors, getters, setters, assignment)
- **Ray** — constructors, `pointAt()`

Still stubs: `Vec3`, `Matrix4x4`, `Transform`, `Camera`, `Renderer`, `SceneParser`, `Sphere`, `Plane`.

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

- `tests/` has **one file**: `.gitkeep` — no tests exist.
- The `make test` target builds `./rt_test` linking all `.o` files (not a dedicated test runner). If you add a test framework, configure it separately.

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
