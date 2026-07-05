Phase 1 — Fondation

| # | Tâche | Détails |
|---|-------|---------|
| 1 | Intégrer Dear ImGui | Ajouter les sources, Makefile, initialisation SDL2 |
| 2 | Hello World overlay | Afficher "Hello RT" dans une fenêtre ImGui |
| 3 | Raccourci H toggle | Afficher/masquer l'interface |

Phase 2 — Panneaux essentiels

| # | Tâche | Détails |
|---|-------|---------|
| 4 | Render Settings panel | Toggle shadows/reflections, samples slider |
| 5 | Info overlay | FPS, render time, objects count |
| 6 | Bouton Re-render | Forcer un re-render depuis l'UI |

Phase 3 — Création d'objets

| # | Tâche | Détails |
|---|-------|---------|
| 7 | Boutons Sphere/Plane/Cylinder/Cone | Création avec paramètres par défaut |
| 8 | Panneau paramètres | Position, rayon, hauteur |
| 9 | Re-render automatique après création |

Phase 4 — Sélection & édition

| # | Tâche | Détails |
|---|-------|---------|
| 10 | Ray-picking au clic | Sélection d'objet via intersection rayon |
| 11 | Panneau d'édition | Position, matériau, couleur |
| 12 | Suppression d'objet | Depuis le panneau d'édition |

Phase 5 — Gestion de scène

| # | Tâche | Détails |
|---|-------|---------|
| 13 | Scene Explorer | Liste arborescente des objets |
| 14 | Save/Load | Export/import de fichiers .rt |
| 15 | Duplicate object | Copie d'objet avec offset |

Phase 6 — Polish

| # | Tâche | Détails |
|---|-------|---------|
| 16 | Debug views | Normals, UV, BVH, Depth |
| 17 | Material presets | Banque de matériaux prédéfinis |
| 18 | Gizmo 3D | Manipulateur translation/rotation |
| 19 | Rendu progressif | Affichage immédiat + raffinage |
| 20 | Material preview | Petite sphère preview en temps réel |
| 21 | Color picker | Avec preview interactive |