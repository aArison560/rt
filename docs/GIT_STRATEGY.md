# Ray Tracer - Git Branching & Collaboration Strategy

## Repository Structure

**Repository**: `rt` (miniRT sequel)  
**Main Branch**: `main` (production-ready code)  
**Default Branch**: `main`

## Branching Strategy: Feature Branch Workflow

### Branch Naming Convention

```
<type>/<developer>/<feature>
```

- `type`: `feature`, `bugfix`, `refactor`, `test`
- `developer`: `dev-a`, `dev-b`, `dev-c`
- `feature`: descriptive name in kebab-case

### Main Branches

#### `main`
- **Status**: Stable, production-ready
- **Protection**: Pull requests required, tests must pass
- **Merge from**: Release branches and critical hotfixes
- **Stable**: Release versions only

#### `develop`
- **Status**: Integration branch, feature-ready
- **Protection**: Pull requests required
- **Merge from**: All feature branches
- **Merges to**: `main` at release points

### Developer Workstreams

#### Dev A - Mathematical Foundations & Camera
**Primary Branches**:
- `feature/dev-a/vec3-implementation`
- `feature/dev-a/ray-implementation`
- `feature/dev-a/matrix4x4-implementation`
- `feature/dev-a/transform-implementation`
- `feature/dev-a/camera-system`
- `feature/dev-a/unit-tests-math`

**Dependencies**: None (foundational)  
**Timeline**: Phase 1-2 (weeks 1-2)  
**Merges to**: `develop`

#### Dev B - Geometric Objects & Scene Management
**Primary Branches**:
- `feature/dev-b/aobject-base`
- `feature/dev-b/sphere-primitive`
- `feature/dev-b/plane-primitive`
- `feature/dev-b/cylinder-primitive`
- `feature/dev-b/cone-primitive`
- `feature/dev-b/hit-record`
- `feature/dev-b/scene-container`

**Dependencies**: Dev A (Vec3, Ray, Transform)  
**Timeline**: Phase 2-4 (weeks 2-3.5)  
**Merges to**: `develop`

#### Dev C - Renderer, Graphics & Scene Parser
**Primary Branches**:
- `feature/dev-c/renderer-core`
- `feature/dev-c/phong-lighting`
- `feature/dev-c/shadow-casting`
- `feature/dev-c/sdl2-window`
- `feature/dev-c/event-handler`
- `feature/dev-c/image-buffer`
- `feature/dev-c/scene-parser`
- `feature/dev-c/main-entry`
- `feature/dev-c/material-system`
- `feature/dev-c/texture-support`

**Dependencies**: Dev A (all math), Dev B (scene, objects)  
**Timeline**: Phase 3-7 (weeks 2.5-6)  
**Merges to**: `develop`

## Workflow

### 1. Creating Features

```bash
# Sync with main
git checkout develop
git pull origin develop

# Create feature branch
git checkout -b feature/dev-a/vec3-implementation

# Work locally
git add src/Vec3.cpp
git commit -m "feat(dev-a): implement Vec3 arithmetic operations

- Add operator+, operator-, operator* implementations
- Add dot and cross products
- Add normalize() method
- Satisfies phase 1 requirements"
```

### 2. Commit Message Convention

Format: `<type>(<scope>): <subject>`

```
feat(dev-a): implement Vec3 magnitude calculation
fix(dev-c): correct window initialization SDL errors
refactor(dev-b): improve sphere intersection algorithm
test(dev-a): add Vec3 unit tests
```

Types: `feat`, `fix`, `refactor`, `test`, `chore`, `docs`

### 3. Pull Request Process

```bash
# Push to origin
git push origin feature/dev-a/vec3-implementation

# On GitHub create PR:
# Title: [Dev A] Implement Vec3 arithmetic
# Description: 
#   Implements basic vector math operations
#   - Fixes: Issue #3
#   - Related: Task spec phase 1
#   - Tests: All unit tests pass

# PR Checklist
- [ ] Code compiles with `make`
- [ ] All tests pass (`make test`)
- [ ] Follows C++23 style guide
- [ ] No memory leaks (valgrind clean)
- [ ] Documentation complete
```

