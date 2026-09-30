#include <conio.h>
#include <stdint.h>
#include <string.h>

#include "charset.h"
#include "crypto/sha256.h"
#include "platform/dos.h"
#include "platform/network.h"
#include "platform/turbo.h"
#include "platform/uci.h"
#include "publisher/direct_auth.h"
#include "publisher/settings_store.h"

#define PROBE_HOST "danielkeating.org"
#define PROBE_PORT 80u
#define PROBE_READ_SIZE 256u
#define PROBE_ATTEMPTS 160u
#define RESPONSE_CAPACITY 640u

static uint8_t response[RESPONSE_CAPACITY];
static uint16_t response_length;
static S64Dos dos;
static S64SettingsExchange settings;
static S64NetTransport network;
static S64Sha256 sha;
static S64TurboGuard turbo;
static uint8_t key[32];
static uint8_t body_digest[32];
static char challenge[33];
static char signature[65];
static const char* request_parts[7];
static uint16_t request_lengths[7];


static uint8_t capture_response(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    uint16_t i;

    (void)context;
    for (i = 0u; i < length && response_length < RESPONSE_CAPACITY; ++i) {
        response[response_length++] = data[i];
    }
    return 1u;
}

static uint8_t request_and_read(
    S64NetTransport* network,
    const char* const* parts,
    const uint16_t* lengths,
    uint8_t part_count
)
{
    S64NetResult result;
    uint8_t handle = 0u;
    uint8_t open = 0u;
    uint8_t i;
    uint8_t attempt;
    uint16_t received;

    result = s64_net_open_tcp(network, PROBE_HOST, PROBE_PORT, &handle);
    if (result != S64_NET_OK) return 0u;
    open = 1u;

    for (i = 0u; i < part_count; ++i) {
        result = s64_net_write_text(
            network,
            handle,
            parts[i],
            lengths[i]
        );
        if (result != S64_NET_OK) goto fail;
    }

    response_length = 0u;
    for (attempt = 0u; attempt < PROBE_ATTEMPTS; ++attempt) {
        result = s64_net_read(
            network,
            handle,
            PROBE_READ_SIZE,
            capture_response,
            0,
            &received
        );

        if (result == S64_NET_NO_DATA) continue;
        if (result == S64_NET_OK && response_length < RESPONSE_CAPACITY) {
            continue;
        }
        if (result == S64_NET_CLOSED || response_length >= RESPONSE_CAPACITY) {
            break;
        }
        goto fail;
    }

    (void)s64_net_close(network, handle);
    return response_length != 0u;

fail:
    if (open) (void)s64_net_close(network, handle);
    return 0u;
}

static uint8_t wire_prefix_native(const char* native_text)
{
    uint16_t i = 0u;
    uint8_t ascii;

    if (native_text == 0) return 0u;
    while (native_text[i] != '\0') {
        if (i >= response_length
            || !s64_charset_native_to_ascii((uint8_t)native_text[i], &ascii)
            || response[i] != ascii) {
            return 0u;
        }
        ++i;
    }
    return 1u;
}

static uint8_t response_status_200(void)
{
    return wire_prefix_native("HTTP/1.1 200");
}

static uint8_t extract_challenge(char challenge[33])
{
    static const char marker[] = "\"challenge\":\"";
    uint16_t i;
    uint8_t j;
    uint8_t native;
    uint8_t expected;
    const uint8_t marker_length = (uint8_t)(sizeof(marker) - 1u);

    if (response_length < (uint16_t)(marker_length + 32u)) return 0u;

    for (i = 0u; i + marker_length + 32u <= response_length; ++i) {
        for (j = 0u; j < marker_length; ++j) {
            if (!s64_charset_native_to_ascii((uint8_t)marker[j], &expected)
                || response[i + j] != expected) {
                break;
            }
        }
        if (j != marker_length) continue;

        for (j = 0u; j < 32u; ++j) {
            uint8_t ascii = response[i + marker_length + j];
            if (!((ascii >= 0x30u && ascii <= 0x39u)
                || (ascii >= 0x61u && ascii <= 0x66u))) {
                return 0u;
            }
            if (!s64_charset_ascii_to_native(ascii, &native)) return 0u;
            challenge[j] = (char)native;
        }
        challenge[32] = '\0';
        return 1u;
    }

    return 0u;
}

static void show_response_prefix(void)
{
    uint16_t i;
    uint8_t lines = 0u;

    for (i = 0u; i < response_length && lines < 8u; ++i) {
        uint8_t value = response[i];
        uint8_t native;

        if (value == '\r') continue;
        if (value == '\n') {
            cputs("\r\n");
            ++lines;
            continue;
        }
        if (value >= 0x20u && value <= 0x7eu
            && s64_charset_ascii_to_native(value, &native)) {
            cputc((char)native);
        }
    }
    cputs("\r\n");
}

