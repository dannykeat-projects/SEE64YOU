#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "platform/module_preload.h"

static unsigned calls, opens, closes, loads;
static unsigned failure;
static uint32_t expected_address, expected_length;
static unsigned char last_command[10];

static S64UciResult execute(const S64UciCommandPart* parts, uint8_t count,
    S64UciReply* reply)
{
    char data[48];
    uint8_t info[4];
    uint8_t op = parts[0].data[1];
    const char* status = "00,OK";
    unsigned i;
    ++calls;
    assert(count >= 1);
    if (failure == 9) return S64_UCI_UNAVAILABLE;
    if (op == 0x11) {
        assert(count == 2 && parts[1].length == 4);
        assert(memcmp(parts[1].data, "/sd/", 4) == 0);
    } else if (op == 2) {
        ++opens;
        if (failure == 6) status = "82,NOT FOUND";
    } else if (op == 3) {
        ++closes;
        if (failure == 7) status = "84,NO FILE TO CLOSE";
        if (!failure) expected_address += 65536UL;
    } else if (op == 7) {
        uint32_t size = expected_length + (failure == 5);
        for (i = 0; i < 4; ++i) info[i] = (uint8_t)(size >> (8 * i));
        assert(reply->data_sink(info, 4, reply->data_context));
    } else if (op == 0x21) {
        ++loads;
        assert(parts[0].length == 10);
        memcpy(last_command, parts[0].data, 10);
        for (i = 0; i < 4; ++i) {
            assert(last_command[2+i] == (uint8_t)(expected_address >> (8*i)));
            assert(last_command[6+i] == (uint8_t)(expected_length >> (8*i)));
        }
        sprintf(data, "$%6lx BYTES LOADED TO REU $%6lx",
            (unsigned long)(expected_length - (failure == 1)),
            (unsigned long)(expected_address + (failure == 2)));
        if (failure == 3) status = "02,REQUEST TRUNCATED";
        if (failure == 4) data[8] = '?';
        if (failure != 8) {
            /* Exercise fragmented UCI replies. */
            assert(reply->data_sink((uint8_t*)data, 7, reply->data_context));
            assert(reply->data_sink((uint8_t*)data + 7, 28, reply->data_context));
        }
    } else assert(0);
    if (failure == 10) status = "";
    strcpy(reply->status, status);
    reply->status_length = (uint16_t)strlen(status);
    return S64_UCI_OK;
}

/* Allocation is tested separately; descriptors below model owned regions. */
uint8_t s64_reu_runtime_valid(const S64ReuRuntimeControl* control)
{ (void)control; return 0; }
uint8_t s64_reu_runtime_allocate(S64ReuRuntimeControl* c, S64ReuOwner owner,
    uint32_t size, uint16_t alignment, S64ReuAllocation* allocation)
{ (void)c; (void)owner; (void)size; (void)alignment; (void)allocation; return 0; }

int main(void)
{
    S64Dos dos;
    S64ModuleManifest manifest;
    unsigned i;
    s64_dos_init(&dos, execute, S64_DOS_TARGET_PRIMARY);
    expected_address = 0x030100UL;
    expected_length = 0xbc06UL;
    assert(s64_dos_load_reu(&dos, expected_address, expected_length) == S64_DOS_OK);
    for (failure = 1; failure <= 4; ++failure)
        assert(s64_dos_load_reu(&dos, expected_address, expected_length) != S64_DOS_OK);
    for (failure = 8; failure <= 10; ++failure)
        assert(s64_dos_load_reu(&dos, expected_address, expected_length) != S64_DOS_OK);
    failure = 0;
    calls = 0;
    assert(s64_dos_load_reu(&dos, 0x1000000UL, 1) == S64_DOS_INVALID_ARGUMENT);
    assert(s64_dos_load_reu(&dos, 0xffffffUL, 2) == S64_DOS_INVALID_ARGUMENT);
    assert(s64_dos_load_reu(&dos, 0, 0) == S64_DOS_INVALID_ARGUMENT);
    assert(s64_dos_load_reu(&dos, 0, 0x1000000UL) == S64_DOS_INVALID_ARGUMENT);
    assert(s64_dos_load_reu(0, 0, 1) == S64_DOS_INVALID_ARGUMENT);
    assert(calls == 0);
    dos.target = S64_DOS_TARGET_SECONDARY;
    expected_address = 0xffffffUL; expected_length = 1;
    assert(s64_dos_load_reu(&dos, expected_address, 1) == S64_DOS_OK);
    assert(last_command[0] == S64_DOS_TARGET_SECONDARY);

    for (failure = 0; failure <= 7; ++failure) {
        s64_module_manifest_init(&manifest, 0x201);
        expected_address = 0x030000UL; expected_length = 48;
        for (i = 0; i < S64_MODULE_COUNT; ++i) {
            manifest.modules[i].cache_address = expected_address + i * 65536UL;
            manifest.modules[i].load_address = 0x801;
            manifest.modules[i].startup_address = 0x80d;
            manifest.modules[i].byte_length = expected_length;
            manifest.modules[i].integrity = 1;
        }
        opens = closes = loads = 0;
        assert(s64_module_preload(&dos, &manifest) == (failure == 0));
        if (!failure) {
            assert(opens == 3 && closes == 3 && loads == 3);
            for (i = 0; i < 3; ++i)
                assert(manifest.modules[i].cache_state == S64_MODULE_CACHE_LOADED_UNVERIFIED);
        } else {
            assert(manifest.modules[0].cache_state == S64_MODULE_CACHE_EMPTY);
            assert(closes == (failure == 6 ? 0 : 1));
            if (failure == 5 || failure == 6) assert(loads == 0);
        }
    }
    puts("module preload tests passed");
    return 0;
}
