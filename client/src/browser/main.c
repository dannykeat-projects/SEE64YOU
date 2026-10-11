#include <conio.h>
#include <string.h>
#include "browser/protocol.h"
#include "browser/http.h"
#include "charset.h"
#include "platform/reu.h"
#include "platform/reu_runtime.h"
#include "platform/dos.h"
#include "publisher/settings_store.h"

/* Fixed strides keep every DMA record inside one REU bank. */
#define MAX_ROWS 1024u
#define MAX_LINKS 256u
#define VIEW_ROWS 19u
typedef struct { char text[40]; uint16_t link; } Row;
static S64ReuRuntimeControl control;
static S64ReuAllocation row_store, link_store;
static S64BrowserParser parser;
static S64BrowserUrl current, next;
static S64NetTransport network;
static S64Dos dos;
static S64SettingsExchange settings;
static uint16_t rows, links, top, selected;
static Row row;
static char input[S64_BROWSER_URL_CAPACITY], native_host[96], native_path[224];
static char href[191];
static const char* status = "O: open a public See64You URL";
static uint8_t network_ready, pulse;

static uint8_t transfer(uint8_t write, uint32_t address, void* data, uint16_t length)
{
    if (write) return s64_reu_store((uint8_t)(address >> 16), (uint16_t)address, data, length);
    return s64_reu_load((uint8_t)(address >> 16), (uint16_t)address, data, length);
}

static uint8_t convert(char* destination, const char* ascii)
{
    uint8_t value;
    while (*ascii) {
        if (!s64_charset_ascii_to_native((uint8_t)*ascii++, &value)) return 0;
        *destination++ = (char)value;
    }
    *destination = 0; return 1;
}

static uint8_t record(uint8_t kind, const char* text, const char* link, void* context)
{
    uint16_t id = 65535u;
    uint8_t column, value;
    (void)context;
    if (kind == 0x4cu) {
        if (links >= MAX_LINKS) return 0;
        id = links++;
        strcpy(href, link);
        if (!transfer(1, link_store.address + (uint32_t)id * 256UL,
            href, (uint16_t)(strlen(href) + 1u))) return 0;
    }
    do {
        if (rows >= MAX_ROWS) return 0;
        memset(row.text, ' ', sizeof(row.text)); row.link = id;
        column = 0;
        while (*text && column < 40u) {
            if (!s64_charset_ascii_to_native((uint8_t)*text++, &value)) return 0;
            row.text[column++] = (char)value;
        }
        if (!transfer(1, row_store.address + (uint32_t)rows * 64UL, &row, sizeof(row))) return 0;
        ++rows;
    } while (*text);
    return 1;
}

static uint8_t receive(const uint8_t* data, uint16_t length, void* context)
{
    (void)context;
    return s64_browser_parser_feed(&parser, data, length) == S64_BROWSER_PROTOCOL_OK;
}

static uint8_t wait_read(void* context)
{
    uint8_t key;
    (void)context;
    gotoxy(0, 1); cclear(40); gotoxy(0, 1);
    cputs("Loading... RUN/STOP cancels ");
    cputc((++pulse & 8u) ? '*' : ' ');
    if (kbhit()) { key = (uint8_t)cgetc(); if (key == 3u) return 0; }
    return 1;
}

static void render(void)
{
    uint16_t index;
    uint8_t y, x;
    clrscr(); textcolor(COLOR_WHITE);
    cputs("SEE64YOU PUBLIC BROWSER"); gotoxy(0,1); cputs(status);
    for (y = 0; y < VIEW_ROWS; ++y) {
        index = top + y;
        if (index >= rows) break;
        if (!transfer(0, row_store.address + (uint32_t)index * 64UL, &row, sizeof(row))) {
            rows = links = 0; status = "REU read failed; reopen URL"; break;
        }
        gotoxy(0, y + 3u);
        textcolor(row.link == 65535u ? COLOR_WHITE : COLOR_YELLOW);
        revers(index == selected);
        for (x = 0; x < 40u; ++x) cputc(row.text[x]);
        revers(0);
    }
    textcolor(COLOR_WHITE); gotoxy(0,23);
    cputs("Cursor: select  RETURN: follow link");
    gotoxy(0,24); cputs("SPACE: next  -: previous  O: URL  STOP");
}

static void load(void)
{
    S64BrowserHttpResult result;
    rows = links = top = selected = 0;
    status = "Connecting... RUN/STOP cancels"; render();
    if (!network_ready) { status = "Ultimate network unavailable"; return; }
    if (!convert(native_host, next.host) || !convert(native_path, next.path)) {
        status = "Invalid URL text"; return;
    }
    s64_browser_parser_init(&parser, record, 0);
    result = s64_browser_http_get(&network, native_host, next.port, native_path,
        receive, 0, wait_read, 0);
    if (result != S64_BROWSER_HTTP_OK || s64_browser_parser_finish(&parser) != S64_BROWSER_PROTOCOL_OK) {
        rows = links = 0;
        if (result == S64_BROWSER_HTTP_CANCELLED) status = "Cancelled; socket closed";
        else if (result == S64_BROWSER_HTTP_CLOSE) {
            network_ready = 0;
            status = "Socket close failed; restart browser";
        }
        else if (result == S64_BROWSER_HTTP_SERVER) status = "Page missing or server error";
        else if (result == S64_BROWSER_HTTP_NETWORK) status = "Network failed; reopen URL";
        else status = "Invalid/too large browser response";
        return;
    }
    current = next; status = "Loaded. Yellow text is a link.";
}

