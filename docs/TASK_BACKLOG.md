# RT - Ray Tracer Task Backlog & Roadmap

## Overview

Complete task breakdown organized by phase (1-14) with developer assignment and time estimates.

---

## PHASE 1: Project Setup & Infrastructure (Days 1-2)

### [PHASE 1] Setup Makefile and Build System
- **Task**: Create Makefile with all, clean, fclean, re targets
- **Assigned to**: Dev A
- **Estimated**: 1h
- **Acceptance**: `make all` produces `rt` executable without errors

### [PHASE 1] Initialize SDL2 Window Infrastructure  
- **Task**: Stub Window class with SDL initialization
- **Assigned to**: Dev C
- **Estimated**: 2h
- **Acceptance**: Window opens, no crashes, basic SDL integration verified

### [PHASE 1] Project Documentation & README
- **Task**: Write comprehensive README with features, build instructions, usage
- **Assigned to**: Dev A
- **Estimated**: 1.5h
- **Acceptance**: Clear, complete documentation; build steps verified

### [PHASE 1] Git Repository Setup & Branching Strategy
- **Task**: Initialize git, create branches, document collaboration workflow
- **Assigned to**: Dev B
- **Estimated**: 1h
- **Acceptance**: All devs have feature branches, strategy documented

---

## PHASE 2: Math Foundations Layer (Days 2-3)

### [PHASE 2] Implement Vec3 Class - Part 1 (Basic Operations)
- **Task**: Implement constructors, assignment, +, -, *, / operators
- **Assigned to**: Dev A
- **Estimated**: 2.5h
- **Acceptance**: 
  - Arithmetic operators compile and pass basic tests
  - No memory leaks
  - Handles division by zero

### [PHASE 2] Implement Vec3 Class - Part 2 (Advanced Operations)
- **Task**: Implement dot, cross, magnitude, normalize, distance, lerp, reflect
- **Assigned to**: Dev A
- **Estimated**: 3h
- **Acceptance**: 
  - All vector operations functionally correct
  - Edge cases handled (zero vectors)
  - Mathematical accuracy verified

### [PHASE 2] Implement Ray Class
- **Task**: Implement Ray with origin, direction, pointAt(t)
- **Assigned to**: Dev A
- **Estimated**: 0.5h
- **Acceptance**: Ray generation and point calculation verified

### [PHASE 2] Implement Matrix4x4 - Initialization & Basic Operations
- **Task**: Implement identity, constructors, multiplication, transpose
- **Assigned to**: Dev A
- **Estimated**: 3h
- **Acceptance**: Matrix math verified against standard library references

### [PHASE 2] Implement Matrix4x4 - Transformations
- **Task**: Implement translate, rotateX/Y/Z, rotateAxis, scale, inverse
- **Assigned to**: Dev A
- **Estimated**: 3.5h
- **Acceptance**: All transformation matrices produce correct results

### [PHASE 2] Implement Transform Class
- **Task**: Wrap matrix operations, implement composition, local transforms
- **Assigned to**: Dev A
- **Estimated**: 2h
- **Acceptance**: Transform composition and application verified

### [PHASE 2] Unit Tests for Math Layer
- **Task**: Write comprehensive unit tests for all math classes
- **Assigned to**: Dev A
- **Estimated**: 3h
- **Acceptance**: All tests pass, >90% code coverage for math module

---

## PHASE 3: Camera System (Day 3)

### [PHASE 3] Implement Camera Class
- **Task**: Position, direction, FOV, generateRay() method, basis vectors
- **Assigned to**: Dev A
- **Estimated**: 2.5h
- **Acceptance**: 
  - Rays generated from screen coordinates
  - Camera controls (move, rotate) functional

### [PHASE 3] Camera Interactive Controls  
- **Task**: Implement forward, backward, left, right, up, down, pitch, yaw, roll
- **Assigned to**: Dev A
- **Estimated**: 1.5h
- **Acceptance**: All camera movements produce expected ray directions

---

## PHASE 4: Base Object System (Day 4)

### [PHASE 4] Implement HitRecord Class
- **Task**: Create structure for ray-object intersection data
- **Assigned to**: Dev B
- **Estimated**: 1h
- **Acceptance**: HitRecord stores all necessary intersection info

