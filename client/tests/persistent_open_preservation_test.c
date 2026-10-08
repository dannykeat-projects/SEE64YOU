#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "editor.h"
#include "platform/reu.h"
#include "publisher/persistent_services.h"

/* Real Editor/buffer/history/codec/allocator; only the REU device is simulated. */
static uint8_t reu[0x1000000];
static uint8_t before[131072];
static S64Editor editor, saved_editor;
static S64Clipboard clipboard;
static S64Document document, saved_document;
static S64ReuRuntimeControl control;
static S64PersistentOpenStage stage;
static uint32_t fail_start, fail_end;
static unsigned int fail_after;
static uint8_t fail_read, fail_write, fail_forever, silent_corruption;
static const uint8_t response[] =
    "SEE64YOU/1\nType: page\nSlug: remote\nTitle: Remote\n"
    "Status: draft\nDate: \nOrder: -7\n\nREPLACEMENT";

uint8_t s64_reu_probe(void) { return 1u; }
uint16_t s64_reu_probe_banks(void) { return 256u; }

static uint8_t fault(uint32_t address, uint16_t length, uint8_t write)
{
    if (!(write ? fail_write : fail_read) || !length
        || address >= fail_end || address + length <= fail_start) return 0u;
    if (fail_after && --fail_after) return 0u;
    if (!fail_forever) fail_read = fail_write = 0u;
    return 1u;
}

uint8_t s64_reu_store(uint8_t bank, uint16_t address,
    const void* source, uint16_t length)
{
    uint32_t offset = ((uint32_t)bank << 16) + address;
    if (!source || offset + length > sizeof(reu)) return 0u;
    if (fault(offset, length, 1u)) {
        /* A failed DMA may have already changed destination bytes. */
        memcpy(reu + offset, source, length / 2u);
        if (silent_corruption && length) reu[offset] ^= 0xffu;
        return silent_corruption;
    }
    memcpy(reu + offset, source, length);
    return 1u;
}

uint8_t s64_reu_load(uint8_t bank, uint16_t address,
    void* destination, uint16_t length)
{
    uint32_t offset = ((uint32_t)bank << 16) + address;
    if (!destination || offset + length > sizeof(reu)
        || fault(offset, length, 0u)) return 0u;
    memcpy(destination, reu + offset, length);
    return 1u;
}

static void inject(uint32_t first, uint32_t end, uint8_t write)
{
    fail_start = first; fail_end = end; fail_after = 1u;
    fail_read = !write; fail_write = write;
}

static void setup(void)
{
    const char* text = "UNSAVED LOCAL SOURCE";
    unsigned int i;
    fail_read = fail_write = fail_forever = silent_corruption = 0u;
    memset(reu, 0, sizeof(reu));
    s64_editor_init(&editor);
    for (i = 0u; text[i]; ++i)
        assert(s64_editor_insert(&editor, (unsigned char)text[i]));
    s64_editor_left(&editor);
    s64_editor_right(&editor);
    assert(s64_editor_backspace(&editor) && editor.dirty);
    memset(&document, 0, sizeof(document));
    document.body = &editor.buffer;
    document.type = S64_DOCUMENT_POST;
    document.status = S64_DOCUMENT_PUBLISHED;
    strcpy(document.slug, "original");
    strcpy(document.title, "Unsaved local title");
    document.order = 42;
    s64_editor_select_all(&editor);
    assert(s64_editor_copy(&editor, &clipboard));
    saved_document = document;
    saved_editor = editor;
    memcpy(before, reu, sizeof(before));
    s64_reu_runtime_init(&control, 514u, 123u);
    assert(s64_persistent_open_stage_init(&stage, &control));
}

static void preserved(void)
{
    assert(!stage.recovery_required);
    assert(!memcmp(before, reu, sizeof(before)));
    assert(!memcmp(&saved_document, &document, sizeof(document)));
    assert(!memcmp(&saved_editor, &editor, sizeof(editor)));
}

static void receive(void)
{
    assert(s64_persistent_open_stage_write(response, sizeof(response) - 1u, &stage));
    assert(s64_persistent_open_stage_finish(&stage));
}

static void commit_fails(void)
{
    assert(s64_persistent_open_stage_commit(&stage, &document) != S64_DOCUMENT_OK);
    preserved();
}

