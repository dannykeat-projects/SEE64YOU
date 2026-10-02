#ifndef SEE64YOU_MODULE_PRELOAD_H
#define SEE64YOU_MODULE_PRELOAD_H
#include "platform/dos.h"
#include "platform/module_manifest.h"

/* Caller supplies the exclusively owned application-resource DOS context. */
uint8_t s64_module_preload(S64Dos* dos, S64ModuleManifest* manifest);
#endif
