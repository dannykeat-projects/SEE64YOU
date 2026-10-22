#include "platform/dos.h"
#include <string.h>

#define REU_END 0x01000000UL
#define REPLY_LENGTH 35u

/* UCI calls are synchronous; keep bounded scratch off the cc65 stack. */
static char load_reply[REPLY_LENGTH];
static char load_status[64];
static uint16_t load_reply_length;
/* Data replies are wire ASCII, independent of cc65's PETSCII literals. */
static const uint8_t reply_marker[22] = {
    0x20,0x42,0x59,0x54,0x45,0x53,0x20,0x4c,0x4f,0x41,0x44,
    0x45,0x44,0x20,0x54,0x4f,0x20,0x52,0x45,0x55,0x20,0x24
};

static uint8_t reply_sink(const uint8_t* data, uint16_t length, void* context)
{
    (void)context;
    if (length > REPLY_LENGTH - load_reply_length) return 0u;
    memcpy(load_reply + load_reply_length, data, length);
    load_reply_length += length;
    return 1u;
}

static uint8_t reply_hex(const char* text, uint32_t* value)
{
    uint8_t i, digit, seen = 0u;
    *value = 0UL;
    for (i = 0u; i < 6u; ++i) {
        if (text[i] == ' ' && !seen) continue;
        if (text[i] >= '0' && text[i] <= '9') digit = text[i] - '0';
        else if (text[i] >= 0x61 && text[i] <= 0x66) digit = text[i] - 0x61 + 10u;
        else if (text[i] >= 0x41 && text[i] <= 0x46) digit = text[i] - 0x41 + 10u;
        else return 0u;
        seen = 1u;
        *value = (*value << 4) | digit;
    }
    return seen;
}

S64DosResult s64_dos_load_reu(S64Dos* dos, uint32_t address, uint32_t length)
{
    uint8_t command[10];
    uint8_t i;
    S64UciCommandPart part;
    S64UciReply reply;
    S64UciResult transport;
    uint32_t transferred, destination;

    /* Firmware masks high bytes. Reject rather than silently truncate. */
    if (dos == 0 || dos->execute == 0
        || (dos->target != S64_DOS_TARGET_PRIMARY
            && dos->target != S64_DOS_TARGET_SECONDARY)
        || address >= REU_END || length == 0UL || length >= REU_END
        || length > REU_END - address) return S64_DOS_INVALID_ARGUMENT;

    command[0] = dos->target;
    command[1] = 0x21u;
    for (i = 0u; i < 4u; ++i) {
        command[2u + i] = (uint8_t)(address >> (i * 8u));
        command[6u + i] = (uint8_t)(length >> (i * 8u));
    }
    part.data = command;
    part.length = sizeof(command);
    part.flags = 0u;
    load_reply_length = 0u;
    reply.data_sink = reply_sink;
    reply.data_context = 0;
    reply.status = load_status;
    reply.status_capacity = sizeof(load_status);
    reply.status_length = 0u;
    transport = dos->execute(&part, 1u, &reply);
    if (transport != S64_UCI_OK) return S64_DOS_TRANSPORT_ERROR;
    if (reply.status_length < 2u || load_status[0] != '0'
        || load_status[1] != '0') return S64_DOS_TARGET_ERROR;
    /* Official firmware emits %6x fields, including leading spaces. */
    if (load_reply_length != REPLY_LENGTH || load_reply[0] != '$'
        || memcmp(load_reply + 7u, reply_marker, sizeof(reply_marker)) != 0
        || !reply_hex(load_reply + 1u, &transferred)
        || !reply_hex(load_reply + 29u, &destination)
        || transferred != length || destination != address) {
        return S64_DOS_TARGET_ERROR;
    }
    return S64_DOS_OK;
}
