#pragma once

#include "dab/import.hpp"

namespace asset
{
    struct TextureAsset : public dab::Texture
    {
        using type = dab::Texture;

        TextureAsset() = default;
        TextureAsset(dab::Texture&& data, const std::string& assetHash);
        TextureAsset(const dab::Texture& data, const std::string& assetHash);

        VkFormat textureFormat() const;

        const std::string hash;
    };
}