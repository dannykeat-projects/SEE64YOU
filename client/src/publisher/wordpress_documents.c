#include "wordpress_documents.h"

#include <stdio.h>
#include <string.h>

#define S64_WP_PATH_CAPACITY 64u
#define S64_WP_TYPE_CAPACITY 8u
#define S64_WP_STATUS_CAPACITY 12u
#define S64_WP_ENCODING_CAPACITY 20u
#define S64_WP_SOURCE_CHUNK_CAPACITY 241u
#define S64_WP_SOURCE_CHUNK_LIMIT 64u

static char response_header[S64_HTTP_RESPONSE_HEADER_CAPACITY];
static char query_path[S64_WP_PATH_CAPACITY];
static char query_type[S64_WP_TYPE_CAPACITY];
static char query_status[S64_WP_STATUS_CAPACITY];
static char query_encoding[S64_WP_ENCODING_CAPACITY];
static char source_chunk[S64_WP_SOURCE_CHUNK_CAPACITY];
static S64WordPressDocumentSummary summary_scratch;

typedef struct {
    S64HttpTransport* http;
    uint8_t body_handle;
    S64HttpResult result;
} S64WordPressWireSink;

static S64WordPressResult map_status(uint16_t status)
{
    if (status >= 200u && status < 300u) return S64_WORDPRESS_OK;
    if (status == 401u) return S64_WORDPRESS_AUTH_FAILED;
    if (status == 403u) return S64_WORDPRESS_PERMISSION_DENIED;
    if (status == 400u) return S64_WORDPRESS_DOCUMENT_REJECTED;
    if (status == 404u) return S64_WORDPRESS_NOT_FOUND;
    if (status == 409u) return S64_WORDPRESS_SLUG_CONFLICT;
    if (status == 413u || status == 422u) return S64_WORDPRESS_INCOMPATIBLE_DOCUMENT;
    if (status == 0u) return S64_WORDPRESS_PROTOCOL_ERROR;
    return S64_WORDPRESS_SERVER_ERROR;
}

static uint8_t id_valid(const char* id)
{
    uint16_t i = 0u;

    if (id == 0 || id[0] == '\0') return 0u;
    while (id[i] != '\0') {
        if (i + 1u >= S64_WORDPRESS_ID_CAPACITY
            || id[i] < '0' || id[i] > '9') {
            return 0u;
        }
        ++i;
    }
    return 1u;
}

static uint8_t parse_type(const char* text, S64DocumentType* type)
{
    if (strcmp(text, "page") == 0) {
        *type = S64_DOCUMENT_PAGE;
        return 1u;
    }
    if (strcmp(text, "post") == 0) {
        *type = S64_DOCUMENT_POST;
        return 1u;
    }
    return 0u;
}

static uint8_t parse_status(const char* text, S64DocumentStatus* status)
{
    if (strcmp(text, "draft") == 0) {
        *status = S64_DOCUMENT_DRAFT;
        return 1u;
    }
    if (strcmp(text, "published") == 0) {
        *status = S64_DOCUMENT_PUBLISHED;
        return 1u;
    }
    return 0u;
}

static S64WordPressResult query_string(
    S64HttpTransport* http,
    uint8_t body_handle,
    const char* path,
    char* destination,
    uint16_t capacity
)
{
    S64HttpResult result = s64_http_body_query_string(
        http,
        body_handle,
        path,
        destination,
        capacity
    );
    return result == S64_HTTP_OK
        ? S64_WORDPRESS_OK
        : s64_wordpress_map_http(result);
}

static S64WordPressResult query_integer(
    S64HttpTransport* http,
    uint8_t body_handle,
    const char* path,
    int32_t* value
)
{
    S64HttpResult result = s64_http_body_query_integer(
        http,
        body_handle,
        path,
        value
    );
    return result == S64_HTTP_OK
        ? S64_WORDPRESS_OK
        : s64_wordpress_map_http(result);
}

