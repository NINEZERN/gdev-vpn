#ifndef GDEV_TUN_H
#define GDEV_TUN_H
#include <stddef.h>
#include <sys/types.h>

#define VPN_MTU 1420

// TUN interface functions
int tun_create(const char *name);
int tun_read(int fd, void *buf, size_t len);
int tun_write(int fd, const void *buf, size_t len);
int tun_close(int fd);

#endif // GDEV_TUN_H