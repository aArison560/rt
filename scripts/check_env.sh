#!/bin/sh
# scripts/check_env.sh — vérifie l'environnement de travail du projet RT.
#
# Sortie : ✔ présent, ✖ manquant (obligatoire), ⚠ manquant (optionnel).
# Code retour : 0 si tous les outils **obligatoires** sont là, 1 sinon.
#
# Usage : sh scripts/check_env.sh
set -u

ok()  { printf '  ✔ %s\n' "$1"; }
ko()  { printf '  ✖ %s\n' "$1"; }
opt() { printf '  ⚠ %s\n' "$1"; }

mandatory_missing=0

# check_cmd <commande> <libellé> <obligatoire|optionnel>
check_cmd() {
	cmd=$1
	label=${2:-$1}
	kind=${3:-optionnel}
	if command -v "$cmd" >/dev/null 2>&1; then
		ok "$label"
	else
		if [ "$kind" = obligatoire ]; then
			ko "$label (MANQUANT)"
			mandatory_missing=1
		else
			opt "$label (optionnel, absent)"
		fi
	fi
}

# check_pkg <module pkg-config> <libellé> <obligatoire|optionnel>
check_pkg() {
	pkg=$1
	label=${2:-$1}
	kind=${3:-optionnel}
	if pkg-config --exists "$pkg" 2>/dev/null; then
		version=$(pkg-config --modversion "$pkg" 2>/dev/null)
		ok "$label $version"
	else
		if [ "$kind" = obligatoire ]; then
			ko "$label (MANQUANT)"
			mandatory_missing=1
		else
			opt "$label (optionnel, absent)"
		fi
	fi
}

echo "=== Compilateurs et build ==="
check_cmd c++     "c++"     obligatoire
check_cmd g++     "g++"     optionnel
check_cmd clang++ "clang++" optionnel
check_cmd make    "make"    obligatoire
printf '  ℹ %s\n' "c++ --version : $(c++ --version 2>/dev/null | head -n1)"

echo "=== Bibliothèques ==="
check_pkg sdl2   "SDL2"   optionnel
check_pkg libpng "libpng" optionnel
check_pkg libjpeg "libjpeg" optionnel

echo "=== Qualité / débogage ==="
check_cmd valgrind "valgrind" obligatoire
check_cmd gdb      "gdb"      optionnel

echo "=== Images ==="
# ImageMagick 6 expose 'convert'/'identify' ; la 7 expose aussi 'magick'.
if command -v magick >/dev/null 2>&1 || command -v convert >/dev/null 2>&1; then
	ok "ImageMagick (convert/magick)"
else
	ko "ImageMagick (MANQUANT)"
	mandatory_missing=1
fi
if command -v montage >/dev/null 2>&1; then
	ok "ImageMagick (montage — cluster)"
else
	opt "ImageMagick (montage — cluster, absent)"
fi
if command -v identify >/dev/null 2>&1; then
	ok "ImageMagick (identify — validation PNG)"
else
	opt "ImageMagick (identify, absent)"
fi

echo "=== Scripts et distribué ==="
check_cmd git    "git"    obligatoire
check_cmd rsync  "rsync"  optionnel
check_cmd ssh    "ssh"    optionnel
check_cmd docker "docker" optionnel

echo "=== Mesure et performance ==="
check_cmd hyperfine "hyperfine" optionnel
check_cmd perf      "perf"      optionnel
check_cmd ffmpeg    "ffmpeg"    optionnel

echo "=== Analyse statique et outillage éditeur ==="
check_cmd python3      "python3"      optionnel
check_cmd bear         "bear"         optionnel
check_cmd clang-format "clang-format" optionnel
check_cmd clang-tidy   "clang-tidy"   optionnel
check_cmd tmux         "tmux"         optionnel
check_cmd cppcheck     "cppcheck"     optionnel

echo
if [ "$mandatory_missing" -eq 0 ]; then
	printf 'Résultat : environnement prêt pour RT (les ⚠ sont optionnels).\n'
	exit 0
else
	printf 'Résultat : outils OBLIGATOIRES manquants — installe-les avant de continuer.\n'
	exit 1
fi
