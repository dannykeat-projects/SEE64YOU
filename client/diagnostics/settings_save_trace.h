#ifndef S64_SETTINGS_SAVE_TRACE_H
#define S64_SETTINGS_SAVE_TRACE_H

#include <string.h>
#include "platform/uci.h"

#define S64_SAVE_TRACE_CAPACITY 8u
typedef struct {
    uint8_t opcode;
    uint8_t attributes;
    uint8_t final_file;
    uint16_t bytes;
    S64UciResult transport;
    char status[32];
} S64SaveTraceEntry;

static S64SaveTraceEntry save_trace[S64_SAVE_TRACE_CAPACITY];
static uint8_t save_trace_count, save_trace_overflow;

static void save_trace_reset(void)
{
    save_trace_count = save_trace_overflow = 0;
    memset(save_trace, 0, sizeof(save_trace));
}

/* Forward the original command/reply unchanged. Never capture payload bytes:
 * WRITE_DATA contains keys. Retain even cleanup statuses after a failed write. */
static S64UciResult save_trace_execute(const S64UciCommandPart* parts,
    uint8_t count, S64UciReply* reply)
{
    S64UciResult result;
    S64SaveTraceEntry* entry;
    uint16_t length;
    result = s64_uci_execute(parts, count, reply);
    if (save_trace_count >= S64_SAVE_TRACE_CAPACITY) {
        save_trace_overflow = 1;
        return result;
    }
    entry = &save_trace[save_trace_count++];
    if (count != 0 && parts[0].length >= 2) {
        entry->opcode = parts[0].data[1];
        if (entry->opcode == 2 && parts[0].length >= 3)
            entry->attributes = parts[0].data[2];
        if (entry->opcode == 5 && count >= 2) entry->bytes = parts[1].length;
        if (entry->opcode == 9 && count >= 2)
            entry->final_file = parts[1].length == sizeof("SEE64YOU.CFG") - 1
                && memcmp(parts[1].data, "SEE64YOU.CFG", parts[1].length) == 0;
    }
    entry->transport = result;
    length = reply->status_length;
    if (reply->status == 0 || reply->status_capacity == 0) length = 0;
    else if (length >= reply->status_capacity) length = reply->status_capacity - 1;
    if (length >= sizeof(entry->status)) length = sizeof(entry->status) - 1;
    if (length != 0) memcpy(entry->status, reply->status, length);
    entry->status[length] = 0;
    return result;
}

static uint8_t save_trace_failed_gate(void)
{
    uint8_t i;
    const S64SaveTraceEntry* entry;
    for (i = 0; i < save_trace_count; ++i) {
        entry = &save_trace[i];
        if (entry->opcode != 2 && entry->opcode != 5 && entry->opcode != 10) continue;
        if (entry->transport != S64_UCI_OK) return i + 1;
        if (entry->status[0] == 0 && entry->opcode != 10) continue;
        if (entry->status[0] != '0' || entry->status[1] != '0') return i + 1;
    }
    return 0;
}

#endif
