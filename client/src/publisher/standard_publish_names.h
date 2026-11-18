#ifndef SEE64YOU_STANDARD_PUBLISH_NAMES_H
#define SEE64YOU_STANDARD_PUBLISH_NAMES_H

int s64_publish_id_valid(const char* id);
int s64_publish_name_valid(const char* name);
int s64_publish_challenge_valid(const char* challenge);
/* Native/PETSCII or ASCII suffix; 0 rejects non-publishable source types. */
unsigned char s64_publish_source_kind(const char* filename);

#endif
