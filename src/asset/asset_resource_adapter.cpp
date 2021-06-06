#include "asset_resource_adapter.hpp"

namespace dry::asset {

asset_resource_adapter::asset_resource_adapter(asset_registry& asset_reg) :
    _asset_reg{ &asset_reg }
{
}

void asset_resource_adapter::attach_renderer_resource_registry(renderer_resource_registry& render_reg) {
    _renderer_reg = &render_reg;
}

void asset_resource_adapter::destroy_renderable(renderable_type renderable) {
    const auto deletion_infos = _renderer_reg->destroy_renderable(renderable);
    // TODO : don't like this switch on types that much
    auto backref_map_lambda = [this](renderer_resource_registry::resource_type type) {
        switch (type) {
        case renderer_resource_registry::resource_type::mesh: return &_mesh_backref;
        case renderer_resource_registry::resource_type::texture: return &_texture_backref;
        case renderer_resource_registry::resource_type::pipeline: return &_shader_backref;
        default: return decltype(&_mesh_backref){};
        }
    };

    for (const auto& deletion_info : deletion_infos) {
        if (deletion_info.type == renderer_resource_registry::resource_type::material) {
            const auto mat_index = std::get<renderer_resource_registry::material_index>(deletion_info.index);
            _material_asset_map.erase(_material_backref[mat_index]);
            _material_backref.erase(mat_index);
        }
        else {
            const auto asset_ind = std::get<renderer_resource_registry::index_type>(deletion_info.index);
            auto backref_ptr = backref_map_lambda(deletion_info.type);
            _material_asset_map.erase((*backref_ptr)[asset_ind]);
            backref_ptr->erase(asset_ind);
        }
    }
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
    _material_backref[ind] = hash;
    return ind;
}

}