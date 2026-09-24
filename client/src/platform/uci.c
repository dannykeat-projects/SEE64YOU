#include "uci.h"

#include <time.h>

#define S64_UCI_CONTROL      (*(volatile uint8_t*)0xDF1Cu)
#define S64_UCI_COMMAND      (*(volatile uint8_t*)0xDF1Du)
#define S64_UCI_RESPONSE     (*(volatile uint8_t*)0xDF1Eu)
#define S64_UCI_STATUS_DATA  (*(volatile uint8_t*)0xDF1Fu)

#define S64_UCI_IDENT_MASK       0x7Fu
#define S64_UCI_IDENT_VALUE      0x49u

#define S64_UCI_PUSH_COMMAND     0x01u
#define S64_UCI_ACCEPT_DATA      0x02u
#define S64_UCI_ABORT            0x04u

#define S64_UCI_STATE_MASK       0x30u
#define S64_UCI_STATE_IDLE       0x00u
#define S64_UCI_STATE_LAST       0x20u
#define S64_UCI_STATE_MORE       0x30u
#define S64_UCI_STATUS_AVAILABLE 0x40u
#define S64_UCI_DATA_AVAILABLE   0x80u

#define S64_UCI_TARGET_HTTP      0x06u
#define S64_UCI_CMD_IDENTIFY     0x01u

#define S64_UCI_WAIT_SECONDS     15UL
#define S64_UCI_STREAM_BUFFER    128u

static uint8_t stream_buffer[S64_UCI_STREAM_BUFFER];

static uint8_t s64_uci_interface_present(void)
{
    /* Firmware 3.15 may clear bit 7 while a completion IRQ is active. */
    return (uint8_t)(S64_UCI_COMMAND & S64_UCI_IDENT_MASK) == S64_UCI_IDENT_VALUE;
}

static uint8_t s64_uci_wait_expired(clock_t started)
{
    clock_t elapsed = (clock_t)(clock() - started);
    return elapsed >= (clock_t)(S64_UCI_WAIT_SECONDS * CLOCKS_PER_SEC);
}

static uint8_t s64_uci_wait_until_idle(void)
{
    clock_t started = clock();

    while (!s64_uci_wait_expired(started)) {
        if ((S64_UCI_CONTROL & S64_UCI_STATE_MASK) == S64_UCI_STATE_IDLE) {
            return 1u;
        }
    }

    return 0u;
}

static uint8_t s64_uci_wait_for_response(void)
{
    clock_t started = clock();

    while (!s64_uci_wait_expired(started)) {
        uint8_t state = (uint8_t)(S64_UCI_CONTROL & S64_UCI_STATE_MASK);
        if (state == S64_UCI_STATE_LAST || state == S64_UCI_STATE_MORE) {
            return state;
        }
    }

    return S64_UCI_STATE_IDLE;
}

static uint8_t s64_uci_command_length(
    const S64UciCommandPart* parts,
    uint8_t part_count,
    uint16_t* total
)
{
    uint8_t part;
    uint16_t length = 0u;

    if (parts == 0 || part_count == 0u) {
        return 0u;
    }

    for (part = 0u; part < part_count; ++part) {
        if (parts[part].length != 0u && parts[part].data == 0) {
            return 0u;
        }
        if (parts[part].length > (uint16_t)(S64_UCI_COMMAND_CAPACITY - length)) {
            return 0u;
        }
        length = (uint16_t)(length + parts[part].length);
    }

    if (length == 0u) {
        return 0u;
    }

    *total = length;
    return 1u;
}

static void s64_uci_reset_reply(S64UciReply* reply)
{
    if (reply == 0) {
        return;
    }

    reply->status_length = 0u;
    if (reply->status != 0 && reply->status_capacity != 0u) {
        reply->status[0] = '\0';
    }
}

static void s64_uci_store_status(S64UciReply* reply, uint8_t value)
{
    uint16_t index;

    if (reply == 0) {
        return;
    }

    index = reply->status_length;
    ++reply->status_length;

    if (reply->status != 0
        && reply->status_capacity > 1u
        && index < (uint16_t)(reply->status_capacity - 1u)) {
        reply->status[index] = (char)value;
        reply->status[index + 1u] = '\0';
    }
}

