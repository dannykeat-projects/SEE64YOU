#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "storage/local.h"
#include "platform/session_journal.h"

#define FILES 8
#define BYTES 512
typedef struct { char path[80]; uint8_t bytes[BYTES]; uint16_t length; uint8_t exists; } File;
typedef struct { const char* cwd; int opened; uint16_t offset; uint8_t writing; } Context;
static File files[FILES];
static Context contexts[2];
static char last_temporary[80];
static uint8_t fail_folder_open;

static void check(int value, const char* message)
{
    if (!value) { fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}

static void status(S64UciReply* reply, const char* text)
{
    size_t n = strlen(text);
    reply->status_length = (uint16_t)n;
    if (reply->status && reply->status_capacity) {
        check(n < reply->status_capacity, "status capacity");
        memcpy(reply->status, text, n + 1);
    }
}

static void resolve(char* path, const Context* context, const S64UciCommandPart* part)
{
    size_t n = part->length;
    if (n && part->data[n - 1] == 0) --n;
    path[0] = 0;
    if (!n || part->data[0] != '/') strcpy(path, context->cwd);
    check(strlen(path) + n < 80, "path capacity");
    strncat(path, (const char*)part->data, n);
}

static int find(const char* path, uint8_t create)
{
    int i;
    for (i = 0; i < FILES; ++i) if (files[i].exists && !strcmp(files[i].path, path)) return i;
    if (!create) return -1;
    for (i = 0; i < FILES; ++i) if (!files[i].exists) {
        memset(&files[i], 0, sizeof(files[i]));
        strcpy(files[i].path, path); files[i].exists = 1; return i;
    }
    check(0, "file capacity"); return -1;
}

static S64UciResult execute(const S64UciCommandPart* parts, uint8_t count, S64UciReply* reply)
{
    uint8_t target = parts[0].data[0];
    uint8_t op = parts[0].data[1];
    Context* context;
    char path[80], destination[80];
    int slot;
    check(target == 1 || target == 2, "valid DOS target");
    context = &contexts[target - 1];
    if (op == 0x11) {
        if (contexts[0].opened >= 0 || contexts[1].opened >= 0) {
            status(reply, "87,ACCESS DENIED"); return S64_UCI_OK;
        }
        check(count == 2, "change directory shape");
        resolve(path, context, &parts[1]);
        if (!strcmp(path, "/sd/")) context->cwd = "/sd/";
        else if (!strcmp(path, "/sd/SEE64YOU/")) context->cwd = "/sd/SEE64YOU/";
        else { status(reply, "87,ACCESS DENIED"); return S64_UCI_OK; }
    } else if (op == 0x08 || op == 0x02 || op == 0x09 || op == 0x0a) {
        check(count >= 2, "filename part"); resolve(path, context, &parts[1]);
        slot = find(path, 0);
        if (op == 0x08) { status(reply, slot < 0 ? "88,FILE NOT FOUND" : "00,OK"); return S64_UCI_OK; }
        if (op == 0x09) { if (slot >= 0) files[slot].exists = 0; }
        else if (op == 0x0a) {
            check(count == 4 && slot >= 0, "rename existing temp");
            resolve(destination, context, &parts[3]);
            check(find(destination, 0) < 0, "rename does not overwrite existing file");
            strcpy(files[slot].path, destination);
        } else {
            context->writing = parts[0].data[2] != 1;
            if (!context->writing && fail_folder_open && !strcmp(path, "/sd/SEE64YOU/LEGACY.S64")) {
                status(reply, "87,ACCESS DENIED"); return S64_UCI_OK;
            }
            if (!context->writing && slot < 0) { status(reply, "88,FILE NOT FOUND"); return S64_UCI_OK; }
            if (context->writing) {
                if (slot < 0) slot = find(path, 1);
                files[slot].length = 0;
                if (strstr(path, ".tmp")) strcpy(last_temporary, path);
            }
            context->opened = slot; context->offset = 0;
        }
    } else if (op == 0x03) context->opened = -1;
    else if (op == 0x04) {
        File* file;
        uint16_t request, delivered;
        check(context->opened >= 0 && !context->writing, "read target handle");
        file = &files[context->opened];
        request = (uint16_t)(parts[0].data[2] | ((uint16_t)parts[0].data[3] << 8));
        delivered = (uint16_t)(file->length - context->offset);
        if (delivered > request) delivered = request;
        if (delivered && reply->data_sink && !reply->data_sink(file->bytes + context->offset,
            delivered, reply->data_context)) return S64_UCI_CONSUMER_ABORTED;
        context->offset = (uint16_t)(context->offset + delivered);
    } else if (op == 0x05) {
        File* file;
        check(context->opened >= 0 && context->writing && count == 2, "write target handle");
        file = &files[context->opened];
        check(file->length + parts[1].length <= BYTES, "write capacity");
        memcpy(file->bytes + file->length, parts[1].data, parts[1].length);
        file->length = (uint16_t)(file->length + parts[1].length);
    } else return S64_UCI_PROTOCOL_ERROR;
    status(reply, "00,OK"); return S64_UCI_OK;
}

static void seed(const char* path, const char* body)
{
    static const char header[] = "SEE64YOU/1\nType: post\nSlug: legacy\nTitle: Legacy\nStatus: draft\nDate: \nOrder: 0\n\n";
    int slot = find(path, 1);
    strcpy((char*)files[slot].bytes, header);
    strcat((char*)files[slot].bytes, body);
    files[slot].length = (uint16_t)strlen((char*)files[slot].bytes);
}

static void body_is(const S64Buffer* body, const char* expected)
{
    uint16_t i;
    check(s64_buffer_length(body) == strlen(expected), "exact body length");
    for (i = 0; i < strlen(expected); ++i) check(s64_buffer_char_at(body, i) == (uint8_t)expected[i], "exact body bytes");
}

int main(void)
{
    S64Dos documents, journal_dos;
    S64Buffer body;
    S64Document document;
    S64LocalDocument local;
    S64SessionJournal journal;
    S64SystemSnapshot snapshot;
    File root_before, folder_before;
    int root_slot, folder_slot, secondary_handle;
    memset(contexts, 0, sizeof(contexts));
    contexts[0].cwd = contexts[1].cwd = "/sd/";
    contexts[0].opened = contexts[1].opened = -1;
    seed("/sd/LEGACY.S64", "Root source"); seed("/sd/SEE64YOU/LEGACY.S64", "Folder source");
    root_slot = find("/sd/LEGACY.S64", 0); folder_slot = find("/sd/SEE64YOU/LEGACY.S64", 0);
    root_before = files[root_slot]; folder_before = files[folder_slot];
    s64_dos_init(&documents, execute, S64_DOS_TARGET_SECONDARY);
    s64_dos_init(&journal_dos, execute, S64_DOS_TARGET_PRIMARY);
    check(s64_dos_change_dir(&documents, "/sd/SEE64YOU/") == S64_DOS_OK, "document folder selected");
    s64_buffer_init(&body); s64_document_init(&document, &body); s64_local_init(&local, &documents, &document);
    check(s64_local_open(&local, "/sd/LEGACY.S64", 1) == S64_LOCAL_OK, "explicit legacy root opens");
    check(!strcmp(local.filename, "/sd/LEGACY.S64"), "absolute identity retained"); body_is(&body, "Root source");

    check(s64_dos_open_read(&documents, "LEGACY.S64") == S64_DOS_OK, "secondary handle opened");
    secondary_handle = contexts[1].opened;
    memset(&snapshot, 0, sizeof(snapshot));
    check(s64_session_journal_prepare(&journal, 12, 1, 0, &snapshot), "prepare journal");
    check(s64_session_journal_store_save(&journal_dos, &journal) == S64_SESSION_JOURNAL_IO_ERROR,
        "simulated busy firmware refuses journal context changes while a document handle is open");
    check(contexts[1].opened == secondary_handle && !strcmp(contexts[1].cwd, "/sd/SEE64YOU/"),
        "journal context changes preserve secondary directory and open handle");
    check(s64_dos_close(&documents) == S64_DOS_OK, "secondary handle closes independently");
    check(s64_session_journal_store_save(&journal_dos, &journal) == S64_SESSION_JOURNAL_OK,
        "journal saves after the synchronous document operation closes");
    check(s64_dos_change_dir(&journal_dos, "/sd/") == S64_DOS_OK, "primary legacy context selected");
    check(!strcmp(contexts[1].cwd, "/sd/SEE64YOU/"), "closed secondary directory remains isolated");
    check(s64_buffer_move_cursor(&body, s64_buffer_length(&body)), "root end cursor");
    check(s64_buffer_insert(&body, '!'), "root edit");
    check(s64_local_save(&local) == S64_LOCAL_OK, "root origin save");
    check(!strcmp(last_temporary, "/sd/LEGACY.S64.tmp"), "root temporary keeps original path");
    check(!memcmp(&folder_before, &files[folder_slot], sizeof(File)), "same-name folder collision unchanged");
    check(s64_local_open(&local, "/sd/LEGACY.S64", 1) == S64_LOCAL_OK, "reopen saved root"); body_is(&body, "Root source!");
    root_before = files[find("/sd/LEGACY.S64", 0)];
    check(s64_local_open(&local, "LEGACY.S64", 1) == S64_LOCAL_OK, "bare name opens folder"); body_is(&body, "Folder source");
    check(s64_buffer_move_cursor(&body, s64_buffer_length(&body)), "folder end cursor");
    check(s64_buffer_insert(&body, '!'), "folder edit");
    check(s64_local_save(&local) == S64_LOCAL_OK, "folder save");
    check(!strcmp(last_temporary, "/sd/SEE64YOU/LEGACY.S64.tmp"), "folder temporary contained");
    check(!memcmp(&root_before, &files[find("/sd/LEGACY.S64", 0)], sizeof(File)), "folder save leaves root byte exact");
    check(s64_local_open(&local, "LEGACY.S64", 1) == S64_LOCAL_OK, "reopen folder"); body_is(&body, "Folder source!");
    fail_folder_open = 1;
    check(s64_local_open(&local, "LEGACY.S64", 1) == S64_LOCAL_IO_ERROR, "folder I/O fails explicitly");
    body_is(&body, "Folder source!"); fail_folder_open = 0;
    folder_slot = find("/sd/SEE64YOU/LEGACY.S64", 0);
    files[folder_slot].bytes[0] = 'X';
    check(s64_local_open(&local, "LEGACY.S64", 1) == S64_LOCAL_INVALID_DOCUMENT, "corrupt folder never falls back to root");
    check(!s64_local_has_filename(&local), "corrupt document does not retain root identity");
    check(!memcmp(&root_before, &files[find("/sd/LEGACY.S64", 0)], sizeof(File)), "failure leaves legacy root unchanged");
#ifdef S64_CODE_ASSET_LAUNCHER
    seed("/sd/S64JRN0.DAT", "Legacy journal must block even when malformed");
    check(s64_session_journal_store_load(&journal_dos, &journal) == S64_SESSION_JOURNAL_INVALID,
        "Launcher refuses every legacy root journal before folder ownership");
    check(find("/sd/S64JRN0.DAT", 0) >= 0, "legacy slot retained after refusal");
#endif
    puts("storage context tests passed"); return 0;
}
