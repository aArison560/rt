# RT — Checklist de tâches (fichier de reprise)

> **Ce fichier est LE point de reprise du projet.** Toute session OpenCode commence par lui,
> et toute session s'arrête après y avoir écrit sa trace.
>
> - **Branche de travail** : `dev` (projet v2, reparti de zéro).
> - **`main`** = archive de la v1 (ancienne implémentation) : **ne jamais y toucher**.
> - **`docs_tasks`** = branche d'origine de la documentation (fusionnée dans `dev`).
>
> **État au démarrage** : dépôt **sans aucune ligne de code** — seul `docs/` existe.
> Le projet est vaste (partie obligatoire éliminatoire + 18 options + 5 options « More ») :
> on l'avance **une tâche à la fois**, dans l'ordre des phases.

---

## 🔁 Relancer une session (à copier-coller dans OpenCode)

> Travaille sur le projet RT. Ouvre `docs/CHECKLIST_TACHES.md` et lis **d'abord** sa
> section 0 (protocole de session), puis exécute **la prochaine tâche `⬜`** en suivant
> son Prompt et son DoD exactement. À la fin : mets le statut de cette tâche à `✅`
> (avec la date et le commit), ajoute une ligne au journal ([`docs/JOURNAL.md`](docs/JOURNAL.md)), committe selon la section 6,
> puis **arrête-toi**. Si `TACHES_PAR_SESSION` vaut `3`, enchaîne trois tâches de la même
> façon. Si une tâche est impossible, marque-la `⛔` avec la raison et passe à la suivante
> uniquement si ses dépendances sont faites. Ne touche jamais à `main`.

---

## 0. Protocole de session (à lire en entier avant de faire quoi que ce soit)

### 0.1 Ordre des opérations

> Toutes les commandes sont exécutées **depuis la racine du dépôt**.

