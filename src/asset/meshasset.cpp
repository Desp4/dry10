#include "meshasset.hpp"

#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "dbg/log.hpp"

namespace asset
{
    static bool operator==(const MeshAsset::VertexData& l, const MeshAsset::VertexData& r)
    {
        return l.pos == r.pos && l.uv == r.uv;
    }

    struct VertexHash
    {
        size_t operator()(const MeshAsset::VertexData& vertex) const noexcept
        {
            return std::hash<glm::vec3>{}(vertex.pos) ^
                (std::hash<glm::vec2>{}(vertex.uv) << 1);
        }
    };

    MeshAsset::MeshAsset(const type& data, const std::string& assetHash) :
        hash(assetHash)
    {
        // looking for a 2d uv component
        const std::vector<uint8_t>* texCoordData = nullptr;
        for (const auto& texSet : data.texCoordSets)
        {
            if (texSet.componentCount == 2)
            {
                texCoordData = &texSet.texCoordData;
                break;
            }
        }
        // NOTE : maybe if not found just memset uv to 0?
        PANIC_ASSERT(texCoordData, "could not find a 2d texture set");

        const uint32_t vertexCount = data.vertexData.size() / sizeof(glm::vec3);
        indices.reserve(vertexCount);

        std::unordered_map<VertexData, uint32_t, VertexHash> uniqueVerts;
        for (int i = 0; i < vertexCount; ++i)
        {
            VertexData vertex{};
            std::memcpy(&vertex.pos, data.vertexData.data() + i * sizeof vertex.pos, sizeof vertex.pos);
            std::memcpy(&vertex.uv, texCoordData->data() + i * sizeof vertex.uv, sizeof vertex.uv);
            // NOTE : flipping coord for vulkan, might do that beforehand in dab
            vertex.uv.y = 1 - vertex.uv.y;

            if (uniqueVerts.count(vertex) == 0)
            {
                uniqueVerts[vertex] = vertices.size();
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVerts[vertex]);
        }
    }
}