#include <conio.h>
#include <stdint.h>

#include "charset.h"
#include "platform/network.h"
#include "platform/uci.h"

#define PROBE_HOST "danielkeating.org"
#define PROBE_PORT 80u
#define PROBE_READ_SIZE 256u
#define PROBE_ATTEMPTS 120u

static uint8_t response[512];
static uint16_t response_length;

static void put_ascii(uint8_t value)
{
    uint8_t native;

    if (s64_charset_ascii_to_native(value, &native)) {
        cputc((char)native);
    } else {
        cputc('?');
    }
}

static uint8_t capture_response(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    uint16_t i;

    (void)context;
    for (i = 0u; i < length && response_length < (uint16_t)sizeof(response); ++i) {
        response[response_length++] = data[i];
    }
    return 1u;
}

static void show_response(void)
{
    uint16_t i;
    uint8_t lines = 0u;

    for (i = 0u; i < response_length && lines < 12u; ++i) {
        uint8_t value = response[i];

        if (value == '\r') {
            continue;
        }
        if (value == '\n') {
            cputs("\r\n");
            ++lines;
            continue;
        }
        if (value >= 0x20u && value <= 0x7eu) {
            put_ascii(value);
        }
    }
    cputs("\r\n");
}

int main(void)
{
    static const char request[] =
        "GET /api.php?action=status HTTP/1.1\r\n"
        "Host: danielkeating.org\r\n"
        "Connection: close\r\n"
        "User-Agent: See64You-C64U-Probe/1\r\n"
        "\r\n";
    S64NetTransport network;
    S64NetResult result;
    uint8_t handle = 0u;
    uint8_t handle_open = 0u;
    uint8_t attempt;
    uint16_t received;

    clrscr();
    cputs("SEE64YOU DIRECT SITE PROBE\r\n\r\n");

    if (!s64_uci_probe_target(0x03u)) {
        cputs("NETWORK TARGET $03 NOT FOUND.\r\n");
        goto finish;
    }

    s64_net_init(&network, s64_uci_execute);

    cputs("OPENING DANIELKEATING.ORG:80...\r\n");
    result = s64_net_open_tcp(&network, PROBE_HOST, PROBE_PORT, &handle);
    if (result != S64_NET_OK) {
        cputs("TCP OPEN FAILED.\r\n");
        goto finish;
    }
    handle_open = 1u;

    cputs("SENDING UNAUTHENTICATED GET...\r\n");
    result = s64_net_write_text(
        &network,
        handle,
        request,
        (uint16_t)(sizeof(request) - 1u)
    );
    if (result != S64_NET_OK) {
        cputs("TCP WRITE FAILED.\r\n");
        goto finish;
    }

    response_length = 0u;
    cputs("WAITING FOR REPLY...\r\n\r\n");

    for (attempt = 0u; attempt < PROBE_ATTEMPTS; ++attempt) {
        result = s64_net_read(
            &network,
            handle,
            PROBE_READ_SIZE,
            capture_response,
            0,
            &received
        );

        if (result == S64_NET_NO_DATA) {
            continue;
        }
        if (result == S64_NET_OK && response_length < sizeof(response)) {
            continue;
        }
        if (result == S64_NET_CLOSED || response_length >= sizeof(response)) {
            break;
        }

        cputs("TCP READ FAILED.\r\n");
        goto finish;
    }

    if (response_length == 0u) {
        cputs("NO RESPONSE RECEIVED.\r\n");
    } else {
        cputs("SERVER RESPONSE:\r\n");
        show_response();
    }

finish:
    if (handle_open) {
        (void)s64_net_close(&network, handle);
    }
    cputs("\r\nPRESS A KEY.\r\n");
    cgetc();
    return 0;
}
