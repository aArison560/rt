# Ray Tracer Project: Implementation Guide

## Quick Start for Each Developer

### For Dev A (Math & Camera Layer)

**Your responsibility**: All mathematical foundations and camera system

1. **Start with**: Phase 2 in [TASK_BACKLOG.md](TASK_BACKLOG.md)
2. **Branch**: `feature/dev-a/math-foundations`
3. **Key files to implement**:
    - `include/core/Vec3.hpp` → `src/core/Vec3.cpp`
    - `include/core/Ray.hpp` → `src/core/Ray.cpp`
    - `include/core/Matrix4x4.hpp` → `src/core/Matrix4x4.cpp`
    - `include/scene/Transform.hpp` → `src/scene/Transform.cpp`
    - `include/scene/Camera.hpp` → `src/scene/Camera.cpp`

4. **Acceptance criteria**:
   - All `make all` compiles without errors
   - Math operations produce geometrically correct results
   - Camera ray generation verified with unit tests

5. **Timeline**: Approximately 27 hours over 3 days

---

### For Dev B (Geometry & Objects Layer)

**Your responsibility**: All geometric primitives and intersection testing

1. **Start with**: Phase 4-5 in [TASK_BACKLOG.md](TASK_BACKLOG.md)
2. **Branch**: `feature/dev-b/geometry-objects`
3. **Key files to implement**:
    - `include/core/HitRecord.hpp` → `src/core/HitRecord.cpp`
    - `include/geometry/AObject.hpp` → `src/geometry/AObject.cpp`
    - `include/geometry/Sphere.hpp` → `src/geometry/Sphere.cpp`
    - `include/geometry/Plane.hpp` → `src/geometry/Plane.cpp`
    - `include/geometry/Cylinder.hpp` → `src/geometry/Cylinder.cpp`
    - `include/geometry/Cone.hpp` → `src/geometry/Cone.cpp`
    - `include/scene/Scene.hpp` → `src/scene/Scene.cpp`

4. **Test your work**:
   - Use scene files in `scenes/` to verify primitives
   - Check that all objects can be instantiated
   - Verify intersection detection logic

5. **Timeline**: Approximately 19 hours over 2.5 days

6. **Note**: Blocks Dev C until primitives are complete

---

### For Dev C (Rendering, Graphics, & Lighting)

**Your responsibility**: Ray tracing engine, display system, scene parsing

1. **Start with**: Phase 6-12 in [TASK_BACKLOG.md](TASK_BACKLOG.md)
2. **Branch**: `feature/dev-c/rendering-system`
3. **Key files to implement**:
    - `include/core/Material.hpp` → `src/core/Material.cpp`
    - `include/lighting/ALight.hpp` → `src/lighting/ALight.cpp`, PointLight, DirectionalLight, AmbientLight
    - `include/rendering/Renderer.hpp` → `src/rendering/Renderer.cpp` (main algorithm)
    - `include/rendering/ImageBuffer.hpp` → `src/rendering/ImageBuffer.cpp`
    - `include/platform/Window.hpp` → `src/platform/Window.cpp`
    - `include/platform/EventHandler.hpp` → `src/platform/EventHandler.cpp`
    - `include/scene/SceneParser.hpp` → `src/scene/SceneParser.cpp`
    - `src/app/main.cpp` (integration)

4. **Test your work**:
   - Begin with Phase 7 (basic raytracing without lighting)
   - Test scene loading from `.rt` files
   - Verify window displays rendered images

5. **Timeline**: Approximately 42 hours over 4-5 days

---

## Example Scene Files

Four example scenes are provided to test your implementation:

### 1. **simple_spheres.rt** - Basic Testing
- **Best for**: Testing fundamental sphere rendering and area lights
- **Features**: 3 spheres, 2 point lights, basic materials
- **Validation**: Shadows should appear correctly, sphere highlights visible

### 2. **cylinders_and_cones.rt** - Primitive Showcase
- **Best for**: Testing all 4 primitive types
- **Features**: Spheres, cylinders, cones, various transformations
- **Validation**: All objects render with proper normals and shadows