static S64WordPressResult query_summary(
    S64HttpTransport* http,
    uint8_t body_handle,
    uint16_t index
)
{
    S64WordPressResult result;
    int32_t order;
    int written;

#define QUERY_SUMMARY_STRING(field, destination) \
    do { \
        written = sprintf(query_path, "documents%%%u/" field, (unsigned int)index); \
        if (written <= 0 || written >= (int)sizeof(query_path)) \
            return S64_WORDPRESS_PROTOCOL_ERROR; \
        result = query_string( \
            http, body_handle, query_path, destination, \
            (uint16_t)sizeof(destination) \
        ); \
        if (result != S64_WORDPRESS_OK) return result; \
    } while (0)

    QUERY_SUMMARY_STRING("id_text", summary_scratch.id);
    if (!id_valid(summary_scratch.id)) return S64_WORDPRESS_PROTOCOL_ERROR;

    QUERY_SUMMARY_STRING("type", query_type);
    if (!parse_type(query_type, &summary_scratch.type)) {
        return S64_WORDPRESS_PROTOCOL_ERROR;
    }

    QUERY_SUMMARY_STRING("slug", summary_scratch.slug);
    if (!s64_document_slug_valid(summary_scratch.slug)) {
        return S64_WORDPRESS_INCOMPATIBLE_DOCUMENT;
    }

    QUERY_SUMMARY_STRING("title", summary_scratch.title);
    if (summary_scratch.title[0] == '\0') {
        return S64_WORDPRESS_PROTOCOL_ERROR;
    }

    QUERY_SUMMARY_STRING("status", query_status);
    if (!parse_status(query_status, &summary_scratch.status)) {
        return S64_WORDPRESS_PROTOCOL_ERROR;
    }

    QUERY_SUMMARY_STRING("date", summary_scratch.date);

    written = sprintf(query_path, "documents%%%u/order", (unsigned int)index);
    if (written <= 0 || written >= (int)sizeof(query_path)) {
        return S64_WORDPRESS_PROTOCOL_ERROR;
    }
    result = query_integer(http, body_handle, query_path, &order);
    if (result != S64_WORDPRESS_OK) return result;
    summary_scratch.order = order;

#undef QUERY_SUMMARY_STRING
    return S64_WORDPRESS_OK;
}

static void reset_document(S64Document* document)
{
    S64Buffer* body;

    if (document == 0 || document->body == 0) return;
    body = document->body;
    s64_buffer_init(body);
    s64_document_init(document, body);
}

static int base64_value(char value)
{
    if (value >= 'A' && value <= 'Z') return value - 'A';
    if (value >= 'a' && value <= 'z') return value - 'a' + 26;
    if (value >= '0' && value <= '9') return value - '0' + 52;
    if (value == '+') return 62;
    if (value == '/') return 63;
    return -1;
}

static S64WordPressResult append_base64_chunk(
    S64Buffer* body,
    const char* encoded,
    uint16_t* decoded_total
)
{
    uint16_t length = 0u;
    uint16_t i;
    int a;
    int b;
    int c;
    int d;
    uint8_t out[3];
    uint8_t out_count;

    while (encoded[length] != '\0') {
        if (length + 1u >= S64_WP_SOURCE_CHUNK_CAPACITY) {
            return S64_WORDPRESS_PROTOCOL_ERROR;
        }
        ++length;
    }

    if (length == 0u || (length & 3u) != 0u) {
        return S64_WORDPRESS_PROTOCOL_ERROR;
    }

    for (i = 0u; i < length; i = (uint16_t)(i + 4u)) {
        a = base64_value(encoded[i]);
        b = base64_value(encoded[i + 1u]);
        c = encoded[i + 2u] == '=' ? -2 : base64_value(encoded[i + 2u]);
        d = encoded[i + 3u] == '=' ? -2 : base64_value(encoded[i + 3u]);

        if (a < 0 || b < 0 || c == -1 || d == -1) {
            return S64_WORDPRESS_PROTOCOL_ERROR;
        }
        if (c == -2 && d != -2) {
            return S64_WORDPRESS_PROTOCOL_ERROR;
        }
        if ((c == -2 || d == -2) && i + 4u != length) {
            return S64_WORDPRESS_PROTOCOL_ERROR;
        }
        if (c == -2 && (b & 0x0f) != 0) {
            return S64_WORDPRESS_PROTOCOL_ERROR;
        }
        if (c >= 0 && d == -2 && (c & 0x03) != 0) {
            return S64_WORDPRESS_PROTOCOL_ERROR;
        }

        out[0] = (uint8_t)((a << 2) | (b >> 4));
        out_count = 1u;

        if (c >= 0) {
            out[1] = (uint8_t)(((b & 0x0f) << 4) | (c >> 2));
            out_count = 2u;
            if (d >= 0) {
                out[2] = (uint8_t)(((c & 0x03) << 6) | d);
                out_count = 3u;
            }
        }

        if ((uint32_t)*decoded_total + out_count > S64_BUFFER_CAPACITY) {
            return S64_WORDPRESS_INCOMPATIBLE_DOCUMENT;
        }

        if (!s64_buffer_insert(body, out[0])
            || (out_count > 1u && !s64_buffer_insert(body, out[1]))
            || (out_count > 2u && !s64_buffer_insert(body, out[2]))) {
            return S64_WORDPRESS_INCOMPATIBLE_DOCUMENT;
        }
        *decoded_total = (uint16_t)(*decoded_total + out_count);
    }

    return S64_WORDPRESS_OK;
}

