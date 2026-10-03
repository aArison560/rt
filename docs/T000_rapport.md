# T000 — Rapport de validation du point de départ

Date : 2026-10-03

## Constats

| Vérification | Résultat |
|---|---|
| `git branch --show-current` | `dev` ✅ |
| `git status` | propre, à jour avec `origin/dev` ✅ |
| `docs/CHECKLIST_TACHES.md` | présent et lisible ✅ |
| `main` | inchangé depuis l'archive v1 ✅ |
| `src/`, `include/`, `tests/` | absents de `dev` ✅ |
| `docs/` | présent (spécifications, options, plan, guides) ✅ |

## Ce qui manque pour démarrer un projet C++23 propre

- Arborescence de cibles : `src/`, `include/rt/`, `tests/`, `scenes/`, `scripts/`, `thirdparty/`
- `src/app/main.cpp` minimal imprimant `rt <version>` (T001)
- `Makefile` conforme §2.2 (T001)
- Cibles qualité : `asan`, `tsan`, `fast`, `compdb` (T002)
- `scripts/check_env.sh` (T002)
- CI `.github/workflows/ci.yml` (T003)
- `.clang-format`, `.clang-tidy`, cibles `format`/`lint` (T004)
- `docs/ADR/001-decisions.md` tranchant les 8 points ouverts (T005)
- Fichier `author` à la racine (T005)
- Dépendances vendorisées : Catch2, microui, stb (T002/T017)

## DoD

- Rapport court écrit : ce document.
- `git status` propre (après commit de ce rapport).
- Branche `dev`.