### 3. **complex_demo.rt** - Advanced Features
- **Best for**: Testing reflections, transparency, multiple lights
- **Features**: Reflective surfaces, transparent spheres, plane walls
- **Validation**: Mirror sphere reflects scene, glass sphere shows refraction

### 4. **transformations_demo.rt** - Transformation Testing
- **Best for**: Testing rotations, scalings, translations
- **Features**: Deliberately varied object sizes and orientations
- **Validation**: All objects render correctly despite transforms

---

## Build & Test Workflow

### Initial Build
```bash
cd /home/nherimam/Git/rt
make clean
make all
```
Expected output: `rt` executable with no warnings

### Run a Scene
```bash
./rt scenes/simple_spheres.rt 800 600
```
Window opens and displays rendered image

### Memory Leak Check
```bash
valgrind --leak-check=full --show-leak-kinds=all ./rt scenes/simple_spheres.rt 100 100
```
Should show "0 errors" or only SDL-related leaks

---

## Git Workflow

### Creating a feature branch
```bash
git checkout develop
git pull
git checkout -b feature/dev-a/your-feature-here
```

### Committing work
```bash
git add src/<module>/YourFile.cpp include/<module>/YourFile.hpp
git commit -m "Implement YourClass with documentation"
```

### Pushing to remote
```bash
git push origin feature/dev-a/your-feature-here
```

### Creating a Pull Request
Push to GitHub and create PR against `develop` branch with:
- Description of changes
- Reference to issue/task number
- Evidence of testing (unit tests passing, valgrind clean)

See [GIT_STRATEGY.md](GIT_STRATEGY.md) for complete workflow details.

---

## Phase Dependencies & Blocking

```
Dev A Phase 2 (Math)
    ↓ (blocks Dev A Phase 3)
Dev A Phase 3 (Camera)
    ↓ (blocks everyone)
Dev B Phase 4 (Base Objects) + Dev C Phase 6 (Materials/Lights)
    ↓ (blocks)
Dev B Phase 5 (Primitives) ← CRITICAL PATH
    ↓ (blocks Dev C Phase 7)
Dev C Phase 7 (Renderer)
    ↓ (blocks)
Dev C Phase 8-12 (Advanced features)
    ↓ (blocks)
Phase 13 (Testing) + Phase 14 (Polish)
```

**Critical Path**: Dev A (Day 1-2) → Dev B (Day 2-3) → Dev C (Day 3-5) → All (Day 5-6)

---

## Code Quality Standards

### Every commit should:
- [ ] Compile with `make all` (zero errors, zero warnings)
- [ ] Pass all unit tests: `./rt_test`
- [ ] Have no memory leaks: `valgrind --leak-check=full ./rt scenes/simple_spheres.rt 100 100`
- [ ] Include Doxygen comments on all public methods
- [ ] Follow C++23 best practices (use modern features, smart pointers)

### Before pull request:
- [ ] 100% of lines have Doxygen documentation
- [ ] All test results pass
- [ ] Another dev has reviewed code on branch

---

## Common Pitfalls & Solutions

### Issue: Build fails with "undefined reference"
**Solution**: Check linking flags in Makefile for required libraries (SDL2, png, jpeg)

### Issue: Seg fault on window display
**Solution**: Verify SDL2 is initialized in Window::initialize(), check pixel buffer allocation

### Issue: Lighting looks flat/wrong
**Solution**: Check normal calculation, verify dot product signs, debug Phong component contributions

### Issue: Memory leak in textures
**Solution**: Ensure libpng/libjpeg cleanup in Texture destructor, use smart pointers

### Issue: Ray-object intersections always miss
**Solution**: Check ray parametric equation, verify tMin/tMax ranges, debug with print statements

---

## Performance Targets (Reference)

- **800×600 @ simple_spheres.rt**: < 2 seconds
- **1024×768 @ complex_demo.rt**: < 5 seconds
- **Full resolution with reflections**: < 30 seconds per frame

Use profiling to identify bottlenecks (likely ray-triangle tests or shadow ray casting).

---

## Defense Preparation

In Phase 14, create an impressive demo that showcases:
1. Multiple objects with different materials
2. Reflective and transparent surfaces
3. Multiple light sources
4. Soft shadows
5. Real-time camera movement (interactive demo)

Suggested demo scene: Create a new scene combining best features of all examples.
