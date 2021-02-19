#pragma once

#include "asset.hpp"

namespace dry::asset {

struct texture_asset : public asset_base<dab::texture>, public dab::texture {
    using asset_base<dab::texture>::hash;

    texture_asset() = default;
    texture_asset(type_t&& data, size_t asset_hash) :
        asset_base(asset_hash), dab::texture(std::move(data)) {}

    texture_asset(const type_t& data, size_t asset_hash) :
        asset_base(asset_hash), dab::texture(data) {}

    constexpr VkFormat texture_format() const noexcept {
        switch (channels) {
        case 1:  return VK_FORMAT_R8_SRGB;
        case 2:  return VK_FORMAT_R8G8_SRGB;
        case 3:  return VK_FORMAT_R8G8B8_SRGB;
        case 4:  return VK_FORMAT_R8G8B8A8_SRGB;
        default: return VK_FORMAT_UNDEFINED;
        }
    }
};

}