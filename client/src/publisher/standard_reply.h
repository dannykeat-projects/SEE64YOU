#ifndef SEE64YOU_STANDARD_REPLY_H
#define SEE64YOU_STANDARD_REPLY_H

#include <stdint.h>

#define S64_PUBLISH_REPLY_LINE_CAPACITY 256u
#define S64_PUBLISH_REPLY_MAX_BYTES 65535u

typedef int (*S64PublishReplyLine)(const char* key,
    const char* value, void* context);

typedef struct {
    char line[S64_PUBLISH_REPLY_LINE_CAPACITY];
    uint16_t length;
    uint32_t total;
    uint8_t saw_status;
    uint8_t is_error;
    uint8_t failed;
} S64PublishReplyReader;

/* Feed ASCII response body bytes, not the HTTP headers.
   Passes subsequent key/value lines as native text to the caller. */
void s64_publish_reply_init(S64PublishReplyReader* reader);
int s64_publish_reply_feed(S64PublishReplyReader* reader,
    const uint8_t* bytes, uint16_t length,
    S64PublishReplyLine on_line, void* context);
int s64_publish_reply_finish(const S64PublishReplyReader* reader);

#endif
