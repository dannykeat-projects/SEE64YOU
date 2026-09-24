#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "publisher/destination.h"

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void require_url(
    S64DestinationResult result,
    const char* actual,
    const char* expected,
    const char* message
)
{
    require(result == S64_DESTINATION_OK, message);
    require(strcmp(actual, expected) == 0, message);
}

int main(void)
{
    S64PublishDestination destination;
    S64DestinationResult result;
    char url[S64_DESTINATION_URL_CAPACITY];
    char too_small[12];

    s64_destination_clear(&destination);
    require(destination.type == S64_DESTINATION_NONE, "clear destination type");
    require(destination.base_url[0] == '\0', "clear base URL");

    result = s64_destination_configure(
        &destination,
        S64_DESTINATION_STANDALONE,
        "http://relay.local:6464/site///",
        1u
    );
    require(result == S64_DESTINATION_OK, "configure standalone destination");
    require(
        strcmp(destination.base_url, "http://relay.local:6464/site") == 0,
        "trailing slash normalized"
    );
    require(destination.allow_insecure_http == 1u, "transport flag retained");

    require_url(
        s64_destination_connection_url(&destination, url, sizeof(url)),
        url,
        "http://relay.local:6464/site/api.php?action=login",
        "standalone login URL"
    );
    require_url(
        s64_destination_list_url(
            &destination, S64_DOCUMENT_POST, url, sizeof(url)
        ),
        url,
        "http://relay.local:6464/site/api.php?action=list&type=post",
        "standalone post list URL"
    );
    require_url(
        s64_destination_open_url(
            &destination, S64_DOCUMENT_PAGE, "about-us", url, sizeof(url)
        ),
        url,
        "http://relay.local:6464/site/api.php?action=get&type=page&slug=about-us&format=s64",
        "standalone page open URL"
    );
    require_url(
        s64_destination_save_url(
            &destination, "about-us", 1u, url, sizeof(url)
        ),
        url,
        "http://relay.local:6464/site/api.php?action=save&ack=1",
        "standalone save URL"
    );
    require_url(
        s64_destination_delete_url(
            &destination, S64_DOCUMENT_PAGE, "about-us", url, sizeof(url)
        ),
        url,
        "http://relay.local:6464/site/api.php?action=delete",
        "standalone delete URL"
    );

    require(
        s64_destination_open_url(
            &destination, S64_DOCUMENT_PAGE, "../bad", url, sizeof(url)
        ) == S64_DESTINATION_INVALID_ID,
        "standalone invalid slug rejected"
    );

    result = s64_destination_configure(
        &destination,
        S64_DESTINATION_WORDPRESS,
        "http://relay.local:6464/blog/",
        1u
    );
    require(result == S64_DESTINATION_OK, "configure WordPress destination");

    require_url(
        s64_destination_connection_url(&destination, url, sizeof(url)),
        url,
        "http://relay.local:6464/blog/wp-json/see64you/v1/connection",
        "WordPress connection URL"
    );
    require_url(
        s64_destination_list_url(
            &destination, S64_DOCUMENT_PAGE, url, sizeof(url)
        ),
        url,
        "http://relay.local:6464/blog/wp-json/see64you/v1/documents?type=page&managed=1",
        "WordPress page list URL"
    );
    require_url(
        s64_destination_open_url(
            &destination, S64_DOCUMENT_POST, "42", url, sizeof(url)
        ),
        url,
        "http://relay.local:6464/blog/wp-json/see64you/v1/documents/42?chunks=1",
        "WordPress open URL"
    );
    require_url(
        s64_destination_save_url(
            &destination, 0, 0u, url, sizeof(url)
        ),
        url,
        "http://relay.local:6464/blog/wp-json/see64you/v1/documents?ack=1",
        "WordPress create URL"
    );
    require_url(
        s64_destination_save_url(
            &destination, "42", 1u, url, sizeof(url)
        ),
        url,
        "http://relay.local:6464/blog/wp-json/see64you/v1/documents/42?ack=1",
        "WordPress update URL"
    );
    require_url(
        s64_destination_delete_url(
            &destination, S64_DOCUMENT_POST, "42", url, sizeof(url)
        ),
        url,
        "http://relay.local:6464/blog/wp-json/see64you/v1/documents/42",
        "WordPress delete URL"
    );

    require(
        s64_destination_open_url(
            &destination, S64_DOCUMENT_POST, "4x", url, sizeof(url)
        ) == S64_DESTINATION_INVALID_ID,
        "WordPress nonnumeric ID rejected"
    );

    require(
        s64_destination_configure(
            &destination,
            S64_DESTINATION_STANDALONE,
            "http://example.test/site?bad=1",
            1u
        ) == S64_DESTINATION_INVALID_BASE_URL,
        "base query rejected"
    );
    require(
        s64_destination_configure(
            &destination,
            S64_DESTINATION_STANDALONE,
            "http://example.test/site#bad",
            1u
        ) == S64_DESTINATION_INVALID_BASE_URL,
        "base fragment rejected"
    );
    require(
        s64_destination_configure(
            &destination,
            S64_DESTINATION_STANDALONE,
            "http://example.test/bad path",
            1u
        ) == S64_DESTINATION_INVALID_BASE_URL,
        "base whitespace rejected"
    );

    require(
        s64_destination_connection_url(&destination, too_small, sizeof(too_small))
            == S64_DESTINATION_INVALID_ARGUMENT,
        "failed configure leaves destination cleared"
    );

    require(
        s64_destination_configure(
            &destination,
            S64_DESTINATION_WORDPRESS,
            "http://example.test",
            1u
        ) == S64_DESTINATION_OK,
        "restore destination"
    );
    require(
        s64_destination_connection_url(&destination, too_small, sizeof(too_small))
            == S64_DESTINATION_URL_TOO_LONG,
        "bounded URL output"
    );

    puts("publisher destination tests passed");
    return 0;
}
