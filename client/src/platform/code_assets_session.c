#include "platform/code_assets_internal.h"
#include "platform/reu.h"
#include <string.h>

uint8_t s64_code_assets_session_wire[S64_CODE_ASSET_SESSION_SIZE];
uint8_t s64_code_assets_session_journal_wire[S64_SESSION_JOURNAL_WIRE_SIZE];

static uint16_t session_size(const S64CodeAssetNamespace* namespace)
{
    return namespace
        ? (uint16_t)(48u + (uint16_t)namespace->overlay_count * 16u)
        : 0u;
}

uint16_t s64_code_assets_session_checksum_for(
    const S64CodeAssetNamespace* namespace, const uint8_t* bytes)
{
    uint16_t value = 0x64a5u;
    uint16_t size = session_size(namespace);
    uint16_t i;
    if (!namespace || !bytes || !namespace->overlay_count
        || namespace->overlay_count > S64_CODE_OVERLAY_COUNT
        || size > sizeof(s64_code_assets_session_wire)) return 0u;
    for (i = 0; i < size - 2u; ++i) {
        value = (uint16_t)((value << 5) | (value >> 11));
        value ^= bytes[i];
    }
    return value ? value : 1u;
}

uint16_t s64_code_assets_session_checksum(const uint8_t* bytes)
{
    return s64_code_assets_session_checksum_for(
        s64_code_assets_namespace(S64_CODE_ASSET_MODULE), bytes);
}

const S64ReuAllocation* s64_code_assets_session_allocation_for(
    const S64CodeAssetNamespace* namespace,
    const S64ReuRuntimeControl* control)
{
    const S64ReuAllocation* found = 0;
    uint16_t size = session_size(namespace);
    uint8_t i;
    if (!namespace || !namespace->overlay_count
        || namespace->overlay_count > S64_CODE_OVERLAY_COUNT
        || !s64_reu_runtime_valid(control)) return 0;
    for (i = 0; i < control->allocation_count; ++i) {
        const S64ReuAllocation* item = &control->allocations[i];
        if (item->owner == S64_REU_OWNER_RUNTIME
            && item->flags == namespace->session_flag) {
            if (found || item->length != size || (item->address & 255UL)) return 0;
            found = item;
        }
    }
    return found;
}

const S64ReuAllocation* s64_code_assets_session_allocation(
    const S64ReuRuntimeControl* control)
{
    return s64_code_assets_session_allocation_for(
        s64_code_assets_namespace(S64_CODE_ASSET_MODULE), control);
}

uint8_t s64_code_assets_session_verify_for(
    const S64CodeAssetNamespace* namespace,
    const S64ReuAllocation* allocation)
{
    uint8_t readback[S64_CODE_ASSET_SESSION_SIZE];
    uint16_t size = session_size(namespace);
    if (!namespace || !allocation || !size
        || size > sizeof(readback) || allocation->length != size) return 0u;
    return s64_reu_load((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, readback, size)
        && memcmp(s64_code_assets_session_wire, readback, size) == 0;
}

uint8_t s64_code_assets_session_verify(const S64ReuAllocation* allocation)
{
    return s64_code_assets_session_verify_for(
        s64_code_assets_namespace(S64_CODE_ASSET_MODULE), allocation);
}

uint8_t s64_code_assets_clear_for(const S64CodeAssetNamespace* namespace,
    const S64ReuRuntimeControl* control)
{
    const S64ReuAllocation* allocation =
        s64_code_assets_session_allocation_for(namespace, control);
    uint16_t size = session_size(namespace);
    if (!allocation || !size || size > sizeof(s64_code_assets_session_wire)) return 0;
    memset(s64_code_assets_session_wire, 0, size);
    return s64_reu_store((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, s64_code_assets_session_wire, size)
        && s64_code_assets_session_verify_for(namespace, allocation);
}

uint8_t s64_code_assets_clear(const S64ReuRuntimeControl* control)
{
    return s64_code_assets_clear_for(
        s64_code_assets_namespace(S64_CODE_ASSET_MODULE), control);
}

uint8_t s64_code_assets_active_session(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal)
{
    return journal != 0 && journal->state == S64_SESSION_STATE_ACTIVE && journal->sequence
        && journal->session_id == control->session_id && journal->package_version == control->app_version
        && s64_session_journal_encode(journal, s64_code_assets_session_journal_wire);
}
