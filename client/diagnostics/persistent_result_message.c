#include "publisher/standalone.h"

const char* s64_standalone_result_message(S64StandaloneResult result)
{
    switch (result) {
        case S64_STANDALONE_OK: return "OK";
        case S64_STANDALONE_AUTH_FAILED: return "Direct key missing or denied";
        case S64_STANDALONE_INSECURE_TRANSPORT: return "Enable plain HTTP";
        case S64_STANDALONE_TLS_UNSUPPORTED: return "Direct mode needs http:// URL";
        case S64_STANDALONE_TRANSPORT_ERROR: return "Network/UCI error";
        case S64_STANDALONE_SERVER_ERROR: return "Server rejected request";
        case S64_STANDALONE_INVALID_ARGUMENT: return "Invalid request/site URL";
        default: return "Unexpected server reply";
    }
}