int main(void)
{
    uint8_t allocation_count;
    uint32_t next_free;
    unsigned int i;
    uint8_t chunk[256];
    S64ReuAllocation unused;
    S64ReuRuntimeControl full;
    S64PersistentOpenStage rejected;

    setup();
    assert(s64_persistent_open_stage_write(response, sizeof(response) - 1u, &stage));
    /* Incomplete transport or cancellation: no finish means no commit authority. */
    preserved();
    commit_fails();
    assert(s64_editor_undo(&editor));
    assert(s64_buffer_length(&editor.buffer) == strlen("UNSAVED LOCAL SOURCE"));
    assert(s64_editor_redo(&editor));
    assert(s64_buffer_length(&editor.buffer) == strlen("UNSAVED LOCAL SOURC"));
    puts("PASS incomplete/cancelled receive; original undo/redo still works");

    setup(); receive();
    reu[stage.address] = '!';
    commit_fails();
    setup(); receive();
    reu[stage.address + S64_OPEN_BODY_OFFSET] = 1u;
    commit_fails();
    puts("PASS malformed metadata and invalid body conversion");

    setup();
    inject(stage.address, stage.address + S64_OPEN_STORAGE_SIZE, 1u);
    assert(!s64_persistent_open_stage_write(response, sizeof(response) - 1u, &stage));
    commit_fails();
    setup(); receive();
    inject(stage.address, stage.address + S64_OPEN_BODY_OFFSET, 0u);
    commit_fails();
    setup(); receive();
    inject(stage.address + S64_OPEN_BODY_OFFSET, stage.address + S64_OPEN_BACKUP_OFFSET, 0u);
    commit_fails();
    setup(); receive();
    inject(stage.address + S64_OPEN_BODY_OFFSET, stage.address + S64_OPEN_BACKUP_OFFSET, 1u);
    commit_fails();
    puts("PASS receive/header/body DMA failures before live writes");

    setup(); receive();
    inject(stage.address + S64_OPEN_BACKUP_OFFSET, stage.address + S64_OPEN_STORAGE_SIZE, 1u);
    commit_fails();
    setup(); receive();
    inject(stage.address + S64_OPEN_BACKUP_OFFSET, stage.address + S64_OPEN_STORAGE_SIZE, 0u);
    commit_fails();
    setup(); receive();
    inject(0u, S64_REU_DOCUMENT_CAPACITY, 1u);
    commit_fails();
    setup(); receive();
    inject(0u, S64_REU_DOCUMENT_CAPACITY, 1u);
    silent_corruption = 1u;
    commit_fails();
    setup();
    assert(s64_persistent_open_stage_write(response,
        (uint16_t)(sizeof(response) - 1u - 11u), &stage));
    memset(chunk, 'x', sizeof(chunk));
    assert(s64_persistent_open_stage_write(chunk, sizeof(chunk), &stage));
    assert(s64_persistent_open_stage_finish(&stage));
    inject(0u, S64_REU_DOCUMENT_CAPACITY, 1u);
    fail_after = 2u;
    commit_fails();
    puts("PASS backup failures; partial/corrupt replacement is rolled back and verified");

    setup(); receive();
    inject(0u, S64_REU_DOCUMENT_CAPACITY, 1u);
    fail_forever = 1u;
    assert(s64_persistent_open_stage_commit(&stage, &document) == S64_DOCUMENT_SINK_ERROR);
    assert(stage.recovery_required);
    assert(!memcmp(before, reu + stage.address + S64_OPEN_BACKUP_OFFSET,
        S64_REU_DOCUMENT_CAPACITY));
    assert(!memcmp(&saved_editor, &editor, sizeof(editor)));
    assert(!memcmp(&saved_document, &document, sizeof(document)));
    fail_write = fail_forever = 0u;
    assert(s64_persistent_open_stage_restore(&stage));
    preserved();
    puts("PASS failed rollback retains original backup and permits verified retry");

    setup();
    allocation_count = control.allocation_count; next_free = control.next_free;
    for (i = 0; i < 40u; ++i) {
        assert(s64_persistent_open_stage_init(&stage, &control));
        receive();
        assert(s64_persistent_open_stage_commit(&stage, &document) == S64_DOCUMENT_OK);
        assert(document.body == &editor.buffer && document.order == -7);
        assert(!strcmp(document.slug, "remote") && !strcmp(document.title, "Remote"));
        assert(s64_buffer_read(&editor.buffer, 0u, chunk, sizeof(chunk)) == 11u);
        assert(!memcmp(chunk, "REPLACEMENT", 11u));
        assert(!memcmp(before + S64_REU_HISTORY_PAYLOAD_BASE,
            reu + S64_REU_HISTORY_PAYLOAD_BASE,
            sizeof(before) - S64_REU_HISTORY_PAYLOAD_BASE));
        assert(s64_persistent_open_stage_commit(&stage, &document) != S64_DOCUMENT_OK);
        assert(!s64_persistent_open_stage_finish(&stage));
    }
    assert(control.allocation_count == allocation_count && control.next_free == next_free);
    /* The owner applies the existing replacement hook only after success. */
    s64_editor_document_loaded(&editor);
    assert(!s64_editor_undo(&editor));
    editor.dirty = 1u;
    puts("PASS successful replacement; unchanged history until owner reset; no allocation leak");

    setup();
    assert(s64_persistent_open_stage_write(response,
        (uint16_t)(sizeof(response) - 1u - 11u), &stage));
    memset(chunk, 'x', sizeof(chunk));
    for (i = 0; i < S64_REU_DOCUMENT_CAPACITY / sizeof(chunk); ++i)
        assert(s64_persistent_open_stage_write(chunk, sizeof(chunk), &stage));
    assert(s64_persistent_open_stage_finish(&stage));
    assert(s64_persistent_open_stage_commit(&stage, &document) == S64_DOCUMENT_OK);
    assert(s64_buffer_length(&editor.buffer) == S64_REU_DOCUMENT_CAPACITY);
    assert(s64_buffer_char_at(&editor.buffer, S64_REU_DOCUMENT_CAPACITY - 1u) == 'x');

    setup();
    assert(s64_persistent_open_stage_write(response,
        (uint16_t)(sizeof(response) - 1u - 11u), &stage));
    for (i = 0; i < S64_REU_DOCUMENT_CAPACITY / sizeof(chunk); ++i)
        assert(s64_persistent_open_stage_write(chunk, sizeof(chunk), &stage));
    assert(!s64_persistent_open_stage_write(chunk, 1u, &stage));
    commit_fails();
    setup();
    memset(chunk, 'x', sizeof(chunk));
    for (i = 0; i < S64_OPEN_BODY_OFFSET / sizeof(chunk); ++i)
        assert(s64_persistent_open_stage_write(chunk, sizeof(chunk), &stage));
    assert(!s64_persistent_open_stage_finish(&stage));
    assert(!s64_persistent_open_stage_write(chunk, sizeof(chunk), &stage));
    commit_fails();
    puts("PASS exact body capacity including bank crossing; over-capacity rejected");

    setup();
    s64_reu_runtime_init(&full, 514u, 123u);
    for (i = 0; i < S64_REU_RUNTIME_MAX_ALLOCATIONS; ++i)
        assert(s64_reu_runtime_allocate(&full, S64_REU_OWNER_MODULE, 256u, 256u, &unused));
    assert(!s64_persistent_open_stage_init(&rejected, &full));
    preserved();
    s64_reu_runtime_init(&full, 514u, 123u);
    assert(s64_reu_runtime_allocate(&full, S64_REU_OWNER_MODULE,
        S64_REU_RUNTIME_HEAP_END - S64_REU_RUNTIME_HEAP_START, 256u, &unused));
    assert(!s64_persistent_open_stage_init(&rejected, &full));
    preserved();
    s64_reu_runtime_init(&full, 514u, 123u);
    for (i = 0; i < S64_REU_RUNTIME_MAX_ALLOCATIONS - 1u; ++i)
        assert(s64_reu_runtime_allocate(&full, S64_REU_OWNER_MODULE, 256u, 256u, &unused));
    assert(s64_persistent_open_stage_init(&rejected, &full));
    assert(full.allocation_count == S64_REU_RUNTIME_MAX_ALLOCATIONS);
    assert(s64_persistent_open_stage_init(&rejected, &full));
    assert(full.allocation_count == S64_REU_RUNTIME_MAX_ALLOCATIONS);
    full.session_id = 0u;
    assert(!s64_persistent_open_stage_init(&rejected, &full));
    preserved();
    puts("PASS allocator exhaustion and invalid owner control");
    return 0;
}
