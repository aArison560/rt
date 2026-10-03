# Cours Mathématiques — RTv1 Blender-like
> Révision L1/M1 : Algèbre + Géométrie analytique appliquées au stade actuel du projet (Phase 1 Quadriques + 5.5 Mouvements)

**Objectif mémoire :** relier chaque formule du code à son cours universitaire pour comprendre pourquoi le moteur fonctionne et où il va (UV → Lumière → CSG).

---

## 0. Où en est le projet (contexte)

| Phase | Fichiers | État |
|-------|----------|------|
| Socle | `include/core/Scene.hpp:8` `include/core/Object.hpp:21` `src/core/Renderer.cpp:41` `src/parser/SceneParser.cpp:19` | **FAIT** |
| Phase 1 Quadriques | `include/core/Quadric.hpp:9` `src/core/Quadric.cpp:34` | **FAIT** : `Quadric` générique + `Cylinder`/`Cone`/`Paraboloid`/`Hyperboloid` avec cache `cachedU/V` `Quadric.hpp:28` |
| Phase 5.5 Mouvements statique | `include/core/Scene.hpp:37` `src/core/Renderer.cpp:48` `scenes/move_*.rt` | **FAIT** : `Y-up` `right=worldUp×forward`, `translate` O(1), `setDir` 1 rebuild |
| Phase 2-5 | UV, Textures, Lumière physique, CSG, Slice | **À venir** (ordre strict `AGENTS.md:60`) |

Optimisation fluidité : `Makefile:17` `-O3 -march=native -ffast-math -flto`, `translate` O(1) sans reallocation — bench 1080x720 6 quadriques ≈188ms.

---

## 1. Algèbre linéaire — R³

### 1.1 Vecteur `Vec3` — `include/math/Vec3.hpp:4`
```
Vec3 {x, y, z}
+  : (a+b) composante par composante
* k : scalage
dot  : a·b = ax bx + ay by + az bz = |a||b| cosθ
cross: a×b = (ay bz - az by, az bx - ax bz, ax by - ay bx) ; |a×b|=|a||b| sinθ, orthogonal à (a,b)
length : |v| = sqrt(v·v)
normalized : v/|v| si |v|>1e-9
```
**Cours → Code :** tout déplacement/calcul d'éclairage est une combinaison de ces 5 opérations.

### 1.2 Base orthonormée — changement de repère
Pour un objet d'axe `dir = Y local` (`Quadric.hpp:21`), on construit `u = X local`, `v = Z local` :

```cpp
// Quadric.cpp:22 buildBasis / updateBasis Quadric.cpp:34
arb = (0,1,0) si |w·arb|<0.999 sinon (0,0,1)
u = (w × arb).normalized()
v = w × u
```
`{u, w, v}` est directe et orthonormée. Tout point/rayon monde → local par produits scalaires :
```
ox = (O - pos)·u , oy = (O - pos)·w , oz = (O - pos)·v
dx = D·u , dy = D·w , dz = D·v          // Quadric.cpp:39
```
**Cours :** changement de base orthonormée = matrice orthogonale `P` (`Pᵀ=P⁻¹`). Ici on ne stocke pas `P`, on projette à la volée via `dot`.

**Optimisation actuelle :** `cachedU/V` `Quadric.hpp:28` calculés 1× à la construction/rotation `updateBasis()`, pas per-ray (gagne 2 `sqrt` + 2 `cross` par rayon/objet).

### 1.3 Transformation affine
- **Translation** `G` Blender : `P' = P + delta` → `setPos`/`translate` `Object.hpp:16` / `Quadric.hpp:35` / `Scene.hpp:37` — O(1), pas de rebuild.
- **Rotation** `R` : `dir' = normalized(dir)` → `setDir` `Quadric.hpp:37` — 1 seul `updateBasis()`.
- **Phase 5.5 doc :** `Y-up` RTv1 `(X,Y,Z)` ↔ `Z-up` Blender `(X,Z,-Y)` `AGENTS.md:45`.

---

## 2. Géométrie analytique — équations

### 2.1 Droite (rayon) — `include/math/Ray.hpp`
```
P(t) = O + t·D , t ≥ 1e-4 , |D|=1
```
### 2.2 Plan — `include/core/Object.hpp:31` `src/core/Object.cpp:23`
```
N·(P - P0) = 0  →  t = (P0 - O)·N / (N·D)   si |N·D|>1e-6
```
Normale = `N` (orientée contre `D` si `N·D>0`).

### 2.3 Sphère — `src/core/Object.cpp:4`
```
|P - C|² = R²  →  a= D·D (=1), b=2·OC·D, c=|OC|²-R²  →  disc = b²-4ac
t1,t2 = (-b ∓ sqrt(disc))/2a
N = (P - C)/|...|
```

### 2.4 Forme générale quadrique — `Quadric.hpp:5`
```
F(x,y,z) = A x² + B y² + C z² + D xy + E xz + F yz + G x + H y + I z + J = 0
```
En local `(x= X·u, y= X·w, z= X·v)`, avec `Q` symétrique :
```
Q = | A  D/2 E/2 |
    | D/2 B  F/2 |
    | E/2 F/2 C  |   L = (G,H,I)
F = Xᵀ Q X + L·X + J
```
**Cours :** rang/signature de `Q` classe la quadrique (cylindre, cône, paraboloïde, hyperboloïde, etc.).

