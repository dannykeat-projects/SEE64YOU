#include <stdio.h>
#include <stdlib.h>
#include "platform/module_manifest.h"

uint8_t s64_reu_store(
    uint8_t bank,
    uint16_t address,
    const void* source,
    uint16_t length
)
{
    (void)bank;
    (void)address;
    (void)source;
    (void)length;
    return 1u;
}

uint8_t s64_reu_load(
    uint8_t bank,
    uint16_t address,
    void* destination,
    uint16_t length
)
{
    (void)bank;
    (void)address;
    (void)destination;
    (void)length;
    return 1u;
}

static void require(int ok,const char* msg){if(!ok){fprintf(stderr,"FAIL: %s\n",msg);exit(1);}}

int main(void){
    S64ModuleManifest manifest;
    S64ReuRuntimeControl control;
    s64_reu_runtime_init(&control,0x0201u,1UL);
    s64_module_manifest_init(&manifest,0x0201u);
    require(s64_module_descriptor_identity_valid(&manifest.modules[0]),"editor identity valid");
    require(!s64_module_descriptor_loadable(&manifest.modules[0]),"unmeasured editor not loadable");
    require(s64_module_manifest_allocate(&manifest,&control),"module cache allocation succeeds");
    require(control.allocation_count==S64_MODULE_COUNT,"three module allocations recorded");
    require(manifest.modules[0].cache_address < manifest.modules[1].cache_address
        && manifest.modules[1].cache_address < manifest.modules[2].cache_address,
        "module cache regions are ordered and distinct");
    manifest.modules[0].load_address=0x0801u;
    manifest.modules[0].startup_address=0x0810u;
    manifest.modules[0].byte_length=32000UL;
    manifest.modules[0].integrity=1UL;
    require(s64_module_descriptor_loadable(&manifest.modules[0]),"measured descriptor becomes loadable");
    manifest.modules[0].byte_length=S64_MODULE_CACHE_CAPACITY+1UL;
    require(!s64_module_descriptor_loadable(&manifest.modules[0]),"oversize module rejected");

    s64_module_manifest_init(&manifest,0x0201u);
    s64_reu_runtime_init(&control,0x0201u,2UL);
    require(s64_module_manifest_allocate(&manifest,&control),"parse fixture cache allocation succeeds");
    {
        static const char fixture[] =
            "S64MODULES1\n"
            "EDITOR|S64EDIT.PRG|0801|080D|48134|9089A375\n"
            "PUBLISHER|S64PUB.PRG|0801|080D|46614|07821DD1\n"
            "SETTINGS|S64CFG.PRG|0801|080D|27536|D75589BA\n";
        require(s64_module_manifest_parse(
            &manifest, fixture, (uint16_t)(sizeof(fixture)-1u)
        ),"exact generated manifest parses");
        require(s64_module_manifest_loadable(&manifest),
            "parsed manifest makes all descriptors loadable");
        require(manifest.modules[1].byte_length==46614UL
            && manifest.modules[2].integrity==0xD75589BAUL,
            "parsed artifact metadata stored exactly");
    }
    {
        static const char bad[] =
            "S64MODULES1\n"
            "EDITOR|S64EDIT.PRG|0801|080D|48134|9089A375\n"
            "PUBLISHER|WRONG.PRG|0801|080D|46614|07821DD1\n"
            "SETTINGS|S64CFG.PRG|0801|080D|27536|D75589BA\n";
        require(!s64_module_manifest_parse(
            &manifest, bad, (uint16_t)(sizeof(bad)-1u)
        ),"wrong module identity is rejected");
    }
    puts("module manifest tests passed");
    return 0;
}
