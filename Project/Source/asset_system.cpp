
#include "asset_system.h"

Model* LoadModel(const char* path, Arena* dst)
{
    auto res = ArenaAllocTyped(Model, dst);
    memset(res, 0, sizeof(*res));
    
    FILE* modelFile = fopen(path, "r+b");
    // TODO: Error reporting
    if(!modelFile)
        return res;
    
    defer { fclose(modelFile); };
    
    String contents = LoadEntireFile(path);
    if(!contents.ptr)
    {
        OS_DebugMessage("Could not even load file...\n");
        return res;
    }
    
    defer { free((void*)contents.ptr); };
    
    char** cursor;
    {
        char* c = (char*)contents.ptr;
        cursor = &c;
    }
    
    String magicBytes = Next(cursor, sizeof("model")-1);
    // TODO: Error reporting
    if(magicBytes != "model")
    {
        OS_DebugMessage("This thing is not even a model file...\n");
        return res;
    }
    
    s32 version = Next<s32>(cursor);
    char buffer[1024];
    snprintf(buffer, 1024, "Read version is: %d\n", version);
    
    
    
    return res;
}