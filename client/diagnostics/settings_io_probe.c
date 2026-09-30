#ifdef __CC65__
#include <conio.h>
#else
/* Host diagnostic regression supplies a keyboard/screen shim. */
void clrscr(void);
void cputs(const char* text);
void cputc(char value);
int cprintf(const char* format, ...);
char cgetc(void);
#endif
#include <string.h>
#include "charset.h"
#include "platform/dos.h"
#include "publisher/settings_store.h"
/* Reuse the private production CREATE_NEW and its status policy without
 * exporting a new API or changing any production binary. */
#include "../src/publisher/settings_store.c"

#define TEMP "S64IOTST.TMP"
#define FINAL "S64IOTST.CFG"

static S64Dos dos;
static S64SettingsExchange expected, actual;
static char status[64];
static uint8_t data_bytes[96];
static uint16_t data_count, copied, read_count, written;
static S64UciResult transport;
static S64UciDataSink downstream;
static void* downstream_context;
static uint8_t first_failed, stage;
static const char* first_label;

static uint8_t capture(const uint8_t* data, uint16_t length, void* context)
{
    uint16_t i;
    (void)context;
    for (i = 0; i < length; ++i) {
        if (copied < sizeof(data_bytes)) data_bytes[copied++] = data[i];
    }
    data_count += length;
    return downstream == 0 || downstream(data, length, downstream_context);
}

/* Tee replies without modifying production command bytes or status policy. */
static S64UciResult trace(const S64UciCommandPart* parts, uint8_t count,
    S64UciReply* reply)
{
    S64UciReply observed;
    data_count = copied = 0;
    memset(status, 0, sizeof(status));
    downstream = reply->data_sink;
    downstream_context = reply->data_context;
    observed = *reply;
    observed.data_sink = capture;
    observed.data_context = 0;
    observed.status = status;
    observed.status_capacity = sizeof(status);
    observed.status_length = 0;
    transport = s64_uci_execute(parts, count, &observed);
    reply->status_length = observed.status_length;
    if (reply->status != 0 && reply->status_capacity != 0) {
        uint16_t n = observed.status_length;
        if (n >= reply->status_capacity) n = reply->status_capacity - 1;
        memcpy(reply->status, status, n);
        reply->status[n] = 0;
    }
    return transport;
}

static void begin(uint8_t number, const char* label)
{
    stage = number;
    clrscr();
    cprintf("SETTINGS I/O %u/16\r\n%s\r\n\r\n", number, label);
    data_count = copied = 0;
    status[0] = 0;
    transport = S64_UCI_OK;
}

static uint8_t finish(const char* label, S64DosResult result, uint8_t okay)
{
    uint16_t i;
    cprintf("DOS %u: %s\r\nUCI %u\r\n", result,
        s64_dos_result_message(result), transport);
    cprintf("STATUS: %s\r\nDATA %u BYTES; HEX:\r\n", status, data_count);
    for (i = 0; i < copied && i < 24; ++i) cprintf("%02X ", data_bytes[i]);
    cputs("\r\n");
    if (!okay && first_failed == 0) {
        first_failed = stage;
        first_label = label;
    }
    cputs(okay ? "STAGE PASS\r\n" : "STAGE FAIL\r\n");
    if (first_failed) cprintf("FIRST FAIL: %u %s\r\n", first_failed, first_label);
    cputs("PRESS KEY FOR NEXT STAGE");
    cgetc();
    return okay;
}

static S64DosResult query(uint8_t opcode)
{
    uint8_t command[2];
    S64UciCommandPart part;
    S64UciReply reply;
    command[0] = dos.target;
    command[1] = opcode;
    part.data = command;
    part.length = 2;
    part.flags = 0;
    memset(&reply, 0, sizeof(reply));
    trace(&part, 1, &reply);
    if (transport != S64_UCI_OK) return S64_DOS_TRANSPORT_ERROR;
    return status[0] == '0' && status[1] == '0'
        ? S64_DOS_OK : S64_DOS_TARGET_ERROR;
}

static void path(void)
{
    uint16_t i;
    uint8_t native;
    cputs("PATH/IDENTITY:\r\n");
    for (i = 0; i < copied; ++i) {
        if (data_bytes[i] == 0) break;
        cputc(s64_charset_ascii_to_native(data_bytes[i], &native) ? native : '?');
    }
    cputs("\r\n");
    if (data_count > copied) cputs("DISPLAY TRUNCATED\r\n");
}

static uint8_t read_record(const uint8_t* data, uint16_t length, void* context)
{
    (void)context;
    if ((uint32_t)read_count + length > sizeof(actual)) return 0;
    memcpy((uint8_t*)&actual + read_count, data, length);
    read_count += length;
    return 1;
}

/* Firmware may use a status code outside the production DOS mapping. Only
 * deliberate absence checks/cleanup may accept the explicit missing-file text. */
static uint8_t expected_absence(S64DosResult result)
{
    return result == S64_DOS_NOT_FOUND
        || (transport == S64_UCI_OK && strstr(status, "FILE NOT FOUND") != 0);
}

static uint8_t stat_file(uint8_t number, const char* label,
    const char* filename, uint8_t wanted)
{
    uint8_t exists;
    S64DosResult result;
    begin(number, label);
    result = s64_dos_file_exists(&dos, filename, &exists);
    if (wanted != 1 && expected_absence(result)) {
        exists = 0;
        cputs("EXPECTED FILE NOT FOUND\r\n");
        return finish(label, result, 1);
    }
    if (wanted == 2) cprintf("EXISTS: %u (INSPECTION ONLY)\r\n", exists);
    else cprintf("EXISTS: %u EXPECTED: %u\r\n", exists, wanted);
    return finish(label, result, result == S64_DOS_OK && (wanted == 2 || exists == wanted));
}

