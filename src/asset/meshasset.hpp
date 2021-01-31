#pragma once

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include "dab/import.hpp"

namespace asset
{
    struct MeshAsset
    {
        struct VertexData
        {
            // NOTE : keep alignment in mind
            glm::vec3 pos;
            glm::vec2 uv;
        };

        using type = dab::Mesh;

        MeshAsset() = default;
        MeshAsset(const type& data, const std::string& assetHash);

        std::vector<uint32_t> indices;
        std::vector<VertexData> vertices;

        const std::string hash;
    };
}