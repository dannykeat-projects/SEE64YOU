#include "resume_editor_gate.h"
#include "resume_fixture.h"
#include "platform/application_state.h"
#include "platform/managed_editor_state.h"
#include "platform/module_integrity.h"
#include "platform/package_version.h"
#include "platform/crc32.h"
#include "platform/reu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t reu[0x40000], workspace[1024], original[512];
static S64LauncherRecoveryRuntime launcher;
static S64ModuleRuntime consumer;
static FILE* file;
static const char* fixture_directory;
static unsigned opened, loaded, stored, verified, crc_checks;
static unsigned fail_control, corrupt_payload, wrong_size, fail_state_read, corrupt_state_read;
static unsigned state_reads;
static unsigned current_file;
static const char* filenames[] = {
    "S64RDOC.DAT", "S64RHP.DAT", "S64RHM.DAT", "S64RCLP.DAT", "S64RAPP.DAT"
};
static const uint32_t lengths[] = {
    S64_RESUME_DOCUMENT_LENGTH, S64_RESUME_HISTORY_PAYLOAD_LENGTH,
    S64_RESUME_HISTORY_META_LENGTH, S64_RESUME_CLIPBOARD_LENGTH, 512
};
static const uint32_t crcs[] = {
    S64_RESUME_DOCUMENT_CRC, S64_RESUME_HISTORY_PAYLOAD_CRC,
    S64_RESUME_HISTORY_META_CRC, S64_RESUME_CLIPBOARD_CRC, S64_RESUME_STATE_CRC
};

uint8_t s64_reu_load(uint8_t bank, uint16_t offset, void* out, uint16_t length)
{
    uint32_t address = ((uint32_t)bank << 16) | offset;
    assert(address + length <= sizeof(reu));
    if (address == 0x20000 && length == S64_REU_RUNTIME_WIRE_SIZE
        && verified == fail_control) return 0;
    if (length == 32 && address >= launcher.reu_control.allocations[0].address
        && address < launcher.reu_control.allocations[0].address + 512) {
        ++state_reads;
        if (state_reads == fail_state_read) return 0;
    }
    memcpy(out, reu + address, length);
    if (length == 32 && corrupt_state_read && state_reads == corrupt_state_read)
        ((uint8_t*)out)[31] ^= 1;
    return 1;
}

uint8_t s64_reu_store(uint8_t bank, uint16_t offset, const void* in, uint16_t length)
{
    uint32_t address = ((uint32_t)bank << 16) | offset;
    assert(address + length <= sizeof(reu));
    if (address == 0x20000 && length == S64_REU_RUNTIME_WIRE_SIZE) {
        ++verified;
        if (verified == 1) assert(!((const uint8_t*)in)[12] && !loaded);
        else {
            assert(verified == 2 && loaded == 5 && crc_checks == 5 && state_reads == 16);
            assert(((const uint8_t*)in)[12] & S64_REU_RUNTIME_STATE_APPLICATION_VALID);
        }
    }
    memcpy(reu + address, in, length); ++stored; return 1;
}

S64DosResult s64_dos_change_dir(S64Dos* dos, const char* directory)
{
    assert(dos == &launcher.dos && !strcmp(directory, "/sd/"));
    return S64_DOS_OK;
}

S64DosResult s64_dos_open_read(S64Dos* dos, const char* filename)
{
    char path[1024];
    assert(dos == &launcher.dos && !file && opened < 5);
    current_file = opened++;
    assert(!strcmp(filename, filenames[current_file]));
    assert(snprintf(path, sizeof(path), "%s/%s", fixture_directory, filename) > 0);
    file = fopen(path, "rb"); assert(file);
    return S64_DOS_OK;
}

S64DosResult s64_dos_file_size(S64Dos* dos, uint32_t* size)
{
    long length;
    assert(dos == &launcher.dos && file);
    assert(!fseek(file, 0, SEEK_END)); length = ftell(file); assert(length >= 0);
    assert(!fseek(file, 0, SEEK_SET));
    *size = (uint32_t)length + (wrong_size == current_file + 1);
    return S64_DOS_OK;
}

S64DosResult s64_dos_load_reu(S64Dos* dos, uint32_t address, uint32_t length)
{
    uint32_t expected;
    assert(dos == &launcher.dos && file && verified == 1);
    assert(length == lengths[current_file] && address + length <= sizeof(reu));
    switch (current_file) {
        case 0: expected = S64_REU_DOCUMENT_BASE; break;
        case 1: expected = S64_REU_HISTORY_PAYLOAD_BASE; break;
        case 2: expected = 0x10000UL + S64_REU_HISTORY_META_BASE; break;
        case 3: expected = 0x10000UL + S64_REU_CLIPBOARD_BASE; break;
        default: expected = s64_application_state_allocation(&launcher.reu_control)->address; break;
    }
    assert(address == expected);
    assert(fread(reu + address, 1, length, file) == length);
    if (current_file == 4) memcpy(original, reu + address, sizeof(original));
    if (corrupt_payload == current_file + 1) reu[address] ^= 1;
    ++loaded; return S64_DOS_OK;
}

S64DosResult s64_dos_close(S64Dos* dos)
{
    assert(dos == &launcher.dos && file); assert(!fclose(file)); file = 0;
    return S64_DOS_OK;
}

uint8_t s64_reu_crc32_matches(uint32_t address, uint32_t length,
    uint32_t expected, S64SystemState* system)
{
    assert(system == &launcher.system_state && system->mode == S64_SYSTEM_MODE_IO_SAFE);
    assert(address + length <= sizeof(reu));
    ++crc_checks;
    if (crc_checks <= 5) assert(expected == crcs[current_file]);
    return expected == (s64_crc32_update(0xffffffffUL, reu + address, length) ^ 0xffffffffUL);
}

