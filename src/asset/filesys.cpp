#include "filesys.hpp"

namespace dry::asset {

void filesystem::add_file(const std::filesystem::path& dab_file) {
    const auto curr_block_ind = _curr_block == _blocks.end() ? -1 : _curr_block - _blocks.begin();

    if (_importer.open(dab_file)) {
        asset_block& new_block = _blocks.emplace_back(asset_block());
        new_block.path = dab_file;
        const auto declarations = _importer.declarations();
        for (const auto& declaration : declarations) {
            new_block.assets[static_cast<size_t>(declaration.type)].emplace_back(declaration.name, declaration.offset);
        }

        _importer.close();
        // NOTE : path defaults to wchar_t on win, have to convert to string sadly
        LOG_DBG("asset added %s", dab_file.string().c_str());
    }
    else {
        LOG_WRN("invalid asset type %s", dab_file.string().c_str());
    }

    // to preserve correct iterator on realloc
    _curr_block = curr_block_ind == -1 ? _blocks.end() : _blocks.begin() + curr_block_ind;
}

void filesystem::load_block(const std::filesystem::path& block_name) {
    const auto block_it = std::find_if(_blocks.begin(), _blocks.end(),
        [&block_name](const asset_block& oth) {
            return oth.path == block_name;
        }
    );

    PANIC_ASSERT(block_it != _blocks.end(), "block name %s is not present", block_name.string().c_str());
    if (block_it == _curr_block) {
        LOG_WRN("block %s already loaded", block_it->path.filename().string().c_str());
        return;
    }

    PANIC_ASSERT(_importer.open(block_name), "could not open asset block %s", block_it->path.string().c_str());

    _curr_block = block_it;
    LOG_DBG("loaded asset block %s", block_it->path.filename().string().c_str());
}

void filesystem::unload_current_block() {
    if (_curr_block == _blocks.end()) {
        LOG_WRN("no block to unload");
        return;
    }

    _importer.close();
    LOG_DBG("unloaded asset block %s", _curr_block->path.filename().string().c_str());
    _curr_block = _blocks.end();
}

void filesystem::clear() {
    _importer.close();
    _blocks.clear();
    _curr_block = _blocks.end();
}

}
