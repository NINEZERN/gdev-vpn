#include "tun.h"

#include <errno.h>
#include <linux/if.h>
#include <unistd.h>
#include "unity.h"


void setUp(void) {
    // This function is called before each test case.
}

void tearDown(void) {
    // This function is called after each test case.
}

static void test_tun_create(void) {
    // NULL name
    TEST_ASSERT_EQUAL_INT(-1, tun_create(NULL));
    TEST_ASSERT_EQUAL_INT(EINVAL, errno);

    // Empty name
    TEST_ASSERT_EQUAL_INT(-1, tun_create(""));
    TEST_ASSERT_EQUAL_INT(EINVAL, errno);

    // Name exceeding IFNAMSIZ
    char long_name[IFNAMSIZ + 1];
    for (size_t i = 0; i < sizeof(long_name) - 1; i++) {
        long_name[i] = 'a';
    }
    long_name[sizeof(long_name) - 1] = '\0';
    TEST_ASSERT_EQUAL_INT(-1, tun_create(long_name));
    TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);

    // Successful creation of a TUN interface (only if running as root)
    if (geteuid() == 0) {
        int fd = tun_create("tun_test_0");
        TEST_ASSERT_TRUE(fd >= 0);
        TEST_ASSERT_EQUAL_INT(0, tun_close(fd));
    }
}


static void test_tun_read(void) {
    char buf[VPN_MTU] = {0};
    // Attempt to read from an invalid file descriptor (-1)
    TEST_ASSERT_EQUAL_INT(-1, tun_read(-1, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


static void test_tun_write(void) {
    char buf[VPN_MTU] = {0};
    // Attempt to write to an invalid file descriptor (-1)
    TEST_ASSERT_EQUAL_INT(-1, tun_write(-1, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


static void test_tun_close(void) {
    // Attempt to close an invalid file descriptor (-1)
    TEST_ASSERT_EQUAL_INT(-1, tun_close(-1));
    TEST_ASSERT_EQUAL_INT(EBADF, errno);

    // Close a real interface (if root)
    if (geteuid() == 0) {
        int fd = tun_create("tun_test_1");
        TEST_ASSERT_TRUE(fd >= 0);
        TEST_ASSERT_EQUAL_INT(0, tun_close(fd));
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_tun_create);
    RUN_TEST(test_tun_read);
    RUN_TEST(test_tun_write);
    RUN_TEST(test_tun_close);
    return UNITY_END();
}