static void setup(void)
{
    assert(!file);
    memset(reu, 0xa5, sizeof(reu)); memset(&launcher, 0, sizeof(launcher));
    opened = loaded = stored = verified = crc_checks = state_reads = 0;
    fail_control = corrupt_payload = wrong_size = fail_state_read = corrupt_state_read = 0;
    s64_reu_runtime_init(&launcher.reu_control, S64_PACKAGE_VERSION, 0x12345678UL);
    assert(s64_application_state_reserve(&launcher.reu_control));
    launcher.lifecycle.active_record_written = 1;
    launcher.lifecycle.journal.state = S64_SESSION_STATE_ACTIVE;
    launcher.lifecycle.journal.sequence = 81;
    launcher.lifecycle.journal.session_id = launcher.reu_control.session_id;
    launcher.lifecycle.journal.package_version = S64_PACKAGE_VERSION;
    launcher.system_state.captured = 1;
    launcher.system_state.mode = S64_SYSTEM_MODE_IO_SAFE;
}

static void adopt_consumer(void)
{
    memset(&consumer, 0, sizeof(consumer));
    consumer.reu_control = launcher.reu_control;
    consumer.lifecycle.journal = launcher.lifecycle.journal;
    consumer.current_module = S64_MODULE_EDITOR;
}

int main(int argc, char** argv)
{
    const S64ReuAllocation* allocation;
    S64ApplicationState* state;
    uint8_t expected[S64_SESSION_JOURNAL_WIRE_SIZE];
    unsigned i;
    assert(argc == 2); fixture_directory = argv[1];
    setup(); assert(s64_resume_editor_fixture(&launcher, 0, workspace));
    assert(opened == 5 && loaded == 5 && verified == 2 && crc_checks == 5 && state_reads == 16);
    allocation = s64_application_state_allocation(&launcher.reu_control);
    assert(s64_session_journal_encode(&launcher.lifecycle.journal, expected));
    assert(!memcmp(reu + allocation->address + 8, expected, sizeof(expected)));
    /* Live journal and checksum are the only edits to generated fixture wire. */
    assert(!memcmp(reu + allocation->address, original, 8));
    assert(!memcmp(reu + allocation->address + 32, original + 32, 476));
    assert(reu[0x20000 + 12] & S64_REU_RUNTIME_STATE_APPLICATION_VALID);
    adopt_consumer(); assert(s64_managed_editor_state_prepare(&consumer, workspace) == 2);
    state = s64_managed_editor_state_decode(&consumer, workspace);
    assert(state && state->generation == 41 && state->source_module == S64_MODULE_EDITOR);
    assert(state->editor.selection_valid && state->editor.buffer.cursor > 0);
    assert(state->clipboard_length == 4 && state->search_valid && state->filename[0]);

    /* Every independently loaded payload must authenticate before authority. */
    for (i = 1; i <= 5; ++i) {
        setup(); corrupt_payload = i;
        assert(!s64_resume_editor_fixture(&launcher, 0, workspace));
        assert(loaded == i && verified == 1 && !launcher.reu_control.state_flags);
        assert(!(reu[0x20000 + 12] & S64_REU_RUNTIME_STATE_APPLICATION_VALID));
        setup(); wrong_size = i;
        assert(!s64_resume_editor_fixture(&launcher, 0, workspace));
        assert(loaded == i - 1 && verified == 1 && !launcher.reu_control.state_flags);
    }
    setup(); ++launcher.lifecycle.journal.session_id;
    assert(!s64_resume_editor_fixture(&launcher, 0, workspace) && !opened && !verified);
    setup(); launcher.system_state.mode = S64_SYSTEM_MODE_INTERACTIVE;
    assert(!s64_resume_editor_fixture(&launcher, 0, workspace) && !opened && !verified);
    setup(); launcher.reu_control.state_flags = S64_REU_RUNTIME_STATE_APPLICATION_VALID;
    assert(!s64_resume_editor_fixture(&launcher, 0, workspace) && !opened && !verified);
    setup(); fail_control = 1;
    assert(!s64_resume_editor_fixture(&launcher, 0, workspace) && !loaded);
    setup(); fail_control = 2;
    assert(!s64_resume_editor_fixture(&launcher, 0, workspace));
    assert(verified == 2 && loaded == 5); /* Caller must not prepare transfer. */
    assert(reu[0x20000 + 12] & S64_REU_RUNTIME_STATE_APPLICATION_VALID);
    setup(); fail_state_read = 16;
    assert(!s64_resume_editor_fixture(&launcher, 0, workspace));
    assert(verified == 1 && !launcher.reu_control.state_flags);
    setup(); corrupt_state_read = 16;
    assert(!s64_resume_editor_fixture(&launcher, 0, workspace));
    assert(verified == 1 && !launcher.reu_control.state_flags);

    setup(); assert(s64_resume_editor_fixture(&launcher, 1, workspace));
    assert(launcher.reu_control.state_flags & S64_REU_RUNTIME_STATE_APPLICATION_VALID);
    adopt_consumer(); assert(s64_managed_editor_state_prepare(&consumer, workspace) == 0);
    assert(!s64_managed_editor_state_decode(&consumer, workspace));
    assert(consumer.reu_control.state_flags & S64_REU_RUNTIME_STATE_APPLICATION_VALID);
    puts("resumed Editor diagnostic fixture binding/failure/consumer tests passed");
    return 0;
}
