#ifndef SEE64YOU_SESSION_LIFECYCLE_INTERNAL_H
#define SEE64YOU_SESSION_LIFECYCLE_INTERNAL_H
#include "platform/session_lifecycle.h"

/* Startup rollback and adopted-session exit use the same CLEAN save path. */
S64SessionLifecycleResult s64_session_lifecycle_save_clean(
    S64SessionLifecycle* lifecycle
);
#endif
