# tests/cases — fixtures de scènes `.rt`

Fixtures du futur parser (T022+). Elles suivent le **format cible** spécifié en
T020 : blocs imbriqués `{ }`, commentaires `#`, nombres, vecteurs `(x y z)`.

| Fichier | Rôle | Rejeté par |
|---------|------|-------------|
| `valid/minimal.rt` | scène minimale valide (1 sphère, 1 lumière) | — (accepté) |
| `valid/group.rt` | scène à groupes + 2 lumières | — (accepté) |
| `valid/material.rt` | matériau complet (diffuse/specular/reflect/transparency+ior) | — (accepté, T027) |
| `valid/transform.rt` | translate + scale + rotate | — (accepté, T027) |
| `valid/lights.rt` | point + directional + spot (position/direction/target) | — (accepté, T027) |
| `valid/limits.rt` | `limits` explicites + background/ambient | — (accepté, T027) |
| `valid/primitives.rt` | 4 primitives (sphère/plan/cylindre/cône) | — (accepté, T027) |
| `valid/alias.rt` | alias (`lookAt`/`color`/`reflect`/`position`) + têtes | — (accepté, T027) |
| `invalid/missing_brace.rt` | accolade fermante manquante (`{` non fermé) | lexer T022 |
| `invalid/unclosed_string.rt` | guillemet non fermé | lexer T022 |
| `invalid/binary.rt` | octet binaire 0xFF (UTF-8 invalide) | lexer T022 |
| `invalid/bad_float.rt` | nombre lexicalement absurde (`1.2.3`) | lexer T022 |
| `invalid/bad_number.rt` | `abc` là où un nombre est attendu (ident valide) | parser T023 |
| `invalid/unknown_directive.rt` | directive inconnue (`witdh`) | parser T023 |
| `invalid/empty.rt` | fichier vide (0 octet) | parser T025 (`empty file`) |
| `invalid/deep_nesting.rt` | 40 groupes imbriqués (profondeur > 32) | lexer T025 (`LimitExceeded`) |
| `invalid/include.rt` | directive `include` inconnue (format sans `include`, aucun cycle) | parser T025 (`NotFound`) |
| `invalid/truncated.rt` | fichier tronqué en plein vecteur (fuzz : suppression) | parser T025 |
| `invalid/garbage.rt` | caractères hors vocabulaire (`@`, `;`, `$`) | lexer T025 |

Les fichiers `invalid/` lexicaux (T022) et sémantiques (T023+) doivent être
**rejetés** avec une erreur localisée `fichier:ligne:colonne` et un code
retour ≠ 0, **sans crash** (vérifié sous ASan). Les fichiers `valid/`
doivent être acceptés. Depuis T023, `./rt <fichier>` parse completement
(lexer + parser) : les 8 valides sortent 0, les 11 invalides sortent ≠ 0.
Depuis T025, `tests/integration/test_bad_files.cpp` rejoue tous ces cas
(inexistant, répertoire, vide, illisible, imbrication folle, `include`,
binaire + fuzz suppression/duplication de tokens) sous `make test`
et `make test-asan`.
Depuis T027, `scripts/run_cases.sh` rejoue les 19 fichiers en headless
(`make test` l'appelle après `./rt_test`, < 10 s : ~0.03 s mesuré).

> Ces fixtures pourront être ajustées quand le format sera gelé (T020/T021) ;
> elles servent dès maintenant de référence exécutable pour le parser.
