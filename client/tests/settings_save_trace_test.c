#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../diagnostics/settings_save_trace.h"

static const S64UciCommandPart* expected_parts;
static S64UciReply* expected_reply;
static const char* next_status;
static S64UciResult next_result;
static unsigned int calls;

S64UciResult s64_uci_execute(const S64UciCommandPart* parts,
    uint8_t count, S64UciReply* reply)
{
    assert(parts == expected_parts && reply == expected_reply && count == 2);
    ++calls;
    strcpy(reply->status, next_status);
    reply->status_length = strlen(next_status);
    return next_result;
}

int main(void)
{
    uint8_t prefix[] = {1, 2, 6};
    uint8_t secret[] = "secret-payload-must-not-be-recorded";
    S64UciCommandPart parts[2];
    S64UciReply reply;
    char status_buffer[64];
    unsigned int i;
    memset(&reply, 0, sizeof(reply));
    reply.status = status_buffer;
    reply.status_capacity = sizeof(status_buffer);
    parts[0].data = prefix; parts[0].length = sizeof(prefix); parts[0].flags = 0;
    parts[1].data = secret; parts[1].length = sizeof(secret); parts[1].flags = 0;
    expected_parts = parts; expected_reply = &reply;
    next_status = ""; next_result = S64_UCI_OK;
    save_trace_reset();
    assert(save_trace_execute(parts, 2, &reply) == S64_UCI_OK);
    assert(save_trace[0].opcode == 2 && save_trace[0].attributes == 6);
    assert(save_trace[0].status[0] == 0 && reply.status_length == 0);
    assert(save_trace_failed_gate() == 0);
    prefix[1] = 5;
    next_status = "63,WRITE FAILED";
    assert(save_trace_execute(parts, 2, &reply) == S64_UCI_OK);
    assert(save_trace[1].bytes == sizeof(secret));
    assert(strcmp(save_trace[1].status, next_status) == 0);
    assert(strcmp(reply.status, next_status) == 0);
    assert(save_trace_failed_gate() == 2);
    prefix[1] = 9;
    next_status = "00,OK";
    for (i = 2; i < S64_SAVE_TRACE_CAPACITY + 2; ++i)
        assert(save_trace_execute(parts, 2, &reply) == S64_UCI_OK);
    assert(save_trace_count == S64_SAVE_TRACE_CAPACITY && save_trace_overflow);
    assert(calls == S64_SAVE_TRACE_CAPACITY + 2);
    assert(strcmp(save_trace[1].status, "63,WRITE FAILED") == 0);
    for (i = 0; i < save_trace_count; ++i)
        assert(strstr(save_trace[i].status, (const char*)secret) == 0);
    save_trace_reset();
    next_result = S64_UCI_TIMEOUT;
    assert(save_trace_execute(parts, 2, &reply) == S64_UCI_TIMEOUT);
    assert(save_trace[0].transport == S64_UCI_TIMEOUT);
    assert(save_trace_failed_gate() == 0); /* Failed cleanup is not a gate. */
    save_trace_reset();
    prefix[1] = 10;
    next_status = ""; next_result = S64_UCI_OK;
    assert(save_trace_execute(parts, 2, &reply) == S64_UCI_OK);
    assert(save_trace_failed_gate() == 1); /* RENAME requires a status. */
    puts("settings save trace tests passed");
    return 0;
}
