#include "publisher/persistent_ui.h"

#include <conio.h>

#define SCREEN_COLUMNS 40u
#define STATUS_ROW 24u

static void clear_row(uint8_t row)
{
    uint8_t i;
    gotoxy(0u, row);
    for (i = 0u; i < SCREEN_COLUMNS; ++i) cputc(' ');
}

static void print_value(uint8_t row, const char* label, const char* value)
{
    uint8_t column = 0u;

    clear_row(row);
    gotoxy(0u, row);
    while (column < SCREEN_COLUMNS && label && *label) {
        cputc((uint8_t)*label++);
        ++column;
    }
    while (column < SCREEN_COLUMNS && value && *value) {
        cputc((uint8_t)*value++);
        ++column;
    }
}

void s64_persistent_publisher_render(
    const S64PersistentPublisher* publisher,
    const char* status)
{
    const S64Document* document;
    const char* destination_name;
    const char* type_name;
    const char* state_name;
    uint8_t i = 0u;

    if (!publisher || !s64_persistent_publisher_valid(publisher)) return;
    document = publisher->document;

    destination_name = publisher->destination.type == S64_DESTINATION_STANDALONE
        ? "Standalone"
        : (publisher->destination.type == S64_DESTINATION_WORDPRESS
            ? "WordPress" : "Not configured");
    type_name = document->type == S64_DOCUMENT_POST ? "post" : "page";
    state_name = document->status == S64_DOCUMENT_PUBLISHED
        ? "published" : "draft";

    clrscr();
    revers(1);
    gotoxy(0u, 0u);
    cputs(" SEE64YOU PUBLISHER                    ");
    revers(0);

    print_value(2u, "Title: ", document->title[0] ? document->title : "(untitled)");
    print_value(3u, "Slug:  ", document->slug[0] ? document->slug : "(unset)");
    print_value(4u, "Type:  ", type_name);
    print_value(5u, "State: ", state_name);
    print_value(7u, "Destination: ", destination_name);
    print_value(8u, "Direct HTTP: ",
        publisher->network_available ? "unencrypted" : "unavailable");
    print_value(9u, "Connection: ",
        publisher->publish_state.connected ? "ready" : "not connected");
    print_value(10u, "Remote: ",
        publisher->publish_state.remote_bound
            ? publisher->publish_state.remote_identity : "(new document)");

    gotoxy(0u, 12u); cputs("1  PUBLISHING SETTINGS");
    gotoxy(0u, 13u); cputs("2  TEST DIRECT KEY");
    gotoxy(0u, 14u); cputs("3  SAVE / UPDATE REMOTE");
    gotoxy(0u, 15u); cputs("4  TOGGLE DRAFT / PUBLISHED");
    gotoxy(0u, 16u); cputs("5  DELETE REMOTE");
    gotoxy(0u, 17u); cputs("6 OPEN PAGE / 7 OPEN POST");
    gotoxy(0u, 18u); cputs("8 PREVIEW");
    gotoxy(0u, 19u); cputs("9 EXPORT TXT  0 IMPORT TXT");
    gotoxy(0u, 20u); cputs("T TOGGLE PAGE / POST");
    gotoxy(0u, 21u); cputs("RUN/STOP  RETURN TO EDITOR");

    clear_row(STATUS_ROW);
    gotoxy(0u, STATUS_ROW);
    revers(1);
    if (status) {
        while (i < SCREEN_COLUMNS && status[i]) {
            cputc((uint8_t)status[i]);
            ++i;
        }
    }
    revers(0);
}

S64PersistentPublisherAction s64_persistent_publisher_key(
    S64PersistentPublisher* publisher,
    uint8_t key)
{
    S64Document* document;

    if (!publisher || !s64_persistent_publisher_valid(publisher)) {
        return S64_PUBLISH_UI_NONE;
    }

    document = publisher->document;

    if (key == '1') return S64_PUBLISH_UI_SETTINGS;
    if (key == '2') return S64_PUBLISH_UI_CONNECT;
    if (key == '3') return S64_PUBLISH_UI_SAVE;
    if (key == '5') return S64_PUBLISH_UI_DELETE;
    if (key == '6') return S64_PUBLISH_UI_LIST_PAGE;
    if (key == '7') return S64_PUBLISH_UI_LIST_POST;
    if (key == '8') return S64_PUBLISH_UI_PREVIEW;
    if (key == '9') return S64_PUBLISH_UI_EXPORT_TEXT;
    if (key == '0') return S64_PUBLISH_UI_IMPORT_TEXT;
    if (key == CH_STOP) return S64_PUBLISH_UI_RETURN_EDITOR;

    if (key == '4') {
        document->status = document->status == S64_DOCUMENT_PUBLISHED
            ? S64_DOCUMENT_DRAFT
            : S64_DOCUMENT_PUBLISHED;
        s64_persistent_publisher_mark_changed(publisher, 0u);
        return S64_PUBLISH_UI_NONE;
    }

    if (key == 't' || key == 'T') {
        document->type = document->type == S64_DOCUMENT_PAGE
            ? S64_DOCUMENT_POST
            : S64_DOCUMENT_PAGE;
        s64_persistent_publisher_mark_changed(publisher, 0u);
        return S64_PUBLISH_UI_TYPE_CHANGED;
    }

    return S64_PUBLISH_UI_NONE;
}
