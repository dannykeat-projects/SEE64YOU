#ifndef SEE64YOU_STANDARD_PUBLISH_H
#define SEE64YOU_STANDARD_PUBLISH_H

#include <stdint.h>
#include "crypto/sha256.h"

/* 8TML publishing API 1 test draft, 2026-10-26, section 7.
   This is not the legacy SEE64YOU direct-auth wire format. */
typedef struct {
    S64HmacSha256 hmac;
    uint32_t expected;
    uint32_t written;
    uint8_t active;
} S64PublishSigner;

int s64_publish_id_valid(const char* id);
int s64_publish_name_valid(const char* name);

/* List uses name="" and size=0. Put requires a valid nonempty name.
   All text arguments are native charset; file content is wire bytes. */
int s64_publish_sign_begin(S64PublishSigner* signer,
    const uint8_t key[S64_HMAC_SHA256_KEY_SIZE],
    const char* op, const char* id, const char* challenge,
    const char* name, uint32_t size);
int s64_publish_sign_update(S64PublishSigner* signer,
    const uint8_t* bytes, uint16_t count);
int s64_publish_sign_finish(S64PublishSigner* signer, char signature[65]);
void s64_publish_sign_cancel(S64PublishSigner* signer);

#endif
