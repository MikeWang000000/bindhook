CC = /usr/bin/gcc
STRIP = /usr/bin/strip
CFLAGS = -O3 -std=c99 -pedantic -Wall -Wextra
LDFLAGS =
LIB_CFLAGS = $(CFLAGS) -fPIC
LIB_LDFLAGS = $(LDFLAGS) -shared -nostartfiles -nostdlib -nodefaultlibs

all: libbindhook.so bindhook

clean:
	$(RM) libbindhook.so bindhook

libbindhook.so: libbindhook.c syscall_asm.h $(arch/*/syscall_arch.h)
	$(CC) $< $(LIB_CFLAGS) $(LIB_LDFLAGS) -o $@
	$(STRIP) $@

bindhook: bindhook.c
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@
	$(STRIP) $@
