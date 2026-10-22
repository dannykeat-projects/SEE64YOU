#include "uci.h"
#include "charset.h"

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

#define S64_UCI_TARGET_DOS       0x01u
#define S64_UCI_TARGET_HTTP      0x06u
#define S64_UCI_CMD_IDENTIFY     0x01u

#define S64_UCI_WAIT_SECONDS     15UL
#define S64_UCI_STREAM_BUFFER    128u

#ifdef S64_PERSISTENT_RUNTIME_FIT
#include "platform/persistent_fit_scratch.h"
#define stream_buffer ((uint8_t*)S64_PERSISTENT_FIT_SCRATCH_BASE)
typedef char S64PersistentFitUciScratchFits[
    S64_PERSISTENT_FIT_SCRATCH_CAPACITY - S64_UCI_STREAM_BUFFER
];
#else
static uint8_t stream_buffer[S64_UCI_STREAM_BUFFER];
#endif

static uint8_t s64_uci_interface_present(void)
{
    uint8_t reads = 4u;

    /*
     * $DF00-$DFFF is cartridge I/O2 on a conventional C64 and may read as
     * changing open-bus data. Require the Ultimate signature to remain stable
     * across several reads rather than accepting one coincidental byte.
     * Firmware 3.15 may clear bit 7 while a completion IRQ is active, hence
     * the existing seven-bit identifier mask.
     */
    while (reads-- != 0u) {
        if ((uint8_t)(S64_UCI_COMMAND & S64_UCI_IDENT_MASK)
            != S64_UCI_IDENT_VALUE) {
            return 0u;
        }
    }
    return 1u;
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

static S64UciResult s64_uci_validate_command(
    const S64UciCommandPart* parts,
    uint8_t part_count
)
{
    uint8_t part;
    uint8_t converted;
    uint16_t i;
    uint16_t length = 0u;

    if (parts == 0 || part_count == 0u) {
        return S64_UCI_PROTOCOL_ERROR;
    }

    for (part = 0u; part < part_count; ++part) {
        if ((parts[part].flags & (uint8_t)~S64_UCI_PART_NATIVE_TEXT) != 0u
            || (parts[part].length != 0u && parts[part].data == 0)) {
            return S64_UCI_PROTOCOL_ERROR;
        }
        if (parts[part].length > (uint16_t)(S64_UCI_COMMAND_CAPACITY - length)) {
            return S64_UCI_COMMAND_TOO_LARGE;
        }
        if ((parts[part].flags & S64_UCI_PART_NATIVE_TEXT) != 0u) {
            for (i = 0u; i < parts[part].length; ++i) {
                if (!s64_charset_native_to_ascii(parts[part].data[i], &converted)) {
                    return S64_UCI_PROTOCOL_ERROR;
                }
            }
        }
        length = (uint16_t)(length + parts[part].length);
    }

    return length == 0u ? S64_UCI_PROTOCOL_ERROR : S64_UCI_OK;
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

static uint8_t s64_uci_store_status(S64UciReply* reply, uint8_t value)
{
    uint8_t native;
    uint16_t index;

    if (reply == 0) {
        return 1u;
    }
    if (!s64_charset_ascii_to_native(value, &native)) {
        return 0u;
    }

    index = reply->status_length;
    ++reply->status_length;

    if (reply->status != 0
        && reply->status_capacity > 1u
        && index < (uint16_t)(reply->status_capacity - 1u)) {
        reply->status[index] = (char)native;
        reply->status[index + 1u] = '\0';
    }
    return 1u;
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
    uint8_t byte;
    uint8_t consumer_failed = 0u;
    uint8_t status_invalid = 0u;
    uint16_t i;
    uint16_t stream_length;
    S64UciResult validation;

    s64_uci_reset_reply(reply);

    validation = s64_uci_validate_command(parts, part_count);
    if (validation != S64_UCI_OK) {
        return validation;
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
            byte = parts[part].data[i];
            if ((parts[part].flags & S64_UCI_PART_NATIVE_TEXT) != 0u
                && !s64_charset_native_to_ascii(byte, &byte)) {
                S64_UCI_CONTROL = S64_UCI_ABORT;
                return S64_UCI_PROTOCOL_ERROR;
            }
            S64_UCI_COMMAND = byte;
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
            if (!s64_uci_store_status(reply, S64_UCI_STATUS_DATA)) {
                status_invalid = 1u;
            }
        }

        S64_UCI_CONTROL = S64_UCI_ACCEPT_DATA;

        if (state == S64_UCI_STATE_LAST) {
            break;
        }
    }

    if (status_invalid) return S64_UCI_PROTOCOL_ERROR;
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
    /*
     * Probe callers always supply a real local status buffer, so a null check
     * here only costs scarce resident-editor bytes without adding safety.
     */
    return length >= 2u
        && status[0] == '0'
        && status[1] == '0';
}

uint8_t s64_uci_probe_target(uint8_t target)
{
    uint8_t identify_command[2];
    S64UciCommandPart part;
    S64UciReply reply;
    S64IdentifySink sink;
    char status[16];

    identify_command[0] = target;
    identify_command[1] = S64_UCI_CMD_IDENTIFY;

    part.data = identify_command;
    part.length = (uint16_t)sizeof(identify_command);
    part.flags = 0u;
    sink.count = 0u;

    reply.data_sink = s64_uci_count_identify;
    reply.data_context = &sink;
    reply.status = status;
    reply.status_capacity = (uint16_t)sizeof(status);
    reply.status_length = 0u;

    return s64_uci_execute(&part, 1u, &reply) == S64_UCI_OK
        /*
         * The dispatcher identifies an unregistered target as the exact
         * 9-byte payload "NO TARGET" with status 00,OK. Published real target
         * identities are longer, so this rejects the generic false positive
         * without carrying another identity buffer in the resident editor.
         */
        && sink.count != 9u
        && s64_uci_status_ok(status, reply.status_length);
}

void s64_uci_probe_capabilities(S64UciCapabilities* capabilities)
{
    if (capabilities == 0) {
        return;
    }

    capabilities->interface_present = 0u;
    capabilities->dos_target = 0u;
    capabilities->http_target = 0u;

    if (!s64_uci_interface_present()) {
        return;
    }

    capabilities->interface_present = 1u;
    capabilities->dos_target = s64_uci_probe_target(S64_UCI_TARGET_DOS);
    capabilities->http_target = s64_uci_probe_target(S64_UCI_TARGET_HTTP);
}
