# Hyperboloid Implementation Plan

## Overview

Implement a hyperboloid geometric primitive for the RT ray tracer project. The hyperboloid will follow the same pattern as existing primitives (Sphere, Plane, Cylinder, Cone) with proper ray intersection, surface normals, UV mapping, and integration with the scene parser.

## Design Specifications

### Hyperboloid Representation
A hyperboloid of revolution can be represented by:
- Center position (Vec3)
- Axis direction (Vec3, normalized for main symmetry axis)
- Hyperboloid equation coefficients: Ax² + Bxy + Cxz + Dy² + Exz + Fy² + Gxz + Hx + Iy + Jz = 0
- Three rotation angles (around X, Y, Z axes)
- Material support via AObject inheritance

### Scene File Format
New directive: `h` (hyperboloid)
```
# Hyperboloid syntax: h cx cy cz ax ay az a b c d e f g h i
# Example: h 0.0 0.0 0.0 0.0 1.0 0.0 1.0 0.0 1.0 1.0 0.0 0.0 0.0 0.0 0.0
```

Where:
- `cx cy cz`: Center position
- `ax ay az`: Axis direction (will be normalized)
- `a b c d e f g h i`: 9 coefficients defining the hyperboloid equation

## Implementation Steps

### Step 1: Create Hyperboloid Header File
**File**: `include/geometry/Hyperboloid.hpp`

**Requirements**:
- Inherit from `AObject`
- Include: `AObject.hpp`, `<optional>`
- Declare public methods matching AObject interface:
  - Constructor (default, with parameters, with material)
  - Destructor
  - Copy constructor and assignment operator
  - `hit()` - ray-hyperboloid intersection
  - `getBoundingBox()` - object space bounding box
  - `getType()` - return "Hyperboloid"
  - Parameter setters/getters
  - `getNormalAt()` - surface normal calculation
  - `getUVAt()` - UV coordinate mapping
- Private members:
  - `Vec3 center` - hyperboloid center
  - `Vec3 axis` - normalized rotation axis
  - `double coeffs[9]` - hyperboloid equation coefficients
  - `Transform transform` - object's world transformation
  - Optional: rotation matrices, cached values

### Step 2: Implement Hyperboloid Source File
**File**: `src/geometry/Hyperboloid.cpp`

**Requirements**:
- Include: `Hyperboloid.hpp`, `HitRecord.hpp`, `<algorithm>`, `<cmath>`, `<optional>`
- Default constructor: creates unit hyperboloid at origin
- Parameterized constructor: initializes center, axis, coefficients
- Copy constructor and assignment operator: proper resource management
- Destructor (default)

**Implementation Details**:
- `hit()`: 
  - Transform ray to object space
  - Solve quadratic equation for hyperboloid intersection
  - Apply height bounds check (if finite hyperboloid)
  - Return HitRecord with point, normal, material, UVs
- `getNormalAt()`:
  - Calculate gradient of hyperboloid equation
  - Transform to world space using object's transform
  - Apply surface normal consistency (front/back face)
- `getUVAt()`:
  - Angular coordinates around axis (angle, height/angles)
  - Map to [0,1] texture coordinates
- `getBoundingBox()`:
  - Calculate bounding box in object space
  - Return in object-space coordinates

### Step 3: Update SceneParser

**File**: `src/scene/SceneParser.cpp`

**Requirements**:
- Add parsing logic for `h` directive in `parse()` method
- Add error handling for 9+ numeric arguments
- Create hyperboloid object: `lastObject = std::make_shared<Hyperboloid>(...)
- Add object to scene: `scene.addObject(lastObject)`
- Maintain existing `lastObject` tracking for postfix `material` support

**New Case in Parser**:
```cpp
if (id == "h") {
    if (v.size() < 10) {
        reportError("h expects at least 10 values: cx cy cz ax ay az a b c ...");
        return false;
    }
    lastObject = std::make_shared<Hyperboloid>(
        Vec3(v[0], v[1], v[2]),
        Vec3(v[3], v[4], v[5]),
        Vec3(v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13], v[14])
    );
    scene.addObject(lastObject);
    continue;
}
```

### Step 4: Integration Testing

Create test scenes in `scenes/hyperboloid_test.rt`:

```
# Basic hyperboloid test
bg 0.2 0.2 0.3
A 0.1 0.1 0.15
L 5.0 5.0 10.0 1.0 1.0 1.0 1.0

