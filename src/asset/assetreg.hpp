#pragma once

#ifndef DRY_ASSETREG_H
#define DRY_ASSETREG_H

#include <unordered_map>
#include <any>

#include "util/type.hpp"
#include "filesys.hpp"

namespace dry::asset {

template<typename>
struct is_hashed_asset : std::false_type {};

template<typename T>
struct is_hashed_asset<hashed_asset<T>> : std::true_type {
    using type = T;
};

// NOTE : hash map references have to be persistent
class asset_registry final {
public:
    template<class T>
    using hashmap_pool = std::unordered_map<std::string, T>;
    template<class T>
    using asset_type_id = util::type_id<T, asset_registry>;

    template<typename Asset> requires is_hashed_asset<Asset>::value
    const Asset& get(const std::string& name);

    template<typename Asset> requires is_hashed_asset<Asset>::value
    void load(const std::string& name);

    template<typename Asset> requires is_hashed_asset<Asset>::value
    void unload(const std::string& name);

    void unload_all() {
        _asset_pools.clear();
    }

    // let those two fail, will fill loads with placeholders
    void load_archive(const std::filesystem::path& path) {
        _filesys.open_archive(path);
    }
    void drop_archive() {
        _filesys.drop_archive();
    }

private:
    filesystem _filesys;
    // NOTE : not expected to loop through it so just store the base ptr
    std::vector<std::any> _asset_pools;
};



template<typename Asset> requires is_hashed_asset<Asset>::value
const Asset& asset_registry::get(const std::string& name) {
    static const auto t_id = asset_type_id<Asset>::value();

    if (t_id >= _asset_pools.size() || !std::any_cast<hashmap_pool<Asset>&>(_asset_pools[t_id]).contains(name)) {
        load<Asset>(name); // NOTE : to avoid logging, meh
    }

    return std::any_cast<hashmap_pool<Asset>&>(_asset_pools[t_id])[name];
}

template<typename Asset> requires is_hashed_asset<Asset>::value
void asset_registry::load(const std::string& name) {
    using underlying = typename is_hashed_asset<Asset>::type;
    static const auto t_id = asset_type_id<Asset>::value();

    if (t_id >= _asset_pools.size()) {
        _asset_pools.resize(t_id + 1);
        _asset_pools[t_id] = hashmap_pool<Asset>{};
    }

    auto& hashmap = std::any_cast<hashmap_pool<Asset>&>(_asset_pools[t_id]);
    if (hashmap.contains(name)) {
        LOG_INF("Asset %s already loaded in", name.data());
        return;
    }

    // NOTE : relying on filesystem autoloading block
    hashmap.insert(std::make_pair(
        name, Asset{ _filesys.load_asset<underlying>(name), _filesys.compute_hash<underlying>(name) }
    ));
    LOG_DBG("Asset %s loaded", name.data());
}

template<typename Asset> requires is_hashed_asset<Asset>::value
void asset_registry::unload(const std::string& name) {
    const auto t_id = asset_type_id<Asset>::value();
    if (t_id >= _asset_pools.size()) {
        LOG_ERR("Attempting to unload asset %s from a not allocated pool", name.data());
        dbg::panic();
    }

    auto& hashmap = std::any_cast<hashmap_pool<Asset>&>(_asset_pools[t_id]);
    const auto asset_it = hashmap.find(name);
    if (asset_it == hashmap.end()) {
        LOG_ERR("Attempting to unload asset %s that is not loaded", name.data());
        dbg::panic();
    }

    hashmap.erase(asset_it);
    LOG_DBG("Asset %s unloaded", name.data());
}

}

#endif
