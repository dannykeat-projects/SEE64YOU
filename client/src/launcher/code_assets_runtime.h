#ifndef SEE64YOU_LAUNCHER_CODE_ASSETS_RUNTIME_H
#define SEE64YOU_LAUNCHER_CODE_ASSETS_RUNTIME_H
#include "launcher/recovery_runtime.h"
/* One attempt per newly planned module session; any failure requires cleanup. */
uint8_t s64_launcher_code_assets_prepare(S64LauncherRecoveryRuntime* runtime);
S64SessionLifecycleResult s64_launcher_code_assets_end(S64LauncherRecoveryRuntime* runtime);
#endif
