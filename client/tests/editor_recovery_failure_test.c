#include <assert.h>
#include <stdio.h>
#define main s64_application_main
#include "../src/main.c"
#undef main

static unsigned deleted, written;
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
    last_recovery_write = clock() - AUTOSAVE_INTERVAL;
    maybe_autosave();
    assert(written == 1);
    puts("Editor failed recovery preserved"); return 0;
}
