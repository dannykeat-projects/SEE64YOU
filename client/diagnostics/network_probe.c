#include <conio.h>
#include <stdint.h>

#include "charset.h"
#include "platform/network.h"
#include "platform/uci.h"

#define PROBE_HOST "example.com"
#define PROBE_PORT 80u
#define PROBE_READ_SIZE 256u
#define PROBE_ATTEMPTS 100u

static char response_line[40];
static uint8_t response_length;
static uint8_t response_complete;

static void put_ascii(uint8_t value)
{
    uint8_t native;

    if (s64_charset_ascii_to_native(value, &native)) {
        cputc((char)native);
    } else {
        cputc('?');
    }
}

static void puts_ascii(const char* text)
{
    while (*text != '\0') {
        put_ascii((uint8_t)*text++);
    }
}

static uint8_t capture_status_line(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    uint16_t i;

    (void)context;
    for (i = 0u; i < length && !response_complete; ++i) {
        uint8_t value = data[i];

        if (value == 0x0Au) {
            response_complete = 1u;
        } else if (value != 0x0Du
            && response_length + 1u < (uint8_t)sizeof(response_line)) {
            response_line[response_length++] = (char)value;
            response_line[response_length] = '\0';
        }
    }
    return 1u;
}

static void show_capability(const char* label, uint8_t available)
{
    cputs(label);
    cputs(available ? "YES\r\n" : "NO\r\n");
}

static uint8_t looks_like_http(void)
{
    return response_length >= 5u
        && (uint8_t)response_line[0] == 0x48u
        && (uint8_t)response_line[1] == 0x54u
        && (uint8_t)response_line[2] == 0x54u
        && (uint8_t)response_line[3] == 0x50u
        && (uint8_t)response_line[4] == 0x2Fu;
}

int main(void)
{
    static const char request[] =
        "GET / HTTP/1.0\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n"
        "\r\n";
    S64UciCapabilities capabilities;
    S64NetTransport network;
    S64NetResult result;
    uint8_t handle = 0u;
    uint8_t handle_open = 0u;
    uint8_t attempt;
    uint16_t received;

    clrscr();
    cputs("SEE64YOU C64U NETWORK PROBE\r\n\r\n");

    s64_uci_probe_capabilities(&capabilities);
    show_capability("UCI:       ", capabilities.interface_present);
    show_capability("DOS $01:   ", capabilities.dos_target);
    show_capability("NET $03:   ", capabilities.network_target);
    show_capability("HTTP $06:  ", capabilities.http_target);
    cputs("\r\n");

    if (!capabilities.network_target) {
        cputs("NETWORK TARGET $03 NOT FOUND.\r\n");
        cputs("PRESS A KEY.\r\n");
        cgetc();
        return 1;
    }

    s64_net_init(&network, s64_uci_execute);

    cputs("OPENING EXAMPLE.COM:80...\r\n");
    result = s64_net_open_tcp(&network, PROBE_HOST, PROBE_PORT, &handle);
    if (result != S64_NET_OK) {
        cputs("TCP OPEN FAILED.\r\n");
        cputs("PRESS A KEY.\r\n");
        cgetc();
        return 2;
    }
    handle_open = 1u;

    cputs("SENDING HTTP REQUEST...\r\n");
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

    cputs("WAITING FOR REPLY...\r\n");
    response_length = 0u;
    response_complete = 0u;
    response_line[0] = '\0';

    for (attempt = 0u; attempt < PROBE_ATTEMPTS && !response_complete; ++attempt) {
        result = s64_net_read(
            &network,
            handle,
            PROBE_READ_SIZE,
            capture_status_line,
            0,
            &received
        );

        if (result == S64_NET_OK || result == S64_NET_NO_DATA) {
            continue;
        }
        if (result == S64_NET_CLOSED) {
            break;
        }

        cputs("TCP READ FAILED.\r\n");
        goto finish;
    }

    if (looks_like_http()) {
        cputs("SUCCESS: ");
        puts_ascii(response_line);
        cputs("\r\n");
        cputs("$03 TCP + RAW HTTP WORKS.\r\n");
    } else if (response_length != 0u) {
        cputs("REPLY: ");
        puts_ascii(response_line);
        cputs("\r\n");
        cputs("UNEXPECTED RESPONSE.\r\n");
    } else {
        cputs("NO HTTP REPLY RECEIVED.\r\n");
    }

finish:
    if (handle_open) {
        (void)s64_net_close(&network, handle);
    }
    cputs("\r\nPRESS A KEY.\r\n");
    cgetc();
    return 0;
}
