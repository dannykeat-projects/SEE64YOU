#include "platform/reu_runtime.h"
#include "platform/reu_runtime_internal.h"
#include "platform/reu.h"
#include <string.h>

void s64_reu_runtime_init(
    S64ReuRuntimeControl* control,
    uint16_t app_version,
    uint32_t session_id
)
{
    if (control == 0) return;

    memset(control, 0, sizeof(*control));
    control->format_version = S64_REU_RUNTIME_FORMAT_VERSION;
    control->abi_version = S64_REU_RUNTIME_ABI_VERSION;
    control->app_version = app_version;
    control->session_id = session_id;
    control->next_free = S64_REU_RUNTIME_HEAP_START;
}


uint32_t s64_reu_runtime_remaining(const S64ReuRuntimeControl* control)
{
    if (!s64_reu_runtime_valid(control)) return 0UL;
    return S64_REU_RUNTIME_HEAP_END - control->next_free;
}

uint8_t s64_reu_runtime_store(const S64ReuRuntimeControl* control)
{
    uint8_t wire[S64_REU_RUNTIME_WIRE_SIZE];

    if (!s64_reu_runtime_encode(control, wire)) return 0u;
    return s64_reu_store(
        S64_REU_RUNTIME_CONTROL_BANK,
        S64_REU_RUNTIME_CONTROL_BASE,
        wire,
        S64_REU_RUNTIME_WIRE_SIZE
    );
}
