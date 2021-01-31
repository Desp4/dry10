#pragma once

#include <unordered_map>
#include <variant>

#include "filesys.hpp"
#include "dbg/log.hpp"

#include "meshasset.hpp"
#include "shaderasset.hpp"
#include "texasset.hpp"

namespace asset
{
    // NOTE : *specifically* for STD hash map references are always persistent
    class AssetRegistry
    {
    public:
        inline static Filesystem filesystem;

        template<class T>
        const T& get(const std::string& name);
        template<class T>
        void load(const std::string& name);
        template<class T>
        void unload(const std::string& name);

    private:
        // NOTE : USAGE : for every new asset, add it to the variant list
        using AssetVariant = std::variant<std::monostate, MeshAsset, ShaderAsset, TextureAsset>;

        using AssetPair = std::pair<std::string, dab::AssetType>;
        struct AssetHash
        {
            size_t operator()(AssetPair pair) const noexcept
            {
                // TODO : lazy xor hash
                return std::hash<std::string>{}(pair.first) ^ pair.second;
            }
        };
        // TODO : don't like it as much honestly
        std::unordered_map<AssetPair, AssetVariant, AssetHash> _assets;
    };



    template<class T>
    const T& AssetRegistry::get(const std::string& name)
    {
        const AssetPair pair = { name, T::type::type };
        if (!_assets.contains(pair))
        {
            LOG_WRN("asset %s not loaded into the registry, loading", name.c_str());
            load<T>(name); // NOTE : to avoid logging, meh
        }
        return std::get<T>(_assets[pair]);
    }

    template<class T>
    void AssetRegistry::load(const std::string& name)
    {
        const AssetPair pair = { name, T::type::type };
        if (_assets.contains(pair))
        {
            LOG_WRN("asset %s already loaded in", name.c_str());
            return;
        }

        // NOTE : relying on filesystem autoloading block
        auto&& assetData = filesystem.loadAsset<T::type>(name);

        _assets.insert(std::make_pair(pair, T(std::move(assetData.first), assetData.second)));
        filesystem.unloadBlock(); // TODO : no automatic block managment, just unload it
        LOG_DBG("asset %s loaded", name.c_str());
    }

    template<class T>
    void AssetRegistry::unload(const std::string& name)
    {
        const AssetPair pair = { name, T::type::type };
        const auto it = _assets.find(pair);
        PANIC_ASSERT(it != _assets.end(), "attempting to unload asset %s that is not loaded", name.c_str());

        _assets.erase(it);
        LOG_DBG("asset %s unloaded", name.c_str());
    }
}