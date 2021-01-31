#include "texasset.hpp"

namespace asset
{
    TextureAsset::TextureAsset(dab::Texture&& data, const std::string& assetHash) :
        dab::Texture(std::move(data)),
        hash(assetHash)
    {
    }

    TextureAsset::TextureAsset(const dab::Texture& data, const std::string& assetHash) :
        dab::Texture(data),
        hash(assetHash)
    {
    }

    VkFormat TextureAsset::textureFormat() const
    {
        // TODO : the same format as in the pixel buffer, otherwise fails
        switch (channels)
        {
        case 1: return VK_FORMAT_R8_SRGB;
        case 2: return VK_FORMAT_R8G8_SRGB;
        case 3: return VK_FORMAT_R8G8B8_SRGB;
        case 4: return VK_FORMAT_R8G8B8A8_SRGB;
        default: return VK_FORMAT_UNDEFINED;
        }
    }
}