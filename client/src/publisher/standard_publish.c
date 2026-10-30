#include "publisher/standard_publish.h"
#include "publisher/direct_auth.h"
#include "charset.h"

#include <string.h>

static int add_native(S64HmacSha256* hmac, const char* text)
{
    uint8_t ascii;
    while (*text) {
        if (!s64_charset_native_to_ascii((uint8_t)*text++, &ascii)) return 0;
        s64_hmac_sha256_update(hmac, &ascii, 1u);
    }
    return 1;
}

static int add_field(S64HmacSha256* hmac, const char* text)
{
    static const uint8_t lf = 10u;
    if (!add_native(hmac, text)) return 0;
    s64_hmac_sha256_update(hmac, &lf, 1u);
    return 1;
}

static void decimal(uint32_t size, char result[11])
{
    char digits[10];
    uint8_t count = 0u, i = 0u;
    do {
        digits[count++] = (char)('0' + size % 10u);
        size /= 10u;
    } while (size);
    while (count) result[i++] = digits[--count];
    result[i] = 0;
}

int s64_publish_sign_begin(S64PublishSigner* signer,
    const uint8_t key[S64_HMAC_SHA256_KEY_SIZE],
    const char* op, const char* id, const char* challenge,
    const char* name, uint32_t size)
{
    char size_text[11];
    if (!signer) return 0;
    memset(signer, 0, sizeof(*signer));
    if (!key || !op || !id || !challenge || !name
        || !s64_publish_id_valid(id)
        || (strcmp(op, "put") != 0 && strcmp(op, "list") != 0)
        || (strcmp(op, "put") == 0 ? !s64_publish_name_valid(name)
                                 : (name[0] != 0 || size != 0u))
        || !s64_publish_challenge_valid(challenge)) return 0;
    decimal(size, size_text);
    s64_hmac_sha256_init(&signer->hmac, key);
    if (!add_field(&signer->hmac, "8tml-publish/1")
        || !add_field(&signer->hmac, op)
        || !add_field(&signer->hmac, id)
        || !add_field(&signer->hmac, challenge)
        || !add_field(&signer->hmac, name)
        || !add_field(&signer->hmac, size_text)) {
        s64_publish_sign_cancel(signer);
        return 0;
    }
    signer->expected = size;
    signer->active = 1u;
    return 1;
}

int s64_publish_sign_update(S64PublishSigner* signer,
    const uint8_t* bytes, uint16_t count)
{
    if (!signer || !signer->active || (count && !bytes)
        || (uint32_t)count > signer->expected - signer->written) return 0;
    if (count) s64_hmac_sha256_update(&signer->hmac, bytes, count);
    signer->written += count;
    return 1;
}

int s64_publish_sign_finish(S64PublishSigner* signer, char signature[65])
{
    uint8_t digest[S64_SHA256_DIGEST_SIZE];
    if (!signer || !signer->active || !signature
        || signer->written != signer->expected) return 0;
    s64_hmac_sha256_final(&signer->hmac, digest);
    s64_direct_digest_hex(digest, signature);
    memset(digest, 0, sizeof(digest));
    s64_publish_sign_cancel(signer);
    return 1;
}

void s64_publish_sign_cancel(S64PublishSigner* signer)
{
    if (signer) memset(signer, 0, sizeof(*signer));
}