### 4. Code Review & Merge

**Approvals Required**: 1 other developer

**Before Merge**:
```bash
# Ensure up-to-date with develop
git fetch origin
git rebase origin/develop

# Verify build
make clean
make -j4

# Run tests
make test
```

**Merge **locally** then push**:
```bash
# Fast-forward merge into develop
git checkout develop
git merge --ff-only feature/dev-a/vec3-implementation
git push origin develop

# Delete feature branch
git branch -d feature/dev-a/vec3-implementation
git push origin --delete feature/dev-a/vec3-implementation
```

## Dependency Graph

```
Dev A (Math/Camera)
  ├── Vec3 ✓
  ├── Ray ✓
  ├── Matrix4x4 ✓
  ├── Transform ✓
  └── Camera ✓
      │
      ├──→ Dev B (Objects)
      │     ├── HitRecord ✓
      │     ├── AObject (abstract) ✓
      │     ├── Sphere ✓
      │     ├── Plane ✓
      │     ├── Cylinder ✓
      │     ├── Cone ✓
      │     └── Scene ✓
      │         │
      │         └──→ Dev C (Renderer)
      │               ├── Renderer ✓
      │               ├── Material ✓
      │               ├── ALight (abstract) ✓
      │               ├── PointLight ✓
      │               ├── DirectionalLight ✓
      │               ├── AmbientLight ✓
      │               ├── Texture ✓
      │               ├── Window ✓
      │               ├── EventHandler ✓
      │               ├── ImageBuffer ✓
      │               ├── SceneParser ✓
      │               └── main.cpp ✓
```

### Critical Path (Blocking Dependencies)
1. **Dev A → Math Layer** (must complete before Dev B/C can proceed full)
2. **Dev A + Dev B → Dev C** (renderer needs scene and objects)

### Parallel Work Possible
- Dev A implements math in parallel
- Dev B waits for Vec3/Ray/Transform, then proceeds independently on objects
- Dev C waits for all A/B before rendering

## Conflict Resolution

**If merge conflicts occur**:

```bash
# Pull latest develop
git pull origin develop

# Resolve conflicts in editor
# Math conflicts: prioritize correct algorithm
# API conflicts: discuss with other developers
# Test conflicts: ensure all tests pass after resolution

git add <resolved files>
git commit -m "merge: resolve conflicts with develop

Resolved Vec3 operator+ conflict with input validation approach"

git push origin feature/your-feature
```

**Contact protocol**:
- Conflicts block work → immediate chat notification
- Discuss resolution before committing
- Peer review conflict resolution code

## Release Process

1. Create `release/v1.0.0` branch from `develop`
2. Only bugfixes and version bumps in release branch
3. Merge to `main` with tag `v1.0.0`
4. Merge back to `develop`

```bash
git checkout -b release/v1.0.0 develop
# Fix critical bugs only
git tag -a v1.0.0 -m "RT v1.0.0 - Initial release"
git checkout main
git merge --no-ff release/v1.0.0
git push origin main --tags
git checkout develop
git merge --no-ff release/v1.0.0
git push origin develop
git branch -d release/v1.0.0
```

## Tools & CI/CD

**Local checks** (before pushing):
```bash
make clean
make all          # Must compile cleanly
make test         # Must pass all tests
valgrind ./rt     # No memory leaks

# Code style
clang-format -i src/*.cpp include/*.hpp
```

**Pre-commit hook** (recommended):
```bash
#!/bin/bash
make all && make test || exit 1
```

## Communication Protocol

- **Daily standups**: Brief async updates in shared channel
- **Blocking issues**: Immediate notification
- **Code reviews**: 24-hour turnaround target
- **Merge conflicts**: Resolve same day

## Timeline Milestones

- **Week 1**: Dev A completes math layer → ready for integration
- **Week 2**: Dev B begins objects, Dev C begins graphics setup
- **Week 3**: All basic classes functional
- **Week 4**: Lighting and rendering working
- **Week 5**: Polish, optimization, advanced features
- **Week 6**: Testing, documentation, defense prep
