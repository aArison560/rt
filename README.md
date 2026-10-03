# RT

Raytracer v2 — projet 42. Branche de travail : `dev` (`main` = archive v1, ne pas toucher).

## Build

```bash
make re      # compile ./rt
./rt         # affiche "rt <version>", code retour 0
make test    # tests (Catch2 à partir de T017)
make asan    # build Address+UBSan (-g -O0) pour déboguer
make tsan    # build ThreadSanitizer (rendu multithread)
make fast    # build -O3 -march=native pour les mesures
make compdb  # compile_commands.json via bear (clangd)
make fclean  # nettoyage complet
```

Vérification de l'environnement : `sh scripts/check_env.sh` (✔ présent, ✖ obligatoire manquant,
⚠ optionnel absent ; code retour 1 si un obligatoire manque). Voir `docs/OUTILS.md` §8–9.
