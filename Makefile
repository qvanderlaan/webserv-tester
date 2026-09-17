# ========= Variables ========= #

NAME			=	tester

SRC_DIR			=	src/
OBJ_DIR			=	build/
INCLUDE_DIR		=	include/

CPP				=	c++
CPP_FLAGS		=	-Wall -Werror -Wextra -std=c++23

# ========= Register Files ========= #

SOURCE_CPPFILES	=	HttpClient.cpp HttpResponse.cpp main.cpp ServerInstance.cpp TestException.cpp TestRegistry.cpp TestRunner.cpp \
					tests/return_200_for_root_request/main.cpp

CPPFILES		=	$(SOURCE_CPPFILES)

SRC_FILES		=	$(addprefix $(SRC_DIR), $(CPPFILES))
OBJ_FILES		=	$(addprefix $(OBJ_DIR), $(CPPFILES:.cpp=.o))

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
