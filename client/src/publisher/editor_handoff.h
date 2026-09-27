#ifndef SEE64YOU_PUBLISHER_EDITOR_HANDOFF_H
#define SEE64YOU_PUBLISHER_EDITOR_HANDOFF_H

#include "document.h"
#include "editor.h"
#include "storage/local.h"

#ifndef S64_REU_BUFFER
#error "editor handoff requires S64_REU_BUFFER"
#endif

int s64_editor_handoff_store(
    const S64Editor* editor,
    const S64Document* document,
    const S64LocalDocument* local
);

int s64_editor_handoff_restore(
    S64Editor* editor,
    S64Document* document,
    S64LocalDocument* local
);

int s64_editor_handoff_clear(void);

#endif
