#include <conio.h>
#include <stdint.h>
#include <string.h>
#ifdef S64_STORAGE_SOURCE_BENCHMARK
#include <time.h>
#endif
#include "buffer.h"
#include "document.h"
#include "charset.h"
#include "platform/dos.h"
#include "platform/reu.h"
#include "platform/uci.h"
#include "platform/package_paths.h"
#include "storage/local.h"

/* Disposable selected-volume fixtures only. The harness backs up/restores the folder.
 * Faults intercept one DOS command; no media removal or machine reset. */
static S64Dos dos;
static S64Buffer body;
static S64Document document;
static S64LocalDocument local;
static uint8_t fault;
static uint8_t fault_hits;
static uint8_t fault_skip;

static S64UciResult execute(const S64UciCommandPart* parts, uint8_t count, S64UciReply* reply)
{
    if (count && parts[0].length >= 2u && parts[0].data[0] == S64_DOS_TARGET_SECONDARY
        && fault && parts[0].data[1] == fault) {
        if (fault_skip) {
            --fault_skip;
            return s64_uci_execute(parts, count, reply);
        }
        ++fault_hits;
        fault = 0u;
        return S64_UCI_UNAVAILABLE;
    }
    return s64_uci_execute(parts, count, reply);
}

#define CHECK(test, stage) do { if (!(test)) { \
    cprintf("8TML FAIL %u DOS %u\r\n", (unsigned)(stage), (unsigned)local.dos_result); \
    return (stage); } } while (0)

static char* path_destination;
static uint16_t path_length;
static char verify_path[440];

#ifdef S64_STORAGE_SOURCE_BENCHMARK
/* Separate from the captured DOS paths at $C000..$C3FF. The harness captures
 * and restores these 50 scratch bytes; publish the marker only on completion. */
#define BENCHMARK_RECORD ((uint8_t*)0xc500u)
static const char* const benchmark_inputs[] = {
    "T8LOAD1.8tml", "T8LOAD16.8tml", "T8LOAD60.8tml"
};
static const char* const benchmark_outputs[] = {
    "T8SAVE1.8tml", "T8SAVE16.8tml", "T8SAVE60.8tml"
};
static const unsigned int benchmark_sizes[] = {1024u, 16384u, 61440u};

static void benchmark_tick(uint8_t offset, clock_t ticks)
{
    uint8_t i;
    for (i = 0u; i < 4u; ++i) {
        BENCHMARK_RECORD[offset + i] = (uint8_t)ticks;
        ticks >>= 8;
    }
}

static uint8_t benchmark_sources(void)
{
    uint8_t i, offset;
    clock_t started;
    memset(BENCHMARK_RECORD, 0, 50u);
    benchmark_tick(4u, (clock_t)CLOCKS_PER_SEC);
    for (i = 0u; i < 3u; ++i) {
        offset = (uint8_t)(8u + i * 14u);
        BENCHMARK_RECORD[offset] = (uint8_t)benchmark_sizes[i];
        BENCHMARK_RECORD[offset + 1u] = (uint8_t)(benchmark_sizes[i] >> 8);
        started = clock();
        CHECK(s64_local_open(&local, benchmark_inputs[i], 1u) == S64_LOCAL_OK, 44);
        benchmark_tick(offset + 2u, clock() - started);
        CHECK(s64_buffer_length(&body) == benchmark_sizes[i], 45);
        started = clock();
        CHECK(s64_local_save_as(&local, benchmark_outputs[i], 0u) == S64_LOCAL_OK, 46);
        benchmark_tick(offset + 6u, clock() - started);
        started = clock();
        CHECK(s64_local_open(&local, benchmark_outputs[i], 1u) == S64_LOCAL_OK, 47);
        benchmark_tick(offset + 10u, clock() - started);
        CHECK(s64_buffer_length(&body) == benchmark_sizes[i], 48);
    }
    BENCHMARK_RECORD[0] = 0x53u;
    BENCHMARK_RECORD[1] = 0x38u;
    BENCHMARK_RECORD[2] = 0x54u;
    BENCHMARK_RECORD[3] = 0x31u;
    return 0u;
}
#endif

