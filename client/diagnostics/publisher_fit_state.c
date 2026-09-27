#include "platform/http.h"
#include "publisher/destination.h"
#include "publisher/state.h"

S64PublishDestination s64_fit_destination;
S64PublishState s64_fit_publish_state;
S64HttpTransport s64_fit_http;
char s64_fit_url[S64_DESTINATION_URL_CAPACITY];
