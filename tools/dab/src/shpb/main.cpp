#include <iostream>
#include <fstream>
#include <filesystem>

#include "share/shpb_type.hpp"

static shpb::shader_type arg_type(const char* str) noexcept {
    if (!strcmp(str, "-v"))
        return shpb::shader_type::vertex;
    if (!strcmp(str, "-f"))
        return shpb::shader_type::fragment;
    if (!strcmp(str, "-g"))
        return shpb::shader_type::geometry;
    if (!strcmp(str, "-c"))
        return shpb::shader_type::compute;
    return shpb::shader_type::none;
}

int main(int argc, char** argv) {
    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        std::cout <<
            "usage: shpb <files...> <out_file>\n" \
            "files\t{ -v | -f | -g | -c } <file>\n" \
            "-v\tvertex shader\n" \
            "-f\tfragment shader\n" \
            "-g\tgeometry shader\n" \
            "-c\tcompute shader\n";
        return 0;
    }

    if (argc < 4) {
        std::cerr << "invalid usage, type --help for details\n";
        return -1;
    }

    std::ofstream out_file(argv[--argc], std::ios_base::binary);
    if (!out_file.is_open()) {
        std::cerr << "could not open file " << argv[argc] << '\n';
        return -1;
    }

    out_file.write(shpb::FILE_MAGIC, sizeof shpb::FILE_MAGIC);
    out_file.seekp(sizeof(uint32_t), std::ios_base::cur);

    uint32_t i = 1;
    while (i < argc) {
        const shpb::shader_type type = arg_type(argv[i]);
        ++i;

        if (type == shpb::shader_type::none || i == argc) {
            std::cerr << "invalid usage, type --help for details\n";
            out_file.close();
            std::filesystem::remove(argv[argc]);
            return -1;
        }

        std::ifstream shader_file(argv[i], std::ios_base::binary | std::ios_base::ate);
        if (!shader_file.is_open()) {
            std::cerr << "could not open file " << argv[i] << '\n';
            out_file.close();
            std::filesystem::remove(argv[argc]);
            return -1;
        }

        const size_t size = shader_file.tellg();
        shader_file.seekg(0, std::ios_base::beg);

        out_file.write(reinterpret_cast<const char*>(&type), sizeof type);
        out_file.write(reinterpret_cast<const char*>(&size), sizeof size);
        out_file << shader_file.rdbuf();
        ++i;
    }

    i /= 2; // rounds down, alright

    out_file.seekp(sizeof shpb::FILE_MAGIC, std::ios_base::beg);
    out_file.write(reinterpret_cast<const char*>(&i), sizeof i);

    return 0;
}