static void url_error(S64BrowserProtocolResult result)
{
    status = result == S64_BROWSER_PROTOCOL_TLS_UNSUPPORTED
        ? "HTTPS unavailable; URL not opened" : "Invalid URL; URL not opened";
}

static void prompt(void)
{
    uint16_t length = 0;
    uint8_t key, ascii;
    S64BrowserProtocolResult result;
    clrscr(); cputs("OPEN PUBLIC URL (HTTP)\r\n\r\n"); cursor(1);
    for (;;) {
        key = (uint8_t)cgetc();
        if (key == 3u) { cursor(0); return; }
        if (key == 13u) break;
        if (key == 20u) {
            if (length) { --length; cputc(20); }
        } else if (length + 1u < sizeof(input)
            && s64_charset_native_to_ascii(key, &ascii) && ascii >= 32u && ascii <= 126u) {
            input[length++] = (char)ascii; cputc((char)key);
        }
    }
    cursor(0); input[length] = 0;
    result = s64_browser_url_parse(input, &next);
    if (result == S64_BROWSER_PROTOCOL_OK) load(); else url_error(result);
}

static void follow(void)
{
    S64BrowserProtocolResult result;
    if (!rows || selected >= rows) return;
    if (!transfer(0, row_store.address + (uint32_t)selected * 64UL, &row, sizeof(row))) {
        rows = links = 0; status = "REU read failed; reopen URL"; return;
    }
    if (row.link == 65535u) { status = "Select yellow link text"; return; }
    if (row.link >= links || !transfer(0, link_store.address + (uint32_t)row.link * 256UL,
        href, sizeof(href))) { rows = links = 0; status = "Link read failed; reopen URL"; return; }
    href[sizeof(href)-1u] = 0;
    result = s64_browser_url_resolve(&current, href, &next);
    if (result == S64_BROWSER_PROTOCOL_OK) load(); else url_error(result);
}

int main(void)
{
    static const char index_path[] = {105,110,100,101,120,46,112,104,112,0};
    uint16_t i;
    uint8_t key, ascii;
    clrscr(); textcolor(COLOR_WHITE); cursor(0);
    cputs("SEE64YOU PUBLIC BROWSER\r\nChecking Ultimate/16 MB REU...");
    if (!s64_reu_probe() || s64_reu_probe_banks() != 256u) {
        cputs("\r\n16 MB REU required\r\n"); return 1;
    }
    s64_reu_runtime_init(&control, 1u, 1UL);
    if (!s64_reu_runtime_allocate(&control, S64_REU_OWNER_PREVIEW, 65536UL, 256u, &row_store)
        || !s64_reu_runtime_allocate(&control, S64_REU_OWNER_PREVIEW, 65536UL, 256u, &link_store)) {
        cputs("\r\nBrowser workspace unavailable\r\n"); return 1;
    }
    s64_net_init(&network, s64_uci_execute);
    network_ready = s64_uci_probe_target(3u);
    s64_dos_init(&dos, s64_uci_execute, S64_DOS_TARGET_PRIMARY);
    if (s64_settings_store_load(&dos, &settings) == S64_SETTINGS_STORE_OK) {
        for (i = 0; settings.base_url[i]; ++i) {
            if (!s64_charset_native_to_ascii((uint8_t)settings.base_url[i], &ascii)) break;
            input[i] = (char)ascii;
        }
        input[i] = 0;
        if (!settings.base_url[i] && i) {
            if ((uint8_t)input[i-1u] != 47u) input[i++] = (char)47;
            strcpy(input + i, index_path);
            if (s64_browser_url_parse(input, &next) == S64_BROWSER_PROTOCOL_OK) load();
        }
        /* Credentials are neither needed nor retained by the public reader. */
        memset(&settings, 0, sizeof(settings));
    }
    for (;;) {
        render(); key = (uint8_t)cgetc();
        if (key == 3u) break;
        if (key == 13u) follow();
        else if (key == 'o' || key == 'O') prompt();
        else if (key == 17u && selected + 1u < rows) {
            ++selected; if (selected >= top + VIEW_ROWS) ++top;
        } else if (key == 145u && selected) {
            --selected; if (selected < top) top = selected;
        } else if (key == ' ' && top + VIEW_ROWS < rows) { top += VIEW_ROWS; selected = top; }
        else if (key == '-' && top) { top = top > VIEW_ROWS ? top - VIEW_ROWS : 0; selected = top; }
    }
    clrscr(); return 0;
}
