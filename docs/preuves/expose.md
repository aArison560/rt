# RT — Preuve expose sans recalcul (T072, M6)

> Exigence éliminatoire M6 : l'exposition de la fenêtre **reblit** le
> framebuffer persistant **sans recalculer** l'image. Protocole rejouable en
> 10 secondes, mesure réelle sur la machine de démo.

## Protocole (10 secondes)

1. `./rt scenes/default.rt 320 240 --window` (sans `--headless`, avec `DISPLAY`).
2. Glisser une autre fenêtre **au-dessus** de la fenêtre `rt`, puis changer le
   focus clavier d'une fenêtre à l'autre (Alt+Tab).
3. Constater : l'image **se redessine** à chaque fois, instantanément.
4. Dans le terminal, chaque exposition affiche :
   `[expose] blit in <us> us` (log dédié de `Window::presentCached()`, visible
   sans `--quiet`, mode debug par `RT_DEBUG` ou toujours sur stderr).
5. Preuve par le code : `grep -n "render(" src/platform/` est vide — le chemin
   expose (`presentCached` + `pollQuit` sur `SDL_WINDOWEVENT_EXPOSED`) ne touche
   ni au tampon ni au moteur (voir `src/platform/Window.cpp`, T071).

## Mesures réelles (poste de dév, 2026-10-09)

- Rendu `scenes/default.rt` 320×240 spp 4, 1 thread (headless) :
  `build=1.6ms render=425.4ms total=427.0ms` (`[stats]`, `T064`).
- Reblit expose (fenêtre 160×120, même scène) : `[expose] blit in 2779 us`
  (≈ 2,8 ms, `Window::presentCached`, `chrono::steady_clock`).
- Ratio : **≈ 150× plus rapide** sans recalcul (2,8 ms vs 427 ms).
  Même à résolution égale (160×120 ≈ 4× moins de pixels que 320×240, rendu
  ≈ 100 ms estimés), le reblit reste **≈ 40×** plus rapide.

## Ce qui est affiché

- Le tampon persistant (`Framebuffer` RGBA8, R4) copié vers la texture SDL
  au premier `blit`, puis re-présenté tel quel à chaque expose.
- Resize : recopie proportionnelle (étirement `SDL_RenderCopy`, aucun nouveau
  calcul, marqué dans le code, T071/T078).

## Régénérer

```bash
./rt scenes/default.rt 320 240 2>&1 | grep '\[stats\]'
timeout 5 ./rt scenes/default.rt 160 120 --window 2>&1 | grep '\[expose\]'
grep -n "render(" src/platform/ || echo "aucun appel moteur dans platform/ (T071)"
```

> Aucune image fabriquée : la preuve, c'est le log + le code + la mesure.
