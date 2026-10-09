#include "tun.h"

#include <assert.h>
#include <errno.h>
#include <linux/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


static void test_tun_create(void) {
    printf("[TEST] tun_create()...\n");

    // NULL name
    assert(tun_create(NULL) == -1);
    assert(errno == EINVAL);

    // Empty name
    assert(tun_create("") == -1);
    assert(errno == EINVAL);

    // Name exceeding IFNAMSIZ
    char long_name[IFNAMSIZ + 1];
    for (size_t i = 0; i < sizeof(long_name) - 1; i++) {
        long_name[i] = 'a';
    }
    long_name[sizeof(long_name) - 1] = '\0';
    assert(tun_create(long_name) == -1);
    assert(errno == ENAMETOOLONG);

    // Successful creation of a TUN interface (only if running as root)
    if (geteuid() == 0) {
        int fd = tun_create("tun_test_0");
        assert(fd >= 0);
        tun_close(fd);
        printf("  [PASS] real interface creation\n");
    }

    printf("  [PASS] tun_create\n");
}


static void test_tun_read(void) {
    printf("[TEST] tun_read()...\n");

    char buf[VPN_MTU];
    // Attempt to read from an invalid file descriptor (-1)
    assert(tun_read(-1, buf, sizeof(buf)) == -1);
    assert(errno == EBADF);

    printf("  [PASS] tun_read\n");
}


static void test_tun_write(void) {
    printf("[TEST] tun_write()...\n");

    char buf[VPN_MTU];
    // Attempt to write to an invalid file descriptor (-1)
    assert(tun_write(-1, buf, sizeof(buf)) == -1);
    assert(errno == EBADF);

    printf("  [PASS] tun_write\n");
}


static void test_tun_close(void) {
    printf("[TEST] tun_close()...\n");

    // Attempt to close an invalid file descriptor (-1)
    assert(tun_close(-1) == -1);
    assert(errno == EBADF);

    // Close a real interface (if root)
    if (geteuid() == 0) {
        int fd = tun_create("tun_test_1");
        assert(fd >= 0);
        assert(tun_close(fd) == 0);
        printf("  [PASS] real interface clean closure\n");
    }

    printf("  [PASS] tun_close\n");
}

int main(void) {
    printf("=== RUNNING TUN MODULE TESTS ===\n");

    test_tun_create();
    test_tun_read();
    test_tun_write();
    test_tun_close();

    printf("=== ALL TUN TESTS PASSED ===\n");
    return EXIT_SUCCESS;
}
