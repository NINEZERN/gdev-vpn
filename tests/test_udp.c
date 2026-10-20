#include "udp.h"
#include "unity.h"

#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>

// Track open sockets so tearDown can close them if Unity aborts the current test.
static int tracked_fds[2];

void setUp(void) {
    // Track file descriptors to ensure they are closed after each test.
    tracked_fds[0] = -1;
    tracked_fds[1] = -1;
}

void tearDown(void) {
    int close_failed = 0;

    // Ensure all tracked file descriptors are closed.
    for (size_t i = 0; i < 2; i++) {
        if (tracked_fds[i] >= 0) {
            int fd = tracked_fds[i];
            tracked_fds[i] = -1;  // Avoid double-close in case of failure.

            if (udp_close(fd) != 0) {
                close_failed = 1;
            }
        }
    }
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, close_failed, "Failed to close a test socket");
}

/* Shared setup helpers keep each test focused on its own checks. */
static int create_socket(void) {
    for (size_t i = 0; i < 2; i++) {
        if (tracked_fds[i] == -1) {
            int fd = udp_create();
            TEST_ASSERT_TRUE(fd >= 0);

            tracked_fds[i] = fd;
            return fd;
        }
    }
    TEST_FAIL_MESSAGE("No free slot for a test socket");
    return -1;
}

static int close_socket(int fd) {
    for (size_t i = 0; i < 2; i++) {
        if (tracked_fds[i] >= 0 && tracked_fds[i] == fd) {
            tracked_fds[i] = -1;
            return udp_close(fd);
        }
    }

    TEST_FAIL_MESSAGE("Attempted to close an untracked socket");
    return -1;
}

static struct sockaddr_in get_local_address(int fd) {
    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(addr);

    // getsockname fills the sockaddr_in structure with the socket address.
    TEST_ASSERT_EQUAL_INT(0, getsockname(fd, (struct sockaddr *)&addr, &addr_len));
    TEST_ASSERT_EQUAL_size_t(sizeof(addr), addr_len);
    return addr;
}

static void set_receive_timeout(int fd) {
    struct timeval timeout = {0};
    timeout.tv_sec = 2;

    TEST_ASSERT_EQUAL_INT(0, setsockopt(
        fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)));
}

/* A fixture owns the resources shared by the datagram exchange tests. */
struct udp_pair {
    int sender_fd;
    int receiver_fd;
    struct sockaddr_in sender_addr;
    struct sockaddr_in receiver_addr;
};

static void assert_loopback_address(const struct sockaddr_in *addr) {
    TEST_ASSERT_EQUAL_INT(AF_INET, addr->sin_family);
    TEST_ASSERT_EQUAL_UINT32(htonl(INADDR_LOOPBACK), addr->sin_addr.s_addr);
    TEST_ASSERT_TRUE(ntohs(addr->sin_port) != 0);
}

static struct udp_pair create_udp_pair(void) {
    struct udp_pair pair = {0};

    // Create UDP sockets.
    pair.sender_fd = create_socket();
    pair.receiver_fd = create_socket();

    // Port 0 lets the OS choose free ports, avoiding conflicts with other tests.
    TEST_ASSERT_EQUAL_INT(0, udp_bind(pair.sender_fd, "127.0.0.1", 0));
    TEST_ASSERT_EQUAL_INT(0, udp_bind(pair.receiver_fd, "127.0.0.1", 0));

    pair.sender_addr = get_local_address(pair.sender_fd);
    pair.receiver_addr = get_local_address(pair.receiver_fd);
    assert_loopback_address(&pair.sender_addr);
    assert_loopback_address(&pair.receiver_addr);

    // Fail instead of waiting indefinitely if the datagram never arrives.
    set_receive_timeout(pair.receiver_fd);
    return pair;
}

static void close_udp_pair(const struct udp_pair *pair) {
    // Close both sockets owned by this fixture.
    TEST_ASSERT_EQUAL_INT(0, close_socket(pair->sender_fd));
    TEST_ASSERT_EQUAL_INT(0, close_socket(pair->receiver_fd));
}

static void test_udp_create_close(void) {
    int fd = create_socket();
    TEST_ASSERT_EQUAL_INT(0, close_socket(fd));
    TEST_ASSERT_EQUAL_INT(-1, udp_close(-1));
    TEST_ASSERT_EQUAL_INT(EBADF, errno);
}

static void test_udp_bind_invalid_arguments(void) {
    int fd = create_socket();

    // A missing IP address.
    TEST_ASSERT_EQUAL_INT(-1, udp_bind(fd, NULL, 0));
    TEST_ASSERT_EQUAL_INT(EINVAL, errno);

    // An invalid IPv4 address.
    TEST_ASSERT_EQUAL_INT(-1, udp_bind(fd, "invalid-ip", 0));
    TEST_ASSERT_EQUAL_INT(EINVAL, errno);

    // A valid address with an invalid descriptor.
    TEST_ASSERT_EQUAL_INT(-1, udp_bind(-1, "127.0.0.1", 0));
    TEST_ASSERT_EQUAL_INT(EBADF, errno);

    // Release the socket created by this test.
    TEST_ASSERT_EQUAL_INT(0, close_socket(fd));
}

static void test_udp_bind(void) {
    int fd = create_socket();

    // Port 0 is valid: the OS chooses an available port.
    TEST_ASSERT_EQUAL_INT(0, udp_bind(fd, "127.0.0.1", 0));

    struct sockaddr_in addr = get_local_address(fd);
    TEST_ASSERT_EQUAL_INT(AF_INET, addr.sin_family);
    TEST_ASSERT_EQUAL_UINT32(htonl(INADDR_LOOPBACK), addr.sin_addr.s_addr);
    TEST_ASSERT_TRUE(ntohs(addr.sin_port) != 0);

    // Release the socket created by this test.
    TEST_ASSERT_EQUAL_INT(0, close_socket(fd));
}

