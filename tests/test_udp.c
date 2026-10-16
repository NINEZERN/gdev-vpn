#include "udp.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <string.h>

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

static void test_udp_send_recv(void) {
    printf("[TEST] UDP send and receive...\n");

    // Create UDP sockets.
    int sender_fd = create_socket();
    int receiver_fd = create_socket();

    // Bind sockets.
    assert(udp_bind(sender_fd, "127.0.0.1", 0) == 0);
    assert(udp_bind(receiver_fd, "127.0.0.1", 0) == 0);

    struct sockaddr_in receiver_addr = get_local_address(receiver_fd);
    assert(receiver_addr.sin_family == AF_INET);
    assert(receiver_addr.sin_addr.s_addr == htonl(INADDR_LOOPBACK));
    assert(ntohs(receiver_addr.sin_port) != 0);

    set_receive_timeout(receiver_fd);

    // Send a datagram, receive it and compare its contents.
    const unsigned char message[] = "Hello, UDP!";
    const size_t message_len = sizeof(message) - 1;

    unsigned char buffer[sizeof(message) - 1] = {0};
    struct sockaddr_in peer = {0};

    ssize_t sent = udp_send(
        sender_fd, message, message_len, &receiver_addr);
    assert(sent == (ssize_t)message_len);

    ssize_t received = udp_recv(
        receiver_fd, buffer, sizeof(buffer), &peer);
    assert(received == (ssize_t)message_len);
    assert(memcmp(buffer, message, message_len) == 0);

    // Verify that udp_recv reports the actual sender address and port.
    struct sockaddr_in sender_addr = get_local_address(sender_fd);
    assert(peer.sin_family == AF_INET);
    assert(peer.sin_addr.s_addr == sender_addr.sin_addr.s_addr);
    assert(peer.sin_port == sender_addr.sin_port);

    printf("  Received: ");
    fwrite(buffer, 1, (size_t)received, stdout);
    printf("\n");

    // Close the sockets.
    assert(udp_close(sender_fd) == 0);
    assert(udp_close(receiver_fd) == 0);

    printf("  [PASS] UDP send and receive\n");
}

int main(void) {
    test_udp_create_close();
    test_udp_bind_invalid_arguments();
    test_udp_bind();
    test_udp_send_recv();
    return 0;
}
