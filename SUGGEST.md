# RT — Interface Interactive & Contrôles UI

> **Date**: 5 juillet 2026
> **Auteur**: Buffy (Codebuff)

---

Ce document détaille les propositions d'ajout d'une **interface utilisateur interactive** (boutons, panneaux, sliders) au raytracer RT, actuellement limité aux contrôles clavier (WASD + touches).

---

## Table des matières

1. [Approche technique recommandée](#1-approche-technique-recommandée)
2. [Panneau de création d'objets](#2-panneau-de-création-dobjets)
3. [Sélection & manipulation d'objets](#3-sélection--manipulation-dobjets)
4. [Contrôles de rendu en temps réel](#4-contrôles-de-rendu-en-temps-réel)
5. [Gestion des lumières](#5-gestion-des-lumières)
6. [Scene Explorer (liste d'objets)](#6-scene-explorer-liste-dobjets)
7. [Sauvegarde / Chargement de scène](#7-sauvegarde--chargement-de-scène)
8. [Debug Visualizations](#8-debug-visualizations)
9. [Fonctionnalités bonus "cool"](#9-fonctionnalités-bonus-cool)
10. [Workflow utilisateur complet](#10-workflow-utilisateur-complet)
11. [Roadmap d'implémentation](#11-roadmap-dimplémentation)

---

## 1. Approche technique recommandée

### ❌ Ne pas faire : UI en SDL2 pur

Coder des boutons, sliders et fenêtres avec les primitives SDL2 (`SDL_RenderFillRect`, `SDL_RenderDrawLine`, etc.) est :

- Extrêmement laborieux (chaque widget = ~50-100 lignes)
- Difficile à maintenir (layout, redimensionnement, z-ordering)
- Pauvre en fonctionnalités (pas de color picker, pas de drag & drop)

### ✅ Recommandation : Dear ImGui

**[Dear ImGui](https://github.com/ocornut/imgui)** est une bibliothèque C++ de GUI **immediate-mode**, standard de facto pour les outils de rendu et de game dev.

| Critère | SDL2 pur | Dear ImGui |
|---------|----------|------------|
| Code pour un bouton | ~50 lignes | **1 ligne** `if (ImGui::Button(...))` |
| Slider | ~80 lignes | **1 ligne** `ImGui::SliderFloat(...)` |
| Color picker | ~200 lignes | **1 ligne** `ImGui::ColorEdit3(...)` |
| Redimensionnement / Layout | À coder soi-même | **Automatique** |
| Thème / Style | Rien | **Intégré** |
| Support clavier/souris | À gérer | **Automatique** |
| Fenêtres flottantes | Non | **Oui** |

**Intégration** avec le projet existant :

```
src/app/main.cpp:
  1. Initialiser ImGui après SDL_Init
  2. Créer texture ImGui dans le renderer SDL
  3. Dans la boucle loop : ImGui_ImplSDL2_NewFrame() → ImGui::NewFrame()
  4. Rendre l'interface utilisateur (boutons, fenêtres)
  5. ImGui::Render() → SDL_RenderPresent()
```

Le rendu raytracing reste inchangé. ImGui s'affiche en **overlay** par-dessus l'image. Aucune modification du pipeline de rendu nécessaire.

**Dépendances à ajouter** :
- `imgui` (bibliothèque header-only ou fichiers .cpp)
- `imgui/backends/imgui_impl_sdl2.h` (backend SDL2)
- `imgui/backends/imgui_impl_sdlrenderer.h` (backend SDL_Renderer)

---

## 2. Panneau de création d'objets

Un volet latéral (ou fenêtre flottante) avec des **boutons** pour spawner des primitives dans la scène.

### Maquette conceptuelle

```
┌─── Create Object ───────┐
│                         │
│  ┌──────────────────┐   │
│  │  🟠 Sphere       │   │
│  └──────────────────┘   │
│  ┌──────────────────┐   │
│  │  🔵 Plane        │   │
│  └──────────────────┘   │
│  ┌──────────────────┐   │
│  │  🟢 Cylinder     │   │
│  └──────────────────┘   │
│  ┌──────────────────┐   │
│  │  🔺 Cone         │   │
│  └──────────────────┘   │
└─────────────────────────┘
```

### Interactions

| Bouton | Action |
|--------|--------|
| **Sphere** | Crée une sphère à la position du focus caméra, rayon = 1.0 |
| **Plane** | Crée un plan horizontal à y=0 |
| **Cylinder** | Crée un cylindre, hauteur=2.0, rayon=0.5 |
| **Cone** | Crée un cône, hauteur=2.0, halfAngle=25° |

### Après clic — Panneau de paramètres

Un sous-panneau s'ouvre pour régler finement l'objet avant validation :

```
┌─── Edit Sphere ────────────┐
│ Name: [Sphere_4          ] │
│                            │
│ ── Position ────────────── │
│ X: [ 0.00]  Y: [ 1.50]    │
│ Z: [-3.00]                 │
│                            │
│ ── Geometry ────────────── │
│ Radius: [──●──────────] 1.0│
│                            │
│ ── Material ────────────── │
│ Color:  [■───────────────] │
│ Ambient:  [──●──────────]  │
│ Diffuse:  [─────●───────]  │
│ Specular: [──●──────────]  │
│ Shininess:[─────●───────]  │
│ Reflect:  [─────────────]  │
│                            │
│  [✕ Cancel] [✓ Confirm]   │
└────────────────────────────┘
```

### Comportement

- **Re-render automatique** après confirmation
- L'objet est ajouté au `Scene::objects` via `addObject()`
- La **BVH** est automatiquement invalidée et reconstruite au prochain render

---

## 3. Sélection & manipulation d'objets

Cliquer sur le rendu pour sélectionner un objet, puis éditer ses propriétés.

### Ray-picking

Implémentation de la sélection par clic :

1. Au clic, calculer les coordonnées UV de la souris dans l'image
2. Lancer un rayon de la caméra à travers ce pixel via `Camera::generateRay()`
3. Utiliser `Renderer::castRay()` existant pour trouver l'intersection la plus proche
4. Récupérer l'objet intersecté depuis le `HitRecord`

### Panneau d'édition

```
┌─── Selected: Sphere #3 ────┐
│                             │
│ ── Transform ────────────── │
│ X: [─●───────────────] 0.0 │
│ Y: [───●─────────────] 1.5 │
│ Z: [─────●───────────] -3.0│
│                             │
│ ┌────────────────────────┐  │
│ │ 📐 Rotation            │  │
│ │ Rx: [──●──] Ry: [──●──]│  │
│ │ Rz: [──●──]            │  │
│ └────────────────────────┘  │
│                             │
│ ── Material ─────────────── │
│ Color:      [■───────────]  │
│ Shininess:  [──────●────]   │
│ Reflectivity:[──────────]0  │
│ Transparency:[──────────]0  │
│                             │
│ ┌────────────────────────┐  │
│ │  🔄 Duplicate  ❌ Delete│  │
│ └────────────────────────┘  │
└────────────────────────────┘
```

### Contrôles clavier pour l'objet sélectionné

| Touche | Action |
|--------|--------|
| **G** | "Grab" — activer le déplacement à la souris |
| **R** | "Rotate" — activer la rotation |
| **S** | "Scale" — activer le redimensionnement |
| **Suppr** | Supprimer l'objet |
| **Ctrl+D** | Dupliquer l'objet |

### Gizmo 3D

Optionnel mais impressionnant : afficher un **gizmo** (manipulateur 3D) sur l'objet sélectionné, avec des flèches pour translater/rotation/scale directionnel.

---

## 4. Contrôles de rendu en temps réel

Un panneau pour modifier les paramètres de rendu sans redémarrer.

```
┌─── Render Settings ────────┐
│                             │
│ Samples per pixel           │
│ [│││││││──────]  8         │
│                             │
│ Max recursion depth         │
│ [││││─────────]  4         │
│                             │
│ ── Toggles ─────────────── │
│ ☑ Shadows                  │
│ ☑ Reflections              │
│ ☑ Refractions              │
│ ☐ Anti-aliasing            │
│ ☑ Soft shadows             │
│                             │
│ ── Quality Presets ─────── │
│ [Draft] [Medium] [High]    │
│                             │
│ Render time: 1.24s         │
│ ┌────────────────────────┐  │
│ │  🅁 Re-render Now      │  │
│ └────────────────────────┘  │
└────────────────────────────┘
```

### Rendu progressif

Amélioration couplée : au lieu d'un render complet bloquant, afficher les pixels **au fur et à mesure** :

1. **Pass 1** : 1 échantillon/pixel → image instantanée mais bruitée
2. **Pass 2** : 4 échantillons/pixel → raffinage
3. **Pass N** : jusqu'au nombre configuré

L'utilisateur voit la scène immédiatement et peut interagir sans attendre.

### Touches de raccourci

| Touche | Action |
|--------|--------|
| **1-8** | Changer samplesPerPixel (1 = 1 sample, 8 = 64 samples) |
| **-** | Réduire profondeur de récursion |
| **=** | Augmenter profondeur de récursion |
| **F11** | Plein écran |
| **H** | Afficher/masquer l'interface (toggle HUD) |

---

## 5. Gestion des lumières

Panneau dédié pour visualiser et modifier les sources lumineuses.

```
┌─── Lights ──────────────────┐
│                              │
│ 💡 Ambient                    │
│    Intensity: [───●─────] 0.2│
│    Color: [■───────────────] │
│                              │
│ 💡 Point Light #1            │
│ ☑ Active                     │
│    Position:                 │
│    X: [ 5.00] Y: [ 5.00]    │
│    Z: [ 5.00]                │
│    Intensity: [─────●───] 1.0│
│    Color: [■───────────────] │
│    Attenuation:              │
│    Constant: [──●────]       │
│    Linear:   [────●──]       │
│    Quadratic:[──────●]       │
│    ┌────────────────────┐    │
│    │  ❌ Remove Light   │    │
│    └────────────────────┘    │
│                              │
│ 💡 Directional Light #2      │
│ ☑ Active                     │
│    Direction:                │
│    Dx: [ 1.00] Dy: [ 1.00]  │
│    Dz: [ 0.50]               │
│    Intensity: [───●────] 0.8 │
│                              │
│ ┌─────────────────────────┐  │
│ │  ✚ Add Point Light     │  │
│ └─────────────────────────┘  │
│ ┌─────────────────────────┐  │
│ │  ✚ Add Directional     │  │
│ └─────────────────────────┘  │
└─────────────────────────────┘
```

### Icônes dans le viewport (optionnel)

Afficher de petites **icônes lumineuses** (●) dans le rendu à la position des lumières ponctuelles, pour savoir où elles se trouvent visuellement.

---

## 6. Scene Explorer (liste d'objets)

Un **outline** affichant tous les objets et lumières de la scène dans une arborescence.

```
┌── Scene Explorer ──────────┐
│                            │
│ 🔍 [Filter objects...   ]  │
│                            │
│ 📦 Objects (4)             │
│  ├─ 👁 🟠 Sphere          │
│  │    └─ material: red    │
│  ├─ 👁 🔵 Plane           │
│  │    └─ material: grey   │
│  ├─ 👁 🟢 Cylinder       │
│  │    └─ material: blue   │
│  └─ 👁 🔺 Cone           │
│       └─ material: green  │
│                            │
│ 💡 Lights (3)              │
│  ├─ 🟡 Ambient            │
│  ├─ 🟡 Point Light #1     │
│  └─ 🟡 Directional #1     │
│                            │
│ ┌────────────────────────┐ │
│ │  +  ─  ↻  ↑  ↓        │ │
│ └────────────────────────┘ │
└────────────────────────────┘
```

### Interactions

| Élément | Action |
|---------|--------|
| **👁️** | Afficher/masquer l'objet sans le supprimer |
| **Clic sur nom** | Sélectionner l'objet + ouvre le panneau d'édition |
| **↑ / ↓** | Réordonner les objets (ordre de rendu) |
| **➕** | Menu contextuel pour créer un objet |
| **➖** | Supprimer l'objet sélectionné |
| **↻** | Renommer l'objet |
| **Drag & drop** | Réorganiser la liste (si supporté par ImGui) |
| **Clic droit** | Menu contextuel : Dupliquer, Supprimer, Rename |

### Informations affichées

- Type d'objet avec icône (🟠 Sphere, 🔵 Plane, 🟢 Cylinder, 🔺 Cone)
- Nom de l'objet
- Aperçu du matériau (cercle coloré)
- Badge si l'objet est masqué

---

## 7. Sauvegarde / Chargement de scène

Permet de ne pas perdre les modifications interactives.

```
┌─── File ────────────────────┐
│                              │
│  💾 Save Scene               │
│  📂 Load Scene…              │
│  🆕 New Scene                │
│  ────────────────────────    │
│  📸 Export Screenshot        │
│  📽 Export PNG Sequence      │
└──────────────────────────────┘
```

### Détail des actions

| Action | Description |
|--------|-------------|
| **Save Scene** | Sérialise la scène actuelle au format `.rt` — régénère le fichier avec tous les objets, lumières, matériaux et position caméra |
| **Load Scene…** | Ouvre un file dialog → parse le fichier `.rt` via `SceneParser` → remplace la scène actuelle |
| **New Scene** | Réinitialise la scène avec les valeurs par défaut (camera + lumière ambiante) |
| **Export Screenshot** | Sauvegarde le framebuffer en PNG (utilise `ImageBuffer::savePNG()` existant) |
| **Export PNG Sequence** | Pour animations : exporte chaque frame re-rendue en PNG numéroté |

### Sauvegarde automatique (optionnel)

- Auto-save toutes les 60 secondes dans un fichier `.rt.autosave`
- Récupération en cas de crash

---

## 8. Debug Visualizations

Des modes d'affichage pour le développement, activables d'un clic.

```
┌─── Debug Visualizations ────┐
│                              │
│  [Normals] [UV] [BVH]       │
│  [Depth]  [Wireframe]       │
│                              │
│  ── Overlay ────────────── │
│  ☑ FPS counter: 24.5        │
│  ☐ Render time              │
│  ☐ Ray count                │
│  ☐ BVH stats                │
└──────────────────────────────┘
```

### Modes de visualisation

| Mode | Description | Résultat visuel |
|------|-------------|-----------------|
| **Normals** | Couleur = direction de la normale | Rendu arc-en-ciel |
| **UV** | Visualisation des coordonnées UV | Quadrillage UV |
| **BVH** | Affiche les AABB (bounding boxes) | Boîtes filaires superposées |
| **Depth** | Carte de profondeur (near→far = blanc→noir) | Z-buffer |
| **Wireframe** | Grille et axes de référence | Grille au sol |

### Overlay d'informations

Affichage en HUD (toujours visible ou toggle) :

```
RT v1.0  |  1920×1080  |  FPS: 24.5
Render: 1.24s  |  Rays: 2,457,600
Objects: 4  |  Lights: 3  |  Samples: 8
Camera: (0.0, 1.5, 8.0)  →  (0.0, -0.2, -1.0)
```

---

## 9. Fonctionnalités bonus "cool"

Des idées supplémentaires pour impressionner.

### 9.1 Camera selfie

Bouton qui **place automatiquement la caméra face à l'objet sélectionné** :

```
Sélectionner un objet → clic droit → "Frame Object"
→ La caméra se déplace pour centrer l'objet dans le viewport
→ Re-render automatique
```

### 9.2 Animation path

Enregistrer une séquence de positions caméra et générer une animation :

```
┌─── Animation ─────────────┐
│                            │
│  [●] Keyframe 1            │
│  [ ] Keyframe 2            │
│  [ ] Keyframe 3            │
│                            │
│  ┌──────────────────────┐  │
│  │  ● Record Keyframe   │  │ ← enregistre position actuelle
│  └──────────────────────┘  │
│  [▶ Preview Animation]    │
│  [📽 Export Frames]       │ → exporte PNG sequence
│  Frames:[100]  FPS:[24]   │
└────────────────────────────┘
```

### 9.3 Material presets

Banque de matériaux pré-définis applicables d'un clic :

```
┌─── Material Presets ───────┐
│  [🔴 Red Plastic]          │
│  [🟢 Green Rubber]         │
│  [🔵 Blue Metallic]        │
│  [⚪ Mirror]               │
│  [🔮 Glass]                │
│  [🏆 Gold]                 │
│  [🌊 Water]                │
│  [🌑 Matte Black]          │
│                            │
│  ┌──────────────────────┐  │
│  │  + Save Current as…  │  │
│  └──────────────────────┘  │
└────────────────────────────┘
```

### 9.4 Color picker avec preview en temps réel

Pour les matériaux et lumières :

```
Color: [■─────────────────────]
       │   ┌─────────────┐   │
       │   │ ●           │   │
       │   │   ■■■■■■■   │   │
       │   │   ■■■■■■■   │   │
       │   │   ■■■■■■■   │   │
       │   └─────────────┘   │
       │ R: [128] G:[64] B:[255]│
       └────────────────────────┘
```

### 9.5 Render queue

Lancer le rendu de plusieurs scènes `.rt` en séquence :

```
┌─── Render Queue ──────────┐
│  [scenes/default.rt]  ✅   │
│  [scenes/complex.rt]  ✅   │
│  [scenes/gallery.rt]  ⏳   │
│  [scenes/showcase.rt] ⏳   │
│  [scenes/still_life.rt]⏳  │
│                            │
│  [▶ Start Batch Render]    │
│  Progress: ██░░░░░ 25%     │
└────────────────────────────┘
```

### 9.6 Material preview sphere

Un petit rendu en temps réel d'une sphère avec le matériau actuel :

```
┌─── Material Preview ──────┐
│  ┌──────────────────────┐  │
│  │                      │  │
│  │    [●] ← rendered   │  │
│  │     sphere with      │  │
│  │    current material  │  │
│  │                      │  │
│  └──────────────────────┘  │
│  Color: [■──────────────] │
│  Roughness: [───●───────] │
└────────────────────────────┘
```

### 9.7 Éditeur de scène visuel

Déplacer les objets directement dans la vue 3D :

- **Click + drag** sur un objet (avec ray-picking) pour le déplacer
- **Molette** pour zoomer sur l'objet sélectionné
- **Shift + click** pour sélection multiple
- **Ctrl+Z** pour undo la dernière action

---

## 10. Workflow utilisateur complet

Exemple de session utilisateur typique avec toutes ces fonctionnalités :

```
1. Lancer ./rt → la scène s'affiche
2. Ajuster la caméra avec WASD (comportement existant)
3. Cliquer sur le panneau "Create" → "Sphere"
4. Régler le rayon et la couleur dans le panneau d'édition
5. Cliquer "Confirm" → la sphère apparaît automatiquement
6. Cliquer sur la sphère dans le rendu → elle se sélectionne
7. Utiliser le slider de position pour la déplacer
8. Ajouter une Point Light depuis le panneau Lights
9. Augmenter samplesPerPixel à 16 pour un rendu plus propre
10. Appliquer le preset "Glass" au matériau
11. Sauvegarder la scène → fichier .rt généré
12. Exporter une capture d'écran en PNG
```

---

## 11. Roadmap d'implémentation

### Phase 1 — Fondation (2-3 jours)

| # | Tâche | Détails |
|---|-------|---------|
| 1 | Intégrer Dear ImGui | Ajouter les sources, Makefile, initialisation SDL2 |
| 2 | Hello World overlay | Afficher "Hello RT" dans une fenêtre ImGui |
| 3 | Raccourci H toggle | Afficher/masquer l'interface |

### Phase 2 — Panneaux essentiels (3-4 jours)

| # | Tâche | Détails |
|---|-------|---------|
| 4 | Render Settings panel | Toggle shadows/reflections, samples slider |
| 5 | Info overlay | FPS, render time, objects count |
| 6 | Bouton Re-render | Forcer un re-render depuis l'UI |

### Phase 3 — Création d'objets (2-3 jours)

| # | Tâche | Détails |
|---|-------|---------|
| 7 | Boutons Sphere/Plane/Cylinder/Cone | Création avec paramètres par défaut |
| 8 | Panneau paramètres | Position, rayon, hauteur |
| 9 | Re-render automatique après création |

### Phase 4 — Sélection & édition (3-4 jours)

| # | Tâche | Détails |
|---|-------|---------|
| 10 | Ray-picking au clic | Sélection d'objet via intersection rayon |
| 11 | Panneau d'édition | Position, matériau, couleur |
| 12 | Suppression d'objet | Depuis le panneau d'édition |

### Phase 5 — Gestion de scène (2-3 jours)

| # | Tâche | Détails |
|---|-------|---------|
| 13 | Scene Explorer | Liste arborescente des objets |
| 14 | Save/Load | Export/import de fichiers .rt |
| 15 | Duplicate object | Copie d'objet avec offset |

### Phase 6 — Polish (3-5 jours)

| # | Tâche | Détails |
|---|-------|---------|
| 16 | Debug views | Normals, UV, BVH, Depth |
| 17 | Material presets | Banque de matériaux prédéfinis |
| 18 | Gizmo 3D | Manipulateur translation/rotation |
| 19 | Rendu progressif | Affichage immédiat + raffinage |
| 20 | Material preview | Petite sphère preview en temps réel |
| 21 | Color picker | Avec preview interactive |

---

## Notes techniques

### Architecture

- **ImGui** s'intègre comme **overlay** — ne modifie pas le pipeline de rendu
- Les modifications de scène (add/remove/edit object) passent par l'API `Scene` existante
- La BVH est invalidée et reconstruite automatiquement via le mécanisme `objectVersion`
- Les contrôles clavier existants (WASD, ESC) restent inchangés
- L'interface est **optionnelle** — le programme fonctionne sans (comportement actuel)

### Fichiers à créer

```
src/gui/
├── GuiManager.cpp         # Initialisation et rendu ImGui
├── GuiManager.hpp         # Header
├── panels/
│   ├── RenderSettingsPanel.cpp
│   ├── ObjectCreationPanel.cpp
│   ├── ObjectEditPanel.cpp
│   ├── SceneExplorerPanel.cpp
│   ├── LightPanel.cpp
│   └── DebugPanel.cpp
└── presets/
    └── MaterialPresets.cpp  # Banque de matériaux
```

### Makefile

```makefile
# À ajouter
IMGUI_DIR = lib/imgui
IMGUI_SRCS = $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_draw.cpp \
             $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp \
             $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp \
             $(IMGUI_DIR)/backends/imgui_impl_sdlrenderer.cpp

# Linker
LDFLAGS += -limgui  # ou inclure les sources directement
```
