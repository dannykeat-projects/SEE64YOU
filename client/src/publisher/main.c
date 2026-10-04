#include <conio.h>
#include <string.h>

#include "buffer.h"
#include "charset.h"
#include "document.h"
#include "platform/http.h"
#ifndef S64_MANAGED_OVERLAY
#include "platform/chain.h"
#endif
#include "platform/dos.h"
#include "platform/reu.h"
#include "platform/uci.h"
#include "publisher/destination.h"
#include "publisher/handoff.h"
#include "publisher/plaintext.h"
#include "publisher/preview.h"
#include "publisher/relay.h"
#include "publisher/state.h"
#include "publisher/standalone.h"
#include "publisher/standalone_documents.h"
#include "publisher/settings_bridge.h"
#ifndef S64_STANDALONE_ONLY
#include "publisher/wordpress.h"
#include "publisher/wordpress_documents.h"
#endif
#include "publisher/workspace.h"
#ifdef S64_DIRECT_PUBLISHER
#include "publisher/standalone_direct.h"
#endif

#ifdef S64_MANAGED_OVERLAY
#include "platform/module_arrival.h"
#include "platform/code_overlay_bootstrap.h"
#include "platform/managed_publisher.h"
#include "platform/package_version.h"
static S64ModuleRuntime managed_runtime;
static void managed_entry_failed(uint8_t stage)
{
    clrscr();
    cputs("Managed entry failed. Restart launcher.");
    cputc((char)('0' + stage));
    for (;;) { }
}
#endif

#define SCREEN_COLUMNS 40u
#define STATUS_ROW 24u
#define PROMPT_CAPACITY S64_DESTINATION_BASE_URL_CAPACITY
#define USERNAME_CAPACITY 61u

typedef struct {
    S64StandaloneSession standalone;
#ifndef S64_STANDALONE_ONLY
    S64WordPressSession wordpress;
#endif
} S64PublisherSession;

static S64Buffer body;
static S64Document document;
static S64HttpTransport http;
static S64Dos dos;
static S64UciCapabilities capabilities;
static S64PublishDestination destination;
static S64PublishState publish_state;
#ifndef S64_DIRECT_PUBLISHER
static S64PublisherSession session;
#define STANDALONE_SESSION (&session.standalone)
#else
/* Direct authentication signs each request; there is no reusable login token. */
#define STANDALONE_SESSION ((const S64StandaloneSession*)0)
#endif
static unsigned char document_changed;
#ifdef S64_DIRECT_PUBLISHER
static unsigned char network_available;
#endif

#define WORK_STATUS ((char*)&s64_publisher_workspace.bytes[0])
#define WORK_URL ((char*)&s64_publisher_workspace.bytes[0])
#define WORK_PROMPT ((char*)&s64_publisher_workspace.bytes[0])
#define WORK_USERNAME ((char*)&s64_publisher_workspace.bytes[S64_PUBLISH_WORK_MAIN_USERNAME_OFFSET])
#define WORK_SECRET ((char*)&s64_publisher_workspace.bytes[S64_PUBLISH_WORK_MAIN_USERNAME_OFFSET])
#define WORK_REMOTE_ID ((char*)&s64_publisher_workspace.bytes[S64_PUBLISH_WORK_MAIN_USERNAME_OFFSET])
#define WORK_HANDOFF ((S64PublisherHandoff*)&s64_publisher_workspace.bytes[S64_PUBLISH_WORK_HANDOFF_OFFSET])
#define WORK_SETTINGS ((S64SettingsExchange*)&s64_publisher_workspace.bytes[0])
#define WORK_SELECTION ((char*)&s64_publisher_workspace.bytes[S64_PUBLISH_WORK_SELECTION_OFFSET])

#ifndef S64_DIRECT_PUBLISHER
static unsigned char selected_state;
#endif
static unsigned char document_replaced;


static void clear_row(unsigned char row)
{
    unsigned char i;

    gotoxy(0u, row);
    for (i = 0u; i < SCREEN_COLUMNS; ++i) {
        cputc(' ');
    }
}

static void set_status(const char* text)
{
    unsigned char i = 0u;

    while (i < SCREEN_COLUMNS && text[i] != '\0') {
        WORK_STATUS[i] = text[i];
        ++i;
    }
    WORK_STATUS[i] = '\0';
}

