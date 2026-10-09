# RT — Architecture technique du moteur

> Objectif : définir la **structure cible** du traceur de rayons, les **algorithmes** à maîtriser,
> le **format de scène** et les **points d'extension** qui permettront d'ajouter les options sans
> tout réécrire.
>
> Références : `AGENTS.md`, `docs/IMPLEMENTATION_GUIDE.md`, `docs/SCENE_INFO.md` (branche `main`).
> L'état « actuel » provient d'un **audit statique** du code (branche `main`) : ⚠ à revalider par test.

---

## Sommaire

1. [Vue d'ensemble : le pipeline de rendu](#1-vue-densemble-le-pipeline-de-rendu)
2. [Modules et conventions de code](#2-modules-et-conventions-de-code)
3. [Modèle de données](#3-modèle-de-données)
4. [Algorithmes cœur](#4-algorithmes-cœur)
5. [Format des fichiers de scène](#5-format-des-fichiers-de-scène)
6. [Redisplay sans recalcul (exigence M6)](#6-redisplay-sans-recalcul-exigence-m6)
7. [Interface, interaction et preuves en direct](#7-interface-interaction-et-preuves-en-direct)
8. [Performance : BVH, multithreading, tiling](#8-performance-bvh-multithreading-tiling)
9. [Tests, qualité et mémoire](#9-tests-qualité-et-mémoire)
10. [Points d'extension pour les options](#10-points-dextension-pour-les-options)
11. [Arborescence cible du dépôt](#11-arborescence-cible-du-dépôt)

---

## 1. Vue d'ensemble : le pipeline de rendu

```
┌─────────────┐   ┌──────────────┐   ┌──────────────┐   ┌─────────────┐
│  fichier    │──►│ SceneParser  │──►│   Scene      │──►│  Renderer   │
│  .rt        │   │ (lex+parse)  │   │ objets+lights│   │  (threads)  │
└─────────────┘   └──────────────┘   │ +camera      │   └──────┬──────┘
                                     └──────┬───────┘          │ framebuffer (RGBA)
┌─────────────┐   ┌──────────────┐          │           ┌──────▼──────┐
│ EventHandler│──►│  main loop   │◄─────────┴───────────│ ImageBuffer │
│ (SDL events)│   │ poll→render  │                       └──────┬──────┘
└─────────────┘   │      │       │                              │
                  │      ▼       │                       ┌──────▼──────┐
                  │  Window      │◄──────────────────────│ SDL texture │
                  │  (blit)      │   expose → reblit     └─────────────┘
                  └──────────────┘   SANS recalcul
```

**Boucle principale** (`src/app/main.cpp`) :

```
1. Parser la scène (.rt)              → Scene, ou erreur + code retour propre
2. Initialiser la fenêtre + handlers  → Window, EventHandler
3. Boucle :
     a. poll des événements SDL        (clavier, souris, resize, EXPOSED, quit)
     b. si scène modifiée (caméra, objet, settings) → dirty = true
     c. si dirty  → Renderer::render(...)  → framebuffer
     d. si dirty  ou expose  → copie framebuffer → texture → présentation
4. Touche S → ImageBuffer::savePNG("screenshot_N.png")
```

**Règle architecturale absolue** : l'étape 3c (calcul) et l'étape 3d (affichage) doivent être
**séparées**. C'est ce qui rend vérifiable l'exigence M6 et l'item de fiche
« Exposes without recalculation ».

---

## 2. Modules et conventions de code

### 2.1 Organisation (inchangée)

```
include/<module>/<Fichier>.hpp     ← interfaces (pragma once)
src/<module>/<Fichier>.cpp         ← implémentations
src/app/main.cpp                   ← point d'entrée (exclu du binaire de tests)
tests/*.cpp                        ← tests unitaires (Catch2 amalgamé)
```

| Module | Responsabilité | Types clés |
|--------|----------------|------------|
| `core/` | Maths et données de base | `Vec3`, `Ray`, `Matrix4x4`, `HitRecord`, `Material` |
| `geometry/` | Primitives et intersections | `AObject`, `Sphere`, `Plane`, `Cylinder`, `Cone` |
| `lighting/` | Sources lumineuses | `ALight`, `PointLight`, `DirectionalLight`, `AmbientLight`, `AreaLight` |
| `scene/` | Scène, caméra, parsing, transforms | `Scene`, `Camera`, `SceneParser`, `Transform`, `ManageFile` |
| `rendering/` | Moteur de rendu et images | `Renderer`, `ImageBuffer`, `Texture`, `BVH`, `ThreadPool` |
| `platform/` | Fenêtre et événements (SDL2) | `Window`, `EventHandler` |
| `gui/` | Interface (microui) | `GuiManager`, `microui.c`, `r_render.c` |

### 2.2 Conventions

- Include relatif à `include/` : `#include "core/Vec3.hpp"`.
- `#pragma once` partout, commentaires **Doxygen** sur les méthodes publiques.
- Smart pointers (`std::shared_ptr` pour objets/lumières/matériaux), RAII, `[[nodiscard]]`.
- **`Vec3::EPSILON = 1e-6`** : constante unique pour les comparaisons flottantes et le *bias* des rayons.
- Build : `g++ -std=c++23 -Wall -Wextra -Werror -O2 -fPIC -march=native -pthread -g`.
  Côté C : `gcc -std=c11` (microui). Lier : `-lSDL2 -lm -lpng -ljpeg -pthread`.

---

## 3. Modèle de données

### 3.1 Schéma des relations

```
Scene
 ├─ Camera         position, orientation (basis), fov, viewport
 ├─ background      Vec3
 ├─ ambient         multiplicateur global               ← « Ambiance light »
 ├─ vector<shared_ptr<AObject>>                          ← « plusieurs du même type »
 │    └─ AObject (abstraite)
 │         ├─ Transform   translation + rotation + échelle  ← M4
 │         ├─ Material    color, amb, diff, spec, shininess,
 │         │              reflectivity, transparency, refractiveIndex,
 │         │              texture*, roughness
 │         └─ boundingBox() + intersect(ray, rec)        ← 1 fonction par objet
 ├─ vector<shared_ptr<ALight>>
 │    └─ ALight (abstraite)  color, intensity
 │         ├─ PointLight       position + attenuation
 │         ├─ DirectionalLight direction                 ← « Parallel light »
 │         ├─ AmbientLight
 │         └─ AreaLight        position + u/v axes + size  ← « spot non ponctuel »
 └─ objectVersion   compteur invalidant la BVH
```

### 3.2 `HitRecord` — la structure de intersection

```cpp
struct HitRecord {
    Vec3   point;        // point d'intersection (monde)
    Vec3   normal;       // normale sortante normalisée
    Vec3   geometricNormal;
    double t;            // paramètre du rayon
    double u, v;         // coordonnées texture
    bool   frontFace;    // rayon incident à l'intérieur ou à l'extérieur ?
    Material* material;  // matériau à l'impact
    AObject*  object;    // pour le picking, les cas particuliers
};
```

`frontFace` est **indispensable** pour la réfraction (orientation de la normale selon le sens de
traversal) et pour le *shadow bias*.

> Implémenté (T013) : `include/rt/base/Ray.hpp` — `rt::Ray` (origine, direction, `at(t)`,
> `depth` = génération), `rt::Interval` (`contains`/`surrounds`/`clamp`/`merged`), `rt::AABB`
> (`hit` par dalles, `merged`, `padded` contre les boîtes dégénérées), `rt::HitRecord`
> (point, normale orientée contre le rayon via `setFaceNormal`, `t`, `frontFace`,
> `materialIndex`, `uv`). Tous trivialement copiables et noexcept. Tailles mesurées
> (g++ 14, `Real = float`) : `Vec3` = 12 o, `Ray` = 28 o, `Interval` = 8 o,
> `AABB` = 24 o, `HitRecord` = 44 o. Tests : `tests/unit/test_ray.cpp`.

---

## 4. Algorithmes cœur

### 4.1 Génération des rayons (caméra)

Orthonormalisation de la base caméra (Gram–Schmidt) :

```
forward  = normalize(target - position)
right    = normalize(cross(forward, up))
trueUp   = cross(right, forward)

aspect   = width / height
halfH    = tan(fov/2)
halfW    = aspect * halfH

// pixel (i, j), centre du pixel, éventuellement jitter pour l'AA
u = ((i + 0.5)/width ) * 2 - 1     // ∈ [-1, 1]
v = 1 - ((j + 0.5)/height) * 2

dir = normalize(forward + u*halfW*right + v*halfH*trueUp)
ray = Ray(position, dir)
```

**Exigence M5** : cette base doit être recalculable à partir de `position` + `lookAt` + `up`
**arbitraires** (œil n'importe où, regard n'importe où) — cf. item « Did you know? ».

> Implémenté (T032) : `include/rt/render/Renderer.hpp` — `rt::render::render(const Scene&,
> Framebuffer&, RenderParams)` : boucle mono-thread pixels × `spp`, rayon via
> `Camera::rayForPixel` (T031), miss → fond de scène, `addSample()` + `present()`
> (T030). Profondeur max bornée 0..32 (`maxDepth`, pour la réflexion T056), graine
> 0..4294967295 déjà validée (jitter/RNG en T036). Aucune allocation dans la boucle
> (R3), aucun `throw` (R2), indépendant de SDL (R6 : `grep -R SDL src/render` vide).
> La recherche d'intersection sera branchée en P4 (T040+) sans changer la boucle.
> Tests : `tests/unit/test_renderer.cpp` (fond, déterminisme, erreurs).
>
> Implémenté (T046, M3) : recherche du plus proche parmi tous les objets —
> conversion scene → `geometry::AObject` une fois avant la boucle (chemin froid :
> `Sphere(center,radius)`, `Plane(point,normal)`, `Cylinder(center,radius)`,
> `Cone(apex=center, degreesToRadians(angle))`, `axis`/`height`/`slice` ignorés
> jusqu'en T130/T133 ; `scene::Transform` (ordre d'écriture) → `rt::Transform`
> (`translate`/`scale`/`rotate` degrés→radians, `M = Op_n-1*...*Op_0`) ; groupes
> aplatis récursivement (`M_monde = M_parent*M_local`, `id` croissant,
> `materialIndex` = rang) ; boucle chaude `findClosestHit` (tri par `t`,
> `tMax` resserré, `tMin` = 0.001, `noexcept`, pile uniquement). Hit → Lambert
> (T033 : `worldMats[rec.materialIndex]` + 1ère lumière avec position + ambiance),
> miss → fond ; dithering T036 conservé (hit comme miss). Tests :
> `test_renderer.cpp` (6 objets dont 2 sphères → proche gagne + far-only bleu,
> 4 types non-fond >5%, 2 sphères rouge/bleu sans doublon, `group.rt` aplati,
> `translate` ≡ placé octet par octet).

### 4.2 Intersections — une fonction par primitive (exigence M3)

Toutes résolvent `P(t) = O + t·D` et renvoient le plus petit `t > tMin` valide.

| Primitives | Équation | Points d'attention |
|------------|----------|--------------------|
| **Plan** | `t = dot(P0 - O, n) / dot(D, n)` | Rejeter `\|dot(D,n)\| < ε` (rayon parallèle). Normale constante. |
| **Sphère** | `a t² + b t + c = 0`, `b = 2·dot(OC,D)`, `c = \|OC\|² - r²` | Discriminant ; racine la plus proche ; normale = `(P - C)/r`. |
| **Cylindre** (fini, caps) | Projection perpendiculaire à l'axe : `a = \|D⊥\|²`, … | Puis **test de hauteur** le long de l'axe ; **caps** = disques (rayon–plan + `\|P-C\| ≤ r`). Normale : radiale sur le fût, axe sur les caps. |
| **Cône** (fini, base) | `(D·ax)² - cos²θ·\|D\|²` … forme quadratique du cône à demi-angle `θ` | Racines + test d'appartenance au **segment** [sommet, base] ; **base** = disque. |

Règles communes :
- Toujours **normaliser** la direction du rayon (sinon `t` est faussé).
- Normaliser la normale **avant** de la stocker.
- Gérer le cas `frontFace` : `if (dot(dir, outwardNormal) > 0) normal = -outwardNormal`.
- **Bias** : origine des rayons dérivés = `point ± normal * EPSILON` (anti *shadow acne*).

> Implémenté (T040) : `include/rt/geometry/Object.hpp` — `rt::geometry::AObject`
> (virtuelles pures `intersect(ray, tMin, tMax, rec)` + `localBounds()`, champs
> `kind`/`id`/`materialIndex`/`objectToWorld` identité par défaut, `setTransform`
> pour T045), `ObjectKind` + `toString`, dispatch **vtable** tranché dans
> `docs/ADR/002-dispatch.md` (aucune macro `INTERSECT(`, aucun `switch`
> générique). Tests : `tests/unit/test_geometry.cpp` (dispatch via base).
>
> Implémenté (T041) : `rt::geometry::Sphere` (centre + rayon, espace objet) —
> quadratique `a·t²+b·t+c`, plus proche dans `[tMin,tMax]`, `setFaceNormal`,
> `uv` sphériques, intermédiaires en `double` pour les sphères très loin,
> dégénérés (`r <= ε`, direction nulle, `tMin > tMax`, `NaN`) → `false`.
> `localBounds()` exacte (`C ± r`). Tests : 6 cas + dégénérés.
>
> Implémenté (T042) : `rt::geometry::Plane` (point + normale normalisée) —
> `t = dot(P0-O,n)/dot(D,n)`, garde `|denom| <= ε` avant division (jamais de
> division par zéro, UBSan propre), `setFaceNormal` cohérente des deux côtés,
> `uv` des axes tangents, `localBounds()` = ±1e6 documentée (plan infini).
> Tests : parallèle, dans le plan, avant/après + dégénérés.
>
> Implémenté (T043) : `rt::geometry::Cylinder` (point sur l'axe + rayon,
> infini autour de Y) — quadratique `a·t²+b·t+c` sur `(x,z)`, plus proche
> dans `[tMin,tMax]`, `a <= ε²` (parallèle/axial) → `false` défini sans
> division, `setFaceNormal` radiale, `uv` cylindriques (`u` = azimut,
> `v` = hauteur), intermédiaires en `double`, dégénérés → `false`.
> `localBounds()` = `x/z` serrée (`cx±r`), `y` = ±1e6 (T133 bornera).
> Tests : face, tangent, intérieur, axial/parallèle + dégénérés.
>
> Implémenté (T044) : `rt::geometry::Cone` (sommet + demi-angle en radians,
> infini deux nappes autour de Y) — `a·t²+b·t+c` avec `k = tan(angle)`,
> `|a| <= ε` → linéaire `b·t+c = 0` (parallèle à la génératrice, jamais de
> division par zéro), nappe par le signe de `y-ay`, normale = gradient
> `(2·px, -2·k²·py, 2·pz)`, sommet (`|P-apex| <= ε`, casse de la v1) →
> candidat ignoré défini sans `throw`, `uv` coniques, `double` stables.
> `localBounds()` = `y` = ±1e6, `x/z` évasées (`±(k·1e6+1)`, T133 bornera).
> Tests : nappes haute/basse, sommet, génératrice + près-apex + dégénérés.

### 4.3 Transformations (exigence M4)

Deux approches, **à choisir explicitement et à documenter** :

| Approche | Principe | Avantages | Inconvénients |
|----------|----------|-----------|---------------|
| **A. Espace objet** (recommandée) | Appliquer `M⁻¹` au rayon, intersecter dans le repère local, retransformer normale avec `(M⁻¹)ᵀ` | Transformations exactes, bbox simple | Coût matriciel par rayon |
| **B. Espace monde** | Géométrie pré-transformée à la modification | Rayons rapides | bbox/normales à recalculer à chaque modification |

**Point obligatoire de la fiche** : « rotations et translations continuent de fonctionner **après**
le tranché » (item *Limited objects*) → le découpage doit être défini **en coordonnées objet**
puisque l'objet bouge. Prévoir dès maintenant la couche `Transform` entre le parseur et les
intersections.

**Preuve obligatoire** : démontrer qu'une sphère déclarée `(0,0,0)` se rend correctement en
`(42,42,42)` (exemple littéral du sujet).

> Implémenté (T012) : `include/rt/base/Mat4.hpp` — `rt::Mat4` (produit, transposée,
> `inverse()` à pivot partiel renvoyant `std::optional`, jamais de throw — `nullopt` si
> singulier), `rt::Transform` (translate/rotateX/Y/Z/scale/compose) et les helpers
> `transformPoint` (w=1), `transformVector` (w=0), `transformNormal` (inverse-transposée
> de la partie 3×3, renormalisée). Approche A retenue : rayons en espace objet,
> normales via `(M⁻¹)ᵀ`. Tests : `tests/unit/test_mat4.cpp`.
>
> Implémenté (T045, M4) : approche A branchée dans les 4 primitives —
> `AObject::worldToObjectRay` (`O' = M⁻¹·O`, `D' = M⁻¹·D` non renormalisée
> pour conserver `t` ; `nullopt` → miss si `M` singulière), intersection en
> espace objet, `objectToWorldPoint` (`M`) + `objectToWorldNormal`
> (`(M⁻¹)ᵀ` renormalisée, unitaire même après scale non uniforme),
> `frontFace` recalculée en monde (même signe). Preuve du sujet :
> sphère `(0,0,0)` + `translate(42,42,42)` ≡ sphère placée en `(42,42,42)`
> (même `t`, point, normale). Tests : translation + rotation sur les 4
> types, scale non uniforme (normales unitaires), singulière → `false`.

### 4.4 Modèle d'ombrage (exigence M7)

**Phong** (composantes additivas, pour chaque lumière) :

```
L        = normalize(lightPos - P)                 // direction vers la lumière
NdotL    = max(dot(N, L), 0)                       // diffuse
V        = normalize(camPos - P)
H        = normalize(L + V)
specTerm = pow(max(dot(N, H), 0), shininess)       // specular (le « petit point blanc »)
att      = 1 / (k0 + k1·d + k2·d²)                 // atténuation ponctuelle
color   += lightColor * intensity * att
             * (diffuse  * NdotL * objColor
              + specular * specTerm * specColor)
```

- **Ambiance** : `ambient = ambientGlobal * material.ambient * objColor` → **aucun objet n'est
  totalement noir** (item *Ambiance light*).
- **Multi-spot** : **somme** des contributions → mélange des luminosités, plusieurs dégradés,
  ombres « assombries selon le nombre de sources visibles » (item *Lights*, image 3).
- **Brillance** : terme spéculaire **ajouté** à la couleur de l'objet → saturation en blanc.

> Implémenté (T053, M7) : `shading::specularTerm()` (Blinn-Phong, `H = norm(L+V)`,
> `spec = pow(max(dot(N,H),0), shininess) * specular * lightColor*intensity`,
> dos à la lumière / dégénérés / NaN → 0, jamais de NaN, `noexcept`, sans
> allocation) + `shadeSpecular()` (ponctuelle, `L` depuis `position`) branchés
> dans `render/` (`V = -ray.dir`, 1 `pow` par lumière non occultée, `saturate`
> final en blanc). Tests : `tests/unit/test_specular.cpp` (pic `N=H` = 0.5,
> dos/dégénérés = 0, DoD saturation `spec=1` → pixels 255 vs `spec=0` → 0,
> dégradé `max-min > 0.5`).

### 4.5 Rayons d'ombre et transparence

```
shadowRay = Ray(P + N·ε, L)
if (anyHit(shadowRay, tMax)) :  lumiere coupée
```

- `anyHit` = test **au premier impact** (plus rapide que `closestHit`), idéal pour la BVH.
- **Ombre modulée par la transparence** (item *Shadows and transparency*) :
  parcourir les transparences sur le trajet, ou échantillonner quelques points du trajet et
  moyener le facteur d'atténuation → `visibility ∈ [0,1]` plutôt qu'un binaire.
- **Ombres douces** (item *In bulk — spot non ponctuel*) : échantillonner
  `AreaLight::samplePoint()` sur `N` échantillons et moyener (`setShadowSamples(n)` existe déjà).

### 4.6 Réflexion, réfraction, Fresnel

```
R = dir.reflect(N)
color += trace(P + N·ε, R, depth+1) * reflectivity * fresnel

ratio = frontFace ? (1.0 / ior) : ior
if (dir.refract(N, ratio, T))                     // Snell/Descartes
    color += trace(P - N·ε, T, depth+1) * transparency * (1 - fresnel)
```

- **Formule de Descartes / Snell** : `n₁·sin θ₁ = n₂·sin θ₂` ; réflexion totale interne quand
  `1 - (ratio)²(1 - cos²θ₁) < 0` (déjà géré dans `Vec3::refract`).
- **Fresnel Schlick** : `F = F0 + (1 - F0)(1 - cosθ)⁵`, `F0 = ((n₁-n₂)/(n₁+n₂))²`.
- **Profondeur bornée** : `maxRecursionDepth` (défaut 4) + *Russian roulette* au-delà.
- Les **5 sous-critères** de l'item *Reflection and transparency* exigent que **reflectivity et
  transparency soient pilotables par fichier** (`0.0 → 1.0`), pas codés en dur.
  ⚠ audit statique : `material` n'expose aujourd'hui que `reflect` (+`roughness`) ;
  `transparency` et `ior` **ne sont pas lisibles depuis le fichier** → à ajouter (voir §5).

> Implémenté (T056, réflexion 1–2/5) : `rt::render::traceRay()` récursif
> (`src/render/Renderer.cpp`) — `R = reflect(D, N)` (`Vec3::reflect`, N
> normalisée), origine `P + N*eps` (anti-acné), `out = saturate(direct*(1-R)
> + réfléchi*R)` avec `R = clamp(reflectivity, 0, 1)` (schéma R1, alias
> `reflect`, `0 = mat` / `1 = miroir pur`), `depth >= maxDepth` (`limits
> { max_depth }` → `RenderParams::maxDepth`, 0..32, défaut 4) → direct seul.
> `noexcept`, sans allocation (R2/R3). Tests :
> `tests/unit/test_reflection.cpp` (R=0 identique octet par octet, R=1 =
> fond réfléchi net, 2 plans face à face terminent en `max_depth` 8/16,
> R=0.5 = moyenne à 0.05 près).
>
> Implémenté (T057, réfraction 3–5/5) : `shading::refractDir()` (`src/shading/Material.cpp`,
> Descartes `n1*sin(t1) = n2*sin(t2)`, `eta = frontFace ? 1/ior : ior`,
> `rPerp = eta*(I+cos1*N)`, `rPar = -sqrt(1-|rPerp|^2)*N`, sentinelle nulle en
> réflexion totale interne) + `traceRay()` étendu (`transmis = trace(P-N*eps, T,
> depth+1)`, `out = saturate(base*(1-T2) + transmis*T2)`, `T2 = clamp(transparency,
> 0, 1)`, `T2 = 0` = base seule octet-identique, `T2 = 1` = transmis pur,
> `ior = 1` = sans déviation, TIR = repli miroir, `depth >= maxDepth` → base).
> `noexcept`, sans allocation (R2/R3). Tests :
> `tests/unit/test_refraction.cpp` (6 cas : T=0 identique + `ior` ignoré, `ior = 1`
> sans déviation unitaire + transmis pur fond, sortie = courbure extérieure
> `0.75/0.66` + `1.33 vs 1.5` sur plan de fond, T=0.5 = moyenne, TIR nulle +
> profondeur 0 vs 8). Scène : `scenes/opt_glass.rt` (verre `0.9/1.5` + opaque).

### 4.7 Textures et UV

- Générer `(u,v)` par primitive : sphère (sphériques), plan (planaires), cylindre/cône
  (azimut + hauteur).
- Échantillonnage : `Texture::sample(u, v)` + `sampleFiltered(u,v)` (bilinear).
- **Étirer / décaler** = `u' = u*scaleX + offX`, `v' = v*scaleY + offY` → 2 champs dans le
  matériau, exposés au parseur (sous-critères 3 et 4 de l'item *Textures*).
- **Bump mapping** : perturber la normale via le gradient de la texture
  (`N' = normalize(N + strength·(dU, dV, 0))` en espace tangent) — sous-critère 3 de
  *More texture applications*.

---

## 5. Format des fichiers de scène

### 5.1 Format actuel (⚠ ne passe pas `File ++`)

```
bg 0.15 0.15 0.18
A 0.05 0.05 0.08
L 0.0 8.0 0.0 1.0 0.8 0.8 0.9
directional 0.2 0.5 0.1 0.9 0.9 0.85 0.4
c 0.0 1.5 15.0 0.0 0.2 -1.0 50.0
pl 0.0 -2.0 0.0 0.0 1.0 0.0
material 0.2 0.2 0.15 0.3 0.3 0.1 5.0 0.0
sp 1.0 0.0 -2.0 0.6
material 0.6 0.8 1.0 0.1 0.3 0.8 32.0 0.2
```

Directives supportées par le parser (audit) : `bg`, `A`, `L`, `directional`, `c`, `sp`, `pl`,
`cy`, `co`, `material` (postfixe, 8–9 valeurs).

**Problèmes** :
1. « Une information par ligne » → **exclu explicitement** par le critère `File ++`.
2. `material` **postfixe** → fragile et illisible.
3. Pas de directives pour : transformation, texture, transparence, indice de réfraction,
   lumière spot/aire, ambiance détaillée, découpage, objets composés/négatifs…

### 5.2 Format structuré proposé (vise `File ++`)

Blocs hiérarchiques `{ … }`, objets nommés, propriétés regroupées. **Syntaxe maison sans
dépendance externe** (le sujet accepte « XML, **ou** une structure/hiérarchie appropriée »).

```
scene "vitrine" {
    background { color 0.15 0.15 0.18 }

    # Ambiance pilotée par le fichier  → item « Ambiance ++ »
    ambient { color 0.06 0.06 0.08  intensity 1.0 }

    camera "cam_1" {                    # « Did you know? » : même scène, autre caméra
        position 0.0 1.5 15.0
        lookAt   0.0 0.2 -1.0
        up       0.0 1.0 0.0
        fov      50.0
    }

    object sphere "boule" {
        position 1.0 0.0 -2.0
        radius   0.6
        transform {                     # M4 : translations/rotations
            translate 42.0 0.0 0.0
            rotate    axis y  angle 30
        }
        material {
            color        0.6 0.8 1.0
            ambient      0.1
            diffuse      0.3
            specular     0.8
            shininess    32.0
            reflect      0.2            # « % de réflexion »
            transparency 0.0            # « % de transparence »
            ior          1.5            # indice de réfraction
            texture "textures/checker.png" { scale 4 4  offset 0.1 0.0  }
            bump        0.0
        }
        slice { axis x  min -1  max 1  frame object }   # « Limited objects »
    }

    light point  "key"  { position 0 8 0   color 1 0.8 0.8  intensity 0.9 }
    light spot   "rim"  { position 0 6 6   target 0 0 0  angle 25  color 1 1 1  intensity 1.0 }
    light area   "soft" { position 5 8 5   size 2 2      color 1 1 1  intensity 0.6 }
    light dir    "sun"  { direction 0.2 0.5 0.1  color 0.9 0.9 0.85  intensity 0.4 }
}
```

**Choix de conception (à valider en équipe, cf. SPECIFICATIONS §O3) :**

| Critère | Blocs `{}` | XML (`<scene>…`) |
|---------|-----------|------------------|
| Dépendance externe | aucune | petit parseur (ex. tinyxml2) |
| Lisibilité | excellente | bonne |
| Acceptation par la fiche | ✔ « structure ou hiérarchie appropriée » | ✔ mentionné explicitement |
| Coût d'implémentation | ~1 jour (lexer récursif) | ~1–2 jours |

**Recommandation** : blocs `{}` — aucun risque de refus (le texte dit « XML *ou* structure
hiérarchique »), zéro dépendance, et le même *lexer* sert aux blocs imbriqués (`transform`,
`slice`, `material`, `texture { … }`).

### 5.3 Contrat du parseur

| Exigence | Implémentation |
|----------|----------------|
| Erreurs **avec numéro de ligne** | `reportError()` → `Line N: message` + **code retour non nul** |
| Inconnu = erreur fatale | directive inconnue → arrêt (évite les silences) |
| Compatibilité | auto-détection : ligne commençant par `{`/`scene` → nouveau format ; sinon legacy. Ou option `--legacy` |
| Valeurs | flottants en `[0,1]` pour les couleurs, validations `ManageFile::checkErrorFile()` |
| Objets multiples du même type | le parser doit simplement **append** à `scene.objects` (aucune déduplication) |
| Matériau | **remplacer le postfixe** par un bloc imbriqué `material { … }` attaché à l'objet courant |
| Migration | réécrire/convertir les ~15 scènes existantes (`scenes/*.rt`) avec un script |

### 5.4 Exigences minimales de couverture du format

Pour **prouver** les options en soutenance, le format doit piloter au moins :

| Option | Champs requis |
|--------|---------------|
| Ambiance ++ | `ambient { color, intensity }` |
| Parallel light | `light dir { direction, color, intensity }` |
| Direct light | `light spot { position, target, angle, … }` |
| Réflexion 5/5 | `reflect`, `transparency`, `ior` |
| Textures 5/5 | `texture "path" { scale, offset }`, `alpha`, `bump` |
| Disruptions | `pattern { type sine\|checker\|perlin, scale … }` |
| Limited objects | `slice { axis, min, max, frame object\|world, shape circle\|triangle }` |
| Composed / négatif | `group "cube" { … }`, `boolean difference { … }` |
| Environment 4/5 | scripts lisant ces champs |

---

## 6. Redisplay sans recalcul (exigence M6)

### 6.1 Ce que la fiche va vérifier

1. Glisser une fenêtre par-dessus, changer le focus → l'image se redessine.
2. Un `printf` ajouté par le correcteur dans le callback expose doit s'afficher à chaque fois.
3. **Le calcul ne doit pas être rejoué** → démontrable par le code **et** par la vitesse.

### 6.2 Implémentation exigée

```
┌──────────────────────────────────────────────────────────┐
│  framebuffer (ImageBuffer, RGBA) — ÉTAT PERSISTANT       │
│  produit par Renderer::render() UNIQUEMENT quand dirty    │
└──────────────────────────────────────────────────────────┘
             │                            ▲
   dirty = true (caméra/objet/resize)     │ expose
             ▼                            │
      Renderer::render()          blit vers texture SDL
             │                            ▲
             └──────────► copie ──────────┘
```

- `EventHandler::onExpose(cb)` existe déjà et est branché sur `SDL_WINDOWEVENT_EXPOSED` ✔.
- Le callback expose doit **uniquement** : `SDL_UpdateTexture` + `SDL_RenderPresent`.
  **Aucun appel** à `Renderer::render()`, aucun `trace()`.
- Pour le **redisplay partiel** (« redraw the view **or part of the view** ») : conserver une
  image pyramidale ou une *dirty region* ; à défaut, blitter en entier est accepté (le correcteur
  demande surtout l'**absence de recalcul**).
- **Preuve à préparer** : afficher le temps de redisplay vs temps de rendu dans un coin
  (ex. `render: 412 ms — redraw: 3 ms`) ou dans les logs → argument imparable pendant la démo.

---

## 7. Interface, interaction et preuves en direct

La fiche *Environment* (5 points) et le sujet (« manipulations en direct avec vos propres
outils ») imposent une vraie surface d'interaction.

| Item fiche | Attendu | État / action |
|------------|---------|---------------|
| Env. 1 | Message de chargement + **barre de progression**, plus que du terminal | À ajouter (callback de progression du renderer) |
| Env. 2 | « Jolie interface » (gtk/qt) avec chargement de fichier et contrôle du rendu | microui est déjà intégré (`GuiManager`) : settings, création/édition d'objets — **à étendre** (chargement, progress) |
| Env. 3 | Interagir avec la scène **sans relancer** | Panneaux de sliders → re-render ✔ à valider |
| Env. 4 | Rendu **automatique avec modifications entre les rendus** (scripts acceptés) | Script shell/batch à fournir |
| Env. 5 | Rendu automatique d'**objets générés pour une scène** (tore de sphères, hélice) | Générateur de scène (script ou commande `rt --gen`) |

**Bonne nouvelle** : l'UI doit rester un **overlay** — ne pas toucher au pipeline de rendu.

---

## 8. Performance : BVH, multithreading, tiling

> Outils de mesure, sanitizers et cibles Makefile : **[OUTILS.md](OUTILS.md)**.
> Stratégie mémoire et « hot path » sans allocation : **[MEMORY_STRATEGY.md](MEMORY_STRATEGY.md)**.

### 8.1 BVH

- Structure construite une fois par version de scène (`Scene::getObjectVersion()` incrémente à
  chaque modification → invalidation automatique).
- Primitives : AABB alignées ; choix du plan de coupe = **Surface Area Heuristic (SAH)** pour
  réduire 20–40 % les traversées.
- Utiliser `intersectAny()` (ombres) et `intersectClosest()` (pixels).

> Implémenté (T060) : `include/rt/accel/Bvh.hpp` + `src/accel/Bvh.cpp` —
> `rt::accel::Bvh` (construction par médiane sur l'axe le plus long,
> `nth_element` sans allocation, feuilles `<= kMaxLeaf = 4`, profondeur
> bornée `kMaxDepth = 32`) ; `BvhNode` compact POD 32 o (`AABB` 24 o +
> 2 × 32 bits, encodage feuille `kLeafBit`, `static_assert(sizeof == 32)`) ;
> buffer préalloué `reserve(2N)` (**zéro `new` par nœud**, R3), `build()`
> → `Status` (jamais de `throw`, R2) ; `worldBounds()` = 8 coins de
> `localBounds()` par `objectToWorld` (approche A, T045). Mesuré : 1000
> sphères en grille → 511 nœuds (<= 2N-1 = 1999) en ~0,3 ms (< 50 ms).
> Tests : `tests/unit/test_bvh.cpp` (vide, singleton, DoD 1000, transform,
> nul refusé).
>
> Implémenté (T061) : `Bvh::traverse(ray, tMin, tMax, rec, objs)` (pile fixe
> `kStackSize = 64`, tableau local, pas de récursion ; test AABB optimisé
> méthode de Williams, `invDir` précalculé une fois, garde parallèle
> `|d| <= kEpsilon` comme `AABB::hit` ; fils proche d'abord, `tMax`
> resserré comme `findClosestHit`) ; `noexcept`, sans allocation (R2/R3).
> Tests : équivalence brute-force sur 200 scènes aléatoires × 20 rayons
> (mêmes `t` à 1e-4, mêmes `materialIndex`) + 1000 sphères × 100 rayons +
> rayons axiaux + dégénérés (vide, direction nulle, taille incohérente,
> fenêtre vide → miss défini). Mesuré (`scripts/bench_bvh.sh`, harness
> `bench.sh`, `docs/BENCH.md` `t061-bvh-linear/traverse`) : 1000 sphères ×
> 2000 rayons déterministes, 0,239 s → 0,009 s (≈ 26×, mêmes 757 hits).
>
> Implémenté (T062) : `rt::accel::BvhCache` (`include/rt/accel/BvhCache.hpp` +
> `src/accel/BvhCache.cpp`) — invalidation ciblée façon *depsgraph* Blender
> (`docs/INSPIRATION_BLENDER.md` §2) : `ensure(objs, scene.objectVersion`)
> ne reconstruit que si la version a changé (ou si le cardinal diffère,
> garde-fou inter-scènes ; **un cache par scène**, `clear()` en changeant).
> Compteur `bvhBuilds()` exposé (DoD) et affiché sur `stderr` quand
> `RT_DEBUG` est défini (`[debug] bvhBuilds=… version=… objs=… nodes=…`,
> chemin froid). `ensure()` verrouillé (`mutex_`, T066) ; `traverse()` reste
> lecture seule (tuiles disjointes). Tests : `tests/unit/test_bvh.cpp`
> `[t062]` (1000 `ensure` même version → 1 build ; version+1 → exactement 1
> de plus ; vide/cardinalité/nul/`clear()` définis).

### 8.2 Multithreading (item *Technical effects*)

- `ThreadPool` réutilisable (`hardware_concurrency()` workers), évite le coût de
  création/destruction de threads.
- **Tiling** : découper l'image en tuiles de 32×32 (localité cache) plutôt que par lignes.
- `renderRegion()` existe déjà (rendu progressif / par région).
- **Aucune donnée partagée mutable** hors du framebuffer : chaque thread écrit ses pixels.

> Implémenté (T063) : `rt::sched::ThreadPool` (`include/rt/sched/ThreadPool.hpp` +
> `src/sched/ThreadPool.cpp`) — pool créé **une fois** par rendu
> (`std::jthread`, 1..256), file de tuiles 32×32 (`splitTiles`, 0 recouvrement),
> `renderTile(camera, ctx, fb, seed, s, x0, y0, w, h)` par travailleur
> (`noexcept`, sans allocation, R2/R3) ; arrêt propre (`stop` + `join`,
> file drainée), erreurs par tâche interceptées (`hasError()`/`firstError()`,
> pas de `std::terminate`), attente économe (2 conditions, pas de spin).
> `Renderer::render()` : batches `spp` séquentiels (1 `onProgress` par batch
> depuis le thread appelant), tuiles parallèles par batch, `threads <= 1` =
> chemin mono historique octet-identique ; déterminisme par graine absolue
> (`rngFor(x, y, s, seed)`, T016) — `--threads 1/2/4/8` = mêmes pixels
> (sha256 identique vérifié sur `default.rt` 64×48). `--threads` câblé dans
> `main` (défaut 1, borne 1..256 validée en `Options` et en `render`).
> Tests : `tests/unit/test_threads.cpp` (100 tâches → compteur exact +
> réutilisable, `throw` intercepté sans `terminate`, 1/2/4/8 identiques,
> bornes 0/257 rejetées, 256 acceptée).
>
> Implémenté (T064) : `rt::render::RenderStats` (`Renderer.hpp`) —
> `primaryRays` (W×H×spp), `objects`/`lights`, `threadsUsed`, `buildMs`
> (collecte), `renderMs` (tuiles + `present()`), `totalMs`, `raysPerSec`,
> `bvhBuilds` (0 jusqu'en T065) ; rempli par `render(..., stats)` après
> `waitIdle` (sans atomique, hors boucle chaude), affiché en fin de rendu
> sur `stderr` sauf `--quiet` (`[stats] rays=… rays/s=…`), prêt pour l'UI
> (T075). Mesuré (`scripts/bench.sh`, `docs/BENCH.md` `t064-threads-1/2/4/8`
> + synthèse) : `fig_vi1.rt` 320×240 spp4, 0.736s → 0.412s → 0.269s →
> 0.252s (speedup 1.00/1.79/2.74/2.92 croissant, efficacité 1.00/0.89/0.69/
> 0.36 < 1). Tests : `[t064]` (compteurs cohérents, `stats==nullptr` =
> mêmes pixels).
>
> Implémenté (T065) : BVH branchée au rendu (**un seul point optimisé**,
> profiling interne car `perf` absent : scaling 5→101 objets 90→456 ms +
> micro-bench T061 26×) — `render()` construit `accel::Bvh` une fois par
> rendu (froid, dans `buildMs`, `bvhBuilds`=1), `TraceCtx::bvh` + `traverse()`
> pour le primaire et les ombres (`findClosestHit`/`shadowTransmittance`
> avec repli linéaire, `noexcept`, sans allocation). Mesuré (`bench.sh`,
> `docs/BENCH.md` `t065-bvh-off/on`) : `perf_many.rt` (101 objets) 320×240
> spp2, 3.759 s ± 0.142 → 0.321 s ± 0.012 (≈ **11.7×**, pixels
> octet-identiques) ; `rt_test [golden]` vert (DoD).
>
> Implémenté (T066) : propreté thread — `BvhCache::ensure()` verrouillé
> (`mutex_`), `traverse()` lecture seule, tuiles disjointes (pas de partage
> mutable), compteurs `RenderStats` calculés après `waitIdle` (sans
> atomique) ; `rt_test_tsan [threads]` vert (6 cas, 0 warning TSan, binaire
> TSan identique au normal octet par octet) ; test de reproductibilité
> `[t066]` (`fig_vi1.rt` 80×60 spp2 seed99, 1 vs 4 threads octet par octet,
> 10/10 en shell — DoD).

### 8.3 Rendu progressif (feedback utilisateur)

```
pass 1 : 1 spp   → image immédiate (bruitée)
pass 2 : 4 spp   → affinage
pass N : jusqu'au spp cible
```

### 8.4 Objectifs chiffrés (à mesurer et à publier)

| Scène | Résolution | Cible |
|-------|-----------|-------|
| `simple_spheres` | 800×600 | < 2 s |
| `complex_demo` | 1024×768 | < 5 s |
| réflexions complètes | plein écran | < 30 s |

> Item *Technical effects* « le rendu est vraiment rapide » : **mesurer et afficher** les
> temps (rays/s) dans l'UI. Un chiffre affiché vaut mieux qu'une affirmation.

---

## 9. Tests, qualité et mémoire

| Outil | Commande | Doit passer |
|-------|----------|-------------|
| Build strict | `make` | **0 warning** (`-Wall -Wextra -Werror`) |
| Tests | `make test` → `./rt_test` | tous verts |
| Mémoire | `valgrind --leak-check=full ./rt scenes/<x>.rt 100 100` | **0 fuite** |
| Rassemblement | `make re` | reproductible |

**Coverage minimal attendu** : maths (`Vec3`, `Matrix4x4`), intersections (4 primitives,
normales, caps), parser (scènes valides **et** invalides), `ImageBuffer`.

**Tests de non-régression visuelle** : versionner une image de référence par scène de
démonstration et comparer (tolérance) — utile pour éviter les régressions « ça a l'air mieux mais
c'est faux ».

**Ajouts recommandés** (audit : `tests/` couvre surtout cylindre/cône/matrice) :
- rayon tangent à une sphère (discriminant nul)
- réfraction : verre 1.5, réflexion totale interne
- expose : le callback ne déclenche pas `render()`
- parser : format structuré, erreur + numéro de ligne

---

## 10. Points d'extension pour les options

> Principe : **ne pas modifier le cœur** ; ajouter des points d'entrée typés.

| Option | Point d'extension |
|--------|-------------------|
| Limited objects | Filtre dans `AObject::intersect()` : après coup de la primitive, tester l'appartenance à la demi-espèce (`slice`) **en espace objet** |
| Disruptions | Hook `pattern(u,v, P) → (deltaNormal, deltaColor)` appelé depuis le shading |
| Textures | `Material::texture` + `sample(u,v)` ; `scale/offset` dans le mapping |
| Négatif (CSG) | Opérateur `Union / Difference / Intersection` en décorateur `AObject` (`intersect` = min/max des `t`) |
| Composé | `Group` = `vector<shared_ptr<AObject>>` + transform parent, instanciable N fois |
| Natifs (paraboloïde, hyperboloïde) | Nouvelle classe `AObject` : forme quadratique générique `ax² + by² + cz² + … = 0` |
| Tore / Möbius | Quartique (tore) — résolution par Newton sur le polynôme de degré 4 |
| Fichiers `.pov`/`.3ds` | Importeur **converti vers primitives équationnelles** (jamais de triangles pour les objets du mandat) |
| Cluster | Découpage d'image par tuiles → exécutions `rt --tile x y w h -o out.png` + montage — **guide complet : [DISTRIBUTED_RENDERING.md](DISTRIBUTED_RENDERING.md)** |
| Caustics / GI | Estimation de photon mapping ou bidirectionnelle — **hors périmètre raisonnable** sauf temps excédentaire |

---

## 11. Arborescence cible du dépôt

```
rt/
├── Makefile                  # make | make test | make re
├── README.md                 # build, usage, formats
├── author                    # ← SPECIFICATIONS §8-O1
├── include/  src/            # modules (cf. §2)
├── scenes/                   # scènes de démo PRÊTES À CALCULER
│   ├── 01_obligatoire_4objets.rt      # figure VI.1
│   ├── 02_obligatoire_meme_scene_oeil.rt  # figure VI.2 (caméra seule changée)
│   ├── 03_obligatoire_mix_ombres.rt   # figure VI.3
│   ├── opt_reflexion_transparence.rt
│   ├── opt_textures.rt
│   ├── opt_limited_objects.rt
│   ├── opt_disruptions.rt
│   ├── opt_composed_negatif.rt
│   └── ...
├── textures/                 # png/jpeg de démonstration
├── scripts/                  # batch render (Env. 4), génération (Env. 5)
├── tests/                    # Catch2
├── docs/                     # cette documentation
│   ├── README.md  SPECIFICATIONS.md  ARCHITECTURE.md
│   ├── PLAN_TRAVAIL.md  OPTIONS_GUIDE.md  CHECKLIST_DEFENSE.md
│   ├── evalsheet/  subjects/
│   └── (main) GIT_STRATEGY.md TASK_BACKLOG.md IMPLEMENTATION_GUIDE.md SCENE_INFO.md
└── tmp.md
```

> **Règle de démonstration** : *un item de fiche = un fichier de scène nommé `opt_*.rt`*.
> Cela rend la soutenance mécanique : le correcteur tape une commande, voit la preuve.
