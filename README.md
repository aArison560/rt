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

## Intégration continue

`.github/workflows/ci.yml` s'exécute à chaque push et PR sur `dev`, sur `ubuntu-24.04` :

| Job | Étapes |
|-----|--------|
| `quality` | `libsdl2-dev libpng-dev libjpeg-dev` + `g++-14`, `make re`, `make test`, puis `valgrind --leak-check=full --error-exitcode=1 ./rt --version` |
| `sanitizers` | `make asan` (ASan + UBSan), `make test`, puis exécution `./rt` sous sanitizers |

> `g++-14` est explicite : le `g++` par défaut d'Ubuntu 24.04 (GCC 13) ne connaît pas
> `-std=c++2c` (support GCC 14+). Le Makefile accepte `CC=g++-14` sans modification.
> Pour reproduire la CI localement : `make re CC=g++-14 && make test CC=g++-14`.
