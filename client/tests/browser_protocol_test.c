#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "browser/protocol.h"

static unsigned records;
static uint8_t reject_record;
static char seen_text[101], seen_href[191];

static uint8_t collect(uint8_t kind, const char* text, const char* href, void* context)
{
    assert(context == &records);
    assert(kind == 0x54u || kind == 0x4cu || kind == 0x43u);
    ++records;
    strcpy(seen_text, text);
    if (href) strcpy(seen_href, href);
    return (uint8_t)!reject_record;
}

static void urls(void)
{
    S64BrowserUrl base, output;
    char formatted[320], long_url[400];
    const char* invalid[] = {
        "", "ftp://host/a", "http://user@host/a", "http://host:0/a",
        "http://host:65536/a", "http://host:/a", "http://host:80x/a",
        "http://host/a\r\nX: injected", "http://host/a b", "http://host/a#part",
        "http://host/a\\b", "http://[::1]/a", "http:///a", "http://host\t/a"
    };
    unsigned i;
    assert(s64_browser_url_parse("HtTp://example.test:8080/site/index.php?old=1", &base)
        == S64_BROWSER_PROTOCOL_OK);
    assert(base.port == 8080u);
    assert(strcmp(base.host, "example.test") == 0);
    assert(s64_browser_url_resolve(&base, "page.php?slug=a%20b", &output) == 0);
    assert(strcmp(output.path, "/site/page.php?slug=a%20b") == 0);
    assert(s64_browser_url_format(&output, formatted, sizeof(formatted)));
    assert(strcmp(formatted, "http://example.test:8080/site/page.php?slug=a%20b") == 0);
    assert(!s64_browser_url_format(&output, formatted, 10u));
    assert(s64_browser_url_resolve(&base, "../posts.php", &output) == 0);
    assert(strcmp(output.path, "/posts.php") == 0);
    assert(s64_browser_url_resolve(&base, "./a/../b/./c/../../d/", &output) == 0);
    assert(strcmp(output.path, "/site/d/") == 0);
    assert(s64_browser_url_resolve(&base, "../../../../outside", &output) == 0);
    assert(strcmp(output.path, "/outside") == 0);
    assert(s64_browser_url_resolve(&base, "/new/?q=../unchanged", &output) == 0);
    assert(strcmp(output.path, "/new/?q=../unchanged") == 0);
    assert(s64_browser_url_resolve(&base, "?q=2", &output) == 0);
    assert(strcmp(output.path, "/site/index.php?q=2") == 0);
    assert(s64_browser_url_resolve(&base, "", &output) == 0);
    assert(strcmp(output.path, base.path) == 0);
    assert(s64_browser_url_resolve(&base, "//other.test/new", &output) == 0);
    assert(strcmp(output.host, "other.test") == 0 && output.port == 80u);
    assert(s64_browser_url_resolve(&base, "HTTP://elsewhere.test/x", &output) == 0);
    assert(strcmp(output.host, "elsewhere.test") == 0);
    assert(s64_browser_url_resolve(&base, "https://elsewhere.test/x", &output)
        == S64_BROWSER_PROTOCOL_TLS_UNSUPPORTED);
    assert(s64_browser_url_resolve(&base, "javascript:alert(1)", &output)
        == S64_BROWSER_PROTOCOL_INVALID);
    assert(s64_browser_url_parse("https://host/a", &output)
        == S64_BROWSER_PROTOCOL_TLS_UNSUPPORTED);
    for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        assert(s64_browser_url_parse(invalid[i], &output) == S64_BROWSER_PROTOCOL_INVALID);
    assert(s64_browser_url_parse("http://host", &output) == 0);
    assert(strcmp(output.path, "/") == 0);
    assert(s64_browser_url_parse("http://host?q=1", &output) == 0);
    assert(strcmp(output.path, "/?q=1") == 0);
    assert(s64_browser_url_parse("http://host/a/../x", &output) == 0);
    assert(strcmp(output.path, "/x") == 0);
    assert(s64_browser_url_parse("http://host:65535/a", &output) == 0);
    assert(output.port == 65535u);
    strcpy(long_url, "http://"); memset(long_url + 7, 'a', 96);
    strcpy(long_url + 103, "/x");
    assert(s64_browser_url_parse(long_url, &output) == S64_BROWSER_PROTOCOL_INVALID);
    strcpy(long_url, "http://h/"); memset(long_url + 9, 'a', 223); long_url[232] = 0;
    assert(s64_browser_url_parse(long_url, &output) == S64_BROWSER_PROTOCOL_INVALID);
    memset(long_url, 'a', 224); long_url[224] = 0;
    assert(s64_browser_url_resolve(&base, long_url, &output) == S64_BROWSER_PROTOCOL_INVALID);
    /* In-place resolution must retain the directory of the previous page. */
    assert(s64_browser_url_resolve(&base, "next.php", &base) == 0);
    assert(strcmp(base.path, "/site/next.php") == 0);
}

static S64BrowserProtocolResult parse_chunks(const char* data, unsigned size)
{
    S64BrowserParser parser;
    unsigned position = 0, length = (unsigned)strlen(data), amount;
    S64BrowserProtocolResult result;
    records = 0;
    s64_browser_parser_init(&parser, collect, &records);
    while (position < length) {
        amount = length - position;
        if (amount > size) amount = size;
        result = s64_browser_parser_feed(&parser, (const uint8_t*)data + position,
            (uint16_t)amount);
        if (result != S64_BROWSER_PROTOCOL_OK) return result;
        position += amount;
    }
    return s64_browser_parser_finish(&parser);
}

