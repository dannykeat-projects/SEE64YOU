#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/dos_path.h"

static const uint8_t* data;
static uint16_t data_length;
static const char* status_text;
static S64UciResult transport;
static uint8_t chunked;
static uint8_t malformed_status;
static void require(int condition, const char* message)
{
    if (!condition) { fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}
static S64UciResult execute(const S64UciCommandPart* parts,
    uint8_t count, S64UciReply* reply)
{
    uint16_t i;
    require(count == 1u && parts[0].length == 2u
        && parts[0].data[0] == S64_DOS_TARGET_PRIMARY
        && parts[0].data[1] == 0x12u && !parts[0].flags,
        "GET_PATH sends read-only binary command");
    if (transport != S64_UCI_OK) return transport;
    if (chunked) {
        for (i = 0u; i < data_length; ++i)
            if (!reply->data_sink(data + i, 1u, reply->data_context))
                return S64_UCI_CONSUMER_ABORTED;
    } else if (!reply->data_sink(data, data_length, reply->data_context))
        return S64_UCI_CONSUMER_ABORTED;
    strcpy(reply->status, status_text);
    reply->status_length = malformed_status ? reply->status_capacity
        : (uint16_t)strlen(status_text);
    return S64_UCI_OK;
}
int main(void)
{
    S64Dos dos;
    char output[32];
    dos.execute = execute; dos.target = S64_DOS_TARGET_PRIMARY;
    data = (const uint8_t*)"/sd/My folder/";
    data_length = (uint16_t)strlen((const char*)data);
    status_text = "00,OK";
    require(s64_dos_get_path(&dos, output, sizeof(output)) == S64_DOS_OK
        && strcmp(output, "/sd/My folder/") == 0, "exact path captured");
    chunked = 1u;
    require(s64_dos_get_path(&dos, output, sizeof(output)) == S64_DOS_OK
        && strcmp(output, "/sd/My folder/") == 0, "chunked response captured");
    ++data_length;
    require(s64_dos_get_path(&dos, output, sizeof(output)) == S64_DOS_OK,
        "single trailing NUL accepted");
    require(s64_dos_get_path(&dos, output, 4u) != S64_DOS_OK && !output[0],
        "truncated path rejected and cleared");
    data = (const uint8_t*)"/sd/\0suffix"; data_length = 11u;
    require(s64_dos_get_path(&dos, output, sizeof(output)) != S64_DOS_OK && !output[0],
        "embedded NUL cannot truncate captured path");
    data = (const uint8_t*)"relative"; data_length = 8u;
    require(s64_dos_get_path(&dos, output, sizeof(output)) != S64_DOS_OK && !output[0],
        "relative path rejected");
    data = (const uint8_t*)"/bad\n"; data_length = 5u;
    require(s64_dos_get_path(&dos, output, sizeof(output)) != S64_DOS_OK && !output[0],
        "wire control characters rejected");
    data = (const uint8_t*)"/"; data_length = 1u;
    status_text = "";
    require(s64_dos_get_path(&dos, output, sizeof(output)) != S64_DOS_OK && !output[0],
        "missing status rejected");
    status_text = "01,error";
    require(s64_dos_get_path(&dos, output, sizeof(output)) != S64_DOS_OK && !output[0],
        "target failure rejected");
    status_text = "00,OK"; malformed_status = 1u;
    require(s64_dos_get_path(&dos, output, sizeof(output)) != S64_DOS_OK && !output[0],
        "oversize status rejected");
    malformed_status = 0u; transport = S64_UCI_TIMEOUT;
    require(s64_dos_get_path(&dos, output, sizeof(output)) == S64_DOS_TRANSPORT_ERROR
        && !output[0], "timeout preserves no partial path");
    require(s64_dos_get_path(0, output, sizeof(output)) == S64_DOS_INVALID_ARGUMENT,
        "invalid arguments rejected");
    puts("DOS path tests passed");
    return 0;
}
