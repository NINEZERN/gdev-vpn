#include <sodium.h>
#include <assert.h>
#include <stdio.h>


int main(void) {
    // Check if libsodium is initialized correctly
    assert(sodium_init() >= 0);
    printf("[OK] libsodium initialized succesfully (version %s)\n", sodium_version_string());
    return 0;
}
