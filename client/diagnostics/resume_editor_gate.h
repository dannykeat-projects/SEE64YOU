#ifndef S64_RESUME_EDITOR_GATE_H
#define S64_RESUME_EDITOR_GATE_H
#include "launcher/recovery_runtime.h"
/* Diagnostic only: live binding of a build-generated production-model fixture.
 * Caller supplies at least 544 bytes of synchronous workspace in IO_SAFE. */
uint8_t s64_resume_editor_fixture(S64LauncherRecoveryRuntime* runtime,
    uint8_t corrupt, uint8_t* workspace);
#endif
