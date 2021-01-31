#include "readers.hpp"

#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int writeTexture(AssetBlock& block, const fs::path& path)
{
    int w, h, ch;

    // just check if it loads
    const int ret = stbi_info(path.string().c_str(), &w, &h, &ch);
    if (!ret)
        return -1;

    block.header.push_back(dab::AssetDecl{
        .name = path.stem().string(),
        .offset = static_cast<size_t>(block.file.tellp()),
        .type = dab::Asset_Texture });

    std::ifstream imgFile(path, std::ios_base::binary | std::ios_base::ate);
    const size_t size = imgFile.tellg();
    imgFile.seekg(0, std::ios_base::beg);

    block.file.write(reinterpret_cast<const char*>(&size), sizeof size);
    block.file << imgFile.rdbuf();

    return 0;
}