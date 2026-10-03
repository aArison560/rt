# RT — Architecture de Blender et transferts vers notre moteur

> Objectif : comprendre **comment un logiciel 3D de référence est architecturé**, et en tirer
> les décisions concrètes pour RT (format de scène, invalidation, séparation calcul/affichage).
>
> Intérêt pour le projet : Blender valide nos choix d'architecture et fournit un **vocabulaire
> et des justifications** utiles pendant la soutenance (item *Group organization* et questions
> « pourquoi ce design ? »).
>
> ⚠ Les détails de version (backends GPU, moteurs) évoluent vite : les affirmations marquées
> `⚠ version` sont à revalider sur la version de Blender utilisée comme référence.

---

## Sommaire

1. [Les couches de Blender](#1-les-couches-de-blender)
2. [Les 4 idées qui font Blender](#2-les-4-idées-qui-font-blender)
3. [Cycles — le moteur de rayons](#3-cycles--le-moteur-de-rayons)
4. [Le rendu réseau dans Blender](#4-le-rendu-réseau-dans-blender)
5. [Les 5 transferts directs vers RT](#5-les-5-transferts-directs-vers-rt)
6. [Ce qu'on ne doit pas copier](#6-ce-quon-ne-doit-pas-copier)
7. [Arguments à réutiliser en soutenance](#7-arguments-à-réutiliser-en-soutenance)

---

## 1. Les couches de Blender

Du bas vers le haut :

```
┌─────────────────────────────────────────────────────────────┐
│ Python (bpy) — add-ons, scripts, tout passe par RNA         │
├─────────────────────────────────────────────────────────────┤
│ Éditeurs : WindowManager → Screen → Area → Region          │
│   view3d, outliner, node editor, properties, timeline…      │
│   (redessin tripartite, hiérarchie de contextes)            │
├─────────────────────────────────────────────────────────────┤
│ Notifiers — bus d'événements : les éditeurs réagissent      │
│             aux changements sans couplage direct            │
├───────────────────────┬─────────────────────────────────────┤
│ Render engines        │ Draw engines / GPU backend          │
│ • EEVEE (raster GPU)  │ abstraction OpenGL/Vulkan/Metal ⚠   │
│ • Cycles (path trace) │ shader compiler, buffers GPU        │
├───────────────────────┴─────────────────────────────────────┤
│ Depsgraph — graphe de dépendances, évaluation incrémentale, │
│             invalidation ciblée, ordonnancement              │
├─────────────────────────────────────────────────────────────┤
│ BKE (Blender Kernel) — logique métier : modifiers,          │
│   armatures, relations objet/scène, opérations sur données  │
├─────────────────────────────────────────────────────────────┤
│ RNA — couche de propriétés déclarative au-dessus du DNA :   │
│   noms, types, contraintes min/max, callbacks, UI auto,     │
│   bindings Python générés                                   │
├─────────────────────────────────────────────────────────────┤
│ DNA — structs C « plats », sérialisés tels quels dans       │
│   le fichier .blend (dump mémoire versionné)                │
├─────────────────────────────────────────────────────────────┤
│ blenlib / IMB — maths, chaînes, listes, images, I/O         │
└─────────────────────────────────────────────────────────────┘
```

---

## 2. Les 4 idées qui font Blender

| Système | Rôle | Pourquoi c'est malin |
|---------|------|----------------------|
| **DNA** | Un **schéma unique** décrit en struct C : format de fichier **et** modèle de données | Le `.blend` est un dump mémoire : charger = *memcpy* + migration de version |
| **RNA** | Propriétés déclarées au-dessus du DNA (type, min/max, doc, callbacks) | L'UI, la validation, l'animation et le Python **dérivent toutes du même schéma** → zéro duplication, zéro divergence |
| **Depsgraph** | Ne réévalue **que ce qui a changé** | Deux graphes distincts : les données « originales » (éditées) et les données « evaluated » (après modifiers) |
| **Notifiers** | Bus d'événements découplé | L'UI ne connaît pas les données : elle **s'abonne** aux changements |

**Règle transposable** : une seule source de vérité décrit les données ; parsing, validation,
interface et export en dérivent. Toute duplication finit par diverger.

---

## 3. Cycles — le moteur de rayons

Cycles est le **path tracer** de Blender : c'est le plus proche de ce que nous construisons.

```
Scene (meshes, objects, lights, shader nodes)
  │  conversion du shader graph en SVM (Shader Virtual Machine)
  ▼
Device abstraction ── CPU : oneAPI/TBB + Embree (BVH) ⚠ version
                     ── GPU : CUDA, OptiX, HIP, Metal, ROCm ⚠ version
  ▼
BVH build  →  Path tracing par tuiles/régions  →  Film
  (accumulation, filtering, adaptive sampling, denoising)  →  Display
```

### 3.1 Ce qui est remarquable

| Mécanisme | Principe | Comparaison avec RT |
|-----------|----------|---------------------|
| **Device abstraction** | Une interface `Device`, plusieurs backends (CPU/GPU) | Notre `ThreadPool` = un « backend » parmi d'autres ; on pourrait ajouter un backend sans toucher au `Renderer` |
| **BVH dédié** | Construction optimisée (SAH), parcours séparé du rendu | Notre `BVH` + `objectVersion` — à documenter pareil |
| **Embarrassingly parallel** | Découpage en régions/tuiles indépendantes | exactement notre `renderRegion()` et la base du cluster |
| **Film ≠ Display** | L'accumulation (samples) est séparée de l'affichage | **l'exigence M6** (redisplay sans recalcul) : Blender ne recalcule jamais pour afficher |
| **Adaptive sampling** | Plus d'échantillons là où le bruit est élevé | idée pour un item « crazy stuff » / qualité |
| **Light tree, MIS** | structures de lumières + combinaison des estimateurs | si on atteint les items *Caustics / GI* |
| **Denoising** (OIDN/OptiX) ⚠ version | post-traitement du bruit | hors périmètre raisonnable pour nous |

---

## 4. Le rendu réseau dans Blender

**Blender ne fait pas de rendu réseau nativement** (la fonctionnalité a été retirée autour de
2.80). Les solutions réelles :

| Solution | Type | Remarque |
|----------|------|----------|
| **Flamenco** | Ferme de rendu officielle (Blender Foundation) | Manager + workers, jobs « frame range » et « tile », communication HTTP |
| Add-ons tiers | CrowdRender, K-Cycles, etc. | Hors projet officiel |
| Découpage par frame | chez un prestataire cloud | le plus courant en production |

> **Leçon pour RT** : même un logiciel mature ne fait **pas** du rendu réseau « gratuitement ».
> C'est un sous-système à part entière → cela justifie notre choix de commencer par
> l'orchestration SSH ([DISTRIBUTED_RENDERING.md](DISTRIBUTED_RENDERING.md)) avant un
> coordinateur maison.

---

## 5. Les 5 transferts directs vers RT

| # | Blender | Notre projet | Priorité |
|---|---------|--------------|----------|
| 1 | **DNA/RNA** : un schéma unique → fichier + UI + validation | Décrire une **seule fois** le format `.rt` structuré ; le parser, l'UI (microui) et la validation en dérivent ([ARCHITECTURE.md §5](ARCHITECTURE.md)) | **Haute** (bloque `File ++`) |
| 2 | **Depsgraph** + invalidation ciblée | `Scene::objectVersion` invalide la BVH ; deux drapeaux `sceneDirty` / `displayDirty` | **Haute** (découpe calcul/affichage) |
| 3 | **Render engine ≠ Display** | Cycles calcule, l'écran affiche : **exactement l'exigence M6** (« expose sans recalcul ») | **Éliminatoire** |
| 4 | **Session / jobs / tuiles** | `ThreadPool` + `renderRegion()` aujourd'hui ; tuiles réseau demain | Moyenne |
| 5 | **Notifiers** (bus découplé) | `EventHandler` SDL : `EXPOSED`, resize, clavier → le calcul ne s'accroche pas à l'événement | Moyenne |

### 5.1 Schéma cible inspiré de Blender

```
        fichier .rt (schéma unique = source de vérité)
                     │
        ┌────────────┼─────────────┐
        ▼            ▼             ▼
   SceneParser   Validation    UI (microui)
        │        (types/ranges)   │
        └────────────┼─────────────┘
                     ▼
                   Scene ──── objectVersion ──► invalidation BVH
                     │
        sceneDirty ──┤
                     ▼
               Renderer::render()  ──► framebuffer (état persistant)
                     │
        displayDirty / EXPOSED ───► blit framebuffer → texture SDL
                                     (aucun recalcul)
```

---

## 6. Ce qu'on ne doit pas copier

| Élément Blender | Pourquoi ce n'est pas pour nous |
|-----------------|----------------------------------|
| **DNA** : structs mémoire = format de fichier | Trop couplé à la version du logiciel ; un format **texte structuré** reste lisible, versionnable et modifiable devant un correcteur |
| **Python partout** | Le sujet impose C/C++/Rust et interdit de s'appuyer sur un outil externe pour la démo |
| **Depsgraph complet** (graphe de dépendances générique) | Nos dépendances sont simples (objet → BVH → rendu) ; un compteur d'invalidation suffit |
| **Écosystème d'éditeurs** | Nous avons besoin d'un viewer + d'un panneau, pas d'un logiciel complet |
| **Toute la couche GPU** | **Interdite par le sujet** pour la génération de l'image finale |

---

## 7. Arguments à réutiliser en soutenance

| Question du correcteur | Réponse préparée |
|------------------------|------------------|
| « Pourquoi ce format de scène ? » | Un **schéma unique** décrit les données ; parsing, validation et UI en dérivent — c'est le même principe que le DNA/RNA de Blender, appliqué à notre `.rt` |
| « Comment évitez-vous de tout recalculer ? » | Deux drapeaux : la scène est invalidée par un compteur (`objectVersion`), l'affichage recopie le framebuffer ; c'est la séparation *render engine / display* que Blender appelle *Film vs Display* |
| « Pourquoi la BVH est reconstruite à ce moment-là ? » | Invalidation **ciblée**, pas de reconstruction à chaque frame — le principe du *depsgraph* |
| « Vous vous êtes inspiré de quoi ? » | Blender (DNA/RNA, invalidation), Webserv (allocation déterministe — [MEMORY_STRATEGY.md](MEMORY_STRATEGY.md)) |

---

## Références

- Code source et documentation : <https://developer.blender.org>
- Cycles : <https://www.cycles-renderer.org>
- Ferme de rendu Flamenco : <https://flamenco.render.org>
- Transferts vers l'implémentation : [ARCHITECTURE.md](ARCHITECTURE.md),
  [DISTRIBUTED_RENDERING.md](DISTRIBUTED_RENDERING.md)
