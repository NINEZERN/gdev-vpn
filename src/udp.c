#include "udp.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <unistd.h>


int udp_create(void) {
    return socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
}

int udp_bind(int fd, const char *ip, uint16_t port) {
    if (ip == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    int result = inet_pton(AF_INET, ip, &addr.sin_addr);

    if (result == 0) {
        errno = EINVAL;
        return -1;
    } else if (result == -1) {
        return -1;
    }
    
    return bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
}

ssize_t udp_send(int fd,
    const void *buf,
    size_t len,
    const struct sockaddr_in *peer) {

    // Validate the peer address is not NULL and the buffer is not NULL
    if (peer == NULL || (buf == NULL && len != 0)) {
        errno = EINVAL;
        return -1;
    }

    // Validate the peer address family is AF_INET
    if (peer->sin_family != AF_INET) {
        errno = EAFNOSUPPORT;
        return -1;
    }
    ssize_t sent;

    // Retry sending the datagram if interrupted by a signal from OS (EINTR)
    do {
        sent = sendto(
            fd,
            buf,
            len,
            0,
            (const struct sockaddr *) peer,
            sizeof(*peer)
        );
    } while (sent < 0 && errno == EINTR);

    return sent;
}

ssize_t udp_recv(
    int fd,
    void *buf,
    size_t capacity,
    struct sockaddr_in *peer) {
    
    // Validate the peer address is not NULL and the buffer is not NULL
    if (peer == NULL || (buf == NULL && capacity != 0)) {
        errno = EINVAL;
        return -1;
    }

    // This struct is used to receive the sender address.
    struct sockaddr_in sender = {0};

    // This struct defines where to store the datagram.
    struct iovec iov = {0};
    iov.iov_base = buf;
    iov.iov_len = capacity;

    // Instruction for recvmsg.
    struct msghdr msg = {0};
    msg.msg_name = &sender;
    msg.msg_namelen = sizeof(sender);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    
    // Retry receiving the datagram if interrupted by a signal from OS (EINTR)
    ssize_t received;
    do {
        msg.msg_namelen = sizeof(sender);
        msg.msg_flags = 0;

        received = recvmsg(fd, &msg, 0);

    } while (received < 0 && errno == EINTR);

    // Return the receive error; errno is already set by recvmsg().
    if (received < 0) {
        return -1;
    }

    // If the received datagram is larger than the capacity, return -1
    if (msg.msg_flags & MSG_TRUNC) {
        errno = EMSGSIZE;
        return -1;
    }

    *peer = sender;
    return received;
}

int udp_close(int fd) {
    return close(fd);
}

