# RT — Preuves visuelles (régénérables)

> **Règle du sujet** : aucune image ne vaut preuve à elle seule. Toute image
> de ce dossier est **régénérable par script** depuis les scènes versionnées —
> jamais fabriquée à la main, jamais unique.

## Régénérer

```bash
sh scripts/render_all.sh   # 9/9 images en ~1 s, sortie 0 si tout rend
```

Le script supprime d'abord les anciens `*.png` (**depuis zéro**), rend chaque
scène en **mode headless** (`./rt <scene> --out <png> --quiet`, aucun `DISPLAY`
requis, T035), puis affiche le résumé (`ok/total`, durée). Tout échec → sortie 1.

## Contenu (T037)

| Image | Scène source | Résolution (limites de la scène) |
|-------|--------------|----------------------------------|
| `default.png` | `scenes/default.rt` | 320×240, spp 4 |
| `minimal.png` | `tests/cases/valid/minimal.rt` | 640×480, spp 4 |
| `group.png` | `tests/cases/valid/group.rt` | scène à groupes (File++) |
| `material.png` | `tests/cases/valid/material.rt` | matériaux |
| `transform.rt` → `transform.png` | `tests/cases/valid/transform.rt` | transformations |
| `lights.png` | `tests/cases/valid/lights.rt` | point + directionnel + spot |
| `limits.png` | `tests/cases/valid/limits.rt` | limites + fond + ambiance |
| `primitives.png` | `tests/cases/valid/primitives.rt` | 4 primitives |
| `alias.png` | `tests/cases/valid/alias.rt` | alias + têtes |

Toutes les images sont **versionnées** (DoD T037) mais restent secondaires :
la preuve, c'est le script + la scène, pas le PNG.

> **Note P3** : aucune primitive n'est encore rendue (miss → fond, P4/T040+),
> donc les scènes partageant même `background` + résolution + `seed`
> produisent des PNG octet-identiques (`alias`/`lights`/`primitives`/
> `transform`). C'est attendu et stable (régénération identique vérifiée par
> `sha256sum`) ; P4 différenciera les images via la géométrie.

## Suite

T083 étendra `render_all.sh` aux 3 scènes obligatoires (`fig_vi1/vi2/vi3`)
puis aux scènes d'options (P9–P11). Le protocole reste identique : une
commande, headless, résumé, zéro image manuelle.
