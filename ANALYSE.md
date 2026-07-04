# Analyse du projet RT — Ray Tracer

> Générée le 04 juillet 2026

## Résumé

Projet de **ray tracer Whitted-style** en **C++23** avec rendu CPU, interface SDL2, et export PNG.
Architecture modulaire propre, mais implémentation incomplète sur plusieurs aspects critiques.

---

## 1. Problèmes critiques

### 1.1 Textures non fonctionnelles

**Fichier** : `src/rendering/Texture.cpp:78-81`

`Texture::load()` retourne toujours `false`. Le système de texture est un stub complet.

```cpp
bool Texture::load(const std::string& /*filename*/)
{
    // TODO: Implement texture loading
    return false;
}
```

**Impact** : Impossible d'utiliser des textures sur les objets. Aucun objet dans les scènes `.rt` ne peut avoir de texture.

### 1.2 Rotation de la caméra non implémentée

**Fichier** : `src/scene/Camera.cpp:141-154`

Les méthodes `rotatePitch()`, `rotateYaw()` et `rotateRoll()` ont le corps vide. Les touches fléchées du clavier (liées dans `EventHandler`) ne produisent aucun effet.

```cpp
void Camera::rotatePitch(double /*angle*/)
{
    // TODO: Implement pitch rotation
}

void Camera::rotateYaw(double /*angle*/)
{
    // TODO: Implement yaw rotation
}
```

**Impact** : Les contrôles de rotation annoncés (↑/↓/←/→) sont inopérants.

### 1.3 Réfractions désactivées par défaut

**Fichier** : `src/rendering/Renderer.cpp:25`

```cpp
m_refractionsEnabled = false;
```

`main.cpp` n'appelle jamais `renderer.setRefractionsEnabled(true)`. La scène `default.rt` contient une sphère en verre avec `transparency:0.9` et `reflectivity:0.0` — elle s'affiche entièrement noire.

**Impact** : Les matériaux réfractifs (verre, eau) sont impossibles à rendre.

### 1.4 Aucune accélération spatiale

**Fichier** : `src/rendering/Renderer.cpp:216-220`

`castRay()` itère **tous les objets** de la scène linéairement pour chaque rayon.

```cpp
for (const auto& obj : scene.getObjects()) {
    if (obj->hit(ray, tMin, tMax, record)) { ... }
}
```

**Impact** : Complexité O(n × m) où n = nombre d'objets, m = nombre de rayons.
Sans BVH, KD-tree, ou grille, le rendu ne passe pas à l'échelle au-delà de ~20 objets.

### 1.5 Transformations inutilisées

**Fichier** : `include/geometry/AObject.hpp:57` / `src/geometry/AObject.cpp:58-59`

Chaque objet possède un `Transform`, mais aucune méthode `hit()` ne l'applique.
Les `getWorldBoundingBox()` retournent la bounding box en espace objet sans transformation.

```cpp
AABB AObject::getWorldBoundingBox() const
{
    // TODO: Transform object-space AABB to world space
    return getObjectBoundingBox();
}
```

**Impact** : Les transformations déclarées dans les classes n'ont aucun effet sur le rendu.

### 1.6 Code mort dans SceneParser

**Fichier** : `src/scene/SceneParser.cpp:273-396` (~150 lignes)

Deux systèmes de parsing coexistent :
1. **Parcourt caractère par caractère** : `skipWhitespace()`, `peekChar()`, `consumeChar()`, `parseNumber()`, `parseVec3()`, `parseSphere()`… — **jamais appelé**
2. **Parcourt ligne par ligne** (`std::getline` + `std::istringstream`) dans `parse()` — **seul utilisé**

Toutes les méthodes du premier système sont du code mort.

```cpp
// === DEAD CODE (non appelé) ===
bool SceneParser::skipWhitespace() { ... }
bool SceneParser::isAtEnd() const { ... }
char SceneParser::peekChar() const { ... }
// ... ~150 lignes inutilisées
```

**Impact** : Maintien inutile, confusion pour les développeurs.

---

## 2. Problèmes de conception

### 2.1 `const_cast` systématique

**Fichiers** : `Sphere.cpp:110`, `Plane.cpp:86`, `Cylinder.cpp:143`, `Cone.cpp:169`

Toutes les méthodes `hit()` sont marquées `const` mais doivent modifier le `HitRecord` pour y stocker le pointeur vers l'objet. Solution actuelle : `const_cast`.