static S64WordPressResult apply_open_metadata(
    S64HttpTransport* http,
    uint8_t body_handle,
    S64Document* document
)
{
    S64WordPressResult result;
    int32_t order;

    result = query_string(
        http, body_handle, "document/type",
        query_type, (uint16_t)sizeof(query_type)
    );
    if (result != S64_WORDPRESS_OK
        || !parse_type(query_type, &document->type)) {
        return result == S64_WORDPRESS_OK ? S64_WORDPRESS_PROTOCOL_ERROR : result;
    }

    result = query_string(
        http, body_handle, "document/slug",
        document->slug, (uint16_t)sizeof(document->slug)
    );
    if (result != S64_WORDPRESS_OK) return result;
    if (!s64_document_slug_valid(document->slug)) {
        return S64_WORDPRESS_INCOMPATIBLE_DOCUMENT;
    }

    result = query_string(
        http, body_handle, "document/title",
        document->title, (uint16_t)sizeof(document->title)
    );
    if (result != S64_WORDPRESS_OK) return result;
    if (document->title[0] == '\0') return S64_WORDPRESS_PROTOCOL_ERROR;

    result = query_string(
        http, body_handle, "document/status",
        query_status, (uint16_t)sizeof(query_status)
    );
    if (result != S64_WORDPRESS_OK
        || !parse_status(query_status, &document->status)) {
        return result == S64_WORDPRESS_OK ? S64_WORDPRESS_PROTOCOL_ERROR : result;
    }

    result = query_string(
        http, body_handle, "document/date",
        document->date, (uint16_t)sizeof(document->date)
    );
    if (result != S64_WORDPRESS_OK) return result;

    result = query_integer(http, body_handle, "document/order", &order);
    if (result != S64_WORDPRESS_OK) return result;
    document->order = order;
    return S64_WORDPRESS_OK;
}

S64WordPressResult s64_wordpress_list_documents(
    S64HttpTransport* http,
    const char* list_url,
    uint8_t allow_insecure_http,
    const S64WordPressSession* session,
    S64WordPressDocumentSummarySink sink,
    void* sink_context,
    uint16_t* total_count,
    uint16_t* delivered_count
)
{
    S64HttpResult http_result;
    S64WordPressResult result;
    uint8_t request_header = 0u;
    uint8_t response_header_handle = 0u;
    uint8_t response_body_handle = 0u;
    uint8_t request_created = 0u;
    uint8_t response_created = 0u;
    int32_t count = 0;
    uint16_t index;
    uint16_t delivered = 0u;

    if (total_count != 0) *total_count = 0u;
    if (delivered_count != 0) *delivered_count = 0u;
    if (http == 0 || session == 0 || sink == 0) {
        return S64_WORDPRESS_INVALID_ARGUMENT;
    }

    result = s64_wordpress_validate_transport(list_url, allow_insecure_http);
    if (result != S64_WORDPRESS_OK) return result;

    http_result = s64_http_free_all(http);
    if (http_result != S64_HTTP_OK) return s64_wordpress_map_http(http_result);

    http_result = s64_http_header_create(http, S64_HTTP_GET, list_url, &request_header);
    if (http_result != S64_HTTP_OK) return s64_wordpress_map_http(http_result);
    request_created = 1u;

    result = s64_wordpress_add_authorization(http, request_header, session);
    if (result != S64_WORDPRESS_OK) goto cleanup;

    response_header[0] = '\0';
    http_result = s64_http_exchange_object(
        http, request_header, S64_HTTP_NO_BODY,
        &response_header_handle, &response_body_handle,
        response_header, (uint16_t)sizeof(response_header), 0
    );
    if (http_result != S64_HTTP_OK) {
        result = s64_wordpress_map_http(http_result);
        goto cleanup;
    }
    response_created = 1u;

    result = map_status(s64_wordpress_http_status(response_header));
    if (result != S64_WORDPRESS_OK) goto cleanup;

    result = query_integer(http, response_body_handle, "count", &count);
    if (result != S64_WORDPRESS_OK) goto cleanup;
    if (count < 0 || count > 65535L) {
        result = S64_WORDPRESS_PROTOCOL_ERROR;
        goto cleanup;
    }

    if (total_count != 0) *total_count = (uint16_t)count;

    for (index = 0u; index < (uint16_t)count; ++index) {
        result = query_summary(http, response_body_handle, index);
        if (result != S64_WORDPRESS_OK) goto cleanup;
        ++delivered;
        if (!sink(&summary_scratch, index, sink_context)) break;
    }

    result = S64_WORDPRESS_OK;

cleanup:
    if (delivered_count != 0) *delivered_count = delivered;
    if (response_created) {
        (void)s64_http_body_free(http, response_body_handle);
        (void)s64_http_header_free(http, response_header_handle);
    }
    if (request_created) {
        (void)s64_http_header_free(http, request_header);
    }
    return result;
}

