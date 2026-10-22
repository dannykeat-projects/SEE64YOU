#include <conio.h>
#include <stdint.h>

#include "charset.h"
#include "platform/http.h"
#include "platform/uci.h"

#define HTTPS_URL "https://danielkeating.org/api.php?action=status"

static uint8_t body[96];
static uint16_t body_length;
static uint8_t identify[48];
static uint16_t identify_length;
static char raw_status[80];
static S64UciResult uci_result;
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

static uint8_t capture_identify(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    uint16_t i;

    (void)context;
    for (
        i = 0u;
        i < length && identify_length < (uint16_t)(sizeof(identify) - 1u);
        ++i
    ) {
        identify[identify_length++] = data[i];
    }
    identify[identify_length] = 0u;
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

static void show_uci_result(S64UciResult value)
{
    switch (value) {
        case S64_UCI_OK:
            cputs("OK");
            break;
        case S64_UCI_UNAVAILABLE:
            cputs("UNAVAILABLE");
            break;
        case S64_UCI_TIMEOUT:
            cputs("TIMEOUT");
            break;
        case S64_UCI_PROTOCOL_ERROR:
            cputs("PROTOCOL ERROR");
            break;
        case S64_UCI_COMMAND_TOO_LARGE:
            cputs("COMMAND TOO LARGE");
            break;
        case S64_UCI_CONSUMER_ABORTED:
            cputs("CONSUMER ABORTED");
            break;
        default:
            cputs("UNKNOWN");
            break;
    }
}

static void show_http_result(const char* label, S64HttpResult value)
{
    cputs(label);
    switch (value) {
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

static uint8_t raw_http_success(void)
{
    return raw_status[0] == '0'
        && raw_status[1] == '0'
        && (raw_status[2] == '0' || raw_status[2] == ',');
}

static void run_raw_command(
    const uint8_t* command,
    uint16_t command_length,
    uint8_t want_identify
)
{
    S64UciCommandPart part;
    S64UciReply reply;

    identify_length = 0u;
    identify[0] = 0u;
    raw_status[0] = '\0';

    part.data = command;
    part.length = command_length;
    part.flags = 0u;

    reply.data_sink = want_identify ? capture_identify : 0;
    reply.data_context = 0;
    reply.status = raw_status;
    reply.status_capacity = (uint16_t)sizeof(raw_status);
    reply.status_length = 0u;

    uci_result = s64_uci_execute(&part, 1u, &reply);
}

int main(void)
{
    static const uint8_t identify_command[] = {0x06u, 0x01u};
    static const uint8_t free_all_command[] = {0x06u, 0x10u};

    header = 0u;
    header_created = 0u;
    response_header_length = 0u;
    header_index = 0u;

    clrscr();
    cputs("SEE64YOU C64U HTTP TARGET PROBE\r\n\r\n");

    cputs("$06 IDENTIFY...\r\n");
    run_raw_command(
        identify_command,
        (uint16_t)sizeof(identify_command),
        1u
    );
    cputs("EXEC: ");
    show_uci_result(uci_result);
    cputs("\r\nID: ");
    if (identify_length != 0u) {
        puts_ascii_bounded(identify, identify_length);
    } else {
        cputs("(NONE)");
    }
    cputs("\r\nSTATUS: ");
    cputs(raw_status[0] != '\0' ? raw_status : "(NONE)");
    cputs("\r\n\r\n");

    cputs("$06 FREE_ALL $10...\r\n");
    run_raw_command(
        free_all_command,
        (uint16_t)sizeof(free_all_command),
        0u
    );
    cputs("EXEC: ");
    show_uci_result(uci_result);
    cputs("\r\nSTATUS: ");
    cputs(raw_status[0] != '\0' ? raw_status : "(NONE)");
    cputs("\r\n\r\n");

    if (uci_result != S64_UCI_OK || !raw_http_success()) {
        cputs("STOPPING BEFORE HTTPS.\r\n");
        cputs("PHOTOGRAPH THIS SCREEN.\r\n");
        goto finish;
    }

    s64_http_init(&http, s64_uci_execute);

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

finish:
    if (header_created) {
        (void)s64_http_header_free(&http, header);
    }

    cputs("\r\nPRESS A KEY.\r\n");
    cgetc();
    return 0;
}
