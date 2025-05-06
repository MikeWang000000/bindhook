CC=cl.exe
RM=del /S /Q
RMDIR=rmdir /S /Q

TRIPLET=x64-windows

CFLAGS=/std:c11 /O2 /W4
INCLUDES=/I vcpkg_installed/$(TRIPLET)/include/detours
LIBPATH=/LIBPATH:vcpkg_installed/$(TRIPLET)/lib
LIBS=detours.lib ws2_32.lib

all: libbindhook.dll bindhook.exe

depinst:
	vcpkg install --triplet $(TRIPLET)

libbindhook.dll: libbindhook.c
	$(CC) $? /LD $(CFLAGS) $(INCLUDES) /link $(LIBPATH) $(LIBS) /DLL /OUT:$@

bindhook.exe: bindhook.c
	$(CC) $? $(CFLAGS) /link /OUT:$@

clean:
	$(RM) *.obj libbindhook.dll bindhook.exe

realclean: clean
	$(RMDIR) vcpkg_installed
