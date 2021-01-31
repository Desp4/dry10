#include <iostream>
#include <array>
#include <filesystem>

#include "readers.hpp"
#include "share/dab_type.hpp"

static AssetBlock _assetBlock;

using assetReader = int (*)(AssetBlock&, const fs::path&);
static constexpr std::array<assetReader, 3> _ASSET_FUNC{ writeShader, writeTexture, writeMesh };

static void abortFile(const char* name)
{
    _assetBlock.file.close();
    fs::remove(name);
}

static int writeAsset(const fs::path& path)
{
    for (const auto reader : _ASSET_FUNC)
    {
        if (!reader(_assetBlock, path))
            return 0;
    }

    return -1;
}

int main(int argc, char** argv)
{
    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))
    {
        std::cout <<
            "usage: dab { -d | -f } <args...> <output>\n" \
            "-d\targs are interpreted as directories(non-recursive)\n" \
            "-f\targs are interpreted as files\n";
        return 0;
    }
    if (argc < 4)
    {
        std::cerr << "invalid usage, type --help or -h for commands\n";
        return -1;
    }

    // mode flag, do both comparisons now, clearer to exit on failure just after
    const char mode = !strcmp(argv[1], "-f") | (!strcmp(argv[1], "-d") << 1);
    if (!mode)
    {
        std::cerr << "invalid usage, type --help or -h for commands\n";
        return -1;
    }

    argc -= 3;
    argv += 2;
    _assetBlock.file.open(argv[argc], std::ios_base::binary);
    if (!_assetBlock.file.is_open())
    {
        std::cerr << "could not open output file " << argv[argc] << '\n';
        return -1;
    }

    
    _assetBlock.file.write(dab::FILE_MAGIC, sizeof dab::FILE_MAGIC);
    _assetBlock.file.seekp(sizeof(size_t), std::ios_base::cur);

    if (mode & 0x1)
    {
        for (int i = 0; i < argc; ++i)
        {
            if (!std::filesystem::is_regular_file(argv[i]))
            {
                if (!std::filesystem::exists(argv[i]))
                    std::cerr << argv[i] << " does not exist\n";
                else
                    std::cerr << argv[i] << " is not a file\n";

                std::cerr << "aborting\n";
                return -1;
            }

            const int ret = writeAsset(argv[i]);
            if (ret)
            {
                std::cerr << argv[i] << " is not a recognized asset\naborting\n";

                abortFile(argv[argc]);
                return -1;
            }

            std::cout << "asset " << argv[i] << " added to block\n";
        }
    }
    else
    {
        for (int i = 0; i < argc; ++i)
        {
            if (!fs::is_directory(argv[i]))
            {
                std::cerr << argv[i] << " is not a valid directory\naborting\n";
                abortFile(argv[argc]);
                return -1;
            }

            for (const auto& entry : fs::directory_iterator(argv[i]))
            {
                if (!entry.is_regular_file())
                    continue;

                const int ret = writeAsset(entry.path());
                if (ret)
                {
                    std::cerr << entry.path() << " is not a recognized asset\naborting\n";

                    abortFile(argv[argc]);
                    return -1;
                }

                std::cout << "asset " << entry.path() << " added to block\n";
            }
        }
    }

    const size_t headerPos = _assetBlock.file.tellp();
    const uint32_t headerCount = _assetBlock.header.size();

    _assetBlock.file.write(reinterpret_cast<const char*>(&headerCount), sizeof headerCount);
    for (const auto& entry : _assetBlock.header)
    {
        const uint8_t nameLen = entry.name.size();
        _assetBlock.file.write(reinterpret_cast<const char*>(&entry.offset), sizeof entry.offset);
        _assetBlock.file.write(reinterpret_cast<const char*>(&entry.type), sizeof entry.type);
        _assetBlock.file.write(reinterpret_cast<const char*>(&nameLen), sizeof nameLen);
        _assetBlock.file.write(entry.name.c_str(), nameLen);
    }
    _assetBlock.file.seekp(sizeof dab::FILE_MAGIC, std::ios_base::beg);
    _assetBlock.file.write(reinterpret_cast<const char*>(&headerPos), sizeof headerPos);

    return 0;
}