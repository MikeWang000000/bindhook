#define _DEFAULT_SOURCE
#include <sys/syscall.h>
#include <sys/socket.h>

#include "syscall_asm.h"

#ifdef __ANDROID__
extern int *__errno(void);
#define errno (*__errno())
#else
extern int *__errno_location(void);
#define errno (*__errno_location())
#endif /* __ANDROID__ */

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int val = 1, err;
    __syscall5(SYS_setsockopt, (long) sockfd, (long) SOL_SOCKET,
               (long) SO_REUSEADDR, (long) &val, (long) sizeof(val));

#ifdef SO_REUSEPORT
    __syscall5(SYS_setsockopt, (long) sockfd, (long) SOL_SOCKET,
               (long) SO_REUSEPORT, (long) &val, (long) sizeof(val));
#else
#warning "SO_REUSEPORT is not available"
#endif /* SO_REUSEPORT */

    err = __syscall3(SYS_bind, (long) sockfd, (long) addr, (long) addrlen);
    if (err) {
        errno = -err;
        return -1;
    }
    return 0;
}
