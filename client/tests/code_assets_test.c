#include "platform/code_overlay_bootstrap.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/code_assets.h"\n#include "platform/package_paths.h"
#include "platform/code_assets_internal.h"
#include "platform/module_session.h"
#include "platform/transfer_capsule.h"
#include "platform/basic_return.h"
#include "platform/application_state_reserve.h"
#include "platform/crc32.h"
#include "platform/reu.h"

static uint8_t memory[0x1000000], files[S64_CODE_OVERLAY_COUNT][300];
static uint8_t manifest_wire[S64_CODE_ASSET_MANIFEST_SIZE];
static uint8_t slot[S64_CODE_OVERLAY_CAPACITY];
static S64CodeAssets assets, loaded;
static S64ReuRuntimeControl control;
static S64ModuleDescriptor editor;
static S64SessionJournal journal;
static S64SystemState system_state;
static S64Dos dos;
static unsigned file_index, opens, closes, loads, reads, writes;
static unsigned fail_open, fail_size, fail_load, fail_close, fail_read, fail_write;
static unsigned mismatch_read;
static unsigned mode_calls, fail_mode;
static uint8_t open_file;
static uint8_t capture(void* context, S64SystemSnapshot* snapshot)
{ (void)context; snapshot->turbo_control = 2; snapshot->turbo_control_valid = 1; return 1; }
static uint8_t apply(void* context, S64SystemMode mode)
{ (void)context; (void)mode; return ++mode_calls != fail_mode; }
static uint8_t restore(void* context, const S64SystemSnapshot* snapshot)
{ (void)context; (void)snapshot; return 1; }
static const S64SystemBackend backend = { capture, apply, restore, 0 };
static uint32_t crc(const uint8_t* bytes, unsigned length)
{
    uint32_t value = 0xffffffffUL;
    unsigned i, bit;
    for (i = 0; i < length; ++i) {
        value ^= bytes[i];
        for (bit = 0; bit < 8; ++bit) value = (value >> 1) ^ ((value & 1) ? 0xedb88320UL : 0);
    }
    return value ^ 0xffffffffUL;
}
static void put16(uint8_t* out, uint16_t value)
{ out[0] = value; out[1] = value >> 8; }
static void put32(uint8_t* out, uint32_t value)
{ put16(out, value); put16(out + 2, value >> 16); }
uint8_t s64_reu_load(uint8_t bank, uint16_t offset, void* destination, uint16_t length)
{
    uint32_t address = ((uint32_t)bank << 16) | offset;
    assert(system_state.mode == S64_SYSTEM_MODE_IO_SAFE);
    assert(address + length <= sizeof(memory));
    if (++reads == fail_read) return 0;
    memcpy(destination, memory + address, length);
    if (reads == mismatch_read) ((uint8_t*)destination)[length - 1u] ^= 1u;
    return 1;
}
uint8_t s64_reu_store(uint8_t bank, uint16_t offset, const void* source, uint16_t length)
{
    uint32_t address = ((uint32_t)bank << 16) | offset;
    assert(system_state.mode == S64_SYSTEM_MODE_IO_SAFE);
    assert(address + length <= sizeof(memory));
    if (++writes == fail_write) return 0;
    memcpy(memory + address, source, length);
    return 1;
}
S64DosResult s64_dos_change_dir(S64Dos* target, const char* path)
{ assert(target == &dos && !strcmp(path, S64_PACKAGE_DIRECTORY)); return S64_DOS_OK; }
S64DosResult s64_dos_open_read(S64Dos* target, const char* name)
{
    static const char* const names[] = { S64_CODE_ASSET_FILE_0, S64_CODE_ASSET_FILE_1, S64_CODE_ASSET_FILE_2
#ifndef S64_CODE_ASSET_PUBLISHER
        , S64_CODE_ASSET_FILE_3
#endif
    };
    assert(target == &dos && !open_file);
    file_index = opens % S64_CODE_OVERLAY_COUNT;
    assert(!strcmp(name, names[file_index]));
    if (++opens == fail_open) return S64_DOS_TARGET_ERROR;
    open_file = 1;
    return S64_DOS_OK;
}
S64DosResult s64_dos_file_size(S64Dos* target, uint32_t* size)
{ assert(target == &dos && open_file); *size = 300 + (opens == fail_size); return S64_DOS_OK; }
S64DosResult s64_dos_load_reu(S64Dos* target, uint32_t address, uint32_t length)
{
    assert(target == &dos && open_file && length == 300);
    assert(address == assets.assets[file_index].cache.address);
    if (++loads == fail_load) return S64_DOS_TARGET_ERROR;
    memcpy(memory + address, files[file_index], length);
    return S64_DOS_OK;
}
S64DosResult s64_dos_close(S64Dos* target)
{ assert(target == &dos && open_file); open_file = 0; return ++closes == fail_close ? S64_DOS_TARGET_ERROR : S64_DOS_OK; }
/* Consumer codecs must not perform journal/DOS reads, writes or capture. */
S64DosResult s64_dos_read(S64Dos* d, uint16_t n, S64UciDataSink sink, void* c, uint16_t* r)
{ (void)d; (void)n; (void)sink; (void)c; (void)r; assert(0); return S64_DOS_TARGET_ERROR; }
S64DosResult s64_dos_write(S64Dos* d, const uint8_t* data, uint16_t n)
{ (void)d; (void)data; (void)n; assert(0); return S64_DOS_TARGET_ERROR; }
S64DosResult s64_dos_open_write(S64Dos* d, const char* name)
{ (void)d; (void)name; assert(0); return S64_DOS_TARGET_ERROR; }
S64DosResult s64_dos_file_exists(S64Dos* d, const char* name, uint8_t* exists)
{ (void)d; (void)name; (void)exists; assert(0); return S64_DOS_TARGET_ERROR; }