| # | Action | Commande / endroit |
|---|--------|--------------------|
| 1 | Vérifier la branche et l'état | `git status` et `git branch --show-current` → doit être `dev` (sinon `git checkout dev`) |
| 2 | Lire ce fichier | section 0 intégralement, puis la phase courante |
| 3 | Trouver la prochaine tâche | `grep -n '^#### T[0-9]* ⬜' docs/CHECKLIST_TACHES.md \| head -5` |
| 4 | Ouvrir le bloc de la tâche | `grep -A6 '^#### T011' docs/CHECKLIST_TACHES.md` |
| 5 | Vérifier les dépendances | toute tâche citée en **Dépend** doit être `✅` |
| 6 | Réaliser la tâche **entièrement** | code + tests + doc, en suivant le **Prompt** |
| 7 | Valider le **DoD** | exécuter chaque commande listée, constater le résultat |
| 8 | Mettre à jour ce fichier | dans le bloc de la tâche : `⬜` → `✅`, remplir **Fait le** et **Commit** |
| 9 | Committer | format de la [section 6](#6-conventions-de-commit) |
| 10 | **S'arrêter** | sauf si `TACHES_PAR_SESSION > 1` |

### 0.2 Paramètre de session

```
TACHES_PAR_SESSION = 3
```

Nombre de tâches à réaliser avant de s'arrêter. L'utilisateur peut le modifier :
`1` (par défaut), `3`, ou `phase` (« jusqu'à la fin de la phase courante »).
Chaque tâche achevée **doit** être tracée avant d'en commencer une autre.

### 0.3 Règles inviolables

1. **Ne jamais sauter une tâche.** Si elle est impossible : statut `⛔` + ligne
   `> **Bloqué** : raison.` dans son bloc. La tâche suivante reste `⬜`.
2. **Ne jamais écrire `✅` sans que le DoD passe** — les commandes du DoD sont exécutées,
   pas supposées. Aucune preuve inventée (image, chiffre, capture).
3. **Toujours partir avec le dépôt vert** : `make re && make test` (et `make asan` si la tâche
   touche à la mémoire ou à la géométrie).
4. **Aucune modification sur `main`** (archive v1) ni sur `origin` sans l'accord de l'équipe.
5. **Une tâche trop grande se découpe** en `T011a`, `T011b` **dans son propre bloc** :
   on ne réutilise jamais un ID, on n'insère pas de tâche au milieu d'une phase.
6. **Le hot path ne doit pas dériver** : voir les 7 règles de la [section 2.3](#23-les-7-règles-inviolables).
7. **Un commit par avancement** : pas de travail non commité en fin de session.

### 0.4 Où est l'information

| Besoin | Fichier |
|--------|---------|
| Ce qui est noté, points, flags d'arrêt | `docs/SPECIFICATIONS.md` |
| Implémentation d'une option + preuve | `docs/OPTIONS_GUIDE.md` |
| Organisation, rôles, jalons | `docs/PLAN_TRAVAIL.md` |
| Outils, sanitizers, cibles Makefile | `docs/OUTILS.md` |
| Mémoire, « hot path » sans allocation | `docs/MEMORY_STRATEGY.md` |
| Cluster (2 pts), script SSH | `docs/DISTRIBUTED_RENDERING.md` |
| Transferts d'architecture depuis Blender | `docs/INSPIRATION_BLENDER.md` |
| Déroulé de la soutenance | `docs/CHECKLIST_DEFENSE.md` |
| Sujet officiel v4.1 / fiche d'éval | `docs/subjects/` / `docs/evalsheet/evalsheet.md` |

### 0.5 Comment ajouter une tâche

Insérer un bloc `#### Txxx ⬜ — Titre` **à la fin de la phase concernée**, avec le premier ID
libre de la tranche de cette phase (P0 = T0xx, P1 = T01x, … P9 = T1xx, P12 = T16x).
Ajouter la phase seulement si elle n'existe pas. Ne jamais réattribuer un ID déjà utilisé.

---

## 1. Vue d'ensemble

### 1.1 Les 13 phases

| Phase | Nom | IDs | Condition de sortie |
|-------|-----|-----|---------------------|
| **P0** | Fondations du dépôt | T000–T005 | `make re && make test` vert, CI verte, `author` présent |
| **P1** | Noyau `base` : maths, mémoire, erreurs, RNG | T010–T019 | tests unitaires verts, asan/tsan/valgrind propres |
| **P2** | Schéma & scène (ADN/RNA) : format, parser, validation | T020–T029 | erreurs `fichier:ligne:colonne`, 0 crash sur fichier mal formé |
| **P3** | Rendu minimal (M2) | T030–T037 | première image PNG générée en mode headless |
| **P4** | Géométrie : 4 primitives + transformations (M3, M4) | T040–T048 | sphère/plan/cylindre/cône + translations/rotations |
| **P5** | Lumières & matériaux (M7) + réflexion/transparence | T050–T059 | ombres, spéculaire, multi-spot, ambiance |
| **P6** | Performance : BVH, multithreading, mesure | T060–T067 | speedup mesuré, TSan vert, benchs enregistrés |
| **P7** | Affichage & interaction (M5, M6) | T070–T079 | expose qui **redessine sans recalcul**, preuve chronométrée |
| **P8** | Partie obligatoire gelée (M1–M8, B1–B4) | T080–T089 | 3 scènes de référence + audit de défense vert + tag |
| **P9** | Options — priorité 1 | T100–T119 | items cochés dans `OPTIONS_GUIDE.md`, preuves versionnées |
| **P10** | Options — priorité 2 | T120–T134 | idem |
| **P11** | Options — priorité 3 et exotiques | T140–T154 | idem |
| **P12** | Soutenance | T160–T169 | répétitions passées, gel final, checklist du matin |

### 1.2 Commandes de situation

```bash
grep -c '^#### T[0-9]* ⬜' docs/CHECKLIST_TACHES.md          # tâches restantes
grep -c '^#### T[0-9]* ✅' docs/CHECKLIST_TACHES.md          # tâches faites
grep -n '^#### T[0-9]* ⬜' docs/CHECKLIST_TACHES.md | head   # prochaine tâche
grep -n '^#### T[0-9]* ⛔' docs/CHECKLIST_TACHES.md          # tâches bloquées
```

### 1.3 Ce qui est déjà acquis (depuis la v1, conservé dans `main`)

La v1 n'est **pas jetable comme référence** : elle reste consultable sans jamais être copiée
aveuglément. Récupération d'un fichier précis :

```bash
git show main:src/geometry/Sphere.cpp      # lecture seule, sur la branche dev
git show main:docs/IMPLEMENTATION_GUIDE.md
```

> ⚠ **Règle** : on relit la v1 pour comprendre, on **réécrit** pour construire. Toute reprise
> de code v1 doit être signalée dans le commit (`refactor: reused v1 idea for X`) et repasser
> par les 7 règles de la [section 2.3](#23-les-7-règles-inviolables).

---

## 2. Architecture cible

> Objectif : un **grand projet structuré en calques**, dans l'esprit de Blender
> (schéma unique, invalidation explicite, calcul découplé de l'affichage).
> Transferts détaillés : [INSPIRATION_BLENDER.md](INSPIRATION_BLENDER.md).

### 2.1 Calques

```
   ┌──────────────────────────────────────────────────────────────────┐
   │ app/        composition root : CLI, wiring, boucle de vie        │  on ne connaît
   ├──────────────────────────────────────────────────────────────────┤  que le calque
   │ ui/         microui : panneau, barre de progression, presets      │  juste en dessous
   │ platform/   SDL : fenêtre, texture, événements, expose, RAII      │
   ├──────────────────────────────────────────────────────────────────┤
   │ sched/      ThreadPool, tuiles, jobs, dirty flags                │
   │ render/     sampler, path tracer, framebuffer, film              │  = « render engine »
   ├──────────────────────────────────────────────────────────────────┤
   │ scene/      modèle de données, chargement, invalidation          │
   │ schema/     RNA : déclaration des directives (type, bornes, doc) │  = « schéma »
   ├──────────────────────────────────────────────────────────────────┤
   │ geometry/   primitives + transformations      ┐                  │
   │ shading/    matériaux, textures, patterns     ├ métier du rendu  │
   │ lighting/   lumières, atténuation             ┘                  │
   │ accel/      BVH (build, traversal, invalidation)                 │
   ├──────────────────────────────────────────────────────────────────┤
   │ io/         PNG/JPEG, écriture d'images, hash                    │
   ├──────────────────────────────────────────────────────────────────┤
   │ base/       math (Vec/Mat/Ray), mémoire (arena), erreurs, RNG,   │  = « blenlib »
   │             logging — aucune dépendance vers le haut             │
   └──────────────────────────────────────────────────────────────────┘
```

**Règle d'or** : un calque ne voit que celui du dessous. `render/` ne connaît ni SDL ni microui ;
`platform/` ne connaît ni les primitives ni le parser. C'est ce qui permet le mode **headless**
(et donc le cluster) et les tests unitaires de chaque calque isolément.

### 2.2 Arborescence du dépôt

```
rt/
├── docs/                     # spécification, plan, preuves (existant)
│   ├── ADR/                  # décisions d'architecture (ADR-001, …)
│   └── preuves/              # images et captures régénérables par script
├── include/rt/               # en-têtes, une arborescence par calque
│   ├── base/  schema/  scene/  geometry/  shading/  lighting/
│   ├── accel/  render/  sched/  io/  platform/  ui/  app/
├── src/                      # .cpp, même arborescence que include/rt/
├── tests/                    # Catch2 : unit/ + integration/ + cases/ (scènes invalides)
├── scenes/                   # *.rt — 3 obligatoires + une par option
├── textures/                 # PNG/JPEG utilisés par les scènes (dans le dépôt)
├── scripts/                  # check_env.sh, render_all.sh, bench.sh, cluster_render.sh
├── thirdparty/               # microui, stb, Catch2 (vendored, versionnés)
├── Makefile                  # all / clean / fclean / re / test / asan / tsan / fast / compdb
├── .github/workflows/ci.yml  # build + tests + valgrind à chaque push
├── .clang-format  .clang-tidy
└── README.md  AGENTS.md      # installation, build, utilisation
```

### 2.3 Les 7 règles inviolables

| # | Règle | Pourquoi | Où c'est vérifié |
|---|-------|----------|------------------|
| **R1** | **Un seul schéma décrit les données** (`schema/`) : parser, validation, UI et docs en dérivent | zéro divergence entre ce qu'on lit, ce qu'on affiche et ce qu'on valide | T021 |
| **R2** | **Aucune exception dans le hot path** : codes d'erreur explicites ; `try/catch` unique dans `main` comme filet | un throw dans `trace()` → `abort` → **note 0** | T015, T085 |
| **R3** | **Aucune allocation dynamique pendant le rendu** : arena par thread, framebuffer/tuiles préalloués | déterminisme, pas de contention `malloc`, zéro fuite | T014, T018 |
| **R4** | **Calcul ≠ affichage** : framebuffer persistant, l'expose **reblit** sans recalcul | exigence éliminatoire M6 | T071 |
| **R5** | **Invalidation explicite** : `sceneDirty` / `displayDirty` / `objectVersion` (BVH) | pas de recalcul inutile, redessin immédiat | T028, T076 |
| **R6** | **Pas de SDL/GPU dans le chemin de calcul** : `render()` écrit dans un buffer, point. | tests sans affichage + cluster | T035 |
| **R7** | **Testé avant d'être marqué fait** : unitaire + `asan` + `valgrind`, 0 warning `-Wall -Wextra -Werror` | aucune tâche `✅` sans DoD exécuté | protocole 0.3 |

### 2.4 Conventions

- **Langue** : C++23, `-Wall -Wextra -Werror -O2`, 0 warning toléré.
- **Nommage** : types `PascalCase`, fonctions/membres `camelCase`, constantes `kSomething`,
  fichiers = nom du type principal, espaces de noms `rt::`.
- **Tests** : un fichier `tests/unit/test_<module>.cpp` par module ; les scènes invalides
  vivent dans `tests/cases/` et sont rejouées par `make test`.
- **Documentation** : toute tâche qui change un comportement met à jour le doc concerné
  dans le **même commit**.
- **Validation visuelle** : images dans `docs/preuves/`, toujours **régénérées par un script**,
  jamais fabriquées à la main.

---

## 3. Détail des tâches

> Le **statut** (`⬜` / `✅` / `⛔`) ne vit que dans la ligne `####` de chaque tâche.
> Ne pas ajouter d'autre marqueur d'état ailleurs dans le fichier.

### Phase P0 — Fondations du dépôt

#### T000 ✅ — Valider le point de départ
> **Fait le** : 2026-10-03 · **Commit** : 2997efa
- **Prompt** : « Vérifie l'état du dépôt avant de commencer : `git branch --show-current` = `dev`, `git status` propre, `docs/CHECKLIST_TACHES.md` présent et lisible, `main` inchangé (archive v1), aucun fichier de code résiduel (`src/`, `include/`, `tests/` absents de `dev`). Liste ce qui manque pour démarrer un projet C++23 propre. »
- **Dépend** : — · **Sert** : organisation · **Doc** : [README.md](README.md)
- **DoD** : rapport court écrit ; `git status` propre ; branche `dev`.

#### T001 ✅ — Arborescence vide + Makefile minimal
> **Fait le** : 2026-10-03 · **Commit** : 5af727c
- **Prompt** : « Crée l'arborescence de la [section 2.2](#22-arborescence-du-dépôt) avec un `src/app/main.cpp` qui affiche `rt <version>` et sort 0, et un `Makefile` standard 42 (`NAME=rt`, `CC=c++`, `CXXFLAGS=-Wall -Wextra -Werror -O2 -std=c++2c`, cibles `all clean fclean re test`, `OBJDIR`, `VPATH`). Aucune dépendance externe pour l'instant. »
- **Dépend** : T000 · **Sert** : M1 · **Doc** : [OUTILS.md §8](OUTILS.md)
- **DoD** : `make re` → 0 warning, 0 erreur ; `./rt` → code retour 0 ; `make fclean` nettoie tout.

#### T002 ✅ — Cibles de qualité et script d'environnement
> **Fait le** : 2026-10-03 · **Commit** : f2f5e3c
- **Prompt** : « Ajoute au Makefile les cibles `asan` (`-fsanitize=address,undefined -g -O0`), `tsan` (`-fsanitize=thread`), `fast` (`-O3 -march=native`), `test` (Catch2) et `compdb` (`bear` si présent, sinon message explicite). Écris `scripts/check_env.sh` qui teste compilateurs, SDL2, libpng/libjpeg, valgrind, ImageMagick et affiche ✔/✖. »
- **Dépend** : T001 · **Sert** : qualité · **Doc** : [OUTILS.md §9](OUTILS.md), [MEMORY_STRATEGY.md §5](MEMORY_STRATEGY.md)
- **DoD** : `make asan && ./rt` fonctionne ; `sh scripts/check_env.sh` affiche un rapport complet (les outils absents sont signalés « optionnel »).

#### T003 ✅ — Intégration continue
> **Fait le** : 2026-10-03 · **Commit** : 82f99e2
- **Prompt** : « Écris `.github/workflows/ci.yml` : install des libs (`libsdl2-dev libpng-dev libjpeg-dev`), `make re`, `make test`, puis `valgrind --leak-check=full --error-exitcode=1 ./rt --version`. Ajoute un job `sanitizers` qui exécute `make asan` et les tests. »
- **Dépend** : T002 · **Sert** : qualité · **Doc** : [OUTILS.md §7](OUTILS.md)
- **DoD** : YAML valide ; la CI passe sur la première push ; le workflow est documenté dans `README.md`.

#### T004 ✅ — Formatage, analyse statique et règles de revue
> **Fait le** : 2026-10-04 · **Commit** : 6d3f290
- **Prompt** : « Installe `.clang-format` et `.clang-tidy` (bases `LLVM`, checks `bugprone-* modernize-* performance-*`), ajoute la cible `make format` et `make lint`. Écris la section « Revue de code » de `AGENTS.md` : 1 relecteur minimum par PR, checklist (tests, 0 warning, style, doc). »
- **Dépend** : T001 · **Sert** : *Group organization* · **Doc** : [PLAN_TRAVAIL.md §4](PLAN_TRAVAIL.md)
- **DoD** : `make format` est idempotent ; `make lint` tourne ; les règles sont écrites et lisibles.

#### T005 ✅ — ADR-001 : décisions structurantes + fichier `author`
> **Fait le** : 2026-10-04 · **Commit** : 62eaef4
- **Prompt** : « Rédige `docs/ADR/001-decisions.md` tranchant : (1) C++23 et Makefile, (2) gestion d'erreurs = codes dans le hot path + `try/catch` unique dans `main`, (3) format de scène `.rt` **structuré imbriqué** pour l'item *File ++*, (4) arbitrage norminette, (5) découpage en calques de la [section 2.1](#21-calques), (6) stratégie de branches (`dev` = travail, `main` = archive v1), (7) répartition des 3 développeurs sur les phases P1–P8. Crée le fichier `author` à la racine au format du sujet. »
- **Dépend** : T001 · **Sert** : B2, *Group organization* · **Doc** : [SPECIFICATIONS.md §4.2](SPECIFICATIONS.md), [SPECIFICATIONS.md §8](SPECIFICATIONS.md)
- **DoD** : l'ADR existe et répond aux 7 questions ; `author` est à la racine, complet (3 noms) ; l'arbitrage norminette est **écrit noir sur blanc**.

### Phase P1 — Noyau `base` (maths, mémoire, erreurs, RNG)

#### T010 ✅ — Scalaires, constantes et utilitaires
> **Fait le** : 2026-10-04 · **Commit** : f23169c
- **Prompt** : « Crée `include/rt/base/Scalar.hpp` : alias `rt::Real` (float ou double, tranché dans l'ADR), `kEpsilon`, `kPi`, `kInfinity`, fonctions `clamp`, `lerp`, `almostEqual(a,b,eps)`, `degrees/radians`. Tout est `constexpr`/`noexcept`, header-only. Tests : `tests/unit/test_scalar.cpp`. »
- **Dépend** : T005 · **Sert** : M1 · **Doc** : [MEMORY_STRATEGY.md §4](MEMORY_STRATEGY.md)
- **DoD** : `make test` vert ; 0 warning ; `static_assert` sur quelques cas constexpr.

#### T011 ✅ — Vec2/Vec3/Vec4 sans exception
> **Fait le** : 2026-10-04 · **Commit** : 8bc4243
- **Prompt** : « Implémente `rt::Vec2/Vec3/Vec4` (`include/rt/base/Vec.hpp`) : opérateurs arithmétiques, `dot`, `cross`, `length`, `normalize`, `reflect`, `refract`, `min/max`, `nearZero`. **Aucune exception** : `operator/(s)` avec `|s| <= epsilon` renvoie le vecteur inchangé (documenté) et `normalize()` d'un vecteur nul renvoie `(0,0,0)` — voir `docs/MEMORY_STRATEGY.md §3` pour le bug de la v1 à ne pas reproduire. `static_assert(std::is_trivially_copyable_v<Vec3>)`. Tests : division quasi nulle, vecteur nul, orthogonalité. »
- **Dépend** : T010 · **Sert** : M1 · **Doc** : [MEMORY_STRATEGY.md §3](MEMORY_STRATEGY.md)
- **DoD** : tests verts ; `grep -R "throw" src/base/` vide ; `make asan` vert.

#### T012 ✅ — Mat4 et transformations
> **Fait le** : 2026-10-04 · **Commit** : dff609e
- **Prompt** : « Implémente `rt::Mat4` : produit, transposée, `inverse()` **sûr** (déterminant sous epsilon → renvoie `std::optional`/code d'erreur, **jamais throw**), et `rt::Transform` (translate, rotateX/Y/Z, scale, compose). Fournis `transformPoint`, `transformVector`, `transformNormal` (inverse-transposée). Tests : matrice identité, inverse×matrice ≈ I, composition, normale conservée après rotation non uniforme. »
- **Dépend** : T011 · **Sert** : M4 · **Doc** : [ARCHITECTURE.md §4](ARCHITECTURE.md)
- **DoD** : tests verts (tolérance 1e-5) ; aucun `throw` ; `make asan` vert.

#### T013 ✅ — Ray, Interval, AABB, HitRecord
> **Fait le** : 2026-10-04 · **Commit** : 252ab4d
- **Prompt** : « Crée `rt::Ray` (origine, direction, `at(t)`, profondeur/génération), `rt::Interval` (`tMin/tMax`, `surrounds`, `clamp`), `rt::AABB` (union, hit par axe, `pad`), `rt::HitRecord` (point, normale, `t`, `frontFace`, index de matériau, coordonnées uv). Structures POD compactes avec `static_assert(sizeof(...))`. Tests : hit d'AABB de tous côtés, `frontFace` cohérent selon le sens du rayon. »
- **Dépend** : T011, T012 · **Sert** : M2 · **Doc** : —
- **DoD** : tests verts ; tailles des structures consignées dans `docs/ARCHITECTURE.md`.

#### T014 ✅ — Mémoire : arena et préallocations
> **Fait le** : 2026-10-04 · **Commit** : ab5601c
- **Prompt** : « Implémente `rt::Arena` (bump allocator : `alloc(n, alignment)`, `reset()`, libération en bloc, O(1), **zéro appel `malloc` après l'initialisation**) dans `include/rt/base/Arena.hpp`, plus `rt::FixedVector<T,N>` (capacité fixe, `push` → code d'erreur si plein). Documente les invariants (style *Padding Invariants* de Webserv) dans `docs/MEMORY_STRATEGY.md §4`. Tests : alignement, reset réutilisable, débordement → code d'erreur et non crash. »
- **Dépend** : T013 · **Sert** : M1, qualité · **Doc** : [MEMORY_STRATEGY.md §1](MEMORY_STRATEGY.md)
- **DoD** : tests verts ; `grep -R "new \|malloc" src/rendering src/base` n'affiche rien d'attendu dans le hot path ; ASan propre.

#### T015 ✅ — Codes d'erreur, logging et filet de sécurité
> **Fait le** : 2026-10-04 · **Commit** : 618bc81
- **Prompt** : « Définis `rt::Status` (enum de codes + `message` + `ligne`) et `rt::Result<T>` (petit type déplaçable, sans exception), le logger `rt::log::{info,warn,error}` (flux unique, niveau par variable d'environnement), et l'usage : **toutes** les fonctions du hot path renvoient `Status`/`bool`, `main` possède un unique `try/catch(...)` de filet qui logge et renvoie un code ≠ 0. Écris la règle dans `docs/ADR/001-decisions.md` (rèle R2). Tests : propagation d'erreur à travers 3 niveaux. »
- **Dépend** : T011 · **Sert** : M1, anti-crash · **Doc** : [MEMORY_STRATEGY.md §3](MEMORY_STRATEGY.md), [SPECIFICATIONS.md §4.1](SPECIFICATIONS.md)
- **DoD** : `grep -R "throw" src/ | grep -v "app/main"` vide ; le test de filet passe ; aucun chemin d'erreur muet.

#### T016 ✅ — RNG déterministe par pixel global
> **Fait le** : 2026-10-04 · **Commit** : f9bd1c0
- **Prompt** : « Implémente un générateur rapide (PCG32 ou xoroshiro128+, header-only) avec une fonction `seedFor(globalX, globalY, sample, sceneSeed)` qui ne dépend **que de coordonnées absolues** — indispensable pour que les tuiles d'un rendu distribué s'assemblent sans couture (voir `docs/DISTRIBUTED_RENDERING.md §2.2`). Tests : même entrée → même suite, deux pixels voisins indépendants, et surtout **le même échantillon qu'il soit calculé dans une tuile pleine ou dans une bande**. »
- **Dépend** : T013 · **Sert** : M2, cluster · **Doc** : [DISTRIBUTED_RENDERING.md §2.2](DISTRIBUTED_RENDERING.md)
- **DoD** : test de couture vert (une image rendue en 2 bandes == image pleine, octet par octet) ; déterminisme sur 3 exécutions.

#### T017 ✅ — Structure des tests et couverture du noyau
> **Fait le** : 2026-10-04 · **Commit** : 5b37efa
- **Prompt** : « Organise `tests/` : `unit/`, `integration/`, `cases/` (fichiers `.rt` valides et invalides). Intègre Catch2 dans `thirdparty/`, branche `make test` (build + exécution, code retour non nul si échec). Ajoute les tests des modules `base` déjà écrits, une cible `make test-asan`. »
- **Dépend** : T002, T010–T016 · **Sert** : qualité · **Doc** : [OUTILS.md §7](OUTILS.md)
- **DoD** : `make test` vert et rapide (< 5 s) ; `make test-asan` vert ; la structure est documentée dans `AGENTS.md`.

#### T018 ✅ — Batterie de qualité : asan, tsan, valgrind
> **Fait le** : 2026-10-05 · **Commit** : 34e78f3
- **Prompt** : « Écris `scripts/quality.sh` qui exécute dans l'ordre `make re`, `make test`, `make asan test`, `make tsan test`, `valgrind --leak-check=full --error-exitcode=1 ./rt --version` et résume ✔/✖. Ajoute la cible `make quality`. »
- **Dépend** : T017 · **Sert** : qualité · **Doc** : [OUTILS.md §3](OUTILS.md)
- **DoD** : `make quality` sort 0 sur le dépôt actuel ; le rapport est dans le commit ou le journal.

#### T019 ✅ — Harnais de benchmark
> **Fait le** : 2026-10-06 · **Commit** : 2f3e136
- **Prompt** : « Écris `scripts/bench.sh <scene> [--runs N]` : mesure le temps de rendu N fois, calcule **moyenne et écart-type**, écrit en JSON/Markdown dans `docs/BENCH.md` (fichier créé avec l'en-tête et la méthode, même si les chiffres viendront plus tard). Utilise `hyperfine` s'il est là, sinon `date +%s.%N`. »
- **Dépend** : T017 · **Sert** : item *vraiment rapide* · **Doc** : [OUTILS.md §4](OUTILS.md)
- **DoD** : la commande produit un fichier `docs/BENCH.md` avec la méthode décrite ; fonctionne sans `hyperfine`.

### Phase P2 — Schéma & scène (format, parser, validation)

#### T020 ✅ — Spécification du format `.rt` structuré (item *File ++*)
> **Fait le** : 2026-10-07 · **Commit** : 6731508
- **Prompt** : « Rédige `docs/FORMAT_SCENE.md` : format **texte structuré à blocs imbriqués** `{ }` (obligation *File++* : ce n'est pas « une information par ligne »), avec `scene { limits{} camera{} background{} lights{ light{} } objects{ object{} group{} } }`, commentaires `#`, chaînes entre guillemets, nombres, vecteurs `(x y z)`. Donne la grammaire, 2 exemples complets (dont une scène à groupes), et la table des directives. Ajoute le même exemple **en XML** en annexe pour montrer l'équivalence. »
- **Dépend** : T005 · **Sert** : *Scene files*, **File ++** (2 pts) · **Doc** : [ARCHITECTURE.md §5](ARCHITECTURE.md), [SPECIFICATIONS.md §5.2 A](SPECIFICATIONS.md)
- **DoD** : la grammaire est précise (EBNF) ; un fichier valide est donné ; la structure est hiérarchique, pas linéaire.

#### T021 ✅ — Schéma unique des directives (rèle R1)
> **Fait le** : 2026-10-07 · **Commit** : 08ee505
- **Prompt** : « Crée `src/schema/Directives.cpp` : **une seule table déclarative** décrivant chaque directive (nom, chemin hiérarchique, type, valeur par défaut, min/max, unité, description, flag). Génère-en : le parsing (T023), la validation (T024), l'UI (T075) et la doc (`scripts/gen_doc.sh` → table Markdown dans `docs/FORMAT_SCENE.md`). Pas de définition dupliquée ailleurs. »
- **Dépend** : T020 · **Sert** : *File ++*, *Environment* · **Doc** : [INSPIRATION_BLENDER.md §5](INSPIRATION_BLENDER.md)
- **DoD** : ajouter une directive = **une seule ligne** dans la table, et elle apparaît dans parser + validation + doc ; test qui le vérifie.

#### T022 ✅ — Lexer avec localisation d'erreur
> **Fait le** : 2026-10-07 · **Commit** : abec9c4
- **Prompt** : « Écris `src/scene/Lexer.cpp` : découpe le fichier en tokens `{ } ( ) ident string number comment`, en gardant **ligne et colonne** pour chaque token, gestion des nombres (`1.5`, `-2e3`), des guillemets non fermés, des caractères invalides (UTF-8 compris), d'une profondeur d'imbriquation bornée. Retourne `Status` avec message localisé. Tests : chaque cas d'erreur. »
- **Dépend** : T020, T015 · **Sert** : anti-crash · **Doc** : [CHECKLIST_DEFENSE.md §5.1](CHECKLIST_DEFENSE.md)
- **DoD** : `tests/cases/` (guillemet non fermé, octet binaire, `{` non fermé, nombre absurde) → message `fichier:ligne:colonne`, code retour ≠ 0, **0 crash**.

#### T023 ✅ — Parser → modèle de scène
> **Fait le** : 2026-10-07 · **Commit** : db91385
- **Prompt** : « Écris `src/scene/Parser.cpp` : consomme les tokens, construit `Scene` à partir de la table `schema/` (T021), gère l'imbrication, les valeurs par défaut, les références (`material "verre"`), les tableaux. Chaque erreur referme proprement les blocs ouverts et renvoie `Status` avec ligne. Aucune allocation avant validation des bornes. »
- **Dépend** : T021, T022, T028 · **Sert** : *Scene files* · **Doc** : `docs/FORMAT_SCENE.md`
- **DoD** : les 3 scènes d'exemple parsent ; `./rt scenes/x.rt` sans SDL renvoie 0 ; erreurs localisées vérifiées par tests.

#### T024 ✅ — Validation, bornes et limites déclarées
> **Fait le** : 2026-10-07 · **Commit** : 1a05837
- **Prompt** : « Implémente la passe de validation : bornes du schéma (min/max, enums, couleurs 0-1), directive `limits { max_objects max_lights max_texture_bytes }` → si dépassé, **erreur propre** (`scene too large: 300 objects, limit 256`) et code retour ≠ 0, pas d'allocation surprise. Validation croisée (ex. : `ior > 1` si transparence). »
- **Dépend** : T021, T023 · **Sert** : anti-crash, qualité mémoire · **Doc** : [MEMORY_STRATEGY.md §2](MEMORY_STRATEGY.md)
- **DoD** : test de limite franchie → message clair, 0 crash, 0 gros pic mémoire (vérifié avec `/usr/bin/time -v`).

#### T025 ✅ — Robustesse des fichiers (critère éliminatoire)
> **Fait le** : 2026-10-07 · **Commit** : 4270062
- **Prompt** : « Écris `tests/integration/test_bad_files.cpp` + `tests/cases/` : fichier inexistant, répertoire, vide, illisible, corrompu (fuzz maison : suppression/duplication de tokens), imbriquation folle, `include` cyclique si le format en a, binaire. Chaque cas → message utile, exit code ≠ 0, **jamais de segfault**. »
- **Dépend** : T022, T023, T024 · **Sert** : anti-crash (note 0 sinon) · **Doc** : [SPECIFICATIONS.md §4.5](SPECIFICATIONS.md)
- **DoD** : `make test` rejoue tous les cas sous ASan ; `for f in tests/cases/*; do ./rt $f; echo $?; done` → aucun code 139/134.

#### T026 ✅ — Ligne de commande complète
> **Fait le** : 2026-10-07 · **Commit** : b080198
- **Prompt** : « Implémente `src/app/Options.cpp` : `./rt <scene.rt> [width height] [--out f.png] [--spp n] [--seed n] [--threads n] [--tile k/n] [--headless] [--quiet] [--version] [--help]`, valeurs invalides → erreur + usage, `--help` complet. Sépare la lecture des arguments du reste (testable unitairement). »
- **Dépend** : T023 · **Sert** : *In bulk*, cluster · **Doc** : [DISTRIBUTED_RENDERING.md §2.1](DISTRIBUTED_RENDERING.md)
- **DoD** : tests unitaires sur le parsing d'arguments ; `./rt` sans argument → usage + code ≠ 0 (comportement écrit dans `README.md`).

#### T027 ✅ — Jeu de scènes de test (golden)
> **Fait le** : 2026-10-07 · **Commit** : cb4fdc8
- **Prompt** : « Crée `tests/cases/valid/*.rt` (une par fonctionnalité du parser) et `tests/cases/invalid/*.rt`, plus `scripts/run_cases.sh` qui exécute toutes les scènes en mode headless et échoue si un code retour inattendu apparaît. Branche la cible `make test` dessus. »
- **Dépend** : T023, T026 · **Sert** : qualité · **Doc** : —
- **DoD** : `make test` rejoue le jeu complet en < 10 s ; liste des cas dans `docs/FORMAT_SCENE.md`.

#### T028 ✅ — Modèle de données `Scene` (init/reset/clear, dirty flags)
> **Fait le** : 2026-10-07 · **Commit** : a5592ea
- **Prompt** : « Implémente `rt::Scene` : objets, lumières, matériaux, textures, caméra, fond — `std::vector` avec `reserve()` dès la scène lue, `init/reset/clear()` (réutilisation sans realloc), **dirty flags** `sceneDirty`, `displayDirty` et compteur `objectVersion` (invalidera la BVH). Expose en lecture seule aux calques supérieurs. Tests : reset réutilise la capacité, `objectVersion` incrémente à chaque mutation. »
- **Dépend** : T014, T020 · **Sert** : M4, *Environment 3* · **Doc** : [INSPIRATION_BLENDER.md §5.1](INSPIRATION_BLENDER.md)
- **DoD** : tests verts ; la mémoire de la scène est bornée par `limits` et documentée.

#### T029 ✅ — Documentation du format + scène par défaut
> **Fait le** : 2026-10-07 · **Commit** : efe4068
- **Prompt** : « Génère la table complète des directives (`scripts/gen_doc.sh` depuis le schéma), complète `docs/FORMAT_SCENE.md` avec « écrire une scène en 10 minutes », et crée `scenes/default.rt` (simple, jolie, sert de fallback). »
- **Dépend** : T021, T023, T027 · **Sert** : *Scene files*, démonstration · **Doc** : `docs/FORMAT_SCENE.md`
- **DoD** : `./rt scenes/default.rt` affiche/produit quelque chose ; la doc correspond au schéma réel (test de cohérence).

### Phase P3 — Rendu minimal (M2)

#### T030 ✅ — Framebuffer préalloué et persistant
> **Fait le** : 2026-10-07 · **Commit** : cb18e32
- **Prompt** : « Implémente `rt::Framebuffer` : pixels RGBA8 (affichage) + tampon `float` d'accumulation, **alloués une seule fois** à la résolution (`W*H*4` octets documentés), `clear()`, `addSample()`, `present()` (tonemapping + gamma 2.2). Aucune allocation par frame (rèle R3). »
- **Dépend** : T014, T017 · **Sert** : M2, M6 · **Doc** : [MEMORY_STRATEGY.md §4.1](MEMORY_STRATEGY.md)
- **DoD** : test de taille/résolution ; ASan propre ; `sizeof` consigné.

#### T031 ✅ — Caméra
> **Fait le** : 2026-10-07 · **Commit** : 875a3cf
- **Prompt** : « Implémente `rt::Camera` : position **et direction** (cible), up vector, FOV, ratio ; `rayForPixel(x, y, jitter)` construit l'orthonormé (attention aux cas dégénérés : caméra verticale → code d'erreur, pas de throw). Hérite des données `camera {}` du schéma. Tests : centre de l'image, coins, changement de cible. »
- **Dépend** : T013, T030 · **Sert** : M5 · **Doc** : `docs/FORMAT_SCENE.md`
- **DoD** : tests verts ; déplacement de la caméra change l'image (constaté par un test de pixels).

#### T032 ✅ — Boucle de rendu mono-thread
> **Fait le** : 2026-10-08 · **Commit** : 6899ba7
- **Prompt** : « Écris `src/render/Renderer.cpp::render(const Scene&, Framebuffer&, RenderParams)` : parcourt les pixels, génère le rayon, cherche la intersection, écrit la couleur (miss → fond de scène), profondeur max bornée, aucune allocation dans la boucle (registres/arena). Le rendu est **indépendant de SDL** (rèle R6). »
- **Dépend** : T031, T028 · **Sert** : M2 · **Doc** : [ARCHITECTURE.md §4](ARCHITECTURE.md)
- **DoD** : `./rt scenes/default.rt 64 64 --out /tmp/x.png` → code 0 ; ASan/TSan verts ; test de déterminisme.

#### T033 ✅ — Shading diffus + ambiante minimale
> **Fait le** : 2026-10-08 · **Commit** : fae89eb
- **Prompt** : « Ajoute `src/shading/Material.cpp` (albedo, ambient, diffuse) et le calcul de Lambert avec **une** lumière ponctuelle + une composante ambiante globale, bornée [0,1] avec gamma en sortie. Objectif : une image lisible avec des volumes. »
- **Dépend** : T032 · **Sert** : M7, *Ambiance light* · **Doc** : [OPTIONS_GUIDE.md](OPTIONS_GUIDE.md)
- **DoD** : image non noire partout (test : luminosité minimale > 0 sur toute l'image) ; couleurs bornées (pas de NaN : test `std::isnan` sur le buffer).

#### T034 ✅ — Écriture d'image (PNG)
> **Fait le** : 2026-10-08 · **Commit** : a80223a
- **Prompt** : « Implémente `src/io/ImageWriter.cpp` : PNG via `stb_image_write` (vendored dans `thirdparty/`) ou libpng, fallback PPM si lib absente, nom de fichier validé, échec → `Status`. Branche `--out`. »
- **Dépend** : T030 · **Sert** : *Technical effects* (screenshot), preuves · **Doc** : [OUTILS.md §5](OUTILS.md)
- **DoD** : le PNG produit est valide (`identify`/`file` le confirme) ; échec d'écriture (répertoire inexistant) → message et code ≠ 0.

#### T035 ✅ — Mode headless obligatoire
> **Fait le** : 2026-10-08 · **Commit** : 03507c0
- **Prompt** : « Garantit que `./rt <scene> --out f.png` fonctionne **sans `DISPLAY`** : aucune initialisation SDL dans ce chemin, détection à l'exécution ou option `--headless`. Structure `main` en : parse → load → render → write → exit (composition root dans `src/app/`). »
- **Dépend** : T032, T034, T026 · **Sert** : cluster, tests · **Doc** : [DISTRIBUTED_RENDERING.md §2.1](DISTRIBUTED_RENDERING.md)
- **DoD** : `env -u DISPLAY ./rt scenes/default.rt 64 64 --out /tmp/a.png` → code 0 ; test automatisé dans `make test`.

#### T036 ✅ — Rendu progressif par batches
> **Fait le** : 2026-10-08 · **Commit** : 8bf2d2b
- **Prompt** : « Ajoute le mode progressif : `--spp n` échantillons par pixel accumulés en batches, callback `onProgress(done, total)` (futur affichage), temps restant estimé. Garantit la reproductibilité : mêmes `spp` + même `seed` → mêmes pixels. »
- **Dépend** : T032, T016 · **Sert** : *Environment 1*, performance · **Doc** : —
- **DoD** : deux exécutions identiques → même hash d'image ; `--spp 4` est plus rapide que `--spp 64` et plus bruité (constaté).

#### T037 ✅ — Première preuve générée par script
> **Fait le** : 2026-10-08 · **Commit** : 85fdc85
- **Prompt** : « Écris `scripts/render_all.sh` qui rend une liste de scènes en mode headless et dépose les images dans `docs/preuves/` avec un nom explicite ; ajoute `docs/preuves/README.md` expliquant **que ces images sont régénérables** (jamais une preuve unique, règle du sujet). »
- **Dépend** : T035 · **Sert** : preuves · **Doc** : [CHECKLIST_DEFENSE.md §9.2](CHECKLIST_DEFENSE.md)
- **DoD** : `sh scripts/render_all.sh` régénère tout depuis zéro ; le résultat est versionné.

### Phase P4 — Géométrie (M3, M4)

#### T040 ✅ — Interface des objets
> **Fait le** : 2026-10-08 · **Commit** : 3ff5426
- **Prompt** : « Définis `rt::AObject` : `intersect(ray, tMin, tMax, HitRecord&) -> bool` **spécifique à chaque type** (pas de macro ni de switch générique — exigence de la fiche), `localBounds()`, `objectToWorld`, `transform` (translation/rotation depuis le schéma), `materialIndex`, `id`. Table de dispatch par type (vtable ou `std::variant` — tranché dans l'ADR). »
- **Dépend** : T013, T028 · **Sert** : M3, M4 · **Doc** : [SPECIFICATIONS.md §3.2 b](SPECIFICATIONS.md)
- **DoD** : `grep -R "INTERSECT(" src/geometry` sans macro ; test de dispatch ; `static_assert` sur `HitRecord`.

#### T041 ✅ — Sphère
> **Fait le** : 2026-10-08 · **Commit** : c730773
- **Prompt** : « `src/geometry/Sphere.cpp` : intersection analytique (racine la plus proche dans `[tMin,tMax]`), normale orientée `frontFace`, uv pour la texture, `localBounds()` exact. Gère rayon tangent, rayon partant de l'intérieur, rayon parallèle. Tests : 6 cas (tangent, intérieur, manquant, hors bornes, centre exact, très loin). »
- **Dépend** : T040 · **Sert** : M3 · **Doc** : —
- **DoD** : tests verts, y compris cas dégénérés ; aucun throw ; ASan propre.

#### T042 ✅ — Plan
> **Fait le** : 2026-10-08 · **Commit** : 8a96458
- **Prompt** : « `src/geometry/Plane.cpp` : plan infini défini par un point et une normale (dans l'espace objet), intersection stable quand le rayon est quasi parallèle (epsilon, pas de division par zéro), normale cohérente, uv dérivés des axes tangents. Tests : parallèle, dans le plan, avant/après. »
- **Dépend** : T040 · **Sert** : M3 · **Doc** : —
- **DoD** : tests verts ; aucun `throw` ni division par zéro détectée (UBSan vert).

#### T043 ⬜ — Cylindre
> **Fait le** : — · **Commit** : —
- **Prompt** : « `src/geometry/Cylinder.cpp` : cylindre **infini** autour de l'axe local Y (la limitation sera faite en T133), intersection quadratique + gestion des racines négatives, normale radiale, uv (θ, y), dégénéré = rayon nul → code d'erreur. Tests : tangent, intérieur, axial, parallèle à l'axe. »
- **Dépend** : T040 · **Sert** : M3 · **Doc** : —
- **DoD** : tests verts ; ASan/UBSan verts.

#### T044 ⬜ — Cône
> **Fait le** : — · **Commit** : —
- **Prompt** : « `src/geometry/Cone.cpp` : cône infini (deux nappes) autour de Y, racines par côté, détection de la nappe touchée, sommet, cas dégénéré (apex dans le rayon) → comportement défini sans throw, uv. Tests : nappe haute/basse, sommet, parallèle au générateur, rayon passant près de l'apex. »
- **Dépend** : T040 · **Sert** : M3 · **Doc** : —
- **DoD** : tests verts, y compris apex ; ASan/UBSan verts (c'est le cas limite qui a cassé la v1).

#### T045 ⬜ — Transformations par objet (M4)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Branche `Transform` (T012) : le rayon est transformé en **espace objet** (inverse de la transformation), l'intersection se fait là, la normale revient en monde via l'**inverse-transposée**. Un objet à `(0,0,0)` devient déplaçable en `(42,42,42)` et orientable. Test explicite : mêmes deux objets, l'un transformé, l'autre non → images cohérentes. »
- **Dépend** : T012, T041–T044 · **Sert** : M4 · **Doc** : [SPECIFICATIONS.md §3.2 b](SPECIFICATIONS.md)
- **DoD** : test de transformation (translation + rotation) sur les 4 types ; normales normalisées après transformation non uniforme.

#### T046 ⬜ — Scène multi-objets : tri, doublons, coexistence
> **Fait le** : — · **Commit** : —
- **Prompt** : « Dans `Renderer`, cherche l'intersection la plus proche parmi tous les objets (tri par `t`), gère **plusieurs objets du même type** et la coexistence des 4 types dans une scène. Ajoute le test : 6 objets dont 2 sphères → le plus proche gagne. »
- **Dépend** : T041–T045 · **Sert** : M3 · **Doc** : [SPECIFICATIONS.md §3.2 b](SPECIFICATIONS.md)
- **DoD** : test de tri vert ; scènes avec doublons rendues sans doublon visuel (test de pixels).

#### T047 ⬜ — Batterie de tests géométrie
> **Fait le** : — · **Commit** : —
- **Prompt** : « Complète `tests/unit/test_geometry.cpp` : cas limites de chaque primitive, normalisation, `t` hors bornes, rayons dégénérés (direction nulle), objets superposés, objets fortement transformés. Exécute tout sous `make asan` et `valgrind`. »
- **Dépend** : T046 · **Sert** : qualité, anti-crash · **Doc** : —
- **DoD** : couverture des primitives > lignes critiques testées ; `make quality` vert.

#### T048 ⬜ — Scène géométrique de référence (figure du sujet)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Écris `scenes/fig_vi1_base.rt` : plan + sphère + cylindre + cône dans la même image, cohérents avec la figure VI.1 du sujet (dispositions, proportions). Sert de base aux tâches lumière (P5) et de scène obligatoire (T080). »
- **Dépend** : T046, T029 · **Sert** : M3, M8 · **Doc** : `docs/subjects/fr.subject.pdf` (figure VI.1)
- **DoD** : la scène rend sans erreur ; l'image est comparable à la figure du sujet (points communs vérifiables à l'œil : nombre d'objets, recouvrements).

### Phase P5 — Lumières & matériaux (M7, réflexion, transparence)

#### T050 ⬜ — Modèle de matériau complet
> **Fait le** : — · **Commit** : —
- **Prompt** : « Étends `rt::Material` : `albedo`, `ambient`, `diffuse`, `specular`, `shininess`, `reflectivity` (0-1), `transparency` (0-1), `ior`, `texture` (référence par nom), `pattern`. Tout pilotable **depuis le fichier de scène** (règle du sujet : rien ne se change uniquement en recompilant). Validation des bornes dans le schéma. »
- **Dépend** : T033, T021 · **Sert** : M7, *Ambiance ++* · **Doc** : `docs/FORMAT_SCENE.md`
- **DoD** : chaque champ est modifiable par fichier et observable dans l'image (test par champ).

#### T051 ⬜ — Lumière ponctuelle et atténuation
> **Fait le** : — · **Commit** : —
- **Prompt** : « `src/lighting/PointLight.cpp` : position, couleur, intensité, atténuation (constante/linéaire/quadrique bornée), distance max de portée. Écrit dans `docs/BENCH.md` le coût par lumière pour justifier les choix. »
- **Dépend** : T050 · **Sert** : M7 · **Doc** : —
- **DoD** : test : doubler l'intensité double la contribution (à epsilon) ; aucun NaN.

#### T052 ⬜ — Rayons d'ombre et multi-spot
> **Fait le** : — · **Commit** : —
- **Prompt** : « Ajoute le shadow ray (`tMin` epsilon pour éviter l'acné, test d'occlusion sur tous les objets, profondeur bornée) et l'accumulation de **plusieurs spots** : luminosités mélangées, ombres assombries selon le nombre de sources bloquantes. »
- **Dépend** : T051, T046 · **Sert** : M7 (« Lights ») · **Doc** : [SPECIFICATIONS.md §3.2 d](SPECIFICATIONS.md)
- **DoD** : scène à 2 lumières → 2 zones d'ombre distinctes ; test d'acné (pas de bandes) ; le cas de la figure VI.3 est anticipé.

#### T053 ⬜ — Spéculaire (brillance, « petit point blanc »)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Implémente Phong ou Blinn-Phong (`specular`, `shininess`) avec saturation volontaire : le reflet du spot s'ajoute à la couleur de l'objet pour former un **point blanc** côté lumière. Test : présence de pixels proches de (1,1,1) sur une sphère lisse. »
- **Dépend** : T052 · **Sert** : M7 · **Doc** : [SPECIFICATIONS.md §3.2 d](SPECIFICATIONS.md)
- **DoD** : le test de saturation passe ; dégradé visible (luminosité décroît de la lumière vers l'ombre).

#### T054 ⬜ — Lumière ambiante globale pilotée par le fichier (*Ambiance light / ++*)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Ajoute une composante ambiante globale `ambient { color intensity }` au schéma : **aucun objet n'est jamais totalement noir**. Test automatique : luminosité minimale > 0 sur tous les pixels d'une scène éclairée et d'une scène sans lumière. »
- **Dépend** : T050, T021 · **Sert** : *Ambiance light*, *Ambiance ++* (2 pts) · **Doc** : [SPECIFICATIONS.md §5.2 B](SPECIFICATIONS.md)
- **DoD** : test de luminosité minimale vert ; valeur modifiable par fichier, visible dans l'image.

#### T055 ⬜ — Lumière directionnelle (*Parallel light*)
> **Fait le** : — · **Commit** : —
- **Prompt** : « `src/lighting/DirectionalLight.cpp` : lumière **parallèle** définie par une direction (soleil), indépendante de la position, mêmes ombres. Compare explicitement dans un test avec une lumière ponctuelle (les ombres divergent pour la ponctuelle, restent parallèles pour la directionnelle). »
- **Dépend** : T052 · **Sert** : *Parallel light* · **Doc** : [SPECIFICATIONS.md §5.2 E](SPECIFICATIONS.md)
- **DoD** : le test comparatif passe ; scène `scenes/opt_parallel.rt` versionnée.

#### T056 ⬜ — Réflexion (miroir, % réglable)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Ajoute le rayon réfléchi avec profondeur bornée (paramètre `max_depth` dans la scène), `reflectivity` en **pourcentage continu** (0 = mat, 1 = miroir pur), pondération correcte avec la composante diffuse. Tests : `reflectivity=0` identique au rendu sans miroir, `reflectivity=1` = reflet net. »
- **Dépend** : T053 · **Sert** : *Reflection & transparency* sous-critères 1-2 · **Doc** : [SPECIFICATIONS.md §5.2 F](SPECIFICATIONS.md)
- **DoD** : les 2 tests de bornes passent ; pas de boucle infinie (profondeur bornée, test le prouve).

#### T057 ⬜ — Transparence et réfraction (Snell/Descartes)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Implémente la réfraction avec l'**indice de réfraction** (`ior`) via la loi de Descartes (code lisible et commenté — le correcteur cherchera la formule), `transparency` en pourcentage, gestion de la réflexion totale interne. Tests : `ior=1` → pas de déviation ; réfraction vers l'extérieur = courbure cohérente. »
- **Dépend** : T056 · **Sert** : *Reflection & transparency* sous-critères 3-5 · **Doc** : [SPECIFICATIONS.md §5.2 F](SPECIFICATIONS.md)
- **DoD** : tests d'`ior` verts ; la formule est commentée dans le code ; scène `scenes/opt_glass.rt`.

#### T058 ⬜ — Ombres affinées par transparence + *Direct light*
> **Fait le** : — · **Commit** : —
- **Prompt** : « (a) *Shadows and transparency* : l'ombre d'un objet translucide est **moins sombre** que celle d'un objet opaque (atténuation proportionnelle à la transparence et au `ior`). (b) *Direct light* : spotlight orienté vers la caméra/observateur qui **aveugle** (éclairage face à l'utilisateur, saturation du pixel). Scènes de preuve pour chacun. »
- **Dépend** : T057, T052 · **Sert** : *Shadows and transparency*, *Direct light* · **Doc** : [SPECIFICATIONS.md §5.2 E](SPECIFICATIONS.md), [SPECIFICATIONS.md §5.2 F](SPECIFICATIONS.md)
- **DoD** : 2 scènes de preuve + test de densité d'ombre (ombre translucide > ombre opaque en luminosité).

#### T059 ⬜ — Scènes lumière de référence + non-régression
> **Fait le** : — · **Commit** : —
- **Prompt** : « Écris `scenes/fig_vi1.rt` (4 objets, 2 spots, ombres, brillance) et `scenes/fig_vi3.rt` (mélange d'ombres), ajoute un test de non-régression : hash des images rendues enregistrés dans `tests/golden/`, comparaison à chaque `make test` (tolérance sur quelques pixels). »
- **Dépend** : T054, T055, T058 · **Sert** : M7, M8 · **Doc** : `docs/subjects/fr.subject.pdf` (figures VI.1, VI.3)
- **DoD** : les 3 scènes rendent ; le test golden détecte une régression volontaire (vérifié une fois, puis remis).

### Phase P6 — Performance (BVH, multithreading, mesure)

#### T060 ⬜ — Construction de la BVH
> **Fait le** : — · **Commit** : —
- **Prompt** : « Implémente `src/accel/Bvh.cpp` : arbre binaire sur les AABB des objets, partition par médiane (ou binned SAH si le temps le permet), nœuds **compacts et POD** (SoA ou struct de 32 octets, `static_assert`), profondeur bornée. Allocation dans un buffer préalloué — **zéro `new` par nœud** (rèle R3). »
- **Dépend** : T046, T014 · **Sert** : *vraiment rapide*, M8 · **Doc** : [ARCHITECTURE.md §8](ARCHITECTURE.md), [MEMORY_STRATEGY.md §4.2](MEMORY_STRATEGY.md)
- **DoD** : test : construire une BVH sur 1000 objets synthétiques < 50 ms ; nœuds bornés (`nbNodes <= 2N-1`).

#### T061 ⬜ — Traversal BVH
> **Fait le** : — · **Commit** : —
- **Prompt** : « `Bvh::traverse(ray, tMin, tMax, HitRecord&)` avec **pile fixe** (tableau local, pas de récursion profonde, pas d'allocation), test AABB optimisé (méthode de Williams), résultats identiques à la recherche linéaire. Test : comparer BVH et brute-force sur 200 scènes aléatoires → mêmes `t` (tolérance). »
- **Dépend** : T060 · **Sert** : performance · **Doc** : —
- **DoD** : test d'équivalence vert ; ASan/TSan verts ; gain mesuré (avant/après dans `docs/BENCH.md`).

#### T062 ⬜ — Invalidation de la BVH (`objectVersion`)
> **Fait le** : — · **Commit** : —
- **Prompt** : « La BVH n'est reconstruite que si `scene.objectVersion` a changé depuis la dernière construction (cache avec numéro de version). Test : 1000 appels de rendu sans modification → 1 reconstruction ; une modification → exactement 1 de plus. C'est l'équivalent du *depsgraph* de Blender (invalidation ciblée). »
- **Dépend** : T060, T028 · **Sert** : *Environment 3* (live), performance · **Doc** : [INSPIRATION_BLENDER.md §2](INSPIRATION_BLENDER.md)
- **DoD** : le test de compteur vert ; compteurs exposés (`bvhBuilds`) et affichés en debug.

#### T063 ⬜ — ThreadPool et rendu par tuiles
> **Fait le** : — · **Commit** : —
- **Prompt** : « `src/sched/ThreadPool.cpp` : pool créé **une fois** (`std::jthread`), file de tuiles de 32×32 ou bandes, chaque travailleur rend `renderRegion(x0,y0,w,h)` ; arrêt propre, exceptions interceptées **par tâche** (un crash de tâche = code d'erreur, pas d'`std::terminate`). Boucle d'attente économe (condition variable, pas de spin). »
- **Dépend** : T032, T016 · **Sert** : *multi-thread* (item K2), M8 · **Doc** : [ARCHITECTURE.md §8](ARCHITECTURE.md)
- **DoD** : `--threads 1/2/4/8` produit des images **identiques** (déterminisme par seed absolue) ; TSan vert.

#### T064 ⬜ — Mesure du speedup et affichage des métriques
> **Fait le** : — · **Commit** : —
- **Prompt** : « Ajoute les compteurs de rendu (`rays/s`, objets/testés, temps par phase, nombre de threads) et affiche-les en fin de rendu + dans l'UI (T075). Écris `docs/BENCH.md` avec le tableau 1/2/4/8 threads et le calcul du speedup (efficacité < 1 attendue). »
- **Dépend** : T063, T019 · **Sert** : *multi-thread*, *vraiment rapide* · **Doc** : [OUTILS.md §4](OUTILS.md)
- **DoD** : chiffres réels obtenus par `scripts/bench.sh` (moyenne + écart-type), pas estimés ; speedup croissant.

#### T065 ⬜ — Profiling et optimisation local
> **Fait le** : — · **Commit** : —
- **Prompt** : « Mesure (`perf stat`/`callgrind` si disponibles, sinon instrumentation interne), identifie le goulot d'étranglement réel, optimise **un seul point** avec un avant/après chiffré dans `docs/BENCH.md`. Candidats : localité cache des tuiles, structure de `HitRecord`, évitement de recalculs, `std::pow` dans le hot path. Aucune optimisation non mesurée. »
- **Dépend** : T064 · **Sert** : *vraiment rapide* · **Doc** : [OUTILS.md §4](OUTILS.md)
- **DoD** : un gain mesuré est documenté (moyenne + variance) ; le test golden reste vert.

#### T066 ⬜ — Propreté thread : TSan + reproductibilité
> **Fait le** : — · **Commit** : —
- **Prompt** : « Exécute toute la suite sous `make tsan`, corrige toute data race (framebuffer écrit par tuiles disjoints, compteurs atomiques, cache BVH verrouillé). Ajoute un test de **reproductibilité** : même scène + même seed + 1 thread == 4 threads, octet par octet. »
- **Dépend** : T063 · **Sert** : qualité, *multi-thread* · **Doc** : [OUTILS.md §3](OUTILS.md)
- **DoD** : `make quality` complet vert ; le test de reproductibilité passe 10 fois de suite.

#### T067 ⬜ — Rapport de performance complet
> **Fait le** : — · **Commit** : —
- **Prompt** : « Finalise `docs/BENCH.md` : matériel, versions, par scène (démo + obligatoires), temps, rays/s, mémoire pic (`/usr/bin/time -v`), variances sur ≥ 5 runs, comparaison 1 thread vs N, et la phrase « le rendu est vraiment rapide » **sous-titrée de chiffres**. »
- **Dépend** : T065 · **Sert** : item K3 *vraiment rapide* · **Doc** : [OUTILS.md §4](OUTILS.md)
- **DoD** : le document est complet et reproductible (commandes fournies) ; aucun chiffre non mesuré.

### Phase P7 — Affichage & interaction (M5, M6)

#### T070 ⬜ — Couche plateforme SDL (RAII)
> **Fait le** : — · **Commit** : —
- **Prompt** : « `src/platform/Window.cpp` : création SDL2 (vidéo), texture de présentation, boucle d'événements, `RAII` (destructeur = `SDL_Destroy*`, pas de fuite de handle), redimensionnement géré. **Aucun appel** vers `render/` ni `scene/` sauf par l'interface de la [section 2.1](#21-calques). Basculer en mode headless ne doit pas toucher à ce fichier. »
- **Dépend** : T035, T034 · **Sert** : M6, interface · **Doc** : [ARCHITECTURE.md §6](ARCHITECTURE.md)
- **DoD** : fenêtre ouverte, image affichée, fermeture propre, `valgrind` sans fuite SDL.

#### T071 ⬜ — Expose sans recalcul (rèle R4 — exigence éliminatoire)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Gère `SDL_WINDOWEVENT_EXPOSED` par un chemin **dédié** qui **reblit le framebuffer persistant** vers la texture sans appeler `Renderer::render()` : c'est exactement l'équivalent de `mlx_expose_hook` attendu par la fiche. Le framebuffer rendu reste en mémoire (règle R4). Vérifie aussi le resize (recopie proportionnelle ou rerender explicite marqué comme tel). »
- **Dépend** : T070, T030 · **Sert** : **M6 (éliminatoire)** · **Doc** : [SPECIFICATIONS.md §3.2 a](SPECIFICATIONS.md)
- **DoD** : `grep -n "render(" src/platform/` ne montre **aucun** appel dans le chemin expose ; preuve écrite en T072.

#### T072 ⬜ — Preuve chronométrée de l'expose (le correcteur va faire ce test)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Ajoute un log dédié (`printf("[expose] blit in %ld us\n", ...)`) visible en mode debug, et écris `docs/preuves/expose.md` : le protocole exact (déplacer une fenêtre au-dessus, changer le focus), ce qui est affiché, et la mesure (blit en µs vs rendu en ms). »
- **Dépend** : T071 · **Sert** : **M6** · **Doc** : [CHECKLIST_DEFENSE.md](CHECKLIST_DEFENSE.md)
- **DoD** : la mesure est réelle (chronométrée sur la machine de démo) ; le chemin expose est démontrable en 10 secondes.

#### T073 ⬜ — Contrôles clavier (M5)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Branches le clavier aux paramètres attendus par le sujet (positions/lumières/paramètres) : touches documentées dans `README.md` et affichées dans l'UI. Chaque action déclenche le bon dirty flag (R5) — jamais de rerender complet gratuit. »
- **Dépend** : T070, T028 · **Sert** : M5 · **Doc** : `README.md`
- **DoD** : chaque touche a un effet visible ; documentée ; pas de rendu intempestif (log de counters).

#### T074 ⬜ — Souris et molette (M5)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Glisser la souris = modifier l'**angle** de la caméra (orbite autour de la cible), molette = **champ de vision** (FOV), relâchement propre, curseur recapturé. Le champ doit être modifiable **en direct** (le sujet teste l'interaction). »
- **Dépend** : T073, T031 · **Sert** : M5 · **Doc** : [SPECIFICATIONS.md §3.2 c](SPECIFICATIONS.md)
- **DoD** : déplacement de la caméra visible immédiatement ; scène inchangée sauf caméra (rappel du test image1/image2).

#### T075 ⬜ — Interface microui : panneau de configuration
> **Fait le** : — · **Commit** : —
- **Prompt** : « Intègre `microui` (vendored) dans `src/ui/` : panneau affichant objets, lumières, matériaux, caméra avec **champs éditables, sliders, couleurs**, boutons de chargement de fichier et de lancement de rendu. Les valeurs viennent de la table `schema/` (rèle R1) — pas de doublon. »
- **Dépend** : T070, T021 · **Sert** : *Environment 1/2* · **Doc** : [INSPIRATION_BLENDER.md §5](INSPIRATION_BLENDER.md)
- **DoD** : on modifie une couleur depuis l'UI, l'image change ; la table du schéma alimente l'UI (test : ajouter une directive l'affiche).

#### T076 ⬜ — Dirty flags et rendu interactif
> **Fait le** : — · **Commit** : —
- **Prompt** : « Implémente la machine d'états : `sceneDirty` → invalide BVH + rerender en **basse qualité** (1 spp) ; ensuite affinage progressif (retour à `--spp` cible) ; `displayDirty` → blit seul. L'interaction ne doit **jamais** bloquer l'UI : découple calcul et affichage avec le ThreadPool. »
- **Dépend** : T063, T062, T071 · **Sert** : *Environment 3*, M6 · **Doc** : [INSPIRATION_BLENDER.md §5.1](INSPIRATION_BLENDER.md)
- **DoD** : manipuler l'UI reste fluide ; le compteur prouve qu'aucun rerender complet n'est lancé à chaque frame d'affichage.

#### T077 ⬜ — Capture d'écran depuis le programme
> **Fait le** : — · **Commit** : —
- **Prompt** : « Touche dédiée + entrée UI « Save PNG » : écrit le framebuffer courant avec horodatage dans `docs/preuves/` (répertoire configurable), feedback visuel, gestion d'erreur. C'est l'item *Technical effects 4* (« sauvegarder/screenshot l'image rendue **dans le RT** »). »
- **Dépend** : T070, T034 · **Sert** : item K4 *screenshot* · **Doc** : [OPTIONS_GUIDE.md](OPTIONS_GUIDE.md)
- **DoD** : la touche produit un PNG valide ; message d'erreur si le chemin est invalide (pas de crash).

#### T078 ⬜ — Redimensionnement et cas limites d'interface
> **Fait le** : — · **Commit** : —
- **Prompt** : « Gère le resize (reproportionnement correct), taille minimale 64×64, ouverture/multiple ouverture, `SDL_QUIT`, perte de focus, absence de `DISPLAY` en mode fenêtré (message utile + code ≠ 0). Test manuel scripté dans `docs/preuves/interactions.md`. »
- **Dépend** : T070 · **Sert** : anti-crash, M6 · **Doc** : [CHECKLIST_DEFENSE.md §5.3](CHECKLIST_DEFENSE.md)
- **DoD** : aucun des cas ne plante (testé sous ASan) ; le pas-à-pas manuel est écrit.

#### T079 ⬜ — Session de test manuel scriptée
> **Fait le** : — · **Commit** : —
- **Prompt** : « Écris `docs/preuves/interactions.md` : liste numérotée des gestes à faire (expose, focus, glisser, molette, slider, screenshot, resize, fichier invalide) avec le résultat attendu, pour que **chaque membre** rejoue le même protocole avant la soutenance. »
- **Dépend** : T072, T074, T075, T077, T078 · **Sert** : M5, M6, qualité · **Doc** : [CHECKLIST_DEFENSE.md §5](CHECKLIST_DEFENSE.md)
- **DoD** : le protocole est exécuté une fois de bout en bout par un autre membre que son auteur (noté dans le journal).

### Phase P8 — Partie obligatoire finalisée (M1–M8, B1–B4)

#### T080 ⬜ — Scène obligatoire 1 (figure VI.1)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Finalise `scenes/fig_vi1.rt` : les **4 formes** dans la même image, **2 spots**, ombres et brillance, à comparer directement à la figure VI.1 du sujet (réglages fins de positions/lumières). Documente les écarts assumés. »
- **Dépend** : T059 · **Sert** : **M8** · **Doc** : `docs/subjects/fr.subject.pdf`
- **DoD** : scène versionnée ; image régénérée par `scripts/render_all.sh` ; écarts évalués et écrits.

#### T081 ⬜ — Scène obligatoire 2 : « seul l'œil déplacé » (le test décisif)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Crée `scenes/fig_vi2.rt` comme **copie quasi identique** de `fig_vi1.rt` : la **seule différence autorisée est la directive `camera`**. Écris un test automatique qui compare les 2 fichiers ligne à ligne et **échoue si autre chose que `camera` diffère**. »
- **Dépend** : T080, T031 · **Sert** : **M5, M8** (« Did you know? ») · **Doc** : [SPECIFICATIONS.md §3.2 c](SPECIFICATIONS.md)
- **DoD** : le test de comparaison existe et passe ; les 2 images montrent la même scène sous deux angles.

#### T082 ⬜ — Scène obligatoire 3 : mélange d'ombres (figure VI.3)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Écris `scenes/fig_vi3.rt` : **mélange d'ombres** multi-spots (ombres superposées, assombrissement selon le nombre de sources bloquantes), conforme à la figure VI.3. »
- **Dépend** : T052, T080 · **Sert** : **M7, M8** · **Doc** : `docs/subjects/fr.subject.pdf`
- **DoD** : l'image montre bien des ombres **cumulées** (plusieurs sources) ; scène versionnée.

#### T083 ⬜ — Les 3 scènes en une commande
> **Fait le** : — · **Commit** : —
- **Prompt** : « `scripts/render_all.sh` doit rendre **les 3 scènes obligatoires + toutes les scènes d'options** en une commande, headless, en un temps raisonnable, et tout déposer dans `docs/preuves/`. Ajoute un résumé (durée, fichier, statut). »
- **Dépend** : T080–T082, T037 · **Sert** : M8, démonstration · **Doc** : [CHECKLIST_DEFENSE.md §9.1](CHECKLIST_DEFENSE.md)
- **DoD** : une commande régénère l'ensemble ; le temps total est acceptable pour une démo (< 2 min si possible).

#### T084 ⬜ — Simulation du protocole du correcteur
> **Fait le** : — · **Commit** : —
- **Prompt** : « Joue littéralement les 4 contrôles de la fiche ([SPECIFICATIONS §3.2](SPECIFICATIONS.md)) : (a) fenêtre au-dessus + focus → expose loggé et **sans recalcul**, (b) les 4 objets + transformations + coexistence, (c) œil déplacé (image2 ≠ image1 mais même scène), (d) lumières (brillance, ombres, multi-spot). Note le résultat de chaque contrôle dans `docs/preuves/protocole_correcteur.md`. »
- **Dépend** : T072, T074, T081, T082 · **Sert** : **M3–M8** · **Doc** : [CHECKLIST_DEFENSE.md](CHECKLIST_DEFENSE.md)
- **DoD** : les 4 cases sont `Yes` avec une preuve associée (commande, log ou image générée).

#### T085 ⬜ — Robustesse globale : fuzz + sanitizers
> **Fait le** : — · **Commit** : —
- **Prompt** : « Écris `scripts/fuzz.sh` : génère ~200 fichiers `.rt` mutés aléatoirement, les exécute en headless sous ASan/UBSan, compte les codes retour inattendus (139 segfault, 134 abort = échec). Corrige tout ce qui ressort. »
- **Dépend** : T025, T047 · **Sert** : anti-crash (note 0) · **Doc** : [SPECIFICATIONS.md §4.1](SPECIFICATIONS.md)
- **DoD** : `sh scripts/fuzz.sh` → 0 crash ; `valgrind --leak-check=full` sur toutes les scènes → 0 fuite.

#### T086 ⬜ — Livrables obligatoires : `author`, norme, README, AGENTS
> **Fait le** : — · **Commit** : —
- **Prompt** : « Vérifie et finalise B1–B4 : dépôt non vide et buildable, **fichier `author`** au format du sujet avec les 3 noms, arbitrage de la **norminette** appliqué (écrit dans l'ADR + exécuté si décidé), README (installation, build, utilisation, options, scènes) et `AGENTS.md` (commandes, état des modules). »
- **Dépend** : T005, T084 · **Sert** : **B1–B4 (sinon 0)** · **Doc** : [SPECIFICATIONS.md §4.2](SPECIFICATIONS.md), [CHECKLIST_DEFENSE.md §9.1](CHECKLIST_DEFENSE.md)
- **DoD** : `git clone` dans un dossier vide + `make re && make test` fonctionne ; les 4 points B sont cochés noir sur blanc.

#### T087 ⬜ — Audit interne avec la checklist de défense
> **Fait le** : — · **Commit** : —
- **Prompt** : « Exécute `docs/CHECKLIST_DEFENSE.md` intégralement (§5 technique, §8 pièges, §9 livrables) comme si tu étais le correcteur, remplis chaque case avec la preuve réelle, et liste les manques en **`⬜`** dans ce fichier comme nouvelles tâches si nécessaire. »
- **Dépend** : T085, T086 · **Sert** : soutenance · **Doc** : [CHECKLIST_DEFENSE.md](CHECKLIST_DEFENSE.md)
- **DoD** : rapport d'audit daté ; les manques sont devenus des tâches (ou sont explicitement reportés).

#### T088 ⬜ — Gel de la partie obligatoire
> **Fait le** : — · **Commit** : —
- **Prompt** : « Tagge `v1-mandatory` sur un dépôt vert (build, tests, valgrind, 3 scènes) et note dans `docs/PLAN_TRAVAIL.md` : à partir de là, **aucune régression de l'obligatoire n'est acceptée** ; toute modification touche d'abord les tests de non-régression. »
- **Dépend** : T087 · **Sert** : organisation · **Doc** : [PLAN_TRAVAIL.md §6](PLAN_TRAVAIL.md)
- **DoD** : le tag existe ; `git status` propre ; la règle est écrite.

#### T089 ⬜ — Point d'organisation d'équipe (jalon J2)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Mets à jour `docs/PLAN_TRAVAIL.md` : avancement réel par personne, revues réalisées, décisions prises, planning restant pour P9–P12. Prépare l'argumentaire écrit de l'item *Group organization* (réunions, répartition, revues, gestion des conflits). »
- **Dépend** : T088 · **Sert** : *Group organization* (Oui/Non) · **Doc** : [PLAN_TRAVAIL.md](PLAN_TRAVAIL.md)
- **DoD** : le plan reflète la réalité ; l'argumentaire est écrit et répétable en 2 minutes.

### Phase P9 — Options priorité 1 (les mieux notées par heure passée)

#### T100 ⬜ — Item *File ++* : prouver la hiérarchie
> **Fait le** : — · **Commit** : —
- **Prompt** : « Écris un test automatique qui échoue si le format retombe à « une information par ligne » : parse un fichier **imbriqué 3 niveaux** (scene → objects → group → object), un fichier avec des sous-blocs `material {}` séparés, et affirme que la structure est lue. Ajoute à `docs/FORMAT_SCENE.md` une section « pourquoi ce n'est pas du ligne-par-ligne ». »
- **Dépend** : T020, T023 · **Sert** : **File ++** · **Doc** : [SPECIFICATIONS.md §5.2 A](SPECIFICATIONS.md)
- **DoD** : le test prouve l'imbrication ; un exemple XML équivalent est en annexe.

#### T101 ⬜ — Éléments composés réutilisables (*Composed elements*)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Bloc `group { ... }` dans le schéma : un ensemble d'objets simples (cube = 6 plans limités, verre = cône + cylindre + sphère) **défini une fois et instancié plusieurs fois** à des positions/orientations différentes. L'instanciation partage la définition (pas de duplication en mémoire). Tests : 2 instances à 2 endroits distincts. »
- **Dépend** : T045, T100 · **Sert** : *Composed elements* · **Doc** : [SPECIFICATIONS.md §5.2 I](SPECIFICATIONS.md)
- **DoD** : scène `scenes/opt_group.rt` avec la même définition utilisée ≥ 2 fois ; test de partage mémoire.

#### T102 ⬜ — Chargement de textures PNG/JPEG
> **Fait le** : — · **Commit** : —
- **Prompt** : « `src/io/TextureLoader.cpp` : charge PNG et JPEG (**bibliothèque autre que MiniLibX/XPM** — exigence du sous-critère 5) via `stb_image` ou libpng/libjpeg, cache `nom → partagé` (`shared_ptr`, RAII), fichier absent → `Status` avec chemin complet, mémoire bornée par `limits`. »
- **Dépend** : T021, T034 · **Sert** : *Textures* sous-critère 5 · **Doc** : [SPECIFICATIONS.md §5.2 G](SPECIFICATIONS.md)
- **DoD** : 2 textures (1 PNG, 1 JPEG) dans `textures/` ; manquant → message, 0 crash ; valgrind propre (libération du cache).

#### T103 ⬜ — UV sur les 4 primitives (*Textures* 1–2)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Génère des coordonnées UV pour plan, sphère, cylindre, cône, et applique une texture à **chacun des 4 objets** dans une scène unique. Test : une texture à damier visible sur les 4 objets sans distorsion aberrante. »
- **Dépend** : T102, T041–T044 · **Sert** : *Textures* sous-critères 1-2 (2 pts) · **Doc** : [SPECIFICATIONS.md §5.2 G](SPECIFICATIONS.md)
- **DoD** : scène `scenes/opt_textures4.rt` ; les 4 objets sont texturés.

#### T104 ⬜ — Échelle et décalage de texture (*Textures* 3–4)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Directives `texture { scale (sx sy) offset (ox oy) rotate r }` appliquées **par objet** : on peut étirer (ou compresser) et décaler la texture. Test visuel + test unitaire sur les UV transformées. »
- **Dépend** : T103, T021 · **Sert** : *Textures* sous-critères 3-4 (2 pts) · **Doc** : [SPECIFICATIONS.md §5.2 G](SPECIFICATIONS.md)
- **DoD** : scène démontrant les 4 cas (étiré/décalé, séparément) ; test unitaire UV.

#### T105 ⬜ — Damier (*Disruptions* 2)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Pattern `checker` évalué en espace objet ou monde (option), taille réglable, applicable à la couleur **et** en masque de transparence plus tard. Test : alternance correcte sur un plan et sur une sphère. »
- **Dépend** : T103 · **Sert** : *Disruptions* 2 pts · **Doc** : [SPECIFICATIONS.md §5.2 D](SPECIFICATIONS.md)
- **DoD** : scène `scenes/opt_checker.rt` ; test unitaire de l'alternance.

#### T106 ⬜ — Bruit de Perlin (*Disruptions* 3–4)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Implémente un **bruit de Perlin** (table de permutation déterministe et seedée, fonctions de lissage, fractal 1-3 octaves) utilisable pour la couleur, la normale et la transparence. Attention : les 2 derniers points ne se comptent que si au moins un autre pattern existe (règle de la fiche). »
- **Dépend** : T105 · **Sert** : *Disruptions* **2 pts** · **Doc** : [SPECIFICATIONS.md §5.2 D](SPECIFICATIONS.md)
- **DoD** : test de déterminisme (même seed → même bruit) ; scène `scenes/opt_perlin.rt`.

#### T107 ⬜ — Perturbation de normale par onde (*Disruptions* 1)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Pattern `wave` : perturbation de la normale par `sin()` (effet vague/onde sur l'objet) — indépendant des couleurs. Test : la normale diffère de la géométrique de façon périodique, et disparaît à `amplitude=0`. »
- **Dépend** : T105 · **Sert** : *Disruptions* 1 pt · **Doc** : [SPECIFICATIONS.md §5.2 D](SPECIFICATIONS.md)
- **DoD** : test `amplitude=0` identique au rendu sans pattern ; scène `scenes/opt_wave.rt`.

#### T108 ⬜ — Barre de progression (*Environment 1*)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Message de chargement graphique + **barre de progression** dans l'UI (tuiles faites/total, spp accumulés, temps restant), affichée pendant le rendu et non seulement en fin de course. Sortie terminal conservée en parallèle pour le mode headless. »
- **Dépend** : T075, T063 · **Sert** : *Environment 1* · **Doc** : [SPECIFICATIONS.md §5.2 L](SPECIFICATIONS.md)
- **DoD** : la barre progresse réellement (vérifié sur une scène lente) ; en headless, la progression est imprimée.

#### T109 ⬜ — Interaction live (*Environment 3*)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Modifier caméra, position d'objet, couleurs, textures **sans relancer le programme** : chaque édition invalide via `sceneDirty` → basse qualité immédiate → affinage. Scène de démonstration `scenes/live.rt` + procédure écrite des gestes à faire devant le correcteur. »
- **Dépend** : T076, T104 · **Sert** : *Environment 3* · **Doc** : [SPECIFICATIONS.md §5.2 L](SPECIFICATIONS.md)
- **DoD** : aucun redémarrage nécessaire ; le protocole de démonstration est écrit en 5 gestes.

#### T110 ⬜ — Interface soignée (*Environment 2*)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Complète l'UI : chargement de fichier (parcourir/scène courante), contrôles de rendu (résolution, spp, pause/reprendre/restart), presets, raccourcis affichés, aspect propre (pas de texte tronqué). Si l'UI est jugée « jolie et complète », le point 1 (*Environment 1*) est inclus — vérifier la règle de la fiche. »
- **Dépend** : T108 · **Sert** : *Environment 1+2* · **Doc** : [SPECIFICATIONS.md §5.2 L](SPECIFICATIONS.md)
- **DoD** : parcours complet sans terminal ; captures dans `docs/preuves/ui/`.

#### T111 ⬜ — Cluster niveau 1 : `--tile` + script SSH (2 points)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Implémente `--tile k/n` (ou `--tile-x/--tile-w`) en mode headless **sans recouvrement** (0 pixel de chevauchement), puis écris `scripts/cluster_render.sh` : rsync du binaire/scènes/textures avec `--checksum`, une tuile par hôte, récupération par `scp`, montage avec ImageMagick. Supporte `HOSTS="127.0.0.1 127.0.0.1"` pour tester sans 2ᵉ machine. »
- **Dépend** : T035, T016, T026 · **Sert** : **item K1 = 2 pts** · **Doc** : [DISTRIBUTED_RENDERING.md](DISTRIBUTED_RENDERING.md)
- **DoD** : image rendue en tuiles **identique octet par octet** à l'image pleine (preuve du découpage propre) ; script exécutable sans interaction.

#### T112 ⬜ — Preuves et mesures du cluster
> **Fait le** : — · **Commit** : —
- **Prompt** : « Exécute et documente dans `docs/preuves/cluster.md` : temps 1 machine vs 2 (et 4 si possible), images côte à côte, zoom sur les coutures (il n'y en a pas), scénario de repli **sans réseau**, et la phrase « voici le script, lance-le ». Vérifie les clés SSH et le `montage` à l'avance. »
- **Dépend** : T111 · **Sert** : item K1 (2 pts) · **Doc** : [DISTRIBUTED_RENDERING.md §8](DISTRIBUTED_RENDERING.md)
- **DoD** : chiffres réels ; scénario de repli testé ; le correcteur peut lancer le script.

#### T113 ⬜ — Consolidation *Technical effects* (multi-thread, perf, screenshot)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Vérifie l'ensemble du bloc K avec des preuves écrites : multi-thread (mesure, TSan), vraiment rapide (`docs/BENCH.md` chiffré), screenshot dans le RT (T077), cluster (T111-112). Ajoute un tableau récapitulatif « item → preuve → commande » dans `docs/preuves/technical_effects.md`. »
- **Dépend** : T064, T067, T077, T112 · **Sert** : items K2, K3, K4 · **Doc** : [OPTIONS_GUIDE.md §9.2](OPTIONS_GUIDE.md)
- **DoD** : les 4 items ont chacun une commande démontrable en < 30 s.

#### T114 ⬜ — Rendu automatique avec modifications (*Environment 4*)
> **Fait le** : — · **Commit** : —
- **Prompt** : « `scripts/auto_render.sh` : une série de rendus où un paramètre varie entre chaque image (rotation de caméra, position d'objet, couleur), sans interface, en mode headless → sortie dans un dossier numéroté. C'est à la fois un item *Environment 4* et la base de la vidéo (T146). »
- **Dépend** : T035, T026 · **Sert** : *Environment 4* · **Doc** : [SPECIFICATIONS.md §5.2 L](SPECIFICATIONS.md)
- **DoD** : 1 commande → N images cohérentes et ordonnées ; durée documentée.

#### T115 ⬜ — Objets générés automatiquement (*Environment 5*)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Génération automatique d'objets pour une scène : hélice de sphères, tore de sphères, etc. — soit une directive de construction (`array`, `helix`) dans le schéma, soit un script qui écrit un `.rt`. Scène de démonstration versionnée. »
- **Dépend** : T101, T021 · **Sert** : *Environment 5* · **Doc** : [SPECIFICATIONS.md §5.2 L](SPECIFICATIONS.md)
- **DoD** : la scène générée existe et rend ; le générateeur est rejouable.

#### T116 ⬜ — Non-régression visuelle en CI
> **Fait le** : — · **Commit** : —
- **Prompt** : « Ajoute à la CI une étape de comparaison d'images (`compare -metric AE` ou hash) entre les rendus de `tests/golden/` et les nouveaux, avec une tolérance explicite. Une régression visuelle casse la CI. »
- **Dépend** : T059, T003 · **Sert** : qualité · **Doc** : [OUTILS.md §5](OUTILS.md)
- **DoD** : une modification volontaire de la couleur casse la CI (testée puis annulée).

#### T117 ⬜ — Mise à jour de `OPTIONS_GUIDE.md` (cocher les items acquis)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Dans `docs/OPTIONS_GUIDE.md`, marque chaque item P1 comme fait **avec la preuve** (scène, commande, image) ou reste ouvert. Aucun item n'est « coché » sans preuve exécutable. »
- **Dépend** : T100–T116 · **Sert** : notation · **Doc** : [OPTIONS_GUIDE.md](OPTIONS_GUIDE.md)
- **DoD** : chaque item coché référence une scène + une commande.

#### T118 ⬜ — Revue et gel des options P1
> **Fait le** : — · **Commit** : —
- **Prompt** : « Revue croisée de tout le code ajouté en P9 (1 relecteur), `make quality` vert, puis tag `v2-options-p1` et note au journal. Les régressions de l'obligatoire sont bloquantes. »
- **Dépend** : T117 · **Sert** : qualité, organisation · **Doc** : [PLAN_TRAVAIL.md §4](PLAN_TRAVAIL.md)
- **DoD** : revue tracée (commit ou PR), `make quality` vert, tag posé.

#### T119 ⬜ — Scène vitrine des options P1
> **Fait le** : — · **Commit** : —
- **Prompt** : « Écris `scenes/showcase.rt` : une scène mettant en valeur texture, damier, réflexion, groupe, lumière — utilisée pour les benchs et la démonstration. »
- **Dépend** : T100–T115 · **Sert** : démonstration · **Doc** : `docs/preuves/README.md`
- **DoD** : la scène rend en headless et en fenêtré ; elle est dans `render_all.sh`.

### Phase P10 — Options priorité 2

#### T120 ⬜ — Antialiasing (*Usual visual effects* 1)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Échantillonnage sous-pixel jitteré (N échantillons par pixel, grille ou aléatoire seedé) → bords nets. Test : une arête en diagonale a moins de « marches d'escalier » (mesure du nombre de pixels intermédiaires). »
- **Dépend** : T016, T036 · **Sert** : *Usual visual effects* 1 · **Doc** : [SPECIFICATIONS.md §5.2 J](SPECIFICATIONS.md)
- **DoD** : la mesure s'améliore ; coût documenté dans `docs/BENCH.md`.

#### T121 ⬜ — Cartoon effect (*Usual* 2)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Quantification des couleurs (palette réduite) + contours (détection de ruptures de normale/profondeur). Activable depuis la scène (`effect "cartoon"`). »
- **Dépend** : T120 · **Sert** : *Usual* 2 · **Doc** : [SPECIFICATIONS.md §5.2 J](SPECIFICATIONS.md)
- **DoD** : scène `scenes/opt_cartoon.rt` ; bascule on/off par fichier.

#### T122 ⬜ — Motion blur (*Usual* 3)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Intervalle de shutter : la caméra ou un objet a une position de début/fin, les rayons tirent un temps aléatoire dans `[t0,t1]` → flou de mouvement. Scène démonstration (objet en déplacement). »
- **Dépend** : T016, T045 · **Sert** : *Usual* 3 · **Doc** : [SPECIFICATIONS.md §5.2 J](SPECIFICATIONS.md)
- **DoD** : scène `scenes/opt_motion.rt` avec flou visible ; `shutter=0` identique au rendu net.

#### T123 ⬜ — Filtres de couleur : sépia et autres (*Usual* 4)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Post-filtres appliqués au framebuffer (sépia, niveaux de gris, inversion, LUT) — **depuis le fichier ou l'UI**, jamais uniquement en recompilant. Test unitaire sur une image synthétique. »
- **Dépend** : T030 · **Sert** : *Usual* 4 · **Doc** : [SPECIFICATIONS.md §5.2 J](SPECIFICATIONS.md)
- **DoD** : 2 filtres au minimum, pilotables par fichier.

#### T124 ⬜ — Stéréoscopie rouge/cyan (*Usual* 5)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Rendu des deux yeux avec une légère convergence (inter-axial réglable) et séparation des canaux rouge/cyan. Scène de démonstration + lunettes fournies si l'équipe en a (détail qui impressionne). »
- **Dépend** : T120, T031 · **Sert** : *Usual* 5 · **Doc** : [SPECIFICATIONS.md §5.2 J](SPECIFICATIONS.md)
- **DoD** : `scenes/opt_stereo.rt` produit une image stéréoscopique lisible.

#### T125 ⬜ — Objet natif simple : paraboloïde (+ hyperboloïde)
> **Fait le** : — · **Commit** : —
- **Prompt** : « `src/geometry/Paraboloid.cpp` (et hyperboloïde si le temps le permet) : intersection quadratique, normales, uv, bornes. Tests identiques aux autres primitives. »
- **Dépend** : T040 · **Sert** : *Simple native objects* · **Doc** : [SPECIFICATIONS.md §5.2 I](SPECIFICATIONS.md)
- **DoD** : au moins un objet natif supplémentaire fonctionnel + tests + scène.

#### T126 ⬜ — Texture : projection grossière + alpha
> **Fait le** : — · **Commit** : —
- **Prompt** : « *More texture applications* 1 et 2 : (1) projection planaire grossière de la texture sur l'objet, (2) texture utilisée pour la **transparence** (canal alpha / niveaux de gris → masque). Scènes distinctes pour chaque sous-critère. »
- **Dépend** : T102 · **Sert** : *More texture applications* 1-2 · **Doc** : [SPECIFICATIONS.md §5.2 H](SPECIFICATIONS.md)
- **DoD** : 2 scènes de preuve ; le masque alpha est réglable.

#### T127 ⬜ — Bump mapping
> **Fait le** : — · **Commit** : —
- **Prompt** : « *More texture applications* 3 : la texture **perturbe la normale** (dérivées finies du gris → normale modifiée) → effet de relief sans géométrie supplémentaire. Scène de preuve avec fort contraste. »
- **Dépend** : T102, T107 · **Sert** : *bump mapping* · **Doc** : [SPECIFICATIONS.md §5.2 H](SPECIFICATIONS.md)
- **DoD** : scène `scenes/opt_bump.rt` ; test : normale modifiée ≠ normale de base.

#### T128 ⬜ — Texture masquant l'albedo par endroits
> **Fait le** : — · **Commit** : —
- **Prompt** : « *More texture applications* 4 : la texture modifie la couleur **à certains endroits** seulement (masque, blend par zones) — distinct d'un simple remplacement d'albedo. Scène de preuve. »
- **Dépend** : T103 · **Sert** : *More texture applications* 4 · **Doc** : [SPECIFICATIONS.md §5.2 H](SPECIFICATIONS.md)
- **DoD** : scène `scenes/opt_mask.rt` montrant la zone modifiée et la zone intacte.

#### T129 ⬜ — Texture qui tranche + diapositive semi-transparente
> **Fait le** : — · **Commit** : —
- **Prompt** : « *More texture applications* 5 et 6 : (5) la texture **limite/tranche** l'objet (le masque masque les fragments) ; (6) un objet **semi-transparent posé devant** un autre agit comme une diapositive (teinte du fond visible à travers). Scènes séparées. »
- **Dépend** : T057, T166 · **Sert** : *More texture applications* 5-6 · **Doc** : [SPECIFICATIONS.md §5.2 H](SPECIFICATIONS.md)
- **DoD** : 2 scènes de preuve ; comportement visible et décrit.

#### T130 ⬜ — *Limited objects* : tranché sur axes, objet ou monde
> **Fait le** : — · **Commit** : —
- **Prompt** : « Sous-critères 1 et 2 : **limite/slice** de l'objet sur x, y, z (intervalles `bounds { x [-1 1] }`), et choix du référentiel — **coordonnées objet** (un cylindre tranché selon *son propre axe*) ou **monde**. Test : mêmes bornes, 2 référentiels → 2 images différentes. »
- **Dépend** : T043, T045, T021 · **Sert** : *Limited objects* 1-2 · **Doc** : [SPECIFICATIONS.md §5.2 C](SPECIFICATIONS.md)
- **DoD** : les 2 sous-critères ont une scène de preuve chacun.

#### T131 ⬜ — *Limited objects* : transformations, indépendance, formes libres
> **Fait le** : — · **Commit** : —
- **Prompt** : « Sous-critères 3, 4, 5 : (3) rotations/translations **continuent de fonctionner** après tranché ; (4) chaque objet a **son propre** tranché (pas d'effet global) ; (5) tranché **autre que par axes** (intersection avec triangle, disque, forme libre). »
- **Dépend** : T130 · **Sert** : *Limited objects* 3-5 · **Doc** : [SPECIFICATIONS.md §5.2 C](SPECIFICATIONS.md)
- **DoD** : 3 tests/scènes distincts ; la rotation après tranché est démontrée.

#### T132 ⬜ — Scènes et preuves de la phase P10
> **Fait le** : — · **Commit** : —
- **Prompt** : « Crée/vérifie une scène par item P10, toutes listées dans `render_all.sh`, avec une ligne de preuve (item → scène → commande) dans `docs/preuves/options_p10.md`. »
- **Dépend** : T120–T131 · **Sert** : notation · **Doc** : [CHECKLIST_DEFENSE.md §4](CHECKLIST_DEFENSE.md)
- **DoD** : chaque item P10 a sa scène et sa commande.

#### T133 ⬜ — Mise à jour de `OPTIONS_GUIDE.md` (P10) et revue
> **Fait le** : — · **Commit** : —
- **Prompt** : « Coche les items P10 avec preuves, exécute `make quality`, revue croisée, tag `v2-options-p2`, note au journal. »
- **Dépend** : T132 · **Sert** : notation, organisation · **Doc** : [OPTIONS_GUIDE.md](OPTIONS_GUIDE.md)
- **DoD** : guide à jour, qualité verte, tag posé.

#### T134 ⬜ — Rééquilibrage des efforts restants
> **Fait le** : — · **Commit** : —
- **Prompt** : « Recalcule le reste du travail (P11 + P12) vs le temps disponible, réordonne P11 en fonction du **rapport points/heure réel** constaté jusqu'ici, et écrit le plan de fin de projet dans `docs/PLAN_TRAVAIL.md`. »
- **Dépend** : T133 · **Sert** : organisation · **Doc** : [PLAN_TRAVAIL.md](PLAN_TRAVAIL.md)
- **DoD** : un ordre de priorité explicite pour P11, validé par l'équipe.

### Phase P11 — Options priorité 3 et exotiques

#### T140 ⬜ — Objets négatifs (*Negative objects*)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Soustraction d'objets (CSG négatif) : `object { subtract "sphere1" }` ou flag `negative`. Sphère qui perce un plan, cylindre qui creuse un cylindre. Test : la zone soustraite ne rend plus (le rayon continue ou est rejeté selon le modèle). »
- **Dépend** : T046, T045 · **Sert** : *Negative objects* · **Doc** : [SPECIFICATIONS.md §5.2 I](SPECIFICATIONS.md)
- **DoD** : scène `scenes/opt_negative.rt` ; test unitaire de la soustraction.

#### T141 ⬜ — Tore (*Exotic objects* 3)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Intersection du tore (équation du 4ᵉ degré ou méthode stochastique/Newton bien testée), normales analytiques, tests de robustesse (rayon au centre, tangence, axe). »
- **Dépend** : T040 · **Sert** : *Exotic objects* 1 pt · **Doc** : [SPECIFICATIONS.md §5.4](SPECIFICATIONS.md)
- **DoD** : scène `scenes/opt_torus.rt` ; tests verts, ASan/UBSan verts.

#### T142 ⬜ — Cube perforé et nappe de table (*Exotic* 1–2)
> **Fait le** : — · **Commit** : —
- **Prompt** : « (1) cube perforé (CSG de plans/cubes avec des trous, réutilise T140) ; (2) nappe de table = surface paramétrique (sinusoïdale ou repliée). Un point par objet, chacun avec sa scène. »
- **Dépend** : T140 · **Sert** : *Exotic objects* 1-2 pts · **Doc** : [SPECIFICATIONS.md §5.4](SPECIFICATIONS.md)
- **DoD** : 2 objets fonctionnels + 2 scènes.

#### T143 ⬜ — Exotique libre (*Exotic* 4–5)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Au choix dans l'ordre de valeur : (4) **résolution d'équations aléatoires depuis un fichier de configuration** (échantillonnage d'un champ pour créer des formes), ou (5) autre : fractale (IFS) générant des objets. Une scène de démonstration parlante. »
- **Dépend** : T141 · **Sert** : *Exotic objects* · **Doc** : [SPECIFICATIONS.md §5.4](SPECIFICATIONS.md)
- **DoD** : au moins un objet exotique supplémentaire, piloteable par fichier.

#### T144 ⬜ — Ruban de Möbius
> **Fait le** : — · **Commit** : —
- **Prompt** : « Paramétrisation du ruban de Möbius (surface non orientable), intersection robuste (sous-divisions ou équation exacte), normales correctes des deux côtés, scène mettant en valeur le retournement. »
- **Dépend** : T141 · **Sert** : item *The Moebius ribbon* (Oui/Non) · **Doc** : [SPECIFICATIONS.md §5.4](SPECIFICATIONS.md)
- **DoD** : `scenes/opt_mobius.rt` est « cool et bien implémenté » (revue par un pair) ; tests verts.

#### T145 ⬜ — Caustiques et/ou Global illumination
> **Fait le** : — · **Commit** : —
- **Prompt** : « Au choix, du plus simple au plus complet : (a) GI par rebonds multiples avec pondération, (b) caustiques par *photon mapping* simplifié (photons émis par les lumières, rassemblement en grille). Images à partager (item demandé explicitement de les partager). »
- **Dépend** : T057, T060 · **Sert** : *Caustics and/or GI* (Oui/Non) · **Doc** : [SPECIFICATIONS.md §5.4](SPECIFICATIONS.md)
- **DoD** : effet visible sur `scenes/opt_caustics.rt` ; coût documenté ; tests verts.

#### T146 ⬜ — Vidéo réalisée avec le RT (*In bulk* 1)
> **Fait le** : — · **Commit** : —
- **Prompt** : « `scripts/make_video.sh` : rend N images (`auto_render.sh`, T114) puis les assemble avec `ffmpeg` en `.mp4` (rotation de caméra autour de la scène), dépôt dans `docs/preuves/video/` et lien prêt à partager. »
- **Dépend** : T114 · **Sert** : *In bulk* 1 pt · **Doc** : [OUTILS.md §5](OUTILS.md)
- **DoD** : la vidéo existe, est lisible, est régénérable par une commande ; `ffmpeg` est listé comme dépendance dans le README.

#### T147 ⬜ — Spot non ponctuel : soft shadows (*In bulk* 4)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Source **étendue** (rayon/filament) : échantillonnage de la surface lumineuse → ombres sans netteté nette (pénombre proportionnelle à la distance). Scène comparative ombre dure/ombre douce. »
- **Dépend** : T051, T016 · **Sert** : *In bulk* 4 pts · **Doc** : [SPECIFICATIONS.md §5.4](SPECIFICATIONS.md)
- **DoD** : `scenes/opt_softshadow.rt` montre la pénombre ; coût documenté.

#### T148 ⬜ — Import de fichiers de modeleur (*In bulk* 2)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Import POV ou 3DS (au choix, format le plus simple à parser) converti en scène interne : découpage lexer/parser dédié dans `src/io/`, erreurs localisées, fichier d'exemple **dans le dépôt**. Fait abstraction de la complexité : triangles + matériaux suffisent. »
- **Dépend** : T023 · **Sert** : *In bulk* 2 pts · **Doc** : —
- **DoD** : un fichier importé rend correctement ; erreur propre sur un fichier invalide.

#### T149 ⬜ — Autre « truc de fou » au choix (*In bulk* 3 ou 5)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Choisis **un** impact fort et réalisable : rendu anaglyphe/3D TV, dépôt de rendu via un service, interface en plein écran façon logiciel pro, dénoiser simple, ou toute idée jugée remarquable. Documente le choix et la démonstration associée dans `docs/preuves/`. »
- **Dépend** : T134 · **Sert** : *In bulk* · **Doc** : [SPECIFICATIONS.md §5.4](SPECIFICATIONS.md)
- **DoD** : l'effet est démontrable en < 1 min pendant la soutenance.

#### T150 ⬜ — Cluster niveau 2 : coordinateur/worker
> **Fait le** : — · **Commit** : —
- **Prompt** : « Petit coordinateur HTTP (réutilise les acquis de Webserv : boucle événementielle, pool de slots, buffers fixes) : `GET /job` donne binaire/scène hashés + tuile, `POST /tile` renvoie l'image + checksum, `GET /status` → barre de progression, idempotence, timeout → re-dispatch, **fallback local** si 0 worker. »
- **Dépend** : T111 · **Sert** : item K1 (consolidation) + *Environment 1* · **Doc** : [DISTRIBUTED_RENDERING.md §4](DISTRIBUTED_RENDERING.md)
- **DoD** : 2 workers réels ou conteneurs exécutent l'image ; le repli local fonctionne (testé en débranchant).

#### T151 ⬜ — Cluster niveau 3 : MPI ou GPU compute (optionnel)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Si et seulement si P12 n'est pas menacé : GPU compute (OpenCL/CUDA, **autorisé** par le sujet, seule la génération finale par pipeline GPU l'est) ou MPI (`mpirun -np N`). Mesure le gain réel. Sinon, marquer `⛔ Reporté : P12 prioritaire`. »
- **Dépend** : T150 · **Sert** : bonus, *In bulk* · **Doc** : [SPECIFICATIONS.md §2.3](SPECIFICATIONS.md), [DISTRIBUTED_RENDERING.md §5](DISTRIBUTED_RENDERING.md)
- **DoD** : gain mesuré **ou** report motivé (le report est une issue acceptable).

#### T152 ⬜ — Scène vitrine : « Est-ce que c'est beau ? »
> **Fait le** : — · **Commit** : —
- **Prompt** : « Construis une scène **soignée compositionnellement** (profondeur de champ, palette cohérente, contraste, réflexion, texture) : c'est l'item subjectif *The last.... and the least*. Prends le temps de la lumière et du cadrage, et garde-la dans `render_all.sh`. »
- **Dépend** : T119, T120 · **Sert** : *Is it beautiful* (subjectif) · **Doc** : [SPECIFICATIONS.md §5.4](SPECIFICATIONS.md)
- **DoD** : l'image est dans `docs/preuves/`, régénérable, et a été revue par les 3 membres.

#### T153 ⬜ — Publication et partage des images
> **Fait le** : — · **Commit** : —
- **Prompt** : « Prépare un lot de images prêtes à publier (forum/slack du 42, comme demandé pour caustiques/GI) avec des noms parlants, une résistance correcte, et un fichier `docs/preuves/GALLERY.md` listant chaque image + l'option qu'elle prouve. »
- **Dépend** : T145, T152 · **Sert** : items qui demandent de « partager » · **Doc** : `docs/preuves/GALLERY.md`
- **DoD** : galerie complète, chaque image est reliée à un item.

#### T154 ⬜ — Revue globale des options et calcul de la note
> **Fait le** : — · **Commit** : —
- **Prompt** : « Passe en revue **tous** les items de la fiche (18 options + interlude + 5 more), mets à jour `docs/OPTIONS_GUIDE.md` avec la preuve de chacun, et calcule une estimation de note (`docs/SPECIFICATIONS.md §5`). Liste les items faisables restants par heure de travail. »
- **Dépend** : T140–T153 · **Sert** : notation · **Doc** : [SPECIFICATIONS.md §5](SPECIFICATIONS.md)
- **DoD** : un tableau complet « item / état / preuve / points estimés » existe ; la suite est priorisée.

### Phase P12 — Soutenance

#### T160 ⬜ — Gel final du code
> **Fait le** : — · **Commit** : —
- **Prompt** : « `make re && make test && make quality` verts, `git status` propre, plus aucune modification en cours, tag `v2-freeze`. À partir d'ici : **correctifs uniquement**, et tout correctif repasse par la suite de tests. »
- **Dépend** : T154 · **Sert** : stabilité · **Doc** : [CHECKLIST_DEFENSE.md §8](CHECKLIST_DEFENSE.md)
- **DoD** : tag posé, dépôt vert, aucune branche orpheline de travail.

#### T161 ⬜ — Vérification B1–B4 le jour J
> **Fait le** : — · **Commit** : —
- **Prompt** : « Contrôle exact : dépôt clonable et buildable dans un dossier vierge (`git clone <url> /tmp/x && cd /tmp/x && make re && make test`), fichier `author` conforme, arbitrage de norme respecté, **présence des 3 membres** confirmée. »
- **Dépend** : T086, T160 · **Sert** : **B1–B4 = 0 sinon** · **Doc** : [SPECIFICATIONS.md §4.2](SPECIFICATIONS.md)
- **DoD** : le clone vierge a été réellement testé, dans les conditions de la salle (sur un autre poste si possible).

#### T162 ⬜ — Toutes les scènes en une commande
> **Fait le** : — · **Commit** : —
- **Prompt** : « `sh scripts/render_all.sh` rend **toutes** les scènes du dépôt sans erreur, en un temps tenable, avec résumé. Une scène absente du dépôt ou qui casse = bloquant. »
- **Dépend** : T083, T132 · **Sert** : M8, démonstration · **Doc** : `docs/preuves/README.md`
- **DoD** : sortie 0 ; liste des scènes == fichiers `.rt` présents.

#### T163 ⬜ — Validation mémoire finale
> **Fait le** : — · **Commit** : —
- **Prompt** : « `valgrind --leak-check=full` sur **toutes** les scènes (0 fuite), `make asan` + `make tsan` verts, `scripts/fuzz.sh` sans crash. Rapport consolidé dans `docs/preuves/quality.md` (exigence « no memory leaks » du sujet). »
- **Dépend** : T085, T162 · **Sert** : exigence sujet · **Doc** : [OUTILS.md §3](OUTILS.md)
- **DoD** : rapport écrit avec les commandes et leurs codes retour réels.

#### T164 ⬜ — Documentation finale (README, AGENTS, galerie)
> **Fait le** : — · **Commit** : —
- **Prompt** : « README à jour : installation des dépendances, build, tous les flags, format de scène, options implémentées, dépendances (SDL2, libpng/libjpeg, ffmpeg optionnel), **comment rendre les images**. `AGENTS.md` : commandes et état des modules. `docs/preuves/GALLERY.md` complète. »
- **Dépend** : T163 · **Sert** : démonstration, *Group organization* · **Doc** : `README.md`
- **DoD** : un nouveau membre build et rend une image en suivant uniquement le README.

#### T165 ⬜ — Dossier de preuves finalisé
> **Fait le** : — · **Commit** : —
- **Prompt** : « Rassemble dans `docs/preuves/` : les 3 images obligatoires, les images d'options avec leur item associé, les mesures (bench, cluster, expose), les captures d'UI, la vidéo, le tableau « item → preuve → commande ». Chaque preuve est **régénérable**. »
- **Dépend** : T164 · **Sert** : preuves · **Doc** : [CHECKLIST_DEFENSE.md §9.2](CHECKLIST_DEFENSE.md)
- **DoD** : le tableau est complet ; aucune image manuelle.

#### T166 ⬜ — Répétition n°1 (mock defense)
> **Fait le** : — · **Commit** : —
- **Prompt** : « Joue la soutenance complète avec les 3 membres (protocole de `docs/CHECKLIST_DEFENSE.md §6`), chronomètre, note chaque point bloquant, corrige **immédiatement** ce qui peut l'être et crée les tâches restantes. »
- **Dépend** : T165 · **Sert** : soutenance · **Doc** : [CHECKLIST_DEFENSE.md §6](CHECKLIST_DEFENSE.md)
- **DoD** : compte rendu daté ; les corrections sont des tâches ou faites.

#### T167 ⬜ — Répétition n°2 chronométrée
> **Fait le** : — · **Commit** : —
- **Prompt** : « Seconde répétition **au chronomètre** dans les conditions réelles (même poste, même écran, sans notes pour celui qui présente), avec le quiz croisé des 3 modules. »
- **Dépend** : T166 · **Sert** : soutenance · **Doc** : [CHECKLIST_DEFENSE.md §6](CHECKLIST_DEFENSE.md)
- **DoD** : tenue dans le temps imparti ; chaque membre a répondu au quiz.

#### T168 ⬜ — Préparation des réponses et répartition de la parole
> **Fait le** : — · **Commit** : —
- **Prompt** : « Remplis `docs/CHECKLIST_DEFENSE.md §7` avec les réponses réelles (archi, choix techniques, mémoire, perf, options, IA), répartis qui dit quoi, et note les 5 questions à ne pas laisser sans réponse. »
- **Dépend** : T167 · **Sert** : soutenance · **Doc** : [CHECKLIST_DEFENSE.md §7](CHECKLIST_DEFENSE.md)
- **DoD** : chaque membre sait quoi dire pour chaque section ; réponses écrites.

#### T169 ⬜ — Checklist du matin et jour de la soutenance
> **Fait le** : — · **Commit** : —
- **Prompt** : « Imprime/remplis la checklist du matin (`CHECKLIST_DEFENSE §10`) : repo à jour et buildé, scènes ouvertes, sauvegardes, notifications coupées, aliments, `author`, présence des 3, ordre de passage. Vérifie le matériel la veille. »
- **Dépend** : T168 · **Sert** : soutenance · **Doc** : [CHECKLIST_DEFENSE.md §10](CHECKLIST_DEFENSE.md)
- **DoD** : la checklist est prête à être cochée le matin J.

## 4. Points ouverts à trancher

Chaque point doit être tranché **avant** la tâche listée, et le résultat écrit dans
`docs/ADR/00X-*.md` (détail complet : [SPECIFICATIONS.md §8](SPECIFICATIONS.md)).

| # | Point ouvert | À trancher avant | Décision | Statut |
|---|--------------|------------------|----------|--------|
| 1 | Format du fichier `author` (B2) | T005 | Un login 42 par ligne, ordre alphabétique | ✅ |
| 2 | Applicabilité de la **norminette** (B3) | T005 | Non applicable au C++ ; équivalent clang-format/clang-tidy + confirmation corrigé | ✅ |
| 3 | Forme exacte du format `.rt` structuré | T020 | — | ⬜ |
| 4 | Ambiguïté de *Direct light* (headlight vs spot) | T058 | — | ⬜ |
| 5 | Périphérie graphique autorisée (SDL/GPU vs pure CPU) | T070 | — | ⬜ |
| 6 | Seuil « vraiment rapide » : que considère-t-on comme rapide ? | T067 | — | ⬜ |
| 7 | Type de rendu de référence : `float` ou `double` | T010 | — | ⬜ |
| 8 | Découpage des tâches entre les 3 membres | T005 | Dev A maths/caméra, Dev B géométrie/scène, Dev C rendu/plateforme (ADR-001 §7) | ✅ |

---

## 5. Journal des sessions

> Le journal des sessions vit dans **[`docs/JOURNAL.md`](docs/JOURNAL.md)** — une ligne
> par session, ajoutée en fin de session (« ajouter, jamais réécrire »), committée avec
> le traçage de la tâche (section 6). Ne **pas** écrire de journal ici : la checklist
> pointe vers le fichier pour éviter toute duplication.

---

## 6. Conventions de commit

### 6.1 Format

```
type(scope): sujet court en anglais, à l'impératif  (#T011)
```

| `type` | Emploi |
|--------|--------|
| `feat` | fonctionnalité (un module, une primitive, une option) |
| `fix` | correction d'un bug |
| `test` | ajout ou correction de tests |
| `docs` | documentation uniquement |
| `perf` | optimisation **mesurée** (chiffrée dans le corps du message) |
| `refactor` | restructuration sans changement de comportement |
| `build`, `ci`, `chore` | Makefile, CI, scripts, outillage |

Corps du message : **ce qui a été fait**, **comment le DoD a été vérifié** (commande réelle),
et les chiffres s'il y en a. Pas de corps vide pour une tâche de la checklist.

### 6.2 Exemples

```
feat(geometry): add sphere intersection (#T041)

Analytic root selection with tMin/tMax bounds, oriented normal.
DoD: make test + make asan green (6 cases incl. tangent and inside-ray).
```

```
fix(scene): localized lexer errors with line and column (#T022)

DoD: tests/cases/invalid/* all return != 0, no crash under ASan.
```

### 6.3 Règles

- **Jamais de commit sur `main`** (archive v1) ni de `push --force`.
- Un commit = un état vert : `make re && make test` passe avant.
- La mise à jour du statut de la tâche (ce fichier) se fait **dans le même commit**
  que le travail qu'elle décrit, sinon la checklist ment.
- Pas de commit « wip » en fin de session : soit fini, soit laissé `⬜`.

---

## 7. État initial de la checklist

- **141 tâches** écrites, toutes `⬜` (sauf mention contraire au journal).
- **Aucune tâche n'est bloquée** au démarrage.
- La tâche d'entrée est **T000**.


