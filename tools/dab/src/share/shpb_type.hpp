#pragma once

#include <cstdint>

namespace shpb
{
    enum ShaderType : uint8_t
    {
        Shader_None     = 0x00,
        Shader_Vertex   = 0x01,
        Shader_Fragment = 0x10,
        Shader_Geometry = 0x08,
        Shader_Compute  = 0x20
    };

    constexpr const char FILE_MAGIC[] = "SHPB";
}