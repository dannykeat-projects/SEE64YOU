#include <assert.h>
#include <stdio.h>
#include <string.h>
#define main probe_main
#include "../diagnostics/settings_io_probe.c"
#undef main

static uint8_t temp_exists, final_exists, opened, fail_write;
static uint8_t renamed, deleted_final, saw_create_new, saw_path;
static uint8_t file[sizeof(S64SettingsExchange)];
static uint16_t file_size, position;
static unsigned int keys;
static uint8_t quiet_open, fail_close, real_stats, stale_survives;

void clrscr(void) {}
void cputs(const char* text) { (void)text; }
void cputc(char value) { (void)value; }
int cprintf(const char* format, ...) { (void)format; return 0; }
char cgetc(void) { return keys++ == 0 ? 'y' : ' '; }

uint8_t s64_uci_probe_target(uint8_t target) { return target == 1; }
S64UciResult s64_uci_execute(const S64UciCommandPart* parts,
    uint8_t count, S64UciReply* reply)
{
    uint8_t opcode = parts[0].data[1];
    const char* message = "00,OK";
    const uint8_t* payload = 0;
    uint16_t length = 0;
    uint8_t is_temp = count > 1 && parts[1].length == strlen(TEMP)
        && memcmp(parts[1].data, TEMP, strlen(TEMP)) == 0;
    uint8_t is_final = count > 1 && parts[1].length == strlen(FINAL)
        && memcmp(parts[1].data, FINAL, strlen(FINAL)) == 0;
    if (opcode == 2 || opcode == 9 || opcode == 10) {
        /* Any attempt to mutate/open the real filenames fails this test. */
        assert(is_temp || is_final);
        if (opcode == 10) {
            assert(is_temp && count == 4);
            assert(parts[3].length == strlen(FINAL));
            assert(memcmp(parts[3].data, FINAL, strlen(FINAL)) == 0);
        }
    }
    switch (opcode) {
        case 1: payload = (const uint8_t*)"ULTIMATE DOS"; length = 12; break;
        case 0x12: payload = (const uint8_t*)"/sd/"; length = 4; ++saw_path; break;
        case 0x11: break;
        case 9:
            if (is_temp) {
                if (!temp_exists) message = "88,NOT FOUND";
                if (!stale_survives) temp_exists = 0;
            }
            else { final_exists = 0; ++deleted_final; }
            break;
        case 8:
            if (!is_temp && !is_final) {
                assert((parts[1].length == strlen(S64_SETTINGS_FILENAME)
                    && memcmp(parts[1].data, S64_SETTINGS_FILENAME, parts[1].length) == 0)
                    || (parts[1].length == strlen(S64_SETTINGS_TEMP_FILENAME)
                    && memcmp(parts[1].data, S64_SETTINGS_TEMP_FILENAME, parts[1].length) == 0));
                ++real_stats;
                break;
            }
            if (!(is_temp ? temp_exists : final_exists)) message = "88,NOT FOUND";
            break;
        case 2:
            if (parts[0].data[2] == 6) {
                saw_create_new = 1;
                if (temp_exists) { message = "63,FILE EXISTS"; break; }
                temp_exists = opened = 1;
                file_size = 0;
                if (quiet_open) message = "";
            } else { assert(parts[0].data[2] == 1); opened = 1; position = 0; }
            break;
        case 5:
            assert(opened);
            if (fail_write) message = "85,WRITE FAILED";
            else { memcpy(file, parts[1].data, parts[1].length); file_size = parts[1].length; }
            break;
        case 3:
            opened = 0;
            if (fail_close) message = "85,NO FILE TO CLOSE";
            break;
        case 10: temp_exists = 0; final_exists = renamed = 1; break;
        case 4:
            assert(opened);
            length = parts[0].data[2] | ((uint16_t)parts[0].data[3] << 8);
            if (length > file_size - position) length = file_size - position;
            payload = file + position;
            position += length;
            break;
        default: assert(0);
    }
    if (length && reply->data_sink) assert(reply->data_sink(payload, length, reply->data_context));
    strcpy(reply->status, message);
    reply->status_length = strlen(message);
    return S64_UCI_OK;
}

int s64_reu_load(uint8_t bank, uint16_t address, void* data, uint16_t length)
{ (void)bank; (void)address; (void)data; (void)length; return 0; }
int s64_reu_store(uint8_t bank, uint16_t address, const void* data, uint16_t length)
{ (void)bank; (void)address; (void)data; (void)length; return 0; }

int main(void)
{
    temp_exists = final_exists = 1;
    assert(probe_main() == 0);
    assert(saw_create_new && saw_path == 2 && renamed && deleted_final == 1);
    assert(real_stats == 2);
    assert(file_size == sizeof(expected) && s64_settings_exchange_validate(&actual));
    keys = read_count = first_failed = renamed = deleted_final = 0;
    temp_exists = final_exists = fail_write = 1;
    assert(probe_main() == 1);
    assert(first_failed == 8 && final_exists && !renamed && !deleted_final && !opened);
    keys = read_count = first_failed = renamed = deleted_final = fail_write = 0;
    temp_exists = final_exists = quiet_open = 1;
    assert(probe_main() == 0); /* Production accepts empty CREATE_NEW status. */
    assert(renamed && deleted_final == 1);
    keys = read_count = first_failed = renamed = deleted_final = 0;
    temp_exists = final_exists = fail_close = 1;
    assert(probe_main() == 1);
    assert(first_failed == 9 && renamed && deleted_final == 1);
    keys = read_count = first_failed = renamed = deleted_final = fail_close = 0;
    temp_exists = final_exists = stale_survives = 1;
    assert(probe_main() == 1);
    assert(first_failed == 6 && final_exists && !renamed && !deleted_final && !opened);
    keys = read_count = first_failed = renamed = deleted_final = stale_survives = 0;
    temp_exists = 0;
    final_exists = 1;
    assert(probe_main() == 0);
    assert(renamed && deleted_final == 1);
    puts("settings I/O probe tests passed");
    return 0;
}