int main(void)
{
    S64DosResult result, close_result;
    uint16_t count, eof;
    uint8_t available, valid;
    clrscr();
    cputs("SETTINGS I/O PROBE\r\n\r\nWRITES ONLY /sd/S64IOTST.TMP\r\nAND /sd/S64IOTST.CFG.\r\nREAL SETTINGS ARE READ-ONLY.\r\n\r\nY TO RUN; OTHER KEY EXITS.");
    if (cgetc() != 'y') return 0;
    s64_dos_init(&dos, trace, S64_DOS_TARGET_PRIMARY);
    s64_settings_exchange_init(&expected);
    begin(1, "IDENTIFY DOS $01");
    available = s64_uci_probe_target(dos.target);
    result = query(0x01);
    path();
    if (!finish("IDENTIFY", result, available && result == S64_DOS_OK)) goto done;
    begin(2, "GET_PATH BEFORE");
    result = query(0x12);
    path();
    finish("GET_PATH BEFORE", result, result == S64_DOS_OK && data_count != 0);
    begin(3, "CHANGE_DIR /sd/");
    result = s64_dos_change_dir(&dos, "/sd/");
    if (!finish("CHANGE_DIR", result, result == S64_DOS_OK)) goto done;
    begin(4, "GET_PATH AFTER");
    result = query(0x12);
    path();
    if (!finish("GET_PATH AFTER", result, result == S64_DOS_OK && data_count != 0)) goto done;
    stat_file(4, "INSPECT REAL CFG", S64_SETTINGS_FILENAME, 2);
    stat_file(4, "INSPECT REAL CFG.TMP", S64_SETTINGS_TEMP_FILENAME, 2);
    stat_file(5, "INSPECT TEST TEMP", TEMP, 2);
    begin(5, "DELETE TEST TEMP");
    result = s64_dos_delete_file(&dos, TEMP);
    finish("DELETE TEMP", result, result == S64_DOS_OK || expected_absence(result));
    stat_file(6, "STAT AFTER DELETE", TEMP, 0);
    begin(7, "OPEN TEMP CREATE_NEW $06");
    result = open_create_new(&dos, TEMP);
    if (!finish("OPEN TEMP", result, result == S64_DOS_OK)) goto done;
    begin(8, "WRITE CURRENT SETTINGS");
    result = s64_dos_write(&dos, (const uint8_t*)&expected, sizeof(expected));
    /* DOS write API has no byte-count return. Readback proves actual length. */
    written = result == S64_DOS_OK ? sizeof(expected) : 0;
    cprintf("ATTEMPT %u ACKNOWLEDGED %u\r\nACTUAL COUNT: READBACK BELOW\r\n", sizeof(expected), written);
    finish("WRITE", result, result == S64_DOS_OK);
    begin(9, "CLOSE TEMP");
    close_result = s64_dos_close(&dos);
    finish("CLOSE TEMP", close_result, close_result == S64_DOS_OK);
    if (result != S64_DOS_OK) goto done;
    stat_file(10, "STAT WRITTEN TEST TEMP", TEMP, 1);
    begin(11, "DELETE TEST FINAL");
    result = s64_dos_delete_file(&dos, FINAL);
    finish("DELETE TEST FINAL", result, result == S64_DOS_OK || expected_absence(result));
    begin(12, "RENAME TEMP TO CFG");
    result = s64_dos_rename_file(&dos, TEMP, FINAL);
    if (!finish("RENAME", result, result == S64_DOS_OK)) goto done;
    stat_file(13, "STAT TEST FINAL", FINAL, 1);
    begin(14, "OPEN/READ FINAL CFG");
    result = s64_dos_open_read(&dos, FINAL);
    if (!finish("OPEN FINAL", result, result == S64_DOS_OK)) goto done;
    begin(14, "READ FINAL CFG");
    result = s64_dos_read(&dos, sizeof(actual), read_record, 0, &count);
    cprintf("READ %u EXPECTED %u\r\n", count, sizeof(expected));
    finish("READ FINAL", result, result == S64_DOS_OK && count == sizeof(expected));
    begin(15, "CHECK EOF EXACT BYTES");
    close_result = s64_dos_read(&dos, 1, 0, 0, &eof);
    valid = result == S64_DOS_OK && count == sizeof(expected)
        && read_count == count && close_result == S64_DOS_OK && eof == 0;
    cprintf("EXTRA BYTES %u\r\n", eof);
    finish("EXACT LENGTH", close_result, valid);
    begin(15, "CLOSE FINAL CFG");
    result = s64_dos_close(&dos);
    finish("CLOSE FINAL", result, result == S64_DOS_OK);
    begin(16, "CHECKSUM/CONTENT");
    valid = valid && s64_settings_exchange_validate(&actual)
        && actual.state == S64_SETTINGS_STATE_READY
        && memcmp(&expected, &actual, sizeof(expected)) == 0;
    finish("VALIDATE", S64_DOS_OK, valid);
done:
    clrscr();
    cputs(first_failed ? "SETTINGS I/O FAIL\r\n" : "SETTINGS I/O PASS\r\n");
    if (first_failed) cprintf("FIRST: %u %s\r\n", first_failed, first_label);
    cputs("PRESS KEY TO EXIT");
    cgetc();
    return first_failed != 0;
}
