#pragma once

#include "dab/import.hpp"

namespace asset
{
    struct ShaderAsset : public dab::Shader
    {
        using type = dab::Shader;

        ShaderAsset() = default;
        ShaderAsset(dab::Shader&& data, const std::string& assetHash);
        ShaderAsset(const dab::Shader& data, const std::string& assetHash);

        const std::string hash;
    };
}