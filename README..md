# GDEV VPN

A learning project for building a Linux VPN in C, with the goal
of eventually using it as a personal VPN hosted on Azure.

## Current status

The project currently provides:

- Linux TUN interface creation and packet I/O.
- ChaCha20-Poly1305-IETF authenticated encryption using libsodium.
- IPv4 UDP socket creation, binding, sending and receiving.
- Tests and standalone examples for the implemented modules.

A complete VPN tunnel is not implemented yet.
The examples demonstrate individual components.

## Requirements

- Linux
- A C11-compatible compiler, such as GCC
- Make
- libsodium development headers and library
- Valgrind for memory checks
- iproute2 for the TUN example

## Build and test

Run all tests:

```bash
make test
```

Run the tests under Valgrind:

```bash
make memcheck
```

Remove generated build artifacts:

```bash
make clean
```

Tests currently use standard C assertions and must be built
without `NDEBUG`.

UDP tests use the local loopback interface and require permission
to create network sockets. Tests that create real TUN interfaces
require the appropriate privileges and access to `/dev/net/tun`.


## Examples

### TUN packet inspection

Build the example:

```bash
make demo-tun
```

Run with the privileges required to create and configure a TUN interface:

```bash
sudo ./build/demo_tun
```

The example creates `tun0`, assigns `10.0.0.1/24` and prints
information about outgoing IP packets.

In another terminal, generate traffic:

```bash
ping -c 3 10.0.0.2
```

The example only inspects packets; it does not forward them
or generate ping replies.

### Message encryption and decryption

```bash
make demo-crypto
./build/demo_crypto
```

The example generates a key and nonce, encrypts a message with AAD,
prints the ciphertext in hexadecimal and decrypts the message.

Both sender and recipient are simulated in one process.
The example does not implement key exchange.

### UDP client and server

Build the example:

```bash
make demo-udp
```

Start the server in one terminal:

```bash
./build/demo_udp server
```

Start the client in another terminal:

```bash
./build/demo_udp client
```

Enter a message in the client and press Enter.

The server listens on `127.0.0.1:5555`, prints the received message
and sends a reply. Both processes exit after one exchange.
Restart the server before running the client again.

Client input is limited to 1024 bytes. The client waits up to
five seconds for a reply and checks its source IP address and port.

This example sends plaintext messages without encryption.

## Project structure

```text
include/    Public module headers
src/        TUN, crypto and UDP implementations
tests/      Module tests
examples/   Standalone demonstrations
.github/    CI workflows
Makefile    Build and test targets
```

## Roadmap

- Migrate tests from standard C assertions to Unity.
- Define the tunnel packet format and session lifecycle.
- Implement authenticated session establishment and key management.
- Add per-direction keys, nonce counters and replay protection.
- Combine TUN, crypto and UDP into an encrypted IP tunnel.
- Test connectivity between two Linux hosts.
- Deploy the server on Azure.
- Configure forwarding, NAT, client routes and DNS.
- Add reconnection and graceful shutdown.

## Limitations

This project is under active development and is not ready
for use as a secure VPN.

The initial VPN implementation will target one connected client.
Support for multiple simultaneous clients is planned for a later stage.