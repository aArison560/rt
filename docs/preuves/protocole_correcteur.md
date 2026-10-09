# RT — Simulation du protocole du correcteur (T084)

> Joue litteralement les 4 controles de la fiche
> ([SPECIFICATIONS §3.2](SPECIFICATIONS.md)). Date : 2026-10-09.
> Chaque case est `Yes` avec sa preuve (commande, log ou image generee).
> Toutes les commandes sont executees depuis la racine du depot.

## (a) Expose sans recalcul — Yes

Le correcteur glisse une fenetre au-dessus de `rt` puis change le focus :
l'image se redessine **sans recalcul** (`SDL_WINDOWEVENT_EXPOSED` → reblit
du framebuffer persistant, regle R4, equivalent de `mlx_expose_hook`).

Preuves :

```bash
grep -n "render(" src/platform/Window.cpp   # vide : aucun appel dans le chemin expose
```

`src/platform/Window.cpp:166` `presentCached()` (reblit + `RenderPresent`,
compteur `exposeCount`) est le seul chemin appele par les branches
`SDL_WINDOWEVENT_EXPOSED` (lignes 204, 239). Le log dedie est visible en
mode debug :

```
[expose] blit in 2952 us
```

Mesure T072 (`docs/preuves/expose.md`) : blit ~2,8 ms contre ~425 ms pour
un rendu complet (~150x). Aucun `Renderer::render()` dans ce chemin.

## (b) 4 objets + transformations + coexistence — Yes

`scenes/fig_vi1.rt` contient les 4 formes de base (plan `sol`, spheres
`rouge`/`verte`, cylindre `magenta`, cone `jaune`), **2 spheres**
coexistantes, et un `transform { rotate axis z angle 45 }` sur le cylindre
(M4 : rayon en espace objet, normale par inverse-transposee).

Preuves :

```bash
./rt scenes/fig_vi1.rt --out /tmp/opencode/fig_vi1_t080.png --quiet  # exit 0
./rt_test "[golden]"   # All tests passed (fig_vi1 = 4 types, 2 spots, brillance)
```

Le test golden affirme `hasPlane && hasSphere && hasCylinder && hasCone`
et la dispatch par vtable (1 `intersect()` par classe, pas de macro :
`grep -R "INTERSECT(" src/geometry` vide, T040). Image :
`docs/preuves/fig_vi1.png` (5 objets visibles, recouvrements).

## (c) Oeil deplace (image2 != image1, meme scene) — Yes

`scenes/fig_vi2.rt` est la copie de `fig_vi1.rt` dont **seule `camera`
differe** (position `(3.5 2.5 4.5)` au lieu de `(0 2 6)`, meme cible,
meme up, meme fov).

Preuves :

```bash
diff <(grep -v '^#' scenes/fig_vi1.rt | grep -v '^$') \
     <(grep -v '^#' scenes/fig_vi2.rt | grep -v '^$')
# 1c1: scene "fig_vi1" -> scene "fig_vi2" (identite du fichier)
# 10c10: position (0 2 6) -> position (3.5 2.5 4.5) (camera uniquement)
./rt_test "[mandatory]"   # All tests passed (109 assertions in 2 test cases)
```

Le test `test_mandatory.cpp` echoue si autre chose que `camera` differe,
parse les 2 fichiers (memes lumières/objets, positions de camera
differentes, cibles identiques) et rend les 2 images : elles different de
plus de 100 pixels (deux angles) sans etre etrangeres. Images :
`docs/preuves/fig_vi1.png` vs `docs/preuves/fig_vi2.png`.

## (d) Lumieres : brillance, ombres, multi-spot — Yes

- **Brillance** (degrade + point blanc) : Blinn-Phong (`specular`,
  `shininess`) sur les spheres de `fig_vi1.rt` ; le test golden compte des
  pixels proches de (1,1,1) (`whites > 0`).
- **Ombres** : shadow ray (`tMin` epsilon anti-acne) sur tous les objets.
- **Multi-spot** : 2 spots melanges, ombres assombries selon le nombre de
  sources bloquees ; `fig_vi3.rt` montre le melange cumule (plein eclairage
  > penombre > ombre cumulee, test golden `plein/ombreG/ombreD > 0`).

Preuves :

```bash
./rt scenes/fig_vi1.rt --out /tmp/x.png --quiet  # exit 0, PNG 320x240 valide
./rt scenes/fig_vi3.rt --out /tmp/y.png --quiet  # exit 0
./rt_test "[golden]"   # All tests passed (134554 assertions in 4 test cases)
```

Images : `docs/preuves/fig_vi1.png` (degrade + point blanc + 2 ombres),
`docs/preuves/fig_vi3.png` (ombres cumulees).
