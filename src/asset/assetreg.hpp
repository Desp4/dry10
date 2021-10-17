#pragma once

#ifndef DRY_ASSETREG_H
#define DRY_ASSETREG_H

#include <unordered_map>
#include <any>

#include "util/type.hpp"
#include "util/sparse_table.hpp"
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
    asset_registry() = default;
    asset_registry(const asset_registry&) = delete;
    asset_registry& operator=(const asset_registry&) = delete;

    template<class T>
    using hashmap_pool = std::unordered_map<hash_t, T>;
    template<class T>
    using asset_type_id = util::type_id<T, asset_registry>;
    // TODO : string_view overload please
    template<typename Asset> requires is_hashed_asset<Asset>::value
    Asset& get(hash_t hash);
    template<typename Asset> requires is_hashed_asset<Asset>::value
    Asset& get(const std::string& name);

    template<typename Asset> requires is_hashed_asset<Asset>::value
    void load(hash_t hash);
    template<typename Asset> requires is_hashed_asset<Asset>::value
    void load(const std::string& name);

    template<typename Asset> requires is_hashed_asset<Asset>::value
    void unload(hash_t hash);
    template<typename Asset> requires is_hashed_asset<Asset>::value
    void unload(const std::string& name);

    template<typename Asset, typename... Ts> requires is_hashed_asset<Asset>::value
    const Asset& create(Ts&&... args);

    void unload_all() { _asset_pools.clear(); }

private:
    template<typename Asset>
    void assure_pool_size();

    filesystem _filesys;

    std::vector<std::any> _asset_pools;
    // TODO: sparse type
    sparse_table<u64_t> _runtime_asset_indices;

    static constexpr hash_t _runtime_hash_flag = 1 << (sizeof(hash_t) * 8 - 1);
};



template<typename Asset> requires is_hashed_asset<Asset>::value
Asset& asset_registry::get(hash_t hash) {
    static const auto t_id = asset_type_id<Asset>::value();

    assure_pool_size<Asset>();

    auto& hashmap = std::any_cast<hashmap_pool<Asset>&>(_asset_pools[t_id]);

    if (!hashmap.contains(hash)) {
        if (hash & _runtime_hash_flag) {
            LOG_ERR("Runtime hash %i not found in registry", hash);
            dbg::panic(); // TODO : can throw a fallback
        } else {
            if constexpr (filesystem::asset_serializable<is_hashed_asset<Asset>::type>()) {
                load<Asset>(hash);
            } else {
                LOG_ERR("Asset type is not serializable, only calls to get with runtime hash values are supported");
                dbg::panic();
            }
        }
    }

    return hashmap[hash];
}

template<typename Asset> requires is_hashed_asset<Asset>::value
Asset& asset_registry::get(const std::string& name) {
    using underlying = typename is_hashed_asset<Asset>::type;
    return get<Asset>(_filesys.compute_hash<underlying>(name));
}

template<typename Asset> requires is_hashed_asset<Asset>::value
void asset_registry::load(hash_t hash) {
    using underlying = typename is_hashed_asset<Asset>::type;
    static const auto t_id = asset_type_id<Asset>::value();

    if (hash & _runtime_hash_flag) {
        LOG_ERR("Can't load an asset with a runtime hash %i", hash);
        dbg::panic();
    }

    assure_pool_size<Asset>();

    auto& hashmap = std::any_cast<hashmap_pool<Asset>&>(_asset_pools[t_id]);
    if (hashmap.contains(hash)) {
        LOG_INF("Asset with hash %i already loaded in", hash);
        return;
    }

    // NOTE : relying on filesystem autoloading block
    hashmap.insert(std::make_pair(
        hash, Asset{ _filesys.load_asset<underlying>(hash), hash }
    ));
}

template<typename Asset> requires is_hashed_asset<Asset>::value
void asset_registry::load(const std::string& name) {
    using underlying = typename is_hashed_asset<Asset>::type;
    load<Asset>(_filesys.compute_hash<underlying>(name));
}

template<typename Asset> requires is_hashed_asset<Asset>::value
void asset_registry::unload(hash_t hash) {
    static const auto t_id = asset_type_id<Asset>::value();
    if (t_id >= _asset_pools.size()) {
        LOG_ERR("Attempting to unload asset %i from a not allocated pool", hash);
        return;
    }

    auto& hashmap = std::any_cast<hashmap_pool<Asset>&>(_asset_pools[t_id]);
    const auto asset_it = hashmap.find(hash);
    if (asset_it == hashmap.end()) {
        LOG_ERR("Attempting to unload asset %i that is not present", hash);
        return;
    }

    if (hash & _runtime_hash_flag) {
        _runtime_asset_indices.remove(hash & (~_runtime_hash_flag));
    }

    hashmap.erase(asset_it);
}

template<typename Asset> requires is_hashed_asset<Asset>::value
void asset_registry::unload(const std::string& name) {
    using underlying = typename is_hashed_asset<Asset>::type;
    return unload<Asset>(_filesys.compute_hash<underlying>(name));
}

template<typename Asset, typename... Ts> requires is_hashed_asset<Asset>::value
const Asset& asset_registry::create(Ts&&... args) {
    using underlying_t = is_hashed_asset<Asset>::type;
    static const auto t_id = asset_type_id<Asset>::value();

    const hash_t runtime_hash = _runtime_hash_flag | static_cast<hash_t>(_runtime_asset_indices.emplace(0));

    assure_pool_size<Asset>();

    auto& hashmap = std::any_cast<hashmap_pool<Asset>&>(_asset_pools[t_id]);
    return hashmap.insert(std::make_pair(runtime_hash, Asset{ underlying_t{ std::forward<Ts>(args)... }, runtime_hash })).first->second;
}

template<typename Asset>
void asset_registry::assure_pool_size() {
    static const auto t_id = asset_type_id<Asset>::value();

    if (t_id >= _asset_pools.size()) {
        _asset_pools.resize(t_id + 1);
        _asset_pools[t_id] = hashmap_pool<Asset>{};
    }
}

}

#endif
