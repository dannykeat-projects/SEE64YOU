#include "platform/module_runtime.h"
#include "platform/package_paths.h"

static uint8_t s64_pubprep_reu_runtime_allocate(
    S64ReuRuntimeControl* control,
    S64ReuOwner owner,
    uint32_t length,
    uint16_t alignment,
    S64ReuAllocation* allocation
)
{
    uint32_t mask;
    uint32_t start;
    S64ReuAllocation* item;

    if (!s64_reu_runtime_valid(control)
        || owner < S64_REU_OWNER_RUNTIME
        || owner > S64_REU_OWNER_UPDATE
        || length == 0UL
        || alignment == 0u
        || (alignment & (uint16_t)(alignment - 1u)) != 0u
        || control->allocation_count >= S64_REU_RUNTIME_MAX_ALLOCATIONS) {
        return 0u;
    }

    mask = (uint32_t)alignment - 1UL;
    start = (control->next_free + mask) & ~mask;
    if (start < control->next_free
        || start >= S64_REU_RUNTIME_HEAP_END
        || length > S64_REU_RUNTIME_HEAP_END - start) {
        return 0u;
    }

    item = &control->allocations[control->allocation_count];
    item->owner = (uint8_t)owner;
    item->flags = 0u;
    item->address = start;
    item->length = length;
    ++control->allocation_count;
    control->next_free = start + length;
    if (allocation) *allocation = *item;
    return 1u;
}

/*
 * Reuse the already-tested compact asset producer verbatim, specialised for
 * the Publisher namespace. Rename its public symbols so the managed Editor's
 * resident Editor-namespace consumer remains independent.
 */
#define S64_CODE_ASSET_PUBLISHER 1
#define s64_code_assets_parse s64_pubprep_code_assets_parse
#define s64_code_assets_allocate s64_pubprep_code_assets_allocate
#define s64_code_assets_preload s64_pubprep_code_assets_preload
#define s64_code_assets_clear s64_pubprep_code_assets_clear
#define s64_code_assets_store_verified s64_pubprep_code_assets_store_verified
#define s64_code_assets_editor_valid s64_pubprep_code_assets_editor_valid
#define s64_code_assets_valid s64_pubprep_code_assets_valid
#define s64_code_assets_session_allocation s64_pubprep_code_assets_session_allocation
#define s64_code_assets_session_verify s64_pubprep_code_assets_session_verify
#define s64_code_assets_active_session s64_pubprep_code_assets_active_session
#define s64_code_assets_session_checksum s64_pubprep_code_assets_session_checksum
#define s64_code_assets_session_wire s64_pubprep_code_assets_session_wire
#define s64_code_assets_session_journal_wire s64_pubprep_code_assets_session_journal_wire
#define s64_reu_runtime_allocate s64_pubprep_reu_runtime_allocate

#include "platform/code_assets.c"
#include "platform/code_assets_validate.c"
#include "platform/code_assets_session.c"
#include "platform/code_assets_session_build.c"

#include "platform/managed_editor_publisher_assets.h"

#include <string.h>

typedef struct {
    uint8_t* wire;
    uint16_t received;
} S64PublisherManifestRead;

static uint8_t manifest_sink(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64PublisherManifestRead* read = (S64PublisherManifestRead*)context;

    if (!data || !read || !read->wire
        || length > S64_CODE_ASSET_MANIFEST_SIZE - read->received) {
        return 0u;
    }
    memcpy(read->wire + read->received, data, length);
    read->received = (uint16_t)(read->received + length);
    return 1u;
}

static uint8_t load_manifest(
    S64ModuleRuntime* runtime,
    S64CodeAssets* assets,
    uint8_t* scratch
)
{
    S64PublisherManifestRead manifest_read;
    S64DosResult result;
    S64DosResult closed;
    uint32_t size;
    uint16_t read = 0u;

    if (!runtime || !runtime->dos || !assets || !scratch
        || s64_dos_change_dir(runtime->dos, S64_PACKAGE_DIRECTORY) != S64_DOS_OK
        || s64_dos_open_read(runtime->dos,
            S64_CODE_ASSET_MANIFEST_FILENAME) != S64_DOS_OK) {
        return 0u;
    }

    manifest_read.wire = scratch;
    manifest_read.received = 0u;
    result = s64_dos_file_size(runtime->dos, &size);
    if (result == S64_DOS_OK && size != S64_CODE_ASSET_MANIFEST_SIZE) {
        result = S64_DOS_TARGET_ERROR;
    }
    if (result == S64_DOS_OK) {
        result = s64_dos_read(runtime->dos,
            S64_CODE_ASSET_MANIFEST_SIZE, manifest_sink,
            &manifest_read, &read);
    }
    closed = s64_dos_close(runtime->dos);

    return result == S64_DOS_OK
        && closed == S64_DOS_OK
        && read == S64_CODE_ASSET_MANIFEST_SIZE
        && manifest_read.received == read
        && s64_code_assets_parse(
            assets,
            scratch,
            S64_CODE_ASSET_MANIFEST_SIZE,
            &runtime->module_manifest.modules[S64_MODULE_PUBLISHER - 1u]
        );
}

static void rollback_control(
    S64ModuleRuntime* runtime,
    const S64ReuRuntimeControl* original
)
{
    /* A stale S64Q record is harmless once its allocation disappears from the
     * verified control, but clear it first when the new allocation is usable. */
    (void)s64_code_assets_clear(&runtime->reu_control);
    runtime->reu_control = *original;
    if (!s64_reu_runtime_store_verified(&runtime->reu_control)) {
        runtime->shared_invalidated = 1u;
    }
}

uint8_t s64_managed_editor_pub_assets_prepare(
    S64ModuleRuntime* runtime,
    uint8_t* scratch
)
{
    S64ReuRuntimeControl original;
    S64CodeAssets assets;

    if (!runtime || !scratch
        || runtime->current_module != S64_MODULE_EDITOR
        || !runtime->lifecycle.active_record_written
        || runtime->lifecycle.clean_record_written
        || runtime->shared_invalidated
        || !runtime->system_state.captured
        || runtime->system_state.restored
        || runtime->system_state.mode != S64_SYSTEM_MODE_IO_SAFE
        || runtime->module_manifest.modules[S64_MODULE_PUBLISHER - 1u].id
            != S64_MODULE_PUBLISHER) {
        return 0u;
    }

    if (!load_manifest(runtime, &assets, scratch)) return 0u;

    original = runtime->reu_control;
    if (!s64_code_assets_allocate(&assets, &runtime->reu_control)
        || runtime->reu_control.allocation_count
            != S64_REU_RUNTIME_MAX_ALLOCATIONS
        || !s64_reu_runtime_store_verified(&runtime->reu_control)) {
        rollback_control(runtime, &original);
        return 0u;
    }

    if (!s64_code_assets_preload(
            runtime->dos,
            &runtime->reu_control,
            &runtime->module_manifest.modules[S64_MODULE_PUBLISHER - 1u],
            &assets,
            &runtime->system_state
        )
        || !s64_code_assets_store_verified(
            &runtime->reu_control,
            &runtime->lifecycle.journal,
            &runtime->module_manifest.modules[S64_MODULE_PUBLISHER - 1u],
            &assets
        )) {
        rollback_control(runtime, &original);
        return 0u;
    }

    return 1u;
}
