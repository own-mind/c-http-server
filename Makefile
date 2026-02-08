CC       = gcc
CFLAGS   = -g -O0 -fsanitize=address -Wall -Wextra -Wno-unused-parameter
LDFLAGS  = -fsanitize=address -lm

SRCS     = server.c http.c charStream.c examples/qrcode/qrcode.c
OBJS     = $(SRCS:.c=.o)

server: $(OBJS) testServer.o
	$(CC) $(OBJS) testServer.o -o server.out $(LDFLAGS)

test: $(OBJS) test.o
	$(CC) $(OBJS) test.o -o test.out $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) testServer.o test.o *.out
