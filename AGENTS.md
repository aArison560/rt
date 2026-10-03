# AGENTS.md — RTv1 → Blender-like Roadmap

> Objectif: app quasi-Blender (raytracing CPU). Ne pas se perdre: 1 étape = 1 branche, 1 primitive/texture/lumière à la fois, tests `valgrind` + rendu ref.

## 0. Socle actuel [FAIT]
- `Scene`, `Sphere`, `Plane`, `Renderer` multi-thread, `SceneParser .rt`, SDL2 2.30.8
- Leaks: `still reachable 271k` = SDL/X11 normal, `definitely lost` = `SDL_DBus_Init` (suppression valgrind)

## 1. Phase 1 — Quadriques (2 sem)
- [ ] `src/core/Quadric.cpp` générique: `intersect(Ray,Hit)` résout `a*t²+b*t+c=0`, normale = `grad(F)`
- [ ] `Cone`, `Cylinder` héritent Quadric (cas limites tronqués `hMin/hMax`)
- [ ] `Hyperboloid` (1 nappe: `x²/a²+y²/b²-z²/c²=1`, 2 nappes: `...=-1`), `Paraboloid` (`z=x²/a²+y²/b²`)
- [ ] Parser: `cone`, `cyl`, `hyp`, `parab` + tests `scenes/quadriques.rt`
- Validation: `make leak_test` + image ref

## 2. Phase 2 — UV & Textures (2 sem)
- [ ] `include/core/Material.hpp`: `albedo`, `type` (diffuse/metal/dielectric)
- [ ] `UV` par primitive: sphere (sphérique), plane (planaire), cylindre/cone (cylindrique)
- [ ] `Wave` procédural: `sinus`, `Gabor`, `Gerner` (param `freq`, `amp`, `phase` animable)
- [ ] `Bump/Disruption`: `normal += bumpScale * grad(noise)` (Perlin `stb_perlin.h`)
- [ ] `Image collage`: `stb_image.h` -> `sampler2D(u,v)` + `repeat/clamp`
- Parser: `mat` + `tex wave|bump|image <path>`

## 3. Phase 3 — Lumière Physique (3 sem)
- [ ] `Reflection` récursive (maxDepth 5): `R = I -2(I·N)N`
- [ ] `Refraction` Snell + Fresnel Schlick: `eta`, `k =1-eta²(1-cos²)`, total internal reflection
- [ ] `Translucide/SSS` simplifié: `Beer-Lambert` + diffusion
- [ ] Ombres douces + `Light` queue (multi-lights)
- Fichiers: `src/core/Material.cpp`, `Renderer::trace(Ray,depth)`

## 4. Phase 4 — CSG Combinaison (2 sem)
- [ ] `CSGObject : Object` avec `op=UNION|INTER|DIFF` + `left/right: unique_ptr<Object>`
- [ ] Interval merging: collecter `t[]` des 2 enfants, trier, tester `inside` (ray marching intervals)
- [ ] Parser: `csg union { sphere ... } { cone ... }`
- Tests: trou, lunette, etc.

## 5. Phase 5 — Slice / Clipping (1 sem)
- [ ] `Slice` = `CSG DIFF` avec `Plane` infini OU `ClipPlane` dans `Object::intersect` (`dot(P - p0, N)>0` => discard)
- [ ] UI: `pl clip 0,1,0 5` par objet
- Bonus: `cap` (fermer coupe avec disque)

## 5.5 Phase Mouvements — Blender-like Statique (0.5 sem) [A FAIRE AVANT INTERACTIF]
> Reste statique (édition `.rt` + relance) mais fige les conventions Blender pour éviter régressions miroir.

- [ ] Repère: `Y up` RTv1 ↔ `Z up` Blender : doc `RTv1 (X,Y,Z) ↔ Blender (X,Z,-Y)`, `worldUp{0,1,0}` `src/core/Renderer.cpp:48`
- [ ] Caméra: `right = worldUp×forward` `Renderer.cpp:52` (fix miroir), `up = forward×right`, `forward=dir.normalized()` `Scene.hpp:9` — `cam.x++` ⇒ objet écran gauche (pan Blender `Shift+MMB`)
- [ ] Lumière: `lightDir = (Lpos - hit).normalized()` `Renderer.cpp:18`, ombre `Ray{hit+ N*1e-4, lightDir}` — `L.x++` ⇒ ombre monde `-X` donc écran gauche
- [ ] Objet: `sp/cyl/cone pos` translation pure `G X/Y/Z` Blender, `dir` normalisée, `SceneParser` doit appliquer `pos += delta`
- [ ] Tests statiques: `scenes/move_camera.rt` (`C 1,0,-5` vs `C 0,0,-5` → sphère gauche), `scenes/move_light.rt` (`L 8,5,-5` → ombre gauche), `scenes/move_object.rt` (`sp 1,0,0` → droite écran)
- [ ] Validation: pas d'interactif SDL (`sdl_events.cpp:1` reste vide), `make && ./bin/rtv1 scenes/move_*.rt` + image ref

## 6. Workflow Agent
- Stack: C++17, SDL2 local, `std::thread`, pas Qt/GTK tant que rendu OK
- Chaque étape: `src/core/*` + `include/core/*` + `scenes/test_etape.rt` + `Renderer` inchangé
- Commandes: `make`, `make leak_test`, `./bin/rtv1 scenes/*.rt` (10s auto-quit)
- Git: 1 feature = 1 commit, pas de `new` nu (utiliser `unique_ptr`), `Scene::clear()` obligatoire
- Valgrind: filtrer SDL via `valgrind --suppressions=./tools/sdl.supp --errors-for-leak-kinds=definite`

## 7. Ordre d'implémentation strict
`Cylinder` -> `Cone` -> `Paraboloid` -> `Hyperboloid` -> `Mouvements Statique (5.5)` -> `UV` -> `Image` -> `Wave/Bump` -> `Reflexion` -> `Refraction` -> `CSG` -> `Slice`

Ne jamais attaquer CSG/lumière avant quadriques+UV+Mouvements statiques stables.
