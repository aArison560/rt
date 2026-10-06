#!/bin/sh
# scripts/bench.sh — harnais de benchmark du projet RT (T019).
#
# Mesure N exécutions d'une commande (par défaut : le rendu d'une scène), calcule
# la moyenne et l'écart-type, et écrit la mesure en Markdown + JSON dans
# docs/BENCH.md. Fonctionne **sans hyperfine** (DoD) : fallback `date +%s.%N`.
#
# Usage :
#   sh scripts/bench.sh <scene.rt> [--runs N] [--warmup N] [--label L]
#                       [--note "texte"] [--args "800 600 --out /tmp/x.png"]
#   sh scripts/bench.sh --cmd "./rt --version" [options...]
#
# Options :
#   --runs N      exécutions chronométrées (défaut : 5, minimum 1)
#   --warmup N    exécutions d'échauffement non comptabilisées (défaut : 1)
#   --label L     nom de la mesure ; une mesure déjà présente avec le même label
#                 est **remplacée** (comparaison avant/après propre)
#   --note TXT    précision écrite dans la mesure (contexte, hypothèses)
#   --args "..."  arguments supplémentaires passés à ./rt après la scène
#                 (découpage en mots simple : pas de métacaractère de shell)
#   --cmd "..."   mesure une commande arbitraire au lieu d'un rendu
#   -h, --help    cette aide
#
# Sortie standard : une ligne par exécution, puis le résumé chiffré.
# Code retour : 0 mesure écrite · 2 erreur d'usage · sinon code de la commande
#               mesurée (docs/BENCH.md a alors été créé quand même).
#
# Variable d'environnement : RT_BENCH_DOC (défaut `docs/BENCH.md`) : sert à tester
# le harnais ailleurs que dans le dépôt.

set -u

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
cd "$ROOT_DIR" || exit 1

BENCH_DOC=${RT_BENCH_DOC:-docs/BENCH.md}
BT='`'

RUNS=5
WARMUP=1
LABEL=""
NOTE=""
CMD=""
SCENE=""
EXTRA_ARGS=""

die() {
	printf 'bench : %s\n' "$*" >&2
	exit 2
}

usage() {
	awk 'NR == 1 { next } /^set -u/ { exit } { sub(/^# ?/, ""); print }' "$0"
}

is_uint() {
	case "$1" in
	'' | *[!0-9]*) return 1 ;;
	*) return 0 ;;
	esac
}

# --- Analyse des arguments --------------------------------------------------

while [ $# -gt 0 ]; do
	case "$1" in
	--runs | -r)
		[ $# -ge 2 ] || die "--runs attend une valeur"
		RUNS=$2
		shift 2
		;;
	--warmup | -w)
		[ $# -ge 2 ] || die "--warmup attend une valeur"
		WARMUP=$2
		shift 2
		;;
	--label | -l)
		[ $# -ge 2 ] || die "--label attend une valeur"
		LABEL=$2
		shift 2
		;;
	--note | -n)
		[ $# -ge 2 ] || die "--note attend une valeur"
		NOTE=$2
		shift 2
		;;
	--args | -a)
		[ $# -ge 2 ] || die "--args attend une valeur"
		EXTRA_ARGS=$2
		shift 2
		;;
	--cmd | -c)
		[ $# -ge 2 ] || die "--cmd attend une valeur"
		CMD=$2
		shift 2
		;;
	-h | --help)
		usage
		exit 0
		;;
	-*)
		die "option inconnue : $1 (voir --help)"
		;;
	*)
		[ -z "$SCENE" ] || die "une seule scène attendue (reçu : $SCENE et $1)"
		SCENE=$1
		shift
		;;
	esac
done

is_uint "$RUNS" || die "--runs doit être un entier (reçu : $RUNS)"
[ "$RUNS" -ge 1 ] || die "--runs doit être >= 1"
is_uint "$WARMUP" || die "--warmup doit être un entier (reçu : $WARMUP)"

# --- Commande mesurée -------------------------------------------------------

if [ -n "$CMD" ]; then
	[ -z "$SCENE" ] || die "soit <scene.rt>, soit --cmd, pas les deux"
	[ -n "$LABEL" ] || LABEL="commande"
else
	[ -n "$SCENE" ] || {
		usage >&2
		die "une scène (--help) ou --cmd est requis"
	}
	[ -f "$SCENE" ] || die "fichier introuvable : $SCENE"
	[ -x ./rt ] || die "./rt introuvable : lancez 'make re' d'abord"
	CMD="./rt $SCENE"
	[ -z "$EXTRA_ARGS" ] || CMD="$CMD $EXTRA_ARGS"
	[ -n "$LABEL" ] || {
		LABEL=$(basename "$SCENE")
		LABEL=${LABEL%.rt}
	}
