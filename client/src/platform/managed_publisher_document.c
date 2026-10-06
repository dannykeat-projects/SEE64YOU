#include <string.h>
#include "platform/managed_publisher.h"
#include "platform/application_state.h"
#include "platform/application_state_internal.h"
#include "platform/crc32.h"
#include "platform/reu.h"
#include "publisher/workspace.h"

#define RETURN_CONTROL_OFFSET S64_APPLICATION_STATE_SIZE
#define RETURN_READBACK_SIZE 32u

/* The 768-byte Publisher workspace reserves bytes 512..767 for synchronous
 * return verification. S64ReuRuntimeControl's fixed 16-entry representation
 * is bounded by the 192-byte runtime wire contract, so it fits this 256-byte
 * tail without adding resident BSS. */
static S64ApplicationState application;
extern S64ModuleRuntime* s64_managed_publisher_document_owner;

static uint16_t get16(const uint8_t* p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t get32(const uint8_t* p)
{
    return (uint32_t)get16(p) | ((uint32_t)get16(p + 2u) << 16);
}

static void put16(uint8_t* p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put32(uint8_t* p, uint32_t value)
{
    put16(p, (uint16_t)value);
    put16(p + 2u, (uint16_t)(value >> 16));
}

static uint8_t metadata_changed(const uint8_t* wire,
    const S64Document* document)
{
    return wire[76] != (uint8_t)document->type
        || wire[77] != (uint8_t)document->status
        || get32(wire + 78) != (uint32_t)document->order
        || memcmp(wire + 82, document->slug, S64_DOCUMENT_SLUG_CAPACITY)
        || memcmp(wire + 147, document->title, S64_DOCUMENT_TITLE_CAPACITY)
        || memcmp(wire + 228, document->date, S64_DOCUMENT_DATE_CAPACITY);
}

static uint8_t control_store_verified(S64ModuleRuntime* runtime, uint8_t* wire)
{
    S64ReuRuntimeControl* readback =
        (S64ReuRuntimeControl*)(wire + RETURN_CONTROL_OFFSET);
    return s64_reu_runtime_store(&runtime->reu_control)
        && s64_reu_runtime_load(readback)
        && memcmp(&runtime->reu_control, readback,
            sizeof(runtime->reu_control)) == 0;
}

static uint8_t application_wire_store_verified(S64ModuleRuntime* runtime,
    uint8_t* wire)
{
    const S64ReuAllocation* allocation =
        s64_application_state_allocation(&runtime->reu_control);
    uint8_t* readback = wire + RETURN_CONTROL_OFFSET;
    uint16_t offset;
    uint32_t address;

    if (!allocation
        || !s64_reu_store((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, wire,
            S64_APPLICATION_STATE_SIZE)) {
        return 0u;
    }

    for (offset = 0u; offset < S64_APPLICATION_STATE_SIZE;
            offset += RETURN_READBACK_SIZE) {
        address = allocation->address + offset;
        if (!s64_reu_load((uint8_t)(address >> 16), (uint16_t)address,
                readback, RETURN_READBACK_SIZE)
            || memcmp(wire + offset, readback, RETURN_READBACK_SIZE)) {
            return 0u;
        }
    }
    return 1u;
}

static uint8_t prepare_return_wire(S64ModuleRuntime* runtime,
    const S64Buffer* body, const S64Document* document, uint8_t replaced,
    uint8_t* wire)
{
    uint32_t generation;
    uint8_t changed;

    if (!s64_application_state_load_wire(&runtime->reu_control,
            &runtime->lifecycle.journal, wire)) {
        return 0u;
    }

    generation = get32(wire + 32);
    if (!generation || generation == 0xffffffffUL) return 0u;
    changed = metadata_changed(wire, document);

    if (replaced) {
        put16(wire + 40, body->gap_start);
        put16(wire + 42, body->gap_end);
        put16(wire + 44, 0u);
        /* History, viewport, selection and preferred-position controls. */
        memset(wire + 46, 0, 25u);
        put16(wire + 56, 1u);
        put16(wire + 58, 1u);
        wire[71] = 1u;
        memset(wire + 269, 0, S64_LOCAL_FILENAME_CAPACITY);
        wire[37] = 1u;
    } else if (changed) {
        wire[71] = 1u;
    }

    wire[36] = S64_MODULE_PUBLISHER;
    put32(wire + 32, generation + 1UL);
    wire[76] = (uint8_t)document->type;
    wire[77] = (uint8_t)document->status;
    put32(wire + 78, (uint32_t)document->order);
    memcpy(wire + 82, document->slug, S64_DOCUMENT_SLUG_CAPACITY);
    memcpy(wire + 147, document->title, S64_DOCUMENT_TITLE_CAPACITY);
    memcpy(wire + 228, document->date, S64_DOCUMENT_DATE_CAPACITY);

    if (!s64_application_state_payload_valid(wire)) return 0u;
    put32(wire + 508,
        s64_crc32_update(0xffffffffUL, wire, 508u) ^ 0xffffffffUL);
    return s64_application_state_wire_valid(&runtime->reu_control,
        &runtime->lifecycle.journal, wire);
}

uint8_t s64_managed_publisher_document(S64ModuleRuntime* runtime,
    S64Buffer* body, S64Document* document, uint8_t* dirty, uint8_t* replaced)
{
    uint8_t authorized = runtime && s64_managed_publisher_document_owner == runtime;
    s64_managed_publisher_document_owner = 0;
    if (!authorized || !body || !document || !dirty || !replaced
        || runtime->current_module != S64_MODULE_PUBLISHER
        || !s64_application_state_decode_payload(&application,
            s64_publisher_workspace.bytes)) return 0u;
    *body = application.editor.buffer;
    document->body = body;
    document->type = application.document.type;
    document->status = application.document.status;
    document->order = application.document.order;
    memcpy(document->slug, application.document.slug, sizeof(document->slug));
    memcpy(document->title, application.document.title, sizeof(document->title));
    memcpy(document->date, application.document.date, sizeof(document->date));
    *dirty = application.editor.dirty;
    *replaced = application.document_replaced;
    return 1u;
}

uint8_t s64_managed_publisher_return_state(S64ModuleRuntime* runtime,
    const S64Buffer* body, const S64Document* document, uint8_t replaced,
    uint8_t* wire)
{
    if (!runtime || !body || !document || !wire || replaced > 1u
        || document->body != body
        || runtime->current_module != S64_MODULE_PUBLISHER
        || !runtime->lifecycle.active_record_written
        || runtime->lifecycle.clean_record_written
        || runtime->shared_invalidated
        || !runtime->system_state.captured
        || runtime->system_state.restored
        || runtime->system_state.mode != S64_SYSTEM_MODE_INTERACTIVE
        || !(runtime->reu_control.state_flags
            & S64_REU_RUNTIME_STATE_APPLICATION_VALID)
        || !prepare_return_wire(runtime, body, document, replaced, wire)) {
        return 0u;
    }

    if (s64_system_state_set_mode(&runtime->system_state,
            S64_SYSTEM_MODE_IO_SAFE) != S64_SYSTEM_OK) {
        return 2u;
    }

    runtime->reu_control.state_flags &=
        (uint8_t)~S64_REU_RUNTIME_STATE_APPLICATION_VALID;
    if (!control_store_verified(runtime, wire)) return 2u;

    if (!application_wire_store_verified(runtime, wire)) return 2u;

    runtime->reu_control.state_flags |= S64_REU_RUNTIME_STATE_APPLICATION_VALID;
    if (!control_store_verified(runtime, wire)) {
        runtime->reu_control.state_flags &=
            (uint8_t)~S64_REU_RUNTIME_STATE_APPLICATION_VALID;
        (void)control_store_verified(runtime, wire);
        return 2u;
    }

    return s64_module_runtime_resume_interactive(runtime) == S64_SYSTEM_OK
        ? 1u : 2u;
}
