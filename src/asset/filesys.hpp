#pragma once

#ifndef DRY_FILESYS_H
#define DRY_FILESYS_H

#include <filesystem>
#include <unordered_map>
#include <optional>

#include <zip.h>

#include "asset_src.hpp"
#include "dbg/log.hpp"

namespace dry::asset {

class filesystem final {
public:
    filesystem();
    filesystem(filesystem&& oth) { *this = std::move(oth); }
    ~filesystem();

    template<typename Asset>
    Asset load_asset(const std::string& name);
    template<typename Asset>
    Asset load_asset(hash_t hash);

    template<typename Asset>
    hash_t compute_hash(const std::string& name) const;

    filesystem& operator=(filesystem&& oth);

private:
    struct asset_dab_location {
        u32_t dab_index;
        u32_t asset_index;
    };
    struct dab_arch_data {
        std::string path;
        u32_t start_hash;
    };

    static constexpr std::string_view _root_dir = "assets";

    std::optional<byte_vec> read_file(hash_t hash);
    std::optional<byte_vec> read_file(asset_dab_location location);
    void drop_archive();
    void open_archive(std::string_view path);
    void open_archive(u32_t ind);

    std::optional<asset_dab_location> consume_hash(hash_t hash) const;

    u32_t _arch_ind;
    zip_t* _arch_handle;

    std::vector<dab_arch_data> _dab_files;
    std::unordered_map<std::string, asset_dab_location> _asset_map;
};

template<typename Asset>
Asset filesystem::load_asset(const std::string& name) {
    return load_asset(compute_hash<Asset>(name));
}

template<>
mesh_source filesystem::load_asset(hash_t hash);
template<>
texture_source filesystem::load_asset(hash_t hash);
template<>
shader_source filesystem::load_asset(hash_t hash);
template<>
material_source filesystem::load_asset(hash_t hash); // TODO : nothing here yet

template<typename Asset>
Asset filesystem::load_asset(hash_t hash) {
    static_assert(false, "Unsupported asset type");
}

template<typename Asset>
hash_t filesystem::compute_hash(const std::string& name) const {
    const std::string full_name = name + asset_source_ext_v<Asset>.data();
    if (!_asset_map.contains(full_name)) {
        LOG_ERR("Asset %s registered as %s not found in filesystem", name.data(), full_name.data());
        return null_hash_v;
    }

    const auto asset_inds = _asset_map.at(full_name);
    return static_cast<hash_t>(_dab_files[asset_inds.dab_index].start_hash + asset_inds.asset_index);
}

}

#endif
