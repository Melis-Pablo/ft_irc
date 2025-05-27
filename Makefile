# General Setup
NAME = ircserv
CC = c++
FLAGS = -Wall -Wextra -Werror -std=c++98 -I$(HEADERS_DIR)
RM = rm -rf

# Files
FILES = main Server IRCMessage Client Channel
HEADERS = Allowed Server IRCMessage Client Channel

# Directories
SRCS_DIR = srcs
HEADERS_DIR = include
OBJDIR = .objs

# Auto-generated paths
SRCS = $(addprefix $(SRCS_DIR)/, $(addsuffix .cpp, $(FILES)))
OBJS = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(FILES)))
HEADER_FILES = $(addprefix $(HEADERS_DIR)/, $(addsuffix .hpp, $(HEADERS)))

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