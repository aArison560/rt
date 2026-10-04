# ADR-001 — Décisions structurantes du projet rt

> Statut : **accepté** · Date : 2026-10-04 · Auteurs : les 3 membres du groupe
>
> Cet ADR tranche les 7 questions ouvertes de `docs/CHECKLIST_TACHES.md` §4
> avant toute ligne de code métier (Phase P1). Toute évolution ultérieure
> passe par un ADR-002, jamais par une modification silencieuse.

---

## 1. Langage et build : **C++23 et Makefile**

- Langage : **C++23** (`-std=c++2c`, GCC ≥ 14), exécutable `rt`, compilateur `c++`.
- Build : **Makefile à la 42** (`NAME`, `CXXFLAGS = -Wall -Wextra -Werror -O2 -std=c++2c`,
  cibles `all clean fclean re test asan tsan fast compdb lint format quality`).
- Pas de CMake à la racine tant que le Makefile suffit ; CMake sera étudié
  uniquement si une dépendance tierce l'impose (décision reportée).
- `main` = archive v1, jamais touchée ; tout le travail se fait sur `dev`.

## 2. Gestion d'erreurs : **codes dans le hot path, un seul `try/catch` dans `main`**

- Règle **R2** : aucune fonction du hot path (rendu, géométrie, scène) ne lance
  d'exception. Les erreurs sont rapportées par `rt::Status` (code + message +
  ligne) ou `rt::Result<T>`, et les fonctions de validation renvoient `bool`.
- `grep -R "throw" src/` doit rester vide **hors `src/app/main.cpp`**.
- `main` possède **un unique** `try { ... } catch (...)` de filet : il logge
  l'erreur inattendue et renvoie un code ≠ 0 (jamais de crash muet).
- Justification : latence prévisible, hot path sans coût d'exception, traçage
  systématique des chemins d'erreur (tests de propagation obligatoires, T015).
- Concrétisation (T015) : `include/rt/base/Status.hpp` (`StatusCode`, `Status`
  avec `message` + `line`, macro `RT_ERROR` qui capture `__LINE__`),
  `include/rt/base/Result.hpp` (`Result<T>` déplaçable, sans exception,
  `fail()` propage le `Status` d'origine), `include/rt/base/Log.hpp`
  (`rt::log::{info,warn,error}` sur `stderr`, niveau par `RT_LOG`),
  filet unique dans `src/app/main.cpp`, propagation sur 3 niveaux testée dans
  `tests/unit/test_status.cpp` (aucun chemin d'erreur muet : message toujours
  non vide et préservé).

## 3. Format de scène : **`.rt` structuré imbriqué** (item *File ++*)

- Le format est **texte structuré à blocs imbriqués `{ }`**, pas « une
  information par ligne » (explicitement exclu par le critère *File ++*).
- Forme : `scene { limits{} camera{} background{} lights{ light{} } objects{ object{} group{} } }`,
  commentaires `#`, chaînes entre guillemets, nombres, vecteurs `(x y z)`.
- Annexe XML équivalente fournie pour montrer que le modèle est un arbre.
- La spécification complète (grammaire EBNF, directives, exemples) est figée
  par **T020** dans `docs/FORMAT_SCENE.md` ; le schéma déclaratif unique des
  directives est **T021**.

## 4. Arbitrage norminette : **non applicable — tranché noir sur blanc**

- Le sujet v4.1 autorise **C, C++ ou Rust** avec « liberté totale
  d'organisation des fichiers » ; la norminette n'a de sens que pour du C.
- La fiche d'évaluation exige « Norm is OK (using the norminette) » alors que
  nous écrivons du **C++** : les deux exigences ne peuvent pas être satisfaites
  littéralement en même temps.
- **Décision** : on ne soumet pas le C++ à la norminette (elle ne le parse pas).
  À la place, on fournit l'équivalent qualité : `.clang-format` (idempotent,
  `make format`) + `.clang-tidy` strict (`make lint`, 0 diagnostic exigé) +
  `-Wall -Wextra -Werror`. `norminette` est bien installée et a été testée sur
  le dépôt (2026-10-04) : elle ne reconnaît pas les en-têtes/sources C++.
- **Action** : demander confirmation au corrigé/campus avant la soutenance ;
  si la norminette est exigée malgré tout, on figera alors dans cet ADR le
  périmètre exact (fichiers C le cas échéant) et on ajoutera une cible
  `make norm`. Jusque-là, B3 est considéré **non applicable au C++**.

## 5. Découpage en calques : **section 2.1 de CHECKLIST_TACHES.md**

- Architecture en couches type Blender, règle d'or : **un calque ne voit que
  celui du dessous** (`render/` ignore SDL et microui, `platform/` ignore les
  primitives et le parser).
- Ordre des dépendances : `app → ui/platform → sched/render → scene/schema →
  geometry/shading/lighting/accel → io → base`.
- Conséquences tenues pour vraies : mode **headless** (et donc cluster) et
  tests unitaires de chaque calque isolément.
- La documentation par calque vit dans `docs/ARCHITECTURE.md`.

## 6. Stratégie de branches : **`dev` = travail, `main` = archive v1**

- **`main`** : archive de la v1, protégée, **jamais de commit** dessus, jamais
  de push forcé. Consultable en lecture seule (`git show main:...`).
- **`dev`** : branche d'intégration du projet v2 ; tout commit y est vert
  (`make re && make test` avant chaque commit) et suit la section 6 de la
  checklist (`type(scope): sujet (#Txxx)`).
- Un commit par avancement de tâche ; pas de « wip » en fin de session.
- Pas de push sur `origin` sans accord de l'équipe.

## 7. Répartition des 3 développeurs sur P1–P8

Rôles de référence (`docs/PLAN_TRAVAIL.md` §2), un module principal par
personne et un backup croisé — personne ne valide seul son code :

| Membre | Principal P1–P8 | Backup |
|--------|------------------|--------|
| **Dev A** (math & caméra) | T010–T013 (base math), T070–T079 (affichage/interaction, caméra) | parser (T021–T026), BVH (T060) |
| **Dev B** (géométrie & scène) | T020–T029 (schéma/scène), T040–T048 (primitives/transforms), T050–T059 (matériaux/lumières) | tests maths, arena (T014) |
| **Dev C** (rendu & plateforme) | T014–T019 (mémoire, erreurs, RNG, benchs), T030–T037 (rendu minimal), T060–T067 (perf/BVH/threads), T080–T089 (gel obligatoire) | affichage (T070), options P9+ |

Rôles transverses tournants : responsable qualité (quality gates), responsable
scènes/démo, responsable organisation (journal, répartitions).

---

## Conséquences

- T010 tranchera le point ouvert n°7 de §4 (`float` vs `double` — décision :
  `float` pour le hot path, tolérances de test en conséquence) — consigné dans
  l'ADR correspondant lors de T010 si différent.
- `author` créé à la racine (format : **un login par ligne**, les 3 logins du
  groupe, ordre alphabétique — voir §1 ci-dessous).

### Format exact du fichier `author`

```
<login1>
<login2>
<login3>
```

Un login 42 par ligne, UTF-8, pas de ligne vide finale superflue. Vérifié à la
racine du dépôt (contrôle B2).
