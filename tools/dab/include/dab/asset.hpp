#pragma once

#include <string>

namespace dab
{
    enum AssetType : uint8_t
    {
        Asset_None,
        Asset_Texture,
        Asset_Shader,
        Asset_Mesh
    };

    struct AssetDecl
    {
        std::string name;
        size_t offset;
        AssetType type;
    };
}
