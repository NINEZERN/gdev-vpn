#include "udp.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>

static int run_server(void) {
    printf("Server is running...\n");

    int fd = udp_create();
    if (fd < 0) {
        perror("Failed to create UDP socket");
        return 1;
    }
    
    int result = udp_bind(fd, "127.0.0.1", 5555);
    if (result < 0) {
        perror("Failed to bind UDP socket");
        udp_close(fd);
        return 1;
    }
    printf("Server is bound to 127.0.0.1:5555\n");

    unsigned char buffer[1024];
    struct sockaddr_in client_addr = {0};

    printf("Waiting for incoming messages...\n");

    ssize_t received = udp_recv(fd, buffer, sizeof(buffer), &client_addr);

    if (received < 0) {
        perror("Failed to receive UDP message");
        udp_close(fd);
        return 1;
    }

    printf("Client says: ");
    fwrite(buffer, 1, (size_t)received, stdout);
    printf("\n");

    const unsigned char response[] = "Hello from server!";
    const size_t reply_len = sizeof(response) - 1;

    ssize_t sent = udp_send(fd, response, reply_len, &client_addr);
    if (sent < 0) {
        perror("Failed to send UDP response");
        udp_close(fd);
        return 1;
    }
    if (sent != (ssize_t)reply_len) {
        fprintf(stderr, "Partial send: sent %zd bytes, expected %zu bytes\n", sent, reply_len);
        udp_close(fd);
        return 1;
    }
    printf("Sent response to client: %s\n", response);

    udp_close(fd);

    return 0;
}

static int run_client(void) {
    printf("Client is running...\n");

    int fd = udp_create();
    if (fd < 0) {
        perror("Failed to create UDP socket");
        return 1;
    }

    struct timeval timeout = {0};
    timeout.tv_sec = 5;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
                &timeout, sizeof(timeout)) < 0) {
        perror("Failed to set receive timeout");
        udp_close(fd);
        return 1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5555);

    int result = inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (result != 1) {
        fprintf(stderr, "Invalid server IP address\n");
        udp_close(fd);
        return 1;
    }

    // Allow 1024 payload bytes, a newline and the terminating '\0'.
    char message[1026];
    printf("Enter a message (up to 1024 bytes): ");
    fflush(stdout);

    if (fgets(message, sizeof(message), stdin) == NULL) {
        if (ferror(stdin)) {
            fprintf(stderr, "Failed to read your message\n");
        } else {
            fprintf(stderr, "No message entered (end of input)\n");
        }
        udp_close(fd);
        return 1;
    }

    // Enter finishes the input; do not include its newline in the datagram.
    const size_t message_len = strcspn(message, "\n");
    if (message_len > 1024) {
        fprintf(stderr, "Message is too long (maximum 1024 bytes)\n");
        udp_close(fd);
        return 1;
    }
    message[message_len] = '\0';

    ssize_t sent = udp_send(fd, message, message_len, &server_addr);
    if (sent < 0) {
        perror("Failed to send UDP message");
        udp_close(fd);
        return 1;
    }
    if (sent != (ssize_t)message_len) {
        fprintf(stderr, "Partial send: sent %zd bytes, expected %zu bytes\n", sent, message_len);
        udp_close(fd);
        return 1;
    }
    printf("Sent message to server: %s\n", message);

    unsigned char buffer[1024];
    struct sockaddr_in reply_addr = {0};

    printf("Waiting for server reply...\n");

    ssize_t received = udp_recv(
        fd,
        buffer,
        sizeof(buffer),
        &reply_addr
    );

    if (received < 0) {
        perror("Failed to receive UDP message");
        udp_close(fd);
        return 1;
    }

    if (reply_addr.sin_family != AF_INET ||
        reply_addr.sin_addr.s_addr != server_addr.sin_addr.s_addr ||
        reply_addr.sin_port != server_addr.sin_port) {

        fprintf(stderr, "Reply came from an unexpected address\n");
        udp_close(fd);
        return 1;
    }

    printf("Server says: ");
    fwrite(buffer, 1, (size_t)received, stdout);
    printf("\n");

    udp_close(fd);

    return 0;
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server|client>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "server") == 0) {
        return run_server();
    } else if (strcmp(argv[1], "client") == 0) {
        return run_client();
    } else {
        fprintf(stderr, "Invalid argument: %s. Use 'server' or 'client'.\n", argv[1]);
        return 1;
    }

    
    
}
