#define _DEFAULT_SOURCE
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/wait.h>


#define LIBNAME "libbindhook.so"

#define CSR_ALLOW_TASK_FOR_PID (1 << 2)

extern int csr_check(uint32_t mask);

static void print_usage(char *name)
{
    fprintf(stderr, "Usage: %s <executable> [args...]\n", name);
}


static char *find_libbindhook(char *name)
{
    int res;
    char selfpath[PATH_MAX], libpath[PATH_MAX], *selfdir;

    if (!realpath("/proc/self/exe", selfpath)) {
        return NULL;
    }
    selfdir = dirname(selfpath);

    res = snprintf(libpath, sizeof(libpath), "%s/../lib/%s", selfdir, name);
    if (res < 0 || (size_t) res >= sizeof(libpath)) {
        return NULL;
    }
    if (access(libpath, F_OK) == 0) {
        return realpath(libpath, NULL);
    }

    res = snprintf(libpath, sizeof(libpath), "%s/lib/%s", selfdir, name);
    if (res < 0 || (size_t) res >= sizeof(libpath)) {
        return NULL;
    }
    if (access(libpath, F_OK) == 0) {
        return realpath(libpath, NULL);
    }

    res = snprintf(libpath, sizeof(libpath), "%s/%s", selfdir, name);
    if (res < 0 || (size_t) res >= sizeof(libpath)) {
        return NULL;
    }
    if (access(libpath, F_OK) == 0) {
        return realpath(libpath, NULL);
    }

    return NULL;   
}


static char *find_executable(char *name)
{
    int res;
    char *pathdup, execpath[PATH_MAX];
    const char *pathenv = getenv("PATH");

    if (!pathenv) {
        pathenv = "/usr/bin:/bin";
    }

    if (access(name, X_OK) == 0) {
        return realpath(name, NULL);
    }

    pathdup = strdup(pathenv);
    if (!pathdup) {
        return NULL;
    }

    char *token = strtok(pathdup, ":");
    while (token) {
        res = snprintf(execpath, sizeof(execpath), "%s/%s", token, name);
        if (res < 0 || (size_t) res >= sizeof(execpath)) {
            continue;
        }
        if (access(execpath, X_OK) == 0) {
            free(pathdup);
            return realpath(execpath, NULL);
        }
        token = strtok(NULL, ":");
    }

    free(pathdup);
    return NULL;
}


int main(int argc, char *argv[])
{
    char *libpath, *execpath;

    if (argc <= 0) {
        return EXIT_FAILURE;
    }

    if (argc == 1) {
        print_usage(argv[0]);
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
        free(libpath);
        return EXIT_FAILURE;
    }

    setenv("LD_PRELOAD", libpath, 1);

    execv(execpath, argv + 1);
    fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));

    free(libpath);
    free(execpath);

    return EXIT_FAILURE;
}
