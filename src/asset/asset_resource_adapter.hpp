#pragma once

#ifndef DRY_ASSET_ASSET_RESOURCE_ADAPTER_H
#define DRY_ASSET_ASSET_RESOURCE_ADAPTER_H

#include "assetreg.hpp"
#include "graphics/renderer_resource_registry.hpp"

namespace dry::asset {

class asset_resource_adapter {
public:
    using index_type = renderer_resource_registry::index_type;
    using material_type = renderer_resource_registry::material_index;
    using renderable_type = renderer_resource_registry::renderable_index;

    template<typename Asset>
    struct asset_index { using type = index_type; };
    template<>
    struct asset_index<material_asset> { using type = material_type; };

    template<typename Asset>
    using asset_index_t = typename asset_index<Asset>::type;

    asset_resource_adapter() = default;
    asset_resource_adapter(asset_registry& asset_reg);
    void attach_renderer_resource_registry(renderer_resource_registry& render_reg);

    template<typename Asset>
    asset_index_t<Asset> get_resource_index(hash_t hash);

    renderable_type create_renderable(material_type material, index_type mesh) {
        return _renderer_reg->allocate_renderable(material, mesh);
    }
    void destroy_renderable(renderable_type renderable);
    void bind_renderable_transform(renderable_type rend, const model_transform& transform) {
        _renderer_reg->bind_renderable_transform(rend, transform);
    }

private:
    template<typename Asset>
    struct resource_binding;

    struct mat_hasher {
        size_t operator()(renderer_resource_registry::material_index mat) const noexcept {
            return (static_cast<size_t>(mat.pipeline) << 32) | static_cast<size_t>(mat.material);
        }
    };

    asset_registry* _asset_reg = nullptr;
    renderer_resource_registry* _renderer_reg = nullptr;

    std::unordered_map<hash_t, index_type> _renderer_asset_map;
    std::unordered_map<hash_t, renderer_resource_registry::material_index> _material_asset_map;

    std::unordered_map<index_type, hash_t> _texture_backref;
    std::unordered_map<index_type, hash_t> _mesh_backref;
    std::unordered_map<renderer_resource_registry::material_index, hash_t, mat_hasher> _material_backref;
    std::unordered_map<index_type, hash_t> _shader_backref;
};



template<>
struct asset_resource_adapter::resource_binding<mesh_asset> {
    static constexpr auto functor = &renderer_resource_registry::allocate_vertex_buffer;
    static constexpr auto backref = &asset_resource_adapter::_mesh_backref;
};
template<>
struct asset_resource_adapter::resource_binding<texture_asset> {
    static constexpr auto functor = &renderer_resource_registry::allocate_texture;
    static constexpr auto backref = &asset_resource_adapter::_texture_backref;
};
template<>
struct asset_resource_adapter::resource_binding<shader_asset> {
    static constexpr auto functor = &renderer_resource_registry::allocate_pipeline;
    static constexpr auto backref = &asset_resource_adapter::_shader_backref;
};

template<typename Asset>
asset_resource_adapter::asset_index_t<Asset> asset_resource_adapter::get_resource_index(hash_t hash) {
    if (_renderer_asset_map.contains(hash)) {
        return _renderer_asset_map[hash];
    }

    const auto& res = _asset_reg->get<Asset>(hash);
    const auto ind = (_renderer_reg->*resource_binding<Asset>::functor)(res);
    _renderer_asset_map[hash] = ind;
    (this->*resource_binding<Asset>::backref)[ind] = hash;

    return ind;
}

template<>
asset_resource_adapter::asset_index_t<material_asset> asset_resource_adapter::get_resource_index<material_asset>(hash_t hash);

}

#endif