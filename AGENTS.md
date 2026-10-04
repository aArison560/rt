# RT — AGENTS.md

> Guide de travail pour **toute personne ou tout agent** intervenant sur le dépôt.
> Branche de travail : `dev` · `main` = archive v1 (**jamais de commit dessus**).
> Source de vérité des tâches : [`docs/CHECKLIST_TACHES.md`](docs/CHECKLIST_TACHES.md)
> (lire sa **section 0** avant de commencer).

## Commandes

| Commande | Rôle |
|----------|------|
| `make re` | build complet, `-Wall -Wextra -Werror -O2` (0 warning exigé) |
| `make test` | tests (`Catch2` à partir de T017) |
| `make asan` / `make tsan` / `make fast` | sanitizers / build `-O3 -march=native` |
| `make format` | reformate tout le code selon `.clang-format` (**idempotent**) |
| `make lint` | analyse statique `.clang-tidy` — **échoue sur le moindre diagnostic** |
| `make compdb` | `compile_commands.json` pour clangd |
| `sh scripts/check_env.sh` | état de l'environnement (✔ / ✖ / ⚠) |

## Structure des tests (T017)

```
tests/
├── unit/          # un fichier test_<module>.cpp par module (Catch2)
├── integration/   # tests de bout en bout (parser, rendu headless)
└── cases/         # fixtures .rt : valid/ et invalid/ (+ README)
```

- **Catch2** est vendored (versionné) dans `thirdparty/catch2/`
  (`catch_amalgamated.hpp` + `.cpp`, v3.16.0). Ne pas le régénérer à la main.
- `make test` : build + exécution de `./rt_test` ; **code retour ≠ 0 si échec**.
- `make test-asan` : mêmes tests compilés/exécutés sous ASan/UBSan
  (objets et binaire séparés : `obj-test-asan/`, `rt_test_asan`).
- Les fixtures `tests/cases/` suivent le format cible (T020) et seront rejouées
  par le parser (T022+) : `valid/` acceptées, `invalid/` rejetées **sans crash**.
- Les tests sont compilés avec les mêmes flags que le projet
  (`-Wall -Wextra -Werror`) : aucun warning toléré sur `tests/`.

Outils de formatage : `clang-format` / `clang-tidy` (noms versionnés acceptés :
`clang-format-19`, `clang-tidy-19`…). Si absent : `sudo apt install clang-format-19 clang-tidy-19`,
ou surcharge manuelle `make lint CLANG_TIDY=/chemin/vers/clang-tidy`.

## Revue de code

**Règle** : aucun travail ne fusionne sur `dev` sans **1 relecteur minimum** (pas d'auto-merge,
pas de « wip » en fin de session). La relecture porte sur la PR/commit, pas sur l'auteur.

Checklist du relecteur — **les 4 cases doivent être cochées** avant merge :

1. **Tests** — `make re && make test` verts ; toute nouvelle fonctionnalité a son test ;
   les cas limites renvoient un code retour ≠ 0 **sans crash** (vérifié sous `make asan`).
2. **0 warning** — compilation sans aucun warning (`-Wall -Wextra -Werror`) **et**
   `make lint` sans diagnostic sur `src/`, `include/`, `tests/`.
3. **Style** — `make format` exécuté avant commit ; le diff de formatage ne cache
   aucune modification fonctionnelle.
4. **Doc** — tout changement de comportement met à jour le document concerné
   **dans le même commit** ; l'avancement est tracé dans `docs/CHECKLIST_TACHES.md`.

Points de vigilance spécifiques au projet :

- **Hot path** : aucune allocation dynamique ni exception pendant le rendu
  (règles R2/R3 de [`docs/CHECKLIST_TACHES.md` §2.3](docs/CHECKLIST_TACHES.md)) ;
  `grep -R "throw" src/` doit rester vide hors `src/app/main.cpp`.
- **Calques** : un calque ne voit que celui du dessous (`render/` ignore SDL et microui).
- **Preuves** : image = régénérée par script (`scripts/…`), jamais fabriquée à la main.
