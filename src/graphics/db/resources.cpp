#include "resources.hpp"
// TODO : this is a placeholder tmp file
namespace gr::res
{
    void writeDefVertex(std::span<const glm::vec3> pos, std::span<const glm::vec2> uv, std::vector<pcsr::def::VertexInput>& outVert, std::vector<uint32_t>& outInd)
    {
        // O(n^2) boo
        outInd.resize(pos.size());
        for (uint32_t i = 0; i < pos.size(); ++i)
        {
            bool dup = false;
            for (uint32_t j = 0; j < outVert.size(); ++j)
            {
                if (outVert[j].inPosition == pos[i] && outVert[j].inUV == uv[i])
                {
                    dup = true;
                    outInd[i] = j;
                    break;
                }
            }
            if (!dup)
            {
                outInd[i] = outVert.size();
                outVert.push_back({ pos[i], uv[i] });
            }
        }
    }
}