### [PHASE 4] Implement AObject Abstract Base Class
- **Task**: Create abstract interface for all geometric objects
- **Assigned to**: Dev B
- **Estimated**: 1.5h
- **Acceptance**: Can't instantiate abstract class, pure virtual hit() defined

### [PHASE 4] Implement Scene Container Class
- **Task**: Manage objects, lights, camera, background color
- **Assigned to**: Dev B
- **Estimated**: 1.5h
- **Acceptance**: Can add/remove objects, retrieve all elements

---

## PHASE 5: Primitive Objects (Days 4-5)

### [PHASE 5] Implement Sphere Primitive
- **Task**: Ray-sphere intersection using quadratic formula
- **Assigned to**: Dev B
- **Estimated**: 2.5h
- **Acceptance**: 
  - Accurate sphere intersection
  - Surface normals correct
  - UV mapping functional

### [PHASE 5] Implement Plane Primitive
- **Task**: Ray-plane intersection using dot product
- **Assigned to**: Dev B
- **Estimated**: 1.5h
- **Acceptance**: Plane intersection verified, normal consistent

### [PHASE 5] Implement Cylinder Primitive - Part 1
- **Task**: Ray-cylinder surface intersection, height bounds checking
- **Assigned to**: Dev B
- **Estimated**: 3h
- **Acceptance**: Cylinder body intersection accurate

### [PHASE 5] Implement Cylinder Primitive - Part 2
- **Task**: End cap intersection for cylinders
- **Assigned to**: Dev B
- **Estimated**: 2h
- **Acceptance**: Caps render correctly with proper normals

### [PHASE 5] Implement Cone Primitive
- **Task**: Ray-cone surface and cap intersection
- **Assigned to**: Dev B
- **Estimated**: 3h
- **Acceptance**: Cone renders with correct shading

---

## PHASE 6: Materials & Lighting Foundation (Day 5)

### [PHASE 6] Implement Material Class
- **Task**: Color, ambient/diffuse/specular, shininess, reflection, transparency
- **Assigned to**: Dev C
- **Estimated**: 1.5h
- **Acceptance**: Material properties storable and retrievable

### [PHASE 6] Implement Abstract Light Base Class
- **Task**: Define ALight interface with color, intensity
- **Assigned to**: Dev C
- **Estimated**: 0.5h
- **Acceptance**: Can't instantiate, pure virtual methods defined

### [PHASE 6] Implement PointLight
- **Task**: Position-based light with attenuation
- **Assigned to**: Dev C
- **Estimated**: 1.5h
- **Acceptance**: Point light direction and distance calculations verified

### [PHASE 6] Implement DirectionalLight
- **Task**: Parallel light rays (like sunlight)
- **Assigned to**: Dev C
- **Estimated**: 0.5h
- **Acceptance**: Directional light behaves as parallel source

### [PHASE 6] Implement AmbientLight
- **Task**: Global ambient lighting
- **Assigned to**: Dev C
- **Estimated**: 0.5h
- **Acceptance**: Ambient provides constant illumination

---

## PHASE 7: Core Rendering Engine (Days 5-6)

### [PHASE 7] Implement Basic Renderer - Ray Casting  
- **Task**: Ray generation, scene intersection, background color fallback
- **Assigned to**: Dev C
- **Estimated**: 2h
- **Acceptance**: 
  - Scene renders to pixel buffer (no lighting yet)
  - Objects show with solid colors

### [PHASE 7] Implement Phong Lighting Model
- **Task**: Ambient + diffuse + specular components
- **Assigned to**: Dev C
- **Estimated**: 3h
- **Acceptance**: 
  - Lighting calculations produce realistic shading
  - All three components contribute correctly

### [PHASE 7] Implement Shadow Ray Casting
- **Task**: Shadow rays from hit point to each light
- **Assigned to**: Dev C
- **Estimated**: 2.5h
- **Acceptance**: 
  - Shadows render correctly
  - No shadow acne artifacts

### [PHASE 7] Implement Reflection Tracing
- **Task**: Recursive reflection rays with depth limit
- **Assigned to**: Dev C
- **Estimated**: 2.5h
- **Acceptance**: 
  - Reflective surfaces show scene reflections
  - Recursion depth prevents infinite loops

---

## PHASE 8: Graphics & Display System (Day 6)

### [PHASE 8] Implement ImageBuffer Class
- **Task**: CPU pixel buffer with get/set pixel, RGBA conversion
- **Assigned to**: Dev C
- **Estimated**: 1.5h
- **Acceptance**: Pixel data correctly stored and retrievable

