#include <stdint.h>
/* Bind both the tested exit implementation and its return-context fixture to
 * actual host storage; production retains the asserted $C800 workspace. */
extern uint8_t s64_test_basic_return_workspace[1024];
#define S64_BASIC_RETURN_WORKSPACE s64_test_basic_return_workspace
