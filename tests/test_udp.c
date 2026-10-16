#include "udp.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>

/* Shared setup helpers keep each test focused on its own checks. */
static int create_socket(void) {
    int fd = udp_create();
    assert(fd >= 0);
    return fd;
}

static struct sockaddr_in get_local_address(int fd) {
    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(addr);

    // getsockname fills the sockaddr_in structure with the socket address.
    assert(getsockname(fd, (struct sockaddr *)&addr, &addr_len) == 0);
    assert(addr_len == sizeof(addr));
    return addr;
}

static void set_receive_timeout(int fd) {
    struct timeval timeout = {0};
    timeout.tv_sec = 2;

    assert(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
                      &timeout, sizeof(timeout)) == 0);
}

/* A fixture owns the resources shared by the datagram exchange tests. */
struct udp_pair {
    int sender_fd;
    int receiver_fd;
    struct sockaddr_in sender_addr;
    struct sockaddr_in receiver_addr;
};

static void assert_loopback_address(const struct sockaddr_in *addr) {
    assert(addr->sin_family == AF_INET);
    assert(addr->sin_addr.s_addr == htonl(INADDR_LOOPBACK));
    assert(ntohs(addr->sin_port) != 0);
}

static struct udp_pair create_udp_pair(void) {
    struct udp_pair pair = {0};

    // Create UDP sockets.
    pair.sender_fd = create_socket();
    pair.receiver_fd = create_socket();

    // Port 0 lets the OS choose free ports, avoiding conflicts with other tests.
    assert(udp_bind(pair.sender_fd, "127.0.0.1", 0) == 0);
    assert(udp_bind(pair.receiver_fd, "127.0.0.1", 0) == 0);

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
    assert(udp_close(pair->sender_fd) == 0);
    assert(udp_close(pair->receiver_fd) == 0);
}

static void test_udp_create_close(void) {
    printf("[TEST] UDP socket creation and closure...\n");

    int fd = create_socket();
    assert(udp_close(fd) == 0);
    assert(udp_close(-1) == -1);
    assert(errno == EBADF);

    printf("  [PASS] UDP socket creation and closure\n");
}

static void test_udp_bind_invalid_arguments(void) {
    printf("[TEST] UDP binding with invalid arguments...\n");

    int fd = create_socket();

    // A missing IP address.
    assert(udp_bind(fd, NULL, 0) == -1);
    assert(errno == EINVAL);

    // An invalid IPv4 address.
    assert(udp_bind(fd, "invalid-ip", 0) == -1);
    assert(errno == EINVAL);

    // A valid address with an invalid descriptor.
    assert(udp_bind(-1, "127.0.0.1", 0) == -1);
    assert(errno == EBADF);

    // Release the socket created by this test.
    assert(udp_close(fd) == 0);

    printf("  [PASS] UDP binding with invalid arguments\n");
}

static void test_udp_bind(void) {
    printf("[TEST] UDP socket binding...\n");

    int fd = create_socket();

    // Port 0 is valid: the OS chooses an available port.
    assert(udp_bind(fd, "127.0.0.1", 0) == 0);

    struct sockaddr_in addr = get_local_address(fd);
    assert(addr.sin_family == AF_INET);
    assert(addr.sin_addr.s_addr == htonl(INADDR_LOOPBACK));
    assert(ntohs(addr.sin_port) != 0);

    printf("  Assigned port: %u\n", (unsigned int)ntohs(addr.sin_port));

    // Release the socket created by this test.
    assert(udp_close(fd) == 0);

    printf("  [PASS] UDP socket binding\n");
}

