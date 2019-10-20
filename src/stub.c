#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "ivdistshm.h"

IVDISTSHM ivdistshm;

//
int socket(int domain, int type, int protocol)
{
	return 0;
}

int setsockopt(int sockfd, int level, int optname, const void *optval,
	       socklen_t optlen)
{
	return 0;
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	return 0;
}

int listen(int sockfd, int backlog)
{
	return 0;
}

//

int accept(int sockfd, struct sockaddr *addr, socklen_t * addrlen)
{
	return ivdistshm_link_remote(&ivdistshm, PHYS_ADDR, SIZE, 1);
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	return ivdistshm_link_remote(&ivdistshm, PHYS_ADDR, SIZE, 0);
}

ssize_t read(int fd, void *buf, size_t count)
{
	ivdistshm_read(buf, &ivdistshm, count);

	return count;
}

ssize_t write(int fd, const void *buf, size_t count)
{
	ivdistshm_write(&ivdistshm, buf, count);

	return count;
}

int close(int fd)
{
	return ivdistshm_unlink(&ivdistshm);
}
