#include "assetreg.hpp"

namespace dry::asset {

void asset_registry::unload_all() {
    _asset_pools.clear();
}

void asset_registry::add_resource_block(const std::filesystem::path& dab_file) {
    _filesys.add_file(dab_file);
}

void asset_registry::load_block(const std::filesystem::path& block) {
    _filesys.load_block(block);
}

void asset_registry::unload_current_block() {
    _filesys.unload_current_block();
}

void asset_registry::clear_all() {
    _filesys.clear();
    unload_all();
}

}