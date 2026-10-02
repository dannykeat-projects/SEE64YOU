#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "platform/module_transfer.h"

static void fixture(S64TransferCapsule* capsule)
{
    memset(capsule, 0, sizeof(*capsule));
    capsule->destination = S64_MODULE_EDITOR;
    capsule->module.id = S64_MODULE_EDITOR;
    capsule->module.cache_state = S64_MODULE_CACHE_READY;
    capsule->module.cache_address = 0x030000UL;
    capsule->module.byte_length = 0x3002UL;
    capsule->module.load_address = 0x0801u;
    capsule->module.startup_address = 0x080du;
}

int main(void)
{
    S64TransferCapsule capsule;
    S64ModuleTransferPlan plan, untouched;

    fixture(&capsule);
    memset(&plan, 0xa5, sizeof(plan));
    assert(s64_module_transfer_plan(&capsule, &plan));
    assert(plan.reu_source == 0x030002UL);
    assert(plan.ram_destination == 0x0801u);
    assert(plan.payload_length == 0x3000u);
    assert(plan.startup_address == 0x080du);

    untouched = plan;
    capsule.module.byte_length = 2UL;
    assert(!s64_module_transfer_plan(&capsule, &plan));
    assert(memcmp(&plan, &untouched, sizeof(plan)) == 0);

    fixture(&capsule);
    capsule.module.byte_length = 65538UL;
    assert(!s64_module_transfer_plan(&capsule, &plan));

    fixture(&capsule);
    capsule.module.load_address = 0xff00u;
    capsule.module.byte_length = 0x0202UL;
    assert(!s64_module_transfer_plan(&capsule, &plan));

    fixture(&capsule);
    capsule.module.startup_address = 0x7000u;
    assert(!s64_module_transfer_plan(&capsule, &plan));

    fixture(&capsule);
    capsule.module.load_address = 0x0700u;
    assert(!s64_module_transfer_plan(&capsule, &plan));

    fixture(&capsule);
    capsule.module.load_address = 0xcf00u;
    capsule.module.startup_address = 0xcf10u;
    capsule.module.byte_length = 0x0202UL;
    assert(!s64_module_transfer_plan(&capsule, &plan));

    fixture(&capsule);
    capsule.module.cache_address = 0x00fffffeUL;
    assert(!s64_module_transfer_plan(&capsule, &plan));

    fixture(&capsule);
    capsule.destination = S64_MODULE_PUBLISHER;
    assert(!s64_module_transfer_plan(&capsule, &plan));

    assert(!s64_module_transfer_plan(0, &plan));
    assert(!s64_module_transfer_plan(&capsule, 0));

    puts("module transfer tests passed");
    return 0;
}
