#!/bin/sh
# scripts/install_sdl2_from_source.sh — repli quand SDL2 est absente du poste.
#
# Ordre de fonctionnement (cas "je change de PC") :
#   1. `pkg-config --exists sdl2` → rien à faire (cas courant, CI incluse).
#   2. `sudo apt install libsdl2-dev` → voie normale (1 min, voir docs/OUTILS.md §1.1).
#   3. Seulement sans sudo / sans apt : ce script clone SDL2 (branche SDL2) dans
#      `./SDL/src` (dossier NON versionné, voir .gitignore `/SDL/`) et l'installe
#      dans `./SDL/install` (surchargables via SDL2_SRC / SDL2_PREFIX).
#
# Usage :
#   sh scripts/install_sdl2_from_source.sh            # installe si sdl2 introuvable
#   sh scripts/install_sdl2_from_source.sh --force    # recompile même si présente
#   SDL2_PREFIX=/opt/sdl2 sh scripts/install_sdl2_from_source.sh
#
# Après installation, compiler avec la SDL2 locale :
#   export SDL2_PREFIX="$PWD/SDL/install"
#   export PKG_CONFIG_PATH="$SDL2_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH"
#   pkg-config --modversion sdl2
#
# Source : https://github.com/libsdl-org/SDL (branche SDL2).
set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
SRC=${SDL2_SRC:-$ROOT/SDL/src}
PREFIX=${SDL2_PREFIX:-$ROOT/SDL/install}
REPO=https://github.com/libsdl-org/SDL.git
BRANCH=SDL2
FORCE=0

for arg in "$@"; do
	case "$arg" in
		--force) FORCE=1 ;;
		-h | --help)
			sed -n '2,/^#$/p' "$0" | sed 's/^# \{0,1\}//'
			exit 0
			;;
		*)
			echo "option inconnue : $arg (voir --help)" >&2
			exit 2
			;;
	esac
done

if [ "$FORCE" -eq 0 ] && pkg-config --exists sdl2 2>/dev/null; then
	echo "SDL2 déjà présente : $(pkg-config --modversion sdl2 2>/dev/null) — rien à faire."
	exit 0
fi

for cmd in git cmake c++ make pkg-config; do
	if ! command -v "$cmd" >/dev/null 2>&1; then
		echo "manquant : $cmd — installe-le d'abord (ex. sudo apt install $cmd)." >&2
		exit 1
	fi
done

if [ -d "$SRC/.git" ]; then
	echo "mise à jour de $SRC ..."
	git -C "$SRC" fetch --depth 1 origin "$BRANCH" || exit 1
	git -C "$SRC" checkout "$BRANCH" || exit 1
else
	echo "clonage de $REPO (branche $BRANCH) dans $SRC ..."
	mkdir -p "$(dirname "$SRC")"
	git clone --depth 1 --branch "$BRANCH" "$REPO" "$SRC" || exit 1
fi

echo "compilation dans $SRC/build, installation dans $PREFIX ..."
cmake -S "$SRC" -B "$SRC/build" \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX="$PREFIX" || exit 1
cmake --build "$SRC/build" -j"$(nproc 2>/dev/null || echo 4)" || exit 1
cmake --install "$SRC/build" || exit 1

if PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:${PKG_CONFIG_PATH-}" pkg-config --exists sdl2; then
	echo "SDL2 installée : $(PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:${PKG_CONFIG_PATH-}" pkg-config --modversion sdl2)"
	echo "pour l'utiliser :"
	echo "  export SDL2_PREFIX=\"$PREFIX\""
	echo "  export PKG_CONFIG_PATH=\"$PREFIX/lib/pkgconfig:\$PKG_CONFIG_PATH\""
else
	echo "échec : sdl2.pc introuvable dans $PREFIX/lib/pkgconfig" >&2
	exit 1
fi