static void print_value(unsigned char row, const char* label, const char* value)
{
    unsigned char col = 0u;

    clear_row(row);
    gotoxy(0u, row);
    while (col < SCREEN_COLUMNS && *label != '\0') {
        cputc((unsigned char)*label++);
        ++col;
    }
    while (col < SCREEN_COLUMNS && *value != '\0') {
        cputc((unsigned char)*value++);
        ++col;
    }
}

static const char* destination_name(void)
{
    if (destination.type == S64_DESTINATION_STANDALONE) {
        return "Standalone";
    }
    if (destination.type == S64_DESTINATION_WORDPRESS) {
        return "WordPress";
    }
    return "Not configured";
}

static void render(void)
{
    clrscr();
    revers(1);
    gotoxy(0u, 0u);
    cputs(" SEE64YOU PUBLISHER                    ");
    revers(0);

    print_value(2u, "Title: ", document.title[0] != '\0'
        ? document.title : "(untitled)");
    print_value(3u, "Slug:  ", document.slug[0] != '\0'
        ? document.slug : "(unset)");
    print_value(4u, "Type:  ", s64_document_type_name(document.type));
    print_value(5u, "State: ", s64_document_status_name(document.status));
    print_value(7u, "Destination: ", destination_name());
#ifdef S64_DIRECT_PUBLISHER
    print_value(8u, "Direct HTTP: ", network_available ? "unencrypted" : "unavailable");
#else
    print_value(8u, "HTTP: ", capabilities.http_target
        ? "available" : "unavailable");
#endif
    print_value(9u, "Connection: ", publish_state.connected
        ? "ready" : "not connected");
    print_value(10u, "Remote: ", publish_state.remote_bound
        ? publish_state.remote_identity : "(new document)");

    gotoxy(0u, 12u);
    cputs("1  PUBLISHING SETTINGS");
    gotoxy(0u, 13u);
#ifdef S64_DIRECT_PUBLISHER
    cputs("2  TEST DIRECT KEY");
#else
    cputs("2  TEST / LOGIN");
#endif
    gotoxy(0u, 14u);
    cputs("3  SAVE / UPDATE REMOTE");
    gotoxy(0u, 15u);
    cputs("4  TOGGLE DRAFT / PUBLISHED");
    gotoxy(0u, 16u);
    cputs("5  DELETE REMOTE");
    gotoxy(0u, 17u);
#ifdef S64_DIRECT_PUBLISHER
    cputs("6 OPEN PAGE / 7 POST BY SLUG");
#else
    cputs("6 OPEN PAGE  7 OPEN POST");
#endif
    gotoxy(0u, 18u);
    cputs("8 PREVIEW");
    gotoxy(0u, 19u);
    cputs("9 EXPORT TXT  0 IMPORT TXT");
#ifdef S64_DIRECT_PUBLISHER
    gotoxy(0u, 20u);
    cputs("T TOGGLE PAGE / POST");
#endif
    gotoxy(0u, 21u);
    cputs("RUN/STOP  RETURN TO EDITOR");

    clear_row(STATUS_ROW);
    gotoxy(0u, STATUS_ROW);
    revers(1);
    cputs(WORK_STATUS);
    revers(0);
}

static void render_prompt(
    const char* label,
    const char* value,
    unsigned char length,
    unsigned char masked
)
{
    unsigned char label_length = (unsigned char)strlen(label);
    unsigned char visible = (unsigned char)(SCREEN_COLUMNS - label_length - 1u);
    unsigned char start = length > visible
        ? (unsigned char)(length - visible)
        : 0u;
    unsigned char shown = 0u;

    clear_row(STATUS_ROW);
    revers(1);
    gotoxy(0u, STATUS_ROW);
    cputs(label);

    while (shown < visible && (unsigned char)(start + shown) < length) {
        cputc(masked ? '*' : (unsigned char)value[start + shown]);
        ++shown;
    }
    while (shown < visible) {
        cputc(' ');
        ++shown;
    }
    gotoxy(
        (unsigned char)(label_length + (length - start)),
        STATUS_ROW
    );
}

static unsigned char prompt_printable(unsigned char key)
{
    uint8_t ascii;

    return s64_charset_native_to_ascii(key, &ascii)
        && ascii >= 0x20u
        && ascii <= 0x7eu;
}

