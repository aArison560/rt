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

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME) compile_commands.json

re: fclean all

.PHONY: all clean fclean re test asan tsan fast compdb