### [PHASE 8] Implement SDL2 Window Initialization
- **Task**: Full window setup, SDL texture creation
- **Assigned to**: Dev C
- **Estimated**: 2h
- **Acceptance**: Window displays without crashes

### [PHASE 8] Implement Pixel Display & Updates
- **Task**: Copy image buffer to SDL texture and display
- **Assigned to**: Dev C
- **Estimated**: 1.5h
- **Acceptance**: Rendered image displays in window correctly

### [PHASE 8] Implement EventHandler for Input
- **Task**: SDL event polling, keyboard/window events
- **Assigned to**: Dev C
- **Estimated**: 2h
- **Acceptance**: 
  - Window close detected
  - Keyboard input captured
  - Camera controls respond to input

---

## PHASE 9: Scene Configuration Parser (Day 7)

### [PHASE 9] Implement Scene Parser - Basic Structure
- **Task**: File reading, tokenization, basic parsing loop
- **Assigned to**: Dev C
- **Estimated**: 2h
- **Acceptance**: Can read .rt files without crashing

### [PHASE 9] Implement Parser for Objects
- **Task**: Parse sphere, plane, cylinder, cone directives
- **Assigned to**: Dev C
- **Estimated**: 2.5h
- **Acceptance**: Objects correctly created from file directives

### [PHASE 9] Implement Parser for Lights & Camera
- **Task**: Parse lighting and camera configuration
- **Assigned to**: Dev C
- **Estimated**: 1.5h
- **Acceptance**: Lights and camera configured from file

### [PHASE 9] Implement Parser Error Handling
- **Task**: Line number reporting, clear error messages
- **Assigned to**: Dev C
- **Estimated**: 1h
- **Acceptance**: Error messages help identify parsing problems

---

## PHASE 10: Transformations on Objects (Days 7-8)

### [PHASE 10] Apply Transformations to All Primitives
- **Task**: Integrate Transform with Sphere, Plane, Cylinder, Cone
- **Assigned to**: Dev B
- **Estimated**: 2.5h
- **Acceptance**: 
  - Objects transform correctly
  - Intersection calculations in transformed space accurate

### [PHASE 10] Parser Support for Transformations
- **Task**: Extend parser for translate/rotate/scale directives
- **Assigned to**: Dev C
- **Estimated**: 1.5h
- **Acceptance**: Scene files can specify object transformations

---

## PHASE 11: Texture Loading & Mapping (Days 8-9)

### [PHASE 11] Implement Texture Class - Image Loading
- **Task**: Load PNG/JPEG using libpng/libjpeg
- **Assigned to**: Dev C
- **Estimated**: 2.5h
- **Acceptance**: 
  - PNG and JPEG files load without errors
  - Pixel data accessible

### [PHASE 11] Implement Texture Sampling
- **Task**: UV coordinate sampling, bilinear filtering
- **Assigned to**: Dev C
- **Estimated**: 2h
- **Acceptance**: 
  - Textures sample correctly at UV coords
  - Filtering produces smooth results

### [PHASE 11] Integrate Textures with Materials
- **Task**: Apply texture color at render time
- **Assigned to**: Dev C
- **Estimated**: 1.5h
- **Acceptance**: Textured objects render with correct colors

---

## PHASE 12: Advanced Features (Days 9-10)

### [PHASE 12] Refraction/Transparency Implementation
- **Task**: Refraction rays for transparent objects, Snell's law
- **Assigned to**: Dev C
- **Estimated**: 3h
- **Acceptance**: Glass-like objects render correctly

### [PHASE 12] Soft Shadows Support  
- **Task**: Multi-sample shadow rays for soft shadow effects
- **Assigned to**: Dev C
- **Estimated**: 2h
- **Acceptance**: Adjustable shadow softness parameter works

### [PHASE 12] Progressive Rendering / Region Rendering
- **Task**: Ability to render image regions for incremental updates
- **Assigned to**: Dev C
- **Estimated**: 2h
- **Acceptance**: Can render and display image progressively

### [PHASE 12] Normal Mapping (Optional)
- **Task**: Use texture for bump mapping effects
- **Assigned to**: Dev C
- **Estimated**: 3h (optional)
- **Acceptance**: Normal-mapped surfaces show surface detail