fi

# Label : pas d'espace de bord (il sert de repère de section dans le document).
LABEL=$(printf '%s' "$LABEL" | sed 's/^[ 	]*//; s/[ 	]*$//')
[ -n "$LABEL" ] || die "label vide"
case "$LABEL" in *'
'*) die "label sur plusieurs lignes non supporté" ;; esac

# --- Document de sortie : en-tête + méthode, créé avant toute mesure --------

create_doc() {
	mkdir -p "$(dirname "$BENCH_DOC")" || die "impossible de créer $(dirname "$BENCH_DOC")"
	cat > "$BENCH_DOC" <<'DOC'
# RT — Benchmarks

> Document **produit et complété par `scripts/bench.sh`** (T019) : chaque mesure est
> générée par le script, jamais saisie à la main (règle du sujet : preuve
> régénérable). Les mesures vivent sous `## Résultats`, la méthode ci-dessous est
> fixe.

## Méthode

- **Harnais** : `sh scripts/bench.sh <scene.rt> [--runs N] [--warmup N] [--label L]
  [--note "..."] [--args "..."]`, ou `--cmd "<commande>"` pour mesurer autre chose
  qu'un rendu. Le script se place toujours à la racine du dépôt.
- **Chronométrage** : `hyperfine` s'il est installé (son JSON natif est exporté tel
  quel), sinon `date +%s.%N` autour de chaque exécution, en dernier recours une
  horloge `python3`. **Le harnais tourne sans hyperfine** : c'est le cas de ce poste
  et de la CI.
- **Protocole** : `--warmup` exécutions d'échauffement non comptabilisées (défaut 1)
  puis `--runs` exécutions mesurées (défaut 5). La sortie de la commande est avalée ;
  en cas d'échec elle est affichée et son code retour est propagé.
- **Statistiques** : moyenne arithmétique et **écart-type échantillon** (dénominateur
  n−1, mise à jour par la formule de Welford, `0` pour n = 1), plus min, max et
  `CV = écart-type / moyenne × 100`. Toutes les valeurs sont en **secondes**.
- **Format d'une mesure** : section `### <label> — <horodatage>` contenant la
  commande exacte, le protocole, un tableau Markdown **et** le détail brut en JSON
  (bloc fenced) : rien n'est recopié à la main.
- **Comparaison avant/après** : rejouer avec le **même `--label`** remplace la
  section homonyme ; deux labels différents coexistent pour comparer deux variantes.
- **Honnêteté** : la ligne *Commande* donne la commande réellement mesurée et la
  ligne *Note* le contexte (par ex. binaire lancé avant que le rendu n'existe) : une
  mesure n'est un temps de rendu que si la commande rend.

## Résultats

_Aucune mesure pour l'instant : `sh scripts/bench.sh <scene.rt>` l'écrit ici._
DOC
}

[ -f "$BENCH_DOC" ] || create_doc

# --- Horloge à décimales ----------------------------------------------------

CLOCK="date +%s.%N"
probe=$(date +%s.%N 2>/dev/null || true)
if ! printf '%s' "$probe" | grep -Eq '^[0-9]+\.[0-9]+$'; then
	CLOCK="python3 perf_counter"
	probe=$(python3 -c 'import time; print("%.9f" % time.perf_counter())' 2>/dev/null || true)
	printf '%s' "$probe" | grep -Eq '^[0-9]+\.[0-9]+$' ||
		die "aucune horloge à décimales (date +%s.%N ni python3) : installez hyperfine"
fi

now() {
	case "$CLOCK" in
	python3*) python3 -c 'import time; print("%.9f" % time.perf_counter())' ;;
	*) date +%s.%N ;;
	esac
}

# --- Exécution --------------------------------------------------------------

TMP_DIR=$(mktemp -d "${TMPDIR:-/tmp}/rt-bench.XXXXXX") || die "mktemp a échoué"
trap 'rm -rf "$TMP_DIR"' EXIT INT TERM
TIMES="$TMP_DIR/times.txt"
OUT="$TMP_DIR/out.log"
STAMP=$(date '+%Y-%m-%d %H:%M:%S %Z' 2>/dev/null || date '+%Y-%m-%d %H:%M:%S')
ISO=$(date -Iseconds 2>/dev/null || date '+%Y-%m-%dT%H:%M:%S%z')
: > "$TIMES"

