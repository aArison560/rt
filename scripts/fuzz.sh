#!/bin/sh
# scripts/fuzz.sh — robustesse globale (T085).
# Genere ~200 fichiers `.rt` mutes aleatoirement, les execute en headless
# (parse + rendu 32x24, sans DISPLAY) et compte les codes retour inattendus
# (139 segfault, 134 abort, 124 timeout = ECHEC). Les codes 0 (rendu ok),
# 1 (erreur scene propre, `fichier:ligne:colonne`) et 2 (usage) sont attendus.
# Usage : sh scripts/fuzz.sh [./rt] [N=200] [graine]
# Passe un binaire ASan/UBSan (`make asan`) en $1 pour le DoD T085.
# DoD : 0 crash ; `valgrind` sur les scenes (voir docs/preuves/quality.md).
set -u

RT=${1:-./rt}
N=${2:-200}
SEED=${3:-424242}

if [ ! -x "$RT" ]; then
    printf 'fuzz: binaire introuvable : %s (lance make re)\n' "$RT" >&2
    exit 2
fi
case "$N" in
    ''|*[!0-9]*) printf 'fuzz: N invalide : %s\n' "$N" >&2; exit 2;;
esac

SRC=tests/cases/valid/minimal.rt
if [ ! -f "$SRC" ]; then
    printf 'fuzz: source introuvable : %s\n' "$SRC" >&2
    exit 2
fi

TMPDIR=${TMPDIR:-/tmp}/rt_fuzz_$$
mkdir -p "$TMPDIR" || exit 2
trap 'rm -rf "$TMPDIR"' EXIT INT TERM

SIZE=$(wc -c < "$SRC")

# Entier aleatoire dans [0, max[ via /dev/urandom (POSIX, pas de $RANDOM).
rand() {
    max=$1
    if [ "$max" -le 1 ]; then
        printf '0\n'
        return
    fi
    od -An -N4 -tu4 /dev/urandom 2>/dev/null | awk -v m="$max" '{print ($1 % m)}'
}

# Octets aleatoires (dont binaire/NUL possibles) de longueur $1.
randbytes() {
    head -c "$1" /dev/urandom 2>/dev/null
}

crash=0
ok_render=0
ok_reject=0
i=0
while [ "$i" -lt "$N" ]; do
    i=$((i + 1))
    f="$TMPDIR/fuzz_$i.rt"
    kind=$(rand 6)
    case "$kind" in
        0) # Troncation a une longueur aleatoire (0..SIZE).
            cut_at=$(rand "$((SIZE + 1))")
            head -c "$cut_at" "$SRC" > "$f"
            ;;
        1) # Suppression d'un segment aleatoire (tokens manquants).
            a=$(rand "$((SIZE + 1))")
            len=$(rand 24)
            b=$((a + len))
            head -c "$a" "$SRC" > "$f"
            tail -c "+$((b + 1))" "$SRC" >> "$f" 2>/dev/null || true
            ;;
        2) # Duplication d'un segment a une position aleatoire.
            a=$(rand "$((SIZE + 1))")
            len=$(rand 24)
            b=$((a + len))
            at=$(rand "$((SIZE + 1))")
            head -c "$at" "$SRC" > "$f"
            tail -c "+$((a + 1))" "$SRC" | head -c "$len" >> "$f" 2>/dev/null || true
            tail -c "+$((at + 1))" "$SRC" >> "$f" 2>/dev/null || true
            ;;
        3) # Remplacement d'octets par du binaire aleatoire.
            a=$(rand "$((SIZE + 1))")
            len=$(rand 16)
            b=$((a + len))
            head -c "$a" "$SRC" > "$f"
            randbytes "$len" >> "$f"
            tail -c "+$((b + 1))" "$SRC" >> "$f" 2>/dev/null || true
            ;;
        4) # Insertion de garbage (accolades, guillemets, NUL, UTF-8).
            at=$(rand "$((SIZE + 1))")
            head -c "$at" "$SRC" > "$f"
            randbytes "$(rand 16)" >> "$f"
            printf '{{{"""\\\x00\xff\xfe' >> "$f" 2>/dev/null || true
            tail -c "+$((at + 1))" "$SRC" >> "$f" 2>/dev/null || true
            ;;
        *) # Imbrication folle : enveloppe + suffixe d'accolades.
            {
                printf 'scene "fuzz" { group "g" { '
                cat "$SRC"
                printf ' }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}'
            } > "$f"
            ;;
    esac
    if timeout 10 "$RT" "$f" 32 24 --out "$TMPDIR/fuzz_$i.png" --quiet \
            >/dev/null 2>&1; then
        code=0
    else
        code=$?
    fi
    case "$code" in
        0) ok_render=$((ok_render + 1));;
        1|2) ok_reject=$((ok_reject + 1));;
        *)
            crash=$((crash + 1))
            printf '  CRASH %-24s kind=%s code=%s\n' "$f" "$kind" "$code"
            ;;
    esac
done

printf 'fuzz: %s fichiers (%s) : %s rendus, %s rejetes propres, %s crash/timeout\n' \
    "$N" "$RT" "$ok_render" "$ok_reject" "$crash"
if [ "$crash" -eq 0 ]; then
    printf 'fuzz: OK (0 crash)\n'
    exit 0
else
    printf 'fuzz: ECHEC (%s crash)\n' "$crash" >&2
    exit 1
fi
