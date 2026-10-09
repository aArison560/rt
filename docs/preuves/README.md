# RT — Preuves visuelles (régénérables)

> **Règle du sujet** : aucune image ne vaut preuve à elle seule. Toute image
> de ce dossier est **régénérable par script** depuis les scènes versionnées —
> jamais fabriquée à la main, jamais unique.

## Régénérer

```bash
sh scripts/render_all.sh   # 18/18 images en ~9 s, sortie 0 si tout rend
```

Le script supprime d'abord les anciens `*.png` (**depuis zéro**), rend chaque
scène en **mode headless** (`./rt <scene> --out <png> --quiet`, aucun `DISPLAY`
requis, T035), puis affiche le résumé (`ok/total`, durée). Tout échec → sortie 1.

## Contenu (T037)

| Image | Scène source | Résolution (limites de la scène) |
|-------|--------------|----------------------------------|
| `default.png` | `scenes/default.rt` | 320×240, spp 4 |
| `fig_vi1_base.png` | `scenes/fig_vi1_base.rt` | 320×240, spp 4 (M3/M4, T048) |
| `fig_vi1.png` | `scenes/fig_vi1.rt` | 320×240, spp 4 (4 types, 2 spots, speculaire, T080, figure VI.1) |
| `fig_vi2.png` | `scenes/fig_vi2.rt` | 320×240, spp 4 (copie de fig_vi1, seule `camera` differe, T081) |
| `fig_vi3.png` | `scenes/fig_vi3.rt` | 320×240, spp 4 (melange d'ombres 2 spots, T082, figure VI.3) |
| `opt_parallel.png` | `scenes/opt_parallel.rt` | 320×240, spp 4 (soleil directionnel, T055) |
| `opt_glass.png` | `scenes/opt_glass.rt` | 320×240, spp 4 (verre `transparency 0.9 ior 1.5` + opaque, T057) |
| `opt_transparent_shadow.png` | `scenes/opt_transparent_shadow.rt` | 320×240, spp 4 (ombre opaque noire + ombre verre `0.8/1.5` éclaircie, T058) |
| `opt_direct.png` | `scenes/opt_direct.rt` | 320×240, spp 4 (spot face caméra, centre saturé 254, T058) |
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

## Obligatoires en une commande (T083)

`sh scripts/render_all.sh` rend les 3 scenes obligatoires (`fig_vi1`,
`fig_vi2`, `fig_vi3`) + toutes les scenes d'options + les fixtures valides,
en headless, en ~9 s (< 2 min, DoD T083), et depose tout dans `docs/preuves/`
avec le resume `ok/total` + duree. Mesure du 2026-10-09 : **18/18 en 9 s**.

## Suite

P9–P11 ajouteront leurs scenes d'options a `render_all.sh`
(`scenes/opt_*.rt`). Le protocole reste identique : une
commande, headless, résumé, zéro image manuelle.
