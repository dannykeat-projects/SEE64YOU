#include <assert.h>
#include <stdio.h>
#define S64_INDEPENDENT_EDITOR 1
#define clock editor_test_clock
#define main s64_application_main
#include "../src/main.c"
#undef main

static unsigned deleted, written;
static unsigned confirmed;
static unsigned char answer = 1u;
static unsigned char exit_choice;
static uint8_t has_filename = 1u;
static S64LocalResult save_result = S64_LOCAL_OK;
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
unsigned char s64_ui_prompt_yes_no(const char* text)
{ (void)text; ++confirmed; return answer; }
unsigned char s64_ui_prompt_exit(void) { return exit_choice; }
unsigned char s64_ui_prompt_text(const char* label, char* value, unsigned char capacity)
{ (void)label; (void)value; (void)capacity; return 0u; }
uint8_t s64_local_has_filename(const S64LocalDocument* local)
{ (void)local; return has_filename; }
S64LocalResult s64_local_save(S64LocalDocument* local)
{ (void)local; return save_result; }
S64LocalResult s64_local_save_as(S64LocalDocument* local, const char* name, uint8_t overwrite)
{ (void)local; (void)name; (void)overwrite; return save_result; }
void s64_editor_mark_saved(S64Editor* value) { value->dirty = 0u; }
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
    editor.dirty = 0u;
    confirmed = 0u;
    answer = 0u;
    assert(!confirm_exit());
    assert(confirmed == 1u); /* A saved document must still ask before exit. */
    answer = 1u;
    assert(confirm_exit());
    assert(confirmed == 2u);
    editor.dirty = 1u;
    exit_choice = CH_STOP;
    assert(!confirm_exit() && editor.dirty);
    assert(confirmed == 2u); /* Dirty documents retain Save/Discard/Cancel. */
    exit_choice = 's';
    save_result = S64_LOCAL_IO_ERROR;
    assert(!confirm_exit() && editor.dirty);
    has_filename = 0u;
    assert(!confirm_exit() && editor.dirty); /* Cancelled Save As cannot exit. */
    has_filename = 1u;
    save_result = S64_LOCAL_OK;
    assert(confirm_exit() && !editor.dirty);
    editor.dirty = 1u;
    exit_choice = 'd';
    assert(confirm_exit());
    puts("Editor recovery, autosave timing and exit confirmation passed"); return 0;
}
