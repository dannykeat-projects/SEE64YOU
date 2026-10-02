#ifndef SEE64YOU_PLATFORM_MODULE_MANIFEST_H
#define SEE64YOU_PLATFORM_MODULE_MANIFEST_H
#include <stdint.h>
#include "platform/reu_runtime.h"

#define S64_MODULE_COUNT 3u
#define S64_MODULE_FILENAME_CAPACITY 13u
#define S64_MODULE_CACHE_CAPACITY 65536UL
#define S64_MODULE_MANIFEST_FILENAME "S64MODULE.MNF"
#define S64_MODULE_MANIFEST_MAX_SIZE 255u

typedef enum {
    S64_MODULE_EDITOR = 1,
    S64_MODULE_PUBLISHER,
    S64_MODULE_SETTINGS
} S64ModuleId;

typedef struct {
    uint8_t id;
    char filename[S64_MODULE_FILENAME_CAPACITY];
    uint16_t abi_version;
    uint16_t app_version;
    uint16_t load_address;
    uint16_t startup_address;
    uint32_t byte_length;
    uint32_t integrity;
    uint32_t cache_address;
    uint32_t cache_capacity;
} S64ModuleDescriptor;

typedef struct {
    S64ModuleDescriptor modules[S64_MODULE_COUNT];
} S64ModuleManifest;

void s64_module_manifest_init(S64ModuleManifest* manifest, uint16_t app_version);
uint8_t s64_module_descriptor_identity_valid(const S64ModuleDescriptor* module);
uint8_t s64_module_descriptor_loadable(const S64ModuleDescriptor* module);
uint8_t s64_module_manifest_allocate(
    S64ModuleManifest* manifest,
    S64ReuRuntimeControl* control
);
uint8_t s64_module_manifest_parse(
    S64ModuleManifest* manifest,
    const char* text,
    uint16_t length
);
uint8_t s64_module_manifest_loadable(const S64ModuleManifest* manifest);
#endif