int main(void)
{
    static const char challenge_request[] =
        "GET /api.php?action=challenge HTTP/1.1\r\n"
        "Host: danielkeating.org\r\n"
        "Connection: close\r\n"
        "User-Agent: See64You-C64U-DirectAuth/1\r\n"
        "\r\n";
    static const char request_line[] =
        "GET /api.php?action=settings HTTP/1.1\r\n";
    static const char host_header[] =
        "Host: danielkeating.org\r\n";
    static const char challenge_header[] =
        "X-See64You-Challenge: ";
    static const char signature_header[] =
        "\r\nX-See64You-Signature: ";
    static const char tail_headers[] =
        "\r\nConnection: close\r\n"
        "User-Agent: See64You-C64U-DirectAuth/1\r\n"
        "\r\n";
    static const char signed_uri[] = "/api.php?action=settings";
    static const char method[] = "GET";
    S64SettingsStoreResult settings_result;

    clrscr();
    cputs("SEE64YOU DIRECT AUTH PROBE\r\n\r\n");

    if (!s64_uci_probe_target(S64_DOS_TARGET_PRIMARY)) {
        cputs("ULTIMATE DOS $01 NOT FOUND.\r\n");
        goto finish;
    }
    if (!s64_uci_probe_target(0x03u)) {
        cputs("NETWORK TARGET $03 NOT FOUND.\r\n");
        goto finish;
    }

    s64_dos_init(&dos, s64_uci_execute, S64_DOS_TARGET_PRIMARY);
    if (s64_dos_change_dir(&dos, "/sd/") != S64_DOS_OK) {
        cputs("CANNOT SELECT /SD/.\r\n");
        goto finish;
    }
    settings_result = s64_settings_store_load(&dos, &settings);
    if (settings_result != S64_SETTINGS_STORE_OK) {
        cputs("CANNOT LOAD SEE64YOU.CFG.\r\n");
        goto finish;
    }
    if (settings.direct_key[0] == '\0') {
        cputs("DIRECT KEY NOT CONFIGURED.\r\n");
        cputs("SET IT IN PUBLISHING SETTINGS.\r\n");
        goto finish;
    }
    if (!s64_direct_key_parse(settings.direct_key, key)) {
        cputs("DIRECT KEY INVALID.\r\n");
        goto finish;
    }

    s64_net_init(&network, s64_uci_execute);

    cputs("REQUESTING ONE-TIME CHALLENGE...\r\n");
    request_parts[0] = challenge_request;
    request_lengths[0] = (uint16_t)(sizeof(challenge_request) - 1u);
    if (!request_and_read(&network, request_parts, request_lengths, 1u)
        || !response_status_200()
        || !extract_challenge(challenge)) {
        cputs("CHALLENGE REQUEST FAILED.\r\n");
        show_response_prefix();
        goto finish;
    }
    cputs("CHALLENGE RECEIVED.\r\n");

    (void)s64_turbo_begin_max(&turbo);
    s64_sha256_init(&sha);
    s64_sha256_final(&sha, body_digest);
    if (!s64_direct_sign(
        key,
        challenge,
        method,
        signed_uri,
        body_digest,
        signature
    )) {
        s64_turbo_restore(&turbo);
        cputs("SIGNATURE BUILD FAILED.\r\n");
        goto finish;
    }
    s64_turbo_restore(&turbo);

    cputs("SENDING AUTHENTICATED GET...\r\n");
    request_parts[0] = request_line;
    request_lengths[0] = (uint16_t)(sizeof(request_line) - 1u);
    request_parts[1] = host_header;
    request_lengths[1] = (uint16_t)(sizeof(host_header) - 1u);
    request_parts[2] = challenge_header;
    request_lengths[2] = (uint16_t)(sizeof(challenge_header) - 1u);
    request_parts[3] = challenge;
    request_lengths[3] = 32u;
    request_parts[4] = signature_header;
    request_lengths[4] = (uint16_t)(sizeof(signature_header) - 1u);
    request_parts[5] = signature;
    request_lengths[5] = 64u;
    request_parts[6] = tail_headers;
    request_lengths[6] = (uint16_t)(sizeof(tail_headers) - 1u);

    if (!request_and_read(&network, request_parts, request_lengths, 7u)) {
        cputs("AUTHENTICATED REQUEST FAILED.\r\n");
        goto finish;
    }
    if (!response_status_200()) {
        cputs("AUTH REJECTED OR SERVER ERROR.\r\n");
        show_response_prefix();
        goto finish;
    }

    cputs("AUTHENTICATED GET PASSED.\r\n\r\n");
    show_response_prefix();

finish:
    memset(key, 0, sizeof(key));
    memset(signature, 0, sizeof(signature));
    cputs("\r\nPRESS A KEY.\r\n");
    cgetc();
    return 0;
}