TOOL=""
MEAN=""
SD=""
MINV=""
MAXV=""
N=0
JSON=""
HF_JSON=""

# Échec de la commande mesurée : on affiche sa sortie et on sort avec son code.
fail_run() {
	printf 'bench : commande en échec (code %s) — %s\n' "$2" "$1" >&2
	sed 's/^/  | /' "$OUT" >&2
	exit "$2"
}

run_manual() {
	TOOL="$CLOCK"
	total=$((WARMUP + RUNS))
	i=1
	while [ "$i" -le "$total" ]; do
		if [ "$i" -le "$WARMUP" ]; then
			phase="échauffement $i/$WARMUP"
		else
			phase="run $((i - WARMUP))/$RUNS"
		fi
		t0=$(now)
		# Découpage en mots simple (documenté) : pas de shell interposé.
		$CMD >"$OUT" 2>&1
		rc=$?
		t1=$(now)
		[ "$rc" -eq 0 ] || fail_run "$phase" "$rc"
		d=$(awk -v a="$t0" -v b="$t1" 'BEGIN { printf "%.6f", b - a }')
		if [ "$i" -gt "$WARMUP" ]; then
			printf '%s\n' "$d" >> "$TIMES"
			printf '  %-18s %s s\n' "$phase" "$d"
		else
			printf '  %-18s (non comptabilisé)\n' "$phase"
		fi
		i=$((i + 1))
	done
	N=$RUNS
}

run_hyperfine() {
	TOOL="hyperfine"
	HF_JSON="$TMP_DIR/hyperfine.json"
	set -- --runs "$RUNS" --export-json "$HF_JSON"
	[ "$WARMUP" -eq 0 ] || set -- "$@" --warmup "$WARMUP"
	printf '  hyperfine : %s exécution(s), commande : %s\n' "$RUNS" "$CMD"
	hyperfine "$@" "$CMD" || fail_run "hyperfine" "$?"

	# Première occurrence de chaque clé = premier (seul) résultat.
	hf_get() {
		grep -o "\"$1\"[ 	]*:[ 	]*[-0-9.eE+]*" "$HF_JSON" |
			head -n 1 | sed 's/.*:[ 	]*//'
	}
	MEAN=$(hf_get mean)
	SD=$(hf_get stddev)
	MINV=$(hf_get min)
	MAXV=$(hf_get max)
	N=$RUNS
}

