#pragma once

#include "asset/shaderasset.hpp"
#include "asset/texasset.hpp"


namespace gr::core
{
    struct Material
    {
        const asset::ShaderAsset* shader;
        std::vector<const asset::TextureAsset*> textures;
    };
}