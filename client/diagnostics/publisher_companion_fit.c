#include "buffer.h"
#include "document.h"
#include "platform/http.h"
#include "platform/reu.h"
#include "publisher/destination.h"
#include "publisher/state.h"
#include "publisher/standalone.h"
#include "publisher/wordpress.h"

typedef union {
    S64StandaloneSession standalone;
    S64WordPressSession wordpress;
} S64PublisherSession;

static S64Buffer body;
static S64Document document;
static S64HttpTransport http;
static S64PublishDestination destination;
static S64PublishState publish_state;
static S64PublisherSession session;
static char url[S64_DESTINATION_URL_CAPACITY];

int main(void)
{
    if (!s64_reu_probe()) {
        return 1;
    }

    s64_http_init(&http, 0);
    s64_destination_clear(&destination);
    s64_publish_state_init(&publish_state);
    s64_standalone_session_clear(&session.standalone);
    s64_wordpress_session_clear(&session.wordpress);

    body.gap_start = 0u;
    body.gap_end = S64_BUFFER_CAPACITY;
    body.cursor = 0u;
    s64_document_init(&document, &body);
    url[0] = '\0';

    return 0;
}
