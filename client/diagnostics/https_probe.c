#include <conio.h>
#include <stdint.h>

#include "charset.h"
#include "platform/http.h"
#include "platform/uci.h"

#define HTTPS_URL "https://danielkeating.org/api.php?action=status"

static uint8_t body[96];
static uint16_t body_length;
static S64UciCapabilities capabilities;
static S64HttpTransport http;
static S64HttpResult result;
static uint8_t header;
static uint8_t header_created;
static char response_header[S64_HTTP_RESPONSE_HEADER_CAPACITY];
static uint16_t response_header_length;
static uint16_t header_index;

static uint8_t capture_body(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    uint16_t i;

    (void)context;
    for (i = 0u; i < length && body_length < (uint16_t)(sizeof(body) - 1u); ++i) {
        body[body_length++] = data[i];
    }
    body[body_length] = 0u;
    return 1u;
}

static void put_ascii(uint8_t value)
{
    uint8_t native;

    if (s64_charset_ascii_to_native(value, &native)) {
        cputc((char)native);
    } else {
        cputc('?');
    }
}

static void puts_ascii_bounded(const uint8_t* text, uint16_t length)
{
    uint16_t i;

    for (i = 0u; i < length; ++i) {
        uint8_t value = text[i];
        if (value == '\r' || value == '\n') {
            cputs("\r\n");
            if (value == '\r' && i + 1u < length && text[i + 1u] == '\n') {
                ++i;
            }
            continue;
        }
        put_ascii(value);
    }
}

static void show_http_result(const char* label, S64HttpResult result)
{
    cputs(label);
    switch (result) {
        case S64_HTTP_OK:
            cputs("OK\r\n");
            break;
        case S64_HTTP_INVALID_ARGUMENT:
            cputs("INVALID ARG\r\n");
            break;
        case S64_HTTP_TRANSPORT_ERROR:
            cputs("TRANSPORT ERROR\r\n");
            break;
        case S64_HTTP_TARGET_ERROR:
            cputs("TARGET ERROR\r\n");
            break;
        default:
            cputs("UNKNOWN\r\n");
            break;
    }
}

int main(void)
{
    header = 0u;
    header_created = 0u;
    response_header_length = 0u;
    header_index = 0u;

    clrscr();
    cputs("SEE64YOU C64U HTTPS PROBE\r\n\r\n");

    s64_uci_probe_capabilities(&capabilities);
    cputs("UCI:      ");
    cputs(capabilities.interface_present ? "YES\r\n" : "NO\r\n");
    cputs("HTTP $06: ");
    cputs(capabilities.http_target ? "YES\r\n" : "NO\r\n\r\n");

    if (!capabilities.http_target) {
        cputs("HTTP TARGET $06 NOT FOUND.\r\n");
        cputs("PRESS A KEY.\r\n");
        cgetc();
        return 1;
    }

    s64_http_init(&http, s64_uci_execute);
    result = s64_http_free_all(&http);
    if (result != S64_HTTP_OK) {
        show_http_result("RESET: ", result);
        goto finish;
    }

    cputs("CREATING HTTPS REQUEST...\r\n");
    result = s64_http_header_create(
        &http,
        S64_HTTP_GET,
        HTTPS_URL,
        &header
    );
    if (result != S64_HTTP_OK) {
        show_http_result("CREATE: ", result);
        goto finish;
    }
    header_created = 1u;

    body_length = 0u;
    body[0] = 0u;
    response_header[0] = '\0';

    cputs("CONTACTING DANIELKEATING.ORG...\r\n");
    result = s64_http_exchange_raw(
        &http,
        header,
        S64_HTTP_NO_BODY,
        capture_body,
        0,
        response_header,
        (uint16_t)sizeof(response_header),
        &response_header_length
    );
    show_http_result("EXCHANGE: ", result);

    if (response_header_length != 0u) {
        cputs("\r\nRESPONSE:\r\n");
        for (
            header_index = 0u;
            header_index < response_header_length && header_index < 120u;
            ++header_index
        ) {
            char value = response_header[header_index];
            cputc(value);
            if (value == '\n') {
                break;
            }
        }
    }

    if (body_length != 0u) {
        cputs("\r\nBODY:\r\n");
        puts_ascii_bounded(body, body_length);
        cputs("\r\n");
    }

    if (result == S64_HTTP_OK
        && response_header_length >= 12u
        && response_header[0] == 'H'
        && response_header[1] == 'T'
        && response_header[2] == 'T'
        && response_header[3] == 'P') {
        cputs("\r\nDIRECT HTTPS EXCHANGE WORKED.\r\n");
    }

finish:
    if (header_created) {
        (void)s64_http_header_free(&http, header);
    }
    (void)s64_http_free_all(&http);

    cputs("\r\nPRESS A KEY.\r\n");
    cgetc();
    return 0;
}
