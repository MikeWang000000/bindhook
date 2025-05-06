#include <stdio.h>
#include <string.h>
#include <windows.h>

#define ENV_BUFSIZ 8192
#define PATH_BUFSIZ 1024
#define LIBNAME "libbindhook.dll"

#ifdef _MSC_VER
#   define strtok_r strtok_s
#   define strdup _strdup
#endif /* _MSC_VER */

static char errmsg[4096];

static void print_usage(char *name)
{
    fprintf(stderr, "Usage: %s <executable> [args...]\n", name);
}


static void save_err(void)
{
    DWORD error_id = GetLastError();
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, error_id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   errmsg, sizeof(errmsg), NULL);
}


static int fexists(const char *path)
{
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}


static char *abspath(const char *path)
{
    char *abspath_buf;
    DWORD res;

    abspath_buf = malloc(PATH_BUFSIZ);
    if (!abspath_buf) {
        return NULL;
    }

    res = GetFullPathNameA(path, PATH_BUFSIZ, abspath_buf, NULL);
    if (res == 0 || res >= PATH_BUFSIZ) {
        free(abspath_buf);
        return NULL;
    }

    return abspath_buf;
}


static char *get_child_commandline(char *arg0)
{
    char *main_cmdline = GetCommandLineA();
    char *ptr;
    size_t space_cnt = 0;

    if (!main_cmdline) {
        return NULL;
    }

    for (ptr = arg0; *ptr; ptr++) {
        if (*ptr == ' ') {
            space_cnt++;
        }
    }

    space_cnt++;
    for (ptr = main_cmdline; *ptr; ptr++) {
        if (*ptr == ' ') {
            space_cnt--;
        }
        if (!space_cnt) {
            break;
        }
    }
    while (*ptr == ' ') {
        ptr++;
    }

    return ptr;
}


static char *find_libbindhook(char *name)
{
    int res;
    char selfpath[PATH_BUFSIZ], selfdir[PATH_BUFSIZ], libpath[PATH_BUFSIZ];
    LPSTR ptr;

    if (!GetModuleFileNameA(NULL, selfpath, PATH_BUFSIZ)) {
        return NULL;
    }

    if (!GetFullPathNameA(selfpath, PATH_BUFSIZ, selfdir, &ptr)) {
        return NULL;
    }
    *ptr = '\0';

    res = snprintf(libpath, sizeof(libpath), "%s\\..\\lib\\%s", selfdir, name);
    if (res < 0 || (size_t) res >= sizeof(libpath)) {
        return NULL;
    }

    if (fexists(libpath)) {
        return abspath(libpath);
    }

    res = snprintf(libpath, sizeof(libpath), "%s\\lib\\%s", selfdir, name);
    if (res < 0 || (size_t) res >= sizeof(libpath)) {
        return NULL;
    }
    if (fexists(libpath)) {
        return abspath(libpath);
    }

    res = snprintf(libpath, sizeof(libpath), "%s\\%s", selfdir, name);
    if (res < 0 || (size_t) res >= sizeof(libpath)) {
        return NULL;
    }
    if (fexists(libpath)) {
        return abspath(libpath);
    }

    return NULL;
}


