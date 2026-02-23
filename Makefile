CFLAGS = -std=gnu11 -Wall -Wextra -Werror -D_GNU_SOURCE

all: clean resize

clean:
	$(RM) resize
