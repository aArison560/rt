#!/bin/sh
# scripts/quality.sh — batterie de qualité complète du projet RT (T018).
#
# Étapes, dans l'ordre du Prompt T018 :
#   1. make re                  build release (-Wall -Wextra -Werror -O2)
#   2. make test                tests unitaires Catch2
#   3. make asan + make test-asan   (« make asan test »)
#   4. make tsan + make test-tsan   (« make tsan test »)
#   5. make re puis valgrind --leak-check=full --error-exitcode=1 ./rt --version
#
# Écarts assumés avec la lettre du Prompt (documentés dans docs/OUTILS.md §3) :
#   * « make asan test » exécuté tel quel relancerait `./rt_test` **non
#     instrumenté** : le sous-make de `asan` vide obj-test/ (fclean), puis le
#     parent recompile les tests avec les flags par défaut. On exécute donc les
#     tests réellement instrumentés via `make test-asan` / `make test-tsan`.
#   * valgrind est incompatible avec un binaire ASan/TSan : l'étape 5
#     reconstruit `./rt` en flags normaux avant de l'analyser.
#
# Sortie : une ligne ✔/✖ par étape, puis le résumé (rapport = commit/journal).
# Code retour : 0 si les 5 étapes passent, 1 sinon (utilisable tel quel en CI).
#
# Usage : sh scripts/quality.sh   —   ou : make quality
set -u

cd "$(dirname "$0")/.." || exit 1

summary=""
failures=0
steps=0

# step <libellé affiché> <commande...>
step() {
	label=$1
	shift
	steps=$((steps + 1))
	printf '\n=== Étape %d — %s ===\n' "$steps" "$label"
	printf '$ %s\n' "$*"
	if "$@"; then
		summary="${summary}
  ✔ ${label}"
	else
		rc=$?
		summary="${summary}
  ✖ ${label} (code retour ${rc})"
		failures=$((failures + 1))
	fi
}

# « make asan test » : build + tests sous ASan/UBSan.
step_asan() {
	make asan && make test-asan
}

# « make tsan test » : build + tests sous TSan.
step_tsan() {
	make tsan && make test-tsan
}

# valgrind doit voir un binaire en flags normaux : on repart d'un make re.
step_valgrind() {
	command -v valgrind >/dev/null 2>&1 || {
		echo "valgrind introuvable — voir sh scripts/check_env.sh"
		return 127
	}
	make re && valgrind --leak-check=full --error-exitcode=1 ./rt --version
}

printf 'RT — batterie de qualité (%s)\n' "$(date '+%Y-%m-%d %H:%M')"

step "make re — build release 0 warning" make re
step "make test — tests unitaires" make test
step "make asan test — build + tests ASan/UBSan" step_asan
step "make tsan test — build + tests TSan" step_tsan
step "valgrind --leak-check=full --error-exitcode=1 ./rt --version" step_valgrind

printf '\n=== Résumé ===\n'
printf '%s\n' "$summary" | sed '/^$/d'

if [ "$failures" -eq 0 ]; then
	printf '\nRésultat : ✔ %d/%d étapes vertes.\n' "$steps" "$steps"
	exit 0
fi

printf '\nRésultat : ✖ %d étape(s) en échec sur %d.\n' "$failures" "$steps"
exit 1
