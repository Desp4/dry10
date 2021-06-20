#include "asset_resource_adapter.hpp"

namespace dry::asset {

asset_resource_adapter::asset_resource_adapter(asset_registry& asset_reg) :
    _asset_reg{ &asset_reg }
{
}

void asset_resource_adapter::attach_renderer(vulkan_renderer& renderer) {
    _renderer = &renderer;
}

template<>
asset_resource_adapter::index_type asset_resource_adapter::get_resource_index<material_asset>(hash_t hash) {
    if (_renderer_asset_map.contains(hash)) {
        return _renderer_asset_map[hash];
    }

    const auto& res = _asset_reg->get<material_asset>(hash);

    const index_type pipeline_ind = get_resource_index<shader_asset>(res.shader);

    std::vector<index_type> texture_inds;
    texture_inds.reserve(res.textures.size());
    for (auto texture : res.textures) {
        texture_inds.push_back(get_resource_index<texture_asset>(texture));
    }

    const auto ind = _renderer->create_material(pipeline_ind, texture_inds);
    _material_backref[ind] = hash;

    return ind;
}

}