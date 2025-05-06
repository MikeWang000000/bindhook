#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <mach-o/dyld.h>
#include <mach-o/fat.h>

#define LIBNAME "libbindhook.dylib"

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
    uint32_t bufsize = sizeof(selfpath);

    res = _NSGetExecutablePath(selfpath, &bufsize);
    if (res) {
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


static int is_sip_enabled(void)
{
    return csr_check(CSR_ALLOW_TASK_FOR_PID) != 0;
}


static int is_mach_o(const char *path)
{
    int fd;
    uint32_t magic = 0;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        return 0;
    }
    if (read(fd, &magic, sizeof(magic)) != sizeof(magic)) {
        close(fd);
        return 0;
    }
    close(fd);

    return (magic == FAT_MAGIC || magic == FAT_CIGAM ||
            magic == FAT_MAGIC_64 || magic == FAT_CIGAM_64 ||
            magic == MH_MAGIC || magic == MH_CIGAM ||
            magic == MH_MAGIC_64 || magic == MH_CIGAM_64);
}


static int is_notarized(const char *path)
{
    char buffer[2048] = {0};
    int pipefd[2], status, ret;
    pid_t child;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    child = fork();
    if (child == -1) {
        return 0;
    } else if (child == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDERR_FILENO) == -1) {
            _Exit(EXIT_FAILURE);
        }
        close(pipefd[1]);
        execl("/usr/bin/codesign", "codesign", "-dvv", path, NULL);
        _Exit(EXIT_FAILURE);
    } else {
        close(pipefd[1]);
        waitpid(child, &status, 0);
        read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (strstr(buffer, "\nAuthority=Apple Root CA\n") != NULL) {
            ret = 1;
        } else {
            ret = 0;
        }
        close(pipefd[0]);
    }
    return ret;
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

    if (is_sip_enabled()) {
        if (!is_mach_o(execpath)) {
            fprintf(stderr, "%s: Warning: SIP is enabled. "
                            "Dylib injection may fail.\n", argv[0]);
        } else if (is_notarized(execpath)) {
            fprintf(stderr, "%s: Warning: SIP is enabled. "
                "Dylib injection may fail. "
                "Use codesign tool to sign an ad-hoc signature.\n", argv[0]);
        }
    }

    setenv("DYLD_INSERT_LIBRARIES", libpath, 1);

    execv(execpath, argv + 1);
    fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));

    free(libpath);
    free(execpath);

    return EXIT_FAILURE;
}
