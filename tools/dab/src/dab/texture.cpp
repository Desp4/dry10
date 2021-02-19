#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "readers.hpp"

bool write_texture(asset_block& block, const fs::path& path) {
    int w = 0, h = 0, ch = 0;
    // just check if it loads
    const int ret = stbi_info(path.string().c_str(), &w, &h, &ch);
    if (!ret) {
        return false;
    }

    block.header.push_back(dab::asset_decl{
        .name = path.stem().string(),
        .offset = static_cast<size_t>(block.file.tellp()),
        .type = dab::asset_type::texture
    });

    std::ifstream imgFile(path, std::ios_base::binary | std::ios_base::ate);
    const size_t size = imgFile.tellg();
    imgFile.seekg(0, std::ios_base::beg);

    block.file.write(reinterpret_cast<const char*>(&size), sizeof size);
    block.file << imgFile.rdbuf();
    return true;
}