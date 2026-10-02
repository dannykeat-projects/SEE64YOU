#include <stdio.h>
#include <stdlib.h>
#include "platform/module_manifest.h"

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
    puts("module manifest tests passed");
    return 0;
}
