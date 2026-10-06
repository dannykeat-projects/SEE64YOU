#include "platform/code_assets.h"

#ifndef S64_CODE_ASSET_PUBLISHER
static const S64CodeAssetNamespace editor_namespace = {
    S64_MODULE_EDITOR, 0x4fu, 3u, 0x21u, 4u,
    "S64OVL.MNF", "S64C",
    {"S64MODEL.BIN", "S64FILE.BIN", "S64RUN.BIN", "S64PREP.BIN"}
};
#endif

#if defined(S64_CODE_ASSET_PUBLISHER) || defined(S64_CODE_ASSET_DUAL_NAMESPACE)
static const S64CodeAssetNamespace publisher_namespace = {
    S64_MODULE_PUBLISHER, 0x50u, 5u, 0x31u, 3u,
    "S64POVL.MNF", "S64Q",
    {"S64PCNT.BIN", "S64PNET.BIN", "S64PRUN.BIN", 0}
};
#endif

const S64CodeAssetNamespace* s64_code_assets_namespace(S64ModuleId module)
{
#ifndef S64_CODE_ASSET_PUBLISHER
    if (module == S64_MODULE_EDITOR) return &editor_namespace;
#ifdef S64_CODE_ASSET_DUAL_NAMESPACE
    if (module == S64_MODULE_PUBLISHER) return &publisher_namespace;
#endif
#else
    if (module == S64_MODULE_PUBLISHER) return &publisher_namespace;
#endif
    return 0;
}
