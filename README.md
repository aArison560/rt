# RT

Raytracer v2 — projet 42. Branche de travail : `dev` (`main` = archive v1, ne pas toucher).

## Build

```bash
make re      # compile ./rt
./rt --help  # usage complet, code retour 0
make test    # tests (Catch2 à partir de T017)
make asan    # build Address+UBSan (-g -O0) pour déboguer
make tsan    # build ThreadSanitizer (rendu multithread)
make fast    # build -O3 -march=native pour les mesures
make compdb  # compile_commands.json via bear (clangd)
make format  # reformate le code (.clang-format, idempotent)
make lint    # analyse statique (.clang-tidy) : 0 diagnostic exigé
make quality # batterie complète : build + tests + ASan/UBSan + TSan + valgrind
make fclean  # nettoyage complet
```

## Usage (T026)

```bash
./rt <scene.rt> [width height] [options]
./rt --help     # aide complète, code 0
./rt --version  # affiche "rt <version>", code 0
./rt            # sans argument : usage sur stderr, code 2
```

| Option | Bornes |
|--------|--------|
| `[width height]` | 1..8192 chacun, les deux ou aucun |
| `--out <f.png>`, `--out=<f>` | chemin non vide |
| `--spp <n>` | 1..1024 |
| `--seed <n>` | 0..4294967295 |
| `--threads <n>` | 1..256 |
| `--tile <k/n>` | `n` 1..64, `k` 0..n-1, 0 pixel de recouvrement |
| `--width <n>`, `--height <n>` | alias de `[width height]` (1..8192) |
| `--headless` | sans fenêtre (défaut avec `--out`) |
| `--quiet`, `-q` | sortie réduite |
| `--` | fin des options |

Codes retour : `0` succès/`--help`/`--version`, `1` erreur de scène
(`fichier:ligne:colonne`), `2` erreur d'usage (valeur invalide, option
inconnue, sans argument). Toute valeur invalide affiche l'erreur puis l'usage.

Exemples :

```bash
./rt tests/cases/valid/minimal.rt
./rt tests/cases/valid/minimal.rt 640 480 --out /tmp/a.png
./rt tests/cases/valid/minimal.rt --width 640 --height 480 --spp 16 --seed 42
./rt tests/cases/valid/minimal.rt --tile 1/4 --spp 16 --seed 42 --out /tmp/t1.png
```

La batterie de qualité (`sh scripts/quality.sh`) résume ses 5 étapes en ✔/✖ et ne
rend 0 que si tout est vert (détail : `docs/OUTILS.md` §3.1).

Vérification de l'environnement : `sh scripts/check_env.sh` (✔ présent, ✖ obligatoire manquant,
⚠ optionnel absent ; code retour 1 si un obligatoire manque). Voir `docs/OUTILS.md` §8–9.

## Prérequis système

```bash
sudo apt install libsdl2-dev libpng-dev libjpeg-dev  # voie normale (CI incluse)
```

Poste sans SDL2 et sans `apt`/`sudo` (changement de PC) : repli documenté en
`docs/OUTILS.md` §1.2 — `make setup-sdl` compile SDL2 depuis ses sources dans
`./SDL/` (non versionné), puis `make re SDL2_PREFIX=$PWD/SDL/install`.
GDK/GTK et Qt ne sont pas requis (l'interface retenue est SDL2 + microui).

## Intégration continue

`.github/workflows/ci.yml` s'exécute à chaque push et PR sur `dev`, sur `ubuntu-24.04` :

| Job | Étapes |
|-----|--------|
| `quality` | `libsdl2-dev libpng-dev libjpeg-dev` + `g++-14`, `make re`, `make test`, puis `valgrind --leak-check=full --error-exitcode=1 ./rt --version` |
| `sanitizers` | `make asan` (ASan + UBSan), `make test`, puis exécution `./rt` sous sanitizers |

> `g++-14` est explicite : le `g++` par défaut d'Ubuntu 24.04 (GCC 13) ne connaît pas
> `-std=c++2c` (support GCC 14+). Le Makefile accepte `CC=g++-14` sans modification.
> Pour reproduire la CI localement : `make re CC=g++-14 && make test CC=g++-14`.