static unsigned char prompt_text(
    const char* label,
    char* value,
    unsigned char capacity,
    unsigned char masked
)
{
    unsigned char key;
    unsigned char length = 0u;

    if (capacity == 0u || strlen(label) >= SCREEN_COLUMNS) {
        return 0u;
    }

    value[0] = '\0';
    render_prompt(label, value, length, masked);

    for (;;) {
        key = (unsigned char)cgetc();

        if (key == CH_STOP) {
            value[0] = '\0';
            revers(0);
            return 0u;
        }
        if (key == CH_ENTER) {
            value[length] = '\0';
            revers(0);
            return 1u;
        }
        if (key == CH_DEL) {
            if (length != 0u) {
                --length;
                value[length] = '\0';
                render_prompt(label, value, length, masked);
            }
            continue;
        }
        if (prompt_printable(key)
            && length + 1u < capacity) {
            value[length++] = (char)key;
            value[length] = '\0';
            render_prompt(label, value, length, masked);
        }
    }
}

static unsigned char prompt_yes_no(const char* label)
{
    unsigned char key;

    clear_row(STATUS_ROW);
    revers(1);
    gotoxy(0u, STATUS_ROW);
    cputs(label);

    for (;;) {
        key = (unsigned char)cgetc();
        if (key == 'y' || key == 'Y') {
            revers(0);
            return 1u;
        }
        if (key == 'n' || key == 'N' || key == CH_STOP) {
            revers(0);
            return 0u;
        }
    }
}

static void clear_session(void)
{
#ifndef S64_DIRECT_PUBLISHER
    memset(&session, 0, sizeof(session));
#endif
    s64_publish_state_set_connected(&publish_state, 0u);
}

#ifndef S64_MANAGED_OVERLAY
static void sync_handoff_document(S64PublisherHandoff* target)
{
    target->buffer = body;
    target->document_type = document.type;
    target->document_status = document.status;
    target->order = document.order;
    memcpy(target->slug, document.slug, sizeof(target->slug));
    memcpy(target->title, document.title, sizeof(target->title));
    memcpy(target->date, document.date, sizeof(target->date));
}
#endif

static void mark_document_changed(void)
{
    document_changed = 1u;
}

#ifndef S64_MANAGED_OVERLAY
static unsigned char preserve_document_handoff(unsigned char returning)
{
    S64PublisherHandoff* target = WORK_HANDOFF;

    if (!s64_handoff_load(target)) {
        set_status("Cannot reload editor handoff");
        return 0u;
    }

    sync_handoff_document(target);
    if (document_replaced) {
        memset(&target->history, 0, sizeof(target->history));
        target->filename[0] = '\0';
    }
    if (document_changed) target->flags |= S64_HANDOFF_FLAG_DIRTY;
    if (returning) {
        target->flags |= S64_HANDOFF_FLAG_RETURN_TO_EDITOR;
    } else {
        target->flags &= (uint8_t)~S64_HANDOFF_FLAG_RETURN_TO_EDITOR;
    }
    if (!s64_handoff_store(target)) {
        set_status("Cannot preserve editor handoff");
        return 0u;
    }
    return 1u;
}

#endif
static void return_to_editor(void)
{
#ifdef S64_MANAGED_OVERLAY
    set_status("Editor transition not ready");
#else
    if (!preserve_document_handoff(1u)) return;
    clrscr();
    cputs("RETURNING TO SEE64YOU EDITOR...\r\n");
    s64_chain_editor();
#endif
}

static void open_settings(void)
{
#ifdef S64_MANAGED_OVERLAY
    set_status("Settings transition not ready");
#else
    if (!preserve_document_handoff(0u)) return;
    if (!s64_settings_bridge_request_edit()) {
        set_status("Cannot open publishing settings");
        return;
    }

    clrscr();
    cputs("OPENING PUBLISHING SETTINGS...\r\n");
    s64_chain_settings();
#endif
}

#ifndef S64_MANAGED_OVERLAY
static const char* settings_status(uint8_t result)
{
    if (result == S64_SETTINGS_RESULT_OK) return "Settings loaded";
    if (result == S64_SETTINGS_RESULT_NOT_FOUND) return "Publishing not configured";
    if (result == S64_SETTINGS_RESULT_INVALID_FILE) return "Publisher settings invalid";
    return "Settings storage unavailable";
}

