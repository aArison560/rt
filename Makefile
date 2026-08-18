# Ray Tracer (RT) - Makefile
# C++23 project with SDL2, libpng, libjpeg + microui (C)

CXX := g++
CC := gcc
CXXFLAGS := -std=c++23 -Wall -Wextra -Werror -O2 -fPIC -march=native -pthread -g
CFLAGS := -std=c11 -Wall -Wextra -Werror -O2 -fPIC -march=native -pthread -g
CXXFLAGS_TEST := -std=c++23 -O2 -fPIC -pthread -g
CFLAGS_TEST := -std=c11 -O2 -fPIC -pthread -g

# microui: pure C, no external deps
CXXFLAGS += -I/usr/include/SDL2 -g
CFLAGS += -I/usr/include/SDL2 -Iinclude -g
CXXFLAGS_TEST += -I/usr/include/SDL2 -g
CFLAGS_TEST += -I/usr/include/SDL2 -Iinclude -g
LDFLAGS := -lSDL2 -lm -lpng -ljpeg -pthread -g

# Directories
SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj
OBJ_DIR_TEST := obj_test
BIN_DIR := .

# Executable name
TARGET := $(BIN_DIR)/rt
TARGET_TEST := $(BIN_DIR)/rt_test

# C++ source files
SOURCES := $(shell find $(SRC_DIR) -name '*.cpp' | sort)
MAIN_SRC := src/app/main.cpp
LIB_SRCS := $(filter-out $(MAIN_SRC), $(SOURCES))
TEST_SRCS := $(sort $(LIB_SRCS) $(shell find tests -name '*.cpp'))

# C source files (microui)
C_SOURCES := $(shell find $(SRC_DIR) -name '*.c' | sort)

# Object files
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES)) \
           $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(C_SOURCES))

OBJECTS_TEST := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR_TEST)/%.o, $(LIB_SRCS)) \
                $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR_TEST)/%.o, $(C_SOURCES)) \
                $(patsubst tests/%.cpp, $(OBJ_DIR_TEST)/%.o, $(filter tests/%.cpp, $(TEST_SRCS)))

# Progress tracking
TOTAL_OBJS := $(words $(OBJECTS))
TOTAL_TEST_OBJS := $(words $(OBJECTS_TEST))
COUNTER_FILE := $(OBJ_DIR)/.counter
COUNTER_FILE_TEST := $(OBJ_DIR_TEST)/.counter
$(shell rm -f $(COUNTER_FILE) $(COUNTER_FILE_TEST))
HEADERS := $(shell find $(INC_DIR) -name '*.hpp' -o -name '*.h' | sort)

# Include path
CXXFLAGS += -I$(INC_DIR)

# Main targets
.PHONY: all clean fclean re test retest

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@printf "\nLinked: $@\n"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@count=$$(cat $(COUNTER_FILE) 2>/dev/null || echo 0); \
	count=$$((count + 1)); \
	echo $$count > $(COUNTER_FILE); \
	pct=$$((count * 100 / $(TOTAL_OBJS))); \
	filled=$$((pct / 2)); \
	bar=""; i=0; while [ $$i -lt $$filled ]; do bar="$$bar#"; i=$$((i + 1)); done; \
	printf "\rCompilation [%-50s] %3d/%d (%3d%%)" "$$bar" $$count $(TOTAL_OBJS) $$pct

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@
	@count=$$(cat $(COUNTER_FILE) 2>/dev/null || echo 0); \
	count=$$((count + 1)); \
	echo $$count > $(COUNTER_FILE); \
	pct=$$((count * 100 / $(TOTAL_OBJS))); \
	filled=$$((pct / 2)); \
	bar=""; i=0; while [ $$i -lt $$filled ]; do bar="$$bar#"; i=$$((i + 1)); done; \
	printf "\rCompilation [%-50s] %3d/%d (%3d%%)" "$$bar" $$count $(TOTAL_OBJS) $$pct

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
	@printf "\nLinked (test): $@\n"

$(OBJ_DIR_TEST)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS_TEST) -I$(INC_DIR) -c $< -o $@
	@count=$$(cat $(COUNTER_FILE_TEST) 2>/dev/null || echo 0); \
	count=$$((count + 1)); \
	echo $$count > $(COUNTER_FILE_TEST); \
	pct=$$((count * 100 / $(TOTAL_TEST_OBJS))); \
	filled=$$((pct / 2)); \
	bar=""; i=0; while [ $$i -lt $$filled ]; do bar="$$bar#"; i=$$((i + 1)); done; \
	printf "\rCompilation test [%-50s] %3d/%d (%3d%%)" "$$bar" $$count $(TOTAL_TEST_OBJS) $$pct

$(OBJ_DIR_TEST)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS_TEST) -I$(INC_DIR) -c $< -o $@
	@count=$$(cat $(COUNTER_FILE_TEST) 2>/dev/null || echo 0); \
	count=$$((count + 1)); \
	echo $$count > $(COUNTER_FILE_TEST); \
	pct=$$((count * 100 / $(TOTAL_TEST_OBJS))); \
	filled=$$((pct / 2)); \
	bar=""; i=0; while [ $$i -lt $$filled ]; do bar="$$bar#"; i=$$((i + 1)); done; \
	printf "\rCompilation test [%-50s] %3d/%d (%3d%%)" "$$bar" $$count $(TOTAL_TEST_OBJS) $$pct

# Test sources from tests/ directory
$(OBJ_DIR_TEST)/%.o: tests/%.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS_TEST) -I$(INC_DIR) -c $< -o $@
	@count=$$(cat $(COUNTER_FILE_TEST) 2>/dev/null || echo 0); \
	count=$$((count + 1)); \
	echo $$count > $(COUNTER_FILE_TEST); \
	pct=$$((count * 100 / $(TOTAL_TEST_OBJS))); \
	filled=$$((pct / 2)); \
	bar=""; i=0; while [ $$i -lt $$filled ]; do bar="$$bar#"; i=$$((i + 1)); done; \
	printf "\rCompilation test [%-50s] %3d/%d (%3d%%)" "$$bar" $$count $(TOTAL_TEST_OBJS) $$pct

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
