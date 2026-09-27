#include <conio.h>
#include <string.h>

#include "buffer.h"
#include "document.h"
#include "platform/http.h"
#include "platform/reu.h"
#include "platform/uci.h"
#include "publisher/destination.h"
#include "publisher/handoff.h"
#include "publisher/state.h"
#include "publisher/standalone.h"
#include "publisher/wordpress.h"

#define SCREEN_COLUMNS 40u
#define STATUS_ROW 24u
#define PROMPT_CAPACITY S64_DESTINATION_BASE_URL_CAPACITY
#define USERNAME_CAPACITY 61u

typedef union {
    S64StandaloneSession standalone;
    S64WordPressSession wordpress;
} S64PublisherSession;

static S64Buffer body;
static S64Document document;
static S64PublisherHandoff handoff;
static S64HttpTransport http;
static S64UciCapabilities capabilities;
static S64PublishDestination destination;
static S64PublishState publish_state;
static S64PublisherSession session;
static char connection_url[S64_DESTINATION_URL_CAPACITY];
static char prompt_buffer[PROMPT_CAPACITY];
static char username[USERNAME_CAPACITY];
static char status_text[SCREEN_COLUMNS + 1u];

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
        status_text[i] = text[i];
        ++i;
    }
    status_text[i] = '\0';
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

    gotoxy(0u, 12u);
    cputs("1  SET DESTINATION");
    gotoxy(0u, 13u);
    cputs("2  TEST / LOGIN");
    gotoxy(0u, 15u);
    cputs("RUN/STOP  EXIT TO BASIC");

    clear_row(STATUS_ROW);
    gotoxy(0u, STATUS_ROW);
    revers(1);
    cputs(status_text);
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
        if (key >= 0x20u && key <= 0x7eu
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

static void clear_connection(void)
{
    memset(&session, 0, sizeof(session));
    s64_publish_state_set_connected(&publish_state, 0u);
    s64_publish_state_clear_remote(&publish_state);
}

static void setup_destination(void)
{
    unsigned char key;
    S64DestinationType type;
    S64DestinationResult result;
    unsigned char allow_insecure;

    clear_row(STATUS_ROW);
    revers(1);
    gotoxy(0u, STATUS_ROW);
    cputs("1=STANDALONE 2=WORDPRESS STOP=CANCEL");

    for (;;) {
        key = (unsigned char)cgetc();
        if (key == '1') {
            type = S64_DESTINATION_STANDALONE;
            break;
        }
        if (key == '2') {
            type = S64_DESTINATION_WORDPRESS;
            break;
        }
        if (key == CH_STOP) {
            revers(0);
            set_status("Destination unchanged");
            return;
        }
    }
    revers(0);

    if (!prompt_text(
            "SITE URL: ",
            prompt_buffer,
            (unsigned char)sizeof(prompt_buffer),
            0u
        )) {
        set_status("Destination unchanged");
        return;
    }

    allow_insecure = prompt_yes_no("ALLOW PLAIN HTTP? Y/N");
    result = s64_destination_configure(
        &destination,
        type,
        prompt_buffer,
        allow_insecure
    );
    if (result != S64_DESTINATION_OK) {
        set_status(s64_destination_result_message(result));
        return;
    }

    clear_connection();
    set_status("Destination configured");
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

    url_result = s64_destination_connection_url(
        &destination,
        connection_url,
        (uint16_t)sizeof(connection_url)
    );
    if (url_result != S64_DESTINATION_OK) {
        set_status(s64_destination_result_message(url_result));
        return;
    }

    clear_connection();

    if (destination.type == S64_DESTINATION_STANDALONE) {
        S64StandaloneResult result;

        if (!prompt_text(
                "PASSWORD: ",
                prompt_buffer,
                (unsigned char)sizeof(prompt_buffer),
                1u
            )) {
            set_status("Login cancelled");
            return;
        }

        result = s64_standalone_login(
            &http,
            connection_url,
            prompt_buffer,
            destination.allow_insecure_http,
            &session.standalone
        );
        memset(prompt_buffer, 0, sizeof(prompt_buffer));
        if (result == S64_STANDALONE_OK) {
            s64_publish_state_set_connected(&publish_state, 1u);
            set_status("Standalone connection ready");
        } else {
            set_status(s64_standalone_result_message(result));
        }
        return;
    }

    if (destination.type == S64_DESTINATION_WORDPRESS) {
        S64WordPressResult result;

        if (!prompt_text(
                "USERNAME: ",
                username,
                (unsigned char)sizeof(username),
                0u
            )
            || !prompt_text(
                "APP PASSWORD: ",
                prompt_buffer,
                (unsigned char)sizeof(prompt_buffer),
                1u
            )) {
            memset(prompt_buffer, 0, sizeof(prompt_buffer));
            set_status("Login cancelled");
            return;
        }

        if (!s64_wordpress_session_configure(
                &session.wordpress,
                username,
                prompt_buffer
            )) {
            memset(prompt_buffer, 0, sizeof(prompt_buffer));
            set_status("Invalid WordPress credentials");
            return;
        }
        memset(prompt_buffer, 0, sizeof(prompt_buffer));

        result = s64_wordpress_test_connection(
            &http,
            connection_url,
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

    s64_handoff_init(&handoff);
    if (!s64_handoff_load(&handoff)
        || !s64_handoff_apply_document(&handoff, &body, &document)) {
        cputs("SEE64YOU PUBLISHER\r\nNO EDITOR HANDOFF\r\n");
        cputs("RUN EDITOR FIRST\r\n");
        return 1;
    }

    s64_http_init(&http, s64_uci_execute);
    s64_uci_probe_capabilities(&capabilities);
    s64_destination_clear(&destination);
    s64_publish_state_init(&publish_state);
    memset(&session, 0, sizeof(session));
    status_text[0] = '\0';

    for (;;) {
        render();
        key = (unsigned char)cgetc();

        if (key == '1') {
            setup_destination();
        } else if (key == '2') {
            connect_destination();
        } else if (key == CH_STOP) {
            break;
        }
    }

    memset(prompt_buffer, 0, sizeof(prompt_buffer));
    memset(username, 0, sizeof(username));
    clrscr();
    cputs("SEE64YOU PUBLISHER EXITED\r\n");
    return 0;
}
