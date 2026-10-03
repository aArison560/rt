# ============================================================
# RTv1 - Makefile C++ POO (.hpp/.cpp) avec SDL local
# ============================================================

CXX = g++
NAME = bin/rtv1

SDL_VERSION = 2.30.8
SDL_TAR = SDL2-$(SDL_VERSION).tar.gz
SDL_URL = https://github.com/libsdl-org/SDL/releases/download/release-$(SDL_VERSION)/$(SDL_TAR)
SDL_SRC = localSDL/src/SDL2-$(SDL_VERSION)
SDL_ROOT = localSDL
SDL_INC  = $(SDL_ROOT)/include
SDL_LIB  = $(SDL_ROOT)/lib
SDL_SO   = $(SDL_LIB)/libSDL2.so

CXXFLAGS = -Wall -std=c++17 -pthread -Iinclude -I$(SDL_INC) -I$(SDL_INC)/SDL2 -D_REENTRANT -O3 -march=native -ffast-math -DNDEBUG
LDFLAGS  = -L$(SDL_LIB) -lSDL2 -pthread -Wl,-rpath,'$$ORIGIN/../localSDL/lib' -Wl,-rpath,'$$ORIGIN/localSDL/lib' -Wl,-rpath,$(abspath $(SDL_LIB)) -O3 

# Sources POO
SRC = src/math/Vec3.cpp src/math/Ray.cpp \
      src/core/Object.cpp src/core/Quadric.cpp src/core/Scene.cpp src/core/Renderer.cpp src/core/Texture.cpp \
      src/parser/SceneParser.cpp \
      src/ui/sdl/sdl_init.cpp src/ui/sdl/sdl_draw.cpp src/ui/sdl/sdl_events.cpp \
      src/main.cpp src/ui/qt_or_gtk/ui_window.cpp

OBJ = $(SRC:src/%.cpp=obj/%.o)
TOTAL = $(words $(OBJ))

GREEN  = \033[32m
CYAN   = \033[36m
YELLOW = \033[33m
RESET  = \033[0m

all: $(SDL_SO) $(NAME)
	@printf "$(GREEN)[100%%]$(RESET) Built $(NAME) C++ POO (SDL $(SDL_VERSION))\n"

$(SDL_SO):
	@mkdir -p localSDL/src
	@if [ ! -f localSDL/src/$(SDL_TAR) ]; then \
		printf "$(YELLOW)[ SDL ] Téléchargement SDL2 $(SDL_VERSION)...$(RESET)\n"; \
		wget -q --show-progress -O localSDL/src/$(SDL_TAR) $(SDL_URL) || curl -L -o localSDL/src/$(SDL_TAR) $(SDL_URL); \
	fi
	@if [ ! -d $(SDL_SRC) ]; then \
		printf "$(YELLOW)[ SDL ] Extraction...$(RESET)\n"; \
		tar -xzf localSDL/src/$(SDL_TAR) -C localSDL/src; \
	fi
	@printf "$(YELLOW)[ SDL ] Build SDL local...$(RESET)\n"
	@mkdir -p $(SDL_SRC)/build
	@cd $(SDL_SRC)/build && ../configure --prefix=$(abspath $(SDL_ROOT)) --disable-dependency-tracking > /dev/null && $(MAKE) -j$$(nproc) > /dev/null && $(MAKE) install > /dev/null
	@printf "$(GREEN)[ SDL ] SDL prête$(RESET)\n"

$(NAME): $(OBJ) | $(SDL_SO)
	@mkdir -p $(dir $@)
	@printf "\n$(YELLOW)[100%%] Linking $(NAME)$(RESET)\n"
	@$(CXX) $(OBJ) -o $@ $(LDFLAGS)

HEADERS = $(shell find include -name '*.hpp' -o -name '*.h')
obj/%.o: src/%.cpp $(HEADERS) | $(SDL_SO)
	@mkdir -p $(dir $@)
	@COUNT=$$(find obj -type f -name "*.o" 2>/dev/null | wc -l); \
	NEXT=$$((COUNT+1)); PERCENT=$$((NEXT*100/$(TOTAL))); if [ $$PERCENT -gt 100 ]; then PERCENT=100; fi; \
	FILLED=$$((PERCENT/5)); EMPTY=$$((20-FILLED)); \
	BAR=$$(printf "%$${FILLED}s" | tr ' ' '#'); EMPTY_BAR=$$(printf "%$${EMPTY}s" | tr ' ' '-'); \
	printf "\r\033[K$(CYAN)[%3d%%]$(RESET) [$(GREEN)%s$(RESET)%s] CXX %s" "$$PERCENT" "$$BAR" "$$EMPTY_BAR" "$<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

SCENE ?= scenes/cyl.rt
# Permet `make run foo.rt` ou `make run scenes/foo.rt` : .rt extrait de MAKECMDGOALS
RT_ARG := $(filter %.rt,$(MAKECMDGOALS))
ifneq ($(RT_ARG),)
ifeq ($(findstring /,$(RT_ARG)),)
override SCENE := scenes/$(RT_ARG)
else
override SCENE := $(RT_ARG)
endif
endif
# `SCENE=foo.rt` sans dossier -> résolu vers scenes/
ifeq ($(findstring /,$(SCENE)),)
override SCENE := scenes/$(SCENE)
endif

run: all
	@./$(NAME) $(SCENE)

# Cible factice pour `make run foo.rt` / `make run scenes/foo.rt` (évite "No rule to make target")
%.rt:
	@:

leak_test: all
	@valgrind --show-leak-kinds=all ./$(NAME) scenes/exemple.rt --once

sdl_clean:
	@rm -rf localSDL/src/SDL2-$(SDL_VERSION) localSDL/src/build

clean:
	@rm -rf obj
	@printf "Clean done\n"

fclean: clean
	@rm -f $(NAME)
	@printf "Fclean done\n"

sdl_fclean: fclean sdl_clean
	@rm -rf localSDL/lib/* localSDL/include/SDL2 localSDL/bin localSDL/share
	@printf "SDL fclean done\n"

re: fclean all

.PHONY: all clean fclean sdl_clean sdl_fclean re run leak_test
