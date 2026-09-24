#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "publisher/state.h"

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

int main(void)
{
    S64PublishDestination destination;
    S64PublishState state;
    S64Buffer body;
    S64Document document;

    s64_destination_clear(&destination);
    s64_publish_state_init(&state);
    s64_buffer_init(&body);
    s64_document_init(&document, &body);

    require(!state.connected && !state.remote_bound, "state starts disconnected/new");

    require(
        s64_destination_configure(
            &destination,
            S64_DESTINATION_STANDALONE,
            "http://relay.local:6464/site",
            1u
        ) == S64_DESTINATION_OK,
        "configure standalone"
    );
    document.type = S64_DOCUMENT_POST;
    require(s64_document_set_slug(&document, "hello-world"), "set document slug");

    require(
        s64_publish_state_bind_remote(
            &state,
            &destination,
            S64_DOCUMENT_POST,
            "hello-world"
        ),
        "bind standalone remote identity"
    );
    require(state.remote_bound, "remote bound");
    require(!s64_publish_state_can_update(&state, &destination, &document), "offline update blocked");

    s64_publish_state_set_connected(&state, 1u);
    require(s64_publish_state_can_update(&state, &destination, &document), "matching standalone update allowed");
    require(s64_publish_state_can_delete(&state, &destination), "matching standalone delete allowed");

    require(s64_document_set_slug(&document, "renamed-post"), "change document slug");
    require(
        !s64_publish_state_can_update(&state, &destination, &document),
        "standalone slug change is not silent update"
    );
    require(
        s64_publish_state_can_delete(&state, &destination),
        "original standalone remote identity remains deletable"
    );

    s64_publish_state_set_connected(&state, 0u);
    require(state.remote_bound, "disconnect preserves remote identity");
    require(!s64_publish_state_can_delete(&state, &destination), "offline delete blocked");

    s64_publish_state_clear_remote(&state);
    require(!state.remote_bound && state.remote_identity[0] == '\0', "new document clears remote identity");

    require(
        s64_destination_configure(
            &destination,
            S64_DESTINATION_WORDPRESS,
            "http://relay.local:6464/blog",
            1u
        ) == S64_DESTINATION_OK,
        "configure WordPress"
    );
    s64_publish_state_set_connected(&state, 1u);
    document.type = S64_DOCUMENT_PAGE;
    require(s64_document_set_slug(&document, "about"), "set page slug");

    require(
        s64_publish_state_bind_remote(
            &state,
            &destination,
            S64_DOCUMENT_PAGE,
            "12345"
        ),
        "bind WordPress numeric ID"
    );
    require(s64_publish_state_can_update(&state, &destination, &document), "WordPress update allowed");
    require(s64_publish_state_can_delete(&state, &destination), "WordPress delete allowed");

    document.type = S64_DOCUMENT_POST;
    require(
        !s64_publish_state_can_update(&state, &destination, &document),
        "remote type change blocks update"
    );

    require(
        !s64_publish_state_bind_remote(
            &state,
            &destination,
            S64_DOCUMENT_POST,
            "12x"
        ),
        "invalid WordPress ID rejected"
    );
    require(
        strcmp(state.remote_identity, "12345") == 0,
        "failed identity replacement preserves existing binding"
    );

    puts("publisher state tests passed");
    return 0;
}
