#ifndef SEE64YOU_PUBLISHER_WORKSPACE_H
#define SEE64YOU_PUBLISHER_WORKSPACE_H

#include <stdint.h>

/*
 * Publisher operations are strictly synchronous. Large request/response,
 * parsing and handoff scratch areas therefore share one aligned workspace
 * instead of permanently consuming separate C64 BSS allocations.
 */
#define S64_PUBLISH_WORKSPACE_CAPACITY 768u
#define S64_PUBLISH_WORK_HEADER_OFFSET 0u
#define S64_PUBLISH_WORK_TEXT_OFFSET 257u
#define S64_PUBLISH_WORK_AUX_OFFSET 449u
#define S64_PUBLISH_WORK_DOC_OFFSET 257u
#define S64_PUBLISH_WORK_WP_SOURCE_OFFSET 361u
#define S64_PUBLISH_WORK_WP_SUMMARY_OFFSET 344u
#define S64_PUBLISH_WORK_MAIN_USERNAME_OFFSET 320u
#define S64_PUBLISH_WORK_HANDOFF_OFFSET 500u
#define S64_PUBLISH_WORK_SELECTION_OFFSET 603u
#define S64_PUBLISH_WORK_RELAY_OFFSET 449u

typedef union {
    void* align_pointer;
    unsigned long align_long;
    uint8_t bytes[S64_PUBLISH_WORKSPACE_CAPACITY];
} S64PublisherWorkspace;

#if defined(S64_MANAGED_OVERLAY) || defined(S64_PERSISTENT_RUNTIME_FIT)
/*
 * Managed or persistent Publisher overlays occupy $8C00-$C7FF. The 1 KiB window
 * $C800-$CBFF is outside the overlay slot and below the software stack, so the
 * strictly synchronous 768-byte Publisher workspace can live there without
 * consuming resident BSS. The inbound transfer capsule at $C800 is one-shot
 * authority and has already been consumed before normal Publisher work begins.
 */
#define s64_publisher_workspace (*(S64PublisherWorkspace*)0xc800u)
#else
extern S64PublisherWorkspace s64_publisher_workspace;
#endif

#endif
