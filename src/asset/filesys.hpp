#pragma once

#ifndef DRY_FILESYS_H
#define DRY_FILESYS_H

#include <filesystem>
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

    bool open_archive(const std::filesystem::path& path);
    bool drop_archive();

    template<typename Asset>
    Asset load_asset(const std::string& name);
    template<typename Asset>
    hash_t compute_hash(const std::string& name);

    filesystem& operator=(filesystem&& oth);

private:
    static constexpr std::string_view _fallback_path = "res/asset_fallback.zip"; // has to be with the exe
    static constexpr std::string_view _fallback_name = "fb";
    static filesystem& fallback_fs();

    std::optional<byte_vec> read_file(std::string_view name);
    std::optional<byte_vec> read_fallback(std::string_view ext);
    template<typename Asset>
    hash_t fallback_hash();

    std::string _arch_path; // for ease of debug
    zip_t* _arch_handle;
    u32_t _arch_id;
};



template<>
mesh_source filesystem::load_asset(const std::string&);
template<>
texture_source filesystem::load_asset(const std::string&);
template<>
shader_source filesystem::load_asset(const std::string&);

template<typename Asset>
Asset filesystem::load_asset(const std::string&) {
    static_assert(false, "Unsupported asset type");
}

template<typename Asset>
hash_t filesystem::compute_hash(const std::string& name) {
    const std::string full_name = name + asset_source_ext_v<Asset>.data();

    const zip_int64_t ind = zip_name_locate(_arch_handle, full_name.data(), ZIP_FL_ENC_GUESS);
    if (ind < 0) {
        LOG_ERR("Asset %s not found", full_name.data());
        return fallback_hash<Asset>();
    }
    else if (ind > (std::numeric_limits<u32_t>::max)()) {
        LOG_WRN("Asset %s index overflow, hash collision possible", full_name.data());
    }

    return (static_cast<hash_t>(_arch_id) << 32) | ind; // NOTE : assume hash is 64, otherwise it introduces collisions anyway
}

template<typename Asset>
hash_t filesystem::fallback_hash() {
    constexpr hash_t id_mask = static_cast<hash_t>((std::numeric_limits<u32_t>::max)()) << 32;

    const std::string full_name = std::string{ _fallback_name } + asset_source_ext_v<Asset>.data();
    const zip_int64_t ind = zip_name_locate(_arch_handle, full_name.data(), ZIP_FL_ENC_GUESS);
    if (ind < 0) {
        LOG_ERR("No fallback asset %s found", full_name.data());
        dbg::panic();
    }
    else if (ind > (std::numeric_limits<u32_t>::max)()) {
        LOG_WRN("Asset %s index overflow, hash collision possible", full_name.data()); // to be safe
    }
    return (ind & id_mask); // don't care about archive id, fill with 1s
}

}

#endif
