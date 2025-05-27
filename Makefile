# General Setup
NAME = ircserv
CC = c++
FLAGS = -Wall -Wextra -Werror -std=c++98 -I$(HEADERS_DIR)
RM = rm -rf

# Files
FILES = Server IRCMessage Client
HEADERS = Allowed.hpp Server.hpp IRCMessage.hpp Client.hpp
SRCS_DIR = srcs
HEADERS_DIR = include
SRCS = main.cpp $(addprefix $(SRCS_DIR)/, $(FILES:=.cpp))
OBJDIR = .objs
OBJS = $(OBJDIR)/main.o $(addprefix $(OBJDIR)/, $(FILES:=.o))
HEADER_FILES = $(addprefix $(HEADERS_DIR)/, $(HEADERS))

# Colors
GREEN		=	\e[92;5;118m
YELLOW		=	\e[93;5;226m
ORANGE		=	\e[33;5;202m
RED			=	\e[31;5;196m
RESET		=	\e[0m

# Rules
all: $(NAME)

$(NAME): $(OBJS) $(HEADER_FILES)
	@$(CC) $(FLAGS) $(OBJS) -o $(NAME)
	@printf "$(GREEN) $(NAME) $(RESET) has been created.\n"

$(OBJDIR)/main.o: main.cpp $(HEADER_FILES)
	@mkdir -p $(OBJDIR)
	@$(CC) $(FLAGS) -c $< -o $@
	@printf "$(YELLOW) Compiling: $(RESET) $< \n"

$(OBJDIR)/%.o: $(SRCS_DIR)/%.cpp $(HEADER_FILES)
	@mkdir -p $(OBJDIR)
	@$(CC) $(FLAGS) -c $< -o $@
	@printf "$(YELLOW) Compiling: $(RESET) $< \n"

clean:
	@$(RM) $(OBJDIR)
	@printf "$(ORANGE) Object files have been removed. \n"

fclean: clean
	@$(RM) $(NAME)
	@printf "$(RED) $(NAME) have been removed. \n"

re: fclean all

cleanly: all clean

.PHONY: all clean fclean re cleanly