#define _GNU_SOURCE

#include "tun.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>


int tun_create(const char *name) {
    // Validate the name parameter is not NULL and not empty
    if (name == NULL || *name == '\0') {
        errno = EINVAL;
        return -1;
    }

    // Validate the name length does not exceed IFNAMSIZ (Linux limit for interface name size)
    if (strlen(name) >= IFNAMSIZ) {
        errno = ENAMETOOLONG;
        return -1;
    }

    // Open /dev/net/tun device file with read/write permissions and close-on-exec flag
    int fd = open("/dev/net/tun", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }

    // Prepare the ifreq structure for ioctl call
    struct ifreq ifr = {0};

    // Copy the interface name into ifr_name
    memcpy(ifr.ifr_name, name, strlen(name) + 1);

    // Set flags for TUN device without packet information
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI; 

    if (ioctl(fd, TUNSETIFF, &ifr) == -1) {
        // Save errno before closing the file descriptor
        int saved_errno = errno; 
        close(fd);
        errno = saved_errno;
        return -1;
    }

    return fd;
}

int tun_read(int fd, void *buf, size_t len) {
    ssize_t nread;

    do {
        nread = read(fd, buf, len);
        // Retry only if the read was interrupted by a signal from OS (EINTR)
    } while (nread < 0 && errno == EINTR);

    return (int)nread;
}

int tun_write(int fd, const void *buf, size_t len) {
    ssize_t nwritten;

    do {
        nwritten = write(fd, buf, len);

    } while (nwritten < 0 && errno == EINTR); // Retry if interrupted by signal

    return (int)nwritten;
}

int tun_close(int fd) {
    return close(fd);
}