# RT — Session de test manuel scriptée (T078/T079, M5/M6)

> Protocole rejouable par **chaque membre** avant la soutenance (T079 : exécuté
> une fois de bout en bout par un autre membre que son auteur, noté au journal).
> Aucun cas ne plante (ASan propre, T078). Avec `DISPLAY` (fenêtré) sauf mention.

## Pré-requis

```bash
make re && make test   # vert
./rt --help | grep -- --window
```

## Gestes

1. **Expose** : `./rt scenes/default.rt 160 120 --window` → glisser une fenêtre
   au-dessus, Alt+Tab. Attendu : image se redessine instantanément,
   `[expose] blit in <us> us` à chaque fois, aucun re-calcul
   (`grep -n "render(" src/platform/` vide, voir `docs/preuves/expose.md`).
2. **Focus** : changer le focus clavier. Attendu : même reblit, pas de recalcul.
3. **Clavier** : `W` avance, `S` recule, `A`/`D` strafe, `Q`/`E` vertical,
   `+`/`-` FOV, `1`/`2` lumière, `R` reset (voir `README.md`). Attendu : effet
   visible + `[keys] rerender` seulement si change ; touche inconnue → rien.
4. **Glisser souris (T074)** : bouton gauche + glisser → orbite caméra autour de
   la cible. Attendu : vue tourne, scène identique sauf caméra.
5. **Molette (T074)** : molette → FOV ±5°. Attendu : zoom immédiat.
6. **Slider UI (T075)** : panneau microui → changer albedo/couleur → image change.
   Attendu : champ issu de `schema/` (ajouter une directive l'affiche).
7. **Screenshot (T077)** : `P` ou bouton `Save PNG` → `docs/preuves/screenshot_*.png`.
   Attendu : PNG valide (`file`), message si chemin invalide, pas de crash.
8. **Resize** : redimensionner la fenêtre. Attendu : recopie proportionnelle
   (étirement, pas de recalcul) ou re-trace explicite marqué ; min 64×64.
9. **Fichier invalide** : `./rt tests/cases/invalid/garbage.rt --window`.
   Attendu : message `fichier:ligne:colonne`, code ≠0, pas de fenêtre, pas de crash.
10. **Sans écran** : `env -u DISPLAY ./rt scenes/default.rt --window`.
    Attendu : `no DISPLAY...` + code 1 (pas de blocage, T078).
11. **Quit** : fermer (X ou `SDL_QUIT`). Attendu : sortie 0, `valgrind` 0 erreur.

## Traçabilité T079

- Auteur du protocole : session P7 (ce fichier, T078).
- Rejeu complet : à faire par un second membre avant soutenance (noter date +
  nom au journal, DoD T079).

## Exécution 2026-10-09 (T079, agent)

Rejeu automatisable vert (pas de crash, codes attendus) :

- `[controls]` 36 assertions / 4 cas, `[mouse]` 21/2, `[panel]` 17/3,
  `[screenshot]` 11/3, `[edges]` 18/4, `[expose]` 10/1 : tous verts.
- Fichier invalide + `--window` : `garbage.rt:5:1` code 1, pas de fenêtre.
- Sans écran + `--window` : `no DISPLAY...` code 1.
- Headless `--out` : code 0, PNG valide.
- Expose fenêtré (`timeout ... --window`, DISPLAY=:1) : `[expose] blit in
  ~2-3 ms` vs rendu ~425 ms (voir `expose.md`), fermeture par `pollQuit`.

Gestes manuels (drag, molette, slider, `P`, resize souris, quit en croix) :
logique testée + code revu (compteurs, fanions R5, RAII) ; rejeu humain
complet recommandé avant soutenance par un second membre (DoD strict).
