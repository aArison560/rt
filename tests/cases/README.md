# tests/cases — fixtures de scènes `.rt`

Fixtures du futur parser (T022+). Elles suivent le **format cible** spécifié en
T020 : blocs imbriqués `{ }`, commentaires `#`, nombres, vecteurs `(x y z)`.

| Fichier | Rôle |
|---------|------|
| `valid/minimal.rt` | scène minimale valide (1 sphère, 1 lumière) |
| `valid/group.rt` | scène à groupes + 2 lumières |
| `invalid/missing_brace.rt` | accolade fermante manquante |
| `invalid/unknown_directive.rt` | directive inconnue |
| `invalid/bad_number.rt` | nombre mal formé |

Les fichiers `invalid/` doivent être **rejetés** par le parser avec une erreur
localisée `fichier:ligne:colonne` et un code retour ≠ 0, **sans crash**
(vérifié sous ASan). Les fichiers `valid/` doivent être acceptés.

> Ces fixtures pourront être ajustées quand le format sera gelé (T020/T021) ;
> elles servent dès maintenant de référence exécutable pour le parser.
