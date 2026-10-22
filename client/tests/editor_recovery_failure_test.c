#include <assert.h>
#include <stdio.h>
#define clock editor_test_clock
#define main s64_application_main
#include "../src/main.c"
#undef main

static unsigned deleted, written;
static clock_t test_now;
clock_t editor_test_clock(void) { return test_now; }
static S64LocalResult open_result;
S64LocalResult s64_local_recovery_exists(S64LocalDocument* local, uint8_t* exists)
{ (void)local; *exists = 1; return S64_LOCAL_OK; }
S64LocalResult s64_local_open_recovery(S64LocalDocument* local)
{ (void)local; return open_result; }
S64LocalResult s64_local_delete_recovery(S64LocalDocument* local)
{ (void)local; ++deleted; return S64_LOCAL_OK; }
S64LocalResult s64_local_save_recovery(S64LocalDocument* local)
{ (void)local; ++written; return S64_LOCAL_OK; }
unsigned char s64_ui_prompt_yes_no(const char* text) { (void)text; return 1; }
void s64_ui_message(const char* text) { (void)text; }
void s64_ui_render(const S64Editor* value) { (void)value; }
void s64_editor_document_loaded(S64Editor* value) { (void)value; }
const char* s64_local_result_message(const S64LocalDocument* local, S64LocalResult result)
{ (void)local; (void)result; return "Read failed"; }
int main(void)
{
    test_now = (clock_t)(100UL * CLOCKS_PER_SEC);
    last_input_time = test_now - AUTOSAVE_QUIET_INTERVAL;
    uci_capabilities.dos_target = 1;
    open_result = S64_LOCAL_IO_ERROR;
    offer_recovery();
    assert(deleted == 0);
    open_result = S64_LOCAL_INVALID_DOCUMENT;
    offer_recovery();
    assert(deleted == 0);
    editor.dirty = 1;
    last_recovery_write = clock() - AUTOSAVE_INTERVAL;
    maybe_autosave();
    assert(written == 0);
    clear_recovery_snapshot();
    last_input_time = test_now;
    test_now += CLOCKS_PER_SEC;
    maybe_autosave();
    assert(written == 0); /* A one-second pause between words is not idle. */
    test_now = last_input_time + AUTOSAVE_QUIET_INTERVAL - 1;
    maybe_autosave();
    assert(written == 0);
    ++test_now;
    maybe_autosave();
    assert(written == 1);
    test_now += AUTOSAVE_INTERVAL;
    last_input_time = test_now;
    maybe_autosave();
    assert(written == 1); /* Recent input postpones even an overdue snapshot. */
    test_now += AUTOSAVE_QUIET_INTERVAL;
    maybe_autosave();
    assert(written == 2);
    ++test_now;
    maybe_autosave();
    assert(written == 2); /* Preserve the regular 30-second write interval. */
    puts("Editor recovery preservation and quiet-period scheduling passed"); return 0;
}
