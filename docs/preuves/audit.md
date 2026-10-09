# RT — Rapport d'audit interne (T087)

> Audit execute le 2026-10-09 en jouant `docs/CHECKLIST_DEFENSE.md`
> (§5 technique, §8 pieges, §9 livrables) comme le correcteur.
> Commandes executees depuis la racine du depot, resultats reels.

## §5.1 Build et qualite — tout vert sauf TSan conteneur (reporte)

| Controle | Resultat |
|----------|----------|
| `make re` 0 warning | ✅ vert (builds T080–T086, `-Wall -Wextra -Werror`) |
| `make test` vert | ✅ 247 cas / 279218 assertions + 20/20 `run_cases.sh` |
| `valgrind` 0 fuite sur les scenes | ✅ 10/10 `scenes/*.rt` : `ERROR SUMMARY: 0 errors` (T085) |
| `make asan` 0 erreur | ✅ fuzz 200 fichiers sous ASan/UBSan : 0 crash (T085) |
| `make tsan` | ⚠ non rejoue dans ce conteneur (`FATAL ThreadSanitizer: unexpected memory mapping`, limitation connue sessions 44/49/50) ; valide en T066 + couvert par la CI ; a rejouer en T163 sur la machine de demo |
| aucune exception dans la boucle de rendu | ✅ `grep -Rn "throw;" src/` vide hors `app/main` (seul `new (std::nothrow)` + commentaires mentionnent `throw`) |
| `./rt` sans argument | ✅ message + usage, code 2 |
| `./rt /nonexistent.rt` | ✅ `cannot open file`, code 1, pas de crash |
| `./rt tests/cases/invalid/garbage.rt` | ✅ `garbage.rt:5:1: invalid character '@'`, code 1 |
| `./rt scenes/default.rt 0 0` | ✅ `bad width: expected 1..8192` + usage, code 2 |

## §5.2 Scenes — ✅

- Toutes les scenes de `scenes/` se lancent : `sh scripts/render_all.sh`
  → **18/18 en 9 s**, exit 0 (T083).
- 3 obligatoires presentes : `fig_vi1.rt` (T080), `fig_vi2.rt` (T081),
  `fig_vi3.rt` (T082) ; ressemblance aux figures VI.1/VI.3 evaluee a l'oeil
  (ecarts assumes ecrits dans les en-tetes).
- Ecart de nommage assume : la checklist de defense cite
  `scenes/01_obligatoire_*.rt` ; nos fichiers s'appellent `fig_vi1/vi2/vi3.rt`
  (noms explicites stables depuis T048/T059, repris par les tests golden).
  Aucune tache : les 3 commandes de §3.6 fonctionnent avec nos noms.
- Aucun chemin absolu dans les scenes (`grep -R "/" scenes/*.rt` ne montre
  que des commentaires) ; pas de `textures/` requises (aucune scene P8 n'en
  charge — les textures arrivent en P9/T102).

## §5.3 Environnement de demo — a faire le jour J

Chargeur, notifications, ecran, `git pull` + build sur la machine de demo,
sauvegardes, PDF ouverts, 3 membres : points d'organisation, pas de code.
Repris dans la checklist du matin (T169).

## §8 Pieges — parades verifiees

| # | Piege | Parade verifiee |
|---|-------|-----------------|
| 1 | segfault sur mauvais chemin | ✅ codes 1/2, fuzz 400 fichiers 0 crash, `run_cases` 0 crash |
| 2 | scene manquante | ✅ 18/18 versionnees + test post-clone T086 |
| 3 | preuve par PNG pre-rendu | ✅ `render_all.sh` recalcule tout depuis zero (`rm *.png` d'abord) |
| 4 | option en dur | ✅ tout est pilote par fichier (R1 : table `src/schema/`) |
| 5 | membre absent | organisation (T089) |
| 6 | `author` | ✅ 3 logins, un par ligne, ordre alphabetique (T086) |
| 7 | norme | ✅ arbitrage ecrit ADR-001 §4 (T086) |
| 8 | mauvais repo | ✅ clone frais teste T086 |
| 9 | ajout la veille | regle de gel posee en T088 |
| 10 | ne pas savoir expliquer | quiz T166–T168 (P12) |
| 11 | expose qui recalcule | ✅ `grep render(` vide dans `src/platform/`, mesure T072 |
| 12 | scene 2 ≠ scene 1 | ✅ test `[mandatory]` T081 |

## §9 Livrables

### 9.1 Indispensables — ✅ tous presents

`rt` (racine, `make re`), `author` (racine, 3 noms), `README.md` (build,
usage, options, scenes, T086), 3 obligatoires (`scenes/fig_vi1/vi2/vi3.rt`),
scripts relus (`render_all.sh`, `run_cases.sh`, `fuzz.sh`, `bench.sh`,
`gen_golden.sh`, `gen_doc.sh`, `check_env.sh`, `quality.sh`).

### 9.2 Pour les points — reporte P9–P11 (pas de tache nouvelle)

`opt_*.rt` par option, `textures/`, `batch_render.sh` (T114), generateur
(T115), `--tile`/cluster (T111) : ce sont les taches P9–P11 deja ecrites
(T100–T115). Aucune tache ⬜ ajoutee : le report est explicite et trace.

### 9.3 Organisation — ✅ / en cours

`docs/JOURNAL.md` (une ligne par session), backlog = `CHECKLIST_TACHES.md`
(sections 1–3 + §4 points ouverts), historique propre (1 commit par tache,
messages avec DoD), transparence IA :_overlay non applicable — tout est
relu et teste (DoD executé, jamais suppose). `PLAN_TRAVAIL.md` mis a jour
en T089.

## Manques — aucun ⬜ nouveau

Tous les manques sont soit corriges dans cette phase (T080–T086), soit des
taches P9–P12 deja ecrites, soit des gestes du jour J (T169). Le report
TSan-conteneur et le report P9–P11 sont explicites ci-dessus.
