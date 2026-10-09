# tests/golden — goldens de non-regression visuelle (T059)

Images de reference des 3 scenes lumiere (`fig_vi1_base.rt`, `fig_vi1.rt`,
`fig_vi3.rt`) rendues en 80x60 spp 4 seed 0 (`maxDepth` de la scene).
Format `.rgba` : tampon d'affichage brut RGBA8 (`width*height*4` octets,
ordre `displayData()`, `y = 0` en haut), 19200 o par image.

| Fichier | Scene | Taille | Hash (sha256) |
|---------|-------|--------|---------------|
| `fig_vi1_base.rgba` | `scenes/fig_vi1_base.rt` | 80x60 spp 4 | voir `hashes.txt` |
| `fig_vi1.rgba` | `scenes/fig_vi1.rt` | 80x60 spp 4 | voir `hashes.txt` |
| `fig_vi3.rgba` | `scenes/fig_vi3.rt` | 80x60 spp 4 | voir `hashes.txt` |

Comparaison a chaque `make test` (`tests/integration/test_golden.cpp`) :
ecart par canal <= 2 ignore (arrondi gamma), au-dela au plus 5 pixels
(0.1 %) peuvent differer. Une vraie regression (couleur, lumiere,
geometrie : 358 pixels pour un albedo change, verifie en T059) fait echouer.

Regenerer (apres un changement volontaire de scene) :

```bash
sh scripts/gen_golden.sh   # UPDATE_GOLDEN=1 ./rt_test "[golden]" + sha256sum
```

Ne jamais regenerer pour faire passer un test qui echoue sans comprendre
la cause : le golden fige le comportement, il ne le valide pas.