static void fixture(void)
{
    S64ModuleManifest modules;
    S64SystemSnapshot snapshot = { 2, 1 };
    unsigned i, j;
    opens = closes = loads = reads = writes = mode_calls = 0;
    fail_open = fail_size = fail_load = fail_close = fail_read = fail_write = fail_mode = 0;
    mismatch_read = 0;
    open_file = 0;
    s64_system_state_init(&system_state, &backend, 0);
    assert(s64_system_state_capture(&system_state) == S64_SYSTEM_OK);
    assert(s64_system_state_set_mode(&system_state, S64_SYSTEM_MODE_IO_SAFE) == S64_SYSTEM_OK);
    s64_reu_runtime_init(&control, 1, 123);
    assert(s64_reu_runtime_allocate(&control, S64_REU_OWNER_MODULE, 65536, 256, 0));
    s64_module_manifest_init(&modules, 1);
    editor = modules.modules[S64_CODE_ASSET_MODULE - 1u];
    editor.load_address = 0x801; editor.startup_address = 0x80d;
    editor.byte_length = 100; editor.integrity = 0x12345678;
    editor.cache_address = control.allocations[0].address; editor.cache_capacity = 65536;
    editor.cache_state = S64_MODULE_CACHE_READY;
    assert(s64_session_journal_prepare(&journal, control.session_id, 1, 0, &snapshot));
    journal.sequence = 1;
    memset(manifest_wire, 0, sizeof(manifest_wire));
    memcpy(manifest_wire, "S64O", 4); manifest_wire[3] = S64_CODE_ASSET_MAGIC_LAST; manifest_wire[4] = S64_CODE_ASSET_FORMAT_VERSION; manifest_wire[5] = S64_CODE_OVERLAY_COUNT;
    put16(manifest_wire + 6, 1); put16(manifest_wire + 8, 1);
    put16(manifest_wire + 10, S64_CODE_OVERLAY_ADDRESS);
    put16(manifest_wire + 12, S64_CODE_OVERLAY_CAPACITY);
    put32(manifest_wire + 16, editor.byte_length); put32(manifest_wire + 20, editor.integrity);
    for (i = 0; i < S64_CODE_OVERLAY_COUNT; ++i) {
        for (j = 0; j < 300; ++j) files[i][j] = i * 17 + j;
        put32(manifest_wire + 24 + i * 8, 300);
        put32(manifest_wire + 28 + i * 8, crc(files[i], 300));
    }
    put32(manifest_wire + S64_CODE_ASSET_MANIFEST_CRC_OFFSET, crc(manifest_wire, S64_CODE_ASSET_MANIFEST_CRC_OFFSET));
    assert(s64_code_assets_parse(&assets, manifest_wire, sizeof(manifest_wire), &editor));
    assert(s64_code_assets_allocate(&assets, &control));
    assert(s64_code_assets_clear(&control));
}
static uint32_t record_address(void) { return control.allocations[S64_CODE_OVERLAY_COUNT + 1u].address; }
static void reject_record_byte(unsigned offset, uint8_t value)
{
    uint8_t original[S64_CODE_ASSET_SESSION_SIZE];
    uint8_t* record = memory + record_address();
    memcpy(original, record, sizeof(original));
    record[offset] = value;
    put16(record + S64_CODE_ASSET_SESSION_SIZE - 2u,
        s64_code_assets_session_checksum(record));
    assert(!s64_code_assets_load(&control, &journal, &editor, &loaded));
    memcpy(record, original, sizeof(original));
}
static void reserve(uint8_t owner, uint8_t flag, uint32_t length)
{
    assert(s64_reu_runtime_allocate(&control, owner, length, 256, 0));
    control.allocations[control.allocation_count - 1u].flags = flag;
}
static void require_unready(void)
{
    unsigned i;
    assert(!open_file);
    for (i = 0; i < S64_CODE_OVERLAY_COUNT; ++i) assert(assets.assets[i].cache_state != S64_MODULE_CACHE_READY);
    memset(&loaded, 0xa5, sizeof(loaded));
    assert(!s64_code_assets_load(&control, &journal, &editor, &loaded));
    for (i = 0; i < sizeof(loaded); ++i) assert(((uint8_t*)&loaded)[i] == 0xa5);
}
int main(int argc, char** argv)
{
    S64CodeAssets saved;
    S64ReuRuntimeControl saved_control;
    unsigned i;
    assert((s64_crc32_update(0xffffffffUL, (const uint8_t*)"123456789", 9) ^ 0xffffffffUL) == 0xcbf43926UL);
    fixture();
    saved = assets;
    editor.id = S64_CODE_ASSET_MODULE == S64_MODULE_EDITOR ? S64_MODULE_PUBLISHER : S64_MODULE_EDITOR;
    assert(!s64_code_assets_parse(&assets, manifest_wire, sizeof(manifest_wire), &editor));
    editor.id = S64_CODE_ASSET_MODULE;
    manifest_wire[3] = S64_CODE_ASSET_MAGIC_LAST == 0x50u ? 0x4fu : 0x50u;
    put32(manifest_wire + S64_CODE_ASSET_MANIFEST_CRC_OFFSET, crc(manifest_wire, S64_CODE_ASSET_MANIFEST_CRC_OFFSET));
    assert(!s64_code_assets_parse(&assets, manifest_wire, sizeof(manifest_wire), &editor));
    fixture(); saved = assets;
    manifest_wire[4] = 1;
    put32(manifest_wire + S64_CODE_ASSET_MANIFEST_CRC_OFFSET, crc(manifest_wire, S64_CODE_ASSET_MANIFEST_CRC_OFFSET));
    assert(!s64_code_assets_parse(&assets, manifest_wire, sizeof(manifest_wire), &editor));
    manifest_wire[4] = S64_CODE_ASSET_FORMAT_VERSION;
    manifest_wire[5] = S64_CODE_OVERLAY_COUNT == 4 ? 3 : 4;
    put32(manifest_wire + S64_CODE_ASSET_MANIFEST_CRC_OFFSET, crc(manifest_wire, S64_CODE_ASSET_MANIFEST_CRC_OFFSET));
    assert(!s64_code_assets_parse(&assets, manifest_wire, sizeof(manifest_wire), &editor));
    fixture(); saved = assets;
    for (i = 0; i < sizeof(manifest_wire); ++i) {
        manifest_wire[i] ^= 1;
        assert(!s64_code_assets_parse(&assets, manifest_wire, sizeof(manifest_wire), &editor));
        assert(!memcmp(&assets, &saved, sizeof(saved)));
        manifest_wire[i] ^= 1;
    }
    assert(!s64_code_assets_parse(&assets, manifest_wire, 51, &editor));
    editor.startup_address = 0x8c00;
    assert(!s64_code_assets_parse(&assets, manifest_wire, sizeof(manifest_wire), &editor));
    editor.startup_address = 0x80d;
    put32(manifest_wire + 24, S64_CODE_OVERLAY_CAPACITY + 1UL);
    put32(manifest_wire + S64_CODE_ASSET_MANIFEST_CRC_OFFSET, crc(manifest_wire, S64_CODE_ASSET_MANIFEST_CRC_OFFSET));
    assert(!s64_code_assets_parse(&assets, manifest_wire, sizeof(manifest_wire), &editor));
    put32(manifest_wire + 24, 300);
    put32(manifest_wire + S64_CODE_ASSET_MANIFEST_CRC_OFFSET, crc(manifest_wire, S64_CODE_ASSET_MANIFEST_CRC_OFFSET));
    saved_control = control;
    assert(!s64_code_assets_allocate(&assets, &control));
    assert(!memcmp(&control, &saved_control, sizeof(control)));
    assert(s64_code_assets_preload(&dos, &control, &editor, &assets, &system_state));
    assert(opens == S64_CODE_OVERLAY_COUNT && closes == S64_CODE_OVERLAY_COUNT && loads == S64_CODE_OVERLAY_COUNT);
    assert(s64_code_assets_store_verified(&control, &journal, &editor, &assets));
    assert(s64_code_assets_load(&control, &journal, &editor, &loaded));
    assert(!memcmp(&assets, &loaded, sizeof(assets)));
    reject_record_byte(4, 1);
    reject_record_byte(5, S64_CODE_OVERLAY_COUNT == 4 ? 3 : 4);
    reject_record_byte(3, S64_CODE_ASSET_SESSION_FLAG == 3 ? 'Q' : 'C');
    reject_record_byte(40 + (S64_CODE_OVERLAY_COUNT - 1u) * 16u, 1);
    reject_record_byte(42 + (S64_CODE_OVERLAY_COUNT - 1u) * 16u, S64_CODE_ASSET_CACHE_FLAG);
    ++control.allocations[2].flags;
    assert(!s64_code_assets_load(&control, &journal, &editor, &loaded));
    --control.allocations[2].flags;
    control.allocations[2].owner = S64_REU_OWNER_EDITOR;
    assert(!s64_code_assets_load(&control, &journal, &editor, &loaded));
    control.allocations[2].owner = S64_REU_OWNER_MODULE;
    --control.allocations[2].length;
    assert(!s64_code_assets_load(&control, &journal, &editor, &loaded));
    ++control.allocations[2].length;
    i = mode_calls;
    assert(!s64_code_assets_bind(&control, &journal, &editor, 0, slot, sizeof(slot), s64_reu_load));
    assert(!s64_code_overlay_select(1));
    assert(s64_code_assets_bind(&control, &journal, &editor, 1, slot, sizeof(slot), s64_reu_load));
    assert(s64_code_overlay_select(1) && !memcmp(slot, files[0], 300));
    assert(s64_code_overlay_select(S64_CODE_OVERLAY_COUNT)
        && !memcmp(slot, files[S64_CODE_OVERLAY_COUNT - 1u], 300));
    assert(mode_calls == i && opens == S64_CODE_OVERLAY_COUNT && system_state.snapshot.turbo_control == 2);
    assert(!s64_code_assets_bind(&control, &journal, &editor, 0, slot, sizeof(slot), s64_reu_load));
    assert(!s64_code_overlay_select(1));
    {
        uint16_t lengths[S64_CODE_OVERLAY_COUNT];
        unsigned asset_index;
        for (asset_index = 0; asset_index < S64_CODE_OVERLAY_COUNT; ++asset_index) lengths[asset_index] = 300;
        assert(!s64_code_overlay_bootstrap(&control, 0, control.app_version,
            lengths, slot, s64_reu_load));
        assert(!s64_code_overlay_select(3));
        assert(s64_code_overlay_bootstrap(&control, 1, control.app_version,
            lengths, slot, s64_reu_load));
        assert(!memcmp(slot, files[2], 300));
        assert(!s64_code_overlay_bootstrap(&control, 1, control.app_version + 1,
            lengths, slot, s64_reu_load));
        assert(!s64_code_overlay_select(3));
        lengths[2] = 299;
        assert(!s64_code_overlay_bootstrap(&control, 1, control.app_version,
            lengths, slot, s64_reu_load));
        lengths[2] = 300;
        fail_read = reads + 1;
        assert(!s64_code_overlay_bootstrap(&control, 1, control.app_version,
            lengths, slot, s64_reu_load));
        assert(!s64_code_overlay_select(3));
        fail_read = 0;
        assert(mode_calls == i && opens == S64_CODE_OVERLAY_COUNT);
    }
    for (i = 0; i < S64_CODE_ASSET_SESSION_SIZE; ++i) {
        memory[record_address() + i] ^= 1;
        assert(!s64_code_assets_load(&control, &journal, &editor, &loaded));
        memory[record_address() + i] ^= 1;
    }
    ++journal.sequence; assert(!s64_code_assets_load(&control, &journal, &editor, &loaded)); --journal.sequence;
    journal.state = S64_SESSION_STATE_CLEAN; assert(!s64_code_assets_load(&control, &journal, &editor, &loaded));
    journal.state = S64_SESSION_STATE_ACTIVE;
    ++control.session_id; assert(!s64_code_assets_load(&control, &journal, &editor, &loaded)); --control.session_id;
    ++editor.integrity; assert(!s64_code_assets_load(&control, &journal, &editor, &loaded)); --editor.integrity;
    files[S64_CODE_OVERLAY_COUNT - 1u][299] ^= 1;
    assert(!s64_code_assets_preload(&dos, &control, &editor, &assets, &system_state));
    require_unready();
    for (i = 1; i <= S64_CODE_OVERLAY_COUNT; ++i) {
        fixture(); fail_open = i; assert(!s64_code_assets_preload(&dos, &control, &editor, &assets, &system_state)); require_unready();
        fixture(); fail_size = i; assert(!s64_code_assets_preload(&dos, &control, &editor, &assets, &system_state)); require_unready();
        fixture(); fail_load = i; assert(!s64_code_assets_preload(&dos, &control, &editor, &assets, &system_state)); require_unready();
        fixture(); fail_close = i; assert(!s64_code_assets_preload(&dos, &control, &editor, &assets, &system_state)); require_unready();
    }
    fixture(); fail_write = writes + 1;
    assert(!s64_code_assets_preload(&dos, &control, &editor, &assets, &system_state) && !opens);
    fixture(); fail_read = reads + 2;
    assert(!s64_code_assets_preload(&dos, &control, &editor, &assets, &system_state)); require_unready();
    fixture(); fail_mode = mode_calls + 1;
    assert(!s64_code_assets_preload(&dos, &control, &editor, &assets, &system_state)); require_unready();
    fixture();
    assert(s64_code_assets_preload(&dos, &control, &editor, &assets, &system_state));
    fail_write = writes + 2;
    assert(!s64_code_assets_store_verified(&control, &journal, &editor, &assets));
    assert(!s64_code_assets_load(&control, &journal, &editor, &loaded));
    /* Revocation and final full-record readback must both succeed. A mismatch
     * at the final byte covers the extended fourth-asset record/trailer. */
    for (i = 1; i <= 2; ++i) {
        fixture();
        assert(s64_code_assets_preload(&dos, &control, &editor, &assets, &system_state));
        fail_read = reads + i;
        assert(!s64_code_assets_store_verified(&control, &journal, &editor, &assets));
        assert(!s64_code_assets_load(&control, &journal, &editor, &loaded));
        fixture();
        assert(s64_code_assets_preload(&dos, &control, &editor, &assets, &system_state));
        mismatch_read = reads + i;
        assert(!s64_code_assets_store_verified(&control, &journal, &editor, &assets));
        assert(!s64_code_assets_load(&control, &journal, &editor, &loaded));
    }
    fixture();
    assert(s64_code_assets_parse(&assets, manifest_wire, sizeof(manifest_wire), &editor));
    /* A partially successful plan must not consume slots or alter descriptors. */
    while (control.allocation_count < S64_REU_RUNTIME_MAX_ALLOCATIONS - S64_CODE_OVERLAY_COUNT) {
        assert(s64_reu_runtime_allocate(&control, S64_REU_OWNER_RUNTIME, 16, 16, 0));
    }
    saved = assets; saved_control = control;
    assert(!s64_code_assets_allocate(&assets, &control));
    assert(!memcmp(&assets, &saved, sizeof(saved)));
    assert(!memcmp(&control, &saved_control, sizeof(control)));
    /* Opposite-module ownership must coexist without sharing caches/records. */
    fixture();
    saved_control = control;
    for (i = 0; i < 4; ++i) {
        assert(s64_reu_runtime_allocate(&control,
            i == 3 ? S64_REU_OWNER_RUNTIME : S64_REU_OWNER_MODULE,
            i == 3 ? S64_CODE_ASSET_SESSION_SIZE : 16, 256, 0));
        control.allocations[control.allocation_count - 1].flags = i == 3
            ? (S64_CODE_ASSET_SESSION_FLAG == 3 ? 5 : 3)
            : (uint8_t)((S64_CODE_ASSET_CACHE_FLAG == 0x21 ? 0x31 : 0x21) + i);
        memset(memory + control.allocations[control.allocation_count - 1].address, 0xa5,
            control.allocations[control.allocation_count - 1].length);
    }
    assert(s64_code_assets_preload(&dos, &control, &editor, &assets, &system_state));
    assert(s64_code_assets_store_verified(&control, &journal, &editor, &assets));
    assert(s64_code_assets_load(&control, &journal, &editor, &loaded));
    assert(s64_code_assets_clear(&control));
    for (i = saved_control.allocation_count; i < control.allocation_count; ++i) {
        uint32_t j;
        for (j = 0; j < control.allocations[i].length; ++j)
            assert(memory[control.allocations[i].address + j] == 0xa5);
    }
    /* The complete production allocation set fits exactly, with both code
     * namespaces and application authority retaining separate ownership. */
    fixture();
    assert(s64_code_assets_parse(&assets, manifest_wire, sizeof(manifest_wire), &editor));
    s64_reu_runtime_init(&control, 1, 123);
    for (i = 0; i < S64_MODULE_COUNT; ++i)
        reserve(S64_REU_OWNER_MODULE, 0, 65536);
    reserve(S64_REU_OWNER_RUNTIME, S64_MODULE_SESSION_ALLOCATION_FLAG, S64_MODULE_SESSION_WIRE_SIZE);
    reserve(S64_REU_OWNER_RUNTIME, S64_TRANSFER_CAPSULE_ALLOCATION_FLAG, S64_TRANSFER_CAPSULE_WIRE_SIZE);
    reserve(S64_REU_OWNER_RUNTIME, S64_BASIC_RETURN_FLAG, S64_BASIC_RETURN_RECORD_SIZE);
    reserve(S64_REU_OWNER_RUNTIME, S64_APPLICATION_STATE_FLAG, S64_APPLICATION_STATE_SIZE);
    for (i = 0; i < (S64_CODE_OVERLAY_COUNT == 4 ? 3u : 4u); ++i)
        reserve(S64_REU_OWNER_MODULE,
            (S64_CODE_ASSET_CACHE_FLAG == 0x21 ? 0x31 : 0x21) + i, 300);
    reserve(S64_REU_OWNER_RUNTIME, S64_CODE_ASSET_SESSION_FLAG == 3 ? 5 : 3,
        S64_CODE_OVERLAY_COUNT == 4 ? 96 : 112);
    assert(control.allocation_count == S64_REU_RUNTIME_MAX_ALLOCATIONS - S64_CODE_OVERLAY_COUNT - 1u);
    assert(s64_code_assets_allocate(&assets, &control));
    assert(control.allocation_count == S64_REU_RUNTIME_MAX_ALLOCATIONS);
    assert(s64_reu_runtime_valid(&control));
    saved_control = control;
    assert(!s64_reu_runtime_allocate(&control, S64_REU_OWNER_RUNTIME, 16, 256, 0));
    assert(!memcmp(&control, &saved_control, sizeof(control)));
    if (argc == 4) {
        FILE* file = fopen(argv[1], "rb");
        assert(file && fread(manifest_wire, 1, sizeof(manifest_wire), file) == sizeof(manifest_wire));
        assert(fgetc(file) == EOF);
        fclose(file);
        editor.byte_length = (uint32_t)strtoul(argv[2], 0, 0);
        editor.integrity = (uint32_t)strtoul(argv[3], 0, 0);
        assert(s64_code_assets_parse(&assets, manifest_wire, sizeof(manifest_wire), &editor));
    }
    puts("code asset manifest/preload/session tests passed");
    return 0;
}
