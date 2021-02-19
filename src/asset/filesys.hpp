#pragma once

#include <array>
#include <filesystem>

#include "dab/import.hpp"
#include "dbg/log.hpp"

namespace dry::asset {

class filesystem final {
public:
    void add_file(const std::filesystem::path& dab_file);

    template<class T>
    std::pair<T, size_t> load_asset_whash(const std::string& name);

    void load_block(const std::filesystem::path& block_name);
    void unload_current_block();
    void clear();

private:
    struct asset_decl_strip {
        std::string name;
        size_t offset;
    };

    struct asset_block {
        std::filesystem::path path;
        std::array<std::vector<asset_decl_strip>, static_cast<size_t>(dab::asset_type::VALUE_COUNT)> assets;
    };

    std::vector<asset_block> _blocks;

    dab::dab_importer _importer;
    decltype(_blocks)::const_iterator _curr_block = _blocks.end();
};



template<class T>
std::pair<T, size_t> filesystem::load_asset_whash(const std::string& name) {
    constexpr size_t type_int = static_cast<size_t>(T::type);

    auto search_lambda = [&name](const asset_decl_strip& oth) {
        return oth.name == name;
    };
    decltype(asset_block::assets)::value_type::const_iterator asset_it;
    if (_curr_block != _blocks.end()) {
        asset_it = std::find_if(_curr_block->assets[type_int].begin(), _curr_block->assets[type_int].end(), search_lambda);
    }

    // NOTE : doing loading automatically, may delete it if not needed
    if (_curr_block == _blocks.end() || asset_it == _curr_block->assets[type_int].end()) {
        LOG_WRN("asset %s not found in loaded block, checking others", name.c_str());
        for (auto p = _blocks.begin(); p != _blocks.end(); ++p) {
            // sure whatever
            if (p == _curr_block) {
                continue;
            }               

            asset_it = std::find_if(p->assets[type_int].begin(), p->assets[type_int].end(), search_lambda);
            if (asset_it != p->assets[type_int].end()) {
                _curr_block = p;
                _importer.open(p->path);

                LOG_DBG("loaded asset block %s", p->path.filename().string().c_str());
                break;
            }
        }
        PANIC_ASSERT(asset_it != _blocks.back().assets[type_int].end(), "could not find asset %s", name.c_str());
    }

    // asset hash
    // higher 16 bits - block index in _blocks array, remaining 48 - offset in file
    static_assert(sizeof(size_t) == 8);
    const size_t block_ind = _curr_block - _blocks.begin();
    WRN_ASSERT(block_ind <= 0xFFFF, "asset %s has block index greater than 2^16, possible collision", name.c_str());

    size_t ret_hash = asset_it->offset;
    WRN_ASSERT(ret_hash <= 0xFFFFFFFFFFFF, "asset %s has dab offset greater than 2^48 bits, possible collision", name.c_str());
    ret_hash |= block_ind << (64 - 16);

    return { _importer.read<T>(asset_it->offset), ret_hash };
}

}
