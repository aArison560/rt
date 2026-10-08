#!/bin/sh
# scripts/run_cases.sh — jeu de scenes golden (T027).
# Rejoue `tests/cases/` en mode headless (aucun DISPLAY requis) :
# `valid/` doit sortir 0, `invalid/` doit sortir != 0 sans crash
# (139 segfault / 134 abort = echec). Toute anomalie fait echouer le script.
# Usage : sh scripts/run_cases.sh [./rt]
# Branche sur `make test` (DoD T027 : < 10 s).
set -u

RT=${1:-./rt}

fail=0
total=0

check_valid() {
    total=$((total + 1))
    if "$RT" "$1" >/dev/null 2>&1; then
        printf '  ok %s\n' "$1"
    else
        printf '  FAIL %s (attendu 0, obtenu %s)\n' "$1" "$?"
        fail=1
    fi
}

check_invalid() {
    total=$((total + 1))
    "$RT" "$1" >/dev/null 2>&1
    code=$?
    if [ "$code" -eq 139 ] || [ "$code" -eq 134 ]; then
        printf '  CRASH %s (code %s)\n' "$1" "$code"
        fail=1
    elif [ "$code" -ne 0 ]; then
        printf '  ok %s (rejete %s)\n' "$1" "$code"
    else
        printf '  FAIL %s (attendu != 0, obtenu 0)\n' "$1"
        fail=1
    fi
}

if [ ! -x "$RT" ]; then
    printf 'run_cases: binaire introuvable : %s (lance make re)\n' "$RT" >&2
    exit 2
fi

for f in tests/cases/valid/*.rt; do
    check_valid "$f"
done

for f in tests/cases/invalid/*.rt; do
    check_invalid "$f"
done

# Porte headless (T035, DoD) : `./rt <scene> --out` sans DISPLAY -> 0.
# Aucune initialisation SDL dans ce chemin (R6) ; echec = ECHEC global.
total=$((total + 1))
HEADLESS_OUT=/tmp/rt_run_cases_headless.png
rm -f "$HEADLESS_OUT"
if env -u DISPLAY "$RT" scenes/default.rt 64 64 --out "$HEADLESS_OUT" >/dev/null 2>&1; then
    if [ -s "$HEADLESS_OUT" ]; then
        printf '  ok headless sans DISPLAY (env -u DISPLAY ./rt scenes/default.rt 64 64 --out %s)\n' "$HEADLESS_OUT"
    else
        printf '  FAIL headless : image vide %s\n' "$HEADLESS_OUT"
        fail=1
    fi
else
    printf '  FAIL headless sans DISPLAY (code %s)\n' "$?"
    fail=1
fi
rm -f "$HEADLESS_OUT"

if [ "$fail" -eq 0 ]; then
    printf 'run_cases: %s fichiers OK\n' "$total"
    exit 0
else
    printf 'run_cases: ECHEC\n' >&2
    exit 1
fi
