#ifndef SEE64YOU_PUBLISHER_PERSISTENT_UI_H
#define SEE64YOU_PUBLISHER_PERSISTENT_UI_H

#include <stdint.h>

#include "publisher/persistent_mode.h"

typedef enum {
    S64_PUBLISH_UI_NONE = 0,
    S64_PUBLISH_UI_SETTINGS,
    S64_PUBLISH_UI_CONNECT,
    S64_PUBLISH_UI_SAVE,
    S64_PUBLISH_UI_DELETE,
    S64_PUBLISH_UI_LIST_PAGE,
    S64_PUBLISH_UI_LIST_POST,
    S64_PUBLISH_UI_PREVIEW,
    S64_PUBLISH_UI_EXPORT_TEXT,
    S64_PUBLISH_UI_IMPORT_TEXT,
    S64_PUBLISH_UI_RETURN_EDITOR
} S64PersistentPublisherAction;

S64PersistentPublisherAction s64_persistent_publisher_key(
    S64PersistentPublisher* publisher,
    uint8_t key
);

#endif
