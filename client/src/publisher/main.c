#include <conio.h>
#include <string.h>

#include "buffer.h"
#include "charset.h"
#include "document.h"
#include "platform/http.h"
#include "platform/chain.h"
#include "platform/reu.h"
#include "platform/uci.h"
#include "publisher/destination.h"
#include "publisher/handoff.h"
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
static S64UciCapabilities capabilities;
static S64PublishDestination destination;
static S64PublishState publish_state;
static S64PublisherSession session;
static unsigned char document_changed;

#define WORK_STATUS ((char*)&s64_publisher_workspace.bytes[0])
#define WORK_URL ((char*)&s64_publisher_workspace.bytes[0])
#define WORK_PROMPT ((char*)&s64_publisher_workspace.bytes[0])
#define WORK_USERNAME ((char*)&s64_publisher_workspace.bytes[S64_PUBLISH_WORK_MAIN_USERNAME_OFFSET])
#define WORK_SECRET ((char*)&s64_publisher_workspace.bytes[S64_PUBLISH_WORK_MAIN_USERNAME_OFFSET])
#define WORK_REMOTE_ID ((char*)&s64_publisher_workspace.bytes[S64_PUBLISH_WORK_MAIN_USERNAME_OFFSET])
#define WORK_HANDOFF ((S64PublisherHandoff*)&s64_publisher_workspace.bytes[S64_PUBLISH_WORK_HANDOFF_OFFSET])
#define WORK_SETTINGS ((S64SettingsExchange*)&s64_publisher_workspace.bytes[0])
#define WORK_SELECTION ((char*)&s64_publisher_workspace.bytes[S64_PUBLISH_WORK_SELECTION_OFFSET])

static unsigned char selected_state;
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
    print_value(8u, "HTTP: ", capabilities.http_target
        ? "available" : "unavailable");
    print_value(9u, "Connection: ", publish_state.connected
        ? "ready" : "not connected");
    print_value(10u, "Remote: ", publish_state.remote_bound
        ? publish_state.remote_identity : "(new document)");

    gotoxy(0u, 12u);
    cputs("1  PUBLISHING SETTINGS");
    gotoxy(0u, 13u);
    cputs("2  TEST / LOGIN");
    gotoxy(0u, 14u);
    cputs("3  SAVE / UPDATE REMOTE");
    gotoxy(0u, 15u);
    cputs("4  TOGGLE DRAFT / PUBLISHED");
    gotoxy(0u, 16u);
    cputs("5  DELETE REMOTE");
    gotoxy(0u, 17u);
    cputs("6 OPEN PAGE  7 OPEN POST");
    gotoxy(0u, 19u);
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
    memset(&session, 0, sizeof(session));
    s64_publish_state_set_connected(&publish_state, 0u);
}

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

static void mark_document_changed(void)
{
    document_changed = 1u;
}

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

static void return_to_editor(void)
{
    if (!preserve_document_handoff(1u)) return;
    clrscr();
    cputs("RETURNING TO SEE64YOU EDITOR...\r\n");
    s64_chain_editor();
}

static void open_settings(void)
{
    if (!preserve_document_handoff(0u)) return;
    if (!s64_settings_bridge_request_edit()) {
        set_status("Cannot open publishing settings");
        return;
    }

    clrscr();
    cputs("OPENING PUBLISHING SETTINGS...\r\n");
    s64_chain_settings();
}

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
    if (!s64_relay_set_key(WORK_SETTINGS->relay_key)) {
        s64_destination_clear(&destination);
        (void)s64_relay_set_key("");
        set_status("Publisher settings invalid");
        return 0u;
    }

    set_status(settings_status(result));
    return 1u;
}

static unsigned char publishing_ready(void)
{
    if (!capabilities.http_target) {
        set_status("C64U HTTP target unavailable");
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
            &session.standalone,
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
            &session.standalone,
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

static void list_open_remote(S64DocumentType type)
{
    S64DestinationResult url_result;

    if (!publishing_ready()) {
        return;
    }

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
            &session.standalone,
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
            &session.standalone,
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

static void connect_destination(void)
{
    S64DestinationResult url_result;

    if (!capabilities.http_target) {
        set_status("C64U HTTP target unavailable");
        return;
    }
    if (destination.type == S64_DESTINATION_NONE) {
        set_status("Set destination first");
        return;
    }

    clear_session();

    if (destination.type == S64_DESTINATION_STANDALONE) {
        S64StandaloneResult result;

        if (!prompt_text(
                "PASSWORD: ",
                WORK_SECRET,
                (unsigned char)PROMPT_CAPACITY,
                1u
            )) {
            set_status("Login cancelled");
            return;
        }

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

        result = s64_standalone_login(
            &http,
            WORK_URL,
            WORK_SECRET,
            destination.allow_insecure_http,
            &session.standalone
        );
        memset(WORK_SECRET, 0, PROMPT_CAPACITY);
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

    s64_http_init(&http, s64_uci_execute);
    s64_uci_probe_capabilities(&capabilities);
    s64_destination_clear(&destination);
    s64_publish_state_init(&publish_state);
    memset(&session, 0, sizeof(session));
    document_changed = 0u;
    document_replaced = 0u;
    (void)restore_settings();

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
        } else if (key == '5') {
            delete_remote_document();
        } else if (key == '6') {
            list_open_remote(S64_DOCUMENT_PAGE);
        } else if (key == '7') {
            list_open_remote(S64_DOCUMENT_POST);
        } else if (key == CH_STOP) {
            return_to_editor();
        }
    }
}
