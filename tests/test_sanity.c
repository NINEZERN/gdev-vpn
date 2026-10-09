#include <sodium.h>
#include <stdio.h>
#include <assert.h>

int main(void) {
    assert(sodium_init() >= 0);
    printf("[OK] Sodium library initialized successfully (version %s)\n", sodium_version_string());
    return 0;
}