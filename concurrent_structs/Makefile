CC = clang
CFLAGS = -Wall -Werror -Wextra -pedantic

all: queue.o rwlock.o

queue.o: queue.c
	$(CC) $(CFLAGS) -c queue.c

rwlock.o: rwlock.c
	$(CC) $(CFLAGS) -c rwlock.c

clean:
	rm -f queue.o rwlock.o

format:
	clang-format -i -style=file queue.c rwlock.c queue.h rwlock.h
