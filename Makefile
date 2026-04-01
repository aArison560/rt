# Ray Tracer (RT) - Makefile
# C++23 project with SDL2, libpng, libjpeg

CXX := g++
CXXFLAGS := -std=c++23 -Wall -Wextra -Werror -O2 -fPIC
CXXFLAGS_TEST := -std=c++23 -O2 -fPIC
LDFLAGS := -lSDL2 -lm -lpng -ljpeg

# Directories
SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj
OBJ_DIR_TEST := obj_test
BIN_DIR := .

# Executable name
TARGET := $(BIN_DIR)/rt
TARGET_TEST := $(BIN_DIR)/rt_test

# Source files
SOURCES := $(shell find $(SRC_DIR) -name '*.cpp' | sort)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
OBJECTS_TEST := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR_TEST)/%.o, $(SOURCES))
HEADERS := $(shell find $(INC_DIR) -name '*.hpp' | sort)

# Include path
CXXFLAGS += -I$(INC_DIR)

# Main targets
.PHONY: all clean fclean re test retest

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Linked: $@"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "Compiled: $<"

clean:
	@rm -rf $(OBJ_DIR) $(OBJ_DIR_TEST)
	@echo "Cleaned object files"

fclean: clean
	@rm -f $(TARGET) $(TARGET_TEST)
	@echo "Cleaned all builds"

re: fclean all

retest: fclean test

test: $(TARGET_TEST)

$(TARGET_TEST): $(OBJECTS_TEST)
	@mkdir -p $(BIN_DIR)
	@$(CXX) $(CXXFLAGS_TEST) -o $@ $^ $(LDFLAGS)
	@echo "Linked (test): $@"

$(OBJ_DIR_TEST)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS_TEST) -I$(INC_DIR) -c $< -o $@
	@echo "Compiled (test): $<"

# Help target
.PHONY: help
help:
	@echo "Ray Tracer Build System"
	@echo "======================"
	@echo "Targets:"
	@echo "  make all        - Build the ray tracer executable"
	@echo "  make test       - Build without strict warning flags (-Wall -Wextra -Werror)"
	@echo "  make retest     - Clean rebuild test executable"
	@echo "  make clean      - Remove object files"
	@echo "  make fclean     - Remove all build artifacts"
	@echo "  make re         - Clean rebuild"
	@echo "  make help       - Show this help message"
	@echo ""
	@echo "Usage: ./rt [scene_file] [width] [height]"
