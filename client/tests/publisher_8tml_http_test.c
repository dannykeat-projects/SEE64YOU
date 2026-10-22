/* Host UCI adapter: production Publisher/REU code against a disposable PHP site. */
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "document.h"
#include "platform/reu.h"
#include "publisher/destination.h"
#include "publisher/standalone_direct.h"
#include "publisher/standalone_documents.h"

static uint8_t fake_reu[S64_BUFFER_CAPACITY];
static uint8_t source[S64_BUFFER_CAPACITY], output[S64_BUFFER_CAPACITY];
static unsigned int source_length, output_length;
static unsigned int opens, closes;
static unsigned long sent_length, maximum_sent;
static int connection = -1, fail_open;
static uint16_t test_port;
static S64Buffer body;
static S64Document document;
static S64HttpTransport http;
static S64PublishDestination destination;
static char url[S64_DESTINATION_URL_CAPACITY];
static char altered_reply[65536];
static size_t altered_length, altered_offset;
static uint8_t alter_status;

uint8_t s64_reu_store(uint8_t bank, uint16_t address, const void* data, uint16_t length)
{
    assert(bank == S64_REU_DOCUMENT_BANK && (unsigned long)address + length <= sizeof(fake_reu));
    memcpy(fake_reu + address, data, length);
    return 1;
}

uint8_t s64_reu_load(uint8_t bank, uint16_t address, void* data, uint16_t length)
{
    assert(bank == S64_REU_DOCUMENT_BANK && (unsigned long)address + length <= sizeof(fake_reu));
    memcpy(data, fake_reu + address, length);
    return 1;
}

static void status(S64UciReply* reply, const char* text)
{
    assert(strlen(text) < reply->status_capacity);
    strcpy(reply->status, text);
    reply->status_length = (uint16_t)strlen(text);
}