static uint8_t path_sink(const uint8_t* data, uint16_t length, void* unused)
{
    uint16_t i;
    uint8_t native;
    (void)unused;
    if (length > 439u - path_length) return 0u;
    for (i = 0u; i < length; ++i) {
        if (!s64_charset_ascii_to_native(data[i], &native)) return 0u;
        path_destination[path_length++] = (char)native;
    }
    path_destination[path_length] = '\0';
    return 1u;
}

static uint8_t get_path(uint8_t target, char* destination)
{
    uint8_t command[2];
    S64UciCommandPart part;
    S64UciReply reply;
    char status[32];
    command[0] = target;
    command[1] = 0x12u; /* Published read-only GET_PATH; also used by Settings. */
    part.data = command;
    part.length = 2u;
    part.flags = 0u;
    path_destination = destination;
    path_length = 0u;
    destination[0] = '\0';
    reply.data_sink = path_sink;
    reply.data_context = 0;
    reply.status = status;
    reply.status_capacity = sizeof(status);
    reply.status_length = 0u;
    return s64_uci_execute(&part, 1u, &reply) == S64_UCI_OK
        && status[0] == '0' && status[1] == '0' && destination[0] == '/';
}

int main(void)
{
    const char* text;
    uint8_t exists;
    unsigned int length;
    char boundary[S64_LOCAL_FILENAME_CAPACITY];
    uint8_t target;
    uint8_t mode;
    char* saved_path;
    clrscr();
    cputs("8TML PROBE\r\nC CAPTURE T TEST R RESTORE\r\n");
    mode = (uint8_t)cgetc();
    if (mode == 'c' || mode == 'r') {
        for (target = 1u; target <= 2u; ++target) {
            saved_path = (char*)(0xc000u + (target - 1u) * 512u);
            if (mode == 'c') {
                memset(saved_path, 0, 512u);
                CHECK(get_path(target, saved_path), 28);
            } else {
                CHECK(saved_path[0] == '/' && strlen(saved_path) < 440u, 29);
                s64_dos_init(&dos, s64_uci_execute, target);
                CHECK(s64_dos_change_dir(&dos, saved_path) == S64_DOS_OK, 30);
                CHECK(get_path(target, verify_path) && strcmp(saved_path, verify_path) == 0, 31);
            }
        }
        cputs(mode == 'c' ? "DOS CONTEXT CAPTURED\r\n" : "DOS CONTEXT RESTORED\r\n");
        return 0;
    }
    CHECK(mode == 't', 32);
    CHECK(s64_reu_probe_banks() == 256u, 1);
    s64_dos_init(&dos, execute, S64_DOS_TARGET_SECONDARY);
    CHECK(s64_dos_change_dir(&dos, S64_PACKAGE_DIRECTORY) == S64_DOS_OK, 2);
    s64_buffer_init(&body);
    s64_document_init(&document, &body);
    s64_local_init(&local, &dos, &document);
    CHECK(s64_local_save_as(&local, "T8EMPTY", 0u) == S64_LOCAL_OK, 3);
    CHECK(s64_local_open(&local, "T8IN.8tml", 1u) == S64_LOCAL_OK, 4);
    CHECK(s64_local_save_as(&local, "T8COPY", 0u) == S64_LOCAL_OK, 5);
    CHECK(s64_local_save_as(&local, "T8COPY", 0u) == S64_LOCAL_EXISTS, 6);
    CHECK(s64_buffer_move_cursor(&body, s64_buffer_length(&body)), 7);
    text = "Hardware edit.\n";
    while (*text) CHECK(s64_buffer_insert(&body, (uint8_t)*text++), 8);
    CHECK(s64_local_save(&local) == S64_LOCAL_OK, 9);
    CHECK(s64_local_open(&local, "T8COPY.8tml", 1u) == S64_LOCAL_OK, 10);
    length = s64_buffer_length(&body);
    CHECK(s64_local_open(&local, "T8MISSING.8tml", 1u) == S64_LOCAL_IO_ERROR, 11);
    CHECK(s64_buffer_length(&body) == length && !s64_local_document_replaced(&local), 12);
    fault = 0x05u;
    CHECK(s64_local_save(&local) == S64_LOCAL_IO_ERROR, 13);
    fault = 0u;
    CHECK(fault_hits == 1u, 14);
    CHECK(s64_dos_file_exists(&dos, "T8COPY.8tml.tmp", &exists) == S64_DOS_OK && !exists, 15);
    CHECK(s64_buffer_move_cursor(&body, s64_buffer_length(&body))
        && s64_buffer_insert(&body, '!'), 16);
    fault = 0x0au;
    fault_skip = 1u; /* Allow original -> backup; fail temporary -> destination. */
    CHECK(s64_local_save(&local) == S64_LOCAL_IO_ERROR, 17);
    fault = 0u;
    CHECK(fault_hits == 2u, 18);
    CHECK(s64_dos_file_exists(&dos, "T8COPY.8tml", &exists) == S64_DOS_OK && exists, 38);
    CHECK(s64_dos_file_exists(&dos, "T8COPY.8tml.bak", &exists) == S64_DOS_OK && !exists, 39);
    CHECK(s64_local_save(&local) == S64_LOCAL_RECOVERY_CONFLICT, 40);
    CHECK(s64_dos_rename_file(&dos, "T8COPY.8tml.tmp", "T8COPY.8tml.bak") == S64_DOS_OK, 41);
    CHECK(s64_local_save(&local) == S64_LOCAL_RECOVERY_CONFLICT, 42);
    CHECK(s64_dos_rename_file(&dos, "T8COPY.8tml.bak", "T8COPY.8tml.tmp") == S64_DOS_OK, 43);
    CHECK(s64_local_open(&local, "T8COPY.8tml.tmp", 1u) == S64_LOCAL_OK, 19);
    CHECK(s64_local_save_as(&local, "T8RECOVER", 0u) == S64_LOCAL_OK, 20);
    CHECK(s64_local_save_as(&local, "T8WRONG.s64", 0u) == S64_LOCAL_INVALID_ARGUMENT, 21);
    CHECK(s64_local_open(&local, "T8BAD.8tml", 1u) == S64_LOCAL_INVALID_DOCUMENT, 22);
    CHECK(!s64_local_has_filename(&local), 23);
    CHECK(s64_local_new(&local, 1u) == S64_LOCAL_OK, 24);
    memset(boundary, 'x', S64_LOCAL_FILENAME_CAPACITY - sizeof(".8tml"));
    boundary[S64_LOCAL_FILENAME_CAPACITY - sizeof(".8tml")] = '\0';
    CHECK(s64_local_save_as(&local, boundary, 0u) == S64_LOCAL_OK, 25);
    CHECK(s64_local_open(&local, boundary, 1u) == S64_LOCAL_OK, 26);
    CHECK(s64_local_open(&local, local.filename, 1u) == S64_LOCAL_OK, 27);
#ifdef S64_STORAGE_MAX_SOURCE_PROBE
    CHECK(s64_local_open(&local, "T8LARGE.8tml", 1u) == S64_LOCAL_OK, 33);
    CHECK(s64_buffer_length(&body) == S64_BUFFER_CAPACITY, 34);
    CHECK(s64_local_save_as(&local, "T8MAXCOPY", 0u) == S64_LOCAL_OK, 35);
    CHECK(s64_local_open(&local, "T8MAXCOPY.8tml", 1u) == S64_LOCAL_OK, 36);
    CHECK(s64_buffer_length(&body) == S64_BUFFER_CAPACITY, 37);
#endif
#ifdef S64_STORAGE_SOURCE_BENCHMARK
    if (benchmark_sources()) return 49;
#endif
    cputs("8TML STORAGE TESTS PASSED\r\n");
    return 0;
}