```cpp
record.setObject(const_cast<Sphere*>(this)); // Dangereux
```

**Solution** : Rendre `hit()` non-const, ou passer le pointeur `this` via un paramètre séparé.

### 2.2 Pointeurs bruts dans HitRecord

**Fichier** : `include/core/HitRecord.hpp:106-107`

```cpp
Material* m_material;
AObject*  m_object;
```

`HitRecord` stocke des pointeurs bruts vers le `Material` et l'`AObject`, alors que toute la scène utilise `std::shared_ptr`. Si la scène est modifiée entre deux frames, ces pointeurs deviennent pendants.

**Solution** : Utiliser `std::shared_ptr` ou `std::optional<HitRecord>`.

### 2.3 `dynamic_cast` + comparaison de `std::string` dans le hot path

**Fichier** : `src/rendering/Renderer.cpp:195-211`

```cpp
if (auto dirLight = dynamic_cast<DirectionalLight*>(light.get())) { ... }
else if (auto pointLight = dynamic_cast<PointLight*>(light.get())) { ... }
```

Et dans `calculateShadow()` :

```cpp
if (lightName == "DirectionalLight") { ... }
else if (lightName == "PointLight") { ... }
```

Les `dynamic_cast` et comparaisons de `std::string` sont exécutés **pour chaque rayon-ombre**. Extrêmement lent.

**Solution** : Ajouter une méthode virtuelle `getLightType()` ou un enum dans `ALight`.

### 2.4 Absence de correction gamma

**Fichier** : `src/rendering/Renderer.cpp:45-48`

```cpp
pixel[0] = static_cast<unsigned char>(std::clamp(color.x(), 0.0, 1.0) * 255.0);
pixel[1] = static_cast<unsigned char>(std::clamp(color.y(), 0.0, 1.0) * 255.0);
pixel[2] = static_cast<unsigned char>(std::clamp(color.z(), 0.0, 1.0) * 255.0);
```

Les couleurs sont clampées linéairement sans appliquer de correction gamma. Les images produites paraissent plus sombres qu'attendu sur les écrans modernes (gamma sRGB ~2.2).

**Impact** : Rendu visuellement incorrect.

### 2.5 Thread safety fragile

**Fichier** : `src/rendering/Renderer.cpp:74-78`

`render()` lance plusieurs threads qui écrivent chacun dans une région de `ImageBuffer`. Si la fenêtre est redimensionnée pendant le rendu, le buffer peut être réalloué pendant que les threads y écrivent.

```cpp
// Aucun mutex ou mécanisme de synchronisation
threads[i] = std::thread(&Renderer::renderRegion, this, region, ...);
```

**Solution** : Mutex, atomic check, ou copie locale par thread.

### 2.6 Pas de gestion d'erreur dans les threads

**Fichier** : `src/rendering/Renderer.cpp:74-88`

Les exceptions levées dans les threads de rendu sont silencieusement perdues.

**Solution** : Capturer les exceptions par thread et les reporter.

---

## 3. Problèmes de performance

### 3.1 Itération linéaire de tous les objets (voir 1.4)

### 3.2 Pas de SIMD

Toutes les opérations `Vec3` sont scalaires. Aucune utilisation de SSE/AVX.

### 3.3 Aucune terminaison anticipée des rayons

Pas de roulette russe. Les rayons continuent de propager même quand leur contribution est négligeable.

### 3.4 Redondance dans le rendu

**Fichier** : `src/app/main.cpp:142`

```cpp
SDL_Delay(16); // Suppose 60 FPS, mais ne tient pas compte du temps de rendu
```

Le délai fixe de 16 ms s'ajoute au temps de rendu. Si le rendu prend 5 secondes, on attend 16 ms supplémentaires inutilement.

---

## 4. Tests insuffisants

**Fichier** : `tests/test_main.cpp` (222 lignes, 13 tests)

### Couverture actuelle

| Module | Tests |
|--------|-------|
| Cylinder | 7 tests (miss, hit body, caps, normal) |
| Cone | 6 tests (miss, hit side, base, normal) |

### Non testé

