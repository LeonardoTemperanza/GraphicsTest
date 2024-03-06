
#include "asset_system.h"

Model* LoadModel(const char* path, Arena* dst)
{
    auto res = ArenaAllocType(Model, dst);
    memset(res, 0, sizeof(*res));
    
    FILE* modelFile = fopen(path, "r+b");
    // TODO: Error handling
    if(!modelFile)
        return res;
    
    defer { fclose(modelFile); };
    
    uintptr_t c = 0;
    uintptr_t* cursor = &c;
    s32 version = ReadAlignedNextValue<s32>(cursor);
    return res;
}