#include <assert.h>
#include <stdio.h>
#define S64_INDEPENDENT_EDITOR 1
#define main s64_application_main
#include "../src/main.c"
#undef main

static unsigned deleted;
static unsigned confirmed;
static unsigned char answer = 1u;
static unsigned char exit_choice;
static uint8_t has_filename = 1u;
static S64LocalResult save_result = S64_LOCAL_OK;
static S64LocalResult open_result;
S64LocalResult s64_local_recovery_exists(S64LocalDocument* local, uint8_t* exists)
{ (void)local; *exists = 1; return S64_LOCAL_OK; }
S64LocalResult s64_local_open_recovery(S64LocalDocument* local)
{ (void)local; return open_result; }
S64LocalResult s64_local_delete_recovery(S64LocalDocument* local)
{ (void)local; ++deleted; return S64_LOCAL_OK; }
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
    uci_capabilities.dos_target = 1;
    open_result = S64_LOCAL_IO_ERROR;
    offer_recovery();
    assert(deleted == 0);
    open_result = S64_LOCAL_INVALID_DOCUMENT;
    offer_recovery();
    assert(deleted == 0);
    open_result = S64_LOCAL_OK;
    offer_recovery();
    assert(editor.dirty && deleted == 0); /* Existing recovery stays until saved/discarded. */
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
    puts("Editor recovery preservation and exit confirmation passed"); return 0;
}
