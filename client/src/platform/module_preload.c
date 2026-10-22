#include "platform/module_preload.h"
#include "platform/package_paths.h"

uint8_t s64_module_preload(S64Dos* dos, S64ModuleManifest* manifest)
{
    uint8_t i;
    uint32_t file_size;
    S64DosResult result, close_result;
    S64ModuleDescriptor* module;

    if (dos == 0 || manifest == 0) return 0u;
    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        manifest->modules[i].cache_state = S64_MODULE_CACHE_EMPTY;
    }
    if (!s64_module_manifest_loadable(manifest)
        || s64_dos_change_dir(dos, S64_PACKAGE_DIRECTORY) != S64_DOS_OK) return 0u;

    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        module = &manifest->modules[i];
        if (module->cache_address >= 0x01000000UL
            || module->cache_capacity > 0x01000000UL - module->cache_address
            || s64_dos_open_read(dos, module->filename) != S64_DOS_OK) return 0u;
        result = s64_dos_file_size(dos, &file_size);
        if (result == S64_DOS_OK && file_size != module->byte_length) {
            result = S64_DOS_TARGET_ERROR;
        }
        if (result == S64_DOS_OK) {
            /* Cache the entire PRG, including its two-byte load header. */
            result = s64_dos_load_reu(dos, module->cache_address, module->byte_length);
        }
        close_result = s64_dos_close(dos);
        if (result != S64_DOS_OK || close_result != S64_DOS_OK) return 0u;
        module->cache_state = S64_MODULE_CACHE_LOADED_UNVERIFIED;
    }
    return 1u;
}