| Module | État |
|--------|------|
| Vec3 (tous les opérateurs, dot, cross, normalize, reflect, refract) | ❌ |
| Ray | ❌ |
| Matrix4x4 (multiplication, inverse, transpose, rotation) | ❌ |
| HitRecord | ❌ |
| Material | ❌ |
| Sphere | ❌ |
| Plane | ❌ |
| Camera (génération de rayons) | ❌ |
| Scene / SceneParser | ❌ |
| Renderer | ❌ |
| ImageBuffer | ❌ |
| Transform | ❌ |
| Lights (toutes) | ❌ |

**Pas de framework de test** (asserts maison avec `#define CHECK`), pas de tests d'intégration, pas de rendu de référence.

---

## 5. Problèmes du build system

**Fichier** : `Makefile`

| Problème | Détail |
|----------|--------|
| `-fPIC` sur des exécutables | Nécessaire pour les librairies partagées, pas pour les binaires |
| Pas de `-pthread` | `std::thread` utilisé mais flag manquant |
| Pas de dépendances automatiques | `.d` files non générés — tout header modifié déclenche une reconstruction complète |
| `g++` hardcodé | Pas de support pour `CC`, `CXX`, `clang++` |
| `-O2` uniquement | Pas de `-march=native`, pas de profilage |
| Pas de cible `install` | `make install` manquant |

---

## 6. Opportunités C++20/23 inexploitées

| Fonctionnalité | Usage possible |
|----------------|----------------|
| `std::optional<HitRecord>` | Remplacer le flag `valid` + pointeurs bruts |
| `std::expected<void, Error>` | Type de retour pour `Renderer::render()` |
| `std::span<const std::byte>` | Buffer de pixels typé |
| `std::mdspan` (C++23) | Accès 2D aux pixels |
| `std::format` / `std::println` (C++23) | Remplacer `std::cout << ...` |
| `std::filesystem::path` | Remplacer `std::string` pour les chemins |
| Concepts (`#include <concepts>`) | Contraintes sur templates géométriques |
| `std::move_only_function` (C++23) | Callbacks EventHandler move-only |
| `[[nodiscard]]` | Marquer les fonctions à retour non-ignorable |
| `std::source_location` (C++20) | Rapports d'erreur |

---

## 7. TODO / Stales

**39 TODO commentaires** sur 15 fichiers sources. La majorité décrit du code déjà implémenté.

Concentration :
- `Texture.cpp` : 6 TODO (fonctionnalité réellement manquante)
- `Window.cpp` : 7 TODO (tous déjà implémentés)
- `ImageBuffer.cpp` : 4 TODO (tous déjà implémentés)
- `Scene.cpp` : 5 TODO (tous déjà implémentés)
- `SceneParser.cpp` : 2 TODO (dont code mort)
- `Matrix4x4.cpp` : 4 TODO (tous déjà implémentés)

---

## 8. Historique Git

- **17 commits** sur branche unique
- Pas de merge, pas de tagging, pas de release
- Messages en anglais et français mélangés
- Pas de CI/CD

---

## 9. Recommandations par priorité

### Priorité 1 — Corriger les bugs fonctionnels
- Implémenter `Texture::load()`
- Implémenter `rotatePitch()` / `rotateYaw()` / `rotateRoll()`
- Activer `setRefractionsEnabled(true)` dans `main.cpp`
- Supprimer le code mort de `SceneParser`

### Priorité 2 — Architecture et nettoyage
- Supprimer les TODO obsolètes (~30)
- Remplacer `const_cast` par une API non-const ou paramètre `AObject*`
- Remplacer `dynamic_cast` + `std::string` par un enum de type
- Utiliser `std::shared_ptr` dans `HitRecord`

### Priorité 3 — Performance
- Ajouter un BVH (Bounding Volume Hierarchy)
- Activer le multi-échantillonnage (`samples > 1`)
- Ajouter `-pthread` au Makefile et `-march=native`
- Terminaison anticipée des rayons (roulette russe)

### Priorité 4 — Qualité visuelle
- Correction gamma (sRGB)
- Tone mapping
- Ombres douces via `shadowSamples`
- Fresnel pour les diélectriques

### Priorité 5 — Tests
- Ajouter des tests unitaires pour Vec3, Ray, Matrix4x4, Sphere, Plane
- Ajouter des tests d'intégration pour SceneParser
- Ajouter un framework de test (Catch2, Google Test)

### Priorité 6 — Modernisation C++23
- `std::optional<HitRecord>`
- `std::span` / `std::format`
- `[[nodiscard]]` sur les API publiques
- `std::filesystem::path` pour les chemins
