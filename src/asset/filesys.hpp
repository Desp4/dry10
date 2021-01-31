#pragma once

#include <span>
#include <filesystem>
#include <utility>

#include "dab/import.hpp"

#include "dbg/log.hpp"

namespace asset
{
    class Filesystem
    {
    public:
        Filesystem();
        Filesystem(const char* assetDir);
        Filesystem(std::span<const char* const> assetDirs);

        void addDirectory(const char* assetDir);
        void addDirectories(std::span<const char* const> assetDirs);

        template<class T>
        std::pair<T, std::string> loadAsset(const std::string& name);

        void loadBlock(const std::filesystem::path& blockName);
        void unloadBlock();

    private:
        struct AssetBlock
        {
            std::filesystem::path path;
            std::vector<dab::AssetDecl> assets;
        };

        std::string assetHash(const std::string& name, dab::AssetType type);

        std::vector<AssetBlock> _blocks;

        dab::BlockImporter _importer;
        decltype(_blocks)::const_iterator _currBlock;
    };



    template<class T>
    std::pair<T, std::string> Filesystem::loadAsset(const std::string& name)
    {
        auto searchLambda = [&name](const dab::AssetDecl& oth) { return oth.type == T::type && oth.name == name; };
        decltype(AssetBlock::assets)::const_iterator assetIt;
        if (_currBlock != _blocks.end())
        {
            assetIt = std::find_if(_currBlock->assets.begin(), _currBlock->assets.end(), searchLambda);
        }

        // NOTE : doing loading automatically, may delete it if not needed
        if (_currBlock == _blocks.end() || assetIt == _currBlock->assets.end())
        {
            LOG_WRN("asset %s not found in loaded block, checking others", name.c_str());
            for (auto p = _blocks.begin(); p != _blocks.end(); ++p)
            {
                // sure whatever
                if (p == _currBlock)
                    continue;

                assetIt = std::find_if(p->assets.begin(), p->assets.end(), searchLambda);
                if (assetIt != p->assets.end())
                {
                    _currBlock = p;
                    _importer.open(p->path);

                    LOG_DBG("loaded asset block %s", p->path.filename().string().c_str());
                    break;
                }
            }

            PANIC_ASSERT(assetIt != _blocks.back().assets.end(), "could not find asset %s", name.c_str());
        }

        return { _importer.read<T>(assetIt->offset), assetHash(name, T::type) };
    }
}
