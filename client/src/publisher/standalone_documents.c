#include "standalone_documents.h"

#include <stdio.h>
#include <string.h>

#define S64_LIST_PATH_CAPACITY 40u

static char document_response_header[S64_HTTP_RESPONSE_HEADER_CAPACITY];
static char list_path[S64_LIST_PATH_CAPACITY];
static char list_type[8];
static char list_status[12];
static S64DocumentSummary list_summary;
static S64DocumentWireReader open_reader;

typedef struct {
    S64HttpTransport* http;
    uint8_t body_handle;
    S64HttpResult result;
} S64WireBodySink;

static uint8_t add_wire_bytes(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64WireBodySink* sink = (S64WireBodySink*)context;

    sink->result = s64_http_body_add_binary(
        sink->http,
        sink->body_handle,
        data,
        length
    );
    return sink->result == S64_HTTP_OK;
}

static S64StandaloneResult map_response_status(uint16_t status_code)
{
    if (status_code == 401u) {
        return S64_STANDALONE_AUTH_FAILED;
    }
    if (status_code < 200u || status_code >= 300u) {
        return status_code == 0u
            ? S64_STANDALONE_PROTOCOL_ERROR
            : S64_STANDALONE_SERVER_ERROR;
    }
    return S64_STANDALONE_OK;
}

static S64StandaloneResult query_list_string(
    S64HttpTransport* http,
    uint8_t body_handle,
    uint16_t index,
    const char* field,
    char* destination,
    uint16_t capacity
)
{
    S64HttpResult result;
    int written = sprintf(list_path, "documents%%%u/%s", (unsigned int)index, field);

    if (written <= 0 || written >= (int)sizeof(list_path)) {
        return S64_STANDALONE_PROTOCOL_ERROR;
    }

    result = s64_http_body_query_string(
        http,
        body_handle,
        list_path,
        destination,
        capacity
    );
    return result == S64_HTTP_OK
        ? S64_STANDALONE_OK
        : s64_standalone_map_http(result);
}

static S64StandaloneResult query_list_summary(
    S64HttpTransport* http,
    uint8_t body_handle,
    uint16_t index
)
{
    S64StandaloneResult result;
    S64HttpResult http_result;
    int32_t order_value;
    int written;

    result = query_list_string(
        http,
        body_handle,
        index,
        "type",
        list_type,
        (uint16_t)sizeof(list_type)
    );
    if (result != S64_STANDALONE_OK
        || !s64_document_type_from_name(list_type, &list_summary.type)) {
        return result == S64_STANDALONE_OK
            ? S64_STANDALONE_PROTOCOL_ERROR
            : result;
    }

    result = query_list_string(
        http,
        body_handle,
        index,
        "slug",
        list_summary.slug,
        (uint16_t)sizeof(list_summary.slug)
    );
    if (result != S64_STANDALONE_OK || !s64_document_slug_valid(list_summary.slug)) {
        return result == S64_STANDALONE_OK
            ? S64_STANDALONE_PROTOCOL_ERROR
            : result;
    }

    result = query_list_string(
        http,
        body_handle,
        index,
        "title",
        list_summary.title,
        (uint16_t)sizeof(list_summary.title)
    );
    if (result != S64_STANDALONE_OK || list_summary.title[0] == '\0') {
        return result == S64_STANDALONE_OK
            ? S64_STANDALONE_PROTOCOL_ERROR
            : result;
    }

    result = query_list_string(
        http,
        body_handle,
        index,
        "status",
        list_status,
        (uint16_t)sizeof(list_status)
    );
    if (result != S64_STANDALONE_OK
        || !s64_document_status_from_name(list_status, &list_summary.status)) {
        return result == S64_STANDALONE_OK
            ? S64_STANDALONE_PROTOCOL_ERROR
            : result;
    }

    result = query_list_string(
        http,
        body_handle,
        index,
        "date",
        list_summary.date,
        (uint16_t)sizeof(list_summary.date)
    );
    if (result != S64_STANDALONE_OK) {
        return result;
    }

    written = sprintf(list_path, "documents%%%u/order", (unsigned int)index);
    if (written <= 0 || written >= (int)sizeof(list_path)) {
        return S64_STANDALONE_PROTOCOL_ERROR;
    }

    http_result = s64_http_body_query_integer(
        http,
        body_handle,
        list_path,
        &order_value
    );
    if (http_result != S64_HTTP_OK) {
        return s64_standalone_map_http(http_result);
    }
    list_summary.order = order_value;
    return S64_STANDALONE_OK;
}

