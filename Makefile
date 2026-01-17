CC      = gcc
CFLAGS  = -Wall -Wextra

SRCS    = server.c http.c charStream.c
OBJS    = $(SRCS:.c=.o)

server: $(OBJS) testServer.o
	$(CC) $(CFLAGS) -o server.out $(OBJS) testServer.o

test: $(OBJS) test.o
	$(CC) $(CFLAGS) -o test.out $(OBJS) test.o

clean:
	rm -f *.o
	rm -f *.out

