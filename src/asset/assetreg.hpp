#pragma once

#include <unordered_map>
#include <any>

#include "util/type.hpp"
#include "filesys.hpp"
#include "asset.hpp"

namespace dry::asset {

template<class T>
concept asset_constructible = std::is_base_of_v<asset_base<typename T::type_t>, T> && requires() {
    T{ typename T::type_t{}, size_t{} };
};

// NOTE : *specifically* for STD hash map references are always persistent
class asset_registry {
public:
    template<class T>
    using hashmap_pool = std::unordered_map<std::string, T>;
    template<class T>
    using asset_type_id = util::type_id<T, 0>;

    template<asset_constructible T>
    const T& get(const std::string& name);
    template<asset_constructible T>
    void load(const std::string& name);
    template<asset_constructible T>
    void unload(const std::string& name);
    void unload_all();

    void add_resource_block(const std::filesystem::path& dab_file);
    void load_block(const std::filesystem::path& block);
    void unload_current_block();

    void clear_all();

private:
    filesystem _filesys;
    // NOTE : not expected to loop through it so just store the base ptr
    std::vector<std::any> _asset_pools;
};



template<asset_constructible T>
const T& asset_registry::get(const std::string& name) {
    const auto t_id = asset_type_id<T>::value();
    //PANIC_ASSERT(t_id < _asset_pools.size(), "asset type pool for %s not allocated", name.c_str()); // auto loading types, don't panic yet

    if (t_id >= _asset_pools.size() || !std::any_cast<hashmap_pool<T>&>(_asset_pools[t_id]).contains(name)) {
        // NOTE : might want to not auto load perhaps
        LOG_WRN("asset %s not loaded into the registry, loading", name.c_str());
        load<T>(name); // NOTE : to avoid logging, meh
    }

    return std::any_cast<hashmap_pool<T>&>(_asset_pools[t_id])[name];
}

template<asset_constructible T>
void asset_registry::load(const std::string& name) {   
    const auto t_id = asset_type_id<T>::value();

    if (t_id >= _asset_pools.size()) {
        _asset_pools.resize(_asset_pools.size() + 1);
        _asset_pools[t_id] = hashmap_pool<T>{};
    }

    auto& hashmap = std::any_cast<hashmap_pool<T>&>(_asset_pools[t_id]);
    if (hashmap.contains(name)) {
        LOG_WRN("asset %s already loaded in", name.c_str());
        return;
    }

    // NOTE : relying on filesystem autoloading block
    auto&& asset_data = _filesys.load_asset_whash<T::type_t>(name);

    // T implements constructor with args of underlying dab type and hash value
    hashmap.insert(std::make_pair(name, T(std::move(asset_data.first), asset_data.second)));
    //_filesys.unload_block(); // don't unload 
    LOG_DBG("asset %s loaded", name.c_str());
}

template<asset_constructible T>
void asset_registry::unload(const std::string& name) {
    const auto t_id = asset_type_id<T>::value();
    PANIC_ASSERT(t_id < _asset_pools.size(), "attempting to unload asset %s from a not allocated pool", name.c_str());

    auto& hashmap = std::any_cast<hashmap_pool<T>&>(_asset_pools[t_id]);
    const auto asset_it = hashmap.find(name);
    PANIC_ASSERT(asset_it != hashmap.end(), "attempting to unload asset %s that is not loaded", name.c_str());

    hashmap.erase(asset_it);
    LOG_DBG("asset %s unloaded", name.c_str());
}

}