static uint8_t s64_uci_flush_data(
    S64UciReply* reply,
    uint16_t length,
    uint8_t* consumer_failed
)
{
    if (length == 0u || reply == 0 || reply->data_sink == 0 || *consumer_failed) {
        return 1u;
    }

    if (!reply->data_sink(stream_buffer, length, reply->data_context)) {
        *consumer_failed = 1u;
        return 0u;
    }

    return 1u;
}

S64UciResult s64_uci_execute(
    const S64UciCommandPart* parts,
    uint8_t part_count,
    S64UciReply* reply
)
{
    uint8_t part;
    uint8_t state;
    uint8_t consumer_failed = 0u;
    uint16_t i;
    uint16_t command_length;
    uint16_t stream_length;

    s64_uci_reset_reply(reply);

    if (!s64_uci_command_length(parts, part_count, &command_length)) {
        return S64_UCI_COMMAND_TOO_LARGE;
    }

    if (!s64_uci_interface_present()) {
        return S64_UCI_UNAVAILABLE;
    }

    if (!s64_uci_wait_until_idle()) {
        S64_UCI_CONTROL = S64_UCI_ABORT;
        return S64_UCI_TIMEOUT;
    }

    for (part = 0u; part < part_count; ++part) {
        for (i = 0u; i < parts[part].length; ++i) {
            S64_UCI_COMMAND = parts[part].data[i];
        }
    }
    S64_UCI_CONTROL = S64_UCI_PUSH_COMMAND;

    for (;;) {
        state = s64_uci_wait_for_response();
        if (state != S64_UCI_STATE_LAST && state != S64_UCI_STATE_MORE) {
            S64_UCI_CONTROL = S64_UCI_ABORT;
            return S64_UCI_TIMEOUT;
        }

        stream_length = 0u;
        while ((S64_UCI_CONTROL & S64_UCI_DATA_AVAILABLE) != 0u) {
            stream_buffer[stream_length++] = S64_UCI_RESPONSE;
            if (stream_length == S64_UCI_STREAM_BUFFER) {
                (void)s64_uci_flush_data(reply, stream_length, &consumer_failed);
                stream_length = 0u;
            }
        }
        (void)s64_uci_flush_data(reply, stream_length, &consumer_failed);

        while ((S64_UCI_CONTROL & S64_UCI_STATUS_AVAILABLE) != 0u) {
            s64_uci_store_status(reply, S64_UCI_STATUS_DATA);
        }

        S64_UCI_CONTROL = S64_UCI_ACCEPT_DATA;

        if (state == S64_UCI_STATE_LAST) {
            break;
        }
    }

    return consumer_failed ? S64_UCI_CONSUMER_ABORTED : S64_UCI_OK;
}

typedef struct {
    uint16_t count;
} S64IdentifySink;

static uint8_t s64_uci_count_identify(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64IdentifySink* sink = (S64IdentifySink*)context;
    (void)data;

    sink->count = (uint16_t)(sink->count + length);
    return 1u;
}

static uint8_t s64_uci_status_ok(const char* status, uint16_t length)
{
    return length >= 3u
        && status != 0
        && status[0] == '0'
        && status[1] == '0'
        && status[2] == '0';
}

void s64_uci_probe_capabilities(S64UciCapabilities* capabilities)
{
    static const uint8_t identify_command[] = {
        S64_UCI_TARGET_HTTP,
        S64_UCI_CMD_IDENTIFY
    };
    S64UciCommandPart part;
    S64UciReply reply;
    S64IdentifySink sink;
    char status[16];

    if (capabilities == 0) {
        return;
    }

    capabilities->interface_present = 0u;
    capabilities->http_target = 0u;

    if (!s64_uci_interface_present()) {
        return;
    }

    capabilities->interface_present = 1u;

    part.data = identify_command;
    part.length = (uint16_t)sizeof(identify_command);
    sink.count = 0u;

    reply.data_sink = s64_uci_count_identify;
    reply.data_context = &sink;
    reply.status = status;
    reply.status_capacity = (uint16_t)sizeof(status);
    reply.status_length = 0u;

    if (s64_uci_execute(&part, 1u, &reply) == S64_UCI_OK
        && sink.count != 0u
        && s64_uci_status_ok(status, reply.status_length)) {
        capabilities->http_target = 1u;
    }
}
