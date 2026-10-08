#!/bin/sh
# scripts/render_all.sh — premiere preuve generee par script (T037).
# Rend une liste de scenes en mode headless et depose les images dans
# `docs/preuves/` avec un nom explicite (`<basename>.png`).
# Aucun DISPLAY requis (T035, R6) : `./rt --out` ne touche jamais a SDL.
# Usage : sh scripts/render_all.sh [./rt]
# DoD T037 : regenere tout depuis zero (anciens PNG supprimes d'abord),
# sortie 0 si tout rend, 1 sinon ; le resultat est versionne (PNG + README).
set -u

RT=${1:-./rt}

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
cd "$ROOT_DIR" || exit 1

OUTDIR=docs/preuves

if [ ! -x "$RT" ]; then
    printf 'render_all: binaire introuvable : %s (lance make re)\n' "$RT" >&2
    exit 2
fi

mkdir -p "$OUTDIR"

# Liste extensible (T083 y ajoutera les 3 obligatoires + options) :
# toutes les scenes `scenes/` plus les fixtures valides du parser.
# shellcheck disable=SC2086
SCENES=""
for f in scenes/*.rt; do
    [ -e "$f" ] || continue
    SCENES="$SCENES $f"
done
for f in tests/cases/valid/*.rt; do
    [ -e "$f" ] || continue
    SCENES="$SCENES $f"
done

if [ -z "$SCENES" ]; then
    printf 'render_all: aucune scene trouvee (scenes/*.rt, tests/cases/valid/*.rt)\n' >&2
    exit 2
fi

# Depuis zero : supprime les PNG generes (garde README + .gitkeep).
# shellcheck disable=SC2086
rm -f $OUTDIR/*.png

fail=0
total=0
ok=0
start=$(date +%s)

for scene in $SCENES; do
    base=$(basename "$scene" .rt)
    out="$OUTDIR/$base.png"
    total=$((total + 1))
    if "$RT" "$scene" --out "$out" --quiet >/dev/null 2>&1; then
        if [ -s "$out" ]; then
            printf '  ok %-28s -> %s\n' "$scene" "$out"
            ok=$((ok + 1))
        else
            printf '  FAIL %-28s (image vide %s)\n' "$scene" "$out"
            fail=1
        fi
    else
        printf '  FAIL %-28s (code %s)\n' "$scene" "$?"
        fail=1
    fi
done

end=$(date +%s)
dur=$((end - start))

printf 'render_all: %s/%s images OK en %ss -> %s/\n' "$ok" "$total" "$dur" "$OUTDIR"
if [ "$fail" -eq 0 ]; then
    exit 0
else
    printf 'render_all: ECHEC\n' >&2
    exit 1
fi
