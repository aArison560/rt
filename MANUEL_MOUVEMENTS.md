# Manuel Mouvements — RTv1 Blender-like

Déplacements interactifs fluides (clavier + souris), conventions Blender.
Lancer : `./bin/rtv1 scenes/move_object.rt` · scripts/tests : `./bin/rtv1 scenes/exemple.rt --once`

## 1. Sélection

| Touche | Effet |
|---|---|
| `Clic` (hors modal) | Sélectionne l'objet sous le curseur (lancer de rayon, ciel = garde sélection) |
| `0-9` | Sélectionne l'objet N (liste affichée au démarrage) |
| `Tab` | Objet suivant |
| `H` | Rappelle l'aide dans le terminal |

Le titre de la fenêtre affiche `[MODE\|AXE] obj i/n (preview)`.

## 2. Modes

| Touche | Mode | Souris | Molette | Flèches |
|---|---|---|---|---|
| `G` | Grab objet sélectionné | Déplace dans le plan caméra | Profondeur (avant/arrière) | Nudge 0.1 (`Shift` = 0.01) |
| `R` | Rotate direction objet (`cyl`, `cone`, `parab`, `hyp`, `pl`) | X = lacet (autour Y monde), Y = tangage (autour `right` caméra) | — | Pas de 2° (`Shift` = 0.5°) |
| `C` | Caméra | Pan (plan caméra) | Dolly avant/arrière (×0.3, `Shift` ×0.05) | Nudge caméra |
| `L` | Lumière | Déplace dans le plan caméra | Profondeur | Nudge lumière |

Hors modal, les flèches bougent l'objet sélectionné et la molette dolly la caméra.

## 3. Contraintes d'axe (en modal)

`X` / `Y` / `Z` = axe monde · `U` = libre (plan caméra, défaut).
En `R`, `Z` = roulis autour de l'axe de vue ; `X` = tangage seul ; `Y` = lacet seul.

## 4. Valider / annuler / quitter

| Touche | Effet |
|---|---|
| `Entrée` / `Espace` / clic | Valider (rendu net pleine résolution) |
| `ESC` en modal | Annuler (restaure position/direction) |
| `ESC` hors modal ou `Q` | Quitter |
| `F` | Forcer un rendu net |

## 5. Conventions d'écran (anti-miroir)

- Caméra : `cam.x++` ⇒ objet glisse à **gauche** (pan Blender `Shift+MMB`). Teste : `move_camera.rt` (`C 1,0,-5`).
- Objet : `pos.x++` ⇒ objet à **droite**. Teste : `move_object.rt` (`sp 1,0,0`).
- Lumière : `L.x++` ⇒ ombre à **gauche**. Teste : `move_light.rt` (`L 8,5,-5`).
- Repère : `Y-up` (`Blender Z-up` ↔ `RTv1 (X,Y,Z) = Blender (X,Z,-Y)`).

## 6. Fluide : preview puis net

Pendant le drag : preview 1 rayon par bloc 3×3 (~9x plus vite, ~25 img/s), validée bit-identique à `step=1`.
Au relâchement/validation : rendu net multi-thread. `Sphere` sans rotation (`R` sans effet) ;
`Quadric` ne reconstruit sa base qu'une fois par rotation (pas de glissement UV).

## 7. Exemples

```
./bin/rtv1 scenes/move_object.rt   # G sur la sphère, X, souris, Entrée
./bin/rtv1 scenes/move_camera.rt   # C, souris = pan, molette = dolly
./bin/rtv1 scenes/move_light.rt    # L, souris = ombre en direct
```

Problème d'affichage headless/CI : `SDL_VIDEODRIVER=dummy ./bin/rtv1 scenes/exemple.rt --once`.
