#ifndef SEE64YOU_STANDARD_KEYFILE_H
#define SEE64YOU_STANDARD_KEYFILE_H

#include <stdint.h>

#define S64_PUBLISH_KEYFILE_MAX_BYTES 512u
#define S64_PUBLISH_KEYFILE_URL_CAPACITY 193u

typedef struct {
    char url[S64_PUBLISH_KEYFILE_URL_CAPACITY];
    char id[33];
    char key[65];
} S64PublishKeyFile;

/* Parse ASCII bytes from a standard 8tml-publish: 1 key file.
   Successful fields are native text, suitable for current Settings.
   No partial credentials are retained when parsing fails. */
int s64_publish_keyfile_parse(const uint8_t* data, uint16_t length,
    S64PublishKeyFile* output);

#endif