static unsigned char restore_settings(void)
{
    uint8_t result;

    if (!s64_settings_bridge_load_ready(WORK_SETTINGS)) {
        if (!s64_settings_bridge_request_load(WORK_SETTINGS)) {
            set_status("Settings handoff unavailable");
            return 0u;
        }
        clrscr();
        cputs("LOADING PUBLISHING SETTINGS...\r\n");
        s64_chain_settings();
        set_status("Settings helper unavailable");
        return 0u;
    }

    result = WORK_SETTINGS->result;
    s64_destination_clear(&destination);
    if (WORK_SETTINGS->destination_type != (uint8_t)S64_DESTINATION_NONE) {
        destination.type = (S64DestinationType)WORK_SETTINGS->destination_type;
        destination.allow_insecure_http = WORK_SETTINGS->allow_insecure_http;
        memcpy(destination.base_url, WORK_SETTINGS->base_url, sizeof(destination.base_url));
    }
#ifdef S64_DIRECT_PUBLISHER
    (void)s64_direct_publisher_key(WORK_SETTINGS->direct_key);
#else
    if (!s64_relay_set_key(WORK_SETTINGS->relay_key)) {
        s64_destination_clear(&destination);
        (void)s64_relay_set_key("");
        set_status("Publisher settings invalid");
        return 0u;
    }
#endif

    set_status(settings_status(result));
    return 1u;
}
#endif