S64WordPressResult s64_wordpress_open_document(
    S64HttpTransport* http,
    const char* open_url,
    uint8_t allow_insecure_http,
    const S64WordPressSession* session,
    S64Document* document
)
{
    S64HttpResult http_result;
    S64WordPressResult result;
    uint8_t request_header = 0u;
    uint8_t response_header_handle = 0u;
    uint8_t response_body_handle = 0u;
    uint8_t request_created = 0u;
    uint8_t response_created = 0u;
    int32_t source_length = 0;
    int32_t source_count = 0;
    uint16_t decoded_total = 0u;
    uint16_t index;
    uint8_t destination_started = 0u;
    int written;

    if (http == 0 || session == 0 || document == 0 || document->body == 0) {
        return S64_WORDPRESS_INVALID_ARGUMENT;
    }

    result = s64_wordpress_validate_transport(open_url, allow_insecure_http);
    if (result != S64_WORDPRESS_OK) return result;

    http_result = s64_http_free_all(http);
    if (http_result != S64_HTTP_OK) return s64_wordpress_map_http(http_result);

    http_result = s64_http_header_create(http, S64_HTTP_GET, open_url, &request_header);
    if (http_result != S64_HTTP_OK) return s64_wordpress_map_http(http_result);
    request_created = 1u;

    result = s64_wordpress_add_authorization(http, request_header, session);
    if (result != S64_WORDPRESS_OK) goto cleanup;

    response_header[0] = '\0';
    http_result = s64_http_exchange_object(
        http, request_header, S64_HTTP_NO_BODY,
        &response_header_handle, &response_body_handle,
        response_header, (uint16_t)sizeof(response_header), 0
    );
    if (http_result != S64_HTTP_OK) {
        result = s64_wordpress_map_http(http_result);
        goto cleanup;
    }
    response_created = 1u;

    result = map_status(s64_wordpress_http_status(response_header));
    if (result != S64_WORDPRESS_OK) goto cleanup;

    reset_document(document);
    destination_started = 1u;
    result = apply_open_metadata(http, response_body_handle, document);
    if (result != S64_WORDPRESS_OK) goto cleanup;

    result = query_string(
        http, response_body_handle, "document/source_encoding",
        query_encoding, (uint16_t)sizeof(query_encoding)
    );
    if (result != S64_WORDPRESS_OK) goto cleanup;
    if (strcmp(query_encoding, "base64-chunks") != 0) {
        result = S64_WORDPRESS_PROTOCOL_ERROR;
        goto cleanup;
    }

    result = query_integer(
        http, response_body_handle, "document/source_length", &source_length
    );
    if (result != S64_WORDPRESS_OK) goto cleanup;
    result = query_integer(
        http, response_body_handle, "document/source_count", &source_count
    );
    if (result != S64_WORDPRESS_OK) goto cleanup;

    if (source_length < 0 || source_length > (int32_t)S64_BUFFER_CAPACITY
        || source_count < 0 || source_count > S64_WP_SOURCE_CHUNK_LIMIT) {
        result = S64_WORDPRESS_INCOMPATIBLE_DOCUMENT;
        goto cleanup;
    }
    if (source_count != (source_length == 0
            ? 0
            : (source_length + 179L) / 180L)) {
        result = S64_WORDPRESS_PROTOCOL_ERROR;
        goto cleanup;
    }

    for (index = 0u; index < (uint16_t)source_count; ++index) {
        written = sprintf(query_path, "document/source_chunks%%%u", (unsigned int)index);
        if (written <= 0 || written >= (int)sizeof(query_path)) {
            result = S64_WORDPRESS_PROTOCOL_ERROR;
            goto cleanup;
        }

        result = query_string(
            http, response_body_handle, query_path,
            source_chunk, (uint16_t)sizeof(source_chunk)
        );
        if (result != S64_WORDPRESS_OK) goto cleanup;

        {
            uint16_t before = decoded_total;
            uint16_t expected_chunk = (uint16_t)(
                source_length - (int32_t)index * 180L
            );
            if (expected_chunk > 180u) expected_chunk = 180u;

            result = append_base64_chunk(document->body, source_chunk, &decoded_total);
            if (result != S64_WORDPRESS_OK) goto cleanup;
            if ((uint16_t)(decoded_total - before) != expected_chunk) {
                result = S64_WORDPRESS_PROTOCOL_ERROR;
                goto cleanup;
            }
        }
    }

    if (decoded_total != (uint16_t)source_length) {
        result = S64_WORDPRESS_PROTOCOL_ERROR;
        goto cleanup;
    }

    (void)s64_buffer_move_cursor(document->body, 0u);
    result = S64_WORDPRESS_OK;

cleanup:
    if (result != S64_WORDPRESS_OK && destination_started) reset_document(document);
    if (response_created) {
        (void)s64_http_body_free(http, response_body_handle);
        (void)s64_http_header_free(http, response_header_handle);
    }
    if (request_created) (void)s64_http_header_free(http, request_header);
    return result;
}

