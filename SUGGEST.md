# RT — Proposition d'Améliorations

> **Date**: 4 juillet 2026
> **Version analysée**: Branche `main` — toutes les fonctionnalités de base implémentées

---

## Table des matières

1. [Priorité Haute — Corrections & Bugs](#1-priorité-haute--corrections--bugs)
2. [Performance & Optimisation](#2-performance--optimisation)
3. [Rendu & Éclairage Avancés](#3-rendu--éclairage-avancés)
4. [Textures & Matériaux](#4-textures--matériaux)
5. [Tests & Qualité](#5-tests--qualité)
6. [Features Interactives](#6-features-interactives)
7. [Architecture & Refactoring](#7-architecture--refactoring)
8. [Documentation](#8-documentation)
9. [Roadmap Suggérée](#9-roadmap-suggérée)

---

## 1. Priorité Haute — Corrections & Bugs

### 1.1 `AObject::getWorldBoundingBox()` — BOX NON TRANSFORMÉE

**Fichier**: `src/geometry/AObject.cpp` — ligne 71-73

```cpp
void AObject::getWorldBoundingBox(Vec3& minCorner, Vec3& maxCorner) const
{
    // TODO: Transform bounding box to world space
    getBoundingBox(minCorner, maxCorner);
}
```

**Problème**: La BVH utilise `getWorldBoundingBox()` pour construire sa hiérarchie. Si un objet a une transformation (rotation, translation, scale), la bounding box renvoyée est en *object space* et non en *world space*. Cela peut :
- Faire échouer la BVH (intersections AABB-ray incorrectes)
- Provoquer des trous visuels ou des objets invisibles dans certains cas

**Solution**: Transformer les 8 coins de la bounding box par la matrice de transformation de l'objet, puis calculer la bounding box englobante résultante.

### 1.2 `Texture::sampleFiltered()` — FILTRAGE BILINÉAIRE NON IMPLÉMENTÉ

**Fichier**: `src/rendering/Texture.cpp` — ligne 141-144

```cpp
Vec3 Texture::sampleFiltered(double u, double v) const
{
    // TODO: Bilinear filtering
    return sample(u, v);
}
```

**Problème**: Appelé mais retombe sur l'échantillonnage nearest-neighbor, ce qui donne un aspect pixelisé sur les textures vues de près ou en biais.

**Solution**: Implémenter le bilinear filtering : interpoler entre les 4 pixels voisins en fonction des coordonnées UV fractionnaires.

### 1.3 Pas de support JPEG chargé

**Problème**: `libjpeg-dev` est listé comme dépendance et linké (`-ljpeg`), mais `Texture::load()` ne contient que le chargement PNG. Le JPEG n'est pas supporté dans les faits.

**Solution**: Ajouter une branche de chargement JPEG dans `Texture::load()` utilisant la libjpeg.

### 1.4 `Renderer::castRay()` — BVH sans fallback cohérent

**Fichier**: `src/rendering/Renderer.cpp`

Si `bvhRoot` est nul (premier appel avant construction), le code itère sur tous les objets. Mais si la BVH est construite puis les objets de la scène changent (via l'API `removeObject`), la BVH n'est pas reconstruite automatiquement.

**Solution**: Ajouter un mécanisme d'invalidation de la BVH quand la scène est modifiée.

### 1.5 `refractionsEnabled = false` par défaut

**Fichier**: `src/rendering/Renderer.cpp` — constructeur

Bien que `main.cpp` active les réfractions, le constructeur de `Renderer` les désactive par défaut. Toute instanciation directe du renderer aura les réfractions désactivées sans le savoir.

---

## 2. Performance & Optimisation

### 2.1 BVH — Surface Area Heuristic (SAH)

**Actuel**: Partitionnement par médiane des centroïdes (median split).

**Amélioration**: Implémenter le **Surface Area Heuristic (SAH)** qui choisit le plan de coupe minimisant le coût attendu de traversée. Peut réduire le nombre d'intersections rayon/objet de 20-40%.

```cpp
// Au lieu de nth_element median split, évaluer plusieurs candidats
// et choisir celui qui minimise : cost = C_trav + C_isect * (SA_left * N_left + SA_right * N_right) / SA_parent
```

### 2.2 Thread Pool persistant

**Actuel**: `Renderer::render()` crée des `std::thread` à chaque frame et les détruit.

```cpp
std::vector<std::thread> threads;
for (...) {
    threads.emplace_back([...] { ... });
}
for (auto& t : threads) t.join();
```

**Amélioration**: Créer un **thread pool** réutilisable initialisé une fois. Évite le coût de création/destruction de threads (~1-5ms par frame). Utiliser `std::jthread` (C++20) ou une queue de tâches atomique.

### 2.3 SIMD pour Vec3

**Actuel**: Opérations scalaires sur `double x, y, z`.

**Amélioration**: Utiliser les intrinsics SSE/AVX pour paralléliser :
- Produit scalaire : `_mm_dp_pd`
- Addition, multiplication component-wise
- Normalisation

Alternative pragmatique : utiliser `__attribute__((vector_size(32)))` (GCC/Clang) pour l'auto-vectorisation.

### 2.4 Coherence mémoire du framebuffer

**Actuel**: Accès à `pixelBuffer[index]` dans une boucle imbriquée.

**Amélioration**: Traiter le framebuffer par tuiles (tiling) de 32×32 pixels pour améliorer la localité du cache CPU. Particulièrement bénéfique avec le multithreading (évite le cache bouncing entre threads).

### 2.5 Early ray termination par threshold

**Actuel**: Russian Roulette pour les rayons de profondeur ≥ 3 (seuil 0.25).

**Amélioration**: Ajouter un **seuil de contribution cumulative** : si la contribution d'un rayon à la couleur finale devient négligeable (< 0.01), le terminer plus tôt. Économise des appels récursifs inutiles.

### 2.6 Profilage intégré

**Manque**: Aucune instrumentation de performance. Impossible de savoir où le temps est passé sans outil externe.

**Solution**: Ajouter un système de profilage simple avec des `std::chrono` et des compteurs :
- Temps total de rendu par frame
- Nombre d'intersections BVH testées
- Nombre de rayons d'ombre lancés
- Affichage optionnel (`--profile`)

---

## 3. Rendu & Éclairage Avancés

### 3.1 Lumières surfaciques (Area Lights)

**Actuel**: Seulement des lumières ponctuelles et directionnelles.

**Ajout**: Créer une classe `AreaLight` héritant de `ALight` :
- Une géométrie rectangulaire ou sphérique qui émet de la lumière
- Échantillonnage de plusieurs points sur la surface pour l'éclairage
- Ombres douces naturelles (plus réalistes que le jittering actuel)

```cpp
class AreaLight : public ALight {
    Vec3 position;
    Vec3 uAxis, vAxis;  // Vecteurs définissant la surface
    int samples;         // Nombre d'échantillons
};
```

### 3.2 Microfacet BRDF (Cook-Torrance)

**Actuel**: Phong specular basique.

**Amélioration**: Remplacer la composante spéculaire par un modèle **Cook-Torrance** avec :
- Distribution GGX (Trowbridge-Reitz)
- Fonction de masquage/ombrage Smith
- Terme de Fresnel (Schlick, déjà présent pour les diélectriques)

Résultat : reflets plus réalistes, aspect métal/plastique plus convaincant.

### 3.3 Ombres douces — Échantillonnage stratifié

**Actuel**: `shadowSamples` avec jittering uniforme aléatoire.

**Amélioration**: Utiliser un **échantillonnage stratifié** (Hammersley, Sobol) pour une meilleure distribution spatiale. Moins de bruit pour le même nombre d'échantillons.

### 3.4 Glossy reflections

**Actuel**: Les réflexions sont parfaitement mirror-like.

**Ajout**: Permettre des réflexions **glossy** (floues) en jitterant la direction réfléchie en fonction d'un paramètre de rugosité du matériau.

```cpp
// Dans le matériau
Vec3 roughness;  // 0 = mirror, 1 = diffuse reflection
```

### 3.5 Tone mapping ACES

**Actuel**: Reinhard tone mapping (correct mais basique).

**Amélioration**: Implémenter le **ACES Filmic Tone Mapping** pour une meilleure préservation des hautes lumières et un aspect plus cinématographique.

```cpp
static float acesToneMap(float c) {
    return (c * (2.51f * c + 0.03f)) / (c * (2.43f * c + 0.59f) + 0.14f);
}
```

### 3.6 Post-processing

**Ajout**: Pipeline de post-processing simple :
- **Bloom** : Détection des zones très lumineuses, flou gaussien, addition
- **Vignette** : Assombrissement progressif des bords
- **Color grading** : LUT (look-up table) pour appliquer des filtres

---

## 4. Textures & Matériaux

### 4.1 Textures procedurales

**Ajout**: Générer des textures à la volée sans fichier image :
- **Checker pattern** : Damier classique
- **Marble** : Bruit de Perlin
- **Wood** : Anneaux concentriques bruités
- **Noise** : Perlin/Simplex noise

```cpp
class ProceduralTexture : public Texture {
    Vec3 sample(double u, double v) const override;
};
```

### 4.2 Normal mapping

**Actuel**: Les textures ne sont utilisées que pour la couleur.

**Ajout**: Charger des **normal maps** et les utiliser pour perturber la normale de surface lors du calcul d'éclairage.

```cpp
void Material::setNormalMap(Texture* normalMap);
// Dans Renderer::calculateLighting():
Vec3 perturbedNormal = sampleNormalMap(hitRecord.getUV());
```

### 4.3 Displacement mapping (optionnel)

Déplacer réellement la géométrie le long de la normale en fonction d'une texture de hauteur. Plus coûteux mais plus précis que le normal mapping.

### 4.4 Skybox / Environment lighting

**Ajout**: Charger un environnement HDR (cubemap ou equirectangular) pour :
- L'éclairage ambiant directionnel (IBL — Image Based Lighting)
- Le reflet du ciel dans les surfaces réfléchissantes
- Fond de scène plus riche que la couleur unie actuelle

---

## 5. Tests & Qualité

### 5.1 Couverture de tests — GAP CRITIQUE

**Actuel**: 13 tests, seulement Cylinder et Cone.

**Tests à ajouter impérativement** :

| Module | Tests suggérés |
|--------|---------------|
| **Vec3** | Opérateurs arithmétiques, dot, cross, normalize, reflect, refract |
| **Ray** | pointAt(), constructeurs |
| **Matrix4x4** | Multiplication, inverse, determinant, rotations, perspective |
| **Sphere** | Intersection (devant, derrière, tangent), normale, UV |
| **Plane** | Intersection, normale, distanceTo() |
| **Cylinder** | Corps + caps, normales (body + caps) |
| **Cone** | Surface + base cap, normales |
| **Camera** | generateRay() pour différents pixels |
| **SceneParser** | Chaque directive, erreurs, edge cases |
| **Material** | getters/setters, texture attachment |
| **Renderer** | Calculs lighting (ambient, diffuse, specular) |
| **BVH** | Construction, intersection AABB |
| **Transform** | Composition, inverse |

### 5.2 Tests de performance

Ajouter des tests de performance basiques :
- `make benchmark` qui chronomètre le rendu de scènes de référence
- Détection de régressions de performance

### 5.3 Tests de fuites mémoire

Le projet exige zéro fuite mémoire. Ajouter au pipeline de test :
```bash
valgrind --leak-check=full --error-exitcode=1 ./rt_test
```

### 5.4 Tests d'intégration

Tester des scènes `.rt` complètes :
- Charger une scène → vérifier le nombre d'objets/lumières
- Vérifier que le rendu ne crashe pas
- Comparer visuellement (snapshot diff) pour détecter les régressions

---

## 6. Features Interactives

### 6.1 Rendu progressif

**Actuel**: Rendu complet → affichage → mise à jour après camera movement.

**Amélioration**: Afficher les pixels au fur et à mesure :
1. Premier passage : 1 échantillon/pixel (instantané)
2. Raffiner progressivement avec plus d'échantillons
3. L'utilisateur voit la scène immédiatement, même floutée

### 6.2 Sélection & inspection d'objets

**Ajout**: Cliquer sur un objet pour voir ses propriétés :
- Type d'objet, matériau, transformation
- Distance, normale au point cliqué
- Afficher en overlay dans une console ou le titre de la fenêtre

### 6.3 Ajustement en temps réel

**Ajout**: Modifier les paramètres de rendu sans redémarrer :
- `1-8` : Changer samplesPerPixel
- `[ ]` : Augmenter/réduire la profondeur de récursion
- `M` : Activer/désactiver les matériaux
- `L` : Activer/désactiver les lumières individuelles

### 6.4 Mode wireframe / Debug

**Ajout**: Modes d'affichage pour debug :
- Normales : couleur basée sur la normale
- UVs : visualisation des coordonnées de texture
- BVH : affichage des bounding boxes
- Profondeur : distance de l'objet le plus proche (z-buffer)

---

## 7. Architecture & Refactoring

### 7.1 Supprimer le code mort et les TODO

Rechercher et traiter tous les `TODO` et `(void)` casts dans le code :

| Fichier | TODO / Problème |
|---------|----------------|
| `src/geometry/AObject.cpp:72` | Transform bounding box to world space |
| `src/rendering/Texture.cpp:143` | Bilinear filtering |
| `src/platform/Window.cpp:116` | Set vertical sync |
| `src/app/main.cpp:42` | createDefaultScene() est un stub |
| `src/rendering/Renderer.cpp:201` | `(void)depth` inutile |

### 7.2 Remplacer `static_cast` par `dynamic_cast` ou `std::visit`

**Actuel**: `const PointLight& pl = static_cast<const PointLight&>(light);`

**Risque**: Si le type réel ne correspond pas (erreur de programmation), undefined behavior.

**Solution**: Utiliser `dynamic_cast` avec vérification, ou mieux, utiliser le polymorphisme :
- Ajouter des méthodes virtuelles à `ALight` pour éviter les casts
- Ou utiliser `std::variant<PointLight, DirectionalLight, AmbientLight>` avec `std::visit`

### 7.3 `try/catch(...)` trop large

**Problème**: Plusieurs endroits utilisent `catch (...) { ... }` pour la normalisation. Cela attrape toutes les exceptions, y compris les erreurs graves.

**Solution**: Lancer des exceptions spécifiques (`std::domain_error`) et les attraper sélectivement.

### 7.4 Const-correctness

Vérifier que toutes les méthodes qui ne modifient pas l'objet sont marquées `const`. La plupart le sont déjà, mais quelques cas restants :
- `getUVAt()` est `const` dans les objets → vérifier la cohérence
- `material->getTexture()` retourne un pointeur non-const → pourrait être `const Texture*`

### 7.5 Utilisation de `std::span` (C++20)

Remplacer les pointeurs nus + taille par `std::span` :
```cpp
// Au lieu de :
bool render(const Scene& scene, int width, int height, unsigned char* pixelBuffer);
// Utiliser :
bool render(const Scene& scene, std::span<unsigned char> pixelBuffer);
```

### 7.6 Factory pour les objets

Créer un pattern Factory pour la création d'objets depuis le parser, plutôt qu'un long if/else chain. Plus facile à étendre.

```cpp
class ObjectFactory {
    static std::shared_ptr<AObject> create(const std::string& type, const std::vector<double>& params);
};
```

---

## 8. Documentation

### 8.1 README.md amélioré

- Ajouter des captures d'écran des scènes d'exemple
- Diagramme d'architecture (rendu pipeline)
- Tableau des performances (taille × nombre d'objets × temps)
- Section "Contributing" pour les nouveaux développeurs

### 8.2 Wiki / Pages GitHub

- Tutoriel "Créer votre première scène .rt"
- Référence complète du format de fichier
- Exemples avancés (réflexions, réfractions, textures)

### 8.3 Doxygen — Compléter les exceptions

Beaucoup de méthodes lancent `std::runtime_error` mais les Doxygen comments ne documentent pas les `@throws` ou `@exception`.

---

## 9. Roadmap Suggérée

### Phase A — Corrections urgentes (1-2 jours)
| # | Tâche | Priorité |
|---|-------|----------|
| 1 | Corriger `getWorldBoundingBox` (world-space) | 🔴 Critique |
| 2 | Implémenter `Texture::sampleFiltered` (bilinéaire) | 🟡 Haute |
| 3 | Tests de base (Vec3, Ray, Sphere, Plane) | 🟡 Haute |
| 4 | Ajouter JPEG loading dans Texture | 🟡 Haute |

### Phase B — Performance (2-3 jours)
| # | Tâche | Priorité |
|---|-------|----------|
| 5 | SAH BVH | 🟢 Moyenne |
| 6 | Thread pool permanent | 🟢 Moyenne |
| 7 | Profilage intégré | 🟢 Moyenne |
| 8 | Tiling du framebuffer | 🔵 Basse |

### Phase C — Rendu avancé (3-5 jours)
| # | Tâche | Priorité |
|---|-------|----------|
| 9 | Microfacet BRDF (Cook-Torrance) | 🟢 Moyenne |
| 10 | Textures procédurales | 🟢 Moyenne |
| 11 | Normal mapping | 🟢 Moyenne |
| 12 | Area lights | 🔵 Basse |

### Phase D — Polish (2-3 jours)
| # | Tâche | Priorité |
|---|-------|----------|
| 13 | Refactoring `static_cast` → polymorphisme | 🟢 Moyenne |
| 14 | Traiter tous les TODO restants | 🟢 Moyenne |
| 15 | Documentation et Doxygen | 🟢 Moyenne |
| 16 | Rendu progressif | 🔵 Basse |

---

## Résumé des forces actuelles

- ✅ Architecture modulaire propre (6 modules bien séparés)
- ✅ Documentation Doxygen exhaustive
- ✅ Multithreading natif opérationnel
- ✅ BVH acceleration structure
- ✅ Anti-aliasing par échantillonnage
- ✅ Ombres douces configurables
- ✅ Réflexions et réfractions récursives
- ✅ Tone mapping (Reinhard) + sRGB encoding
- ✅ Smart pointers partout (`shared_ptr`, `unique_ptr`)
- ✅ Bonne gestion des edge cases (division par zéro, normalisation)

## Métriques du projet

| Métrique | Valeur |
|----------|--------|
| Fichiers source | ~30 (15 headers + 15 implémentations) |
| Lignes de code estimées | ~6000-7000 |
| Tests unitaires | 13 (Cylinder + Cone) |
| Scènes d'exemple | 8 |
| Dépendances | SDL2, libpng, libjpeg |
| Langage | C++23 |
| Bugs connus | `getWorldBoundingBox` (TODO), bilinear filtering (TODO) |
