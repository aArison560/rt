NAME	= rt

CC		= c++
CXXFLAGS = -Wall -Wextra -Werror -O2 -std=c++2c

# Cibles qualité (docs/OUTILS.md §8) : chaque cible repart de zéro pour ne pas
# mélanger des objets compilés avec des jeux de flags différents.
ASANFLAGS = -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer
TSANFLAGS = -g -O0 -fsanitize=thread -fno-omit-frame-pointer
FASTFLAGS = -O3 -march=native

SRCDIR	= src
OBJDIR	= obj

# PNG (T034) : libpng systeme via pkg-config, vide si absente (fallback PPM).
# La compilation garde `-Wall -Wextra -Werror` : `<png.h>` est un header
# systeme (warnings neutralises), `stb` n'est pas vendored (ne passerait pas
# ces flags sans `-w` dedie, voir docs/OUTILS.md §1.1).
PNG_CFLAGS := $(shell pkg-config --cflags libpng 2>/dev/null)
PNG_LIBS   := $(shell pkg-config --libs libpng 2>/dev/null)

SRCS	= main.cpp Options.cpp Directives.cpp Lexer.cpp Scene.cpp Parser.cpp Validator.cpp Framebuffer.cpp Camera.cpp Renderer.cpp Material.cpp PointLight.cpp DirectionalLight.cpp SpotLight.cpp ImageWriter.cpp Object.cpp Sphere.cpp Plane.cpp Cylinder.cpp Cone.cpp
OBJS	= $(SRCS:%.cpp=$(OBJDIR)/%.o)

VPATH	= src/app src/base src/schema src/scene src/geometry src/shading \
	  src/lighting src/accel src/render src/sched src/io src/platform src/ui

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CXXFLAGS) $(LDFLAGS) $(OBJS) $(PNG_LIBS) -o $(NAME)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CXXFLAGS) $(PNG_CFLAGS) -Iinclude -c $< -o $@

# --- Tests (Catch2 vendored, T017) -----------------------------------------------
#
# `rt_test` : binaire de tests = amalgame Catch2 (fournit main) + tests/unit/*.cpp
# + tests/integration/*.cpp.
# Les calques `schema`/`scene`/`render` sont lies en objets (Directives, Lexer, Scene, Parser, Validator, Framebuffer).
# Les flags sont les mêmes que le projet (-Wall -Wextra -Werror exigés sur tests/).

CATCHDIR    = thirdparty/catch2
TESTDIR     = tests/unit
INTDIR      = tests/integration
TESTBIN     = rt_test
TEST_OBJDIR = obj-test

