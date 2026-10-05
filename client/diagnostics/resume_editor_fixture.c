#include "resume_editor_gate.h"
#include "resume_fixture.h"
#include "platform/application_state.h"
#include "platform/crc32.h"
#include "platform/module_integrity.h"
#include "platform/reu.h"
#include <string.h>

static uint8_t load_fixture(S64LauncherRecoveryRuntime* r, const char* filename,
    uint32_t address, uint32_t length, uint32_t crc)
{
    uint32_t size;
    S64DosResult result, closed;
    if (s64_dos_open_read(&r->dos, filename) != S64_DOS_OK) return 0;
    result = s64_dos_file_size(&r->dos, &size);
    if (result == S64_DOS_OK && size != length) result = S64_DOS_TARGET_ERROR;
    if (result == S64_DOS_OK) result = s64_dos_load_reu(&r->dos, address, length);
    closed = s64_dos_close(&r->dos);
    return result == S64_DOS_OK && closed == S64_DOS_OK
        && s64_reu_crc32_matches(address, length, crc, &r->system_state);
}

uint8_t s64_resume_editor_fixture(S64LauncherRecoveryRuntime* r,
    uint8_t corrupt, uint8_t* w)
{
    const S64ReuAllocation* allocation;
    uint32_t crc;
    uint32_t address;
    uint16_t offset;
    uint8_t i;
    if (!r || !w || !r->lifecycle.active_record_written
        || r->lifecycle.clean_record_written || r->module_session_invalidated
        || !r->system_state.captured || r->system_state.restored
        || r->system_state.mode != S64_SYSTEM_MODE_IO_SAFE
        || r->lifecycle.journal.state != S64_SESSION_STATE_ACTIVE
        || !r->lifecycle.journal.sequence
        || r->lifecycle.journal.session_id != r->reu_control.session_id
        || r->lifecycle.journal.package_version != r->reu_control.app_version)
        return 0;
    allocation = s64_application_state_allocation(&r->reu_control);
    if (!allocation || (r->reu_control.state_flags & S64_REU_RUNTIME_STATE_APPLICATION_VALID))
        return 0;
    /* Persist clear authority before replacing any fixture bytes. */
    if (!s64_reu_runtime_store_verified(&r->reu_control)
        || s64_dos_change_dir(&r->dos, "/sd/") != S64_DOS_OK
        || !load_fixture(r, "S64RDOC.DAT", S64_REU_DOCUMENT_BASE,
            S64_RESUME_DOCUMENT_LENGTH, S64_RESUME_DOCUMENT_CRC)
        || !load_fixture(r, "S64RHP.DAT", S64_REU_HISTORY_PAYLOAD_BASE,
            S64_RESUME_HISTORY_PAYLOAD_LENGTH, S64_RESUME_HISTORY_PAYLOAD_CRC)
        || !load_fixture(r, "S64RHM.DAT", 0x10000UL + S64_REU_HISTORY_META_BASE,
            S64_RESUME_HISTORY_META_LENGTH, S64_RESUME_HISTORY_META_CRC)
        || !load_fixture(r, "S64RCLP.DAT", 0x10000UL + S64_REU_CLIPBOARD_BASE,
            S64_RESUME_CLIPBOARD_LENGTH, S64_RESUME_CLIPBOARD_CRC)
        || !load_fixture(r, "S64RAPP.DAT", allocation->address,
            S64_APPLICATION_STATE_SIZE, S64_RESUME_STATE_CRC)
        || !s64_reu_load((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, w, S64_APPLICATION_STATE_SIZE)) return 0;
    /* Only the existing journal binding and CRC change in the production wire.
     * BASIC capture has already been durably published, so C800 is reusable. */
    if (!s64_session_journal_encode(&r->lifecycle.journal, w + 8)) return 0;
    crc = s64_crc32_update(0xffffffffUL, w, 508) ^ 0xffffffffUL;
    for (i = 0; i < 4; ++i) w[508 + i] = (uint8_t)(crc >> (8 * i));
    if (!s64_reu_store((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, w, S64_APPLICATION_STATE_SIZE)) return 0;
    /* The appended CRC has a fixed full-record residue, so verify exact bytes,
     * following production state-store readback rather than hashing all 512. */
    for (offset = 0; offset < S64_APPLICATION_STATE_SIZE; offset += 32) {
        address = allocation->address + offset;
        if (!s64_reu_load((uint8_t)(address >> 16), (uint16_t)address, w + 512, 32)
            || memcmp(w + offset, w + 512, 32)) return 0;
    }
    r->reu_control.state_flags |= S64_REU_RUNTIME_STATE_APPLICATION_VALID;
    if (!s64_reu_runtime_store_verified(&r->reu_control)) return 0;
    if (corrupt) {
        /* One bounded negative: alter generation after valid publication,
         * retaining authority and the original record CRC. */
        w[32] ^= 1;
        if (!s64_reu_store((uint8_t)(allocation->address >> 16),
                (uint16_t)(allocation->address + 32), w + 32, 1)
            || !s64_reu_load((uint8_t)(allocation->address >> 16),
                (uint16_t)(allocation->address + 32), w + 512, 1)
            || w[512] != w[32]) return 0;
    }
    return 1;
}
