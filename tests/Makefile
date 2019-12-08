CFLAGS = -Wall -Werror -std=gnu99 -I../src
LDFLAGS = -lpthread ../src/libx.a

TARGETS=test_quebuf test_xsocket_server test_xsocket_client

all: $(TARGETS)

clean:
	rm -f *.o
	rm -f $(TARGETS)

.PHONY: all clean

test_quebuf: test_quebuf.o
	$(CC) -o $@ $< $(LDFLAGS)

test_xsocket_server: test_xsocket_server.o
	$(CC) -o $@ $< $(LDFLAGS)

test_xsocket_client: test_xsocket_client.o
	$(CC) -o $@ $< $(LDFLAGS)
