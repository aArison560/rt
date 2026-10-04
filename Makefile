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

SRCS	= main.cpp
OBJS	= $(SRCS:%.cpp=$(OBJDIR)/%.o)

VPATH	= src/app src/base src/schema src/scene src/geometry src/shading \
	  src/lighting src/accel src/render src/sched src/io src/platform src/ui

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CXXFLAGS) $(LDFLAGS) $(OBJS) -o $(NAME)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CXXFLAGS) -Iinclude -c $< -o $@

test: $(NAME)
	@echo "No tests yet (Catch2 integrated in T017)"

# --- Cibles de qualité -------------------------------------------------------

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

fclean: clean
	rm -f $(NAME) compile_commands.json

re: fclean all

.PHONY: all clean fclean re test asan tsan fast compdb format lint
