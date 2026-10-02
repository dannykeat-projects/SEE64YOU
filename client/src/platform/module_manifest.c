#include "platform/module_manifest.h"
#include <string.h>

static void init_module(
    S64ModuleDescriptor* module,
    S64ModuleId id,
    const char* filename,
    uint16_t app_version
)
{
    memset(module, 0, sizeof(*module));
    module->id = (uint8_t)id;
    strncpy(module->filename, filename, sizeof(module->filename) - 1u);
    module->abi_version = S64_REU_RUNTIME_ABI_VERSION;
    module->app_version = app_version;
    module->cache_capacity = S64_MODULE_CACHE_CAPACITY;
}

void s64_module_manifest_init(S64ModuleManifest* manifest, uint16_t app_version)
{
    if (manifest == 0) return;
    memset(manifest, 0, sizeof(*manifest));
    init_module(&manifest->modules[0], S64_MODULE_EDITOR, "S64EDIT.PRG", app_version);
    init_module(&manifest->modules[1], S64_MODULE_PUBLISHER, "S64PUB.PRG", app_version);
    init_module(&manifest->modules[2], S64_MODULE_SETTINGS, "S64CFG.PRG", app_version);
}

uint8_t s64_module_descriptor_identity_valid(const S64ModuleDescriptor* module)
{
    return module != 0
        && module->id >= S64_MODULE_EDITOR
        && module->id <= S64_MODULE_SETTINGS
        && module->filename[0] != '\0'
        && module->abi_version == S64_REU_RUNTIME_ABI_VERSION
        && module->app_version != 0u
        && module->cache_capacity != 0UL;
}

uint8_t s64_module_descriptor_loadable(const S64ModuleDescriptor* module)
{
    return s64_module_descriptor_identity_valid(module)
        && module->load_address != 0u
        && module->startup_address != 0u
        && module->byte_length > 2UL
        && module->byte_length <= module->cache_capacity
        && module->integrity != 0UL
        && module->cache_address >= S64_REU_RUNTIME_HEAP_START;
}

uint8_t s64_module_manifest_allocate(
    S64ModuleManifest* manifest,
    S64ReuRuntimeControl* control
)
{
    uint8_t i;
    S64ReuAllocation allocation;
    if (manifest == 0 || !s64_reu_runtime_valid(control)) return 0u;

    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        if (!s64_module_descriptor_identity_valid(&manifest->modules[i])
            || !s64_reu_runtime_allocate(
                control,
                S64_REU_OWNER_MODULE,
                manifest->modules[i].cache_capacity,
                256u,
                &allocation
            )) {
            return 0u;
        }
        manifest->modules[i].cache_address = allocation.address;
    }
    return 1u;
}


static uint8_t hex_digit(char ch, uint8_t* value)
{
    if (ch >= '0' && ch <= '9') {
        *value = (uint8_t)(ch - '0');
        return 1u;
    }
    if (ch >= 'A' && ch <= 'F') {
        *value = (uint8_t)(ch - 'A' + 10);
        return 1u;
    }
    return 0u;
}

static uint8_t parse_hex(
    const char* text,
    uint8_t digits,
    uint32_t* value
)
{
    uint8_t i;
    uint8_t digit;
    uint32_t parsed = 0UL;

    if (text == 0 || value == 0 || digits == 0u) return 0u;
    for (i = 0u; i < digits; ++i) {
        if (!hex_digit(text[i], &digit)) return 0u;
        parsed = (parsed << 4) | digit;
    }
    *value = parsed;
    return 1u;
}

static uint8_t parse_decimal(
    const char* start,
    const char* end,
    uint32_t* value
)
{
    uint32_t parsed = 0UL;
    const char* cursor = start;

    if (start == 0 || end == 0 || value == 0 || start >= end) return 0u;
    while (cursor < end) {
        uint8_t digit;
        if (*cursor < '0' || *cursor > '9') return 0u;
        digit = (uint8_t)(*cursor - '0');
        if (parsed > 429496729UL
            || (parsed == 429496729UL && digit > 5u)) {
            return 0u;
        }
        parsed = parsed * 10UL + digit;
        ++cursor;
    }
    *value = parsed;
    return 1u;
}

static const char* find_char(
    const char* start,
    const char* end,
    char wanted
)
{
    while (start < end) {
        if (*start == wanted) return start;
        ++start;
    }
    return 0;
}

static uint8_t parse_line(
    S64ModuleDescriptor* module,
    const char* label,
    const char* line,
    const char* end
)
{
    const char* fields[6];
    const char* cursor = line;
    const char* separator;
    uint8_t i;
    uint32_t parsed;

    fields[0] = line;
    for (i = 0u; i < 5u; ++i) {
        separator = find_char(cursor, end, '|');
        if (separator == 0) return 0u;
        fields[i + 1u] = separator + 1;
        cursor = separator + 1;
    }
    if (find_char(cursor, end, '|') != 0) return 0u;

    if ((uint16_t)(fields[1] - fields[0] - 1) != strlen(label)
        || memcmp(fields[0], label, strlen(label)) != 0) {
        return 0u;
    }
    if ((uint16_t)(fields[2] - fields[1] - 1) != strlen(module->filename)
        || memcmp(fields[1], module->filename, strlen(module->filename)) != 0) {
        return 0u;
    }
    if ((uint16_t)(fields[3] - fields[2] - 1) != 4u
        || !parse_hex(fields[2], 4u, &parsed)) return 0u;
    module->load_address = (uint16_t)parsed;

    if ((uint16_t)(fields[4] - fields[3] - 1) != 4u
        || !parse_hex(fields[3], 4u, &parsed)) return 0u;
    module->startup_address = (uint16_t)parsed;

    if (!parse_decimal(fields[4], fields[5] - 1, &parsed)) return 0u;
    module->byte_length = parsed;

    if ((uint16_t)(end - fields[5]) != 8u
        || !parse_hex(fields[5], 8u, &parsed)) return 0u;
    module->integrity = parsed;

    return s64_module_descriptor_loadable(module);
}

uint8_t s64_module_manifest_parse(
    S64ModuleManifest* manifest,
    const char* text,
    uint16_t length
)
{
    static const char magic[] = "S64MODULES1";
    static const char* labels[S64_MODULE_COUNT] = {
        "EDITOR", "PUBLISHER", "SETTINGS"
    };
    const char* cursor;
    const char* end;
    const char* line_end;
    uint8_t i;

    if (manifest == 0 || text == 0
        || length == 0u || length > S64_MODULE_MANIFEST_MAX_SIZE) {
        return 0u;
    }

    cursor = text;
    end = text + length;
    line_end = find_char(cursor, end, '\n');
    if (line_end == 0
        || (uint16_t)(line_end - cursor) != sizeof(magic) - 1u
        || memcmp(cursor, magic, sizeof(magic) - 1u) != 0) {
        return 0u;
    }
    cursor = line_end + 1;

    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        if (cursor >= end) return 0u;
        line_end = find_char(cursor, end, '\n');
        if (line_end == 0
            || !parse_line(
                &manifest->modules[i],
                labels[i],
                cursor,
                line_end
            )) {
            return 0u;
        }
        cursor = line_end + 1;
    }

    return cursor == end && s64_module_manifest_loadable(manifest);
}

uint8_t s64_module_manifest_loadable(const S64ModuleManifest* manifest)
{
    uint8_t i;
    if (manifest == 0) return 0u;
    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        if (!s64_module_descriptor_loadable(&manifest->modules[i])) {
            return 0u;
        }
    }
    return 1u;
}
