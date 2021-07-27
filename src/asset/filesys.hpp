#pragma once

#ifndef DRY_FILESYS_H
#define DRY_FILESYS_H

#include <optional>
#include <filesystem>

#include <dablib/dab.hpp>

#include "asset_src.hpp"
#include "dbg/log.hpp"

namespace dry::asset {
// NOTE : hash collisions for different types will occur(T and U can have the same hash), keep in mind
class filesystem final {
public:
    filesystem();

    template<typename Asset>
    Asset load_asset(std::string_view name);
    template<typename Asset>
    Asset load_asset(hash_t hash);

    template<typename Asset>
    hash_t compute_hash(std::string_view name) const;

private:
    template<typename Asset>
    struct asset_dab_ind : std::integral_constant<u32_t, (std::numeric_limits<u32_t>::max)()> {};
    struct asset_eq_range_comp {
        bool operator()(std::string_view str, const dab::dab_asset& asset) const { return str < asset.name; }
        bool operator()(const dab::dab_asset& asset, std::string_view str) const { return asset.name < str; }
    };

    static constexpr std::string_view _root_dir = "assets";

    template<typename Asset>
    Asset load_asset(u32_t header, u32_t pos);

    template<typename Asset>
    byte_vec read_file(u32_t header, u32_t pos);
    void drop_archive();
    void open_archive(u32_t ind);

    struct header_info {
        std::filesystem::path path;
        u32_t hash_offset;
    };

    std::vector<dab::dab_header> _headers;
    std::vector<header_info> _header_infos;
    std::ifstream _dab_file;
    u32_t _curr_header;
};



template<> struct filesystem::asset_dab_ind<mesh_source> : std::integral_constant<u32_t, dab::folder_mesh> {};
template<> struct filesystem::asset_dab_ind<texture_source> : std::integral_constant<u32_t, dab::folder_texture> {};
template<> struct filesystem::asset_dab_ind<shader_source> : std::integral_constant<u32_t, dab::folder_shader> {};

template<> mesh_source filesystem::load_asset(u32_t header, u32_t pos);
template<> texture_source filesystem::load_asset(u32_t header, u32_t pos);
template<> shader_source filesystem::load_asset(u32_t header, u32_t pos);
template<> material_source filesystem::load_asset(u32_t header, u32_t pos); // TODO : nothing here yet

template<typename Asset>
Asset filesystem::load_asset(u32_t, u32_t) {
    static_assert(false, "Unsupported asset type");
}

template<typename Asset>
Asset filesystem::load_asset(std::string_view name) {
    for (auto i = 0u; i < _headers.size(); ++i) {
        const auto& folder = _headers[i].folders[asset_dab_ind<Asset>::value];
        const auto it = std::lower_bound(folder.begin(), folder.end(), name, asset_eq_range_comp{});
        if (it != folder.end() && name == it->name) {
            return load_asset<Asset>(i, static_cast<u32_t>(it - folder.begin()));
        }
    }

    LOG_ERR("Asset %s not found if filesystem", name.data());
    dbg::panic();
}

template<typename Asset>
Asset filesystem::load_asset(hash_t hash) {
    struct hash_eq_range_comp {
        bool operator()(hash_t hash, const header_info& header) const { return hash <= header.hash_offset; }
        bool operator()(const header_info& header, hash_t hash) const { return header.hash_offset <= hash; }
    };

    auto its = std::equal_range(_header_infos.begin(), _header_infos.end(), hash, hash_eq_range_comp{});
    if (its.first != _header_infos.begin() && its.first == its.second) {
        --its.first;
        return load_asset<Asset>(static_cast<u32_t>(its.first - _header_infos.begin()), hash - its.first->hash_offset);
    }

    LOG_ERR("Asset hash %i not found if filesystem", hash);
    dbg::panic();
}

template<typename Asset>
byte_vec filesystem::read_file(u32_t header, u32_t pos) {
    open_archive(header);

    const auto& asset = _headers[header].folders[asset_dab_ind<Asset>::value][pos];
    _dab_file.seekg(asset.offset, std::ios_base::beg);

    auto ret_bin = byte_vec(asset.size);
    _dab_file.read(reinterpret_cast<char*>(ret_bin.data()), asset.size);

    return ret_bin;
}

template<typename Asset>
hash_t filesystem::compute_hash(std::string_view name) const {
    for (auto i = 0u; i < _headers.size(); ++i) {
        const auto& folder = _headers[i].folders[asset_dab_ind<Asset>::value];
        const auto it = std::lower_bound(folder.begin(), folder.end(), name, asset_eq_range_comp{});
        if (it != folder.end() && it->name == name) {
            return static_cast<hash_t>(_header_infos[i].hash_offset + (it - folder.begin()));
        }
    }

    LOG_ERR("Could not compute hash for asset %s, asset not present", name.data());
    return null_hash_v;
}

}

#endif