static unsigned char publishing_ready(void)
{
#ifdef S64_DIRECT_PUBLISHER
    if (!network_available) {
        set_status("C64U network target unavailable");
#else
    if (!capabilities.http_target) {
        set_status("C64U HTTP target unavailable");
#endif
        return 0u;
    }
    if (destination.type == S64_DESTINATION_NONE) {
        set_status("Set destination first");
        return 0u;
    }
    if (!publish_state.connected) {
        set_status("Test / login first");
        return 0u;
    }
    return 1u;
}

static void toggle_document_status(void)
{
    document.status = document.status == S64_DOCUMENT_PUBLISHED
        ? S64_DOCUMENT_DRAFT
        : S64_DOCUMENT_PUBLISHED;
    mark_document_changed();

    set_status(document.status == S64_DOCUMENT_PUBLISHED
        ? "Status set to published; save to apply"
        : "Status set to draft; save to unpublish");
}

static void save_remote_document(void)
{
    S64DestinationResult url_result;
    unsigned char updating;

    if (!publishing_ready()) {
        return;
    }
    if (!s64_document_slug_valid(document.slug)
        || document.title[0] == '\0') {
        set_status("Document needs valid title and slug");
        return;
    }

    updating = (unsigned char)s64_publish_state_can_update(
        &publish_state,
        &destination,
        &document
    );

    url_result = s64_destination_save_url(
        &destination,
        updating ? publish_state.remote_identity : 0,
        updating,
        WORK_URL,
        (uint16_t)S64_DESTINATION_URL_CAPACITY
    );
    if (url_result != S64_DESTINATION_OK) {
        set_status(s64_destination_result_message(url_result));
        return;
    }

    if (destination.type == S64_DESTINATION_STANDALONE) {
        S64StandaloneResult result;

        result = s64_standalone_save_document(
            &http,
            WORK_URL,
            destination.allow_insecure_http,
            STANDALONE_SESSION,
            &document
        );
        if (result != S64_STANDALONE_OK) {
            set_status(s64_standalone_result_message(result));
            return;
        }
        if (!s64_publish_state_bind_remote(
                &publish_state,
                &destination,
                document.type,
                document.slug
            )) {
            set_status("Saved, but remote binding failed");
            return;
        }
#ifndef S64_STANDALONE_ONLY
    } else if (destination.type == S64_DESTINATION_WORDPRESS) {
        S64WordPressResult result;

        result = s64_wordpress_save_document(
            &http,
            WORK_URL,
            destination.allow_insecure_http,
            &session.wordpress,
            &document,
            updating,
            WORK_REMOTE_ID,
            (uint16_t)S64_WORDPRESS_ID_CAPACITY
        );
        if (result != S64_WORDPRESS_OK) {
            set_status(s64_wordpress_result_message(result));
            return;
        }
        if (!s64_publish_state_bind_remote(
                &publish_state,
                &destination,
                document.type,
                WORK_REMOTE_ID
            )) {
            set_status("Saved, but remote binding failed");
            return;
        }
#endif
    } else {
#ifdef S64_STANDALONE_ONLY
        set_status("WordPress deferred");
#else
        set_status("Invalid destination");
#endif
        return;
    }

    set_status(document.status == S64_DOCUMENT_PUBLISHED
        ? (updating ? "Published document updated" : "Document published")
        : (updating ? "Remote draft updated" : "Remote draft created"));
}

static void delete_remote_document(void)
{
    S64DestinationResult url_result;

    if (!publishing_ready()) {
        return;
    }
    if (!s64_publish_state_can_delete(&publish_state, &destination)) {
        set_status("No bound remote document");
        return;
    }
    if (!prompt_yes_no("DELETE REMOTE DOCUMENT? Y/N")) {
        set_status("Delete cancelled");
        return;
    }

    url_result = s64_destination_delete_url(
        &destination,
        publish_state.remote_type,
        publish_state.remote_identity,
        WORK_URL,
        (uint16_t)S64_DESTINATION_URL_CAPACITY
    );
    if (url_result != S64_DESTINATION_OK) {
        set_status(s64_destination_result_message(url_result));
        return;
    }

    if (destination.type == S64_DESTINATION_STANDALONE) {
        S64StandaloneResult result;

        result = s64_standalone_delete_document(
            &http,
            WORK_URL,
            destination.allow_insecure_http,
            STANDALONE_SESSION,
            publish_state.remote_type,
            publish_state.remote_identity
        );
        if (result != S64_STANDALONE_OK) {
            set_status(s64_standalone_result_message(result));
            return;
        }
#ifndef S64_STANDALONE_ONLY
    } else if (destination.type == S64_DESTINATION_WORDPRESS) {
        S64WordPressResult result;

        result = s64_wordpress_delete_document(
            &http,
            WORK_URL,
            destination.allow_insecure_http,
            &session.wordpress
        );
        if (result != S64_WORDPRESS_OK) {
            set_status(s64_wordpress_result_message(result));
            return;
        }
#endif
    } else {
#ifdef S64_STANDALONE_ONLY
        set_status("WordPress deferred");
#else
        set_status("Invalid destination");
#endif
        return;
    }

    s64_publish_state_clear_remote(&publish_state);
    if (document.status == S64_DOCUMENT_PUBLISHED) {
        document.status = S64_DOCUMENT_DRAFT;
        mark_document_changed();
        set_status("Remote deleted; local status is draft");
    } else {
        set_status("Remote document deleted");
    }
}

#ifndef S64_DIRECT_PUBLISHER
static unsigned char choose_remote(
    const char* identity,
    const char* title
)
{
    unsigned char key;

    clrscr();
    print_value(2u, "Title: ", title);
    print_value(3u, "Remote: ", identity);
    gotoxy(0u, 5u);
    cputs("O=OPEN  OTHER=NEXT  STOP=CANCEL");

    key = (unsigned char)cgetc();
    if (key == CH_STOP) {
        selected_state = 2u;
        return 0u;
    }
    if (key == 'o' || key == 'O' || key == CH_ENTER) {
        strcpy(WORK_SELECTION, identity);
        selected_state = 1u;
        return 0u;
    }
    return 1u;
}

static uint8_t choose_standalone_summary(
    const S64DocumentSummary* summary,
    uint16_t index,
    void* context
)
{
    (void)index;
    (void)context;
    return choose_remote(summary->slug, summary->title);
}

#ifndef S64_STANDALONE_ONLY
static uint8_t choose_wordpress_summary(
    const S64WordPressDocumentSummary* summary,
    uint16_t index,
    void* context
)
{
    (void)index;
    (void)context;
    return choose_remote(summary->id, summary->title);
}

#endif

#endif

static void list_open_remote(S64DocumentType type)
{
    S64DestinationResult url_result;

    if (!publishing_ready()) {
        return;
    }

#ifdef S64_DIRECT_PUBLISHER
    WORK_SELECTION[0] = '\0';
    if (!prompt_text("REMOTE SLUG: ", WORK_SELECTION,
            (unsigned char)S64_DOCUMENT_SLUG_CAPACITY, 0u)
        || !s64_document_slug_valid(WORK_SELECTION)) {
        set_status("Invalid slug or cancelled");
        return;
    }
#else
    url_result = s64_destination_list_url(
        &destination,
        type,
        WORK_URL,
        (uint16_t)S64_DESTINATION_URL_CAPACITY
    );
    if (url_result != S64_DESTINATION_OK) {
        set_status(s64_destination_result_message(url_result));
        return;
    }

    selected_state = 0u;
    if (destination.type == S64_DESTINATION_STANDALONE) {
        S64StandaloneResult result = s64_standalone_list_documents(
            &http,
            WORK_URL,
            destination.allow_insecure_http,
            STANDALONE_SESSION,
            choose_standalone_summary,
            0,
            0,
            0
        );
        if (result != S64_STANDALONE_OK) {
            set_status(s64_standalone_result_message(result));
            return;
        }
    } else {
#ifndef S64_STANDALONE_ONLY
        S64WordPressResult result = s64_wordpress_list_documents(
            &http,
            WORK_URL,
            destination.allow_insecure_http,
            &session.wordpress,
            choose_wordpress_summary,
            0,
            0,
            0
        );
        if (result != S64_WORDPRESS_OK) {
            set_status(s64_wordpress_result_message(result));
            return;
        }
#else
        set_status("WordPress deferred");
        return;
#endif
    }

    if (selected_state == 2u) {
        set_status("Remote open cancelled");
        return;
    }
    if (selected_state == 0u) {
        set_status("No document selected");
        return;
    }
#endif
    if (!prompt_yes_no("REPLACE CURRENT DOCUMENT? Y/N")) {
        set_status("Remote open cancelled");
        return;
    }

    url_result = s64_destination_open_url(
        &destination,
        type,
        WORK_SELECTION,
        WORK_URL,
        (uint16_t)S64_DESTINATION_URL_CAPACITY
    );
    if (url_result != S64_DESTINATION_OK) {
        set_status(s64_destination_result_message(url_result));
        return;
    }

    document_replaced = 1u;
    document_changed = 1u;

    if (destination.type == S64_DESTINATION_STANDALONE) {
        S64StandaloneResult result = s64_standalone_open_document(
            &http,
            WORK_URL,
            destination.allow_insecure_http,
            STANDALONE_SESSION,
            &document
        );
        if (result != S64_STANDALONE_OK) {
            set_status(s64_standalone_result_message(result));
            return;
        }
    } else {
#ifndef S64_STANDALONE_ONLY
        S64WordPressResult result = s64_wordpress_open_document(
            &http,
            WORK_URL,
            destination.allow_insecure_http,
            &session.wordpress,
            &document
        );
        if (result != S64_WORDPRESS_OK) {
            set_status(s64_wordpress_result_message(result));
            return;
        }
#else
        set_status("WordPress deferred");
        return;
#endif
    }

    if (!s64_publish_state_bind_remote(
            &publish_state,
            &destination,
            document.type,
            WORK_SELECTION
        )) {
        set_status("Remote binding failed");
        return;
    }

    set_status("Remote document opened");
}

static void preview_emit(
    uint8_t row,
    uint8_t column,
    uint8_t value,
    uint8_t style,
    void* context
)
{
    (void)context;
    gotoxy(column, (unsigned char)(row + 2u));
    revers((style & S64_PREVIEW_STYLE_REVERSE) != 0u);
    cputc(value);
}

static void preview_document(void)
{
    S64PreviewState start;
    S64PreviewState next;
    uint8_t at_end;
    unsigned char key;
    unsigned int page = 1u;

    s64_preview_init(&start);

    for (;;) {
        clrscr();
        revers(1);
        gotoxy(0u, 0u);
        cputs(" SEE64YOU PREVIEW                      ");
        revers(0);
        print_value(1u, "", document.title[0] != '\0'
            ? document.title : "(untitled)");

        next = start;
        if (!s64_preview_render_page(
                &body,
                &next,
                preview_emit,
                0,
                &at_end
            )) {
            set_status("Preview failed");
            revers(0);
            return;
        }
        revers(0);

        clear_row(STATUS_ROW);
        gotoxy(0u, STATUS_ROW);
        revers(1);
        cputs(at_end
            ? "R RESTART  STOP BACK"
            : "SPACE/N NEXT  R RESTART  STOP BACK");
        revers(0);

        key = (unsigned char)cgetc();
        if (key == CH_STOP) {
            set_status("Preview closed");
            return;
        }
        if (key == 'r' || key == 'R' || key == CH_HOME) {
            s64_preview_init(&start);
            page = 1u;
            continue;
        }
        if (!at_end && (key == 'n' || key == 'N' || key == ' ')) {
            start = next;
            ++page;
            continue;
        }
    }
}

static void export_plain_text(void)
{
    S64PlainTextResult result;

    if (!capabilities.dos_target) {
        set_status("C64U DOS target unavailable");
        return;
    }
    if (!prompt_text(
            "EXPORT FILE: ",
            WORK_PROMPT,
            S64_PLAINTEXT_FILENAME_CAPACITY,
            0u
        )) {
        set_status("Export cancelled");
        return;
    }

    result = s64_plaintext_export(
        &dos,
        &body,
        WORK_PROMPT,
        0u
    );
    if (result == S64_PLAINTEXT_EXISTS) {
        if (!prompt_yes_no("OVERWRITE TEXT FILE? Y/N")) {
            set_status("Export cancelled");
            return;
        }
        result = s64_plaintext_export(
            &dos,
            &body,
            WORK_PROMPT,
            1u
        );
    }

    if (result == S64_PLAINTEXT_OK) {
        set_status("Text exported");
    } else {
        set_status(s64_plaintext_result_message(result));
    }
}

static void import_plain_text(void)
{
    S64PlainTextResult result;

    if (!capabilities.dos_target) {
        set_status("C64U DOS target unavailable");
        return;
    }
    if (!prompt_text(
            "IMPORT FILE: ",
            WORK_PROMPT,
            S64_PLAINTEXT_FILENAME_CAPACITY,
            0u
        )) {
        set_status("Import cancelled");
        return;
    }
    if (!prompt_yes_no("REPLACE CURRENT DOCUMENT? Y/N")) {
        set_status("Import cancelled");
        return;
    }

    result = s64_plaintext_import(&dos, &body, WORK_PROMPT);
    if (result != S64_PLAINTEXT_OK) {
        set_status(s64_plaintext_result_message(result));
        return;
    }

    s64_document_init(&document, &body);
    s64_publish_state_clear_remote(&publish_state);
    document_replaced = 1u;
    document_changed = 1u;
    set_status("Text imported; save as source");
}

static void connect_destination(void)
{
    S64DestinationResult url_result;

#ifdef S64_DIRECT_PUBLISHER
    if (!network_available) {
        set_status("C64U network target unavailable");
#else
    if (!capabilities.http_target) {
        set_status("C64U HTTP target unavailable");
#endif
        return;
    }
    if (destination.type == S64_DESTINATION_NONE) {
        set_status("Set destination first");
        return;
    }

    clear_session();

    if (destination.type == S64_DESTINATION_STANDALONE) {
        S64StandaloneResult result;

#ifndef S64_DIRECT_PUBLISHER
        if (!prompt_text(
                "PASSWORD: ",
                WORK_SECRET,
                (unsigned char)PROMPT_CAPACITY,
                1u
            )) {
            set_status("Login cancelled");
            return;
        }
#endif

        url_result = s64_destination_connection_url(
            &destination,
            WORK_URL,
            (uint16_t)S64_DESTINATION_URL_CAPACITY
        );
        if (url_result != S64_DESTINATION_OK) {
            memset(WORK_SECRET, 0, PROMPT_CAPACITY);
            set_status(s64_destination_result_message(url_result));
            return;
        }

#ifdef S64_DIRECT_PUBLISHER
        result = s64_direct_publisher_test(&http, WORK_URL,
            destination.allow_insecure_http);
#else
        result = s64_standalone_login(
            &http,
            WORK_URL,
            WORK_SECRET,
            destination.allow_insecure_http,
            STANDALONE_SESSION
        );
        memset(WORK_SECRET, 0, PROMPT_CAPACITY);
#endif
        if (result == S64_STANDALONE_OK) {
            s64_publish_state_set_connected(&publish_state, 1u);
            set_status("Standalone connection ready");
        } else {
            set_status(s64_standalone_result_message(result));
        }
        return;
    }

#ifndef S64_STANDALONE_ONLY
    if (destination.type == S64_DESTINATION_WORDPRESS) {
        S64WordPressResult result;

        if (!prompt_text(
                "USERNAME: ",
                WORK_USERNAME,
                (unsigned char)USERNAME_CAPACITY,
                0u
            )
            || !prompt_text(
                "APP PASSWORD: ",
                WORK_PROMPT,
                (unsigned char)PROMPT_CAPACITY,
                1u
            )) {
            memset(WORK_PROMPT, 0, PROMPT_CAPACITY);
            set_status("Login cancelled");
            return;
        }

        if (!s64_wordpress_session_configure(
                &session.wordpress,
                WORK_USERNAME,
                WORK_PROMPT
            )) {
            memset(WORK_PROMPT, 0, PROMPT_CAPACITY);
            set_status("Invalid WordPress credentials");
            return;
        }
        memset(WORK_PROMPT, 0, PROMPT_CAPACITY);

        url_result = s64_destination_connection_url(
            &destination,
            WORK_URL,
            (uint16_t)S64_DESTINATION_URL_CAPACITY
        );
        if (url_result != S64_DESTINATION_OK) {
            set_status(s64_destination_result_message(url_result));
            return;
        }

        result = s64_wordpress_test_connection(
            &http,
            WORK_URL,
            destination.allow_insecure_http,
            &session.wordpress
        );
        if (result == S64_WORDPRESS_OK) {
            s64_publish_state_set_connected(&publish_state, 1u);
            set_status("WordPress connection ready");
        } else {
            set_status(s64_wordpress_result_message(result));
        }
    }
#else
    set_status("WordPress deferred");
#endif
}

int main(void)
{
    unsigned char key;

    bgcolor(COLOR_BLUE);
    bordercolor(COLOR_BLUE);
    textcolor(COLOR_LIGHTBLUE);
    clrscr();
    cursor(0);

#ifdef S64_MANAGED_OVERLAY
    if (!s64_module_transfer_consume_arrival(S64_MODULE_PUBLISHER)) managed_entry_failed(1u);
    if (!s64_reu_runtime_load(&managed_runtime.reu_control)) managed_entry_failed(2u);
    if (!s64_code_overlay_bootstrap(&managed_runtime.reu_control,
            1u, S64_PACKAGE_VERSION, s64_code_overlay_linked_lengths,
            (void*)S64_CODE_OVERLAY_ADDRESS, s64_reu_load)) managed_entry_failed(3u);
    s64_dos_init(&dos, s64_uci_execute, S64_DOS_TARGET_PRIMARY);
    if (!s64_managed_publisher_adopt(&managed_runtime, &dos, S64_PACKAGE_VERSION)) managed_entry_failed(4u);
    if (!s64_managed_publisher_document(&managed_runtime, &body, &document,
            &document_changed, &document_replaced)) managed_entry_failed(5u);
    if (s64_module_runtime_resume_interactive(&managed_runtime)
            != S64_SYSTEM_OK) managed_entry_failed(6u);
#else
    if (!s64_reu_probe() || s64_reu_probe_banks() < 2u) {
        cputs("SEE64YOU PUBLISHER\r\n128K REU REQUIRED\r\n");
        return 1;
    }

    s64_handoff_init(WORK_HANDOFF);
    if (!s64_handoff_load(WORK_HANDOFF)
        || !s64_handoff_apply_document(WORK_HANDOFF, &body, &document)) {
        cputs("SEE64YOU PUBLISHER\r\nNO EDITOR HANDOFF\r\n");
        cputs("RUN EDITOR FIRST\r\n");
        return 1;
    }

#endif
#ifdef S64_DIRECT_PUBLISHER
    http.execute = s64_uci_execute;
    network_available = s64_uci_probe_target(0x03u);
#else
    s64_http_init(&http, s64_uci_execute);
#endif
    s64_dos_init(&dos, s64_uci_execute, S64_DOS_TARGET_PRIMARY);
    s64_uci_probe_capabilities(&capabilities);
    s64_destination_clear(&destination);
    s64_publish_state_init(&publish_state);
#ifndef S64_DIRECT_PUBLISHER
    memset(&session, 0, sizeof(session));
#endif
#ifndef S64_MANAGED_OVERLAY
    document_changed = 0u;
    document_replaced = 0u;
#endif
#ifdef S64_MANAGED_OVERLAY
    set_status("Settings transition not ready");
#else
    (void)restore_settings();
#endif

    for (;;) {
        render();
        key = (unsigned char)cgetc();

        if (key == '1') {
            open_settings();
        } else if (key == '2') {
            connect_destination();
        } else if (key == '3') {
            save_remote_document();
        } else if (key == '4') {
            toggle_document_status();
#ifdef S64_DIRECT_PUBLISHER
        } else if (key == 't' || key == 'T') {
            document.type = document.type == S64_DOCUMENT_PAGE
                ? S64_DOCUMENT_POST : S64_DOCUMENT_PAGE;
            s64_publish_state_clear_remote(&publish_state);
            mark_document_changed();
#endif
        } else if (key == '5') {
            delete_remote_document();
        } else if (key == '6') {
            list_open_remote(S64_DOCUMENT_PAGE);
        } else if (key == '7') {
            list_open_remote(S64_DOCUMENT_POST);
        } else if (key == '8') {
            preview_document();
        } else if (key == '9') {
            export_plain_text();
        } else if (key == '0') {
            import_plain_text();
        } else if (key == CH_STOP) {
            return_to_editor();
        }
    }
}
