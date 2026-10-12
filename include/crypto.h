#ifndef GDEV_CRYPTO_H
#define GDEV_CRYPTO_H

#include <stddef.h>
#include <sodium.h>

#define CRYPTO_KEY_SIZE   crypto_aead_chacha20poly1305_ietf_KEYBYTES
#define CRYPTO_NONCE_SIZE crypto_aead_chacha20poly1305_ietf_NPUBBYTES
#define CRYPTO_TAG_SIZE   crypto_aead_chacha20poly1305_ietf_ABYTES

enum {
    GDEV_CRYPTO_OK = 0,
    GDEV_CRYPTO_INVALID_ARGUMENT = -1,
    GDEV_CRYPTO_BUFFER_TOO_SMALL = -2,
    GDEV_CRYPTO_AUTH_FAILED = -3,
    GDEV_CRYPTO_INIT_FAILED = -4,
    GDEV_CRYPTO_OPERATION_FAILED = -5
};

/* Call successfully before any other crypto operation. Safe to call again. */
int crypto_init(void);
int crypto_generate_key(unsigned char key[CRYPTO_KEY_SIZE]);


int crypto_encrypt(
    unsigned char *ciphertext,
    size_t ciphertext_capacity,
    size_t *ciphertext_len,
    const unsigned char *plaintext,
    size_t plaintext_len,
    const unsigned char *aad,
    size_t aad_len,
    const unsigned char nonce[CRYPTO_NONCE_SIZE],
    const unsigned char key[CRYPTO_KEY_SIZE]);

int crypto_decrypt(
    unsigned char *plaintext,
    size_t plaintext_capacity,
    size_t *plaintext_len,
    const unsigned char *ciphertext,
    size_t ciphertext_len,
    const unsigned char *aad,
    size_t aad_len,
    const unsigned char nonce[CRYPTO_NONCE_SIZE],
    const unsigned char key[CRYPTO_KEY_SIZE]);
#endif // GDEV_CRYPTO_H
