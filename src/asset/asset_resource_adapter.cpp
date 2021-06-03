#include "asset_resource_adapter.hpp"

namespace dry::asset {

asset_resource_adapter::asset_resource_adapter(asset_registry& asset_reg) :
    _asset_reg{ &asset_reg }
{
}

void asset_resource_adapter::attach_renderer_resource_registry(renderer_resource_registry& render_reg) {
    _renderer_reg = &render_reg;
}

template<>
asset_resource_adapter::asset_index_t<material_asset> asset_resource_adapter::get_resource_index<material_asset>(hash_t hash) {
    // TODO : materials warrant an actual configuration check in case identical one already exists, only checking hashes now
    if (_material_asset_map.contains(hash)) {
        return _material_asset_map[hash];
    }

    const auto& mat = _asset_reg->get<material_asset>(hash);
    const index_type shader_ind = get_resource_index<shader_asset>(mat.shader);
    std::vector<index_type> textures;

    textures.reserve(mat.textures.size());
    for (auto texture : mat.textures) {
        textures.push_back(get_resource_index<texture_asset>(texture));
    }
    const auto ind = _renderer_reg->allocate_material(shader_ind, textures);
    _material_asset_map[hash] = ind;
    return ind;
}

}