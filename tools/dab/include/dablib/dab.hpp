#pragma once

#ifndef DAB_LIB_H
#define DAB_LIB_H

#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <fstream>

#include <int.hpp>

namespace dab {

using namespace dry_common;

constexpr u32_t folder_shader   = 0;
constexpr u32_t folder_texture  = 1;
constexpr u32_t folder_mesh     = 2;
constexpr u32_t folder_count    = 3;

struct dab_asset {
    std::string name;
    u64_t offset;
    u64_t size;
    u64_t mod_time;
};

struct dab_header {
    std::array<std::vector<dab_asset>, folder_count> folders;
    u64_t binary_offset;
};

inline dab_header parse_dab_header(std::ifstream& file);
inline u64_t write_dab_header(const dab_header& header, std::ofstream& file);

// Implementation

namespace impl {

/*
    ==== DAB STRUCTURE ====
        -- Header --
    3 bytes - 'D''A''B'
    1 u64   - payload offset/header size

    1 u64   - asset count           |
                                    |<====== times number of asset types
    1 u64   - offset                |   |
    1 u64   - length                |   |
    1 u64   - modify time(time_t)   |   |<== times number of assets of type
    1 u32   - name str size         |   |
    N bytes - name str              |   |

        -- Payload --
    - binaries
*/

constexpr std::string_view dab_magic{ "DAB" };

}

inline dab_header parse_dab_header(std::ifstream& file) {
    using namespace impl;

    dab_header ret_header;

    file.seekg(0, std::ios_base::beg);
    {
        std::array<char, dab_magic.size()> magic_buffer;
        file.read(magic_buffer.data(), dab_magic.size());
        if (std::memcmp(magic_buffer.data(), dab_magic.data(), dab_magic.size())) {
            throw std::runtime_error{ "Missing dab file signature" };
        }
    }

    file.read(reinterpret_cast<char*>(&ret_header.binary_offset), sizeof ret_header.binary_offset);

    auto header_bin = std::vector<std::byte>(ret_header.binary_offset - dab_magic.size() - sizeof(u64_t));
    const std::byte* header_it = header_bin.data();
    file.read(reinterpret_cast<char*>(header_bin.data()), header_bin.size());

    auto extract_from_bytestream_l = []<typename T>(T& val, const std::byte*& ptr) {
        val = *reinterpret_cast<const T*>(ptr);
        ptr += sizeof(T);
    };

    for (auto& folder : ret_header.folders) {
        u64_t size;
        extract_from_bytestream_l(size, header_it);
        
        folder.resize(size);

        for (auto& asset : folder) {
            extract_from_bytestream_l(asset.offset, header_it);
            asset.offset += ret_header.binary_offset;
            extract_from_bytestream_l(asset.size, header_it);
            extract_from_bytestream_l(asset.mod_time, header_it);

            u32_t name_len;
            extract_from_bytestream_l(name_len, header_it);
            asset.name = std::string{ reinterpret_cast<const char*>(header_it), reinterpret_cast<const char*>(header_it + name_len) };
            header_it += name_len;
        }
    }

    return ret_header;
}

inline u64_t write_dab_header(const dab_header& header, std::ofstream& file) {
    using namespace impl;

    file.seekp(0, std::ios_base::beg);
    file.write(dab_magic.data(), dab_magic.size());

    // file.seekp(sizeof(u64_t), std::ios_base::cur);
    u64_t header_size = sizeof(u64_t) * header.folders.size() + dab_magic.size() + sizeof(u64_t);
    file.write(reinterpret_cast<const char*>(&header_size), sizeof header_size);
    
    for (const auto& folder : header.folders) {
        const u64_t folder_size = static_cast<u64_t>(folder.size());
        file.write(reinterpret_cast<const char*>(&folder_size), sizeof folder_size);

        for (const auto& asset : folder) {
            // paranoid
            const u64_t mod_time = static_cast<u64_t>(asset.mod_time);
            const u32_t str_len = static_cast<u32_t>(asset.name.size());
            // TODO : writing to a buffer and then submitting should be better
            file.write(reinterpret_cast<const char*>(&asset.offset), sizeof asset.offset);
            file.write(reinterpret_cast<const char*>(&asset.size), sizeof asset.size);
            file.write(reinterpret_cast<const char*>(&mod_time), sizeof mod_time);
            file.write(reinterpret_cast<const char*>(&str_len), sizeof str_len);
            file.write(asset.name.data(), str_len);

            header_size += sizeof(u64_t) * 3 + sizeof(u32_t) + str_len;
        }
    }

    const u64_t end_pos = file.tellp();
    file.seekp(dab_magic.size(), std::ios_base::beg);
    file.write(reinterpret_cast<const char*>(&header_size), sizeof header_size);
    file.seekp(end_pos, std::ios_base::beg);
    return end_pos;
}

}

#endif