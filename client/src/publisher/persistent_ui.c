#include "publisher/persistent_ui.h"

#include <conio.h>

S64PersistentPublisherAction s64_persistent_publisher_key(
    S64PersistentPublisher* publisher,
    uint8_t key)
{
    S64Document* document;

    if (!publisher || !s64_persistent_publisher_valid(publisher)) {
        return S64_PUBLISH_UI_NONE;
    }

    document = publisher->document;

    if (key == '1') return S64_PUBLISH_UI_SETTINGS;
    if (key == '2') return S64_PUBLISH_UI_CONNECT;
    if (key == '3') return S64_PUBLISH_UI_SAVE;
    if (key == '5') return S64_PUBLISH_UI_DELETE;
    if (key == '6') return S64_PUBLISH_UI_LIST_PAGE;
    if (key == '7') return S64_PUBLISH_UI_LIST_POST;
    if (key == '8') return S64_PUBLISH_UI_PREVIEW;
    if (key == '9') return S64_PUBLISH_UI_EXPORT_TEXT;
    if (key == '0') return S64_PUBLISH_UI_IMPORT_TEXT;
    if (key == CH_STOP) return S64_PUBLISH_UI_RETURN_EDITOR;

    if (key == '4') {
        document->status = document->status == S64_DOCUMENT_PUBLISHED
            ? S64_DOCUMENT_DRAFT
            : S64_DOCUMENT_PUBLISHED;
        s64_persistent_publisher_mark_changed(publisher, 0u);
        return S64_PUBLISH_UI_NONE;
    }

    if (key == 't' || key == 'T') {
        document->type = document->type == S64_DOCUMENT_PAGE
            ? S64_DOCUMENT_POST
            : S64_DOCUMENT_PAGE;
        s64_persistent_publisher_mark_changed(publisher, 0u);
        return S64_PUBLISH_UI_TYPE_CHANGED;
    }

    return S64_PUBLISH_UI_NONE;
}
