#include "tun.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <sys/wait.h>

static volatile int running = 1;

void handle_sigint(int sig) {
    (void)sig;
    running = 0;
}

int main(void) {
    signal(SIGINT, handle_sigint);

    printf("=== GDEV-VPN: LIVE TUN DEMO ===\n");
    printf("[1] Creating virtual interface 'tun0'...\n");

    int fd = tun_create("tun0");
    if (fd < 0) {
        perror("Failed to create tun0 (Did you run with sudo?)");
        return 1;
    }
    printf("[2] Interface 'tun0' created successfully! (fd = %d)\n", fd);

    // Assign IP address 10.0.0.1/24 and bring the interface UP
    printf("[3] Configuring IP 10.0.0.1/24 and setting interface UP...\n");
    int status = system("ip addr add 10.0.0.1/24 dev tun0");
    if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Failed to assign IP address to tun0\n");
        tun_close(fd);
        return 1;
    }

    status = system("ip link set dev tun0 up");
    if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Failed to bring tun0 up\n");
        tun_close(fd);
        return 1;
    }

    printf("\n>>> TUNNEL IS LIVE! Waiting for IP packets... (Press Ctrl+C to exit) <<<\n\n");

    uint8_t buffer[VPN_MTU];
    int packet_count = 0;

    while (running) {
        int nread = tun_read(fd, buffer, sizeof(buffer));
        if (nread > 0) {
            packet_count++;
            int ip_version = buffer[0] >> 4;

            printf("[Packet #%03d] Length: %4d bytes | Version: IPv%d\n",
                   packet_count, nread, ip_version);

            if (ip_version == 4 && nread >= 20) {
                // Byte 9: Protocol (1 = ICMP/Ping, 2 = IGMP, 6 = TCP, 17 = UDP)
                uint8_t proto = buffer[9];
                const char *proto_name = "Other";
                if (proto == 1) proto_name = "ICMP (Ping)";
                else if (proto == 2) proto_name = "IGMP (Multicast)";
                else if (proto == 6) proto_name = "TCP";
                else if (proto == 17) proto_name = "UDP";

                printf("             Source:      %d.%d.%d.%d\n",
                       buffer[12], buffer[13], buffer[14], buffer[15]);
                printf("             Destination: %d.%d.%d.%d\n",
                       buffer[16], buffer[17], buffer[18], buffer[19]);
                printf("             Protocol:    %s (id: %d)\n\n",
                       proto_name, proto);
            } else if (ip_version == 6) {
                printf("             Info:        IPv6 Kernel Discovery / Multicast Packet\n\n");
            }
        }
    }

    printf("\n[4] Closing tun0 and cleaning up...\n");
    tun_close(fd);
    printf("=== DONE: tun0 removed from OS ===\n");
    return 0;
}