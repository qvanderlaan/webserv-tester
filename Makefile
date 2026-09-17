# ========= Variables ========= #

NAME			=	tester

SRC_DIR			=	src/
OBJ_DIR			=	build/
INCLUDE_DIR		=	include/

CPP				=	c++
CPP_FLAGS		=	-Wall -Werror -Wextra -std=c++23

# ========= Register Files ========= #

SRC_FILES		=	$(shell find $(SRC_DIR) -name "*.cpp")
OBJ_FILES		=	$(patsubst $(SRC_DIR)%.cpp, $(OBJ_DIR)%.o, $(SRC_FILES))

# ========= Register Functions ========= #

all: $(NAME)

$(NAME): $(OBJ_FILES)
	$(CPP) $(CPP_FLAGS) $(LDFLAGS) -I $(INCLUDE_DIR) $^ -o $@

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp Makefile
	@mkdir -p $(dir $@)
	$(CPP) $(CPP_FLAGS) -I $(INCLUDE_DIR) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

# ========= Docker Functions ========= #

docker:
	docker compose run --rm make

docker-re:
	docker compose run --rm make re

.PHONY: all clean fclean re docker docker-re
