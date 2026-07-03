NAME        := ft_traceroute

FILES		:=	\
				packet.c \
				error.c  \
				utils.c  \
				init.c   \
				output.c \
				main.c

SRCS_PATH	:=	src
SRCS		:=	$(addprefix $(SRCS_PATH)/,$(FILES))

INC     	:=	-I.

OBJS_PATH	:=	obj
OBJS		:=	$(addprefix $(OBJS_PATH)/,$(FILES:.c=.o))
DEPS		:=	$(OBJS:.o=.d)

CC			:=	gcc
CFLAGS		:=	-Wall -Wextra -Werror -g3

DOCKER		:=	docker
DOCKER_IMAGE	:=	ft_traceroute
DOCKER_WORKDIR	:=	/workspace
DOCKER_RUN_OPTS	:=	--rm -it --cap-add=NET_RAW

.PHONY: all clean fclean re debug docker-build docker-make docker-shell

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -lm -o $@
	@command -v setcap >/dev/null 2>&1 && setcap cap_net_raw+ep $(NAME) || true

-include $(DEPS)

$(OBJS_PATH)/%.o: $(SRCS_PATH)/%.c | $(OBJS_PATH)
	$(CC) $(CFLAGS) $(INC) -MMD -MP -c $< -o $@

$(OBJS_PATH):
	mkdir $@

clean:
	rm -r $(OBJS_PATH)

fclean: clean
	rm -f $(NAME) $(SHORTNAME)

re: fclean all

debug: CFLAGS += -DDEBUG
debug: re

docker-build:
	$(DOCKER) build -t $(DOCKER_IMAGE) .

docker-make: docker-build
	$(DOCKER) run $(DOCKER_RUN_OPTS) -v "$(CURDIR):$(DOCKER_WORKDIR)" -w $(DOCKER_WORKDIR) $(DOCKER_IMAGE) sh -lc "make"

docker-shell: docker-build
	$(DOCKER) run $(DOCKER_RUN_OPTS) -v "$(CURDIR):$(DOCKER_WORKDIR)" -w $(DOCKER_WORKDIR) $(DOCKER_IMAGE) bash
