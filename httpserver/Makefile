CC = clang
CFLAGS = -Wall -Wpedantic -Werror -Wextra

all: httpserver

httpserver: httpserver.o
	$(CC) -o httpserver httpserver.o asgn2_helper_funcs.a

httpserver.o: httpserver.c
	$(CC) $(CFLAGS) -c httpserver.c

clean:
	rm -f httpserver httpserver.o asgn2_helper_funcs.o

format:
	clang-format -i -style=file httpserver.c
