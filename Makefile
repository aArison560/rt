NAME	= rt

CC		= c++
CXXFLAGS = -Wall -Wextra -Werror -O2 -std=c++2c

SRCDIR	= src
OBJDIR	= obj

SRCS	= main.cpp
OBJS	= $(SRCS:%.cpp=$(OBJDIR)/%.o)

VPATH	= src/app src/base src/schema src/scene src/geometry src/shading \
	  src/lighting src/accel src/render src/sched src/io src/platform src/ui

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CXXFLAGS) -Iinclude -c $< -o $@

test: $(NAME)
	@echo "No tests yet (Catch2 integrated in T017)"

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re test
