#include "crypto.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    // Show the current test even if an assertion aborts with redirected stdout.
    setbuf(stdout, NULL);
    puts("=== RUNNING CRYPTO MODULE TESTS ===");
    const unsigned char message[] = {
        0x01, 0x02, 0x03, 0x04, 0x05,
        0x06, 0x07, 0x08, 0x09, 0x0A
    };
    const unsigned char aad[] = {
        0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };

    unsigned char key[CRYPTO_KEY_SIZE];
    unsigned char nonce[CRYPTO_NONCE_SIZE] = {0};

    unsigned char ciphertext[sizeof(message) + CRYPTO_TAG_SIZE];
    unsigned char decrypted[sizeof(message)];

    size_t ciphertext_len = 0;
    size_t decrypted_len = 0;

    // Initialize the crypto library and generate a key
    puts("[TEST] crypto_init()...");
    assert(crypto_init() == GDEV_CRYPTO_OK);
    puts("  [PASS] crypto_init");

    // Generate a random key for encryption
    puts("[TEST] crypto_generate_key()...");
    assert(crypto_generate_key(key) == GDEV_CRYPTO_OK);
    puts("  [PASS] crypto_generate_key");

    // Encrypt the message
    puts("[TEST] crypto_encrypt() and output length...");
    assert(crypto_encrypt(
        ciphertext,
        sizeof(ciphertext),
        &ciphertext_len,
        message,
        sizeof(message),
        aad,
        sizeof(aad),
        nonce,
        key) == GDEV_CRYPTO_OK);
    
    assert(ciphertext_len == sizeof(message) + CRYPTO_TAG_SIZE);
    printf("  [PASS] encrypted %zu bytes into %zu bytes (including tag)\n",
           sizeof(message), ciphertext_len);
    
    // Decrypt the message
    puts("[TEST] crypto_decrypt() and original data recovery...");
    assert(crypto_decrypt(
        decrypted,
        sizeof(decrypted),
        &decrypted_len,
        ciphertext,
        ciphertext_len,
        aad,
        sizeof(aad),
        nonce,
        key) == GDEV_CRYPTO_OK);

    assert(decrypted_len == sizeof(message));
    // Verify that the decrypted message matches the original message
    assert(memcmp(message, decrypted, sizeof(message)) == 0);
    puts("  [PASS] decrypted data matches the original message");

    // Test authentication failure with corrupted ciphertext
    puts("[TEST] corrupted ciphertext rejection...");
    ciphertext[0] ^= 1;
    decrypted_len = 123;

    assert(crypto_decrypt(
        decrypted,
        sizeof(decrypted),
        &decrypted_len,
        ciphertext,
        ciphertext_len,
        aad,
        sizeof(aad),
        nonce,
        key) == GDEV_CRYPTO_AUTH_FAILED);
    
    assert(decrypted_len == 0);
    puts("  [PASS] corrupted ciphertext rejected; output length is zero");
    puts("=== ALL CRYPTO TESTS PASSED ===");
    return 0;
}