TEST_SRCS   = $(CATCHDIR)/catch_amalgamated.cpp $(wildcard $(TESTDIR)/*.cpp) $(wildcard $(INTDIR)/*.cpp) src/app/Options.cpp src/schema/Directives.cpp src/scene/Lexer.cpp src/scene/Scene.cpp src/scene/Parser.cpp src/scene/Validator.cpp src/render/Framebuffer.cpp src/render/Camera.cpp src/render/Renderer.cpp src/shading/Material.cpp src/lighting/PointLight.cpp src/lighting/DirectionalLight.cpp src/lighting/SpotLight.cpp src/io/ImageWriter.cpp src/geometry/Object.cpp src/geometry/Sphere.cpp src/geometry/Plane.cpp src/geometry/Cylinder.cpp src/geometry/Cone.cpp
TEST_OBJS   = $(TEST_SRCS:%.cpp=$(TEST_OBJDIR)/%.o)

$(TESTBIN): $(TEST_OBJS)
	$(CC) $(CXXFLAGS) $(LDFLAGS) $(TEST_OBJS) $(PNG_LIBS) -o $(TESTBIN)

$(TEST_OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CXXFLAGS) $(PNG_CFLAGS) -Iinclude -Ithirdparty -c $< -o $@

# Build + exécution ; le code retour de Catch2 (≠ 0 si échec) est propagé.
# Rejoue ensuite le jeu golden `tests/cases/` en headless (T027, < 10 s).
test: $(NAME) $(TESTBIN)
	./$(TESTBIN)
	sh scripts/run_cases.sh

# Tests sous ASan/UBSan : objets et binaire séparés pour ne pas mélanger les
# jeux de flags (même discipline que asan/tsan/fast).
test-asan:
	$(MAKE) test CXXFLAGS="$(CXXFLAGS) $(ASANFLAGS)" LDFLAGS="$(ASANFLAGS)" \
		TEST_OBJDIR=obj-test-asan TESTBIN=rt_test_asan

# Tests sous TSan : miroir exact de test-asan (objets et binaire dédiés).
test-tsan:
	$(MAKE) test CXXFLAGS="$(CXXFLAGS) $(TSANFLAGS)" LDFLAGS="$(TSANFLAGS)" \
		TEST_OBJDIR=obj-test-tsan TESTBIN=rt_test_tsan

# --- SDL2 (T070) : système par défaut, repli local si absente -----------------
#
# Fonctionnement principal (docs/OUTILS.md §1.1) : la SDL2 du système via
# pkg-config. Sur un poste sans SDL2 et sans apt, le repli consiste à compiler
# SDL2 depuis ses sources dans ./SDL/ (NON versionné, voir .gitignore) :
#   sh scripts/install_sdl2_from_source.sh
# puis compiler avec SDL2_PREFIX (les flags pkg-config suivent) :
#   make re SDL2_PREFIX=$PWD/SDL/install
# Tant que T070 (couche platform) n'est pas faite, ces variables sont sans
# effet sur le build : elles documentent le mécanisme à l'avance.

SDL2_PREFIX ?=
ifneq ($(strip $(SDL2_PREFIX)),)
export PKG_CONFIG_PATH := $(SDL2_PREFIX)/lib/pkgconfig:$(PKG_CONFIG_PATH)
endif

# Repli local : clone + compile SDL2 dans ./SDL/ (rien à committer).
setup-sdl:
	sh scripts/install_sdl2_from_source.sh

# --- Cibles de qualité -------------------------------------------------------

# Batterie complète (T018) : build, tests, ASan/UBSan, TSan, valgrind, résumé
# ✔/✖ ; code retour 0 uniquement si les 5 étapes sont vertes.
quality:
	sh scripts/quality.sh

asan:
	$(MAKE) re CXXFLAGS="$(CXXFLAGS) $(ASANFLAGS)" LDFLAGS="$(ASANFLAGS)"

tsan:
	$(MAKE) re CXXFLAGS="$(CXXFLAGS) $(TSANFLAGS)" LDFLAGS="$(TSANFLAGS)"

fast:
	$(MAKE) re CXXFLAGS="$(CXXFLAGS) $(FASTFLAGS)"

# compile_commands.json pour clangd via bear ; sans bear, message explicite.
compdb:
	@command -v bear >/dev/null 2>&1 || \
		{ echo "bear introuvable : installe-le (sudo apt install bear) ou utilise 'make compdb' sur une machine qui l'a."; exit 1; }
	bear --output compile_commands.json -- $(MAKE) re

# --- Formatage et analyse statique (T004) ------------------------------------

# Le nom nu (clang-format/clang-tidy) n'existe pas partout : on accepte aussi
# une version suffixée. Surcharge possible : make lint CLANG_TIDY=/chemin/vers.
CLANG_FORMAT ?= $(shell command -v clang-format 2>/dev/null || \
	command -v clang-format-19 2>/dev/null || command -v clang-format-18 2>/dev/null)
CLANG_TIDY ?= $(shell command -v clang-tidy 2>/dev/null || \
	command -v clang-tidy-19 2>/dev/null || command -v clang-tidy-18 2>/dev/null)

# Sources C++ du projet (implémentations + en-têtes + tests), ordre déterministe.
SOURCES := $(shell find $(SRCDIR) include tests -type f \
	\( -name '*.cpp' -o -name '*.hpp' \) 2>/dev/null | LC_ALL=C sort)

# Reformate en place ; idempotent : une seconde exécution ne change rien.
format:
	@command -v "$(CLANG_FORMAT)" >/dev/null 2>&1 || \
		{ echo "clang-format introuvable : sudo apt install clang-format-19 (ou make format CLANG_FORMAT=/chemin)"; exit 1; }
	@test -n "$(strip $(SOURCES))" || { echo "format : aucune source C++ à traiter"; exit 0; }
	$(CLANG_FORMAT) -i $(SOURCES)
	@echo "format : $(words $(SOURCES)) fichier(s) traités (2e passe = aucun changement)"

# Analyse statique ; tout diagnostic dans nos fichiers = échec (0 warning toléré).
lint:
	@command -v "$(CLANG_TIDY)" >/dev/null 2>&1 || \
		{ echo "clang-tidy introuvable : sudo apt install clang-tidy-19 (ou make lint CLANG_TIDY=/chemin)"; exit 1; }
	@test -n "$(strip $(SOURCES))" || { echo "lint : aucune source C++ à analyser"; exit 0; }
	@set -e; for f in $(SOURCES); do \
		$(CLANG_TIDY) --quiet --warnings-as-errors='*' $$f -- -Iinclude -std=c++23; \
	done
	@echo "lint : $(words $(SOURCES)) fichier(s) analysés, 0 diagnostic sur src/ include/ tests/"

clean:
	rm -rf $(OBJDIR)

fclean-test:
	rm -rf $(TEST_OBJDIR) obj-test-asan obj-test-tsan rt_test rt_test_asan rt_test_tsan

fclean: clean fclean-test
	rm -f $(NAME) compile_commands.json

re: fclean all

.PHONY: all clean fclean fclean-test re test test-asan test-tsan asan tsan fast compdb format lint quality setup-sdl
