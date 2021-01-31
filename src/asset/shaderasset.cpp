#include "shaderasset.hpp"

namespace asset
{
    ShaderAsset::ShaderAsset(dab::Shader&& data, const std::string& assetHash) :
        dab::Shader(std::move(data)),
        hash(assetHash)
    {
    }

    ShaderAsset::ShaderAsset(const dab::Shader& data, const std::string& assetHash) :
        dab::Shader(data),
        hash(assetHash)
    {
    }
}