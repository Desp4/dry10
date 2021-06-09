#include "dab.hpp"

#include <fstream>

#include <stb_image.h>

parsed_file parse_texture(const fs::path& path) {
    int w = 0, h = 0, ch = 0;
    // check if it loads
    if (!stbi_info(path.string().c_str(), &w, &h, &ch)) {
        throw std::runtime_error{ "Error reading texture " + path.string() };
    }

    std::ifstream tex_src{ path, std::ios_base::binary | std::ios_base::ate };

    byte_vector file_src(tex_src.tellg());
    tex_src.seekg(0, std::ios_base::beg);
    tex_src.read(reinterpret_cast<char*>(file_src.data()), file_src.size());

    parsed_file ret_file;
    ret_file.emplace_back(std::move(file_src), path.stem().string() + ".texture");
    return ret_file;
}