S64StandaloneResult s64_standalone_list_documents(
    S64HttpTransport* http,
    const char* list_url,
    uint8_t allow_insecure_http,
    const S64StandaloneSession* session,
    S64DocumentSummarySink sink,
    void* sink_context,
    uint16_t* total_count,
    uint16_t* delivered_count
)
{
    S64HttpResult http_result;
    S64StandaloneResult result;
    uint8_t request_header = 0u;
    uint8_t response_header_handle = 0u;
    uint8_t response_body_handle = 0u;
    uint8_t request_created = 0u;
    uint8_t response_created = 0u;
    uint16_t response_header_length = 0u;
    uint16_t index;
    uint16_t delivered = 0u;
    int32_t count_value = 0;

    if (total_count != 0) *total_count = 0u;
    if (delivered_count != 0) *delivered_count = 0u;

    if (http == 0 || session == 0 || sink == 0) {
        return S64_STANDALONE_INVALID_ARGUMENT;
    }

    result = s64_standalone_validate_transport(list_url, allow_insecure_http);
    if (result != S64_STANDALONE_OK) {
        return result;
    }

    http_result = s64_http_free_all(http);
    if (http_result != S64_HTTP_OK) {
        return s64_standalone_map_http(http_result);
    }

    http_result = s64_http_header_create(http, S64_HTTP_GET, list_url, &request_header);
    if (http_result != S64_HTTP_OK) {
        return s64_standalone_map_http(http_result);
    }
    request_created = 1u;

    result = s64_standalone_add_authorization(http, request_header, session);
    if (result != S64_STANDALONE_OK) {
        goto cleanup;
    }

    document_response_header[0] = '\0';
    http_result = s64_http_exchange_object(
        http,
        request_header,
        S64_HTTP_NO_BODY,
        &response_header_handle,
        &response_body_handle,
        document_response_header,
        (uint16_t)sizeof(document_response_header),
        &response_header_length
    );
    if (http_result != S64_HTTP_OK) {
        result = s64_standalone_map_http(http_result);
        goto cleanup;
    }
    response_created = 1u;

    result = map_response_status(s64_standalone_http_status(document_response_header));
    if (result != S64_STANDALONE_OK) {
        goto cleanup;
    }

    http_result = s64_http_body_query_integer(
        http,
        response_body_handle,
        "count",
        &count_value
    );
    if (http_result != S64_HTTP_OK || count_value < 0 || count_value > 65535L) {
        result = http_result == S64_HTTP_OK
            ? S64_STANDALONE_PROTOCOL_ERROR
            : s64_standalone_map_http(http_result);
        goto cleanup;
    }

    if (total_count != 0) {
        *total_count = (uint16_t)count_value;
    }

    for (index = 0u; index < (uint16_t)count_value; ++index) {
        result = query_list_summary(http, response_body_handle, index);
        if (result != S64_STANDALONE_OK) {
            goto cleanup;
        }

        ++delivered;
        if (!sink(&list_summary, index, sink_context)) {
            break;
        }
    }

    result = S64_STANDALONE_OK;

cleanup:
    if (delivered_count != 0) {
        *delivered_count = delivered;
    }
    if (response_created) {
        (void)s64_http_body_free(http, response_body_handle);
        (void)s64_http_header_free(http, response_header_handle);
    }
    if (request_created) {
        (void)s64_http_header_free(http, request_header);
    }
    return result;
}