# Standard hyperboloid opening along Z-axis (hyperbolic shape)
h 0.0 0.0 0.0 0.0 0.0 1.0 1.0 1.0 1.0 1.0 1.0 -1.0 0.0 0.0 0.0
material 0.6 0.2 0.8 0.2 0.6 0.8 32.0 0.3

# Rotated hyperboloid
c 3.0 1.0 15.0 0.0 0.0 -1.0 45.0
h -2.0 0.5 5.0 1.0 0.0 0.0 0.5 1.0 1.5 1.0 0.5 0.5 0.0 -0.5 0.0
material 0.3 0.7 0.4 0.3 0.4 0.6 16.0 0.2
```

### Step 5: Run Tests

1. Build the project:
```bash
make clean
make
```

2. Test the hyperboloid:
```bash
./rt scenes/hyperboloid_test.rt 800 600
```

3. Verify:
- Hyperboloid renders with correct shape
- Surface normals are accurate
- Materials apply correctly
- Interactive camera controls work

## Technical Details

### Hyperboloid Intersection Algorithm
Transform ray to object space, solve quadratic equation:

```cpp
double a = coeffs[0] * dir.x * dir.x + coeffs[1] * dir.x * dir.y + ...;
double b = coeffs[0] * origin.x * dir.x + coeffs[1] * origin.x * dir.y + ...;
double c = coeffs[0] * origin.x * origin.x + coeffs[1] * origin.x * origin.y + ...;

// Discriminant
double discriminant = b * b - 4 * a * c;
if (discriminant < 0) return 0; // no intersection

// Solve for t
t0 = (-b - sqrt(discriminant)) / (2 * a);
t1 = (-b + sqrt(discriminant)) / (2 * a);
```

### Surface Normal Calculation
Gradient of hyperboloid equation:

```cpp
Vec3 gradient = Vec3(
    2 * coeffs[0] * point.x + coeffs[1] * point.y + coeffs[2] * point.z + coeffs[6],
    coeffs[1] * point.x + 2 * coeffs[3] * point.y + coeffs[5] * point.z + coeffs[8],
    coeffs[2] * point.x + coeffs[5] * point.y + 2 * coeffs[4] * point.z + coeffs[7]
);
Vec3 normal = gradient.normalized();
```

### UV Mapping
```cpp
double angle = atan2(point.dot(uDir), point.dot(vDir));
double height = point.dot(axis);
u = (angle + M_PI) / (2 * M_PI);
v = (height + maxHeight) / (2 * maxHeight);
```

## Files Created/Modified

### New Files:
1. `include/geometry/Hyperboloid.hpp` - Hyperboloid class definition
2. `src/geometry/Hyperboloid.cpp` - Hyperboloid implementation
3. `scenes/hyperboloid_test.rt` - Test scene file

### Modified Files:
1. `src/scene/SceneParser.cpp` - Add hyperboloid parsing

## Testing Acceptance Criteria

- [ ] Hyperboloid renders without rendering errors
- [ ] Ray intersection works correctly (points on surface satisfy equation)
- [ ] Surface normals are perpendicular to hyperboloid surface
- [ ] UV mapping produces continuous coordinates
- [ ] Material properties apply correctly
- [ ] Interactive camera controls work
- [ ] No memory leaks (valgrind)
- [ ] All existing tests still pass

## Dependencies

- Existing codebase includes:
  - Core: Vec3, Ray, HitRecord, Material
  - Scene: Transform, Camera, Scene, SceneParser
- SDL2, libpng, libjpeg (existing dependencies)