static void test_udp_send_recv_invalid_arguments(void) {
    printf("[TEST] UDP send and receive with invalid arguments...\n");

    int fd = create_socket();
    unsigned char byte = 0x42;
    struct sockaddr_in peer = {0};
    peer.sin_family = AF_INET;
    peer.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    peer.sin_port = htons(5555);

    // Each case has one invalid argument; valid arguments isolate its error.
    // No datagram is sent: every call below must fail before transmission.
    assert(udp_send(fd, &byte, sizeof(byte), NULL) == -1);
    assert(errno == EINVAL);

    // NULL is invalid with a nonzero length; empty datagrams are tested separately.
    assert(udp_send(fd, NULL, sizeof(byte), &peer) == -1);
    assert(errno == EINVAL);

    struct sockaddr_in unsupported_peer = peer;
    unsupported_peer.sin_family = AF_INET6;
    assert(udp_send(fd, &byte, sizeof(byte), &unsupported_peer) == -1);
    assert(errno == EAFNOSUPPORT);

    assert(udp_send(-1, &byte, sizeof(byte), &peer) == -1);
    assert(errno == EBADF);

    // These checks must fail without waiting for an incoming datagram.
    set_receive_timeout(fd);
    assert(udp_recv(fd, &byte, sizeof(byte), NULL) == -1);
    assert(errno == EINVAL);

    assert(udp_recv(fd, NULL, sizeof(byte), &peer) == -1);
    assert(errno == EINVAL);

    assert(udp_recv(-1, &byte, sizeof(byte), &peer) == -1);
    assert(errno == EBADF);

    assert(udp_close(fd) == 0);
    printf("  [PASS] UDP send and receive with invalid arguments\n");
}

static void test_udp_send_recv(void) {
    printf("[TEST] UDP send and receive...\n");

    struct udp_pair pair = create_udp_pair();

    // Send a datagram, receive it and compare its contents.
    const unsigned char message[] = "Hello, UDP!";
    const size_t message_len = sizeof(message) - 1;
    unsigned char buffer[sizeof(message) - 1] = {0};
    struct sockaddr_in peer = {0};

    ssize_t sent = udp_send(
        pair.sender_fd, message, message_len, &pair.receiver_addr);
    assert(sent == (ssize_t)message_len);

    ssize_t received = udp_recv(
        pair.receiver_fd, buffer, sizeof(buffer), &peer);
    assert(received == (ssize_t)message_len);
    assert(memcmp(buffer, message, message_len) == 0);

    // Verify that udp_recv reports the actual sender address and port.
    assert(peer.sin_family == AF_INET);
    assert(peer.sin_addr.s_addr == pair.sender_addr.sin_addr.s_addr);
    assert(peer.sin_port == pair.sender_addr.sin_port);

    printf("  Received: ");
    fwrite(buffer, 1, (size_t)received, stdout);
    printf("\n");

    close_udp_pair(&pair);
    printf("  [PASS] UDP send and receive\n");
}

static void test_udp_empty_datagram(void) {
    printf("[TEST] UDP send and receive empty datagram...\n");

    struct udp_pair pair = create_udp_pair();
    struct sockaddr_in peer = {0};

    // A datagram can have no payload. A zero return value is success, not EOF.
    assert(udp_send(pair.sender_fd, NULL, 0, &pair.receiver_addr) == 0);
    assert(udp_recv(pair.receiver_fd, NULL, 0, &peer) == 0);

    printf("  Received an empty datagram (0 bytes)\n");

    close_udp_pair(&pair);
    printf("  [PASS] UDP send and receive empty datagram\n");
}

static void test_udp_truncated_datagram(void) {
    printf("[TEST] UDP send and receive truncated datagram...\n");

    struct udp_pair pair = create_udp_pair();
    const unsigned char message[] = "This message is too large";
    // Send the text bytes without the C string terminator.
    const size_t message_len = sizeof(message) - 1;

    // Intentionally smaller than the payload to trigger MSG_TRUNC in udp_recv.
    unsigned char buffer[4] = {0};
    struct sockaddr_in peer = {0};

    // The complete datagram must be sent; only the receive buffer is too small.
    assert(udp_send(pair.sender_fd, message, message_len, &pair.receiver_addr)
           == (ssize_t)message_len);

    // The wrapper translates MSG_TRUNC into -1/EMSGSIZE. Check errno immediately.
    assert(udp_recv(pair.receiver_fd, buffer, sizeof(buffer), &peer) == -1);
    assert(errno == EMSGSIZE);

    // The datagram is consumed and its tail discarded. Do not use the partial
    // bytes in buffer as a valid packet or try to receive the remainder.
    close_udp_pair(&pair);
    printf("  [PASS] UDP truncated datagram rejected with EMSGSIZE\n");
}

int main(void) {
    // Preserve progress messages if an assertion aborts with redirected stdout.
    setbuf(stdout, NULL);

    test_udp_create_close();
    test_udp_bind_invalid_arguments();
    test_udp_bind();
    test_udp_send_recv_invalid_arguments();
    test_udp_send_recv();
    test_udp_empty_datagram();
    test_udp_truncated_datagram();
    return 0;
}
