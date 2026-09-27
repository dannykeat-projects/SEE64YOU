#ifndef SEE64YOU_PUBLISHER_HANDOFF_RECORD_H
#define SEE64YOU_PUBLISHER_HANDOFF_RECORD_H

#include <stddef.h>
#include <stdint.h>

#include "publisher/handoff.h"

#define S64_HANDOFF_VERSION 1u
#define S64_HANDOFF_MAGIC_0 'S'
#define S64_HANDOFF_MAGIC_1 '6'
#define S64_HANDOFF_MAGIC_2 '4'
#define S64_HANDOFF_MAGIC_3 'H'
#define S64_HANDOFF_FLAGS_MASK (S64_HANDOFF_FLAG_DIRTY \
    | S64_HANDOFF_FLAG_INSERT_MODE | S64_HANDOFF_FLAG_WORD_WRAP)

typedef struct {
    char magic[4];
    uint8_t version;
    uint8_t flags;
    uint16_t gap_start;
    uint16_t gap_end;
    uint16_t cursor;
    uint8_t document_type;
    uint8_t document_status;
    int32_t order;
    char slug[S64_DOCUMENT_SLUG_CAPACITY];
    char title[S64_DOCUMENT_TITLE_CAPACITY];
    char date[S64_DOCUMENT_DATE_CAPACITY];
    char filename[S64_LOCAL_FILENAME_CAPACITY];
    S64History history;
    uint16_t checksum;
} S64HandoffRecord;

static uint16_t s64_handoff_record_checksum(const S64HandoffRecord* value)
{
    const uint8_t* bytes = (const uint8_t*)value;
    uint8_t sum1 = 0x64u;
    uint8_t sum2 = 0x11u;
    uint16_t i;

    for (i = 0u; i < (uint16_t)offsetof(S64HandoffRecord, checksum); ++i) {
        sum1 = (uint8_t)(sum1 + bytes[i]);
        sum2 = (uint8_t)(sum2 + sum1);
    }

    return (uint16_t)(((uint16_t)sum2 << 8) | sum1);
}

#endif