static char *find_executable(char *name)
{
    int res;
    DWORD cnt;
    char *pathdup, *token, *tokctx, execpath[PATH_BUFSIZ], pathenv[ENV_BUFSIZ];
    static const char default_path[] = "C:\\WINDOWS\\system32;C:\\WINDOWS";

    cnt = GetEnvironmentVariableA("PATH", pathenv, ENV_BUFSIZ);
    if (cnt == 0 || cnt >= ENV_BUFSIZ) {
        memcpy(pathenv, default_path, sizeof(default_path));
    }

    if (fexists(name)) {
        return abspath(name);
    }

    res = snprintf(execpath, sizeof(execpath), "%s.exe", name);
    if (res < 0 || (size_t) res >= sizeof(execpath)) {
        return NULL;
    }

    if (fexists(name)) {
        return abspath(name);
    }

    pathdup = strdup(pathenv);
    if (!pathdup) {
        return NULL;
    }

    token = strtok_r(pathdup, ";", &tokctx);
    while (token) {
        res = snprintf(execpath, sizeof(execpath), "%s\\%s", token, name);
        if (res < 0 || (size_t) res >= sizeof(execpath)) {
            continue;
        }
        if (fexists(execpath)) {
            free(pathdup);
            return abspath(execpath);
        }

        res = snprintf(execpath, sizeof(execpath), "%s\\%s.exe", token, name);
        if (res < 0 || (size_t) res >= sizeof(execpath)) {
            continue;
        }
        if (fexists(execpath)) {
            free(pathdup);
            return abspath(execpath);
        }

        token = strtok_r(NULL, ";", &tokctx);
    }

    free(pathdup);
    return NULL;
}


static int inject_dll(HANDLE proc, const char* dllpath) {
    int ret;
    DWORD res;
    HANDLE thr;
    LPVOID allocmem, thrfunc;

    allocmem = VirtualAllocEx(proc, NULL, strlen(dllpath) + 1,
                              MEM_COMMIT, PAGE_READWRITE);
    if (!allocmem) {
        save_err();
        return -1;
    }

    if (!WriteProcessMemory(proc, allocmem, dllpath, strlen(dllpath) + 1,
                            NULL)) {
        ret = -1;
        save_err();
        goto free_virt;
    }

    thrfunc = (LPVOID) GetProcAddress(GetModuleHandle("kernel32.dll"),
                                      "LoadLibraryA");
    if (!thrfunc) {
        ret = -1;
        save_err();
        goto free_virt;
    }

    thr = CreateRemoteThread(proc, NULL, 0, (LPTHREAD_START_ROUTINE) thrfunc,
                             allocmem, 0, NULL);
    if (!thr) {
        ret = -1;
        save_err();
        goto free_virt;
    }

    res = WaitForSingleObject(thr, INFINITE);
    if (res) {
        ret = -1;
        save_err();
        goto close_thr;
    }

    ret = 0;

close_thr:
    CloseHandle(thr);

free_virt:
    VirtualFreeEx(proc, allocmem, 0, MEM_RELEASE);

    return ret;
}


int main(int argc, char *argv[])
{
    char *libpath, *execpath, *childcmd;
    DWORD exitcode = EXIT_FAILURE;
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};

    if (argc <= 0) {
        return EXIT_FAILURE;
    }

    if (argc == 1) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    childcmd = get_child_commandline(argv[0]);
    if (!childcmd) {
        fprintf(stderr, "%s: failed to get command line.\n", argv[0]);
        return EXIT_FAILURE;
    }

    libpath = find_libbindhook(LIBNAME);
    if (!libpath) {
        fprintf(stderr, "%s: library file %s was not found.\n", argv[0],
                LIBNAME);
        return EXIT_FAILURE;
    }

    execpath = find_executable(argv[1]);
    if (!execpath) {
        fprintf(stderr, "%s: executable file %s was not found.\n", argv[0],
                argv[1]);
        goto free_libpath;
    }

    si.cb = sizeof(si);

    if (!CreateProcess(execpath, childcmd, NULL, NULL, FALSE, CREATE_SUSPENDED,
                       NULL, NULL, &si, &pi)) {
        save_err();
        fprintf(stderr, "%s: failed to create process: %s", argv[0], errmsg);
        goto free_execpath;
    }

    if (inject_dll(pi.hProcess, libpath)) {
        printf("%s: WARNING: failed to inject %s: %s\n", argv[0], libpath,
               errmsg);
    }

    ResumeThread(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exitcode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

free_libpath:
    free(libpath);

free_execpath:
    free(execpath);

    return exitcode;
}
