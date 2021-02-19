#pragma once

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include "asset.hpp"

namespace dry::asset {

struct mesh_asset : public asset_base<dab::mesh> {
    using asset_base<dab::mesh>::hash;
    struct vertex_data {
        // NOTE : keep alignment in mind
        glm::vec3 pos;
        glm::vec2 uv;
    };

    mesh_asset() = default;
    mesh_asset(const type_t& data, size_t asset_hash);

    std::vector<uint32_t> indices;
    std::vector<vertex_data> vertices;
};

}