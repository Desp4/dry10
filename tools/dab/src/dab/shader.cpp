#include "readers.hpp"

#include <filesystem>

#include "share/shpb_type.hpp"

int writeShader(AssetBlock& block, const fs::path& path)
{
    // no checks yet
    std::ifstream shpbFile(path, std::ios_base::binary);
    char magicBuffer[sizeof shpb::FILE_MAGIC]{ 0 };

    shpbFile.read(magicBuffer, sizeof magicBuffer);
    if (memcmp(magicBuffer, shpb::FILE_MAGIC, sizeof shpb::FILE_MAGIC))
        return -1;

    block.header.push_back(dab::AssetDecl{
        .name = path.stem().string(),
        .offset = static_cast<size_t>(block.file.tellp()),
        .type = dab::Asset_Shader });

    shpbFile.seekg(0, std::ios_base::end);
    const size_t size = shpbFile.tellg();
    shpbFile.seekg(0, std::ios_base::beg);

    block.file.write(reinterpret_cast<const char*>(&size), sizeof size);
    block.file << shpbFile.rdbuf();

    return 0;
}