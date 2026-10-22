/* Account for the future Publisher owner's actual cc65 resident state.
 * This diagnostic reserves storage only; it does not adopt or execute modules. */
#include "platform/module_runtime.h"
S64ModuleRuntime s64_managed_publisher_fit_state;
