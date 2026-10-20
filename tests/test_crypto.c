#include "crypto.h"
#include "unity.h"

/* Each test gets a fresh key; tearDown clears it even after a Unity failure. */
static unsigned char key[CRYPTO_KEY_SIZE];

void setUp(void) {
    sodium_memzero(key, sizeof(key));
}

void tearDown(void) {
    sodium_memzero(key, sizeof(key));
}

static void prepare_key(void) {
    TEST_ASSERT_EQUAL_INT(GDEV_CRYPTO_OK, crypto_init());
    TEST_ASSERT_EQUAL_INT(GDEV_CRYPTO_OK, crypto_generate_key(key));
}

static void test_crypto_initialization(void) {
    int result = crypto_init();
    TEST_ASSERT_EQUAL_INT(GDEV_CRYPTO_OK, result);
}

static void test_crypto_key_generation(void) {
    TEST_ASSERT_EQUAL_INT(GDEV_CRYPTO_OK, crypto_init());
    TEST_ASSERT_EQUAL_INT(GDEV_CRYPTO_OK, crypto_generate_key(key));
}

static void test_crypto_round_trip(void) {
    const unsigned char message[] = "Hello! Let's meet at 18:00.";
    const unsigned char aad[] = "message-to-friend:v1";

    // Exclude C string terminators: encryption operates on explicit byte lengths.
    const size_t message_len = sizeof(message) - 1;
    const size_t aad_len = sizeof(aad) - 1;
    unsigned char ciphertext[sizeof(message) - 1 + CRYPTO_TAG_SIZE];
    unsigned char decrypted[sizeof(message) - 1];
    size_t ciphertext_len = 0;
    size_t decrypted_len = 0;

    // One encryption with a fresh key makes this fixed nonce safe for the test.
    unsigned char nonce[CRYPTO_NONCE_SIZE] = {0};
    prepare_key();

    TEST_ASSERT_EQUAL_INT(GDEV_CRYPTO_OK, crypto_encrypt(
        ciphertext,
        sizeof(ciphertext),
        &ciphertext_len,
        message,
        message_len,
        aad,
        aad_len,
        nonce,
        key));

    TEST_ASSERT_EQUAL_size_t(message_len + CRYPTO_TAG_SIZE, ciphertext_len);

    // The recipient uses the same key, nonce and public AAD as the sender.
    TEST_ASSERT_EQUAL_INT(GDEV_CRYPTO_OK, crypto_decrypt(
        decrypted,
        sizeof(decrypted),
        &decrypted_len,
        ciphertext,
        ciphertext_len,
        aad,
        aad_len,
        nonce,
        key));

    TEST_ASSERT_EQUAL_size_t(message_len, decrypted_len);
    // No terminator was encrypted, so compare bytes rather than C strings.
    TEST_ASSERT_EQUAL_MEMORY(message, decrypted, message_len);
}

static void test_crypto_rejects_corrupted_ciphertext(void) {
    const unsigned char message[] = "Hello! Let's meet at 18:00.";
    const unsigned char aad[] = "message-to-friend:v1";
    const size_t message_len = sizeof(message) - 1;
    const size_t aad_len = sizeof(aad) - 1;
    unsigned char ciphertext[sizeof(message) - 1 + CRYPTO_TAG_SIZE];
    unsigned char decrypted[sizeof(message) - 1];
    unsigned char nonce[CRYPTO_NONCE_SIZE] = {0};
    size_t ciphertext_len = 0;
    size_t decrypted_len = 123;

    // Build our own valid ciphertext; do not depend on another test's output.
    prepare_key();
    TEST_ASSERT_EQUAL_INT(GDEV_CRYPTO_OK, crypto_encrypt(
        ciphertext,
        sizeof(ciphertext),
        &ciphertext_len,
        message,
        message_len,
        aad,
        aad_len,
        nonce,
        key));

    // Flip one bit without updating the authentication tag.
    ciphertext[0] ^= 1;

    TEST_ASSERT_EQUAL_INT(GDEV_CRYPTO_AUTH_FAILED, crypto_decrypt(
        decrypted,
        sizeof(decrypted),
        &decrypted_len,
        ciphertext,
        ciphertext_len,
        aad,
        aad_len,
        nonce,
        key));

    // The nonzero sentinel proves the function resets output length on failure.
    TEST_ASSERT_EQUAL_size_t(0, decrypted_len);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_crypto_initialization);
    RUN_TEST(test_crypto_key_generation);
    RUN_TEST(test_crypto_round_trip);
    RUN_TEST(test_crypto_rejects_corrupted_ciphertext);
    return UNITY_END();
}
