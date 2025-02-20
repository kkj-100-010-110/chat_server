NAME = server
CC = gcc
CFLAGS = -Werror -Wall -Wextra -g
RM = rm -rf

INCLUDES = -I./include
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include

SRCS = $(SRC_DIR)/main.c \
	   $(SRC_DIR)/rb.c \
	   $(SRC_DIR)/user.c \
	   $(SRC_DIR)/queue.c \
	   $(SRC_DIR)/thread_pool.c \
	   $(SRC_DIR)/err_log.c \
	   $(SRC_DIR)/command.c \
	   $(SRC_DIR)/server.c

OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

VALGRIND_FLAGS = --leak-check=full --show-leak-kinds=all -v

all		: $(OBJ_DIR) $(NAME)
$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(NAME) : $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(NAME) $(OBJS)
clean	:
	$(RM) $(OBJ_DIR)/*
fclean	: clean
	$(RM) $(OBJ_DIR) $(NAME)
re		: fclean all

valgrind:
	  valgrind $(VALGRIND_FLAGS) ./server

.PHONY	:	all clean fclean re valgrind
