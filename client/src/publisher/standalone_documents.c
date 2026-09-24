#include "standalone_documents.h"

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
