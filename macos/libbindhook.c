#include <sys/syscall.h>
#include <sys/socket.h>

extern int __syscall(quad_t number, ...);
typedef int (*bind_func_t)(int, const struct sockaddr *, socklen_t);

int bind__bindhook_(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int val = 1;

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val));

    return __syscall((quad_t) SYS_bind, (quad_t) sockfd, (quad_t) addr,
                     (quad_t) addrlen);
}

struct {
    bind_func_t newfunc;
    bind_func_t oldfunc;
} dyld_interpose__bindhook_ __attribute__((section ("__DATA,__interpose"))) = {
    &bind__bindhook_, &bind
};
