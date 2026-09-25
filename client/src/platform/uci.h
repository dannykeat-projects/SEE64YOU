#ifndef SEE64YOU_PLATFORM_UCI_H
#define SEE64YOU_PLATFORM_UCI_H

#include <stdint.h>

#define S64_UCI_COMMAND_CAPACITY 896u
#define S64_UCI_PART_NATIVE_TEXT 0x01u

typedef enum {
    S64_UCI_OK = 0,
    S64_UCI_UNAVAILABLE,
    S64_UCI_TIMEOUT,
    S64_UCI_PROTOCOL_ERROR,
    S64_UCI_COMMAND_TOO_LARGE,
    S64_UCI_CONSUMER_ABORTED
} S64UciResult;

typedef struct {
    const uint8_t* data;
    uint16_t length;
    uint8_t flags;
} S64UciCommandPart;

typedef uint8_t (*S64UciDataSink)(const uint8_t* data, uint16_t length, void* context);

typedef struct {
    S64UciDataSink data_sink;
    void* data_context;
    char* status;
    uint16_t status_capacity;
    uint16_t status_length;
} S64UciReply;

typedef S64UciResult (*S64UciExecuteFn)(
    const S64UciCommandPart* parts,
    uint8_t part_count,
    S64UciReply* reply
);

typedef struct {
    uint8_t interface_present;
    uint8_t dos_target;
    uint8_t http_target;
} S64UciCapabilities;

/*
 * Execute one bounded UCI command. Command parts are written directly to the
 * hardware queue so callers do not need a second 896-byte staging buffer.
 * Parts marked S64_UCI_PART_NATIVE_TEXT are converted to wire ASCII byte by
 * byte; unmarked parts are binary and are transferred unchanged.
 * Response data may be streamed through data_sink; status is drained even if
 * the caller's buffer is smaller than the hardware status queue.
 */
S64UciResult s64_uci_execute(
    const S64UciCommandPart* parts,
    uint8_t part_count,
    S64UciReply* reply
);

/*
 * Probe the published Ultimate Command Interface without assuming a firmware
 * version. Missing/unsupported UCI is a normal offline condition.
 */
void s64_uci_probe_capabilities(S64UciCapabilities* capabilities);

#endif
