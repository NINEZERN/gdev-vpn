#include "crypto.h"


int crypto_init(void) {
    if (sodium_init() < 0) {
        return GDEV_CRYPTO_INIT_FAILED;
    }
    return GDEV_CRYPTO_OK;
}


int crypto_generate_key(unsigned char key[CRYPTO_KEY_SIZE]) {
    if (key == NULL) {
        return GDEV_CRYPTO_INVALID_ARGUMENT;
    }
    crypto_aead_chacha20poly1305_ietf_keygen(key);
    return GDEV_CRYPTO_OK;
}

int crypto_encrypt(
    unsigned char *ciphertext,
    size_t ciphertext_capacity,
    size_t *ciphertext_len,
    const unsigned char *plaintext,
    size_t plaintext_len,
    const unsigned char *aad,
    size_t aad_len,
    const unsigned char nonce[CRYPTO_NONCE_SIZE],
    const unsigned char key[CRYPTO_KEY_SIZE]) {

    if (ciphertext_len == NULL) {
        return GDEV_CRYPTO_INVALID_ARGUMENT;
    }

    // Initialize output length to 0
    *ciphertext_len = 0; 

    if (ciphertext == NULL || 
        nonce == NULL || 
        key == NULL ||
        (plaintext == NULL && plaintext_len > 0) ||
        (aad == NULL && aad_len > 0)) {
            return GDEV_CRYPTO_INVALID_ARGUMENT;
        }
    
    // Check if the plaintext length exceeds the maximum allowed size for ChaCha20-Poly1305
    if (plaintext_len > 
        crypto_aead_chacha20poly1305_ietf_MESSAGEBYTES_MAX) {
        return GDEV_CRYPTO_INVALID_ARGUMENT;
    }

    // Check if the ciphertext buffer is large enough to hold the encrypted data
    if (ciphertext_capacity < CRYPTO_TAG_SIZE ||
        plaintext_len > ciphertext_capacity - CRYPTO_TAG_SIZE) {
        return GDEV_CRYPTO_BUFFER_TOO_SMALL;
    }

    unsigned long long written_len = 0;

    // placeholder for empty message to avoid passing NULL pointer to crypto_aead_chacha20poly1305_ietf_encrypt
    const unsigned char empty_message = 0;

    int status = crypto_aead_chacha20poly1305_ietf_encrypt(
        ciphertext,
        &written_len,
        plaintext ? plaintext : &empty_message,
        (unsigned long long)plaintext_len,
        aad ? aad : &empty_message,
        (unsigned long long)aad_len,
        NULL, // No secret nonce
        nonce,
        key
    );

    if (status != 0) {
        return GDEV_CRYPTO_OPERATION_FAILED;
    }

    *ciphertext_len = (size_t)written_len;

    return GDEV_CRYPTO_OK;
}


int crypto_decrypt(
    unsigned char *plaintext,
    size_t plaintext_capacity,
    size_t *plaintext_len,
    const unsigned char *ciphertext,
    size_t ciphertext_len,
    const unsigned char *aad,
    size_t aad_len,
    const unsigned char nonce[CRYPTO_NONCE_SIZE],
    const unsigned char key[CRYPTO_KEY_SIZE]) {

    if (plaintext_len == NULL) {
        return GDEV_CRYPTO_INVALID_ARGUMENT;
    }

    // Initialize output length to 0
    *plaintext_len = 0; 

    if (plaintext == NULL || 
        nonce == NULL || 
        key == NULL ||
        (ciphertext == NULL) ||
        (aad == NULL && aad_len > 0)) {
            return GDEV_CRYPTO_INVALID_ARGUMENT;
        }
    
    // Check if the ciphertext length is less than the tag size
    if (ciphertext_len < CRYPTO_TAG_SIZE) {
        return GDEV_CRYPTO_INVALID_ARGUMENT;
    }

    size_t expected_plaintext_len = ciphertext_len - CRYPTO_TAG_SIZE;

    if (expected_plaintext_len > crypto_aead_chacha20poly1305_ietf_MESSAGEBYTES_MAX) {
        return GDEV_CRYPTO_INVALID_ARGUMENT;
    }

    // Check if the plaintext buffer is large enough to hold the decrypted data
    if (plaintext_capacity < expected_plaintext_len) {
        return GDEV_CRYPTO_BUFFER_TOO_SMALL;
    }


    unsigned long long written_len = 0;

    // placeholder for empty message to avoid passing NULL pointer to crypto_aead_chacha20poly1305_ietf_decrypt
    const unsigned char empty_message = 0;

    int status = crypto_aead_chacha20poly1305_ietf_decrypt(
        plaintext,
        &written_len,
        NULL, // No secret nonce
        ciphertext,
        (unsigned long long)ciphertext_len,
        aad ? aad : &empty_message,
        (unsigned long long)aad_len,
        nonce,
        key
    );

    if (status != 0) {
        // Clear the plaintext buffer on authentication failure
        sodium_memzero(plaintext, expected_plaintext_len);
        return GDEV_CRYPTO_AUTH_FAILED;
    }

    *plaintext_len = (size_t)written_len;

    return GDEV_CRYPTO_OK;
}