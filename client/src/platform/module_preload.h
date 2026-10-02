#ifndef SEE64YOU_MODULE_PRELOAD_H
#define SEE64YOU_MODULE_PRELOAD_H
#include "platform/dos.h"
#include "platform/module_manifest.h"
#include "platform/system_state.h"

/* Caller supplies the exclusively owned application-resource DOS context. */
uint8_t s64_module_preload(S64Dos* dos, S64ModuleManifest* manifest);
/* Caller owns an active IO_SAFE scope. Hashing temporarily uses CPU_INTENSIVE;
 * DMA and return use IO_SAFE. Any failure leaves the package unready. */
uint8_t s64_module_cache_verify(S64ModuleManifest* manifest, S64SystemState* system);
#endif
