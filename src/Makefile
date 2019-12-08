PREFIX?=/usr/local
INCLUDE_PATH=include
LIBRARY_PATH=lib
BINARY_PATH=bin
INSTALL_INCLUDE_PATH= $(PREFIX)/$(INCLUDE_PATH)
INSTALL_LIBRARY_PATH= $(PREFIX)/$(LIBRARY_PATH)
INSTALL_BINARY_PATH= $(PREFIX)/$(BINARY_PATH)

LIBX_OBJS = $(patsubst %.c, %.o, $(wildcard *.c))
STATICLIB = libx.a
SHAREDLIB = libx.so
SHAREDLIB_MAJOR = libx.so.0
SHAREDLIB_MINOR = libx.so.0.1

CFLAGS = -Wall -Werror -std=gnu99
LDFLAGS = -shared -soname $(SHAREDLIB_MINOR)

# for darwin
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifeq ($(uname_S),Darwin)
  SHAREDLIB = libx.dylib
  SHAREDLIB_MAJOR = libx.0.dylib
  SHAREDLIB_MINOR = libx.0.1.dylib
  LDFLAGS = -lc -dylib -install_name $(INSTALL_LIBRARY_PATH)/$(SHAREDLIB_MINOR) -macosx_version_min 10.14
endif

all: $(STATICLIB) $(SHAREDLIB)

$(STATICLIB): $(LIBX_OBJS)
	$(AR) -rcs $@ $^

$(SHAREDLIB): $(LIBX_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -fPIC -o $@ $<

clean:
	rm -f *.o
	rm -f $(STATICLIB) $(SHAREDLIB)

install: all
	mkdir -p $(INSTALL_LIBRARY_PATH) $(INSTALL_INCLUDE_PATH)/libx
	cp -a *.h $(INSTALL_INCLUDE_PATH)/libx
	cp -a $(STATICLIB) $(INSTALL_LIBRARY_PATH)
	cp -a $(SHAREDLIB) $(INSTALL_LIBRARY_PATH)/$(SHAREDLIB_MINOR)
	ln -sf $(SHAREDLIB_MINOR) $(INSTALL_LIBRARY_PATH)/$(SHAREDLIB_MAJOR)
	ln -sf $(SHAREDLIB_MAJOR) $(INSTALL_LIBRARY_PATH)/$(SHAREDLIB)

uninstall:
	rm -rf $(INSTALL_INCLUDE_PATH)/libx
	rm -f $(INSTALL_LIBRARY_PATH)/$(STATICLIB)
	rm -f $(INSTALL_LIBRARY_PATH)/$(SHAREDLIB)
	rm -f $(INSTALL_LIBRARY_PATH)/$(SHAREDLIB_MAJOR)
	rm -f $(INSTALL_LIBRARY_PATH)/$(SHAREDLIB_MINOR)

.PHONY: all clean install uninstall

quebuf.o: quebuf.c quebuf.h
xsocket.o: xsocket.c xsocket.h quebuf.h list.h showmsg.h
packet_parser.o: packet_parser.c packet_parser.h
plugin.o: plugin.c plugin.h list.h
showmsg.o: showmsg.c showmsg.h
