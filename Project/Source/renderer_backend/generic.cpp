
#include "renderer_backend/generic.h"

#ifdef GFX_OPENGL
#include "renderer_backend/renderer_opengl.cpp"
#elif defined(GFX_D3D11)
#include "renderer_backend/d3d11.cpp"
#else
#error "No gfx api selected from the implemented ones"
#endif

u32 R_NumChannels(R_TextureFormat format)
{
    switch(format)
    {
        case TextureFormat_Invalid:         return 0;
        case TextureFormat_Count:           return 0;
        case TextureFormat_R32Int:          return 1;
        case TextureFormat_R:               return 1;
        case TextureFormat_RG:              return 2;
        case TextureFormat_RGBA:            return 4;
        case TextureFormat_RGBA_SRGB:       return 4;
        case TextureFormat_RGBA_HDR:        return 4;
        case TextureFormat_DepthStencil:    return 0;
    }
    
    return 0;
}

bool R_IsInteger(R_TextureFormat format)
{
    switch(format)
    {
        case TextureFormat_Invalid:         return false;
        case TextureFormat_Count:           return false;
        case TextureFormat_R32Int:          return true;
        case TextureFormat_R:               return false;
        case TextureFormat_RG:              return false;
        case TextureFormat_RGBA:            return false;
        case TextureFormat_RGBA_SRGB:       return false;
        case TextureFormat_RGBA_HDR:        return false;
        case TextureFormat_DepthStencil:    return false;
    }
    
    return false;
}