#pragma once

#include "asset.hpp"

namespace dry::asset {

struct shader_asset : public asset_base<dab::shader>, public dab::shader {
    using asset_base<dab::shader>::hash;

    shader_asset() = default;
    shader_asset(type_t&& data, size_t asset_hash) :
        asset_base(asset_hash), dab::shader(std::move(data)) {}
    shader_asset(const type_t& data, size_t asset_hash) :
        asset_base(asset_hash), dab::shader(data) {}
};

}