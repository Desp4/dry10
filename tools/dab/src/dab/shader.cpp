#include <filesystem>

#include "readers.hpp"
#include "share/shpb_type.hpp"

bool write_shader(asset_block& block, const fs::path& path) {
    // no checks yet
    std::ifstream shpb_file(path, std::ios_base::binary);
    char magic_buffer[sizeof shpb::FILE_MAGIC]{ 0 };

    shpb_file.read(magic_buffer, sizeof magic_buffer);
    if (memcmp(magic_buffer, shpb::FILE_MAGIC, sizeof shpb::FILE_MAGIC)) {
        return false;
    }

    block.header.push_back(dab::asset_decl{
        .name = path.stem().string(),
        .offset = static_cast<size_t>(block.file.tellp()),
        .type = dab::asset_type::shader
    });

    shpb_file.seekg(0, std::ios_base::end);
    const size_t size = shpb_file.tellg();
    shpb_file.seekg(0, std::ios_base::beg);

    block.file.write(reinterpret_cast<const char*>(&size), sizeof size);
    block.file << shpb_file.rdbuf();
    return true;
}