static uint8_t wire_sink(const uint8_t* data, uint16_t length, void* context)
{
    S64WordPressWireSink* sink = (S64WordPressWireSink*)context;

    sink->result = s64_http_body_add_binary(
        sink->http,
        sink->body_handle,
        data,
        length
    );
    return sink->result == S64_HTTP_OK;
}

S64WordPressResult s64_wordpress_save_document(
    S64HttpTransport* http,
    const char* save_url,
    uint8_t allow_insecure_http,
    const S64WordPressSession* session,
    const S64Document* document,
    uint8_t updating,
    char* id,
    uint16_t id_capacity
)
{
    static const char content_type[] = "Content-Type: application/x-see64you";
    S64HttpResult http_result;
    S64WordPressResult result;
    S64DocumentResult document_result;
    S64WordPressWireSink sink;
    uint8_t request_header = 0u;
    uint8_t request_body = 0u;
    uint8_t response_header_handle = 0u;
    uint8_t response_body_handle = 0u;
    uint8_t request_header_created = 0u;
    uint8_t request_body_created = 0u;
    uint8_t response_created = 0u;

    if (http == 0 || session == 0 || document == 0
        || id == 0 || id_capacity == 0u) {
        return S64_WORDPRESS_INVALID_ARGUMENT;
    }
    id[0] = '\0';

    result = s64_wordpress_validate_transport(save_url, allow_insecure_http);
    if (result != S64_WORDPRESS_OK) return result;

    http_result = s64_http_free_all(http);
    if (http_result != S64_HTTP_OK) return s64_wordpress_map_http(http_result);

    http_result = s64_http_header_create(
        http,
        updating ? S64_HTTP_PATCH : S64_HTTP_POST,
        save_url,
        &request_header
    );
    if (http_result != S64_HTTP_OK) return s64_wordpress_map_http(http_result);
    request_header_created = 1u;

    result = s64_wordpress_add_authorization(http, request_header, session);
    if (result != S64_WORDPRESS_OK) goto cleanup;

    http_result = s64_http_header_add(http, request_header, content_type);
    if (http_result != S64_HTTP_OK) {
        result = s64_wordpress_map_http(http_result);
        goto cleanup;
    }

    http_result = s64_http_body_create(http, S64_HTTP_BODY_BINARY, &request_body);
    if (http_result != S64_HTTP_OK) {
        result = s64_wordpress_map_http(http_result);
        goto cleanup;
    }
    request_body_created = 1u;

    sink.http = http;
    sink.body_handle = request_body;
    sink.result = S64_HTTP_OK;
    document_result = s64_document_write_wire(document, wire_sink, &sink);
    if (document_result == S64_DOCUMENT_INVALID_ARGUMENT
        || document_result == S64_DOCUMENT_INVALID_METADATA) {
        result = S64_WORDPRESS_INVALID_ARGUMENT;
        goto cleanup;
    }
    if (document_result != S64_DOCUMENT_OK) {
        result = s64_wordpress_map_http(sink.result);
        if (result == S64_WORDPRESS_OK) {
            result = S64_WORDPRESS_PROTOCOL_ERROR;
        }
        goto cleanup;
    }

    response_header[0] = '\0';
    http_result = s64_http_exchange_object(
        http, request_header, request_body,
        &response_header_handle, &response_body_handle,
        response_header, (uint16_t)sizeof(response_header), 0
    );
    if (http_result != S64_HTTP_OK) {
        result = s64_wordpress_map_http(http_result);
        goto cleanup;
    }
    response_created = 1u;

    result = map_status(s64_wordpress_http_status(response_header));
    if (result != S64_WORDPRESS_OK) goto cleanup;

    result = query_string(
        http, response_body_handle, "id_text", id, id_capacity
    );
    if (result != S64_WORDPRESS_OK) goto cleanup;
    if (!id_valid(id)) {
        result = S64_WORDPRESS_PROTOCOL_ERROR;
        id[0] = '\0';
        goto cleanup;
    }

    result = S64_WORDPRESS_OK;

cleanup:
    if (response_created) {
        (void)s64_http_body_free(http, response_body_handle);
        (void)s64_http_header_free(http, response_header_handle);
    }
    if (request_body_created) (void)s64_http_body_free(http, request_body);
    if (request_header_created) (void)s64_http_header_free(http, request_header);
    if (result != S64_WORDPRESS_OK) id[0] = '\0';
    return result;
}

