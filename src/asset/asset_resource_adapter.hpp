#pragma once

#ifndef DRY_ASSET_ASSET_RESOURCE_ADAPTER_H
#define DRY_ASSET_ASSET_RESOURCE_ADAPTER_H

#include "assetreg.hpp"
#include "graphics/renderer.hpp"

namespace dry::asset {

class asset_resource_adapter {
public:
    using index_type = vulkan_renderer::resource_id;

    asset_resource_adapter() = default;
    asset_resource_adapter(asset_registry& asset_reg) : _asset_reg{ &asset_reg } {}
    void attach_renderer(vulkan_renderer& renderer) { _renderer = &renderer; }

    // TODO : by name would be nice to have
    // TODO : don't really like the overload
    template<typename Asset> requires (!std::is_same_v<Asset, material_asset>)
    index_type get_resource_index(hash_t hash);
    template<typename Asset, typename Material> requires std::is_same_v<Asset, material_asset>
    index_type get_resource_index(hash_t hash);


private:
    template<typename Asset>
    struct resource_binding;

    using backref_map = std::unordered_map<index_type, hash_t>;

    asset_registry* _asset_reg = nullptr;
    vulkan_renderer* _renderer = nullptr;

    std::unordered_map<hash_t, index_type> _renderer_asset_map;

    backref_map _texture_backref;
    backref_map _mesh_backref;
    backref_map _material_backref;
    backref_map _shader_backref;
};



template<>
struct asset_resource_adapter::resource_binding<mesh_asset> {
    static constexpr auto functor = &vulkan_renderer::create_mesh;
    static constexpr auto backref = &asset_resource_adapter::_mesh_backref;
};
template<>
struct asset_resource_adapter::resource_binding<texture_asset> {
    static constexpr auto functor = &vulkan_renderer::create_texture;
    static constexpr auto backref = &asset_resource_adapter::_texture_backref;
};
template<>
struct asset_resource_adapter::resource_binding<shader_asset> {
    static constexpr auto functor = &vulkan_renderer::create_shader;
    static constexpr auto backref = &asset_resource_adapter::_shader_backref;
};

template<typename Asset> requires (!std::is_same_v<Asset, material_asset>)
asset_resource_adapter::index_type asset_resource_adapter::get_resource_index(hash_t hash) {
    if (_renderer_asset_map.contains(hash)) {
        return _renderer_asset_map[hash];
    }

    const auto& res = _asset_reg->get<Asset>(hash);
    const auto ind = (_renderer->*resource_binding<Asset>::functor)(res);
    _renderer_asset_map[hash] = ind;
    (this->*resource_binding<Asset>::backref)[ind] = hash;

    return ind;
}

template<typename Asset, typename Material> requires std::is_same_v<Asset, material_asset>
asset_resource_adapter::index_type asset_resource_adapter::get_resource_index(hash_t hash) {
    if (_renderer_asset_map.contains(hash)) {
        return _renderer_asset_map[hash];
    }

    const auto& res = _asset_reg->get<material_asset>(hash);

    const index_type pipeline_ind = get_resource_index<shader_asset>(res.shader);

    const auto ind = _renderer->create_material<Material>(pipeline_ind, *dynamic_cast<Material*>(res.material.get()));
    _material_backref[ind] = hash;

    return ind;
}

}

#endif