static void test_udp_send_recv_invalid_arguments(void) {
    int fd = create_socket();
    unsigned char byte = 0x42;
    struct sockaddr_in peer = {0};
    peer.sin_family = AF_INET;
    peer.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    peer.sin_port = htons(5555);

    // Each case has one invalid argument; valid arguments isolate its error.
    // No datagram is sent: every call below must fail before transmission.
    TEST_ASSERT_EQUAL_INT64(-1, udp_send(fd, &byte, sizeof(byte), NULL));
    TEST_ASSERT_EQUAL_INT(EINVAL, errno);

    // NULL is invalid with a nonzero length; empty datagrams are tested separately.
    TEST_ASSERT_EQUAL_INT64(-1, udp_send(fd, NULL, sizeof(byte), &peer));
    TEST_ASSERT_EQUAL_INT(EINVAL, errno);

    struct sockaddr_in unsupported_peer = peer;
    unsupported_peer.sin_family = AF_INET6;
    TEST_ASSERT_EQUAL_INT64(-1, udp_send(fd, &byte, sizeof(byte), &unsupported_peer));
    TEST_ASSERT_EQUAL_INT(EAFNOSUPPORT, errno);

    TEST_ASSERT_EQUAL_INT64(-1, udp_send(-1, &byte, sizeof(byte), &peer));
    TEST_ASSERT_EQUAL_INT(EBADF, errno);

    // These checks must fail without waiting for an incoming datagram.
    set_receive_timeout(fd);
    TEST_ASSERT_EQUAL_INT64(-1, udp_recv(fd, &byte, sizeof(byte), NULL));
    TEST_ASSERT_EQUAL_INT(EINVAL, errno);

    TEST_ASSERT_EQUAL_INT64(-1, udp_recv(fd, NULL, sizeof(byte), &peer));
    TEST_ASSERT_EQUAL_INT(EINVAL, errno);

    TEST_ASSERT_EQUAL_INT64(-1, udp_recv(-1, &byte, sizeof(byte), &peer));
    TEST_ASSERT_EQUAL_INT(EBADF, errno);

    TEST_ASSERT_EQUAL_INT(0, close_socket(fd));
}

static void test_udp_send_recv(void) {
    struct udp_pair pair = create_udp_pair();

    // Send a datagram, receive it and compare its contents.
    const unsigned char message[] = "Hello, UDP!";
    const size_t message_len = sizeof(message) - 1;
    unsigned char buffer[sizeof(message) - 1] = {0};
    struct sockaddr_in peer = {0};

    ssize_t sent = udp_send(
        pair.sender_fd, message, message_len, &pair.receiver_addr);
    TEST_ASSERT_EQUAL_INT64((ssize_t)message_len, sent);

    ssize_t received = udp_recv(
        pair.receiver_fd, buffer, sizeof(buffer), &peer);
    TEST_ASSERT_EQUAL_INT64((ssize_t)message_len, received);
    TEST_ASSERT_EQUAL_MEMORY(message, buffer, message_len);

    // Verify that udp_recv reports the actual sender address and port.
    TEST_ASSERT_EQUAL_INT(AF_INET, peer.sin_family);
    TEST_ASSERT_EQUAL_UINT32(pair.sender_addr.sin_addr.s_addr, peer.sin_addr.s_addr);
    TEST_ASSERT_EQUAL_UINT16(pair.sender_addr.sin_port, peer.sin_port);

    close_udp_pair(&pair);
}

static void test_udp_empty_datagram(void) {
    struct udp_pair pair = create_udp_pair();
    struct sockaddr_in peer = {0};

    // A datagram can have no payload. A zero return value is success, not EOF.
    TEST_ASSERT_EQUAL_INT64(0, udp_send(pair.sender_fd, NULL, 0, &pair.receiver_addr));
    TEST_ASSERT_EQUAL_INT64(0, udp_recv(pair.receiver_fd, NULL, 0, &peer));

    close_udp_pair(&pair);
}

static void test_udp_truncated_datagram(void) {
    struct udp_pair pair = create_udp_pair();
    const unsigned char message[] = "This message is too large";
    // Send the text bytes without the C string terminator.
    const size_t message_len = sizeof(message) - 1;

    // Intentionally smaller than the payload to trigger MSG_TRUNC in udp_recv.
    unsigned char buffer[4] = {0};
    struct sockaddr_in peer = {0};

    // The complete datagram must be sent; only the receive buffer is too small.
    TEST_ASSERT_EQUAL_INT64((ssize_t)message_len,
        udp_send(pair.sender_fd, message, message_len, &pair.receiver_addr));

    // The wrapper translates MSG_TRUNC into -1/EMSGSIZE. Check errno immediately.
    TEST_ASSERT_EQUAL_INT64(-1, udp_recv(pair.receiver_fd, buffer, sizeof(buffer), &peer));
    TEST_ASSERT_EQUAL_INT(EMSGSIZE, errno);

    // The datagram is consumed and its tail discarded. Do not use the partial
    // bytes in buffer as a valid packet or try to receive the remainder.
    close_udp_pair(&pair);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_udp_create_close);
    RUN_TEST(test_udp_bind_invalid_arguments);
    RUN_TEST(test_udp_bind);
    RUN_TEST(test_udp_send_recv_invalid_arguments);
    RUN_TEST(test_udp_send_recv);
    RUN_TEST(test_udp_empty_datagram);
    RUN_TEST(test_udp_truncated_datagram);
    return UNITY_END();
}
