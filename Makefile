# Ray Tracer (RT) - Makefile
# C++23 project with SDL2, libpng, libjpeg

CXX := g++
CXXFLAGS := -std=c++23 -Wall -Wextra -Werror -O2 -fPIC
LDFLAGS := -lSDL2 -lm -lpng -ljpeg

# Directories
SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj
BIN_DIR := .

# Executable name
TARGET := $(BIN_DIR)/rt

# Source files
SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
HEADERS := $(wildcard $(INC_DIR)/*.hpp)

# Include path
CXXFLAGS += -I$(INC_DIR)

# Main targets
.PHONY: all clean fclean re

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Linked: $@"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "Compiled: $<"

clean:
	rm -rf $(OBJ_DIR)
	@echo "Cleaned object files"

fclean: clean
	rm -f $(TARGET)
	@echo "Cleaned all builds"

re: fclean all

# Help target
.PHONY: help
help:
	@echo "Ray Tracer Build System"
	@echo "======================"
	@echo "Targets:"
	@echo "  make all        - Build the ray tracer executable"
	@echo "  make clean      - Remove object files"
	@echo "  make fclean     - Remove all build artifacts"
	@echo "  make re         - Clean rebuild"
	@echo "  make help       - Show this help message"
	@echo ""
	@echo "Usage: ./rt [scene_file] [width] [height]"