S64StandaloneResult s64_standalone_open_document(
    S64HttpTransport* http,
    const char* open_url,
    uint8_t allow_insecure_http,
    const S64StandaloneSession* session,
    S64Document* document
)
{
    S64HttpResult http_result;
    S64StandaloneResult result;
    S64DocumentResult document_result;
    uint8_t header_handle = 0u;
    uint8_t header_created = 0u;
    uint16_t response_header_length = 0u;

    if (http == 0 || session == 0 || document == 0 || document->body == 0) {
        return S64_STANDALONE_INVALID_ARGUMENT;
    }

    result = s64_standalone_validate_transport(open_url, allow_insecure_http);
    if (result != S64_STANDALONE_OK) {
        return result;
    }

    http_result = s64_http_free_all(http);
    if (http_result != S64_HTTP_OK) {
        return s64_standalone_map_http(http_result);
    }

    http_result = s64_http_header_create(http, S64_HTTP_GET, open_url, &header_handle);
    if (http_result != S64_HTTP_OK) {
        return s64_standalone_map_http(http_result);
    }
    header_created = 1u;

    result = s64_standalone_add_authorization(http, header_handle, session);
    if (result != S64_STANDALONE_OK) {
        goto cleanup;
    }

    s64_document_wire_reader_init(&open_reader, document);
    document_response_header[0] = '\0';

    http_result = s64_http_exchange_raw(
        http,
        header_handle,
        S64_HTTP_NO_BODY,
        s64_document_wire_reader_write,
        &open_reader,
        document_response_header,
        (uint16_t)sizeof(document_response_header),
        &response_header_length
    );

    result = map_response_status(s64_standalone_http_status(document_response_header));
    if (result != S64_STANDALONE_OK) {
        (void)s64_document_wire_reader_finish(&open_reader);
        s64_buffer_init(document->body);
        s64_document_init(document, document->body);
        goto cleanup;
    }

    if (http_result != S64_HTTP_OK) {
        document_result = s64_document_wire_reader_finish(&open_reader);
        if (document_result == S64_DOCUMENT_SINK_ERROR) {
            result = S64_STANDALONE_RESPONSE_TOO_LARGE;
        } else if (document_result != S64_DOCUMENT_OK) {
            result = S64_STANDALONE_PROTOCOL_ERROR;
        } else {
            result = s64_standalone_map_http(http_result);
            s64_buffer_init(document->body);
            s64_document_init(document, document->body);
        }
        goto cleanup;
    }

    document_result = s64_document_wire_reader_finish(&open_reader);
    if (document_result == S64_DOCUMENT_SINK_ERROR) {
        result = S64_STANDALONE_RESPONSE_TOO_LARGE;
    } else if (document_result != S64_DOCUMENT_OK) {
        result = S64_STANDALONE_PROTOCOL_ERROR;
    } else {
        result = S64_STANDALONE_OK;
    }

cleanup:
    if (header_created) {
        (void)s64_http_header_free(http, header_handle);
    }
    return result;
}

