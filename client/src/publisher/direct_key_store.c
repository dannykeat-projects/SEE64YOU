#include "publisher/standalone_direct.h"
#include "publisher/direct_auth.h"

#include <string.h>

uint8_t s64_direct_key_bytes[S64_HMAC_SHA256_KEY_SIZE];
uint8_t s64_direct_key_ready;

int s64_direct_publisher_key(const char* hex)
{
    memset(s64_direct_key_bytes, 0, sizeof(s64_direct_key_bytes));
    s64_direct_key_ready = (uint8_t)s64_direct_key_parse(
        hex, s64_direct_key_bytes);
    return s64_direct_key_ready;
}
