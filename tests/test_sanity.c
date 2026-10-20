#include <sodium.h>
#include "unity.h"

void setUp(void) {
    // This function is called before each test case.
}

void tearDown(void) {
    // This function is called after each test case.
}

static void test_sodium_initialization(void) {
    // Test that sodium_init() returns 0 or 1, indicating successful initialization.
    int result = sodium_init();
    TEST_ASSERT_TRUE(result >= 0);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sodium_initialization);
    return UNITY_END();
}