S64StandaloneResult s64_standalone_save_document(
    S64HttpTransport* http,
    const char* save_url,
    uint8_t allow_insecure_http,
    const S64StandaloneSession* session,
    const S64Document* document
)
{
    S64HttpResult http_result;
    S64StandaloneResult result;
    S64DocumentResult document_result;
    S64WireBodySink wire_sink;
    uint8_t header_handle = 0u;
    uint8_t body_handle = 0u;
    uint8_t header_created = 0u;
    uint8_t body_created = 0u;

    if (http == 0 || document == 0) {
        return S64_STANDALONE_INVALID_ARGUMENT;
    }

    result = s64_standalone_validate_transport(save_url, allow_insecure_http);
    if (result != S64_STANDALONE_OK) {
        return result;
    }

    http_result = s64_http_free_all(http);
    if (http_result != S64_HTTP_OK) {
        return s64_standalone_map_http(http_result);
    }

    http_result = s64_http_header_create(http, S64_HTTP_POST, save_url, &header_handle);
    if (http_result != S64_HTTP_OK) {
        return s64_standalone_map_http(http_result);
    }
    header_created = 1u;

    result = s64_standalone_add_authorization(http, header_handle, session);
    if (result != S64_STANDALONE_OK) {
        goto cleanup;
    }

    http_result = s64_http_header_add(
        http,
        header_handle,
        "Content-Type: application/x-see64you"
    );
    if (http_result != S64_HTTP_OK) {
        result = s64_standalone_map_http(http_result);
        goto cleanup;
    }

    http_result = s64_http_body_create(http, S64_HTTP_BODY_BINARY, &body_handle);
    if (http_result != S64_HTTP_OK) {
        result = s64_standalone_map_http(http_result);
        goto cleanup;
    }
    body_created = 1u;

    wire_sink.http = http;
    wire_sink.body_handle = body_handle;
    wire_sink.result = S64_HTTP_OK;

    document_result = s64_document_write_wire(document, add_wire_bytes, &wire_sink);
    if (document_result == S64_DOCUMENT_INVALID_ARGUMENT
        || document_result == S64_DOCUMENT_INVALID_METADATA) {
        result = S64_STANDALONE_INVALID_ARGUMENT;
        goto cleanup;
    }
    if (document_result != S64_DOCUMENT_OK) {
        result = s64_standalone_map_http(wire_sink.result);
        goto cleanup;
    }

    result = s64_standalone_exchange_ack(http, header_handle, body_handle);

cleanup:
    if (body_created) {
        (void)s64_http_body_free(http, body_handle);
    }
    if (header_created) {
        (void)s64_http_header_free(http, header_handle);
    }
    return result;
}

S64StandaloneResult s64_standalone_delete_document(
    S64HttpTransport* http,
    const char* delete_url,
    uint8_t allow_insecure_http,
    const S64StandaloneSession* session,
    S64DocumentType type,
    const char* slug
)
{
    const char* type_name = s64_document_type_name(type);
    S64HttpResult http_result;
    S64StandaloneResult result;
    uint8_t header_handle = 0u;
    uint8_t body_handle = 0u;
    uint8_t header_created = 0u;
    uint8_t body_created = 0u;

    if (http == 0 || type_name == 0 || !s64_document_slug_valid(slug)) {
        return S64_STANDALONE_INVALID_ARGUMENT;
    }

    result = s64_standalone_validate_transport(delete_url, allow_insecure_http);
    if (result != S64_STANDALONE_OK) {
        return result;
    }

    http_result = s64_http_free_all(http);
    if (http_result != S64_HTTP_OK) {
        return s64_standalone_map_http(http_result);
    }

    http_result = s64_http_header_create(http, S64_HTTP_POST, delete_url, &header_handle);
    if (http_result != S64_HTTP_OK) {
        return s64_standalone_map_http(http_result);
    }
    header_created = 1u;

    result = s64_standalone_add_authorization(http, header_handle, session);
    if (result != S64_STANDALONE_OK) {
        goto cleanup;
    }

    http_result = s64_http_body_create(
        http,
        S64_HTTP_BODY_URL_ENCODED,
        &body_handle
    );
    if (http_result != S64_HTTP_OK) {
        result = s64_standalone_map_http(http_result);
        goto cleanup;
    }
    body_created = 1u;

    http_result = s64_http_body_add_string(http, body_handle, "type", type_name);
    if (http_result == S64_HTTP_OK) {
        http_result = s64_http_body_add_string(http, body_handle, "slug", slug);
    }
    if (http_result != S64_HTTP_OK) {
        result = s64_standalone_map_http(http_result);
        goto cleanup;
    }

    result = s64_standalone_exchange_ack(http, header_handle, body_handle);

cleanup:
    if (body_created) {
        (void)s64_http_body_free(http, body_handle);
    }
    if (header_created) {
        (void)s64_http_header_free(http, header_handle);
    }
    return result;
}
