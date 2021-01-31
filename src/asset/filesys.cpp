#include "filesys.hpp"

namespace asset
{
    Filesystem::Filesystem() :
        _currBlock(_blocks.end())
    {
    }

    Filesystem::Filesystem(const char* assetDir) :
        Filesystem({ &assetDir, 1 })
    {
    }

    Filesystem::Filesystem(std::span<const char* const> assetDirs) :
        Filesystem()
    {
        addDirectories(assetDirs);
    }

    void Filesystem::addDirectory(const char* assetDir)
    {
        addDirectories({ &assetDir, 1 });
    }

    void Filesystem::addDirectories(std::span<const char* const> assetDirs)
    {
        const int32_t currBlockAbs = _currBlock == _blocks.end() ? -1 : _currBlock - _blocks.begin();

        for (const char* dir : assetDirs)
        {
            for (const auto& file : std::filesystem::recursive_directory_iterator(dir))
            {
                if (file.is_directory())
                    continue;

                if (_importer.open(file.path()))
                {
                    _blocks.emplace_back(file.path(), _importer.assetDeclarations());

                    _importer.close();
                    // NOTE : path defaults to wchar_t on win, have to convert to string sadly
                    LOG_DBG("asset added %s", file.path().string().c_str());
                }
                else
                {
                    LOG_WRN("invalid asset type %s, skipping", file.path().string().c_str());
                }
            }
        }

        // to preserve correct iterator
        _currBlock = currBlockAbs == -1 ? _blocks.end() : _blocks.begin() + currBlockAbs;
    }

    void Filesystem::loadBlock(const std::filesystem::path& blockName)
    {
        const auto blockIt = std::find_if(_blocks.begin(), _blocks.end(),
            [&blockName](const AssetBlock& oth) { return oth.path == blockName; });

        PANIC_ASSERT(blockIt != _blocks.end(), "block name %s is not present", blockName.string().c_str());
        if (blockIt == _currBlock)
        {
            LOG_WRN("block %s already loaded", blockIt->path.filename().string().c_str());
            return;
        }

        PANIC_ASSERT(_importer.open(blockName), "could not open asset block %s", blockIt->path.string().c_str());

        _currBlock = blockIt;
        LOG_DBG("loaded asset block %s", blockIt->path.filename().string().c_str());
    }

    void Filesystem::unloadBlock()
    {
        if (_currBlock == _blocks.end())
        {
            LOG_WRN("no block to unload");
            return;
        }

        _importer.close();
        LOG_DBG("unloaded asset block %s", _currBlock->path.filename().string().c_str());
        _currBlock = _blocks.end();
    }

    std::string Filesystem::assetHash(const std::string& name, dab::AssetType type)
    {
        // '/' is append
        const std::filesystem::path path = "static" / _currBlock->path.filename() / std::to_string(type) / name;
        return path.string();
    }
}
