#pragma once

#ifndef DRY_ASSET_ASSET_RESOURCE_ADAPTER_H
#define DRY_ASSET_ASSET_RESOURCE_ADAPTER_H

#include "assetreg.hpp"
#include "graphics/renderer_resource_registry.hpp"

namespace dry::asset {

class asset_resource_adapter {
public:
    using index_type = renderer_resource_registry::index_type;

    template<typename Asset>
    struct asset_index { using type = index_type; };
    template<>
    struct asset_index<material_asset> { using type = renderer_resource_registry::material_index; };

    template<typename Asset>
    using asset_index_t = typename asset_index<Asset>::type;

    asset_resource_adapter() = default;
    asset_resource_adapter(asset_registry& asset_reg);
    void attach_renderer_resource_registry(renderer_resource_registry& render_reg);

    template<typename Asset>
    asset_index_t<Asset> get_resource_index(hash_t hash);

    auto create_renderable(renderer_resource_registry::material_index material, renderer_resource_registry::index_type mesh) {
        return _renderer_reg->allocate_renderable(material, mesh);
    };

private:
    template<typename Asset>
    struct resource_binding;

    asset_registry* _asset_reg = nullptr;
    renderer_resource_registry* _renderer_reg = nullptr;

    std::unordered_map<hash_t, index_type> _renderer_asset_map;
    std::unordered_map<hash_t, renderer_resource_registry::material_index> _material_asset_map;
};



template<>
struct asset_resource_adapter::resource_binding<mesh_asset> {
    static constexpr auto functor = &renderer_resource_registry::allocate_vertex_buffer;
};
template<>
struct asset_resource_adapter::resource_binding<texture_asset> {
    static constexpr auto functor = &renderer_resource_registry::allocate_texture;
};
template<>
struct asset_resource_adapter::resource_binding<shader_asset> {
    static constexpr auto functor = &renderer_resource_registry::allocate_pipeline;
};

template<typename Asset>
asset_resource_adapter::asset_index_t<Asset> asset_resource_adapter::get_resource_index(hash_t hash) {
    if (_renderer_asset_map.contains(hash)) {
        return _renderer_asset_map[hash];
    }

    const auto& res = _asset_reg->get<Asset>(hash);
    // TODO : botched up fix
    if constexpr (std::is_same_v<Asset, shader_asset>) {
        const auto ind = _renderer_reg->allocate_pipeline(res);
        _renderer_asset_map[hash] = ind;
        return ind;
    }
    else {
        const auto ind = (_renderer_reg->*resource_binding<Asset>::functor)(res);
        _renderer_asset_map[hash] = ind;
        return ind;
    }
}

template<>
asset_resource_adapter::asset_index_t<material_asset> asset_resource_adapter::get_resource_index<material_asset>(hash_t hash);

}

#endif