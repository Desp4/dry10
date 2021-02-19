#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "meshasset.hpp"
#include "dbg/log.hpp"

namespace dry::asset {

static bool operator==(const mesh_asset::vertex_data& l, const mesh_asset::vertex_data& r) {
    return l.pos == r.pos && l.uv == r.uv;
}

struct vertex_hash {
    size_t operator()(const mesh_asset::vertex_data& vertex) const noexcept {
        return std::hash<glm::vec3>{}(vertex.pos) ^ (std::hash<glm::vec2>{}(vertex.uv) << 1);
    }
};

mesh_asset::mesh_asset(const type_t& data, size_t asset_hash) :
    asset_base(asset_hash)
{
    // looking for a 2d uv component
    const std::vector<uint8_t>* tex_coord_data = nullptr;
    for (const auto& tex_set : data.tex_coord_sets) {
        if (tex_set.component_count == 2) {
            tex_coord_data = &tex_set.tex_data;
            break;
        }
    }
    // NOTE : maybe if not found just memset uv to 0?
    PANIC_ASSERT(tex_coord_data, "could not find a 2d texture set");

    const uint32_t vertex_count = data.vertex_data.size() / sizeof(glm::vec3);
    indices.reserve(vertex_count);

    std::unordered_map<vertex_data, uint32_t, vertex_hash> unique_verts;
    for (int i = 0; i < vertex_count; ++i) {
        vertex_data vertex{};
        std::memcpy(&vertex.pos, data.vertex_data.data() + i * sizeof vertex.pos, sizeof vertex.pos);
        std::memcpy(&vertex.uv, tex_coord_data->data() + i * sizeof vertex.uv, sizeof vertex.uv);
        // NOTE : flipping coord for vulkan, might do that beforehand in dab
        vertex.uv.y = 1 - vertex.uv.y;

        if (!unique_verts.contains(vertex)) {
            unique_verts[vertex] = vertices.size();
            vertices.push_back(vertex);
        }
        indices.push_back(unique_verts[vertex]);
    }
}

}