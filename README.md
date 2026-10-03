# RT

Raytracer v2 — projet 42. Branche de travail : `dev` (`main` = archive v1, ne pas toucher).

## Build

```bash
make re      # compile ./rt
./rt         # affiche "rt <version>", code retour 0
make test    # tests (Catch2 à partir de T017)
make fclean  # nettoyage complet
```

Cibles qualité prévues : `asan`, `tsan`, `fast`, `compdb` (T002). Voir `docs/OUTILS.md` §8.
