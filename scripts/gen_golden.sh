#!/bin/sh
# scripts/gen_golden.sh — regenere les goldens T059 (non-regression visuelle).
# Rend les 3 scenes de reference en 80x60 spp 4 (resolution du test
# `tests/integration/test_golden.cpp`, rapide pour `make test`) et depose les
# tampons RGBA bruts dans `tests/golden/*.rgba` + `tests/golden/hashes.txt`
# (sha256, pour l'audit et la CI T116). Les PNG de preuve pleine resolution
# restent generes par `scripts/render_all.sh` dans `docs/preuves/`.
# Usage : sh scripts/gen_golden.sh [./rt_test]
# DoD T059 : `make test` compare chaque golden avec tolerance (5 pixels) ;
# une regression volontaire (couleur, lumiere) fait echouer le test.
set -u

cd "$(dirname "$0")/.." || exit 1

TESTBIN=${1:-./rt_test}

if [ ! -x "$TESTBIN" ]; then
    printf 'gen_golden: binaire de tests introuvable : %s (lance make re && make test)\n' "$TESTBIN" >&2
    exit 2
fi

mkdir -p tests/golden

if ! UPDATE_GOLDEN=1 "$TESTBIN" "[golden]" 2>&1 | tail -n 20; then
    printf 'gen_golden: ECHEC de la regeneration (UPDATE_GOLDEN=1 %s "[golden]")\n' "$TESTBIN" >&2
    exit 1
fi

if command -v sha256sum >/dev/null 2>&1; then
    sha256sum tests/golden/*.rgba > tests/golden/hashes.txt
    printf 'gen_golden: %s\n' "tests/golden/hashes.txt"
    cat tests/golden/hashes.txt
else
    printf 'gen_golden: sha256sum introuvable, hashes.txt non regenere\n' >&2
    exit 1
fi

printf 'gen_golden: OK (%s)\n' "$(ls tests/golden/*.rgba | tr '\n' ' ')"
exit 0