stats_from_times() {
	# Moyenne + écart-type échantillon (n−1) par la formule de Welford, min, max.
	set -- $(awk '
		{
			NN++
			delta = $1 - mean
			mean += delta / NN
			m2 += delta * ($1 - mean)
			if (NN == 1) { mn = $1; mx = $1 } else { if ($1 < mn) mn = $1; if ($1 > mx) mx = $1 }
		}
		END {
			if (NN == 0) { print "0 0 0 0 0"; exit }
			sd = (NN > 1) ? sqrt(m2 / (NN - 1)) : 0
			printf "%d %.6f %.6f %.6f %.6f\n", NN, mean, sd, mn, mx
		}' "$TIMES")
	N=$1
	MEAN=$2
	SD=$3
	MINV=$4
	MAXV=$5
}

json_escape() {
	printf '%s' "$1" | sed 's/\\/\\\\/g; s/"/\\"/g'
}

build_json() {
	# Arguments : $1 = liste des durées (vide si hyperfine), $2 = outil.
	times=$1
	if [ -z "$times" ] && [ -n "$HF_JSON" ] && [ -f "$HF_JSON" ]; then
		if command -v python3 >/dev/null 2>&1; then
			times=$(python3 -c 'import json,sys; r=json.load(open(sys.argv[1]))["results"][0]; print(json.dumps(r.get("times", []), separators=(",", ":")))' \
				"$HF_JSON" 2>/dev/null || true)
		else
			# Pas de python3 : le JSON d'hyperfine est sur plusieurs lignes,
			# on le replie avant d'extraire le tableau « times ».
			times=$(tr -d '\n' < "$HF_JSON" 2>/dev/null |
				grep -o '"times"[^]]*]' | head -n 1 | sed 's/^"times"[^[]*//' || true)
		fi
		[ -n "$times" ] || times="[]"
	fi
	JSON=$(printf '{"command":"%s","label":"%s","n":%s,"warmup":%s,"tool":"%s","clock":"%s","unit":"s","mean":%s,"stddev":%s,"min":%s,"max":%s,"times":%s,"timestamp":"%s"}' \
		"$(json_escape "$CMD")" "$(json_escape "$LABEL")" "$N" "$WARMUP" \
		"$(json_escape "$2")" "$(json_escape "$CLOCK")" \
		"$MEAN" "$SD" "$MINV" "$MAXV" "$times" "$ISO")
}

if command -v hyperfine >/dev/null 2>&1; then
	run_hyperfine
	build_json "" "hyperfine"
else
	run_manual
	stats_from_times
	times=$(awk 'BEGIN { printf "[" } { printf "%s%s", (NR > 1 ? "," : ""), $1 } END { printf "]" }' "$TIMES")
	build_json "$times" "$CLOCK"
fi

[ "$N" -gt 0 ] || die "aucune mesure enregistrée"
for v in "$MEAN" "$SD" "$MINV" "$MAXV"; do
	printf '%s' "$v" | grep -Eq '^[0-9]+(\.[0-9]+)?$' ||
		die "statistique illisible ('$v') : extraction impossible, rien n'est écrit"
done
CV=$(awk -v sd="$SD" -v m="$MEAN" 'BEGIN { printf "%.2f", (m != 0) ? sd / m * 100 : 0 }')

# --- Écriture dans docs/BENCH.md -------------------------------------------

BLOCK="$TMP_DIR/block.md"
{
	printf '### %s — %s\n\n' "$LABEL" "$STAMP"
	printf '%s\n' "- **Commande** : $BT$CMD$BT"
	[ -z "$NOTE" ] || printf '%s\n' "- **Note** : $NOTE"
	printf '%s\n' "- **Protocole** : $RUNS exécution(s) mesurée(s), $WARMUP échauffement(s), outil $BT$TOOL$BT, horloge $BT$CLOCK$BT"
	printf '%s\n' "- **Machine** : $(uname -srm 2>/dev/null || echo inconnu) · $(hostname 2>/dev/null || echo hôte inconnu)"
	if command -v git >/dev/null 2>&1 && git rev-parse --short HEAD >/dev/null 2>&1; then
		printf '%s\n' "- **Commit au moment de la mesure** : $BT$(git rev-parse --short HEAD)$BT"
	fi
	printf '\n'
	printf '%s\n' '| runs | moyenne (s) | écart-type (s) | min (s) | max (s) | CV (%) |'
	printf '%s\n' '|---:|---:|---:|---:|---:|---:|'
	printf '| %s | %s | %s | %s | %s | %s |\n' "$N" "$MEAN" "$SD" "$MINV" "$MAXV" "$CV"
	printf '\n%s\n' "Détail brut :"
	printf '\n%s\n' '```json'
	printf '%s\n' "$JSON"
	printf '%s\n' '```'
} > "$BLOCK"

# 1. retire une éventuelle section portant le même label (mesure remplacée) :
#    de « ### <label> » jusqu'au prochain titre, ce qui évite d'accumuler des
#    lignes blanches à chaque rejeu.
TMP_DOC="$TMP_DIR/doc.md"
awk -v label="$LABEL" '
	{
		pfx = "### " label " "
		if (skip) { if (substr($0, 1, 1) == "#") skip = 0; else next }
		if (substr($0, 1, length(pfx)) == pfx) { skip = 1; next }
		print
	}
' "$BENCH_DOC" > "$TMP_DOC"

# 2. insère la nouvelle section juste après « ## Résultats » (mesure la plus
#    récente en tête) et retire le rappel « aucune mesure ».
awk -v block="$BLOCK" '
	index($0, "Aucune mesure pour") > 1 { next }
	{ print }
	index($0, "## Résultats") == 1 && !done {
		while ((getline line < block) > 0) print line
		close(block)
		done = 1
	}
	END { if (!done) { while ((getline line < block) > 0) print line } }
' "$TMP_DOC" > "$BENCH_DOC.tmp" || die "écriture de $BENCH_DOC impossible"
mv "$BENCH_DOC.tmp" "$BENCH_DOC"

# --- Résumé -----------------------------------------------------------------

printf '\nMesure %s écrite dans %s\n' "$LABEL" "$BENCH_DOC"
printf '  commande  : %s\n' "$CMD"
printf '  runs      : %s (+ %s échauffement(s)) — outil : %s\n' "$RUNS" "$WARMUP" "$TOOL"
printf '  moyenne   : %s s\n' "$MEAN"
printf '  écart-type: %s s (CV %s %%)\n' "$SD" "$CV"
printf '  min / max : %s s / %s s\n' "$MINV" "$MAXV"
exit 0
