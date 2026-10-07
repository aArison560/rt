# tests/cases — fixtures de scènes `.rt`

Fixtures du futur parser (T022+). Elles suivent le **format cible** spécifié en
T020 : blocs imbriqués `{ }`, commentaires `#`, nombres, vecteurs `(x y z)`.

| Fichier | Rôle | Rejeté par |
|---------|------|-------------|
| `valid/minimal.rt` | scène minimale valide (1 sphère, 1 lumière) | — (accepté) |
| `valid/group.rt` | scène à groupes + 2 lumières | — (accepté) |
| `invalid/missing_brace.rt` | accolade fermante manquante (`{` non fermé) | lexer T022 |
| `invalid/unclosed_string.rt` | guillemet non fermé | lexer T022 |
| `invalid/binary.rt` | octet binaire 0xFF (UTF-8 invalide) | lexer T022 |
| `invalid/bad_float.rt` | nombre lexicalement absurde (`1.2.3`) | lexer T022 |
| `invalid/bad_number.rt` | `abc` là où un nombre est attendu (ident valide) | parser T023 |
| `invalid/unknown_directive.rt` | directive inconnue (`witdh`) | parser T023 |

Les fichiers `invalid/` lexicaux (T022) et sémantiques (T023+) doivent être
**rejetés** avec une erreur localisée `fichier:ligne:colonne` et un code
retour ≠ 0, **sans crash** (vérifié sous ASan). Les fichiers `valid/`
doivent être acceptés. Depuis T023, `./rt <fichier>` parse completement
(lexer + parser) : les 2 valides sortent 0, les 6 invalides sortent ≠ 0.

> Ces fixtures pourront être ajustées quand le format sera gelé (T020/T021) ;
> elles servent dès maintenant de référence exécutable pour le parser.
