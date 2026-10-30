#ifndef SEE64YOU_PLATFORM_DOS_DIR_H
#define SEE64YOU_PLATFORM_DOS_DIR_H

#include <stdint.h>
#include "platform/dos.h"

/* Directory names are returned in ASCII by Ultimate-DOS. Never use a
 * truncated or unrepresentable name for a local file operation. */
#define S64_DOS_DIR_NAME_CAPACITY 34u
#define S64_DOS_DIR_ATTRIBUTE_DIRECTORY 0x10u

typedef struct {
    char name[S64_DOS_DIR_NAME_CAPACITY];
    uint8_t attributes;
    uint8_t skipped;
} S64DosDirEntry;

typedef uint8_t (*S64DosDirEntrySink)(const S64DosDirEntry*, void*);

/* OPEN_DIR returns 01,DIRECTORY EMPTY as a successful empty listing. */
S64DosResult s64_dos_dir_open(S64Dos* dos, uint8_t* empty);

/* READ_DIR is ONE command returning MANY separately framed UCI packets.
 * Each packet represents one entry, not one READ_DIR invocation. The packet
 * executor is injectable for host tests; on C64U use s64_uci_execute_packets.
 * Callback failure fails the enumeration; never show a partial list as whole. */
S64DosResult s64_dos_dir_read(
    S64Dos* dos, S64UciPacketExecuteFn execute_packets,
    S64DosDirEntrySink on_entry, void* context);

#endif
