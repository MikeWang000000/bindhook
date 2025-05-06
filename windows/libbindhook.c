#include <winsock2.h>
#include <windows.h>

#include <detours.h>

static int (WINAPI *bind_sys)(SOCKET s, const struct sockaddr *, int);

static int WINAPI bind__bindhook_(SOCKET s, const struct sockaddr *addr,
                                  int namelen)
{
    int zero = 0, one = 1;

    setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (PVOID) &zero, sizeof(zero));
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (PVOID) &one, sizeof(one));

    return bind_sys(s, addr, namelen);
}


BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    (void) hinst;
    (void) reserved;

    if (reason == DLL_PROCESS_ATTACH)
    {
        bind_sys = &bind;
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((PVOID *) &bind_sys, (PVOID) bind__bindhook_);
        DetourTransactionCommit();
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach((PVOID *) &bind_sys, (PVOID) bind__bindhook_);
        DetourTransactionCommit();
    }
    return TRUE;
}