static void wire(void)
{
    const char* good = "SEE64YOU-BROWSE/1\r\nT\tHello world\r\nL\tpage.php?slug=hello\tMy page\n";
    const char* invalid[] = {
        "", "SEE64YOU-BROWSE/2\n", "SEE64YOU-BROWSE/1", "SEE64YOU-BROWSE/1\r",
        "SEE64YOU-BROWSE/1\rT\tbad\n", "SEE64YOU-BROWSE/1\nT\tunfinished",
        "SEE64YOU-BROWSE/1\n\n", "SEE64YOU-BROWSE/1\nT\t\n",
        "SEE64YOU-BROWSE/1\nT\ta\tb\n", "SEE64YOU-BROWSE/1\nX\tbad\n",
        "SEE64YOU-BROWSE/1\nL\t\tlabel\n", "SEE64YOU-BROWSE/1\nL\thref\t\n",
        "SEE64YOU-BROWSE/1\nL\thref\tlabel\textra\n",
        "SEE64YOU-BROWSE/1\nL\ta b\tlabel\n", "SEE64YOU-BROWSE/1\nL\ta|b\tlabel\n",
        "SEE64YOU-BROWSE/1\nT\t\x80\n",
        "SEE64YOU-BROWSE/1\nT\ta\rb\n", "SEE64YOU-BROWSE/1\nT\ta\x1b\n"
    };
    char maximum[300];
    S64BrowserParser parser;
    unsigned i;
    uint8_t nul = 0;
    for (i = 1; i <= strlen(good); ++i) {
        assert(parse_chunks(good, i) == 0);
        assert(records == 2u && strcmp(seen_text, "My page") == 0);
        assert(strcmp(seen_href, "page.php?slug=hello") == 0);
    }
    for (i = 1u; i <= 20u; ++i) {
        assert(parse_chunks("SEE64YOU-BROWSE/2\nC\t617\nT\tColoured page\n", i)
            == S64_BROWSER_PROTOCOL_OK);
        assert(records == 2u && strcmp(seen_text, "Coloured page") == 0);
    }
    assert(parse_chunks("SEE64YOU-BROWSE/2\nT\tMissing colours\n", 1u)
        == S64_BROWSER_PROTOCOL_INVALID);
    assert(parse_chunks("SEE64YOU-BROWSE/2\nC\t617\nC\t617\n", 1u)
        == S64_BROWSER_PROTOCOL_INVALID);
    assert(parse_chunks("SEE64YOU-BROWSE/2\nC\t61G\n", 1u)
        == S64_BROWSER_PROTOCOL_INVALID);
    assert(parse_chunks("SEE64YOU-BROWSE/2\nC\t617\nL\tpage.php\tLink\n", 1u)
        == S64_BROWSER_PROTOCOL_OK);
    assert(parse_chunks("SEE64YOU-BROWSE/1\nC\t617\n", 1u)
        == S64_BROWSER_PROTOCOL_INVALID);
    for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        assert(parse_chunks(invalid[i], 1u) == S64_BROWSER_PROTOCOL_INVALID);
    assert(parse_chunks("SEE64YOU-BROWSE/1\nL\thttps://host/a#part\tUnsupported link\n", 1u)
        == S64_BROWSER_PROTOCOL_OK);
    strcpy(maximum, "SEE64YOU-BROWSE/1\nT\t");
    memset(maximum + 20, 'x', 100); strcpy(maximum + 120, "\n");
    assert(parse_chunks(maximum, 1u) == 0 && records == 1u);
    maximum[120] = 'x'; strcpy(maximum + 121, "\n");
    assert(parse_chunks(maximum, 1u) == S64_BROWSER_PROTOCOL_INVALID);
    strcpy(maximum, "SEE64YOU-BROWSE/1\nL\t");
    memset(maximum + 20, 'a', 190); maximum[210] = '\t';
    memset(maximum + 211, 'b', 58); strcpy(maximum + 269, "\n");
    assert(parse_chunks(maximum, 7u) == 0);
    maximum[269] = 'b'; strcpy(maximum + 270, "\n");
    assert(parse_chunks(maximum, 7u) == S64_BROWSER_PROTOCOL_INVALID);
    strcpy(maximum, "SEE64YOU-BROWSE/1\nL\t");
    memset(maximum + 20, 'a', 191); strcpy(maximum + 211, "\tlabel\n");
    assert(parse_chunks(maximum, 1u) == S64_BROWSER_PROTOCOL_INVALID);
    strcpy(maximum, "SEE64YOU-BROWSE/1\nT\t");
    memset(maximum + 20, 'a', 260); strcpy(maximum + 280, "\n");
    assert(parse_chunks(maximum, 7u) == S64_BROWSER_PROTOCOL_INVALID);
    reject_record = 1u;
    assert(parse_chunks(good, 1u) == S64_BROWSER_PROTOCOL_CALLBACK_FAILED);
    reject_record = 0u;
    s64_browser_parser_init(&parser, collect, &records);
    assert(s64_browser_parser_feed(&parser, &nul, 1u) == S64_BROWSER_PROTOCOL_INVALID);
    assert(s64_browser_parser_feed(&parser, (const uint8_t*)good, 1u)
        == S64_BROWSER_PROTOCOL_INVALID);
    assert(s64_browser_parser_finish(&parser) == S64_BROWSER_PROTOCOL_INVALID);
}

int main(void)
{
    urls(); wire(); puts("Browser protocol tests passed"); return 0;
}