**Gradient = normale (non normalisée) :**
```
∇F = (2Ax + Dy + Ez + G,
      Dx + 2By + Fz + H,
      Ex + Fy + 2Cz + I)   // Quadric.cpp:89
N_world = u·gx + w·gy + v·gz , puis normalized, flip si N·D>0
```

---

## 3. Quadriques du projet — coefficients `setCoeffs` `Quadric.cpp:17`

Toutes sont **Y-up** (`Y = axe`). Tableau (autres coeff =0) :

| Primitive | Équation locale | `A` | `B` | `C` | `H` | `J` | Clip |
|-----------|-----------------|-----|-----|-----|-----|-----|------|
| **Cylinder** `Quadric.cpp:119` | `x²+z² -R²=0` | 1 | 0 | 1 | 0 | `-R²` | `y∈[0,h]` + 2 caps disques `Quadric.cpp:139` |
| **Cone** `Quadric.cpp:164` | `x²+z² -R²/h² (h-y)²=0` | 1 | `-R²/h²` | 1 | `2R²/h` | `-R²` | `y∈[0,h]` + 1 cap base |
| **Paraboloid** `Quadric.cpp:230` | `x²/a²+z²/b² -y=0` | `1/a²` | 0 | `1/b²` | `-1` | 0 | `y∈[0,h]` + cap `x²/a²+z²/b²≤h` `Quadric.cpp:260` |
| **Hyperboloïde 1 nappe** `Quadric.cpp:293` | `x²/a²+z²/b²-y²/c²=1` | `1/a²` | `-1/c²` | `1/b²` | 0 | `-1` | `y∈[-h,h]` si fini |
| **Hyperboloïde 2 nappes** | `...=-1` | `1/a²` | `-1/c²` | `1/b²` | 0 | `+1` | `y∈[-h,h]` |

**Inverseur :** si tu lis `AGENTS.md:9` avec `Z` axe, permute `y ↔ z` pour retrouver `Y-up` code.

---

## 4. Intersection rayon → quadrique — `Quadric.cpp:48`

1. **Mise en local** : `ox,oy,oz` et `dx,dy,dz` (dot avec `cachedU/V/w`).
2. **Coeffs second degré** :
```
a = A dx² + B dy² + C dz² + D dx dy + E dx dz + F dy dz
b = 2A ox dx + 2B oy dy + 2C oz dz + D(ox dy+oy dx)+E(ox dz+oz dx)+F(oy dz+oz dy)+G dx+H dy+I dz
c = A ox²+... + G ox+ H oy+ I oz+ J
```
3. **Résolution** :
```
si |a|<1e-9 → linéaire t=-c/b
sinon disc=b²-4ac ; si <0 → miss ; sinon t1,t2 = (-b ∓ sqrt(disc))/2a (trié)
```
4. **Clipping** `y=oy+t·dy` doit être dans `[hMin,hMax]` `Quadric.cpp:84` sinon rejet.
5. **Normale** via `∇F` `Quadric.cpp:89` → monde → `t` le plus petit `≥1e-4` gardé.

**Cours :** c'est l'exercice type "intersection droite / conique-quadrique" de géométrie analytique.

---

## 5. Caméra & lumière — `src/core/Renderer.cpp`

### 5.1 Caméra sténopé `Renderer.cpp:41`
```
forward = normalized(cam.dir)
worldUp = (0,1,0) sauf si |forward·worldUp|>0.999 → (0,0,1)
right = (worldUp × forward).normalized()   // AGENTS.md:46 fix miroir
up    = (forward × right).normalized()
aspect = w/h , scale = tan(fov/2 * π/180)
nx = (2(x+0.5)/w -1)*aspect*scale
ny = (1-2(y+0.5)/h)*scale
rayDir = normalized(right·nx + up·ny + forward)  // Renderer.cpp:67
```
**Test statique :** `scenes/move_camera.rt:3` `C 1,0,-5` → sphère à gauche (pan `Shift+MMB`).

### 5.2 Éclairage diffus + ombres `Renderer.cpp:17`
```
lightDir = normalized(Lpos - hitPos)
diff = max(0, N·lightDir) * intensity
col = albedo * (ambient + diff*0.9)
shadowRay = { hitPos + N*1e-4 , lightDir }  // biais 1e-4 évite acne
si hit shadow t∈(1e-4, |Lpos-hit|) → en ombre → col = albedo*ambient
```
**Test :** `L 8,5,-5` → ombre à gauche `scenes/move_light.rt:4`.

---

## 6. Pour réviser / interro

**Exercices 10 min (papier) :**
1. Retrouve `B, H, J` du cône en développant `(h-y)²`.
2. Paraboloïde `a=1, b=2, h=2` : quel disque ferme la base ? Rayon en `x` et `z` ?
3. Pourquoi `arb=(0,0,1)` quand `|w·(0,1,0)|>0.999` ? (évite `w×arb≈0`)
4. Soit `forward=(0,0,1)` : calcule `right`/`up`. Que vaut `right` si `forward=(0,1,0)` ?
5. Explique pourquoi `translate` est O(1) et `setDir` O(1) + 1 rebuild — lien avec `Scene::translateObject` `Scene.hpp:37`.

**Références code à rouvrir :** `include/core/Quadric.hpp:9`, `src/core/Quadric.cpp:34`, `include/core/Object.hpp:16`, `include/math/Vec3.hpp:4`.

---

*Généré pour RTv1_building — Phase 1+5.5 terminées, optimisation cache pour fluidité Blender.*
