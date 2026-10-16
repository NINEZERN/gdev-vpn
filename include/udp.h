#ifndef GDEV_UDP_H
#define GDEV_UDP_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <netinet/in.h>

/* Return an IPv4 UDP socket descriptor, or -1 with errno set. */
int udp_create(void);

/* Bind to a local IPv4 address and port. */
int udp_bind(int fd, const char *ip, uint16_t port);

/* Send one datagram. Return bytes sent, or -1 with errno set*/
ssize_t udp_send(
    int fd,
    const void *buf,
    size_t len,
    const struct sockaddr_in *peer
);

/* Receive one datagram and save its sender address.
 * Return bytes received, or -1 with errno set.
 * A zero return value means an empty datagram, not a closed connection.
 */
ssize_t udp_recv(
    int fd,
    void *buf,
    size_t capacity,
    struct sockaddr_in *peer
);

/* Close socket. Return 0 on success or -1 with errno set. */
int udp_close(int fd);


#endif // GDEV_UDP_H