static S64UciResult execute(const S64UciCommandPart* parts, uint8_t count, S64UciReply* reply)
{
    uint8_t packet[514], handle = 1;
    ssize_t amount;
    assert(parts[0].data[0] == 3);
    switch (parts[0].data[1]) {
        case 7: {
            struct sockaddr_in address;
            struct timeval timeout = {5, 0};
            assert(count == 3 && parts[0].data[2] == (test_port & 255u)
                && parts[0].data[3] == (test_port >> 8));
            assert(parts[1].length == strlen("127.0.0.1"));
            assert(memcmp(parts[1].data, "127.0.0.1", parts[1].length) == 0);
            if (fail_open) return S64_UCI_TIMEOUT;
            assert(connection == -1);
            connection = socket(AF_INET, SOCK_STREAM, 0);
            assert(connection >= 0);
            assert(setsockopt(connection, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == 0);
            memset(&address, 0, sizeof(address));
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            address.sin_port = htons(test_port);
            assert(connect(connection, (struct sockaddr*)&address, sizeof(address)) == 0);
            ++opens;
            sent_length = 0;
            altered_length = altered_offset = 0;
            assert(reply->data_sink(&handle, 1, reply->data_context));
            break;
        }
        case 17: {
            uint16_t offset = 0;
            assert(count == 2 && connection >= 0);
            while (offset < parts[1].length) {
                amount = send(connection, parts[1].data + offset, parts[1].length - offset, 0);
                assert(amount > 0);
                offset += (uint16_t)amount;
            }
            sent_length += offset;
            if (sent_length > maximum_sent) maximum_sent = sent_length;
            packet[0] = (uint8_t)offset;
            packet[1] = (uint8_t)(offset >> 8);
            assert(reply->data_sink(packet, 2, reply->data_context));
            break;
        }
        case 16: {
            uint16_t maximum = parts[0].data[3] | ((uint16_t)parts[0].data[4] << 8);
            assert(count == 1 && maximum <= 512);
            /* Small reads split response headers, metadata and source repeatedly. */
            if (maximum > 37) maximum = 37;
            if (alter_status) {
                if (!altered_length) {
                    char* header;
                    do {
                        assert(altered_length + 1 < sizeof(altered_reply));
                        amount = recv(connection, altered_reply + altered_length,
                                      sizeof(altered_reply) - altered_length - 1, 0);
                        assert(amount >= 0);
                        altered_length += (size_t)amount;
                    } while (amount);
                    altered_reply[altered_length] = 0;
                    header = strstr(altered_reply, "\r\nX-See64You-Status: ");
                    if (header) {
                        char* end = strstr(header + 2, "\r\n");
                        size_t line_length = (size_t)(end - header);
                        if (alter_status == 1) {
                            memmove(header, end, altered_length - (size_t)(end - altered_reply) + 1);
                            altered_length -= line_length;
                        } else if (alter_status == 2) {
                            /* Same length as published: invalid value must fail closed. */
                            memcpy(header + strlen("\r\nX-See64You-Status: "), "garbageee", 9);
                        } else {
                            assert(altered_length + line_length < sizeof(altered_reply));
                            memmove(end + line_length, end, altered_length - (size_t)(end - altered_reply) + 1);
                            memcpy(end, header, line_length);
                            altered_length += line_length;
                        }
                    }
                }
                amount = (ssize_t)(altered_length - altered_offset);
                if (amount > maximum) amount = maximum;
                memcpy(packet + 2, altered_reply + altered_offset, (size_t)amount);
                altered_offset += (size_t)amount;
            } else {
                amount = recv(connection, packet + 2, maximum, 0);
                if (amount < 0 && errno == EINTR) return execute(parts, count, reply);
                assert(amount >= 0);
            }
            packet[0] = (uint8_t)amount;
            packet[1] = (uint8_t)((unsigned int)amount >> 8);
            if (!reply->data_sink(packet, (uint16_t)(amount + 2), reply->data_context))
                return S64_UCI_CONSUMER_ABORTED;
            status(reply, amount ? "00,OK" : "01,CLOSED");
            return S64_UCI_OK;
        }
        case 9:
            assert(close(connection) == 0);
            connection = -1;
            ++closes;
            break;
        default: assert(0);
    }
    status(reply, "00,OK");
    return S64_UCI_OK;
}

static uint8_t collect(const uint8_t* data, uint16_t length, void* context)
{
    (void)context;
    assert(output_length + length <= sizeof(output));
    memcpy(output + output_length, data, length);
    output_length += length;
    return 1;
}

static void fixture(unsigned int kind)
{
    static const char complex[] = "---\n8tml: 1\ntitle: Native source\nX-Future:  Keep : CASE  \n# comment\n---\n"
        "# Hello\n:::future data\nopaque\ttext with \"quotes\" and \\slash\n:::\n";
    static const char minimal[] = "---\n8tml: 1\n---\n";
    unsigned int i;
    const char* prefix = kind == 0 ? complex : minimal;
    source_length = (unsigned int)strlen(prefix);
    memcpy(source, prefix, source_length);
    if (kind == 1) {
        static const uint8_t pattern[] = {'"', '\\', '\t', '\n'};
        for (i = source_length; i < sizeof(source); ++i) source[i] = pattern[(i - source_length) % 4];
        source_length = sizeof(source);
    }
}

static void load_fixture(S64DocumentType type, const char* slug, S64DocumentStatus state)
{
    S64DocumentWireReader reader;
    s64_document_wire_reader_init(&reader, &document);
    assert(s64_document_wire_reader_write(source, (uint16_t)source_length, &reader));
    assert(s64_document_wire_reader_finish(&reader) == S64_DOCUMENT_OK);
    document.type = type;
    document.status = state;
    assert(s64_document_set_slug(&document, slug));
}

static void save_remote(void)
{
    assert(s64_destination_save_url(&destination, 0, 0, url, sizeof(url)) == S64_DESTINATION_OK);
    assert(s64_standalone_save_document(&http, url, 1, 0, &document) == S64_STANDALONE_OK);
}

static void open_remote(S64DocumentType type, const char* slug, S64DocumentStatus state)
{
    S64StandaloneResult result;
    assert(s64_destination_open_url(&destination, type, slug, url, sizeof(url)) == S64_DESTINATION_OK);
    assert(strstr(url, "format=8tml"));
    result = s64_standalone_open_document(&http, url, 1, 0, &document);
    if (result != S64_STANDALONE_OK) fprintf(stderr, "Open %s failed: %u (%s)\n", slug,
        (unsigned int)result, s64_standalone_result_message(result));
    assert(result == S64_STANDALONE_OK);
    assert(document.status == state);
    output_length = 0;
    assert(s64_document_write_wire(&document, collect, 0) == S64_DOCUMENT_OK);
    assert(output_length == source_length && memcmp(output, source, source_length) == 0);
    /* Raw source contains no publishing identity; production UI restores selection. */
    document.type = type;
    assert(s64_document_set_slug(&document, slug));
}

static void export_source(const char* directory, const char* name)
{
    char path[1024];
    FILE* file;
    assert(snprintf(path, sizeof(path), "%s/%s", directory, name) < (int)sizeof(path));
    file = fopen(path, "wb");
    assert(file);
    assert(fwrite(output, 1, output_length, file) == output_length);
    assert(fclose(file) == 0);
}

int main(int argc, char** argv)
{
    S64DocumentSummary summary;
    uint16_t total;
    unsigned int before;
    char base_url[64];
    assert(argc == 4 && getenv("S64_TEST_KEY"));
    test_port = (uint16_t)strtoul(argv[1], 0, 10);
    http.execute = execute;
    s64_buffer_init(&body);
    s64_document_init(&document, &body);
    assert(s64_direct_publisher_key(getenv("S64_TEST_KEY")));
    sprintf(base_url, "http://127.0.0.1:%u", test_port);
    assert(s64_destination_configure(&destination, S64_DESTINATION_STANDALONE,
        base_url, 1) == S64_DESTINATION_OK);
    assert(s64_destination_connection_url(&destination, url, sizeof(url)) == S64_DESTINATION_OK);
    assert(s64_direct_publisher_test(&http, url, 1) == S64_STANDALONE_OK);

    if (strcmp(argv[2], "create") == 0) {
        fixture(0);
        load_fixture(S64_DOCUMENT_PAGE, "native-page", S64_DOCUMENT_PUBLISHED);
        save_remote();
        open_remote(S64_DOCUMENT_PAGE, "native-page", S64_DOCUMENT_PUBLISHED);
        assert(s64_destination_save_url(&destination, 0, 0, url, sizeof(url)) == S64_DESTINATION_OK);
        assert(s64_standalone_save_document(&http, url, 1, 0, &document) != S64_STANDALONE_OK);
        open_remote(S64_DOCUMENT_PAGE, "native-page", S64_DOCUMENT_PUBLISHED);
        export_source(argv[3], "publisher-small.8tml");
        for (alter_status = 1; alter_status <= 3; ++alter_status) {
            assert(s64_destination_open_url(&destination, S64_DOCUMENT_PAGE,
                "native-page", url, sizeof(url)) == S64_DESTINATION_OK);
            assert(s64_standalone_open_document(&http, url, 1, 0, &document)
                == S64_STANDALONE_PROTOCOL_ERROR);
        }
        alter_status = 0;
        open_remote(S64_DOCUMENT_PAGE, "native-page", S64_DOCUMENT_PUBLISHED);
        fixture(1);
        load_fixture(S64_DOCUMENT_POST, "native-post", S64_DOCUMENT_PUBLISHED);
        save_remote();
        assert(maximum_sent > 65535ul);
        open_remote(S64_DOCUMENT_POST, "native-post", S64_DOCUMENT_PUBLISHED);
        export_source(argv[3], "publisher-large.8tml");
        fixture(2);
        load_fixture(S64_DOCUMENT_PAGE, "empty-page", S64_DOCUMENT_DRAFT);
        save_remote();
        open_remote(S64_DOCUMENT_PAGE, "empty-page", S64_DOCUMENT_DRAFT);
        export_source(argv[3], "publisher-empty.8tml");
        assert(s64_destination_list_url(&destination, S64_DOCUMENT_POST, url, sizeof(url)) == S64_DESTINATION_OK);
        assert(s64_direct_list_summary(&http, url, 1, S64_DOCUMENT_POST, 0, &summary, &total) == S64_STANDALONE_OK);
        assert(total == 1 && strcmp(summary.slug, "native-post") == 0);
        assert(s64_destination_list_url(&destination, S64_DOCUMENT_PAGE, url, sizeof(url)) == S64_DESTINATION_OK);
        assert(s64_direct_list_summary(&http, url, 1, S64_DOCUMENT_PAGE, 0, &summary, &total) == S64_STANDALONE_OK);
        assert(total == 2);
        before = opens;
        s64_buffer_init(&body);
        assert(s64_buffer_insert(&body, 'x'));
        assert(s64_destination_save_url(&destination, 0, 0, url, sizeof(url)) == S64_DESTINATION_OK);
        assert(s64_standalone_save_document(&http, url, 1, 0, &document) == S64_STANDALONE_INVALID_ARGUMENT);
        assert(opens == before);
        fail_open = 1;
        assert(s64_destination_connection_url(&destination, url, sizeof(url)) == S64_DESTINATION_OK);
        assert(s64_direct_publisher_test(&http, url, 1) == S64_STANDALONE_TRANSPORT_ERROR);
    } else if (strcmp(argv[2], "update") == 0) {
        fixture(0);
        load_fixture(S64_DOCUMENT_PAGE, "missing-page", S64_DOCUMENT_PUBLISHED);
        assert(s64_destination_save_url(&destination, document.slug, 1, url, sizeof(url)) == S64_DESTINATION_OK);
        assert(s64_standalone_save_document(&http, url, 1, 0, &document) != S64_STANDALONE_OK);
        fixture(1);
        open_remote(S64_DOCUMENT_POST, "native-post", S64_DOCUMENT_PUBLISHED);
        document.status = S64_DOCUMENT_DRAFT;
        assert(s64_destination_save_url(&destination, document.slug, 1, url, sizeof(url)) == S64_DESTINATION_OK);
        assert(s64_standalone_save_document(&http, url, 1, 0, &document) == S64_STANDALONE_OK);
        open_remote(S64_DOCUMENT_POST, "native-post", S64_DOCUMENT_DRAFT);
    } else {
        assert(strcmp(argv[2], "delete") == 0);
        assert(s64_destination_delete_url(&destination, S64_DOCUMENT_PAGE, "native-page", url, sizeof(url)) == S64_DESTINATION_OK);
        assert(s64_standalone_delete_document(&http, url, 1, 0, S64_DOCUMENT_PAGE, "native-page") == S64_STANDALONE_OK);
        assert(s64_destination_delete_url(&destination, S64_DOCUMENT_PAGE, "empty-page", url, sizeof(url)) == S64_DESTINATION_OK);
        assert(s64_standalone_delete_document(&http, url, 1, 0, S64_DOCUMENT_PAGE, "empty-page") == S64_STANDALONE_OK);
        assert(s64_destination_delete_url(&destination, S64_DOCUMENT_POST, "native-post", url, sizeof(url)) == S64_DESTINATION_OK);
        assert(s64_standalone_delete_document(&http, url, 1, 0, S64_DOCUMENT_POST, "native-post") == S64_STANDALONE_OK);
    }
    assert(opens == closes && connection == -1);
    printf("native 8TML HTTP %s passed; largest request %lu bytes\n", argv[2], maximum_sent);
    return 0;
}
