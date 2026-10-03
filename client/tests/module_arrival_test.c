#include <assert.h>
#include <stdio.h>
#include "platform/module_arrival.h"

int main(void)
{
    uint8_t marker[3];
    unsigned int destination, byte, value;
    for (destination = 0; destination < 256; ++destination) {
        marker[0] = 'S'; marker[1] = '6'; marker[2] = destination;
        assert(s64_module_arrival_consume(marker, destination)
            == (destination >= S64_MODULE_EDITOR
                && destination <= S64_MODULE_SETTINGS));
        assert(!marker[0] && !marker[1] && !marker[2]);
        assert(!s64_module_arrival_consume(marker, destination));
    }
    for (byte = 0; byte < 3; ++byte) {
        for (value = 0; value < 256; ++value) {
            marker[0] = 'S'; marker[1] = '6';
            marker[2] = S64_MODULE_EDITOR;
            if (marker[byte] == value) continue;
            marker[byte] = value;
            assert(!s64_module_arrival_consume(marker, S64_MODULE_EDITOR));
            assert(!marker[0] && !marker[1] && !marker[2]);
        }
    }
    marker[0] = 'S'; marker[1] = '6'; marker[2] = S64_MODULE_PUBLISHER;
    assert(!s64_module_arrival_consume(marker, S64_MODULE_EDITOR));
    assert(!marker[0] && !marker[1] && !marker[2]);
    assert(!s64_module_arrival_consume(0, S64_MODULE_EDITOR));
    puts("module arrival tests passed");
    return 0;
}