S64WordPressResult s64_wordpress_delete_document(
    S64HttpTransport* http,
    const char* delete_url,
    uint8_t allow_insecure_http,
    const S64WordPressSession* session
)
{
    S64HttpResult http_result;
    S64WordPressResult result;
    uint8_t request_header = 0u;
    uint8_t response_header_handle = 0u;
    uint8_t response_body_handle = 0u;
    uint8_t request_created = 0u;
    uint8_t response_created = 0u;
    int32_t deleted = 0;

    if (http == 0 || session == 0) return S64_WORDPRESS_INVALID_ARGUMENT;

    result = s64_wordpress_validate_transport(delete_url, allow_insecure_http);
    if (result != S64_WORDPRESS_OK) return result;

    http_result = s64_http_free_all(http);
    if (http_result != S64_HTTP_OK) return s64_wordpress_map_http(http_result);

    http_result = s64_http_header_create(
        http, S64_HTTP_DELETE, delete_url, &request_header
    );
    if (http_result != S64_HTTP_OK) return s64_wordpress_map_http(http_result);
    request_created = 1u;

    result = s64_wordpress_add_authorization(http, request_header, session);
    if (result != S64_WORDPRESS_OK) goto cleanup;

    response_header[0] = '\0';
    http_result = s64_http_exchange_object(
        http, request_header, S64_HTTP_NO_BODY,
        &response_header_handle, &response_body_handle,
        response_header, (uint16_t)sizeof(response_header), 0
    );
    if (http_result != S64_HTTP_OK) {
        result = s64_wordpress_map_http(http_result);
        goto cleanup;
    }
    response_created = 1u;

    result = map_status(s64_wordpress_http_status(response_header));
    if (result != S64_WORDPRESS_OK) goto cleanup;

    result = query_integer(http, response_body_handle, "deleted_int", &deleted);
    if (result != S64_WORDPRESS_OK) goto cleanup;
    result = deleted == 1 ? S64_WORDPRESS_OK : S64_WORDPRESS_PROTOCOL_ERROR;

cleanup:
    if (response_created) {
        (void)s64_http_body_free(http, response_body_handle);
        (void)s64_http_header_free(http, response_header_handle);
    }
    if (request_created) (void)s64_http_header_free(http, request_header);
    return result;
}
