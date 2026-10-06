# RT — Outils nécessaires au projet

> Inventaire des outils par tâche (développement, qualité, mesure, rendu, distribué),
> avec leur **présence réelle sur le poste de développement** (vérifiée le 3 octobre 2026)
> et les commandes d'installation.
>
> Associe chaque outil à **l'item de fiche** qu'il permet de prouver.

---

## Sommaire

1. [État des lieux sur le poste](#1-état-des-lieux-sur-le-poste)
2. [Développement et compilation](#2-développement-et-compilation)
3. [Qualité : mémoire, sanitizers, analyse](#3-qualité--mémoire-sanitizers-analyse)
4. [Mesure et performance](#4-mesure-et-performance)
5. [Images, textures et vidéo](#5-images-textures-et-vidéo)
6. [Distribué et environnement de démo](#6-distribué-et-environnement-de-démo)
7. [Tests, benchmark et intégration continue](#7-tests-benchmark-et-intégration-continue)
8. [Cibles Makefile recommandées](#8-cibles-makefile-recommandées)
9. [Vérification rapide de l'environnement](#9-vérification-rapide-de-lenvironnement)

---

## 1. État des lieux sur le poste

| Présent | Absent (à installer si besoin) |
|---------|--------------------------------|
| `ssh`, `rsync`, `docker`, `montage`/`convert` (ImageMagick), `valgrind`, `clang++`, `python3`, `make`, `gdb`* | `tmux`, `pdsh`, `ansible`, `ffmpeg`, `perf`, `bear`, `hyperfine`, `mpicc` (OpenMPI) |

\* à confirmer : `command -v gdb`

```bash
# Installation des manquants (Debian/Ubuntu)
sudo apt install tmux ffmpeg linux-tools-common bear \
                 libomp-dev openmpi-bin
pip install --user hyperfine          # ou : cargo install hyperfine
```

---

## 2. Développement et compilation

| Outil | Usage | Commande | Sert l'item |
|-------|-------|----------|-------------|
| `g++` / `clang++` | C++23, `-Wall -Wextra -Werror -O2` | `make` | M1 (code C++ moderne) |
| `make` | build, `clean`, `fclean`, `re`, `test` | `make re` | — |
| `clang-format` | formatage automatique, style homogène dans l'équipe | `make format` | *Group organization* |
| `clang-tidy` | analyse statique (`bugprone`, `modernize`, `performance`) | `make lint` | qualité de code |
| `bear` → `compile_commands.json` | alimente **clangd** : autocomplétion, aller-à-la-définition, refs | `make compdb` | vélocité |
| `clangd` | LSP C++ dans l'éditeur | — | — |
| `git` | branches `feature/<dev>/<feat>`, revues | voir [PLAN_TRAVAIL.md §4](PLAN_TRAVAIL.md) | *Group organization* |

```bash
# Les deux fichiers de configuration sont versionnés à la racine (créés en T004) :
#   .clang-format  base LLVM (IndentWidth 4, ColumnLimit 100, tabs d'indentation)
#   .clang-tidy    bugprone-* modernize-* performance-* + analyzer/cert/misc
make format   # écrit en place, idempotent
make lint     # échoue si un diagnostic tombe sur src/ include/ tests/
```

> Installe l'outil s'il manque : `sudo apt install clang-format-19 clang-tidy-19`
> (les noms versionnés `clang-format-19`, `clang-tidy-19`, `…-18` sont détectés
> automatiquement ; sinon `make lint CLANG_TIDY=/chemin`).

---

## 3. Qualité : mémoire, sanitizers, analyse

| Outil | Ce qu'il détecte | Commande | Exigence servie |
|-------|------------------|----------|-----------------|
| **`valgrind --leak-check=full`** | fuites, accès invalides | `valgrind --leak-check=full ./rt scenes/x.rt 100 100` | « no memory leaks » (sujet) |
| **ASan** (`-fsanitize=address`) | fuites + dépassements de tampon, **10× plus rapide que valgrind** | `make asan && ./rt …` | anti-crash (soutenance) |
| **UBSan** (`undefined`) | comportements indéfinis : débordements entiers, NaN, aliasing | inclus dans `make asan` | **critique pour du code maths** |
| **TSan** (`thread`) | **data races** entre threads de rendu | `make tsan && ./rt …` | multithreading (item *Technical effects*) |
| `gdb` | backtrace post-mortem | `gdb --args ./rt scenes/x.rt` | débogage |
| `cppcheck` | analyse complémentaire sans compilation | `cppcheck --enable=all src/` | qualité |
| Revue par un pair | logique, design, cas limites | 1 relecteur minimum par PR | *Group organization* |

> **Ordre conseillé** : `UBSan` d'abord (il trouve les vrais bugs maths), puis `ASan`
> (mémoire), puis `TSan` (races), `valgrind` en validation finale.

### 3.1 Batterie complète : `make quality` (T018)

`sh scripts/quality.sh`, appelé par la cible **`make quality`**, exécute les 5 étapes
**dans l'ordre** ci-dessous, imprime une ligne ✔/✖ par étape puis le résumé ;
code retour `0` **uniquement** si les 5 sont vertes (utilisable tel quel en CI) :

| # | Étape | Commande réelle | Ce qu'elle prouve |
|---|-------|-----------------|-------------------|
| 1 | build release | `make re` | 0 warning `-Wall -Wextra -Werror -O2` |
| 2 | tests unitaires | `make test` | suite Catch2 verte (47 cas / 3853 assertions) |
| 3 | ASan/UBSan | `make asan` puis `make test-asan` | fuites et UB sur `./rt` **et** sur les tests |
| 4 | TSan | `make tsan` puis `make test-tsan` | data races (item *Technical effects*) |
| 5 | valgrind | `make re` puis `valgrind --leak-check=full --error-exitcode=1 ./rt --version` | « no memory leaks » (sujet) |

Deux précisions de mise en œuvre :

- Les étapes 3 et 4 passent par `test-asan` / `test-tsan` (**objets et binaires de test
  dédiés** : `obj-test-asan/`, `obj-test-tsan/`) plutôt que par `make asan test` nu :
  le sous-make de `asan` vide `obj-test/` (`fclean`), et le `test` parent recompilerait
  les tests **sans** flags sanitizer — ils ne seraient alors pas instrumentés.
- L'étape 5 commence par un `make re` : valgrind et un binaire ASan/TSan sont
  incompatibles (le `./rt` laissé par l'étape 4 est instrumenté en TSan).

Le rapport (résumé ✔/✖ + chiffres) est recopié dans le **commit** et dans
[docs/JOURNAL.md](JOURNAL.md) à chaque tâche qui l'exécute.

```bash
make quality                          # 5 étapes, ~2 min au 5 octobre 2026
sh scripts/quality.sh                 # idem, sans passer par make
```

---

## 4. Mesure et performance

| Outil | Usage | Sert l'item |
|-------|-------|-------------|
| **`hyperfine`** | benchs avec **variance** : `hyperfine --runs 10 './rt scenes/x.rt --out /tmp/x.png'` | *Technical effects* « really fast » (preuve chiffrée) |
| `perf` | `perf stat -e cycles,instructions,cache-misses` → goulots d'étranglement | *Technical effects* |
| `valgrind --tool=callgrind` + `kcachegrind` | coût par fonction | optimisation ciblée |
| `time` / `/usr/bin/time -v` | temps + **mémoire pic (RSS)** | documentation de performance |
| `nm`, `objdump`, `size`, `ldd` | symboles, taille du binaire, libs liées | vérification « pas de lib interdite » |
| Compteurs internes | `bvhCount`, `shadowRayCount`, `raysPerSec` (déjà dans `Renderer`) | affichage UI |

```bash
# Preuve chiffrée pour la fiche
hyperfine --runs 10 --export-json bench/spheres.json \
  './rt scenes/simple_spheres.rt 800 600 --out /tmp/x.png'
```

### 4.1 Harnais automatisé : `scripts/bench.sh` → `docs/BENCH.md` (T019)

La preuve chiffrée **ne se saisit jamais à la main** : le harnais mesure, calcule et
écrit. Chaque exécution garantit d'abord l'existence de `docs/BENCH.md` (en-tête +
méthode), puis y ajoute une section de résultats.

```bash
sh scripts/bench.sh tests/cases/valid/minimal.rt --runs 10   # rendre N fois
sh scripts/bench.sh scenes/x.rt --label avant --note "BVH off"   # comparaison
sh scripts/bench.sh --cmd './rt --version' --runs 5          # autre commande
```

| Option | Rôle |
|--------|------|
| `--runs N` | exécutions chronométrées (défaut 5, minimum 1) |
| `--warmup N` | exécutions d'échauffement non comptabilisées (défaut 1) |
| `--label L` | nom de la mesure ; **même label = section remplacée** (avant/après propre) |
| `--note TXT` | contexte écrit dans la mesure (hypothèses, version du binaire) |
| `--args "..."` | arguments supplémentaires passés à `./rt` après la scène |
| `--cmd "..."` | mesure une commande arbitraire au lieu d'un rendu |
| `-h`, `--help` | aide complète (aussi dans l'en-tête de `scripts/bench.sh`) |

- **Sortie** : `docs/BENCH.md` — une section `### <label> — <horodatage>` par mesure,
  avec la commande exacte, le protocole, un tableau Markdown **et** le détail brut en
  JSON (aucun chiffre recopié). Deux labels différents coexistent, le même est remplacé.
- **Statistiques** : moyenne arithmétique, **écart-type échantillon** (n−1, mise à jour
  par la formule de Welford), min, max et `CV = écart-type / moyenne`.
- **Sans hyperfine** : `hyperfine` est employé s'il est installé (son JSON est exporté
  tel quel), sinon `date +%s.%N`, en dernier recours une horloge `python3`.
  *DoD de T019 vérifié sur un poste **sans** `hyperfine`.*
- **Codes retour** : `0` mesure écrite · `2` erreur d'usage (scène absente, `--runs`
  invalide…) · sinon code de la commande mesurée, la sortie de celle-ci étant affichée.
  L'en-tête du document est écrit **avant** toute mesure : il existe donc même si la
  première mesure échoue.
- **Répertoire de travail** : le script se replace toujours à la racine du dépôt ;
  `RT_BENCH_DOC=<chemin>` détourne le fichier de sortie (tests du harnais ailleurs).

---

## 5. Images, textures et vidéo

| Outil | Usage | Sert l'item |
|-------|-------|-------------|
| **libpng / libjpeg** | chargement de textures (bibliothèques **explicitement autorisées** par le sujet) | *Textures* sous-critère 5 |
| **ImageMagick** (`convert`, `montage`) | montage des tuiles distribuées, comparaison d'images, thumbnails | cluster + non-régression |
| `ffmpeg` | vidéo à partir de frames (rotation de caméra) | *In bulk* « video made from your RT » |
| `python3` + `PIL`/`numpy` | scripts : différence d'images, génération de scènes, contrôle de coutures | outillage |
| GIMP / Krita / Photopea | création des textures de démo (damier, brique, bois) | *Textures*, *Disruptions* |
| **Catch2** (déjà amalgamé dans `tests/`) | tests unitaires | qualité |

```bash
# Comparer deux rendus (non-régression visuelle)
compare -metric AE reference.png rendered.png diff.png 2>&1   # nb de pixels différents
# Vérifier l'absence de couture après rendu distribué
convert tiles/*.png +append final.png
```

---

## 6. Distribué et environnement de démo

| Outil | Usage | État |
|-------|-------|------|
| **`ssh` + clés** | lancer une tuile sur un 2ᵉ poste | ✔ — `ssh-keygen -t ed25519`, `ssh-copy-id pc2` |
| **`rsync --checksum`** | distribuer binaire + scènes **identiques** partout | ✔ |
| **ImageMagick `montage`** | assembler les tuiles | ✔ |
| **Docker / Podman** | image unique = binaire + scènes garantis identiques sur tous les nœuds | ✔ `docker` |
| `tmux` | sessions persistantes pour la démo (résister à une coupure SSH) | ✖ |
| `pdsh` / `pssh` | ssh en parallèle sur N hôtes | ✖ |
| `ansible` | configuration de N machines (sur-ingenierie ici) | ✖ |
| **OpenMPI** (`mpirun`) | rendu MPI (niveau exotique) | ✖ `mpicc` |

Détails et script complet : [DISTRIBUTED_RENDERING.md](DISTRIBUTED_RENDERING.md).

---

## 7. Tests, benchmark et intégration continue

| Outil | Usage |
|-------|-------|
| `make test` → `./rt_test` | tests unitaires (Catch2, vendored dans `thirdparty/catch2/`) |
| `make test-asan` → `./rt_test_asan` | mêmes tests sous ASan/UBSan |
| `make test-tsan` → `./rt_test_tsan` | mêmes tests sous TSan (objets dédiés, T018) |
| `make quality` → `scripts/quality.sh` | batterie complète : build + tests + ASan/UBSan + TSan + valgrind, résumé ✔/✖ (T018) |
| `sh scripts/bench.sh <scene>` → `docs/BENCH.md` | harnais de benchmark : moyenne + écart-type sur N rendus, Markdown + JSON (T019, voir §4.1) |
| **GitHub Actions / GitLab CI** | à chaque push : `make && make test && valgrind` → le dépôt est **toujours** vert |
| `./scripts/check_env.sh` | vérifie l'environnement avant une session de travail |
| **pre-commit** (optionnel) | bloque un push si `make` échoue |

```yaml
# .github/workflows/ci.yml — squelette
jobs:
  quality:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: sudo apt-get install -y libsdl2-dev libpng-dev libjpeg-dev
      - run: make re
      - run: make test
      - run: valgrind --leak-check=full --error-exitcode=1 ./rt scenes/default.rt 100 100
```

---

## 8. Cibles Makefile recommandées

À ajouter au `Makefile` de RT (inspirées de Webserv, voir
[MEMORY_STRATEGY.md §5](MEMORY_STRATEGY.md)) :

| Cible | Flags | Quand |
|-------|-------|-------|
| `make` | `-Wall -Wextra -Werror -O2` | quotidien (0 warning exigé) |
| `make test` | sans flags stricts + `tests/` | avant chaque merge |
| `make asan` | `-fsanitize=address,undefined -g -O0` | debugging mémoire/UB |
| `make tsan` | `-fsanitize=thread -g -O0` | **obligatoire** (rendu multithread) |
| `make fast` | `-march=native -O3 -ffast-math` | mesures de performance |
| `make compdb` | `bear` → `compile_commands.json` | éditeur/clangd |
| `make format` | `.clang-format` (base LLVM) → reformate en place | avant chaque commit |
| `make lint` | `.clang-tidy` → analyse statique, 0 diagnostic exigé | avant chaque merge |
| `make quality` | `scripts/quality.sh` → build + tests + ASan/UBSan + TSan + valgrind, résumé ✔/✖ | avant chaque merge, en CI |
| `make bench` | exécute `hyperfine` sur les scènes de démo | avant la soutenance |

> **État (T002/T004, T017/T018)** : `asan`, `tsan`, `fast`, `compdb`, `format`, `lint` sont
> **implémentées** dans le `Makefile` (T002/T004), `test`/`test-asan` en T017 et
> `test-tsan`/`quality` en T018. Chaque cible de build repart de zéro (`$(MAKE) re CXXFLAGS=… LDFLAGS=…`) :
> pas de mélange d'objets compilés avec des flags différents. `compdb` échoue avec un message
> explicite si `bear` est absent (le `compile_commands.json` est nettoyé par `make fclean`).
> `format`/`lint` détectent `clang-format`/`clang-tidy` **et** leurs noms versionnés
> (`-19`, `-18`) et donnent la commande d'installation s'ils manquent ; `make bench` arrivera
> avec T019.

---

## 9. Vérification rapide de l'environnement

```bash
sh scripts/check_env.sh
```

Le script (créé en T002) inspecte : compilateurs (`c++`, `g++`, `clang++`), `make`, `valgrind`,
SDL2 / libpng / libjpeg via `pkg-config`, ImageMagick (`magick`/`convert`, `identify`, `montage`),
`git`, `rsync`, `ssh`, `docker`, et l'outillage optionnel de mesure (`hyperfine`, `perf`, `ffmpeg`,
`bear`, `clang-format`, `clang-tidy`, `tmux`, `cppcheck`).

Trois niveaux de sortie, et un code retour fiable pour la CI :

| Marque | Sens |
|--------|------|
| `✔` | présent |
| `✖` | **obligatoire** manquant → code retour `1` |
| `⚠` | optionnel absent — le projet continue sans |

> L'ancien pseudo-script de cette section est remplacé par le vrai script versionné
> `scripts/check_env.sh` (même logique, sortie groupée par thème et code retour exploitable).

---

## Correspondance outil → item de fiche

| Item de fiche | Outils qui servent à le prouver |
|---------------|--------------------------------|
| Obligatoire M6 (expose) | `printf`/logs, chrono UI (`time`) |
| *Textures* (5 pts) | libpng/libjpeg, GIMP, ImageMagick |
| *Disruptions* (5 pts) | Python (génération de patterns de référence) |
| *Technical effects* — multithread | **TSan**, `nproc`, compteurs de threads |
| *Technical effects* — vraiment rapide | **hyperfine**, `perf`, compteurs rays/s |
| *Technical effects* — clustering (2 pts) | ssh, rsync, Docker, ImageMagick |
| *Technical effects* — screenshot | `ImageBuffer::savePNG` (déjà fait) |
| *Environment* 1 — barre de progression | microui + compteurs de tuiles |
| *Environment* 4 — rendu automatique | scripts shell + `ffmpeg` |
| *In bulk* — vidéo | `ffmpeg` |
| « Pas de fuite mémoire » (sujet) | **valgrind**, **ASan** |
| *Group organization* | git, clang-format, CI, journal |

---

## Voir aussi

- Plan de travail et quality gates : [PLAN_TRAVAIL.md §8](PLAN_TRAVAIL.md)
- Mémoire et cibles Makefile : [MEMORY_STRATEGY.md](MEMORY_STRATEGY.md)
- Rendu distribué : [DISTRIBUTED_RENDERING.md](DISTRIBUTED_RENDERING.md)
- Checklist de soutenance : [CHECKLIST_DEFENSE.md §5](CHECKLIST_DEFENSE.md)
