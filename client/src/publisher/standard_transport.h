#ifndef SEE64YOU_STANDARD_TRANSPORT_H
#define SEE64YOU_STANDARD_TRANSPORT_H
#include "platform/network.h"
typedef enum {
 S64_STANDARD_OK=0, S64_STANDARD_INVALID, S64_STANDARD_TLS,
 S64_STANDARD_CONSENT, S64_STANDARD_NETWORK, S64_STANDARD_PROTOCOL,
 S64_STANDARD_SERVER, S64_STANDARD_TOO_LARGE, S64_STANDARD_UNCERTAIN
} S64StandardResult;
typedef struct {
 S64NetTransport network;
 const char *url, *id, *key;
 uint8_t allow_http;
} S64StandardClient;
typedef struct {
 uint32_t max_size, size;
 uint8_t exists, warning;
 char name[65], url[193], message[81];
} S64StandardReply;
/* Writer emits exact source bytes once. Never retries an ambiguous write.
   Caller lists and confirms replacement before put; API 1 has no atomic
   create/replace protection. */
typedef int (*S64StandardWriter)(S64UciDataSink sink, void* sink_context, void* context);
S64StandardResult s64_standard_status(S64StandardClient*, S64StandardReply*);
S64StandardResult s64_standard_list(S64StandardClient*, const char*, S64StandardReply*);
S64StandardResult s64_standard_put(S64StandardClient*, const char*, uint32_t,
 S64StandardWriter, void*, S64StandardReply*);
#endif