---

## PHASE 13: Testing & Example Scenes (Days 10-11)

### [PHASE 13] Create Test Scenes  
- **Task**: Create 3+ demo .rt files showcasing features
- **Assigned to**: Dev B
- **Estimated**: 2.5h
- **Acceptance**: 
  - Spheres scene (reflections, shadows)
  - Planes scene (transformations, textures)
  - Complex scene (all features combined)

### [PHASE 13] Comprehensive Unit Testing
- **Task**: Unit tests for all major classes
- **Assigned to**: All Devs
- **Estimated**: 4h total (distributed)
- **Acceptance**: >85% code coverage, all tests pass

### [PHASE 13] Memory Leak Testing
- **Task**: Run valgrind on all test scenes
- **Assigned to**: Dev C
- **Estimated**: 1.5h
- **Acceptance**: Zero memory leaks detected

### [PHASE 13] Performance Profiling
- **Task**: Identify and document performance characteristics
- **Assigned to**: Dev A
- **Estimated**: 1.5h
- **Acceptance**: Profiling results documented

---

## PHASE 14: Polish & Defense Preparation (Days 11-12)

### [PHASE 14] Code Review & Refactoring
- **Task**: Cross-review all code, address issues
- **Assigned to**: All Devs
- **Estimated**: 3h total (distributed)
- **Acceptance**: Code follows C++23 best practices

### [PHASE 14] Edge Case Handling
- **Task**: Handle division by zero, null pointers, invalid transforms
- **Assigned to**: All Devs (their modules)
- **Estimated**: 2h
- **Acceptance**: Graceful error handling in all code paths

### [PHASE 14] Documentation & Doxygen
- **Task**: Complete Doxygen comments on all classes/methods
- **Assigned to**: Dev B
- **Estimated**: 2h
- **Acceptance**: All public classes documented

### [PHASE 14] Defense Demo Preparation
- **Task**: Create impressive demo scene showcasing all features
- **Assigned to**: Dev C
- **Estimated**: 2h
- **Acceptance**: Demo scene renders smoothly on test machine

### [PHASE 14] Live Demo Script & Presentation
- **Task**: Prepare interactive demo walkthrough
- **Assigned to**: All Devs
- **Estimated**: 2h total
- **Acceptance**: Demo runs smoothly, explains features clearly

---

## Dependency Chain for Planning

```
PHASE 1: Setup
  └─→ PHASE 2: Math (Dev A)
      └─→ PHASE 3: Camera (Dev A)
          └─→ PHASE 4: Base Objects (Dev B)
              ├─→ PHASE 5: Primitives (Dev B)
              │   └─→ PHASE 10: Transformations
              │       └─→ PHASE 13: Test Scenes
              │
              └─→ (in parallel) PHASE 6: Materials/Lights (Dev C)
                  └─→ PHASE 7: Rendering (Dev C)
                      ├─→ PHASE 8: Graphics Display (Dev C)
                      ├─→ PHASE 9: Scene Parser (Dev C)
                      ├─→ PHASE 11: Textures (Dev C)
                      └─→ PHASE 12: Advanced Features (Dev C)

PHASE 13: Testing (All Devs)
  └─→ PHASE 14: Polish & Defense (All Devs)
```

---

## Summary Statistics

| Metric | Value |
|--------|-------|
| **Total Tasks** | 46 |
| **Estimated Total Hours** | ~95 hours |
| **Estimated Per Developer** | ~32 hours |
| **Number of Phases** | 14 |
| **Phases in Parallel** | 3 max (phases 5,6,9) |
| **Critical Path Duration** | ~11 days |

### Time by Developer
- **Dev A** (Math + Camera + Tests): ~18 hours
- **Dev B** (Objects + Scene + Tests): ~19 hours  
- **Dev C** (Rendering + Graphics + Parser): ~42 hours + optional features

---

## Quality Gates

### Before Each Phase Completion
- [ ] Compiles with `make` (no errors/warnings)
- [ ] Unit tests pass for functionality
- [ ] No memory leaks (valgrind clean)
- [ ] Code reviewed by another developer
- [ ] Doxygen documentation complete

### Before Final Submission
- [ ] All tasks completed
- [ ] Full test coverage >85%
- [ ] Zero memory leaks
- [ ] Demo scene renders successfully
- [ ] README and documentation complete
