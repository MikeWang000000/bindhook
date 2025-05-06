CC = /usr/bin/clang
STRIP = /usr/bin/strip
CFLAGS = -mmacosx-version-min=10.6 -O3 -std=c99 -pedantic -Wall -Wextra
LDFLAGS = -mmacosx-version-min=10.6
LIB_CFLAGS = $(CFLAGS) -fPIC
LIB_LDFLAGS = $(LDFLAGS) -dynamiclib

all: libbindhook.dylib bindhook

clean:
	$(RM) libbindhook.dylib bindhook

libbindhook.dylib: libbindhook.c
	$(CC) $< $(LIB_CFLAGS) $(LIB_LDFLAGS) -o $@
	$(STRIP) -S $@

bindhook: bindhook.c
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@
	$(STRIP) -S $@
